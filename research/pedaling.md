# How pianists use the sustain (damper) pedal

Status: 2026-11-21. Research (§1–6), the planner built on it (`-pianist`, §7) and its validation
against the pedalling of performed MIDI files (§4.7, §8).

Marks: each claim carries its evidence. [paper] means a paper, thesis or primary document that was read (full text, or the abstract when that is said). [web] means a book quoted on the web, a teaching site, a catalogue or a forum. [web, snippet] means known only from a search-engine summary, not read. [memory] means general knowledge that still needs checking. [calc] means computed here: either arithmetic, or the analysis of the MAESTRO v3.0.0 MIDI files done for this note (section 4.3). Numbers in square brackets such as [12] point to the Sources list at the end. The note does not invent numbers. Where a number could not be found, it says so.

## 1. Short answer

- **The basic gesture is the legato ("syncopated") pedal.** The foot comes up when the new harmony is struck and goes down again a short moment later, so the old chord is damped and the new one is caught. Hofmann (1909): press "always immediately after--mark me, after--the striking of the keys, never simultaneously", and release "promptly and simultaneously with the striking of the next tone" [29] [paper]. The older "rhythmic"/"direct" pedal goes down with the note and is still used for accents, at phrase starts and after rests [36] [37].
- **Measured timing.** 1276 MAESTRO performances on a Disklavier, 198.5 h, about 480 000 pedal changes [5] [calc]:
  - The pedal comes up at the new onset: median +12 ms after the nearest chord onset, interquartile range (IQR) −18…+44 ms.
  - It goes down again a median 146 ms after that onset (IQR 86–257 ms).
  - The pedal stays up (below CC64 = 64) for a median 122 ms (10th–90th percentile 59–455 ms).
  - All three grow with the time to the next onset (IOI). For IOI < 0.2 s the pedal goes back down about 50 ms after the onset; for IOI 1.0–1.5 s about 300 ms after.
  - This matches Repp's Disklavier finding [2] [paper, abstract]: as tempo rises, releases and depressions come "a little sooner" and changes are "executed a little more quickly", but by proportionally less than the change in beat duration. So the timing is neither fixed in ms nor fixed as a fraction of the beat.
- **How often pianists change.** In MAESTRO the pedal is down 64 % of the time, with 45 pedal presses per minute and a median of 3 chord onsets per pedal. Both depend strongly on style. The pedal-down fraction is 0.22 for Bach, 0.27 for Haydn, 0.34 for Mozart, 0.53 for Beethoven, 0.64 for Schubert, 0.72 for Schumann and Brahms, 0.74 for Liszt, 0.80 for Chopin, 0.81 for Debussy, 0.82 for Rachmaninoff and 0.86 for Scriabin [calc].
  - When the pedal is down, the chance that a chord onset brings a change rises from 0.24 (next onset within 0.1 s) to about 0.65 (next onset ≥ 0.6 s away) [calc].
  - A low-register bass that moves by step forces a change more often (0.65) than a bass that stays (0.45) [calc].
- **Half pedal is real but a minority.** The CC64 value spends about 7 % of the time held at a partial level (16–63) for 300 ms or longer. About 24 % of pedal presses never pass CC 100 [calc]. Hu et al. put the "sound-off" (damper-contact) point at CC ≈ 60–76 on three corpora [14] [paper].
- **Acoustics behind the rules:**
  - Bass strings decay slowly, and their dense partials fall inside one critical band, so stepwise bass under one pedal beats and turns to "mud" [27] [21] [calc].
  - With the dampers up, the whole string register resonates in sympathy [21] [24] [paper].
  - The top ~1.5 octaves have no dampers at all, so the pedal there changes only resonance, not note length [24] [47] [paper, web].
- **For the planner** (section 6): change the pedal at harmony and bass changes, with up at the onset (0 to +40 ms) and down at onset + clamp(40 + 0.25·IOI, 50, 450) ms. Keep the pedal up for at least ~60–80 ms, and longer (≥ 120 ms) when bass notes have to be damped. Use direct pedal at starts and after rests. Pedal sparsely in Baroque and Classical textures and richly in Romantic, Impressionist and ballad textures. Use no pedal, or short pedals, under low stepwise bass and detached textures. With a binary damper, imitate half and flutter pedalling by quick extra changes, and keep a held bass by holding its key ("finger pedal") rather than by half-pedalling.

- **The pianists in this collection** (§4.7): ten performed files with on/off pedal release at the
  new onset (medians −8 … +24 ms) and press a median 92–146 ms after it, 31–111 presses a minute,
  and hold the pedal through most of the hands' short silences.
