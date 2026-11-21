## p000
- **Staccato note dies at once (32 ms to −20 dB vs 131…208)**: a 100 ms note falls 20 dB within 32 ms. Together with the sharp onset (28 vs 8…14 dB/ms), this means the first few ms peak about 20 dB above what follows: a hammer/contact spike over the body. The bright piano gives 88 ms and the CP-80 217 ms on the same engine. Likely FAULT: look at the first 30 ms of a v80 note.
- **Key-to-key level scatter (5.0 vs 1.3…3.2 dB)**: the probe includes A0 and A7, where the grand drops. FluidR3 is 3.2 too. Worth checking, but not clearly wrong.
- **Brightens with velocity (+1.07 vs 0 ERB)**: DELIBERATE (README "Concert grand": the felt hardens). This is mostly a TiMidity ARTIFACT: Jnsgm2 through FluidSynth brightens +1.4 ERB.
- **Floor −90 dB, release 328 ms**: room ARTIFACT.

## p001
- **Key-to-key level scatter (5.1 vs 1.3…3.2 dB)**: the same engine as the grand and the same picture. research/shigeru_kawai_sk7.md measured 0.8–2.5 dB rms off the neighbours at v100. The extra here comes from the probe's extremes and its velocities: check before calling it a fault.
- **Sharper onset (27.6 vs 8…15 dB/ms)**: the same hammer spike as p000, but the staccato length (88 ms) is less extreme. Possible FAULT shared with the grand.
- **Brighter (11.3 vs 10.3…10.8 ERB), brightens with velocity (+1.16 ERB)**: DELIBERATE. This agrees with the earlier FluidR3 comparison (shigeru_kawai_sk7.md, "Against FluidR3"): FluidR3's GM 2 is its grand sample with a fixed brightness, and here the brightness grows with the blow. The >6 kHz excess re SGM (+33 dB) is the dark SGM sample.
- **Release 408 ms**: room ARTIFACT.

## p002
- **Sharp onset (37.8 vs 5.5…13.8 dB/ms) but slow attack (39.5 vs 2…12 ms)**: the onset is steep and the peak comes late. This pattern repeats across plucked and struck programs (p004, p007, p014, p024, p025) and is most likely the hall's early reflections and reverb build-up adding to the first tens of ms. Probably a room ARTIFACT; a `-room dry` render would settle it.
- **Key-to-key scatter (3.6 vs 0.75…2.8 dB)**: SGM is 2.8. Partly DELIBERATE: research/cp80.md models per-pickup spread.
- **Brightens with velocity (+0.72 ERB)**: DELIBERATE (README "Electric grand"). Jnsgm2 through FluidSynth gives +1.7 ERB, so this is a TiMidity ARTIFACT.
- Overall the nearest to the fonts of all programs (rank 67).

## p004
- **Brightens strongly with velocity (+2.63 vs 0 ERB), brighter (12.1 vs 8.0…9.9 ERB)**: DELIBERATE. The pickup's bark grows with the swing (README "Electric piano 1"; research/rhodes.md §4.6 quotes Muenster & Pfeifle: the timbre changes more than the volume). The fonts' TiMidity renders here do not move under FluidSynth either.
- **Key-to-key level (4.6 vs 0.9…1.4 dB) and +2.8 dB/oct toward the top**: DELIBERATE uneven voicing. rhodes.md §5 gives ±3–6 dB between neighbours on a real harp. Still the outlier among the fonts: consider halving `PJ_RHODES_WEAR`'s spread if it sounds lumpy.
- **Slow attack (36 vs 3…7 ms) with a sharp onset**: the room pattern from p002, perhaps with the Suitcase vibrato (on by default). Likely ARTIFACT.
- **Release 346 ms**: room.

## p005
- **Velocity barely changes level (2.7 vs 16.6 dB) but brightens hugely (+4.3 ERB)**: DELIBERATE. This is the DX7's own velocity law (README "Electric piano 2"). The fonts' range is TiMidity's law.
- **Speaks late (onset 44 vs 1…9 ms; attack 148 vs 5…53 ms; phrase onset 26 ms)** and **key scatter 3.8 vs 0.6…1.3 dB**: likely FAULT. A plausible common cause: each key is three whole DX7 voices 1/7 Hz apart with free-running phases. Their sum depends on the starting phases, partly cancelling at the start of some keys and swelling over a 7 s beat. Test with `PJ_DX_ONE=128` (a single voice).
- **Louder (+3.1 vs −4.5…−0.2 dB)**: minor; a trim.

## p006
- **No dynamics (−0.6 vs 16.7 dB)**: DELIBERATE, as on the instrument (README "Harpsichord": the plectrum lets go at a fixed force). The fonts' range is TiMidity's law.
- **Key scatter (4.3 vs 0.4…1.7 dB) and uneven legato scale (3.5 vs 0.3…0.8 dB)**: likely FAULT. Candidates: the 0.8 % split between the two swing directions, which beats at ~3.5 Hz at A4, deep inside a 0.5 s window, and the rows plucking one after another.
- **Darker above 2.5 kHz (−3…−9 dB re all four), brightens slower up the keys (2.0 vs 2.8…3.4 ERB/oct)**: likely FAULT, known. README: the board is still the grand's, and the treble plucks at a third of the string. TimGM6mb and Jnsgm2 are 1.3–1.4 ERB darker under FluidSynth, which narrows the gap.
- **Quieter (−5.9 vs −4.7…−1.2 dB)**: a trim.

## p007
- **Odd/even partials (−13.4 vs −1.6…−4.1 dB)**: possible FAULT. The default pickup pair's comb is unverified. README "Clavinet" says the recorded D#3's first comb gap lies two partials lower than the model's.
- **Release 708 ms vs 5…44**: the room accounts for ~300–400 ms (cf. p002, p004). The rest is the thunk and the dropped string, which research/clavinet.md says the yarn kills in 0.1–0.3 s. Probably too long: FAULT worth checking.
- **Brightens with velocity (+3.7 ERB)**: DELIBERATE (README: "the Clavinet's dynamics are mostly its brightness"). Jnsgm2 through FluidSynth gives +3.0 ERB, so this is a TiMidity ARTIFACT.
- **Slow attack (85 vs 1…4 ms)**: likely the beating of the two swing directions (up to 15 dB deep, 0.5–2 Hz) moving the peak, plus the room. Mostly ARTIFACT of the measure.
- **−20…−30 dB above 6 kHz**: DELIBERATE (stage 1 rolls off from 5 kHz).

## p008
- **Stops sooner after the key (205 vs 658…841 ms)**: DELIBERATE. The celesta has dampers (README "Celesta"); the fonts let the sample ring.
- **Decays slower while held (7 vs 11…24 dB/s)**: DELIBERATE. The box ring times are fitted to a recording (research/celesta.md: ~11 s at middle C).
- **Uneven in tone and level from key to key (1.1 vs 0.2…0.55 ERB; 3.4 vs 0.1…2.7 dB) and −2.3 dB/oct toward the top**: mostly the fitted model. SGM is also uneven (2.7 dB). Low priority; listen to the top octave's level.
- **Louder (+7.6 dB)**: the fonts span −8…+4, so not a clear fault.

## p009
- **Spectrum: −24…−39 dB below 400 Hz, spectrogram far from the fonts (0.92 vs 0.37)**: ARTIFACT of the compass. The glockenspiel sounds G5–C8 (README "Chromatic percussion"), so probe keys 48–76 fold up by octaves, while the fonts play at key pitch. This was chosen DELIBERATELY.
- **Slow attack (18 vs 1…3 ms)**: hard mallets on bare steel should peak at once. Probably the room build-up pattern (cf. p002). Check dry.
- **Louder (+3.7 vs −6.9…−2.8 dB), noisier**: the noise may be the room at high pitch. The level is a trim; research/chromatic_percussion.md §9 says levels were set only "roughly even".

## p010
- **Narrow dynamics (6.9 vs 16.6 dB)**: plausible as DELIBERATE: a pin plucks a tooth with a fixed bend (research/chromatic_percussion.md §6). TiMidity's law sets the fonts' range.
- **Louder (+6.3 vs −5.7…−1.3 dB)**: a trim FAULT, low risk.
- **Key scatter 4.1 vs 0.1…1.3 dB; brightens slower up the keys (1.8 vs 3.2…3.9 ERB/oct)**: a first form, unchecked (§9 "What is owed"). Possible FAULT: the box's fixed rise to 300 Hz and fall above 7 kHz shape the top teeth too much.
- **−30 dB below 150 Hz**: ARTIFACT (the C4–C8 compass folds key 48).

## p011
- **Key scatter (5.3 vs 0.2…2.6 dB) and tone scatter (1.5 vs 0.2…1.0 ERB)**: possible FAULT. Each bar's resonator coupling and the mallet straying give per-key differences; a real M55 is voiced even. Look at which keys stand out.
- **Rougher (−4.3 vs −6.5…−9 log2), Zimtohrli 2.4× the fonts' spread**: worth a listen. Odd/even (−34 dB) is an ARTIFACT of the measure: 1:4:10 bars have no harmonic series.
- **<150 Hz −38…−53 dB**: ARTIFACT (F3–F6 compass; key 48 folded).
- **Brightens with velocity (+0.6 ERB)**: DELIBERATE (README "Vibraphone": the mallet hardens with velocity).

## p012
- **Louder (+8.5 vs −7.2…−0.7 dB)**: likely trim FAULT. Struck programs sit consistently above the fonts here (p008–p015).
- **Rougher (−2.5 vs −7…−9 log2), Zimtohrli ~2× the fonts' spread, key scatter 2.9 vs 0.5…1.4 dB**: possible FAULT. research/chromatic_percussion.md §3 admits uniform-bar shapes and estimated ratios (1:3.92:9.24). The 3.92 partial, 24 c off the 4th harmonic, may be what reads as rough.
- **Odd/even (−21 dB) and "sustains more after 2 s"**: ARTIFACTS. Bars have no harmonic series; −86 dB at 2 s is the room floor.

## p014
- **Darker (15.3 vs 19.4…20.7 ERB; −10 dB at 2.5–6 kHz, −28…−36 dB above 6 kHz)**: partly ARTIFACT: the compass is C4–F5, so 8 of the 13 probe keys fold down. Mostly a FAULT, though: only nine modes (up to 8.9× the note) and radiation that grows with frequency (research/chromatic_percussion.md §5) cut the metallic shimmer real chimes have. All four fonts agree.
- **Weaker low partials (tristimulus 0.17 vs 1.0)**: ARTIFACT. Bells have no harmonic series.
- **Slow attack (57 vs 2…12 ms)**: a rawhide strike should be instant. Room and modal bloom; check dry.
- **Brightens with velocity (+2.35 ERB), louder (+9.9 dB; fonts −11…−0.3)**: the hammer hardness is plausible. The level is a trim, though the fonts disagree widely.

## p015
- **Key scatter (5.0 vs 0.5…1.6 dB), tone scatter 1.8 ERB, uneven legato (3.0 dB)**: KNOWN FAULT. research/chromatic_percussion.md §7: "the soundboard is still the grand's… the largest known fault"; decays are uneven note to note.
- **Decays slower (11.6 vs 17…23 dB/s), −31.5 vs −42…−51 dB after 2 s**: DELIBERATE, partly. There are no dampers, and the ring was set by ear at the user's request (§7). The grand's board likely adds sustain.
- **Louder toward the top (+0.8 vs −2.3…−0.5 dB/oct), louder overall (−3.1 vs −9.3…−5.1 dB)**: levels set only "roughly even" (§9). A FAULT, minor.

## p016
- **No velocity response (0.7 vs 16.6…22 dB)**: DELIBERATE: a Hammond has none (README "Hammond B-3": "Velocity has no other effect" than the click).
- **Floor −57 vs −112…−125 dB**: DELIBERATE: hum, leakage and the generator's whisper (README: "the idle floor is ~−53 dB under a note").
- **Release 390 vs 60…80 ms**: room plus the Leslie at 4 m: ARTIFACT.
- **Wobble 15.9 vs 0.1…6.9 c**: DELIBERATE (Leslie Doppler).
- **Louder toward the bottom (−3.0 vs −0.5…+0.1 dB/oct)**: possible FAULT; also in p018 and p019. Check the generator taper or the Leslie drum's share. The level against a band agreed with TiMidity/FluidR3 to a median −0.3 dB (research/hammond_b3.md "Level against TiMidity").

## p017
- **No velocity response, floor −53 dB, release 306 ms**: DELIBERATE or room, as in p016.
- **Speaks later in a phrase (30 vs −4…5 ms)**: DELIBERATE. The percussion is single-trigger (README "Hammond B-3"), so legato notes lack the ping and their steepest rise comes later.
- **Brightens slightly with velocity (+0.37 ERB)**: DELIBERATE (touch-sensitive percussion).
- Otherwise close (rank 49).