- **The planner** (§7) changes at the bass's and the harmony's changes and at revoiced chords,
  keeps the pedal up in counterpoint and in low stepwise runs, and uses MAESTRO's timing. Against
  the ten pianists (§8) it finds 72 % of their changes within 150 ms (Don't Explain: 79 %), 63 % of
  its own are theirs; Bach played on the piano gets the pedal 0–4 % of the time.

## 2. Pedal technique and its kinds

### 2.1 Direct (rhythmic) pedal

- The pedal goes down with the note (or chord) and comes up with or before the next one. In Rowland's history this "rhythmic pedalling technique", "in which the pedal was depressed and released as the note was played", is the older practice that syncopated pedalling later "supplanted" [36] [web, snippet].
- Czerny (Pianoforte-Schule Op. 500, 1839), as quoted by Poli [32] [paper, quote]: "The release and depression of the pedal shall be realized with extreme rapidity, so as not to leave any void or interstice between the chords; and must take place exactly with the first note of each chord [...] as if the pedal were held down without interruption."
  - Poli reads this as Czerny describing "what we identify today as syncopated pedal" [32] [paper]. The wording "exactly with the first note" sits between the two.
- Modern uses of direct ("straight") pedal: accents, loud chords, clear bass entries, two-note slurs, sforzandi and cadential chords in Classical music [37] [paper] [46] [web].
- MAESTRO [calc]: when the pedal goes down after it has been up for ≥ 1 s (a direct pedal at a start or after a dry passage), the press lands at a median +23 ms from the nearest onset (IQR −27…+82 ms, n = 46 921). That is "with the note", and about a quarter of these presses come before the onset (anticipatory, 2.3).

### 2.2 Legato (syncopated) pedal

- **The sequence** (Liang et al. [10] [paper]): the pedal is pressed while the fingers still hold chord 1. When the fingers leave, the pedal sustains chord 1. "Immediately after the [next] chord onset, the pedal is released to avoid blurring", and the dampers stop all strings whose keys are no longer held. Then "the pedal is pressed again to sustain" chord 2.
- **Why the foot lags.** Hofmann's reason [29] [paper]: "we must allow the damper to press upon the vibrating strings long enough to do its work. If, however, we tread down exactly with the finger-stroke we simply inhibit this stopping, because the damper in question is lifted again before it has had time to fall down."
- **History.** The first tutor devoted wholly to pedalling is Hans Schmitt, *Das Pedal des Claviers* (Vienna 1875), four lectures at the Vienna Conservatory. An earlier detailed account is in Louis Köhler's *Systematische Lehrmethode* (1857–8). Köhler wrote his own pedal tutor in 1882, and Anton Rubinstein published one in 1896 (German) and 1897 (English) [36] [web, chapter page].
- **Teaching definitions.** Al Bakri (2025) separates two pedals [37] [paper]:
  - "late" pedal: down just after the note, up just before the next one, so each new sound is clean;
  - "legato" pedal: down slightly after the note, up just after the next note sounds, so the two blend.
  The MAESTRO data (4.3) show both: in fast passages the release comes before the next onset, in slow ones after it.

### 2.3 Anticipatory (pre-) pedal

- The pedal goes down before the key, so the strings are already free when the hammer strikes. The result is a "richer" attack and the sympathetic halo from the first instant [37] [paper].
- Debussy advised performers "to depress the pedal before starting to play, so that the overtones would vibrate immediately upon contact" (via Metaxaki [35] [paper]).
- Pre-pedalling softens the opening of a phrase [46] [web].
- Neuhaus is cited in German pedagogy under the slogan "Dem Flügel die Poren öffnen!" ("open the pores of the piano") next to a section on the advantages of pedalling in advance [52] [web, snippet].
- MAESTRO [calc]: for direct pedals after ≥ 1 s without pedal, 25 % go down ≥ 27 ms before the nearest onset and 10 % go down ≥ 115 ms before it.

### 2.4 Half, quarter and fractional pedal

- **Depth.** Pianists colour the resonance with partial pedal. "Three or four levels are commonly defined" (quarter, half, three-quarter, full) [8] [paper].
- **What happens at the string.** Lehtonen, Askenfelt and Välimäki (2009) [22] [paper, abstract]: with part-pedalling a tone has three phases, "initial free vibration, damper-string interaction, and final free vibration".
  - During the interaction phase the decay is rapid, and "the timbre of the tone is affected by the nonlinear amplitude limitation of the string motion".
  - In the bass, this amplitude limitation moves energy from lower to higher partials [23] [web, snippet].
  - So the half pedal is a damping that depends on amplitude, not a fixed "half sustain". Loud low partials are clipped, and quiet residues ring on.
- **Two meanings of "half pedal" in teaching:**
  - holding the pedal part-way, so the dampers only graze the strings ("partial", "quarter", "eighth" pedal in Al Bakri [37] [paper]);
  - a half change: from full, the foot rises only to the middle and comes back down. This clears the upper, weaker sound while the strong bass survives [37] [web dictionary entries via search, snippet].
  Debussy scholarship uses the second sense for keeping long bass pedal points [35] [paper].
- **Schmitt (1875)**, as summarised by Metaxaki [35] [paper], says the pedal "must be partially released":
  1. with pedal points that the hand cannot sustain;
  2. to renew the tone;
  3. when the tone is to be "vibrated".
  He also says: "The foot must trill the pedal when a pedal point occurs in connection with rapid scales or ornaments".
- **Measured depth.** Liang et al. had a pianist play ten Chopin excerpts with experimenter-marked depths. The counts were 76 quarter, 224 half, 71 three-quarter and 167 full pedals (538 presses). An SVM recognised the four classes from the pedal sensor at micro-F1 0.93 [8] [paper]. These depths were imposed by the experimenter, so the counts say nothing about natural use.
- **Natural use in MAESTRO** [calc]: section 4.4.

### 2.5 Flutter (vibrato) pedal

- Rapid, shallow up-down movements of the foot. They are "similar to half-damping, which consists of very quick and light movements in order to reduce accumulating sound" [8] [paper].
- Al Bakri distinguishes [37] [paper]:
  - "vibrating" pedal: fully pressed and shaken;
  - "repetitive" pedal: rapid full exchanges, "not shaken", during trills, chord series or scale passages.
- No measured rate was found. Liang's detector merges onsets closer than a "fluttering threshold", but the paper does not give its value [8].

### 2.6 After-pedal (catching a note)

- After-pedal means taking the pedal after the hand has struck a note, typically a low bass note that must ring while the hand leaps away. It is the legato pedal applied to a single note.
- No source found here uses "after-pedal" as a named term. Banowetz's book is the likely place, but it was not readable [38].
- Hofmann's general rule covers the mechanism: the pedal must follow "immediately after the striking of the note for which it is intended" [29] [paper].
- Zhang et al. (2025) call the short, sharp press used "as catch pedal" a "pinnacle" gesture [15] [paper].
- The timing must fall between the hammer strike and the key release. For a leaping left hand that window can be 50–150 ms [memory]. In MAESTRO, the shortest IOI class (< 0.2 s) has a median down-lag of 53 ms [calc].

### 2.7 Pedal at rests

- Chopin's pedal marks often run over rests and leave gaps of about a sixteenth-note value before the next "Ped." [32] [paper].
- Poli, playing on an 1840 a period maker, found that the post-release blur on period pianos was slight. He often chose to make the rests audible on the modern piano [32] [paper].
- MAESTRO [calc]: at onsets after ≥ 0.5 s with no key held, the pedal was already up just before the onset in only 36 % of the cases. Pianists keep the pedal through most short silences, so the silence is filled by the resonance. When the pedal was up, it went down a median 97 ms after the onset (IQR 34–283 ms, n = 8918).

### 2.8 Soft pedal (una corda) and sostenuto in combination

- **Una corda on a grand** shifts the whole action so the hammers strike two of three strings, with a less-used part of the felt, which gives a duller tone [42] [web]. On most uprights it moves the hammers closer to the strings instead.
- Hofmann [29] [paper] warns against using either pedal to make up for touch: "the left pedal does not soften the tone without" changing its colour.
- **Combination with the sustain pedal is common.** Bernays and Traube (a reproducing grand, four pianists, five timbres) found:
  - sustain-pedal use differed between timbres: "dark", "round" and "velvety" got more of it (p < 0.01);
  - "velvety" combined heavy soft pedal with part-soft-pedalling;
  - one pianist never used the soft pedal [19] [paper].
- **Sostenuto** (middle pedal on grands) keeps the dampers up only for keys held at the moment it is pressed. It is used to hold a bass pedal point while the sustain pedal changes above it [memory].
  - Gieseking advised against using it in Debussy [35] [web, snippet in a 2019 paper; Metaxaki also discusses Gieseking].
  - For the planner, sostenuto is the clean way to get the "half-change keeps the bass" effect on a binary damper (rule P9).

## 3. Pedalling by era and composer

### 3.1 Baroque on the modern piano (Bach)

- **Schiff.** He now plays Bach with no pedal at all. His essay "Senza pedale ma con tanti colori" (Florence, 2012) says: "The pedal is to the piano as the vibrato is to string players", and "a discreet use of the pedal is not forbidden as long as these rules are observed... with care, control and in moderation" [41] [web, quotes].
- **Hofmann's middle position.** "Qualitatively speaking, the pedal is as necessary in Bach's music as in any other; quantitatively, I recommend the utmost caution in its use, so as not to blur the fine texture of his polyphony" [29] [paper]. He lists two uses: organ points, and harmonies the hand cannot span. In an interview section of the same book he rejects both extremes, playing Bach like an organ with constant pedal or like a clavichord with none [29] [paper].
- **Neuhaus** calls Baroque pedalling "very clever, careful and extremely economical" (quoted by Al Bakri [37] [paper]).
- **"Finger pedal".** In Baroque music the fingers hold notes over (legatissimo, overlapping key releases) instead of using the foot [memory]. No primary source using exactly this term was found. Hu et al. measure key-overlap ratios in Chopin, Mozart and Beethoven corpora and treat pedal sound-off and key overlap together [14] [paper].
- **MAESTRO Bach** (145 performances, 13.8 h) [calc]:
  - pedal down only 22 % of the time; 23 presses per minute (other composers 27–53);
  - short segments (median 0.44 s, 2 chord onsets per pedal);
  - longer up-gaps between pedals (median 217 ms, against 117 ms in Chopin);
  - a later press (median +160 ms after the onset, against +83 ms in Chopin).
  This is touch pedalling: short dabs that add resonance to single beats, not a sustain.

### 3.2 Classical: Mozart, Haydn, Beethoven

- **Mozart.** His solo piano works carry no pedal marks [memory]. Stein's pianos, from the one Mozart described in 1777 onwards, had a knee lever that lifted all dampers [51] [web].
  - Whether Mozart's own Walter fortepiano had knee levers in his lifetime is disputed. Latcham argued they "might have been added to the instrument after Mozart had died". Eva Badura-Skoda (2000) and Paul Badura-Skoda ("Mozart without the Pedal?", 2002) replied [43] [web, snippet].
  - Pianos of the period had knee levers for the damper lift and the moderator, a cloth strip between hammers and strings: a Stein of 1792 and a Dulcken of about 1795 [43] [web].
- **Haydn.** The "open Pedal" marking in the London-period C major sonata, Hob. XVI:50, is usually read as a long, deliberately blurred pedal on an English piano [memory, not verified here]. The search did not reach a source for it.
- **Beethoven:**
  - *Moonlight* Sonata, Op. 27/2, first movement: "Si deve suonare tutto questo pezzo delicatissimamente e senza sordino", i.e. without dampers [42] [web].
    - Readings range from holding the pedal throughout, to changing with the harmony on the modern piano, to Sakimura's view that "senza sordini" refers to the moderator [53] [42] [web].
    - Petzold, playing a period instrument with the pedal held throughout, says that on a modern piano the pianist "needs to pedal periodically or the mist will turn to a dense choking smog" [50] [web].
    - Wikipedia lists it as a piece some pianists play with half pedalling [42] [web].
  - *Tempest*, Op. 31/2: the first-movement recitatives are marked to be played under one long pedal ("a ghostly recitative set over a long, atmospheric pedal") [web, snippet]. Bar numbers and exact pedal spans [memory].
  - *Waldstein*, Op. 53, Rondo: long pedals that mix tonic and dominant. Al Bakri cites one pedal held for about 10 bars [37] [paper].
  - Schnabel's 1935 edition "never advocated anything other than literal adherence to Beethoven's pedal markings" in its footnotes [web, snippet].
  - Czerny's remark that Beethoven "used a lot of pedal, much more than is indicated in his works" is widely quoted [web, snippet]; the exact source was not verified.

### 3.3 Chopin

- **Notation.** Chopin wrote "Ped." for down and a crossed-circle sign for up, and closely followed each release by a new "Ped." [32] [paper]. Poli: "Accuracy seems not to have been one of Chopin's greatest concerns when it came to notating pedal". The marks vary between manuscripts with space, handwriting and figuration [32] [paper].
- **The Rosenblum debate.** Rosenblum ("Some Enigmas of Chopin's Pedal Indications", *Journal of Musicological Research*, 1996) suggests that the visual gaps between release and the next "Ped." reflect pianos of the 1830s–40s whose resonance did not stop at once on release [33] via [32] [paper].
  - Poli counters with his a period maker experiments and with Czerny's 1839 instruction for gapless changes [32] [paper].
  - Eigeldinger reads irregular marks in the D-flat Prelude, Op. 28/15, as deliberate variety. Poli shows that on an 1840 a period maker "the sound clears more rapidly", while on the modern piano the same pedal "blurs the harmony" [32] [paper].
- **Discretion.** Marmontel: Chopin "used the pedals with marvelous discretion". Chopin: "The correct employment of the pedal remains a study for life" [46] [web, secondary].
- **How literal.** The modern consensus in these sources: the marks show where Chopin wanted resonance and the harmonic span of a pedal. On a modern piano with longer sustain, pianists keep the starts and change more often inside long marks [32] [paper] [memory].
- **MAESTRO Chopin** (201 performances, 26.2 h) [calc]:
  - pedal down 80 % of the time; 51 presses per minute;
  - median up-gap 117 ms (10th–90th percentile 59–316 ms);
  - down at a median +83 ms after the onset;
  - segments of median 0.65 s with 4 chord onsets.

### 3.4 Liszt and Schumann

- **Liszt** is quoted (German pedagogy, secondary): "Ohne Pedal ist das Klavier nur eine Art Hackbrett" ("without pedal the piano is only a kind of dulcimer") [52] [web, snippet].
- MAESTRO Liszt [calc]: pedal down 74 %; the earliest re-press of all composers (median +62 ms); long segments (median 0.64 s, 90th percentile 2.07 s).
  - Changes at treble-only melodic steps are rare: P = 0.30 that a treble step with an unchanged bass brings a change.
  - Liszt's textures put big resonance under fast figuration and clear the pedal mainly at the bass.
- MAESTRO Schumann [calc]: pedal down 72 %; 53 presses per minute (the most frequent changer); median up-gap 118 ms.

### 3.5 Debussy and Ravel

- **Few marks.** Debussy and Scriabin "rarely notated pedalling" [8] [paper].
- Debussy said pedalling "cannot be written down" because "it varies from one instrument to another, from one room, or one hall, to another" [35] [paper, quoting Long].
- **Implicit marks.** Long bass notes, ties over bar lines and notes that the fingers cannot hold are the usual implicit pedal indications. Pedal points appear in "almost half" of the bars of *La cathédrale engloutie* [35] [paper].
- **Welte rolls.** In Debussy's own Welte-Mignon roll of *La cathédrale engloutie*, "out of the total of 89 bars, there is not a single bar that is not pedalled". Metaxaki explains this partly by the Welte's inability to record fractional pedalling, which it registers as continuous pedal [35] [paper]. Roll data therefore overstate continuous pedal.
- **Instruments.** Debussy's pianos (period German, another maker, a period maker, Érard) had less sustain and a lighter bass than a modern concert grand, so "long pedals must be understood as half" pedals. Small pedal changes "clear up the sound without loosing the long-held bass" [35] [paper].
- **Gieseking.** Dean Elder's chapter "Gieseking's Pedalling in Debussy and Ravel" is in the Banowetz book [38] [web, table of contents]; it was not read.
- MAESTRO Debussy [calc]: pedal down 81 %; the longest segments of the core composers (median 0.79 s, 90th percentile 3.1 s).
  - With an unchanged bass, the change probability is only 0.19 (same bass) and 0.23–0.30 (treble-only motion). This is the "wash".
  - Rachmaninoff (median segment 0.85 s) and Scriabin (0.84 s) behave the same way [calc].

### 3.6 Satie

- In *Gymnopédie* No. 1, "Pedalling goes with the bass notes, which should be sustained through until the next bass note", because "the chords tend to stay the same until the bass note changes" [44] [web].
- Satie's original prints carry no pedal marks [memory]. The piece is the textbook case of one pedal per bar (one bass and chord per bar at a slow 3/4).
- As a planner rule: one pedal per bass note, up at the next bass onset, down ~150–300 ms later at this slow tempo (IOI ≈ 0.7–1 s gives a lag of about 175–240 ms, section 4.3) [calc].

### 3.7 Jazz and pop ballads

- **Change on the chord, not on the melody.** The pedal follows the harmony: "always lift the pedal when transitioning between chords to avoid a muddy sound"; lift "right before changing chords" [45] [web].
- **More changes under an active melody.** Change "more frequently when you're also playing a melodic line", and with a slow melody you may change before the harmony changes "so the melody doesn't get too blurry" [45] [web].
- **Shell voicings.** With shell or spread voicings held in the left hand, the pedal can come up without losing the chord, so comping pianists pedal less and hold more with the fingers [45] [web].
- **Stride and boogie.** In boogie-woogie "the last thing you want to do is put the pedal down", except for effect [45] [web]. Stride left hands (bass on 1 and 3, chord on 2 and 4) are played mostly dry or with short direct pedals on the bass notes [memory].
- **Bill Evans.** No sourced numbers were found. His ballad style is commonly described as rich legato pedalling that changes with the (often rootless) left-hand voicings, sometimes with half-changes to keep a low root ringing [memory].

## 4. Measured pedalling in performance

### 4.1 Early studies: Heinlein (1929–1930)

- C. P. Heinlein published three papers in the *Journal of General Psychology* [4] [paper, citation]:
  - "A Discussion of the Nature of Pianoforte Damper-Pedalling Together with an Experimental Study of Some Individual Differences in Pedal Performance", 2:489–508 (1929);
  - "The Functional Rôle of Finger Touch and Damper-Pedalling in the Appreciation of Pianoforte Music", 2:462–469 (1929);
  - "Pianoforte Damper-Pedalling under Ten Different Experimental Conditions", 3:511–528 (1930).
- Method [web, snippet]: Duo-Art rolls of four pianists playing Schumann's *Träumerei*, and a kymograph connected to the pedal mechanism.
- Findings [web, snippet]: the patterns were "highly idiosyncratic". The 1930 paper concludes that a trained pianist's pedalling is largely an automatic response to fingering, phrasing, tempo and dynamics, and that changing one of these alters it.
- No numbers from Heinlein were obtained. The full texts are paywalled.

### 4.2 Repp (1994–1997)

- **Repp 1994** (relational invariance across tempo; ASA 1993 abstract): "chord asynchronies, tone overlaps, and possibly pedal timing" were "unaffected by tempo", while expressive timing scaled with tempo [3] [paper, abstract].
- **Repp 1996**, *Psychology of Music* 24:199–221 [1] [paper, abstract]:
  - two pianists, nine performances each of *Träumerei* (three at each of three tempi), on an electronic instrument;
  - neither absolute nor relative invariance of release, onset or change times was found consistently;
  - pedal timing "is sensitive to local and global tempo changes in varying degrees, yet exhibits consistency across repeated performances by the same pianist";
  - "striking differences in pedal timing between the two pianists, who differed in level of skill".
- **Repp 1997**, *Psychological Research* 60:164–172 [2] [paper, abstract]:
  - ten pianists, two excerpts "that required repeated use of the damper pedal to connect successive chords", three tempi, one maker Disklavier;
  - "As the tempo increases, both pedal releases and depressions usually occur a little sooner and pedal changes (release-depression sequences) are executed a little more quickly, but these effects are proportionally smaller than the changes in manual (and pedal) period duration."
- **The ms values in Repp's papers could not be obtained.** Both journals are paywalled, and no Haskins preprint was found. The MAESTRO analysis below replaces them for the planner. It agrees with Repp's qualitative result: lag and gap grow with IOI, but by less than proportionally.

### 4.3 MAESTRO v3.0.0: analysis done for this note [calc]

**Data.** MAESTRO v3.0.0 [5]: 1276 performances, 198.5 h, from the International Piano-e-Competition, recorded on one maker Disklavier grands with key and CC64 pedal capture. The MIDI files were parsed with a small parser of our own.

**Method:**
- **Pedal state:** CC64 ≥ 64 counts as down (the General MIDI convention; Kong et al. [7] and Liang et al. [9] [10] use the same threshold).
- **Chord onsets:** note-ons within 35 ms are merged into one chord onset.
- **Pedal change:** a release (crossing below 64) followed by a press within 1 s.
- **Two anchorings:**
  - (a) the chord onset nearest the release time (the onset that caused the change);
  - (b) the last chord onset before the re-press, used for the IOI tables.
- **Caveats:**
  - The Disklavier CC stream has about 20 ms steps during motion: gaps cluster at 59, 79, 99… ms. The median file has 12 CC64 messages per second.
  - MAESTRO does not identify the performers.
  - Hu et al. estimate the real damper-contact threshold at CC 60–76 on other corpora [14]; the sensitivity to the threshold is shown below.

**Core timing (anchoring a, threshold 64, n = 482 593 changes):**

| quantity | p10 | p25 | median | p75 | p90 |
|---|---|---|---|---|---|
| release relative to nearest onset (ms) | −47 | −18 | +12 | +44 | +89 |
| re-press relative to that onset (ms) | 48 | 86 | 146 | 257 | 482 |
| time pedal is up (ms) | 59 | 81 | 122 | 219 | 455 |

**Sensitivity to the threshold** (medians): at CC 40, up-gap 100 ms and re-press +126 ms; at CC 90, up-gap 189 ms and re-press +197 ms. The release time stays within ±15 ms of the onset at every threshold. The re-press moves by about ±50 ms, because a slow press takes 50–100 ms to travel from CC 40 to CC 90.

**Dependence on tempo** (IOI = time from the chord onset to the next chord onset; anchoring b; medians with IQR):

| IOI (s) | n | re-press after onset (ms) | up-gap (ms) | release vs onset (ms) |
|---|---|---|---|---|
| < 0.20 | 217 520 | 53 (26–87) | 111 (79–180) | −55 (−137…−9) |
| 0.20–0.35 | 115 218 | 102 (55–161) | 120 (81–197) | −14 (−86…+32) |
| 0.35–0.50 | 60 277 | 130 (76–198) | 138 (99–217) | +2 (−57…+48) |
| 0.50–0.75 | 44 511 | 176 (116–245) | 158 (109–236) | +25 (−32…+73) |
| 0.75–1.00 | 18 726 | 237 (163–323) | 188 (137–281) | +49 (−12…+105) |
| 1.00–1.50 | 13 938 | 315 (223–428) | 235 (158–357) | +76 (+13…+143) |
| 1.50–3.00 | 7 962 | 436 (301–611) | 326 (208–505) | +104 (+33…+182) |

How to read the table:
- **Fast music** (IOI < 0.35 s): the release comes just before or at the onset, and the re-press 50–100 ms after it.
- **Slow music:** the release comes 50–100 ms after the new onset. This is the true syncopated overlap that Hofmann and Czerny describe ("release promptly and simultaneously with the striking of the next tone" [29]). The re-press comes 250–450 ms after the onset.
- The re-press lag grows less than proportionally with IOI: about 0.5 × IOI at 0.1 s, but 0.25 × IOI at 1.25 s and 0.2 × IOI at 2.2 s. This matches Repp [2].
- In the shortest IOI class the "re-press after onset" is bounded by the IOI itself, because of the anchoring, so it is biased low.
- **Fit for the planner:** re-press lag ≈ 40 ms + 0.25 × IOI, clamped to 50–450 ms; up-gap ≈ 100 ms + 0.1 × IOI, with a minimum of 60 ms.

**How often the pedal changes, by IOI.** Given that the pedal is down 150 ms before a chord onset, the probability that it is released within min(300 ms, 0.8 × IOI) of that onset:

| IOI (s) | 0–0.1 | 0.1–0.2 | 0.2–0.3 | 0.3–0.4 | 0.4–0.5 | 0.5–0.6 | 0.6–0.8 | 0.8–2.0 |
|---|---|---|---|---|---|---|---|---|
| P(change) | 0.24 | 0.34 | 0.37 | 0.42 | 0.51 | 0.57 | 0.60–0.63 | 0.64–0.68 |

Fast notes (ornaments, runs, broken chords) are taken under one pedal. From IOI ≈ 0.5 s up, about two thirds of the onsets get a change, which in practice means every harmony change.

**Bass motion.** Same condition (pedal down before the onset); the release is counted within −250…+300 ms of the onset:

| context | all | Bach | Mozart | Beethoven | Chopin | Liszt | Debussy |
|---|---|---|---|---|---|---|---|
| bass < G3 unchanged | 0.45 | 0.85 | 0.50 | 0.50 | 0.43 | 0.41 | 0.19 |
| bass < G3 moves by 1–2 semitones | 0.65 | 0.85 | 0.85 | 0.70 | 0.59 | 0.58 | 0.43 |
| bass < G3 leaps ≥ 3 semitones | 0.56 | 0.78 | 0.79 | 0.56 | 0.56 | 0.48 | 0.44 |
| whole chord ≥ C4, top/bass moves by step | 0.45 | 0.67 | 0.73 | 0.52 | 0.45 | 0.30 | 0.24 |

A stepwise low bass produces the most changes in every Romantic and Impressionist group: a 0.15–0.24 rise over an unchanged bass. In Bach and Mozart the pedal is changed (or released) at nearly every onset anyway.

**Per-composer summary:**

| composer | h | pedal-down fraction | presses/min | onsets per press | median segment (s) | median up-gap (ms) | median re-press after onset (ms) |
|---|---|---|---|---|---|---|---|
| Bach | 13.8 | 0.22 | 23 | 12.7 | 0.44 | 217 | 160 |
| Scarlatti | 1.7 | 0.37 | 42 | 8.3 | 0.45 | 228 | 111 |
| Haydn | 4.7 | 0.27 | 27 | 12.5 | 0.44 | 238 | 91 |
| Mozart | 5.6 | 0.34 | 36 | 9.2 | 0.44 | 198 | 104 |
| Beethoven | 26.9 | 0.53 | 44 | 7.9 | 0.47 | 157 | 89 |
| Schubert | 35.8 | 0.64 | 51 | 5.9 | 0.53 | 138 | 91 |
| Chopin | 26.2 | 0.80 | 51 | 6.9 | 0.65 | 117 | 83 |
| Schumann | 15.7 | 0.72 | 53 | 6.0 | 0.52 | 118 | 81 |
| Liszt | 20.3 | 0.74 | 44 | 9.1 | 0.64 | 109 | 62 |
| Brahms | 8.1 | 0.72 | 49 | 5.9 | 0.64 | 118 | 100 |
| Debussy | 6.0 | 0.81 | 33 | 10.1 | 0.79 | 118 | 81 |
| Scriabin | 5.3 | 0.86 | 39 | 8.2 | 0.84 | 100 | 86 |
| Rachmaninoff | 6.7 | 0.82 | 39 | 8.3 | 0.85 | 99 | 82 |
| all | 198.5 | 0.64 | 45 | 7.4 | 0.56 | 121 | 87 |

("Median re-press after onset" here uses anchoring b, so it is a little smaller than the +146 ms under anchoring a.)

The segment lengths agree with Liang et al., who cut pedalled segments from 1392 e-competition MIDI files: their "durations distribute between 0.3 and 2.3 seconds" [9] [paper]. Ours have p10 = 0.22 s and p90 = 1.72 s.

### 4.4 Pedal depth

- **Time spent at each CC64 value** (all MAESTRO, bins of 8) [calc]:
  - 0–7: 23.4 %; 8–15: 0.4 %; 16–63: 12.5 % in total (rising from 0.7 % to 4.4 % per bin); 64–119: 23.5 %; 120–127: 40.2 %.
  - Much of the 16–63 time is foot travel. Holding a partial level (16–63 for ≥ 300 ms) takes 6.9 % of the time.
- **Peak depth of each press:** < 100 in 24.0 % of presses, < 115 in 30.3 %, ≥ 120 in 68.3 %. About a quarter of all presses are shallow (half or three-quarter) [calc].
- **Partial-depth time by composer** (fraction of total time at CC 16–63, including transitions) [calc]: Bach 0.22, Mozart 0.20, Haydn 0.17, Beethoven 0.17, Schubert 0.14, Brahms 0.11, Schumann 0.10, Chopin 0.08, Liszt 0.085, Rachmaninoff 0.07, Debussy 0.067, Scriabin 0.06.
  - Shallow pedal, relative to the total pedal use, is most frequent in Baroque and Classical repertoire: light "colour" pedal.
  - This partly contradicts the textbook picture of Debussy as the half-pedal composer. The Debussy players in MAESTRO use the full pedal most of the time and change it less often.
- **Sound-off points** (Hu et al. [14] [paper]): Vienna 4×22 at CC 76 (another maker, which rests at about CC 15), Batik at 75, Magaloff at 60. Each is estimated as the mode of the smoothed CC histogram in 50–100, the value where "Performers briefly hold the pedal at the damping point before re-pressing it". The damper meets the strings in the upper half of the MIDI range, not at 64 exactly.
- **Gesture shapes** (Zhang et al. [15] [paper], MAESTRO):
  - pinnacle: short, peaked; brief accents or catch pedal;
  - hill: short, partial;
  - highland: long plateau, "common in large Romantic piano repertoires";
  - mountain: long with modulation, "typical in Impressionist compositions".
  Gesture durations have median 70 frames and mean 147.8 frames. The companion paper by the same authors [16] uses 100 frames/s, which would give a median of 0.7 s, but the frame rate of the gesture statistic is not stated, so treat this as uncertain. The ratio of time near maximum depth has median 0.69.

### 4.5 Differences between pianists

- Repp 1996: "striking differences" between a more and a less skilled pianist, with consistency within each pianist [1].
- Heinlein: "highly idiosyncratic" patterns [4] [web, snippet].
- Bernays and Traube: one of four pianists used the soft pedal massively, one never; sustain-pedal use also differed by pianist [19] [paper].
- Lipke-Perry et al. (nine pianists, Bartók) found that pedalling changed with the pianist's focus of attention. The most experienced pianists and the least experienced one were the most consistent [20] [paper, abstract].
- MAESTRO, per performance (performers not identified) [calc]:
  - median up-gap: 10th–90th percentile across files 97–241 ms (median 126 ms);
  - median re-press lag: 100–268 ms (median 146 ms);
  - pedal-down fraction: 0.18–0.89 (median 0.68).
  The spread between performances of the same style is about a factor of two in timing.

### 4.6 Pedal from audio, and generating pedal from notes

- **Transcription.** Kong et al. transcribe pedal on/off from audio on MAESTRO with event F1 = 91.86 % at a 50 ms onset tolerance (86.58 % with onset and offset). They note that "the press of a sustain pedal is usually before the press of piano notes" and detect pedal onsets frame-wise for that reason [7] [paper]. That observation fits direct and anticipatory pedals but not the overall MAESTRO median (+146 ms after the onset).
- **Depth estimation.** Continuous depth from audio plus MIDI reaches binary F1 0.937 and 4-class F1 0.755 on MAESTRO [15] [paper]. Reverberation biases audio-only estimates upward [16] [paper].
- **Generating pedal from the score is unsolved.**
  - DExter (2024): "both models with sustain pedal prediction did a poor job in mimicking human pedaling techniques, with almost no time-wise correlation and on average one standard deviation away from the ground truth" [17] [paper].
  - Worrall, Yin and Collins (IEEE TAI 2024) predict a beat-wise pedal value (0–3) from notes only (CFE+P). A listener comment on a human performance in their study: "rather muffled with too much pedal" [18] [paper].
  - VirtuosoNet also predicts pedal [18] [paper].
  - Simple rule-based planners are therefore not behind the state of the art.
- **Liang et al. (QMUL).** The detection cues are sympathetic resonance (EUSIPCO 2018: "a legato-pedal onset" co-excites the strings of the previous chord [10] [paper]) and CNN features (ICASSP 2019, IJCNN 2019 [9] [13]). The thesis [12] and the JAES 2018 paper [11] could not be fetched, so their measured depth and timing statistics are missing from this note.

### 4.7 The performed files in this collection [calc]

- **What is there.** 311 of the 357 MIDI files under `~/Music/midi` and `~/midi` carry CC64; 300 of them
  have 100 pedal events or more. Nearly all are Doug McKenzie's performed solo-piano recordings in
  `~/Music/midi/bushgrafts` (jazz ballads and standards, a few arrangements of Chopin, Bach,
  Dvořák), recorded from a keyboard in real time, the pedal on/off (CC64 0/127; a few files use 8
  values). `franz_schubert_piano_track07.cdda.mid` is a transcription of a classical recording
  with on/off pedal. `~/midi` holds 4 files with CC64; its Bach is organ music (quantized, no pedal).
- **Method** (as 4.3: onsets within 35 ms merged; the file's tempo map, which in these real-time
  files is a fixed grid with no relation to the music's beat). "Up vs onset" is the nearest onset
  minus the release (positive: the release before the onset); "down after onset" is the press
  minus the last onset before it; "up time" is a release to the next press.

| file | presses | per min | down share | up vs onset p10/50/90 (ms) | down after onset p10/50/90 (ms) | up time p10/50/90 (ms) |
|---|---|---|---|---|---|---|
| Don't Explain (solo) | 219 | 56 | 0.65 | −8 / 24 / 62 | 43 / 121 / 224 | 146 / 211 / 705 |
| I Remember You (solo) | 813 | 70 | 0.49 | −18 / 6 / 50 | 33 / 113 / 210 | 106 / 165 / 429 |
| Mood Indigo (solo) | 490 | 111 | 0.62 | −33 / 5 / 54 | 21 / 92 / 217 | 92 / 146 / 367 |
| Day Dream | 421 | 55 | 0.78 | −47 / −6 / 40 | 27 / 140 / 267 | 87 / 167 / 433 |
| Peace Piece | 76 | 31 | 0.88 | −37 / 0 / 32 | 52 / 146 / 218 | 120 / 167 / 229 |
| Nocturne in E-flat (Chopin, arr.) | 303 | 46 | 0.63 | −22 / 23 / 91 | 23 / 126 / 303 | 147 / 226 / 1051 |
| Bach Prelude No. 1 (Ave Maria, arr.) | 293 | 37 | 0.62 | −39 / 5 / 60 | 28 / 128 / 271 | 110 / 231 / 1573 |
| It's the Talk of the Town (solo) | 335 | 69 | 0.74 | −47 / 0 / 46 | 32 / 115 / 229 | 99 / 177 / 391 |
| Some Other Time | 317 | 45 | 0.83 | −42 / −8 / 17 | 33 / 125 / 209 | 83 / 142 / 442 |
| Schubert (track 7, transcribed) | 362 | 45 | 0.76 | −22 / 15 / 61 | 32 / 97 / 245 | 109 / 168 / 499 |

- **Release at the onset, press ~120 ms after it.** In every file the release falls within
  ±25 ms of the new onset (medians −8 … +24 ms, i.e. at or just before it), and the press a median
  92–146 ms after the last onset. That is MAESTRO's picture at the IOIs of a ballad (4.3: +130 …
  +176 ms for IOI 0.35–0.75 s), except that these pianists release a little before the onset even
  in slow music, where MAESTRO's competition pianists release 25–100 ms after it.
- **The pedal is held through the hands' silences.** Where every key is up for 0.25–0.5 s the
  pedal is down in 70–89 % of the cases, for 0.5–1 s in 36–100 % (Don't Explain 94 %): the pedal
  is what joins the hands' leaps (MAESTRO: 64 %, 2.7). A rest alone is no reason to lift.
- **The bass is caught.** At 67–85 % of the presses the lowest note struck since the release is
  still held when the pedal goes down; the rest are short bass notes (p10 of the bass's length
  51–126 ms) the pianist let go early, the pedal catching their tails.
- **Up time.** 10–21 % of the time goes to the changes' short lifts (< 0.3 s); longer lifts
  (0.3–3 s) take another 10–25 %, the dry passages (in I Remember You, 24 % of the time in lifts
  over 3 s: up-tempo choruses played dry).
- **Inter-pianist ceiling.** No file is played twice by two pianists here, so the agreement of two
  humans could not be measured; Repp and Heinlein (4.1, 4.2) found pedalling consistent within a
  pianist and idiosyncratic between pianists.

## 5. The acoustics behind the rules

### 5.1 What the pedal does

- With the pedal down, all dampers are raised. All strings are free to vibrate in sympathy with what is played, coupled through the bridge and soundboard [8] [24] [paper].
- **Lehtonen et al. (JASA 2007)** [21] [paper, abstract]:
  - the pedal increases the decay time of partials "in the middle range of the keyboard, but this effect is not observed in the case of the bass and treble tones";
  - it "introduces interesting distortions of the two-stage decay";
  - "the amount of sympathetic vibrations is increased" when the string register is free;
  - their synthesis models the string register with 12 string models tuned to the lowest tones.
  - Zambon, Lehtonen and Bank (DAFx 2008) instead use parallel second-order filters calibrated from soundboard and string-register impulse responses [28] [web, snippet].
- **Schutz et al. (AES 2008)** [24] [paper]:
  - with and without pedal, the attack is "undistinguishable below a time interval shorter than 100ms". After that, decay times change and beating appears.
  - The pedal adds a sustained noise floor of resonance.
  - In the top two octaves, which have no dampers, "the note does not last longer with or without the pedal, but a natural reverberation due to the resonance of the sound board appears".
- **Undamped treble.** The undamped section starts around F♯6 or G6, typically the top ~18–22 notes ("the top octave and a half"), and varies with the piano [47] [web].

### 5.2 Why stepwise bass under one pedal makes mud

- **Critical bands.** Plomp and Levelt (1965): two simple tones sound consonant when their frequency difference exceeds the critical bandwidth, and "the most dissonant intervals correspond with frequency differences of about a quarter of this bandwidth" [27] [paper]. Below about 500 Hz the critical band is roughly 100 Hz wide [memory: Zwicker's Bark scale]. At the low end it can exceed a major third [48] [web].
- **Bass fundamentals** [calc]:
  - C2 = 65.4 Hz, D♭2 = 69.3 Hz, D2 = 73.4 Hz.
  - A whole step at C2 is 8 Hz apart, a major third (C2–E2 = 82.4 Hz) 17 Hz. Both are inside one band, at or below the most-dissonant spacing (¼ × ~100 Hz ≈ 25 Hz).
  - The fundamentals beat at 4–8 Hz, which is heard as roughness or wobble.
- **Bass partials** [calc]: the harmonics of two adjacent bass notes interleave densely. Harmonics 1–10 of C2 and D2 are at most 80 Hz apart below 750 Hz. Many pairs fall inside one critical band, so the roughness covers the whole spectrum that carries the bass timbre.
- **The same step in the treble** [calc]: at C6 = 1047 Hz a whole step is 128 Hz. That is close to the full critical band there (~160 Hz at 1 kHz [memory]) and far above the most-dissonant ¼-band spacing (~40 Hz), so it is much less rough. The upper partials also lie above the hearing-relevant range.
- **Duration.** Bass tones sustain much longer than treble tones:
  - Weinreich: the "prompt sound" decays at about 8 dB/s and the "aftersound" at less than a quarter of that rate [26] [paper]. That figure is for E♭4 = 311 Hz.
  - With an aftersound of ≈ 2 dB/s, a 60 dB fall takes about 30 s [calc].
  - Bass tones last tens of seconds; the highest damped treble tones last a few seconds [memory: Martin 1947, JASA 19; not read].
  - So a bass note under one pedal is still loud when the next bass note arrives, while a treble passing note has largely died away.
- **Sympathetic resonance adds to the mix.** With the dampers up, every undamped string whose partials coincide with the played partials is excited. Low strings have many partials, so they respond to almost anything [21] [paper].
- **Hofmann's practical version of the rule** [29] [paper]: blending "is especially permissible when the passing (foreign) tones are more than one octave removed from the lowest tone and from the harmony built upon it".
- **Dynamic layering.** Hofmann adds that a ruling harmony can survive passing chords under one pedal if it is played louder, so the weaker chords "die of their own weakness" [29] [paper].
- **Planner implication:** passing notes are tolerable when they are high, short, soft and far above the bass. Low, loud, long, stepwise notes are not.

### 5.3 Why the high register can take more

- Short decays [memory, 5.2]. No dampers at all in the top ~1.5 octaves [24] [47], so the pedal there only adds resonance.
- Partials are widely spaced relative to the critical band [calc, 5.2].
- MAESTRO agrees: changes at treble-only steps (whole chord ≥ C4) are rare in Liszt (0.30), Debussy (0.24) and Scriabin (0.22) [calc].

### 5.4 Tone colour: the halo

- Hofmann: the pedal is "a means of colouring--and pre-eminently that". It works like orchestral horns that "support sustained harmonies" and give "a glazing, a binding, a unifying" of colours. Releasing it gives "a certain soberness of tone" [29] [paper].
- Rachmaninoff (in Cooke, 1917): "The pedal has been called the soul of the piano"; "The pedal is the study of a lifetime. It is the most difficult branch of higher pianoforte study"; and rules "may often be skilfully broken" [30] [paper].
  - The "soul of the piano" saying is usually given to Anton Rubinstein [8] [37] [paper]. One blog gives it to Arthur Rubinstein instead, with no source [46] [web]. Rachmaninoff mentions it right before praising Anton Rubinstein's pedal.
- Leschetizky's pupils were known for "inaudible pedalling", and his teaching stressed "the necessity of an immaculate pedalling" (Hullah 1906 [31] [paper]).
- The halo is what Debussy's "vibrations in the air" and pre-pedal ask for: sympathetic partials ringing from the attack onwards [35] [paper].
- Physically it is (a) the sympathetic response of the string register [21], and (b) the change of the two-stage decay of the played notes, which is audible after about 100 ms [24] [26] [paper]. The decay-timbre detail of Askenfelt and Weinreich beyond what is quoted here was not fetched.

### 5.5 Damper mechanics relevant to timing

- **Askenfelt and Jansson**, staccato note on a grand [25] [paper]:
  - the key lifts the damper "plenty of time (15 ms) before the hammer strikes the string";
  - the damper "falls down on the string approximately 80 ms after hammer-string contact" after an immediate release;
  - a staccato note lasted about 100 ms.
- So a key-driven damper cycle takes tens of ms. The pedal lifts all dampers through the damper tray, and its re-engagement time depends on the foot.
- **Release time.** At the MAESTRO median, the foot keeps the pedal below CC 64 for about 120 ms per change. The 10th percentile, 59 ms, is about the shortest change that still damps [calc].
- **Pedal travel.** About 21–25 mm at the pedal (13/16" on one 211 cm grand; 29/32" on another piano), as measured by a technician on 9 pianos [49] [web, forum].
- **Half-pedal point.** On a Disklavier this is about CC 60–76 [14] [paper]. One maker's patents define a "half pedal point" stroke but do not give a value [web, snippet].

## 6. Rules for a planner working from MIDI notes

These rules assume the model has a binary damper (on/off, ~50 ms felt re-engagement, no half pedal). Each rule names its sources. Times are in ms; IOI is the time from a chord onset to the next chord onset (onsets within 35 ms merged).

### P1. Where to change (harmony and bass)

- Change at every onset where the harmony changes. Proxies from notes:
  - the lowest sounding pitch (bass) changes;
  - or the pitch-class set of the notes sounding in the next ~100 ms has two or more pitch classes not in the current pedal's set, other than passing tones.
- Do not change at onsets that only add notes of the current chord (arpeggios, broken chords, melody chord tones).
- Evidence: the Hofmann rule ("this alternation must be kept up where there is either a change of harmony or a succession of 'passing notes'" [29]); jazz and ballad teaching [45]; Satie [44]; MAESTRO change probabilities by bass motion [calc, 4.3].
- Priority order of change triggers, highest first [calc]:
  1. bass moves by step below about G3 (P(change) 0.59–0.85);
  2. bass leaps (0.48–0.79);
  3. same bass with a new upper harmony (0.19–0.50);
  4. treble-only motion (0.22–0.45, style-dependent).
- At IOIs below 0.2 s do not change within a run (P(change) 0.24–0.34 [calc]). Take fast figuration under the pedal of the underlying harmony.

### P2. Timing of a legato change

- **Release (pedal up)** at onset + u, with u depending on IOI [calc, 4.3; Hofmann [29]]:
  - u ≈ −40 ms for IOI < 0.2 s (just before the onset);
  - u ≈ 0 ms for IOI 0.2–0.5 s;
  - u ≈ +25…+50 ms for IOI 0.5–1.0 s;
  - u ≈ +75…+100 ms for IOI > 1 s (overlap for legato).
  - Never later than the re-press minus the minimum gap (P3).
- **Re-press (pedal down)** at onset + d, with d = clamp(40 + 0.25·IOI, 50, 450) ms [calc, fit to the IOI table]. MAESTRO medians: 53, 102, 130, 176, 237, 315 and 436 ms for the IOI classes up to 3 s. Overall median 146 ms with anchoring a (IQR 86–257).
- **Humanisation:** jitter d and u per change with a spread like the MAESTRO IQR (about ±40 % of d). Keep a per-performance bias (one "pianist") drawn from the across-file spread, 100–268 ms median lag [calc, 4.5]. Repp found pianists consistent with themselves [1].

### P3. How long the pedal stays up

- Up-gap g = re-press − release ≥ g_min. Target g ≈ 100 + 0.1·IOI ms [calc]; MAESTRO medians run 111–326 ms over the IOI classes, overall median 122 ms.
- g_min for the binary model:
  - at least the felt re-engagement (~50 ms, a model constant) plus time for the damping to act;
  - MAESTRO's 10th percentile is 59 ms [calc];
  - use g_min = 60 ms when the outgoing chord is in the middle and treble;
  - use g_min = 120 ms when bass notes below about C3 must be silenced, because their energy is high and a partly decayed bass still leaks through a short change [21] [26] [calc]. This is engineering judgement.
- If d − u < g_min (very fast IOI), move the release earlier, not the re-press later. MAESTRO's releases come before the onset at short IOIs [calc].

### P4. Direct pedal at starts and after rests

- At the first note of a piece, and at the first onset after the pedal has been up for ≥ 1 s: press at onset + 20 ms. MAESTRO median +23 ms, IQR −27…+82 ms [calc].
- With probability ~0.25, press 30–120 ms before the onset (anticipatory pedal). Use this especially in pp openings and Impressionist music [35] [37] [calc].
- For accented chords and sf in Classical style, press with the chord, 0–20 ms [37] [46].

### P5. Releasing at phrase ends and rests

- When all keys are up and a notated rest of at least a beat (or ≥ 0.5 s of silence) follows, release the pedal at the rest's notated end or at the next onset (legato change), and not at the start of the rest. MAESTRO pianists kept the pedal through 64 % of such silences [calc]; Chopin's marks run over rests [32].
- For staccato or "secco" endings, or a Classical-style cadence followed by a rest: release at the rest start + 0–50 ms so the silence is heard [32] [memory].
- Release before a general pause (≥ 1 bar or ≥ 2 s) at the latest 200–400 ms into it [memory: a judgement, no measurement found].

### P6. Stepwise bass and low-register scales

- When the bass moves by step (1–2 semitones) below about G3:
  - for IOI ≥ 0.3 s, change at every bass note;
  - for IOI < 0.3 s (a low scale or run), use no pedal, or a short pedal on the first note of the run (catch, then release after one or two notes) [5.2 acoustics; calc P(change) 0.65 against 0.45].
- When the stepwise motion is in the treble only (all notes ≥ C4) and the bass holds, keep the pedal in Romantic, Impressionist and ballad styles. Change on the beat in Classical style [calc: treble-only P 0.22–0.30 in Liszt, Debussy and Scriabin, 0.73 in Mozart].
- Use Hofmann's octave rule as a tie-breaker: passing tones more than an octave above the bass and chord can stay under the pedal [29].

### P7. Staccato and detached textures

- If most notes in a beat have duration ≤ 50 % of their IOI and no long bass note is held, use no pedal. Exceptions:
  - a short direct pedal on accented chords;
  - "touch" pedals of about 0.2–0.45 s on downbeats in Baroque and Classical style (Bach median segment 0.44 s, 2 onsets) [calc].
- Never pedal through notated rests inside a staccato figure [memory].

### P8. Style presets (from MAESTRO, section 4.3)

| preset | target pedal-down fraction | change rule | segment median | re-press after onset |
|---|---|---|---|---|
| Baroque (Bach, Scarlatti) | 0.2–0.35 | touch pedals on harmonic beats; organ points; none in counterpoint | ~0.45 s | ~110–160 ms |
| Classical (Haydn, Mozart) | 0.25–0.35 | short pedals per harmony/beat; none in Alberti runs unless the harmony holds | ~0.45 s | ~90–105 ms |
| Beethoven/Schubert | 0.5–0.65 | legato per harmony; honour long marked pedals (Op. 27/2, 31/2, 53) with periodic clean changes on a modern piano | ~0.5 s | ~90 ms |
| Romantic (Chopin, Schumann, Liszt, Brahms) | 0.7–0.8 | legato per harmony/bass; keep through fast figuration | ~0.55–0.65 s | ~60–100 ms |
| Impressionist/late Romantic (Debussy, Ravel, Scriabin, Rachmaninoff) | 0.8–0.85 | change mainly with the bass; washes over unchanged bass (P ≈ 0.2) | ~0.8 s | ~80 ms |
| Ballad/jazz | — | change on chord symbols/bass, not on melody notes; extra changes under busy melody | per chord | as Romantic |
| Stride/boogie | low | dry; short direct pedal on bass notes at most | — | 0–20 ms |

(Ballad and jazz rules are from teaching sources [45]; there is no measured data.)

### P9. The binary-damper case (no half pedal)

- **Half-change ("keep the bass, clean the top").** A binary damper cannot do it. Emulate it by:
  - (a) a normal quick change (g ≈ 60–80 ms) plus holding the bass note's key (extend its note-off to the end of the pedal point: a "finger pedal" or sostenuto), so the bass string's own damper stays up [35] [2.8];
  - (b) if the bass cannot be held because of the pitch range or another voice, a quick change of g ≈ 60 ms followed by re-striking nothing. The bass then decays faster than the treble; accept the loss.
- **Flutter or vibrato pedal.** Emulate by quick full changes (g ≈ 60 ms) every 150–250 ms during the passage. This is about 4–6 extra changes per second, consistent with Schmitt's "trill the pedal" in scales over a pedal point [35]. Each change damps the upper resonance for one short gap. No measured flutter rate was found, so the 150–250 ms period is an engineering choice.
- **Quarter and colour pedal (Bach/Mozart "touch").** Emulate by short full pedals of 150–400 ms after the onset (pinnacle gestures [15]) instead of shallow holds. Of MAESTRO pedal presses, 24 % never pass CC 100 [calc]. A binary model plays these as full pedal, so keep them shorter than the notated value.
- **The CC64 to damper map.** If the model reads incoming CC64 (non-planner mode), map down at CC ≥ ~64–70 and up at CC < ~56–60 (hysteresis). The real damper-contact points are 60–76 [14]. The planner itself emits only 0 and 127.

### P10. Catching a bass note under a leap (after-pedal)

- Pattern: the left hand strikes a bass note (≤ C3) and leaps up to chord(s) within the same harmony, and the bass key is released before the harmony ends.
- Action: re-press the pedal at bass onset + d, with d = clamp(40 + 0.25·IOI, 50, min(150, bass key-up − 20 ms)). The pedal must be down before the bass key comes up, or the bass is lost.
- If the previous pedal is still down and the bass belongs to a new harmony, release at the bass onset (P2) and catch as above.
- Evidence: Hofmann [29]; the MAESTRO short-IOI lag of 53 ms [calc]; the "catch pedal" pinnacle [15].

### P11. Pedal must not override the notation where it matters

- If the score gives explicit CC64 or "Ped." spans (for example a MIDI file from a notation program), prefer those spans and only fix the change timing (P2–P3).
- Long composer pedals (Beethoven Op. 27/2, 31/2; Chopin) are part of the composition [32] [42].
- On a modern-piano model, insert extra quick changes only where the bass changes by step under the long pedal, which keeps the composer's span.

### Summary of numbers for the planner

| parameter | value | source |
|---|---|---|
| release relative to new onset | −40 (fast) … +100 ms (slow); median +12 ms | [calc] [29] |
| re-press after onset | clamp(40 + 0.25·IOI, 50, 450) ms; median 146 ms | [calc] [2] |
| up-gap | median 122 ms; p10 59 ms; target 100 + 0.1·IOI | [calc] |
| minimum gap, binary damper | 60 ms (mid/treble), 120 ms (bass) | [calc] + judgement |
| direct pedal | onset + 20 ms; 25 % anticipatory −30…−120 ms | [calc] [35] |
| change probability per onset | 0.24 (IOI < 0.1 s) … 0.65 (IOI ≥ 0.6 s) | [calc] |
| stepwise low bass vs same bass | 0.65 vs 0.45 change probability | [calc] |
| pedal-down fraction | Bach 0.22 … Scriabin 0.86 | [calc] |
| pedalled segment | median 0.56 s; 3 onsets | [calc] [9] |
| shallow presses (peak < CC 100) | 24 % | [calc] |
| damper contact on Disklavier/CEUS | CC 60–76 | [14] |
| undamped treble | top ~18–22 keys (from ~F♯6/G6) | [47] [24] |
| key damper fall after staccato | ~80 ms after hammer contact | [25] |

## 7. The planner as built (`-pianist`, `src/pianoplan.{h,cc}`)

`./pajupilli -pianist -play FILE.mid` gives every channel played wholly by a piano with dampers
(`grand-piano`, `bright-piano`, `upright-piano`, `honky-tonk`; not the dulcimer, the harpsichord or
the electric pianos) a planned sustain pedal. The file's own CC64 on that channel is dropped and the
plan's CC64 events (0/127) are put among the song's events after the director has fixed the times,
so they reach `PianoPlayer::setHold` by the same path a file's pedal takes. Without `-pianist`
nothing changes (the renders are byte-identical to the previous build's). Sostenuto and una corda
are not planned: the model has neither.

The model's damper is on/off (`src/piano.cc`: the felt falls onto the string in 50 ms), so the half
pedal, the half change and the flutter pedal (2.4, 2.5) are out of reach; P9's quick change stands
in for the half change.

| rule | what the planner does | from |
|---|---|---|
| R1 attacks | notes within 35 ms are one attack (a chord as played) | 4.3 |
| R2 texture | each four-bar section: **detached** if over 60 % of its notes are let go before 55 % of an IOI under 0.5 s (a slow chord let go early is a performed file's pedal at work, 4.7); **linear** if under 12 % of its attacks are chords of three notes or more, its single notes move mostly by step, and no low note is held under them (counterpoint; a line over a held bass, Peace Piece's, is pedalled), or the director read a quantized file as Baroque and under 30 % are chords; else **by harmony** | 3.1, P7, P8 |
| R3 change triggers (by harmony) | a new bass: a step (1–2 semitones) from the lowest note under the pedal always, a leap to a pitch class not under it once the pedal has stood 0.4 s (a broken chord's notes are not each a bass); a chord of three or more notes below G5 bringing two new pitch classes; a chord revoiced (three notes below G5, not the last chord's voicing) 0.8 beat after the last press; the bass struck again 0.6 s after the last press; a low note (< A3) a second from one under the pedal in the last 6 s (mud); eight pitch classes gathered (a wash). Not a melody note over the harmony | P1, P6, 3.6, 3.7, 4.3 (bass-motion table), 4.7, 5.2 |
| R4 quick change | a melody (≥ C5) that has gathered three semitone steps over a pedal held two beats: up ~50 ms (the half change's stand-in) | 2.4, P9 |
| R5 no change too soon | not within 0.16 s of the last press | 4.3 (P(change) 0.24–0.34 at IOI < 0.2 s) |
| R6 release | the onset −10 ms ± 25 ms (4.7); `PJ_PIANIST_UPSHIFT=1` adds MAESTRO's IOI-dependent offset (−55 ms at IOI 0.1 s … +104 ms at 2.25 s) | 4.3, 4.7 |
| R7 press | onset + clamp(40 + 0.25·IOI, 50, 450) ms × a lognormal spread (σ 0.45, `PJ_PIANIST_LAG` scales it), before the next onset that may change the pedal again (0.8 of the way), and before the bass's key comes up unless that would cut the lag below 60 % of itself | P2, P10, 4.7 |
| R8 minimum lift | 60 ms; 120 ms when a bass under C3 is to be silenced (the release moved earlier, P3) | P3 |
| R9 direct pedal | at the start and after a lift: down at the onset + 23 ms ± 40 ms (5–120 ms, before the bass's release) | P4, 2.1 |
| R10 lifts | every key up over 1 s and over a beat: up 0.25 s after the last release (at most 30 % into the gap); a low stepwise run (four or more single notes below C4, a second apart, under 0.3 s each): up through it; a linear or detached section: up, the pedal only on a long chord there (three notes held 1.5 beats; detached: two) and the last chord; the end: 50 ms after the last release | 2.7, P5, P6, P7, 4.7 |
| R11 human timing | the spreads drawn from a xorshift generator seeded by the channel: the same file, the same pedalling | 4.5 |

It prints, per piano channel, the texture decided, the changes by kind, the share of the time the
pedal is down, the median press lag and the sections; `PJ_PIANIST_LOG=1` every decision (the time,
the bar, the reason, the release and press against the onset), `=2` also every pedal event.
Knobs: `PJ_PIANIST_LAG` (× 1), `PJ_PIANIST_LAGSPREAD` (0.45), `PJ_PIANIST_UPSHIFT` (0),
`PJ_PIANIST_RESTRIKE` (0.8 beat), `PJ_PIANIST_BASSAGAIN` (0.6 s), `PJ_PIANIST_MINLEAP` (0.4 s).

## 8. Validation against the pianists' own pedalling [calc]

Each performed file of 4.7 played with `-pianist`; its own CC64 compared with the plan. A human
release is matched when a planned release lies within ±150 ms of it. "Lag" is the press minus the
last onset before it (the same measure for both).

| file | his lifts | planned | his matched | plan's his | lag median planned / his (ms) | down share planned / his |
|---|---|---|---|---|---|---|
| Don't Explain | 219 | 249 | **79 %** | **70 %** | 93 / 121 | 0.80 / 0.65 |
| I Remember You | 812 | 852 | 65 % | 62 % | 84 / 113 | 0.80 / 0.49 |
| Mood Indigo | 490 | 383 | 63 % | 80 % | 72 / 92 | 0.81 / 0.62 |
| Day Dream | 421 | 460 | 73 % | 67 % | 92 / 140 | 0.82 / 0.78 |
| Peace Piece | 76 | 106 | 87 % | 62 % | 206 / 146 | 0.74 / 0.88 |
| Nocturne in E-flat | 297 | 292 | 62 % | 63 % | 94 / 135 | 0.74 / 0.63 |
| Bach Prelude (arr.) | 293 | 345 | 51 % | 43 % | 107 / 144 | 0.83 / 0.62 |
| Talk of the Town | 335 | 351 | 76 % | 73 % | 78 / 115 | 0.80 / 0.74 |
| Some Other Time | 317 | 392 | 87 % | 70 % | 119 / 125 | 0.81 / 0.83 |
| Schubert (track 7) | 362 | 640 | 74 % | 42 % | 73 / 97 | 0.81 / 0.76 |
| **mean** | | | **72 %** | **63 %** | | |

- **Chance and a naive baseline.** As many releases placed at random would match ~28 % of
  Don't Explain's (219 releases × a 0.3 s window / 234 s). A naive rule (a change at every onset
  with a note below C4, at most one per 0.4 s) matches 71 % of the pianists' releases with 59 % of
  its own right (F 0.64 against the planner's 0.67): these ballad players re-pedal at almost every
  left-hand attack. The planner's harmony rules mainly buy precision, and the sparse pedalling of
  counterpoint, which the naive rule would not give (below).
- **What the planner misses.** Changes at revoicings that keep the pitch classes, releases with
  no onset near (lifted to clear before a rest), and the pianist's own phrasing. The Schubert's
  inner voices around G3 read as new basses, so it changes almost twice as often as the pianist.
  The arranged Bach prelude's broken chords at F3–C4 defeat the bass rule.
- **Lag.** The planned presses fall 20–50 ms earlier than these pianists' by the same measure,
  because they are drawn from MAESTRO's fit and kept before the bass's release. Raising them (×1.3,
  ×1.6) moved the medians to the pianists' but lowered the matches (F 0.65, 0.64), so the
  MAESTRO fit stays.
- **Down share.** Planned 0.74–0.83 against the pianists' 0.49–0.88: the plan has no dry choruses
  (I Remember You's 24 % of the time in lifts over 3 s) and no phrasing lifts.
- **Files with no pedal of their own** (the plan's only pedalling):
  - BWV 525/1 (trio sonata, quantized, `-reinstrument '*=grand-piano'`): the director reads it
    Baroque; linear throughout, 10 changes at the final cadence, the pedal down 4 % of the time
    (MAESTRO's Bach: 22 %, mostly short touches the planner does not add).
  - BWV 530/2: 0 and 4 pedals on its three channels.
  - BWV 543: 0–6 per channel, down 0–1 % of the time.
  - The jazz ballads above are all planned "by harmony", 0.74–0.83 down; MAESTRO's Romantic
    composers 0.72–0.80 (4.3).
- **Tests** (`tools/test.py pianist`): without `-pianist` no plan and the same sound; a chord
  progression (C Am F G at 90, a melody over it) changed at each of its seven chord changes, the
  press 137–259 ms after, no change at the melody's notes; a two-part sixteenth-note invention
  unpedalled; Don't Explain matched at ≥ 70 % of the pianist's releases and ≥ 60 % of the plan's
  (measured 80 % and 70 %; chance ~28 %, the naive rule 77 % and 65 %).

## 9. What could not be found or fetched

- **Repp 1996 and 1997 numbers** (ms lags, change durations): only the abstracts [1] [2]. The papers are paywalled, and no Haskins preprint was found. MAESTRO numbers stand in for them.
- **Heinlein 1929–1930 numbers:** citations and secondary summaries only [4].
- **Liang**: the JAES 2018 paper [11], the ICASSP 2019 paper [13] and the PhD thesis [12] were not fetched (the thesis download was blocked in this environment). Their measured pedal-depth and timing statistics, and any half-pedal statistics from natural playing, are missing.
- **Pedagogy books:** Banowetz [38], Gieseking/Leimer [39] and Neuhaus [40] were not readable, so only secondary quotes are used. The same holds for the Chopin pupil testimony (Eigeldinger [34]) and for Rosenblum [33], known only via Poli [32].
- **Physics:** Martin (1947) bass decay times, a measured flutter rate, a measured after-pedal window, and Bill Evans pedalling data were not found.
- **Not measured:** the "~50 ms" damper re-engagement of the model is the model's own constant. No paper here measures the pedal-tray damper fall time; Askenfelt's 80 ms is for key-driven dampers after a staccato [25].

## Sources

1. B. H. Repp, "Pedal timing and tempo in expressive piano performance: A preliminary investigation", *Psychology of Music* 24 (1996) 199–221. doi:10.1177/0305735696242011. Abstract via Crossref API.
2. B. H. Repp, "The effect of tempo on pedal timing in piano performance", *Psychological Research* 60 (1997) 164–172. doi:10.1007/BF00419764; PMID 9342960. Abstract via Europe PMC.
3. B. H. Repp, ASA 126th meeting abstract 4pPP1 (Denver 1993), on relational invariance across tempo: https://auditory.org/asamtgs/asa93dnv/4pPP/4pPP1.html (related paper: *Psychological Research* 1994).
4. C. P. Heinlein, *Journal of General Psychology*: "A Discussion of the Nature of Pianoforte Damper-Pedalling…", 2 (1929) 489–508, doi:10.1080/00221309.1929.9918087; "The Functional Rôle of Finger Touch and Damper-Pedalling…", 2 (1929) 462–469, doi:10.1080/00221309.1929.9918084; "Pianoforte Damper-Pedalling under Ten Different Experimental Conditions", 3 (1930) 511–528, doi:10.1080/00221309.1930.9918227.
5. C. Hawthorne et al., "Enabling Factorized Piano Music Modeling and Generation with the MAESTRO Dataset", ICLR 2019. Data: https://storage.googleapis.com/magentadata/datasets/maestro/v3.0.0/maestro-v3.0.0-midi.zip (analysis in section 4.3 is ours).
6. MAESTRO v3.0.0 pedal statistics computed for this note (own MIDI parser and scripts; section 4.3), marked [calc].
7. Q. Kong, B. Li, X. Song, Y. Wan, Y. Wang, "High-resolution Piano Transcription with Pedals by Regressing Onset and Offset Times", IEEE/ACM TASLP 2021; arXiv:2010.01815.
8. B. Liang, G. Fazekas, A. McPherson, M. Sandler, "Piano Pedaller: A Measurement System for Classification and Visualisation of Piano Pedalling Techniques", NIME 2017, pp. 325–329. https://nime.org/proc/nime2017_bliang/
9. B. Liang, G. Fazekas, M. Sandler, "Transfer Learning for Piano Sustain-Pedal Detection", IJCNN 2019; arXiv:2103.13219.
10. B. Liang, G. Fazekas, M. Sandler, "Piano Legato-Pedal Onset Detection Based on a Sympathetic Resonance Measure", EUSIPCO 2018, pp. 2484–2488. doi:10.23919/EUSIPCO.2018.8553341. https://new.eurasip.org/Proceedings/Eusipco/Eusipco2018/papers/1570437306.pdf
11. B. Liang, G. Fazekas, M. Sandler, "Measurement, Recognition, and Visualization of Piano Pedaling Gestures and Techniques", *J. Audio Eng. Soc.* (2018). https://aes2.org/publications/elibrary-page/?id=19584 (not read).
12. B. Liang, *Modelling Instrumental Gestures and Techniques: A Case Study of Piano Pedalling*, PhD thesis, Queen Mary University of London, 2019. https://qmro.qmul.ac.uk/xmlui/handle/123456789/64236 (not read).
13. B. Liang, G. Fazekas, M. Sandler, "Piano Sustain-Pedal Detection Using Convolutional Neural Networks", ICASSP 2019 (not read; cited by [7]).
14. P. Hu, H. Zhang, S. D. Peter, C. E. Cancino-Chacón, S. Dixon, G. Widmer, "Revisiting Expressive Timing in Piano Performance at Scale: A Distant Listening, Multi-Corpus Study", *Trans. ISMIR* 9(1) (2026) 329–346. doi:10.5334/tismir.317.
15. H. Zhang, K. Fang, Z. Wang, I. Fujinaga, "Evaluating High-Resolution Piano Sustain Pedal Depth Estimation with Musically Informed Metrics", arXiv:2510.03750 (v2 2026).
16. K. Fang, H. Zhang, Z. Wang, I. Fujinaga, "High-Resolution Sustain Pedal Depth Estimation from Piano Audio Across Room Acoustics", arXiv:2507.04230 (2025).
17. H. Zhang, S. Chowdhury, C. E. Cancino-Chacón, J. Liang, S. Dixon, G. Widmer, "DExter: Learning and Controlling Performance Expression with Diffusion Models", arXiv:2406.14850 (2024).
18. K. Worrall, Z. Yin, T. Collins, "Comparative evaluation in the wild: Systems for the expressive rendering of music", *IEEE Trans. Artificial Intelligence* (2024). https://eprints.whiterose.ac.uk/id/eprint/217634/
19. M. Bernays, C. Traube, "Investigating pianists' individuality in the performance of five timbral nuances through patterns of articulation, touch, dynamics, and pedaling", *Frontiers in Psychology* 5:157 (2014). https://pmc.ncbi.nlm.nih.gov/articles/PMC3941302/
20. T. Lipke-Perry, M. Levy, D. J. Dutto, "Probing Focus of Attention: Multiple Case-Study Analysis of Pianists' Pedaling…", *Music & Science* (2022). doi:10.1177/20592043221123225 (abstract).
21. H.-M. Lehtonen, H. Penttinen, J. Rauhala, V. Välimäki, "Analysis and modeling of piano sustain-pedal effects", *J. Acoust. Soc. Am.* 122(3) (2007) 1787ff.. doi:10.1121/1.2756172 (abstract).
22. H.-M. Lehtonen, A. Askenfelt, V. Välimäki, "Analysis of the part-pedaling effect in the piano", *J. Acoust. Soc. Am.* (2009). doi:10.1121/1.3162438 (abstract).
23. A. Stulov, V. Välimäki, H.-M. Lehtonen, "Modeling of the part-pedaling effect in the piano" (2012), catalogue record via lib.cas.cz (snippet).
24. A. Schutz, N. Bertin, D. T. M. Slock, B. David, R. Badeau, "Piano 'Forte Pedal' Analysis and Detection", AES 124th Convention, Amsterdam 2008, paper 7427. https://eurecom.fr/publication/2478/download/cm-schuan-080517.pdf
25. A. Askenfelt, E. Jansson, "From touch to string vibration: Measuring the timing", in *Five Lectures on the Acoustics of the Piano*, Royal Swedish Academy of Music, 1990. https://www.speech.kth.se/music/5_lectures/askenflt/measure.html
26. G. Weinreich, "The coupled motion of piano strings", same volume. https://www.speech.kth.se/music/5_lectures/weinreic/motion.html
27. R. Plomp, W. J. M. Levelt, "Tonal consonance and critical bandwidth", *J. Acoust. Soc. Am.* 38 (1965) 548–560. https://www.math.miami.edu/~armstrong/592sp15/Plomp_Levelt_1965.pdf (quoted via search).
28. S. Zambon, H.-M. Lehtonen, B. Bank, "Simulation of piano sustain-pedal effect by parallel second-order filters", DAFx 2008 (snippet).
29. J. Hofmann, *Piano Playing, with Piano Questions Answered* (1909/1920), Project Gutenberg #39211 (mirror https://aleph.pglaf.org/3/9/2/1/39211/39211.txt). Chapters "The Use of the Pedal" and "The Pedals".
30. J. F. Cooke, *Great Pianists on Piano Playing* (1917), Project Gutenberg #28026; S. V. Rachmaninoff chapter, "The Significance of the Pedal".
31. A. Hullah, *Theodor Leschetizky* (London: John Lane, 1906), Project Gutenberg #43915.
32. R. Poli, *The Secret Life of Musical Notation* (Amadeus Press, 2010), ch. 4 "…of pedals" (excerpt: https://s3.amazonaws.com/halleonard-closerlook/00332939/SecretLife_pedals.pdf); quotes Czerny, *Pianoforte-Schule* Op. 500 (1839).
33. S. P. Rosenblum, "Some Enigmas of Chopin's Pedal Indications: What do the Sources Tell us?", *Journal of Musicological Research* (1996) (via [32]). Also Rosenblum, *Performance Practices in Classic Piano Music* (Indiana UP, 1988) [memory, not read].
34. J.-J. Eigeldinger, *Chopin: Pianist and Teacher as Seen by His Pupils* (Cambridge UP, 1986) (via [32] and search snippets).
35. M. Metaxaki, *Considerations for Pedalling Debussy's Piano Music*, DMA thesis, City University London, 2005. https://openaccess.city.ac.uk/id/eprint/8449/
36. D. Rowland, *A History of Pianoforte Pedalling* (Cambridge UP, 1993), ch. 7 "The emergence of modern pedalling", doi:10.1017/CBO9780511597251.008 (chapter page and search snippets).
37. T. Al Bakri, "Pedalization: types, characteristics, and techniques of the right piano pedal", *Per Musi* 26 (2025). doi:10.35699/2317-6377.2025.58395. https://www.scielo.br/j/pm/a/nYGZJ4y7jFBZHYLWfhkh85x/
38. J. Banowetz (with D. Elder, M. Hansen, M. Hinson, W. S. Newman), *The Pianist's Guide to Pedaling* (Indiana UP, 1985) (publisher page and catalogue table of contents only).
39. W. Gieseking, K. Leimer, *Piano Technique* (Dover, 1972; includes *Rhythmics, Dynamics, Pedal and Other Problems of Piano Playing*, 1938) (not read).
40. H. Neuhaus, *The Art of Piano Playing* (Russian 1958; English transl. K. A. Leibovitch, 1973) (via [37]).
41. A. Schiff, "Senza pedale ma con tanti colori" (Florence, 2012), quoted in Piano Street Magazine, 12 Feb 2013. https://www.pianostreet.com/blog/?p=5758
42. Wikipedia: "Piano Sonata No. 14 (Beethoven)", "Sustain pedal", "Soft pedal" (accessed 2026-11-21).
43. Mozart knee-lever debate (M. Latcham; E. Badura-Skoda 2000; P. Badura-Skoda 2002; R. Grohman 2016), via search snippets and https://danieladammaltz-com.squarespace.com/tcp/knee-levers-pedals-effects-lost-on-modern-pianos
44. e-musicmaestro, "Satie – Gymnopédie No. 1" teaching notes. https://e-musicmaestro.com/members/resources/view/179
45. Jazz and ballad pedalling teaching pages: PianoGroove (https://pianogroove.com/live-seminars/how-to-use-the-sustain-pedal), Dummies pedal exercises, Tim Richards boogie lesson (musicgurus.com) (search snippets).
46. Cross-Eyed Pianist, pedalling posts (Marmontel and Chopin quotes, Rubinstein attribution). https://crosseyedpianist.com/tag/pedalling/
47. Undamped treble: PianoWorld forum "undamped last octave"; Mark Goodwin Pianos, "top register no dampers" (https://markgoodwinpianos.co.uk/yamaha/top-register-no-dampers); Kawai and Roland FAQs.
48. SFU Sonic Studio handbook, "Critical Bandwidth". https://www.sfu.ca/~gotfrit/ZAP_Sept.3_99/c/critical_band.html
49. PTG discussion "sustain pedal height and travel". https://my.ptg.org/discussion/sustain-pedal-height-and-travel (snippet).
50. C. Petzold, "The Complete Beethoven", day 140 (Op. 27/2). https://charlespetzold.com/beethoven250/day140.html (snippet).
51. Wikipedia: "Johann Andreas Stein" (knee lever, 1777).
52. Lehmanns.ch pedagogy book page (Neuhaus "Dem Flügel die Poren öffnen!", Liszt "Hackbrett" quote) (search snippet).
53. Sakimura, on "senza sordini" as moderator on Beethoven's Walter piano. https://www.sakimura.org/en/2018/05/4156/ (snippet).