## p018
- **No velocity response, floor −53 dB, release 462 ms**: DELIBERATE or room, as in p016.
- **Speaks late (onset 44 vs 2…15 ms; phrase 38 vs ~0 ms)**: the Leslie heard at 4 m accounts for ~12 ms. The remaining ~25–30 ms is unexplained; possible FAULT in the driven 122 path (the amp's envelope?). Worth a look, since it is audible against a band.
- **Louder toward the bottom (−3.4 vs −1.3…0 dB/oct)**: as in p016.

## p019
- **No velocity response (0.3 vs 16.6 dB)**: DELIBERATE (pipe organ).
- **Release 1838 ms, floor −26 dB, onset 222 vs 34…86 ms**: DELIBERATE. These are the church and the pipes' speech: README "Church organ" gives ~230 ms at 8' C and ~70 at middle C, plus 24 ms pallets.
- **Quieter (−7.9 vs −1.9…+0.4 dB)**: partly ARTIFACT: the slow speech and the room's build-up eat into the 0.5 s level window. Likely also a trim FAULT. Check the level against a band, as was done for the Hammond (hammond_b3.md).
- **Uneven legato scale (3.1 vs 0.3…1.6 dB)**: probably pallet variation plus room. Minor.

## p020
- **Quieter (−4.9 vs +0.4…+3.8 dB)**: partly the slow speech in the 0.5 s window. Likely a trim FAULT as well: all four fonts sit above the median.
- **Slow attack (285 vs 49…102 ms), onset 86 vs 28…34 ms, duller attack (−3.6 vs −0.3…+0.4 ERB)**: DELIBERATE in kind. A pressure harmonium's reeds swell, start nearly sinusoidal and brighten (README "Reed organ"). But 285 ms with the Percussion on (README: C3 speaks in ~75 ms) looks slow; check.
- **Brightens with velocity (+1.4 ERB)**: DELIBERATE (Expression: velocity is wind pressure).
- **Release 380 ms**: room.

## p021
- **Key scatter (5.9 vs 0.5…1.1 dB)**: likely FAULT, or a probe interaction. Candidates: the musette's 3.5 beats/s within the 0.5 s window, and a bellows reversal (every 2.5–5 s, ~30 ms gap; README "Accordion and bandoneón") landing in some notes. Check by holding the bellows (one direction) and with `PJ_ACC_BEAT=0`.
- **Duller attack (−6.4 vs −1.4…+0.2 ERB), speaks later (104 vs 19…50 ms)**: DELIBERATE in kind (the duct inertia makes the reed speak; research/free_reeds.md). The size of the difference is large, though: the reed may start too sinusoidally.
- **Narrower dynamics (11.1 vs 16.6 dB)**: plausible for free reeds. TiMidity sets the fonts' range.
- **Stereo coherence 0.35, release 616 ms**: DELIBERATE (bass left, treble right) and room.

## p023
- **Brighter (19.7 vs 14.3…15.6 ERB; −8…−17 dB below 400 Hz)**: likely FAULT. research/free_reeds.md §4 describes zinc-plate bandoneóns as "a large, deep sound", "sweet and somewhat dull". The model's brighter grille, thicker plates and the 4' octave overshoot that, and nothing is fitted to a recording yet (§10; *Escualo* is the candidate).
- **Key scatter (8.3 vs 0.5…1.1 dB)**: the worst of the set. It shares p021's suspects: bellows reversals and the probe window.
- **Slow attack (252 vs 58…84 ms), onset 85 ms**: the tango marcato wants a quicker attack. Possible FAULT.
- **Quieter (−3.4 dB), release 719 ms**: a trim; the release is room.

## p024
- **Louder (+5.9 vs −2…+1 dB) together with a velocity-curve kink (−13 / −10 / 0 / +5 vs −13 / −6 / 0 / +4)**: likely FAULT in the velocity law. v64 sits 4 dB under the fonts' shape and v96 is 10 dB above it, so at v96 the guitar is too loud and at v64 about right. The nail coming in between 40 and 105 (research/guitars.md, "Picking's colour by velocity") may add a level step on top of the colour.
- **Brightens with velocity (+2.1 vs 0 ERB)**: DELIBERATE, the user's design (flesh to nail; README "Classical guitar").
- **"Brighter" (10.9 vs 8.7…9.4 ERB)**: the band table says the opposite (+12…+19 dB at 150 Hz–1 kHz, −13…−27 dB above 2.5 kHz). This is a MEASUREMENT conflict: the guitar is fuller in the body and darker on top.
- **Key scatter (4.2 vs 0.8…1.6 dB), decays faster (16.6 vs 9…12 dB/s)**: the scatter is partly the guitarist's string and fret choice. The decay is within a real nylon guitar's range and was set by ear (`PJ_GUITAR_RING`).

## p025
- **Louder (+7 vs −4.9…+1.1 dB)** at every velocity: a trim FAULT.
- **Sharper onset (52.9 vs 10…15 dB/ms), brighter at 1–6 kHz (+7…+13 dB)**: DELIBERATE pick click (README steel guitar, `PJ_STEEL_CLICK`), targeted at FluidR3's first-5 ms excess above 5 kHz (research/steel_guitar_build.md §1.3). The ~−17 dB above 6 kHz re the TiMidity fonts suggests the click lacks top end rather than having too much.
- **Brightens with velocity (+2.0 ERB)**: DELIBERATE: a hard stroke is nearer the bridge. steel_guitar_build.md §1.3 found FluidR3 has no timbre change under TiMidity.
- **Partial envelopes alike (0.52 vs 0.92…1.0)**: possible FAULT. Even FluidR3's recorded guitar keeps 0.96; the two-polarisation beating or the sympathetic strings may be overdone.
- **Decays slower (7.8 vs 10.6…18.4 dB/s)**: DELIBERATE. It is fitted to the in-recording decays of FluidR3's samples (§1.2), not to the fonts' loop envelopes.
## p003
- **Bright, with little bass (centroid 14.6 vs 10.3…11.9 ERB; −15…−20 dB under 150 Hz re all four; fundamental share 0.22 vs 0.43…0.89)**: probable FAULT. README "Honky-tonk piano" says the closed case adds "no overall tilt". The upright's smaller board (piano_variants.md §3.1) explains a weaker bass, but not 15–20 dB against every font. FluidR3 through FluidSynth is only +1.0 ERB brighter. Check A0–A2 at v96.
- **Sharp onset (30.5 vs 8…13 dB/ms)**: the grand's hammer spike (notes_a p000) plus the tacks' knock (README). Shared possible FAULT.
- **Pitch scatter 12.5 vs 4.6…8.3 c; slower decay (6.4 vs 9.6…11.4 dB/s)**: DELIBERATE. These come from the detuned unisons and bad strings, which no longer lock, so the prompt decay goes (README; piano_variants.md §3.3).
- **Range 10.3 vs 16.7 dB**: mostly TiMidity's law (the grand gives 14.1). **Release 482 ms**: room.

## p013
- **Bright and flat over the keys (15.5 vs 11.0 ERB; 0.23 vs 3.3…3.5 ERB/oct; −32…−60 dB under 400 Hz)**: mostly a MEASUREMENT ARTIFACT. The compass is F4–C8 sounding (README "Chromatic percussion"), so probe keys 48–64 fold up an octave. The fonts play a near-pure fundamental at the key (fundamental share 1.0). The 1:3:6 bar (chromatic_percussion.md §4) is DELIBERATE.
- **Louder (+1.4 vs −8.4…−11.9 dB re the other instruments)**: FAULT, a trim of ~10 dB.
- **Onset 64 vs 0.3…1.4 dB/ms, noisier (−26 vs −49…−60 dB)**: DELIBERATE hard mallets. The fonts' samples start softly.
- **Pitch scatter 8.3 vs 0.3…2.1 c; tone scatter 1.1 vs 0.2…0.5 ERB**: possible FAULT. README calls the model a first form, "not yet checked". The fold boundary accounts for part of it.

## p022
- **Velocity 127 is 24 dB softer than 96 (range −10.3 vs +16.6…20.1 dB; darkens −1.45 ERB)**: FAULT, the largest. At the top of the breath map (~3 kPa at v127, harmonica.md §10.3) the reeds appear to choke. Check a held C5 at v127.
- **Key-to-key level 7.8 vs 0.8…1.0 dB**: probable FAULT. It may be the same choke on some reeds, or a blow/draw imbalance. Keys 36–44 fold into C3–D7.
- **Slow (attack 92 vs 44…61 ms; onset 64 vs 28…32)**: mostly DELIBERATE: breath attacks take 20–70 ms (README "Chromatic harmonica").
- **Mid-heavy (+8…+13 dB at 400 Hz–1 kHz, −11…−16 dB at 2.5–6 kHz)**: DELIBERATE: the cupped hands' ~1.1 kHz resonance (README).
- **Release 486 ms**: room.

## p028
- **Brighter (12.7 vs 7.8…11.2 ERB; +5…+23 dB at 1–6 kHz re three fonts)**: probable FAULT. The palm was fitted to FluidR3's decay times (README "Electric guitars", "The palm"; electric_guitars.md §7), but not to its tone. That note measured a centroid of 220–360 Hz below E4. Ours uses bridge+middle pickups with the pick ~6 cm from the saddle. Only TimGM6mb (11.2) comes near.
- **Rough (Plomp-Levelt −0.2 vs −3.4…−5.7)**: possibly the pick click over a dark note. Listen first.
- **Onset 62 vs 4…13 dB/ms, attack 1 ms**: DELIBERATE: the pick slips off in 0.15 ms.
- **Release 430 vs 58…69 ms**: room ARTIFACT. The palm-muted release itself is 25/s.

## p038
- **Speaks slowly (attack 28 vs 4…10 ms; onset 24 vs 2…5; 12 ms in a phrase)**: probable FAULT for a bass meant to snap. The patch's amp attack is 2 ms (src/lead.cc), so something else delays the peak, perhaps the filter pluck's resonance or the core's start (it also starts −7.5 c flat).
- **Holds (decay 1.2 vs 4.8…9.3 dB/s; −11 dB after 2 s vs −14…−22)**: DELIBERATE: sustain 0.7 (synth_leads.md, "synth families" table). TiMidity misreads the fonts' envelopes badly here (11.7–14.3 dB rms for FluidR3 and TimGM6mb), so this is partly an ARTIFACT.
- **Darker (2.9 vs 3.2…5.7 ERB)**: mostly a TiMidity ARTIFACT. FluidSynth makes the fonts 1.3–2.1 ERB darker, and SGM gives 3.2. **Range 10.8 dB**: TiMidity's law.

## p047
- **Wider dynamics (26.4 vs 16.6…20.1 dB; −20/+7 dB at v40/127)**: DELIBERATE: the felt mallet's F = K x^2.54 (README timpani). The fonts' range is TiMidity's law.
- **Louder (+7.7 vs −12.4…+1.2 dB re the other instruments)**: a trim of ~5 dB. The fonts disagree widely.
- **Flat over the keys (1.67 vs 3.7…4.1 ERB/oct)**: MEASUREMENT ARTIFACT. Probe keys 68–96 lie above the five drums' C2–E4 and go to the outer drums (README).
- **Speaks 25 ms late (21 ms in a phrase vs 0)**: possible FAULT. It is likely the thrown mallet's flight. Timpanists play ahead of the beat, but GM files don't, so consider starting the throw earlier.

## p056
- **Sounds a tone low under -play**: FAULT (the transposition bug: src/midifile.cc adds `transpose` −2). It also voids the pitch rows (median +28 c, scatter 20 c, wobble 86 c, fundamental share ~0): the f0 search stays within ±5 % of the key (tools/gmcompare.py), and −200 c falls outside.
- **Dark (11.2 vs 17.8…19.3 ERB)**: known FAULT: brass_section.md §12 and §14, "Against FluidR3": ours is much darker above 2 kHz under v~110.
- **High bands lead the low (−7.5 vs +19…+59 ms); attack 4.6 ERB brighter than the body (fonts −1…−2)**: probable FAULT, shared with p058/p060. trumpet.md §7 says the higher harmonics build up later. Something in the onset (the tongue or the breath push) is too bright.
- **Low notes weak (+5.6 dB/oct; scatter 3.4 dB)**: known (README brass test table: low notes ~20 dB softer). **Release 722 ms**: room.

## p058
- **Uneven and flat over the keys (+5.6 dB/oct vs 0.3…1.1; scatter 4.2 vs 0.4…0.6 dB; 0.9 vs 2.9…4.0 ERB/oct)**: mostly a MEASUREMENT ARTIFACT. The probe plays 40–82 against E1–F4 (README "Brass"), so 67–82 fold down and E1–D2 is never heard. Recheck within F2–F4.
- **Speaks late (onset 74 vs 13…28 ms; 56 ms in a phrase)**: DELIBERATE: tuba.md §7.2 gives 80–200 ms at low pitch and says "accept the physics".
- **Wide dynamics (24.1 vs 16.6…20.1 dB)**: DELIBERATE (README "Brass", Dynamics). The fonts' range is TiMidity's law.
- **High bands first (−6.3 vs +1.5…+27 ms), attack 2.9 ERB brighter**: the brass onset pattern of p056.
- **Release 504 ms**: room. Louder by ~3 dB: a small trim.

## p059
- **Sounds a tone low**: FAULT (the transposition, as p056). The pitch +31.7 c, wobble 50 c, scatter 15.8 c and fundamental share 0.005 are ARTIFACTS of it.
- **Dark and full below 1 kHz (8.1 vs 10.6…22.3 ERB; +6…+17 dB at 150 Hz–1 kHz re the fonts)**: probable FAULT. A stem-out Harmon should have little below 1 kHz and a bump at 2 kHz (README "Brass": 16–25 dB less below 1 kHz; mutes.md §5 "What to validate"). Under FluidSynth the fonts are 1.5–2.5 ERB darker but still brighter than ours. Part of this is the open trumpet's darkness.
- **Little dynamics (7.8 vs 16.6…21 dB)**: known: soft notes stop near mf (README brass test table). The open trumpet gives 14.4.
- **Key scatter 4.2 dB, legato dip 12.9 vs 6.5…8.9 dB**: probable FAULT. The ~160 Hz parasite makes low notes wobbly (mutes.md §3).

## p060
- **Sounds a fifth low**: FAULT (the transposition, −7). The fundamental share 0, odd/even −24 dB and wobble 33 c are ARTIFACTS: the key's harmonics catch only every third partial.
- **Quiet and weak at the bottom (−7.2 vs +1.1…+5.2 dB; +7.7 vs 0.3…2.0 dB/oct)**: partly the fault, which drops the probe into the horn's weak low register. Recheck after the fix. A trim will likely remain.
- **Narrow range (8.8 vs 16.6 dB)**: known (README brass test table: "many notes hold only a narrow dynamic range").
- **Late (158 vs 64…88 ms)**: README gives 30–60 ms mid-register and ~100 ms at A2. Mostly the transposition.
- **High bands first (−23.5 vs +18…+42 ms), attack 6.2 ERB brighter**: the brass onset pattern (p056), strongest here.

## p078
- **Uneven over the keys (level scatter 4.8 vs 0.05…1.4 dB; tone scatter 3.0 vs 0.1…0.4 ERB; −4.2 dB/oct; legato evenness 3.7 vs 0.3…2.3 dB)**: partly an ARTIFACT. The compass is D5–A6 (src/designs.cc), so keys 60–73 fold up and 96 down. The legato scale is uneven too, so this is a probable FAULT.
- **Quieter (0.1 vs 5.4…8.2 dB)**: FAULT, a trim of ~7 dB.
- **Noisy (flatness −30 vs −60…−72 dB; +13…+36 dB above 2.5 kHz)**: partly DELIBERATE: a fipple is breathy, and the fonts are near-sines. But nothing in README or research/ documents a target, and 30 dB is a lot.
- **Pitch scatter 18.9 vs 0…6.5 c**: tools/test.py has 19/20 notes in tune, so suspect the folds and the noise. Low confidence.
- **Release 388 ms**: room.
## p026
- **Darker above 1 kHz than three fonts (−3 to −12 dB at 1–2.5 kHz; +6 to +16 dB below 1 kHz)**: deliberate. The warm archtop (the neck humbucker's comb, tone at "3", closed 15") was chosen over FluidR3's brighter sample (research/jazz_guitar.md §12.4, "Not met": centroid 348 Hz against FluidR3's 901). The centroid is inside the fonts' range (9.2 vs 8.7–15.8 ERB), and SGM is darker than ours above 2.5 kHz (−15 dB).
- **Speaks late: onset 22–24 ms vs 0–6 ms, attack 20 ms vs 1–5**: a small fault. The 8 ms gesture wait (README "Jazz guitar") plus the hand's push (up to 10 ms soft) add up. A noteLead of about 15 ms would put the comping back on the beat.
- **Wobblier and rougher held note (7 c p-p vs ≤1.7; dissonance −4.0 vs −5.0…−6.7)**: mostly deliberate. The late shallow vibrato and the floating bridge's two swing directions beat (README). Worth a listen on long chords.
- **Quieter (−2.7 dB vs +0.7 median)**, long release, high floor: the room's release and floor are an artifact; the level difference is minor.

## p027
- **Brightness hardly rises up the neck (0.47 ERB/oct vs 3.2–4.8)**: probable fault. All four fonts agree. Either the low strings are too bright (modes to 8 kHz through the bridge and middle pickups) or the treble strings are too dull. The centroid at velocity 96 is inside the fonts' range (16.0 vs 12.8–18.4), so the problem is the tilt over the keyboard, not the overall level.
- **Soft attack: 20 ms vs 1–8 ms, darker than the body (−0.5 vs +1.1…+5.0 ERB)**: possible fault. The pick has no click on the Strat (the steel guitar has `PJ_STEEL_CLICK`), and the two-polarisation bloom delays the peak.
- **Decays faster (14.1 vs 8.4–11.8 dB/s)**: fault-ish. README "Electric guitars" says the string losses are "not yet fitted to recordings".
- **Brightens with velocity (+2.1 ERB vs 0)**: deliberate (pick physics). FluidR3's velocity layers change only the release (research/electric_guitars.md §2.7), and TiMidity drops filter modulators.
- **Release 508 ms vs 124–140 ms**: room artifact (the fretting hand mutes at once).

## p029
- **A 100 ms staccato sounds 400 ms (the cap) vs 116–155; release 732 ms vs 30–122**: suspect. The room alone gives the clean guitar 170 ms. Here the compressor pedal (0.2 s release) and the crunch probably hold up the muted string's tail. Check how fast the fretting-hand mute falls through the drive.
- **Narrow velocity range (12.3 vs 16.6–20.1 dB; +0 dB from 96 to 127)**: deliberate. At crunch the pickup's 17 dB come out as 6 (README "The Marshall"). The fonts' 16.6 dB is TiMidity's velocity law.
- **Slow swell (attack 256 ms; the fonts range from 32 to 193)**: deliberate. The compressor and the feedback hold make the note grow (README "Feedback").
- **Quieter (−2.9 vs −0.7…+2 dB)**, more low end (<150 Hz +1…+17 dB), stereo coherence 0.47: the stereo is the room (artifact); the rest is minor.

## p030
- **Speaks late: onset 92 ms vs 1–13; 58 ms in a phrase vs −2…22**: likely fault, the most audible one here. Possible causes are the gate opening, the grid-current bias slide blocking the attack, or the GM-31 player's look-ahead. The high bands also lag the low ones by 17.5 ms (fonts 1–10).
- **Spectrum differs from the earlier fit: −8 to −15 dB at 400 Hz–1 kHz and +6 to +21 dB above 6 kHz against every font**: research/distortion_guitar.md §11 check 1 asks for ±4 dB of FluidR3. This average over all the probe keys does not show that, so check whether the >6 kHz excess is hiss, aliasing or the room.
- **A 100 ms note sounds 400 ms (vs 126–145)**: suspect. Gate check 7 says −60 dB within 200 ms; see p029.
- **Velocity range 4.2 dB (vs 16.6) and brightening +4.6 ERB**: deliberate (distortion_guitar.md §11 check 3; README "The high-gain amp").
- **Wobble 26 c**: deliberate, the rock player's wide vibrato from E3 up.

## p031
- **Darker, more fundamental (12.5 vs 17.1–19.0 ERB; tristimulus-1 0.60 vs 0)**: measurement artifact. Every font sounds two octaves above the key, so the measures don't find the fonts' fundamental at the key's pitch. Ours plays at the written pitch (README "Guitar harmonics"; electric_guitars.md §9.5). The same artifact explains the odd/even, partial-stretch and tristimulus rows.
- **Uneven from key to key (2.8 dB vs 0.2–0.8)**: a known residue. The player's evening fit leaves 1.5 dB rms and ±4 dB at worst (README). The fonts are flatter, and FluidR3 stays within ±0.8 dB.
- **Instant attack (2 ms vs 20–33)**: deliberate (picked). The fonts' samples fade in.
- **−34 dB after 2 s (vs −1…−14)**: deliberate. The string decays on its own, while FluidR3's samples are cut at 0.2–1.5 s and looped.

## p032
- **Speaks late: onset 47 ms vs 15–19; 70 ms in a phrase vs 0–34**: fault worth fixing. Double-bass has no noteLead, so a walking line drags behind the beat. A lead of about 40 ms, as the slap and fretless basses have, would fix it.
- **Uneven from key to key (3.9 dB vs 0.03–1.4)**: deliberate in kind (notes near A0/T1 loud and short: research/double_bass.md §2), but larger than any font. Unlike the contrabass, nothing evens the plucked notes.
- **Darker above 1 kHz (−5 to −10 dB)**: consistent with the gut, the old-record targets (double_bass.md §7d).
- **Narrow velocity range (13 vs 16.6 dB)**: TiMidity convention. **Purer (−82 vs −60…−68 dB flatness)**: the samples' noise, an artifact. The floor and the release come from the room.

## p033
- **Louder toward the top (+2.4 dB/oct vs −0.3…+0.9)**: probable fault, shared with every electric bass. The pickup hears the string's velocity, and the bassist does not even out the line.
- **Quieter against the others (−3.7 vs +0.8…+1.4 dB)**: worth retrimming in the level table.
- **Brightens with velocity (+1.3 ERB vs 0)**: deliberate (README "Electric bass": centroid 141/154/173 Hz at v40/80/120, "FluidR3's does not move"). Part of the fonts' 0 is TiMidity ignoring the filters.
- **Narrow range (12.6 dB; +1 dB from 96 to 127)**: deliberate. The LA-2A narrows it, and README says "-play's velocity gain makes up the difference", but the top step is still flat.
- **Purer**: sample-noise artifact. The spectrum agrees with FluidR3 within ±2 dB.

## p034
- **Louder toward the top (+4.4 dB/oct vs 0…+2.0)**: probable fault, the same pattern as p033 but larger: about 11 dB over E1–A#3 against ≤5.
- **Purer (−88 vs −58…−67 dB flatness)**: artifact. The fonts' samples carry noise. Fret clank is missing ("Not yet" in README "Pick bass").
- **Quieter (−4.5 vs −0.2 median)**: small level trim.
- **Brightens with velocity (+0.55 ERB)**, partials less alike (0.44 vs 0.78–1.0): deliberate. README "Pick bass" says "FluidR3's samples change only in level with velocity; ours brightens". The spectrum agrees with the earlier FluidR3 band check from 120 Hz to 4 kHz; ±12 dB against the other fonts, which disagree with one another.
- Close overall (see its rank above).

## p035
- **Pitch wobble 17.8 c vs 2.5–6.4**: deliberate. The landing error (sd 8 c), the scoops and the ±10 c vibrato are in README "Fretless bass". Audible, and the defining trait.
- **Uneven from key to key (2.5 dB vs 0.04–1.1)**: partly the neck's dead spot (D3 on the G string, README "Electric bass"). Minor.
- **Brightens with velocity (+1.6 ERB vs ≤0)**: deliberate. TiMidity ignores Jnsgm2's velocity-to-filter modulator (research/fretless_bass.md §4.2).
- **Brighter above 2.5 kHz (+6 to +26 dB) and hollower at 400 Hz–1 kHz (−7 to −12)**: the bridge pickup at full and the B-15 blend. The upper partials' bloom (FluidR3's recording) is still "Not yet". Possibly too bright: worth a listen.
- **Smoother legato (dip 13.6 vs 18–21 dB)**: deliberate (one voice). The release comes from the room.

## p036
- **Brightens strongly with velocity (+3.0 ERB vs 0)**: mostly a TiMidity artifact. The thumb/pop and fret physics are deliberate (README "slap"; slap_bass_2.md §1.4, where under FluidSynth Jnsgm2 gives +24 dB above 2 kHz).
- **Attack 15 ms vs 1–4**: possible fault. FluidR3's thumb samples peak 4–7 ms after their start (research/slap_bass_1.md §1.3), so a thumb slap should be the fastest attack on the list.
- **Uneven from key to key (2.6 dB vs ≤1.2)**: thumb, ghost and late-pop rules plus the dead spot. Check that the probe's single notes don't flip technique.
- **Narrow range (13.7 vs 16.6 dB)**: TiMidity convention. The release comes from the room.

## p037
- **Louder toward the top (+10.6 dB/oct vs −4.1…+2.2)**: largely deliberate. On GM 38 everything from D2 up is popped (README), so the probe crosses from thumb to pop. The size of the step (≈+25 dB over the range) still looks too large: FluidR3's pop is +4…+9 dB above 2 kHz.
- **Brightening +5.2 ERB vs 0**: TiMidity artifact plus deliberate (slap_bass_2.md §1.4 and README "Against the references": E3 above 2 kHz +23 dB from v32 to v127, Jnsgm2 +24).
- **Attack 15 ms vs 0.5–4**: as p036, worth a look.
- **Narrow range (11.9 dB)**: GM gain curve after the frets' cap; TiMidity convention. The release comes from the room.

## p039
- **Quieter than the fonts (−0.4 vs +1.4…+6.2 dB)**: a small level fault. The fonts all make this patch loud.
- **Narrow velocity range (11.9 vs 16.6–20.1 dB; −9 at v40)**: a TiMidity convention, and also the patch's velocity-to-envelope routing. Jnsgm2 is misread by TiMidity here (+17 dB range, 11 dB envelope under FluidSynth), so treat the fonts' spread with care.
- **Brightens with velocity (+0.8 ERB)**: deliberate (synth_leads.md §"velocity to envelope depth and filter").
- **Release 164 ms vs 13–53, onset 21 ms vs 2–5**: the room, and `relMul` 0.4. Rank 64: close overall.

## p040
- **Loses level toward the top (−5.2 dB/oct vs −1.4…+0.8) and is quieter overall (−4.8 vs −0.6…+2.3 dB)**: probable fault. About 17 dB from G3 to A#6. The fonts' E strings are not weaker, so check the violin's bow-evening table.
- **Weak fundamental (tristimulus-1 0.32 vs 0.56–0.67)**: deliberate in kind. The body is fitted to 17 recorded violins (README, within 5 dB from 196 Hz to 1.4 kHz). The fonts' close samples have more fundamental. The centroid is inside the fonts' range (18.3 vs 15.7–19.9).
- **Speaks late in a phrase (70 ms vs 2–24)**: the bow's onset. The 100 ms pizzicato wait is compensated by noteLead. Real, but on the slow side.
- **Staccato 400 ms**: a 100 ms key is a pizzicato by design (README "Double bass": "a key let go within 100 ms is a pizzicato"). Release and floor come from the room.

## p041
- **Darkens with velocity (−1.55 ERB vs 0…+0.5)**: known fault. Viola.md §2 and "Fixed" item 1 note that the centroid still falls from p to ff after `bowBright`.
- **Brighter (15.7 vs 11.0–13.9 ERB; +4…+8 dB at 1–2.5 kHz)**: probable fault. The upper-body resonance bank is "a stand-in, not physics" (Viola.md fix 3), and the model has not been compared with a recording.
- **Speaks late in a phrase (68 ms vs 0–24)**: bow onset, as on the violin.
- **Staccato 354 ms, release 803 ms**: by design (a 100 ms key is a pizzicato; README) plus the room.

## p042
- **Brighter and nasal (11.1 vs 6.7–9.5 ERB; +9…+13 dB at 400 Hz–1 kHz; tristimulus-1 0.19 vs 0.33–0.87)**: probable fault. All four fonts agree, and research/cello.md §10 says "no comparison with a recording".
- **Staccato 400 ms vs 132–184, release 683 ms**: by design (a 100 ms key is a pizzicato: README "Double bass (plucked)") plus the room.
- **Darkens slightly with velocity (−0.6 ERB)**: same bow-bandwidth issue as the viola (Viola.md §2). The `bowBright` fix was viola-only.
- Onset and attack are inside the fonts' range (FluidR3 and Jnsgm2 are just as slow).

## p043
- **Uneven from key to key (4.0 dB vs 0.3–2.1)**: likely a regression. research/double_bass.md §7e reports 1.3 dB std after the bow-evening table. The body changed to modes on 2026-11-20 (§7f), so `tools/bowlevels.py` probably needs a re-run, as the viola's table did.
- **Darkens with velocity (−1.2 ERB vs ≤0)**: the bow-bandwidth fault (Viola.md §2, not applied to the bass).
- **Speaks late (onset 126 ms, 64 ms in a phrase, vs 37–108 and −1…24)**: bow onset (double_bass.md §7e: 40–180 ms). There is no noteLead for the contrabass.
- **Wobble 26 c vs 4–18**: the vibrato knob plus the finger's tremble. Possibly heavy for a bass.
- **Brighter (6.5 vs 3.9–5.5 ERB)**: the fonts disagree (+10…+19 dB at 150–400 Hz against FluidR3 and SGM, about 0 against the other two). Minor.

## p044
- **Brighter with a weak fundamental (15.4 vs 11.9–12.9 ERB; tristimulus-1 0.16 vs 0.43–0.64)**: probable fault, the same as the bowed sections (p048). The fonts agree tightly.
- **Darkens with velocity (−2.1 ERB)**: the bow-bandwidth fault (Viola.md §2).
- **Speaks late in a phrase (72 ms vs 0–13)**: the seats' spread (20 ± 15 ms, the desks) plus the bow. Fonts' tremolo samples start at once. Partly deliberate (README string ensembles), but late.
- **Release 914 ms**: the room. Wobble and stereo are inside the fonts' range.

## p045
- **Bows instead of plucking: level after 2 s −7 dB vs −92…−127; attack 708 ms vs 14–56; tristimulus-1 0.23 vs 0.86–0.96; centroid 16.8 vs 9.1–9.6 ERB**: FAULT, a bug. `BowPlayer::setPizzicatoOnly(true)` (src/bassinst.h:151) only sets `wait_ = 0`, and with `wait_ <= 0` `BowPlayer::noteOn` calls `strike()`, a bow stroke (src/bassinst.cc:673). The pluck happens only in `due()` for keys released while waiting. So GM 46 is the bowed section 3 dB down. Every other row follows from this.
- **Onset 174 ms**: the same bug (a slow bow stroke).

## p046
- **Staccato 78 ms vs 311–400**: deliberate. A note let go within 0.1 s outside a quick passage is étouffé (README "Concert harp").
- **Uneven from key to key (4.8 dB vs 0.07–2.5)**: deliberate and known. The board's T1/A0 strings D3–F3 sound up to 14 dB over the middle (README: FluidR3 is within 5). It may be stronger than a real harp.
- **Starts 16 c sharp (vs −1…+3)**: deliberate in kind (tension modulation: README), but more than "a few cents". Worth a check at velocity 96.
- **More even partials (−9.9 vs −3…+0.3 dB)**: the phantom partials (README). **Purer**: the samples' noise, an artifact.
- **Onset 29 ms vs 4–10**: the pad's draw and slip. Slightly late, minor.

## p048
- **Brighter with a weak fundamental (17.6 vs 11.8–13.5 ERB; +4…+9 dB at 1–2.5 kHz; tristimulus-1 0.22 vs 0.55–0.67)**: probable fault. Recorded sections heard from the hall are darker, and the fonts agree tightly. "Bow nearer the bridge" (README string ensembles) is chosen, but 4–5 ERB is too much.
- **Louder (+4.9 vs −5.2…−0.6 dB), 8 dB over Ensemble 2**: a fault for single notes. A lone key goes to every section whose register covers it, but the `PJ_STRINGS_GAIN` trim was set on a chord.
- **Uneven (2.7 dB; −2.5 dB/oct)**: the number of sections on a key changes across the register. Deliberate dealing, but it is audible as steps.
- **Darkens with velocity (−1.6 ERB)**: the bow-bandwidth fault (Viola.md §2).

## p049
- **Darkens with velocity (−2.1 ERB) and brighter (17.3 vs 11.7–13.5 ERB)**: as p048, a fault. Even with brightness 0.7 (toward the fingerboard), it is as bright as Ensemble 1.
- **Uneven from key to key (3.8 dB vs 0.9–1.7)**: fault-ish, the section register overlap as in p048.
- **Late (onset 404 ms vs 202–270; 104 ms in a phrase vs 22–53)**: deliberate. The players come in one by one over 300 ms (README string ensembles), slower than the fonts' slow attack.
- Release and staccato are inside the fonts' range, the level too (−3.2 dB).

## p050
- **Late in a phrase (106 ms vs 1–18) and a slow onset (301 ms vs 22–166)**: deliberate (`ampA` 0.28 s, "a slow bow": synth_leads.md table), but slower than every font, so legato lines go soft. Consider a shorter attack.
- **Dark top (−18…−23 dB above 6 kHz; +8…+15 dB below 2.5 kHz)**: `cutoff` 0 by choice. It loses the sheen. The centroid is inside the fonts' range (11.2 vs 10.2–13.7).
- **Uneven from key to key (2.6 dB vs 0.05–1.7)**: likely an artifact. The two cores 12 c apart beat slowly at low keys inside the measuring window.
- Rank 68: close overall. The release comes from the room.

## p051
- **Brightens with velocity (+1.1 ERB vs 0)**: deliberate (velocity to filter). The fonts' 0 is TiMidity. Under FluidSynth Jnsgm2 moves +0.5.
- **Quieter (−3 vs −1.6…+1.5 dB)**: small level trim.
- **Pitch scatter 9.9 c vs 4–6**: deliberate. The cores are 20 c apart with the chorus (synth_leads.md table).
- **Slightly brighter (13.0 vs 10.8–12.5 ERB)** and a slower attack (460 vs 138–407 ms): minor. Rank 69: the closest of all programs.
## p052
- **Quieter (-6.3 dB vs -0.4..-3 against the other instruments)**: FAULT, small and easy: the choir's level trim leaves it ~4 dB under the fonts' balance.
- **Speaks later (onset 314 ms vs 107-186; in a phrase 100 ms vs 0-80)**: borderline FAULT. choir.md 5.1 aims for full level in 150-300 ms, and README "Choir (Choir Aahs)" adds ~25 ms entry scatter per singer. Every font is quicker.
- **Brightens with velocity (2.23 ERB vs 0-0.33)**: DELIBERATE. The glottal pulse closes faster with pressure (README "Choir", "The lungs"; choir.md 5.3). The fonts' 0 is TiMidity's convention.
- **Brighter top (+10..+31 dB above 6 kHz)**: partly the church room and breath noise. Overall it is only 1 ERB brighter than the fonts. Watch it, but not a priority.
- **Pan rises across the keys (3.6 dB/oct), wider vibrato (52 vs 36-42 c)**: DELIBERATE (sopranos left, basses right; each singer's vibrato extent ±35 c).

## p053
- **Very slow speech (attack 774 ms vs 39-80; onset 444 ms vs 22-39; 76 ms in a phrase vs 0-10)**: FAULT. README "Voice Oohs" asks for half level at ~110 ms and choir.md 5.1 for 250-400 ms. 774 ms is about twice the plan, so rhythmic "ooh" parts will drag.
- **Quieter (-7.1 dB vs -0.3..+2.8)**: FAULT. README says it is ~6 dB under the aahs before its trim. The trim does not make up for it.
- **Uneven over the keys (level scatter 3.2 dB vs 0.5-1.4; legato evenness 4.1 dB)**: probable FAULT. Above C4 the men drop out (a DELIBERATE split), but the level then jumps there.
- **Release 664 ms, stereo coherence 0.16**: MEASUREMENT ARTIFACT of the church room.
- **Vibrato 47 c vs 7-15; brightens with velocity**: DELIBERATE (singer physics). The fonts are flat loops.

## p054
- **Uneven tone from key to key (brightness scatter 1.62 ERB vs 0.17-0.45)**: likely the per-key formant/cutoff spread (synth_leads.md 10, K1 "each key its own fixed cutoff offset"). DELIBERATE, but the default looks too wide for a pad-like voice.
- **Darker when played harder (-1.11 ERB vs 0..-0.3)**: probable FAULT. A synth voice should open up with velocity, or at least stay the same.
- **Mid-heavy spectrum (+10..+15 dB at 1-2.5 kHz, -11..-18 dB above 6 kHz; fundamental share 0.26 vs 0.64-0.99)**: the formant bank's choice of vowel. Debatable: the fonts disagree with each other by up to 30 dB above 6 kHz.
- **Faster attack (55 ms vs 167-480)**: odd, since synth_leads.md says "slow onset". Minor.
- **Quieter (-3.1 dB)**: small trim.

## p055
- **Holds as long as the key (decay 0.6 dB/s vs 18-37; level after 2 s -9 dB vs -40..-124)**: DELIBERATE. README "-inst orchestra-hit" and the src/orchhit.cc header say it is players, not a recording (`PJ_HIT_LEN` gives an eighth instead). The GM habit is a one-shot stab, though, so the eighth may be the better default for `-play`.
- **Slow for a "hit" (attack 374 ms vs 44-103; onset 72 ms vs 4-41)**: FAULT for this program. The piccolo and flute run-ups, the piano roll and the reeds' slow speech blunt the stab.
- **Brighter and thin in the bass (13.3 ERB vs 6.5-9.4; -11..-22 dB under 150 Hz)**: probable FAULT. The low sections fold into their own ranges and the reeds are bright.
- **Smoother, little "wobble" (Plomp-Levelt -2.4 vs -0.3; wobble 16 c vs 67-82)**: MEASUREMENT ARTIFACT plus DELIBERATE choice. The fonts' sample is the Firebird chord, ours the key in octaves (orchhit.cc). The wobble measure misreads a chord.

## p057
- **Dark, fundamental-heavy (centroid 9.3 ERB vs 13.5-14.5, about 400 vs 800 Hz; fundamental share 0.54 vs 0.03-0.05)**: FAULT, the largest. A radiated trombone tone at mf has a weak fundamental and its peak at 500-1000 Hz. This is the same gap as the trumpet's darkness at mf in research/brass_section.md 14 ("against FluidR3": ours much darker above 2 kHz).
- **Much louder toward the top (12.2 dB/oct vs 1.0-1.8; key scatter 3.3 dB)**: FAULT. The f^1.45 breath law (README "Brass", the horn player) is uneven over 3.5 octaves. Keys 79 and 82 lie above F5 and fold, but that does not explain it.
- **Attack 4.8 ERB brighter than the body (fonts -0.5..-2.1)**: a brassy blip on a dull body. It follows from the darkness above.
- **Onset 74 ms vs 18-42; 64 ms in a phrase**: mildly slow (slide travel). Release 630 ms is the room.

## p061
- **Almost no dynamics (velocity 40->127: 5 dB vs 17-20; curve -3/0/0/+2)**: FAULT. It contradicts brass_section.md 14, check 5 (13 dB over 40->127, `PJ_BRASS_VELDYN`). Something on the `-play` path (the plan or the section's gain) loses velocity. Check it first.
- **Much louder toward the top (7.6 dB/oct vs -0.1..0.8)**: partly DELIBERATE voicing (a low lone key gets only trombones; README "Brass section", "A lone key"). Combined with each player's f^1.45 breath it is too steep: probable FAULT.
- **Bass-heavy and dull (+14..+17 dB under 150 Hz, -8..-14 dB above 6 kHz; fundamental share 0.47 vs 0.14-0.27)**: FAULT, as in p057. It agrees with brass_section.md 14's FluidR3 comparison (centroid 0.84 vs 1.5 kHz).
- **Release 952 ms; envelope far from the fonts**: mostly the room and the bells' tails (ARTIFACT). The fonts' samples sound at once.

## p062
- **Brightens with velocity (1.88 ERB vs ~0, the same under FluidSynth)**: a DELIBERATE analog patch choice, since velocity opens the filter (synth_leads.md 10). The fonts have no velocity-to-filter here. Harmless.
- **Late speech (onset 32 ms, 42 ms in a phrase vs 0-12)**: mild FAULT for a synth. The circuit should speak at once; the filter opening late (60 ms) is DELIBERATE, the level's late start is not.
- **Uneven from key to key (2.2 dB vs 0.1-1.4)**: per-voice analog spread (README "Synth leads", "Each voice its own"). DELIBERATE, but on the large side.
- **Release 622 ms, staccato 239 ms**: room plus the RC release (ARTIFACT). Overall close to the fonts (rank 50).

## p063
- **Uneven from key to key (3.0 dB vs 0.1-1.4)**: probable FAULT. Larger than the analog spread needs.
- **Brightens with velocity (1.49 ERB vs ~0)**: DELIBERATE, as in p062.
- **Late speech (onset 42 ms, 40 ms in a phrase vs 0-5); legato dips 13.8 dB vs 8.4-9.4**: mild FAULT. The legato retriggers and dips more than any font.
- **More even partials (-12 dB odd/even vs -3..-6), smoother**: DELIBERATE (ramp plus wide pulse, ladder offsets; synth_leads.md 10).
- **Release 756 ms**: the room (ARTIFACT).

## p064
- **Wrong pitch: sounds a tone low**: FAULT, the biggest for all transposing winds. Fundamental share 0 and high partials 0.999 against the key's harmonic series. src/midifile.cc plays `noteOn(pk + inst->transpose)`, so a GM concert-pitch key is taken as written (soprano-sax transpose -2).
- **Pitch +31.5 c, key scatter 27 c, wobble 103 c (fonts 0-16)**: re-measure after the transposition fix. The pitch tracker is reading a note a tone off. If any of it stays, it is a FAULT.
- **Slow speech (onset 88 ms vs 25-45; attack 226 ms vs 21-81)**: FAULT. Compare the bassoon's priming fix (README "Other woodwinds": rise 20 ms).
- **Noisy (flatness -26 dB vs -34..-57)**: breathier than every font.
- **Release 782 ms, staccato 400 ms**: the room (ARTIFACT).

## p065
- **Wrong pitch: sounds a major sixth low (alto-sax transpose -9)**: FAULT, the same `-play` transposition bug as p064 (fundamental share 0 vs 0.26-0.45).
- **Slow speech (onset 112 ms vs 15-24; attack 208 ms vs 19-50)**: FAULT, common to the reed player.
- **Attack 4 ERB brighter than the body; noisy (flatness -28 dB vs -34..-47)**: a breath or noise burst before the reed speaks. Probable FAULT.
- **Pitch -14 c (fonts -4..+2)**: re-measure after the fix. README Status says the alto plays 28/30 notes in tune.
- **Release 722 ms, staccato 400 ms**: the room (ARTIFACT).

## p066
- **Wrong pitch: sounds a ninth low (transpose -14)**: FAULT, the `-play` transposition bug (fundamental share 0). The note an octave lower also makes it darker (11.3 vs 11.6-16 ERB).
- **Slow speech (onset 129 ms vs 17-39; attack 273 ms vs 17-38)**: FAULT.
- **Attack 14 ERB brighter than the body (fonts -0.4..+0.8)**: FAULT. A noise or key burst precedes the tone and is heard as a hiss.
- **Vibrato 47 c vs 1-7**: the jaw vibrato is DELIBERATE (README "Other woodwinds"), but too wide unless it is the tracker misreading the wrong pitch. Re-measure.
- **Release 750 ms**: the room.

## p067
- **Wrong pitch: sounds an octave and a sixth low (transpose -21)**: FAULT, the `-play` transposition bug (fundamental share 0, partials 2-4 share 0.8). It explains much of "darker" (9.2 vs 12.4-18.1 ERB) and the spectrogram distance (0.81 vs 0.08 among the fonts).
- **Very slow speech (onset 182 ms vs 13-28; 94 ms in a phrase vs 0-6)**: FAULT.
- **Attack 10 ERB brighter than the body**: onset noise burst (FAULT, as on the tenor).
- **Vibrato 72 c vs 5-23; pitch scatter 10 c**: re-measure after the fix.
- **Release 612 ms**: the room.

## p068
- **Much brighter (22.1 ERB, about 2.2 kHz, vs 15.6-16.8, about 1 kHz; partials 5+ share 0.44 vs 0.02-0.11; +28..+35 dB above 6 kHz)**: probable FAULT. All four fonts agree closely, and flatness of -27 vs -45..-51 dB points to reed noise or buzz above 6 kHz.
- **Attack 8.5 ERB brighter than the body**: an onset noise burst (FAULT, shared by the reeds).
- **Brightness over the keyboard -0.8 vs +3.6 ERB/oct**: MEASUREMENT ARTIFACT. Probe keys 48-57 lie below Bb3 and fold up an octave.
- **Onset in a phrase 91 ms vs -2..12**: slow slurs, FAULT. The single-note onset (114 ms) is inside the fonts (16-128).
- **Release 746 ms**: the room.

## p069
- **Wrong pitch: sounds a fifth low (english-horn transpose -7)**: FAULT, the `-play` transposition bug (fundamental share 0).
- **Much brighter (20.4 ERB vs 13.3-14.4; partials 5+ share 0.43 vs ≤0.04; flatness -27 vs -44..-69 dB)**: probable FAULT, the same double-reed brightness and noise as p068.
- **Onset in a phrase 60 ms vs -2..8**: slow slurs.
- **Release 753 ms, staccato 398 ms**: the room (ARTIFACT).
- Not in README's woodwind table: undocumented.

## p070
- **Brighter (17.6 ERB vs 12.2-13.2; flatness -24 dB vs -47..-82)**: probable FAULT, reed noise. It holds even though 7 of 13 probe keys (66-84) lie above the bassoon's Eb4 and fold down an octave, which should make it darker.
- **Brightness and spectrogram against the keyboard**: largely a MEASUREMENT ARTIFACT of that folding. The fonts stretch bassoon samples up to C6.
- **Onset in a phrase 53 ms vs 2-5; staccato 318 ms**: slurs slow-ish. The staccato is the room.
- **Vibrato 24 c vs 0-14**: DELIBERATE breath vibrato (README "Other woodwinds", the reed player).
- **Release 654 ms**: the room.

## p071
- **Wrong pitch: sounds a tone low (clarinet transpose -2)**: FAULT, the `-play` transposition bug (fundamental share 0, high partials 0.999). The pitch +29.6 c, scatter 37.7 c and wobble 24 c are likely the tracker misreading it. Re-measure.
- **Brightens strongly with velocity (4.9 ERB vs ~0; FluidR3 under FluidSynth +0.6)**: DELIBERATE physics (the beating reed). The fonts' 0 is TiMidity's convention.
- **Slow speech (onset 102 ms vs 19-46; 86 ms in a phrase)**: FAULT.
- **Attack 15.6 ERB brighter than the body; noisy (-30 vs -56..-67 dB)**: onset noise burst, FAULT.
- **Key level scatter 3.5 dB vs 0.6-0.8; loudness range 13 dB vs 16-20**: uneven and a little narrow.

## p072
- **Wrong octave: sounds an octave high (piccolo transpose +12)**: FAULT. It is the `-play` transposition bug: fundamental share 0, partials 2-4 share 0.91 (energy on the key's even harmonics), centroid 24.2 vs 13.3-15 ERB. GM piccolo sounds at the key. The `-ensemble` piccolo (README "The flute family") is right to transpose; `-play` is not.
- **Key-to-key scatter (brightness 3.4 ERB, level 3.4 dB)**: re-measure after the fix. Some of it is the top keys folding past B7.
- **Release 677 ms, staccato 318 ms, coherence 0.48**: the room (ARTIFACT).
- **Noisy (flatness -24 dB vs -37..-81)**: breath noise. Plausible for a piccolo; the fonts disagree widely.

## p073
- **Far too much louder toward the top (16.9 dB/oct vs 1.7-3.5; about 50 dB over C4-C7)**: FAULT. Real flutes gain maybe 15-20 dB over the range. This needs a per-note level evening like the choir's `PJ_CHOIR_EVEN`.
- **Quieter overall (-3.1 dB vs +1.8..+3.9); key level scatter 3.5 dB; legato scale uneven 4.2 dB**: FAULT, the same root (low notes far too soft).
- **Brighter with pitch (5.1 ERB/oct vs 0.4-2.4)**: follows from the jet overblowing more at the top. Partly DELIBERATE physics (README Status, the jet).
- **Release 634 ms**: the room. Onset (55 ms) and pitch are inside the fonts.

## p074
- **Uneven from key to key (level 5.1 dB vs 0.04-1.1; brightness 2.1 ERB; pitch scatter 15 c vs 0.1-7)**: partly MEASUREMENT ARTIFACT. 5 of 13 probe keys lie outside F4-E6 and fold by an octave. The rest is a known FAULT: README Status lists A#5, C#6 and D6 as off (second register barely leaving the first mode).
- **Louder toward the top (6.7 dB/oct vs 0.8-2.7)**: a duct's breath sets pitch and level together (README "Other woodwinds", "A duct"). Partly physics, but steeper than any font.
- **Breathier (flatness -29 vs -43..-66 dB; fundamental share 0.83 vs 0.94-0.99)**: plausible chiff. Minor.
- **Release 414 ms**: the room.

## p075
- **Louder toward the bottom (-5.6 dB/oct vs -0.4..+1.8); quieter (-5.1 dB); legato scale uneven 4.7 dB**: FAULT. The high pipes are weak. Probe keys 60-66 fold up an octave, which hides part of it.
- **Key-to-key scatter (brightness 1.8 ERB, level 2.8 dB)**: FAULT, per-pipe levels not evened (one stopped pipe per note).
- **Brightens less up the keyboard (1.6 vs 2.8-3.4 ERB/oct)**: partly the folding (ARTIFACT).
- **Release 592 ms, staccato 394 ms**: the room. Pitch is fine (README Status: "all 30 in tune").

## p076
- **Much louder toward the top (18.2 dB/oct vs -1.1..+0.3; key level scatter 2.6 dB)**: FAULT. Keys 87-96 fold down into C4-C6, which should flatten the slope, so the small high bottles are genuinely much louder. Even out the per-bottle levels.
- **Legato separated and uneven (dip 12.6 dB vs 4-7.7; evenness 5.7 dB)**: the dip is DELIBERATE physics, since each note is another bottle and cannot be slurred. The unevenness is the level FAULT above.
- **Slightly darker (12.1 vs 13.0-15.3 ERB)**: small, inside the fonts' spread on the bands.
- **Release 398 ms**: the room.

## p077
- **Key-to-key level scatter 6.3 dB vs 0.2-1.0**: known FAULT. The probe hits D#4 (63, -76 c) and D#5 (75, silent), both listed in README Status. Keys 87-96 also fold.
- **Onset 109 ms vs 33-38; attack 11.7 ERB brighter than the body**: a long breathy chiff before the tone. A breathy attack is DELIBERATE idiom (end-blown, README "Other woodwinds"), but this one is too long and too loud: probable FAULT.
- **Darker (11.3 vs 13.2-15.5 ERB); more partials 2-4 (0.26 vs 0.01-0.08)**: debatable. Real shakuhachi tone is breathy and hollow; the fonts disagree.
- **Stereo coherence 0.46, release 572 ms**: the room (ARTIFACT).

## p079
- **Key-to-key level scatter 8 dB vs 0.06-1.3; brightness scatter 1.8 ERB; pitch scatter 22 c vs 0.4-2.2**: partly MEASUREMENT ARTIFACT, since 6 of 13 probe keys lie outside A4-F6 and fold. The pitch scatter is a FAULT: the duct's "louder is sharper" is not held at velocity 96 (README Status only lists F5 -22 c).
- **Noisy and impure (flatness -18 dB vs -40..-80; fundamental share 0.61 vs ≥0.997)**: probable FAULT. An ocarina is close to a sine; ours is very breathy.
- **Quieter (-4.7 dB vs +2.8..+8.2)**: FAULT, the level trim.
- **Louder toward the bottom (-4.6 dB/oct)**: partly folding (ARTIFACT).
- **Release 279 ms**: the room.
## p080
- **Not a square (odd/even −2 dB vs +50…+60; fundamental share 0.74 vs 0.86…0.92)**: DELIBERATE pulse-width sweep (README "Synth leads", pad table: threshold swept 35–65 % at 0.6 Hz), but averaged over the sweep the even partials are as strong as the odd ones. synth_leads.md §4 asked for "hollow, odd harmonics", so the default sweep is too wide or off-centre for GM 81. Possible FAULT.
- **Soft front (attack 44 ms, phrase onset 36 ms vs 1…3 / −3…8)**: a lead should snap. Partly the room's build-up (notes_a p002), but likely FAULT in the default "hardness".
- **Quieter (−0.4 vs +3.3…+5.3 dB), darker above 6 kHz (−11…−19 dB)**: a trim; the ladder's default cutoff.
- **Key scatter 2.8 vs 0.4…1.1 dB; release 588 ms**: room ARTIFACT (see p102).

## p081
- **Brightens with velocity (+2.1 vs 0 ERB)**: DELIBERATE: the dynamics knob drives level and cutoff (README "Synth leads", Controls). FluidSynth renders of the fonts don't brighten either, so this is a real difference, but the analog one.
- **Darker above 6 kHz (−22…−30 dB) with the same overall centroid (10.7 vs 10.6…12.0 ERB)**: the ladder. TimGM6mb and Jnsgm2 are ~2 ERB darker under FluidSynth (their filter envelopes), so the gap is overstated.
- **Phrase onset 30 ms vs −4…21**: the slur/glide logic or the room. Check before calling it a fault.
- **Key scatter 2.0 vs 0.3…0.6 dB, release 586 ms, floor −69 dB**: room ARTIFACT plus the per-voice board.
- Rank 73: close overall.

## p082
- **Purer and hollower (fundamental share 0.98 vs 0.69…0.84; odd/even +24 vs −4…+7 dB)**: DELIBERATE near-sine from a triangle into a differential pair (synth_leads.md §10 table).
- **Not breathy (flatness −63 vs −34…−47 dB)**: FAULT against our own spec. synth_leads.md §4 asks for "near-sine with noise", but the pitched breath is far below the fonts' chiff.
- **Key scatter (3.8 vs 0.6…1.4 dB)**: a near-sine in the hall cancels or adds key by key (README "FX 7": 3.9 dB in the hall, 0.5 dry). Room ARTIFACT.
- **Phrase onset 52 ms vs −2…14; staccato 296 ms**: possible slow speech, plus the room.

## p083
- **Near-sine body (fundamental share 0.95 vs 0.73…0.79; −7…−9 dB at 1–2.5 kHz)**: DELIBERATE: "the sine with a little pulse" (synth_leads.md §10). It is mellower than every font, so maybe a bit more pulse.
- **Faster attack (25 vs 38…99 ms) and brighter attack (+1.1 ERB)**: the chiff resonator works as planned.
- **Key scatter (3.2 vs 0.2…1.5 dB)**: near-sine in the room (see p082). ARTIFACT.
- **Release 466 ms, floor −72 dB**: room ARTIFACT. FluidR3 brightens +2.0 ERB with velocity under FluidSynth, so our +0.36 isn't out of line.
- Rank 89: one of the closer leads.

## p084
- **Lacks the fuzz (centroid 11.5 vs 14.4…18.3 ERB; −17…−25 dB above 6 kHz; partials 5+ share 0.01 vs 0.09…0.36)**: likely FAULT. An overdriven guitar-like lead (synth_leads.md §4) should be brighter than a saw, and this is darker. FluidR3 is 4 ERB darker under FluidSynth, but the other three are not. Raise the cutoff after the diode stage or weaken the 650 Hz scoop.
- **Brightens with velocity (+1.7 ERB)**: DELIBERATE (the dynamics knob).
- **Phrase onset 50 ms vs −2…1**: as on the other leads. Check the slur.
- **Release 619 ms, key scatter 2.4 dB**: room.

## p085
- **Brightens with velocity (+1.6 vs 0 ERB)**: DELIBERATE: "oo" on a soft key to "ah" on a hard one (synth_leads.md §10 table).
- **Brightness rises slower up the keys (2.2 vs 3.5…3.7 ERB/oct)**: DELIBERATE consequence of fixed formants. A voice does this too.
- **Late in a phrase (65 vs −1…10 ms)**: probable FAULT. The largest phrase delay among the leads, and this voice should be quick.
- **Quieter (−1.3 vs +1.9…+6.1 dB)**: a trim.
- **Release 772 ms, floor −63 dB, coherence 0.67**: the room plus the bucket-brigade chorus. ARTIFACT or by design.

## p086
- **Brightens with velocity (+1.9 vs 0 ERB) and brighter (11.0 vs 8.6…10.2 ERB)**: DELIBERATE: the dynamics knob and the sine wavefolder.
- **Smoother (Plomp-Levelt −3.4 vs −1.0…−2.7)**: DELIBERATE. The fifth is locked at 3:2 within 1 c by soft sync (synth_leads.md §10), where the fonts are detuned stacks.
- **Phrase onset 56 vs −2…−1 ms**: the same lead-family delay as p081–p085.
- **Floor −69 dB, key scatter 1.5 dB**: room.
- Rank 82, and the fonts agree closely (0.6 JND): close.

## p087
- **Darker (6.95 vs 9.2…10.2 ERB; +4…+9 dB under 150 Hz)**: FAULT for GM 88. The ÷2 and ÷4 flip-flops (synth_leads.md §10) let the sub-octaves swamp the "bright lead" on top that §4 asks for.
- **Slow attack (97 vs 2…8 ms; phrase onset 44 vs −2…0)**: FAULT. It is the slowest lead, and too slow for a bass+lead stab. The room explains only part of it.
- **Quieter (+0.5 vs +3.5…+6.5 dB), noisier (flatness −47 vs −59…−64 dB)**: a trim; the noise is probably the ring modulator's hum and board noise.
- **Release 565 vs 5…31 ms**: room ARTIFACT.

## p088
- **Speaks later (onset 162 vs 3…81 ms; attack 328 vs 7…763)**: borderline. The fonts disagree widely (SGM 763 ms). In a phrase it is quicker than every font (10 vs 35…62 ms), so it is not a fault.
- **Fundamental share 0.98 vs 0.72…0.78; odd/even +3.4 vs −5…−11 dB**: the bell is ring-modulated at 2.76×, which is inharmonic, so the harmonic measures miss it. A MEASUREMENT ARTIFACT, partly.
- **Brightens with velocity (+0.6 ERB), key scatter 1.4 dB**: small; the dynamics knob and the room.
- Rank 109: among the closest programs.

## p089
- **Release 866 vs 412…507 ms**: the fonts' pads already release in ~0.45 s, and the room adds ~0.3–0.4 s. ARTIFACT.
- **Darker above 6 kHz (−22…−27 dB re FluidR3 and SGM)**: DELIBERATE warmth: the saturating output transformer and the ladder (README pad table). TimGM6mb and Jnsgm2 have nothing up there, so the fonts disagree.
- **Brightens with velocity (+0.8 ERB), key scatter 2.2 vs 0.6…1.2 dB**: the dynamics knob, and the transformer compressing low notes; plus the room.
- Rank 106: close.

## p090
- **Thin bass, louder toward the top (+1.55 vs −0.3…+0.6 dB/oct; brightness rises 2.5 vs 3.0…3.8 ERB/oct)**: DELIBERATE consequence of the 140 Hz RC high-pass before the ladder (README pad table; synth_leads.md "The pads").
- **Phrase onset 66 vs −2…17 ms**: the lead family's slur delay again. Check it.
- **Brightens with velocity (+1.4 ERB)**: DELIBERATE (the dynamics knob).
- **Quieter (−1.2 vs +0.8…+3.6 dB)**: a trim.
- Rank 107: close.

## p091
- **Far too bright and thin (centroid 18.6 vs 7.3…10.8 ERB; −26…−34 dB under 150 Hz, +10…+23 dB above 6 kHz; fundamental share 0.03 vs 0.19…0.92; flatness −27 dB)**: FAULT. Puhuja's singers on a pad should not peak near 4–5 kHz. Suspect the breath (K3) default or the tract's radiation, and choir.md 1.3 already finds the /a/ too open and front. Listen first.
- **Slow (attack 518 vs 102…149 ms; onset 279 vs 18…35 ms; phrase 108 ms)**: borderline FAULT. choir.md 1.2 sets `ampA` 0.25 s, slower than every font's choir pad.
- **Quieter (−3.1 vs +0.1…+2.5 dB)**: a trim. The early level window also catches the slow attack.
- Rank 24.

## p092
- **Slow (onset 270 vs 48…96 ms; attack 544 vs 146…243 ms)**: probable FAULT. The README pad table doesn't ask for a slow bow, and this is twice the slowest font.
- **Uneven (key scatter 2.7 vs 0.4…0.7 dB; legato scale 2.9 vs 0.7…1.5 dB)**: probably the fixed formant bank at 280/460/1150/2700 Hz meeting each key's harmonics differently. Partly DELIBERATE (a body), partly the room.
- **Quieter (−6.1 vs −3.0…+2.3 dB)**: a trim, inflated by the slow attack in the early window.
- **No vibrato (1.6 vs 8.5…31 c), partials moving as one (0.96)**: DELIBERATE, one core through filters. But a bowed pad would gain from some motion.

## p093
- **Holds (decay 0.6 vs 4.1…8.7 dB/s; −6 vs −11…−19 dB after 2 s)**: DELIBERATE. Two of the four pipes are bowed and swell in over ~1 s (README pad table).
- **Speaks sooner and sharper (onset 44 vs 216…356 ms; 16 vs 6…8 dB/ms) but the full attack is slower (762 vs 449…498 ms)**: DELIBERATE: felt hammers strike two pipes, then the bowed ones swell.
- **Key scatter 3.5 vs 1.2…1.6 dB**: DELIBERATE. Each key's highest pipe has its own ratio (synth_leads.md "Bowing, the room, the per-key pipe").
- **Darker above 6 kHz re the TiMidity fonts (−13…−19 dB), −2 re SGM**: the fonts disagree. Not an issue.

## p094
- **A formant voice where the fonts have a dark near-sine (centroid 13.6 vs 7.5…8.6 ERB; fundamental share 0.31 vs 0.88…0.93)**: DELIBERATE: the five-formant cascade, whose compressors lift F3–F5 (synth_leads.md "pad-halo"). It is the farthest program from the fonts (rank 1). Consider a darker default (lower formant ranges or less lift) if GM files use Halo as a soft bed.
- **Brightness scatter 1.8 vs 0.3…0.6 ERB**: DELIBERATE: per-key Halton formants.
- **Slow (onset 422 vs 16…104 ms; phrase 130 ms)**: FAULT for GM use. The 1.5 s compressor integrators are the likely cause.
- **Quieter (−7.3 vs −1.3…+2.1 dB)**: FAULT (a trim). Part of it is the early window over the slow onset.

## p095
- **Key scatter (4.4 vs 0.4…2.2 dB), brightness scatter 1.3 ERB**: DELIBERATE. The 0.12 Hz phaser runs free, so each key starts at another phase of the sweep. SGM shows the same at 2.2 dB.
- **Less vibrato (12.5 vs 17…31 c), smoother**: the default vibrato knob (~42/127; synth_leads.md §10). Minor.
- **Brightens with velocity (+0.8 ERB)**: the dynamics knob.
- Rank 113, and the fonts disagree (1.8 JND): about as close as a sweep pad gets.

## p096
- **Key scatter (3.1 vs 0.2…0.5 dB), brightness scatter 1.9 ERB**: DELIBERATE: each note gets its own random rain (README "FX 1 (rain)"; fx_sounds.md FX 1 comparison).
- **Darker when harder (−1.0 vs 0 ERB)**: DELIBERATE: large drops have a longer contact.
- **Noisier (flatness −28 vs −37…−46 dB), wide (coherence 0.47)**: DELIBERATE: the bubbles above 6 kHz; random drops plus the room.
- **Sharp onset (40 dB/ms)**: a drop on glass. SGM is 49, the TiMidity fonts 3…7: the fonts disagree (1.9 JND).
- Rank 105: close. The decay and release match (fx_sounds.md).

## p097
- **Quieter (−23 vs −1.8…+2.6 dB) and late (onset 778 vs 107…238 ms)**: the level is read in the first ~0.3–0.5 s, before the strings bloom (~0.8 s), so the number is mostly a MEASUREMENT ARTIFACT. The slowness behind it is a known limitation (fx_sounds.md FX 2 "Limitations": fast passages sound late and soft; K2 speeds it). For GM files, a faster default swell is worth considering.
- **Narrow velocity range (7.6 vs 16…20 dB); uneven legato scale (7.5 vs 1.2…3 dB)**: the same cause. Held for 6 s it spans 25 dB (fx_sounds.md).
- **Smoother, partials moving as one (0.83 vs ~0); key scatter 2.9 dB**: DELIBERATE: locked strings that beat.

## p098
- **Pitch wobble 32 vs 0…15 c**: MEASUREMENT ARTIFACT. The beating 1/1.022/1.164 cluster fools the tracker (synth_leads.md "FX 3").
- **Fundamental share 0.38 vs 0.83…1.0; −17…−26 dB under 150 Hz**: the plate's inharmonic modes. The harmonic measures are meaningless here, and the fonts' near-sine crystals are a different idea. DELIBERATE.
- **Brightens with velocity (+2.2 ERB), key scatter 2.4 vs 0.04…0.6 dB**: DELIBERATE: the felt hardens, and the strikes land at random places.
- **Sustains more (−20 vs −23…−33 dB after 2 s)**: DELIBERATE: the bow keeps it singing.
- The 1.164 mode carries 17 % of the energy, a cluster ~2.6 semitones up. Listen for whether the note reads clearly.

## p099
- **Key scatter (3.1 vs 0.6…1.5 dB), brightness scatter 1.5 vs 0.6…0.9 ERB**: DELIBERATE. Each course beats its own way (README "FX 4 (atmosphere)").
- **Wide (coherence 0.29 vs 1)**: room ARTIFACT (README).
- **A little darker (8.4 vs 9.2…10.8 ERB) and purer (flatness −61 vs −37…−51 dB)**: DELIBERATE. Locked partials against the fonts' detuned pad clusters. The README's earlier run had it inside the range, so this is minor.
- **Phrase onset 37 vs 1…14 ms; velocity range 13.9 vs 16.7…21 dB**: minor. The plectrum's press (3–8 ms) plus the room.
- **No vibrato (4 vs 14…30 c)**: DELIBERATE.

## p100
- **Smoother (Plomp-Levelt −4.3 vs −1.8…−2.8), no vibrato (4.8 vs 22…51 c)**: DELIBERATE. Exact harmonics from the arc, where the samples are detuned clusters with an LFO (README "FX 5 (brightness)").
- **Key scatter 1.7 vs 0.3…0.9 dB**: DELIBERATE: the two arcs beat, each key in its own phase.
- **Phrase onset 38 vs −4…7 ms**: the room and the bloom of soft treble notes (README). Minor.
- **Holds (−5.7 dB after 2 s)**: like FluidR3 (−8.4); the others fall to −21…−28. The fonts disagree (2.4 JND).
- Rank 114: the closest of my programs.

## p101
- **Quieter (−29 vs −8…−10 dB)**: the fonts swell too, but ours starts 20 dB lower. Its oscillation grows out of the lighting (onset 938 vs 538…690 ms), and the early level window misses it. Partly a MEASUREMENT ARTIFACT, but the effect is real: notes under ~1 s nearly vanish. The trim came from 1.5 s notes (fx_sounds.md FX 6 "Measurements"). FAULT for GM use.
- **Brighter (12.8 vs 7.0…9.4 ERB), brightness scatter 2.5 vs 0.2…0.6 ERB**: DELIBERATE: the pipe's upper partials, and each flame puffs in its own phase (README "FX 6 (goblins)").
- **Duller attack (−4.5 ERB), darker when harder (−1.4 ERB)**: DELIBERATE. It opens as it warms. TiMidity also drops the fonts' ~9 s filter sweep.

## p102
- **It echoes**: DELIBERATE. The fonts' "Echo Drops" don't echo at all (README "FX 7 (echoes)"; fx_sounds.md FX 7).
- **Key scatter 4.0 vs 0.5…1.6 dB**: room ARTIFACT. A near-sine and its repeats add or cancel in the hall (0.5 dB dry, README).
- **Phrase onset 104 vs 0…33 ms**: probably a MEASUREMENT ARTIFACT. The 0.335 s repeats of the previous note overlap the 0.4 s phrase steps, and a struck fork otherwise speaks quickly (attack 132 vs 208…543 ms).
- **Near-sine, no breath (fundamental share 0.99; flatness −81 vs −29…−54 dB; −10…−25 dB above 2.5 kHz); highs lead by 35 ms**: DELIBERATE: a fork with a fast-dying clang, against the fonts' choir/synth voices.

## p103
- **Dies away (−50 vs −8…−18 dB after 2 s; decay 25 vs 0.3…7 dB/s; staccato 148 vs 275…396 ms)**: DELIBERATE. A shot and a ping whose T60 is ~2.3 s at middle C (README "FX 8 (sci-fi)", K4), against the fonts' held pads. K4 lengthens it.
- **Narrow velocity range (6.7 vs 17…21 dB; −5 dB at v40)**: probable FAULT. Velocity drives the fold, the drop and the ping's index, but hardly the loudness. GM files expect quiet notes to be quiet.
- **Sharp onset (57 vs 4.5…11 dB/ms), brightens with velocity (+2.6 ERB)**: DELIBERATE: a discharge.
- Effects: the fonts disagree (1.5 JND), and the pitch measures mean little here.
## p104
- **Louder (+6.1 dB vs −3.6..+0.4 against the other instruments)**: FAULT, small. README "Ethnic and percussive (modes)", Levels, sets every modal instrument to one common loudness; the fonts keep the sitar lower in a mix.
- **Uneven onsets in a phrase (jitter 42.6 ms vs 1.6–8.4)**: probably a MEASUREMENT ARTIFACT. The phrase is at velocity 80, so no meend (that needs ≤ 45, src/modalinst.cc). The jawari buzz blooms ~0.3 s after the stroke (README), and the steepest-rise detector can lock onto the bloom. Listen before fixing.
- **Key-to-key level scatter (3.1 vs 0.06–0.62 dB), jagged overtones (6.8 vs 1.9–3.9 dB)**: mostly DELIBERATE: the jod/main-string split at F#3, the tarafs ringing on some keys, and the jawari spectrum (sitar_shamisen.md 1.2, 1.5). Probe keys 36–44 fold up an octave into the 48–84 range.
- **Brightens with velocity (+1.58 vs 0–0.39 ERB)**: DELIBERATE. A harder stroke, plus the chikari above velocity 90. The fonts' 0 is TiMidity's convention.

## p105
- **Pitch scatter (11.6 c vs 0.7–4.3)**: probable FAULT. That is audible in chords, and README `make test` only checks three notes to ~7 c. Check the stopped-string retuning (`stopString`) against the light bridge and head.
- **Rough, bright, sharp onset (dissonance −0.04 vs −3.2..−4.5 log2; tristimulus 3 0.64 vs 0.10–0.26; onset 51 vs 11–18 dB/ms)**: DELIBERATE. The Mylar head's 500–800 Hz formant and fast death are what a banjo is (README "Banjo"; banjo_koto.md 2.2–2.3). The fonts are tamer.
- **Louder (+4.6 dB vs −6.6..+0.2)**: the common-loudness rule. A small trim would match GM balance.
- **Stereo coherence 0.43**: room ARTIFACT.

## p106
- **Louder (+4.4 dB vs −7.8..−4.8)**: FAULT. The fonts agree closely (they disagree by only 0.8 JND), so this is a trim of ~9 dB against the GM balance.
- **Darker (10.3 vs 14.2–14.4 ERB) with +13..+15 dB below 150 Hz, but strong high partials (tristimulus 3 0.46 vs 0.06–0.10)**: partly DELIBERATE. The skin's low modes and the bachi's knock on the skin (README "Shamisen"; sitar_shamisen.md 2.3–2.4) put weight at the bottom; the sawari buzz feeds the top. All four fonts sit ~4 ERB brighter, a thin plucked twang. Worth a listen: the boom may be too heavy.
- **Pitch wobble 21 c vs 0–1**: sawari contact and tension modulation. DELIBERATE, though large.
- **Stereo coherence 0.35**: room ARTIFACT.

## p107
- **Darker and duller attack (10.4 vs 13.6–18.3 ERB; attack −1.4 vs +0.5..+3.5 ERB re body; −7..−17 dB above 6 kHz)**: probable FAULT. A koto played with ivory tsume should have a bright, twangy attack (banjo_koto.md 7.1). The pluck (`rise 0.002`) seems too soft.
- **Uneven onsets in a phrase (jitter 38.5 ms vs 0.7–1.5)**: check. The E-major phrase (keys 52–61) falls off hirajōshi: 4 of the 6 notes are pressed (oshide) or on a moved bridge, and two pairs share a string. A string still sounding is damped and replucked 20 ms into the next note.
- **Rings 1268 ms after the key (vs 562–812)**: DELIBERATE (the strings are not damped on key release) plus the room.
- **Louder (−0.4 vs −5.1..−2.1 dB)**: a small trim.

## p108
- **Speaks late (phrase onset 28 ms vs −3..0; onset delay 31 ms; jitter 29.7 ms)**: borderline FAULT. README "Kalimba" calls for a slow press; the press is the pluck's 20 ms `rise`. Real, but in a band it sounds behind the beat. Halving it would keep the character.
- **Brighter attack (+5.5 vs 0.3–1.6 ERB) and brighter overall (8.1 vs 7.0 ERB)**: DELIBERATE: the nail's quick release (kalimba_fiddle.md A6). The −30..−40 dB below 150 Hz is a fold ARTIFACT: probe keys 36–44 fold up into the 48–88 compass.
- **Rings on: 100 ms note sounds 400 ms (vs 45–204), decay 20.9 vs 30–47 dB/s**: DELIBERATE. A tine cannot be damped by letting the key go.
- **Brightness scatter 1.33 vs 0.21–0.60 ERB**: per-tine tuning and support. Minor.

## p109
- **Darker, hollow, fundamental-heavy (8.3 vs 16.2–18.3 ERB; odd/even +38 vs −2.8..+1.3 dB; tristimulus 1 0.68 vs 0.01–0.16)**: MEASUREMENT ARTIFACT. The chanter covers only G4–A5 (67–81, src/designs.cc). Probe keys 36–64 and the whole phrase (52–61) play the **drones**, which are cylindrical pipes with odd harmonics (README "Ethnic and percussive", drone paragraph). Transpose is 0, so there is no transposed-wind shift.
- **Brightness scatter 2.7 ERB, speaks late (205 vs 10–37 ms)**: the same drone/chanter split, and the drone reed's build-up.
- **Design question**: a GM bagpipe melody written below G4 becomes a drone at that pitch, not a chanter note. That is DELIBERATE, but the timbre jumps at G4.
- **Release 702 ms**: room ARTIFACT.

## p110
- **Missing low end, far too bright (centroid 23.3 vs 9.9–15.3 ERB; −30..−66 dB below 400 Hz; tristimulus 3 0.94 vs 0.04–0.19)**: FAULT, the largest of this group. The fundamental is almost absent. pajupilli's own violin sits inside the fonts (18.3 vs 15.7–19.9 ERB), so the fiddle's waveguide bridge-force-to-body output (README "Fiddle") loses the low string modes or over-weights the bridge hill.
- **Brightness flat over the keyboard (−0.04 vs 3.2–4.3 ERB/oct)**: the same fault. The fixed high band dominates every note.
- **Quieter (−3.9 vs −1.2..+2.8 dB)**: follows from the missing fundamental.
- **Phrase onset 41 ms, release 595 ms**: bow build-up (the fonts span −3..18 ms) and the room.

## p111
- **Slow to speak (attack 222 vs 15–40 ms; onset 104 vs 12–32 ms; phrase onset 64 ms vs 0; a 100 ms note sounds 400 vs 40–121 ms)**: FAULT. A shehnai is played with crisp tonguing; this one blurs fast passages.
- **Key-to-key level scatter 5.9 vs 0.3–1.8 dB, pitch scatter 14.2 vs 0–1.5 c (legato evenness 5.2 vs 0.5–2.3 dB)**: FAULT. The per-note design (kShehnaiNotes, C4–G5) has weak or out-of-tune fingerings. The folded probe keys reuse the same notes, so this is not a range artifact.
- **Attack 8.2 ERB brighter than the body (vs −0.6..+0.4)**: a squeal before the note settles. Likely the same reed set-up fault.
- Transpose is 0 (src/designs.cc). Release 686 ms: room ARTIFACT.

## p112
- **Legato dip 17.9 vs 5–9 dB, phrase onset 22 ms, attack 24.5 vs 1–16 ms**: DELIBERATE, but debatable for GM. Below C5 the tinkle bell is a church bell struck by soft felt that "blooms" (README "Tinkle bell"), and the phrase (keys 52–61) lies entirely there. The GM Tinkle Bell is a small bell everywhere. Consider moving the split lower.
- **Level scatter 2.74 vs 0.2–0.57 dB, brightness scatter 1.13 vs 0.23–0.33 ERB, tristimulus 2 0.32 vs 0–0.03**: the same handbell/church-bell split at key 72.
- **Brightens with velocity (+1.1 vs ≈0 ERB)**: DELIBERATE: the Hertz contact.
- **Release 2000 ms**: the bell rings on (DELIBERATE), plus the room.

## p113
- **Click dominates the ring (a 100 ms note sounds 11 ms vs 66–150; flatness −11.8 vs −38..−48 dB)**: probable FAULT. The ring decays more slowly than the fonts' (34 vs 49–79 dB/s), so the strike transient must sit ~20 dB over it. The stick tip (`PJ_AGOGO_TIP`, K 3e9, ~0.05 ms contact) is too hard against the 0.08 kg bell mass. The agogo should clank and ring, not tick.
- **Louder toward the top (+3.3 vs −1.5..−3.3 dB/oct), brightness scatter 1.97 vs 0.26–0.49 ERB**: worth a check, but secondary.
- **Tristimulus 1/2 (0.47 vs 0; 0.02 vs 0.7–0.99)**: MEASUREMENT ARTIFACT: an inharmonic bell has no meaningful partial series.

## p114
- **A scale that drops out (legato evenness 48.8 vs 0.2–0.8 dB; 16 silent probe notes)**: FAULT. `steel-drums` declares 52–90 (src/modalinst.cc), but note areas exist only for keys 60–90 (`vib = midi - 60; if (vib < 0) return`). Keys 52–59 are silent: probe keys 40, 44, 52 and 56 (folded) and five of the six phrase notes. Fix: lo = 60 (folding then works) or add note areas below C4.
- **Brightens with velocity (+1.62 vs 0–0.03 ERB)**: DELIBERATE. The stick contact, and the octave mode taking energy (Monteil; README "Steel drums").
- **Brightens slowly up the keys (1.36 vs 3.7–4.0 ERB/oct)**: partly the duplicated folded keys.
- **Stereo coherence 0.29**: room ARTIFACT.

## p115
- **Louder (+1.6 vs −11.8..−7.1 dB against the other instruments)**: FAULT for GM balance. The common-loudness rule (README, Levels) puts the woodblock 9–13 dB above where all four fonts agree it belongs.
- **Shorter (staccato 16 vs 27–43 ms)**: DELIBERATE: the block decays in tens of ms (README "Woodblock"; bells_blocks.md 3.2).
- **Sustains more after 2 s (−83 vs −111..−125 dB), stereo coherence 0.40**: room ARTIFACT.
- **Brightens with velocity (+0.68 ERB)**: DELIBERATE: the Hertz contact.

## p116
- **Brighter, without the boom (7.3 vs 2.8–3.1 ERB; −19..−28 dB below 150 Hz)**: probable FAULT for GM use. The drum's f0 follows the key (`hz(midi)`, range 24–72), so the probe's 52–72 is a 165–520 Hz drum. The fonts play one big o-daiko whatever the key (their pitch scatter is 41–64 c). Consider limited key tracking around the 68–135 Hz sizes in steelpan_taiko.md 2.6.
- **Key-to-key level scatter 4.7 vs 0.03–0.74 dB**: the same key mapping, and probe keys 76–84 fold down.
- **Evenly tuned (3 c), less wobble (56 vs 87–102 c)**: DELIBERATE. The glide is ≤ a semitone (README "Taiko"). The fonts' pitch numbers are unreliable on a drum.
- **Stereo coherence 0.19**: room ARTIFACT.

## p117
- **Slow attack (43.5 vs 4–14 ms)**: probable FAULT. A tom should peak at once. The ampA is 1 ms (src/lead.cc), so the peak probably comes late as the falling pitch (4 semitones, 120 ms) passes the filter. Look at the first 50 ms.
- **Brighter (7.3 vs 4.2–5.1 ERB), +8..+12 dB at 2.5–6 kHz**: the stick-noise chiff (0.6). Debatable.
- **A 100 ms note sounds 212 vs 92–122 ms; release 2000 ms**: relMul 0.5 plus the room. Partly ARTIFACT.
- **Odd/even +22.5 dB, jaggedness 8 dB**: MEASUREMENT ARTIFACT: a sine has almost no partials.
- **Evenly tuned (4 c vs 28–53)**: DELIBERATE.

## p118
- **Brighter (7.1 vs 4.0–5.0 ERB; +10..+19 dB at 400 Hz–1 kHz, −13..−23 dB at 150–400 Hz)**: DELIBERATE design (an analog drum through a resonant filter, src/lead.cc), but it tracks the key more steeply than the fonts (3.5 vs 0.9–1.4 ERB/oct). Debatable: the fonts' synth drum stays low and thuddy on every key.
- **Less pitch sweep (wobble 36 vs 87–98 c)**: the 12-semitone sweep over 45 ms reads as less than the fonts' longer drop. Consider a slower `pitchTau`.
- **Sustains more (−65 vs −108..−130 dB), release 2000 ms**: room ARTIFACT.
- **Evenly tuned (0.7 vs 23–50 c), loudness range 14 vs 17–20 dB**: DELIBERATE / minor.

## p119
- **Dark and tonal, not a hiss (12.8 vs 22.6–27.9 ERB; flatness −34.5 vs −9.5..−13.5 dB; +15..+33 dB below 400 Hz)**: FAULT. The pre-rendered crash (src/modalinst.cc) uses 192 modes **log-spaced** from 81 Hz to 14 kHz, so above ~2 kHz they are 100+ Hz apart instead of the ~14 Hz the comment intends, and the highs come out sparse and tonal. Every font's swell is a bright wash. Space the modes evenly in frequency, or reuse the drum kit's crash (cymbals_built.md).
- **Narrow loudness range (10.3 vs 16.7–20 dB)**: the `revGain = 0.3 + amp` floor. Minor FAULT.
- **Release 555 ms**: room ARTIFACT (note-off stops the swell at once).

## p120
- **Velocity hardly matters (loudness range 5.7 vs 16.6–20.1 dB; curve −6/−6/0/0)**: FAULT. In src/sfx.cc, velocity mainly shortens the slide, and the pluck amplitude is only `0.4 + vel`.
- **Speaks late in a phrase (54 ms vs −1..0), onset 86 ms**: DELIBERATE physics: the fingertip must slide before it squeaks (README "Sound effects"). For a GM effect it is late, though the fonts' onset delays range from 6 to 114 ms.
- **Sustains more (−50 vs −98..−123 dB), decays slower**: the string and guitar body ring after the squeak (DELIBERATE), plus the room.
- **Louder (−0.2 vs −11.7..−4.6 dB)**: trim. The fonts disagree widely (3.2 JND).

## p121
- **No velocity response (loudness range 0.3 vs 16.7–20.1 dB)**: FAULT. The tract's pressure follows velocity (`300 + 900 * vel_`), but something (the whisper's level or the velgain table) cancels it. Velocity does nothing audible.
- **A held whisper, not a puff (−3.5 dB after 2 s vs −81..−120)**: DELIBERATE (README "Sound effects": the vocal tract whispering while the key is held). The fonts play short breaths.
- **Brighter (32.5 vs 7.3–26 ERB)**: the fonts disagree enormously (2.8 JND). Not a verdict.
- **Partial envelopes alike 0.008**: MEASUREMENT ARTIFACT (noise).

## p122
- **Darker (13.1 vs 16.4–23.7 ERB), brighter attack (+3.9 vs −1.9..+0.2 ERB)**: DELIBERATE character: Minnaert bubbles and wash rather than the fonts' looped white-noise surf (README "Sound effects"). The fonts disagree widely (2.6 JND; TimGM6mb is 16.4).
- **Odd velocity curve (−9/−11/0/−1 dB; range 8.1 vs 9.3–20.1), level −5 dB/oct down the keys**: mostly a MEASUREMENT ARTIFACT. Wave timing is random, so a 2 s window catches crash or backwash by chance. Velocity sets the bubble rate, not a gain.
- **Louder (−0.6 vs −20.4..−5.1 dB)**: trim, though the fonts span 15 dB.

## p123
- **Starts sharp (+44 c vs 0–11)**: DELIBERATE. Each syllable sweeps down (README "Sound effects", after Gardner et al.).
- **Speaks later in a phrase (48 vs −2..6 ms)**: the pressure gesture's rise. Minor; the fonts' attacks range from 107 to 601 ms anyway.
- **Rings 583 ms after the key**: the bird finishes its syllable, plus the room. ARTIFACT.
- **Louder (+4.1 vs −5.1..+0.8 dB)**: small trim. The fonts disagree widely (3.1 JND), so this is the closest of the effects (rank 113).

## p124
- **Narrow loudness range (7.9 vs 16.6–20.1 dB)**: FAULT, mild. A real bell's loudness hardly depends on anything, but GM expects velocity to scale the effect.
- **Speaks late (onset 73 vs 0–34 ms; phrase 40 ms)**: DELIBERATE physics. The 20 Hz ring current must swing the armature (~18 Hz) into a gong (README "Sound effects").
- **Release 692 ms, floor −60 dB, stereo coherence 0.53**: the gongs ring on, plus the room. ARTIFACT.
- Pitch measures are meaningless here. The fonts disagree widely (2.8 JND).

## p125
- **Far too loud (+5.9 vs −24.4..−21.0 dB against the other instruments)**: FAULT, the largest balance error in the set. All four fonts agree within 3 dB that the helicopter sits ~22 dB under the median instrument; here it is set to the common loudness, so in a GM mix it is ~28 dB too hot.
- **Release 2000 ms**: DELIBERATE: the rotor spins down (README "Sound effects").
- **Narrow loudness range (9.8 vs 16.7–20.1 dB), purer (flatness −28.8 vs −11.9..−18.7 dB)**: the blade slap is periodic. Plausible; minor.
- **Darker with velocity (−0.66 ERB)**: minor.

## p126
- **Starts at once (onset 32 vs 412–498 ms; attack 435 vs 693–980 ms)**: DELIBERATE, but unlike GM. Every font fades the applause in over ~0.5 s; here the crowd claps from the key. Debatable: GM files expect the swell.
- **Darker (16.0 vs 18.9–23.2 ERB; −6..−15 dB at 2.5–6 kHz)**: the palm-cavity resonance (Peltola et al., README "Sound effects"). Plausible, perhaps slightly dull.
- **Louder (−0.4 vs −18.3..−5.8 dB)**: trim. The fonts span 12 dB.
- **Stereo coherence 0.35**: room ARTIFACT, and arguably right for a crowd.

## p127
- **A crack, not a boom (a 100 ms note sounds 19 vs 92–189 ms; onset 64 vs 4.5–16 dB/ms)**: DELIBERATE physics: an N-wave and a Friedlander blast last milliseconds (README "Sound effects"). The fonts' gunshots carry a recorded tail and echo. In a GM mix this sounds thin and small; consider a longer ground-reflection or report tail.
- **Louder (+5.6 vs −6.2..+1.7 dB)**: trim. The fonts disagree widely.
- **Pitch +26 c, smoother, less wobble**: MEASUREMENT ARTIFACT. A gunshot has no pitch.
## drums
The kit-level numbers mix very different pieces. The per-key table in this section is the useful
view. In it, the level is in dB against each synth's kit median and the brightness is the
centroid in ERB at velocity 96.

- **The splash cymbal (55) is far too dark.** 12.6 ERB, about 0.9 kHz, against 30–33 ERB (5–7 kHz)
  in all four fonts. FAULT, already known: research/cymbals_built.md §"The dome pile-up" finds
  that the splash's energy collects at 800–1600 Hz. The `PJ_CYM_DOME` workaround does not cure it.
  The crash cymbals are also darker (49: 22 against 27–32 ERB; 57: 26 against 28–34) and 3–9 dB
  quieter in the kit. The china (52) and the ride (51, 59) sit inside the fonts.
- **The guiro (73/74) is 7–9 dB louder in the kit and much darker** (8.5–8.8 against 19.5–30 ERB).
  research/scrapers_friction.md built it from silence. FAULT: the scrape lacks its high rasp. The
  vibraslap (58) is also darker (15 against 22–31).
- **Some quiet pieces:**
  - woodblocks 76/77 sit −12/−15 dB against −0 … −8 (FAULT, level trim);
  - side stick 37: −12 against −3 … −5;
  - hand clap 39: −10 against −5 … +2, and darker at 12.4 against 15–18 ERB
    (research/clap_claves_surdo.md);
  - cowbell 56: −6 against −1 … −2;
  - low conga 64: −4 against +4 … +6.
  These are level trims, and the most audible kit-balance faults in a groove.
- **The snares are darker:** 38 is 7.3 ERB against 8.8–15.9, and 40 is 7.0 against 8.6–18.6. The
  fonts disagree widely here, and research/stick_snare.md tunes against literature rather than
  samples. Probably DELIBERATE, worth an ear.
- **Kick and snare decay at about 32 dB/s against 54–193** in the fonts, and "sustain" ends 45 dB
  higher. MEASUREMENT ARTIFACT of the room: the decay is fitted over 2 s, which the room tail
  dominates.
- The kick (35/36), the toms (41–50) and the hi-hats (42/44/46) are within the fonts' spread in
  level and brightness. TimGM6mb and Jnsgm2 give no usable body for several short keys (marked
  "-"), so the cloud there has only two members.
