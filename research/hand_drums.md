# Congas, bongos and timbales: research, plan, and what was built

Status: 2026-11-15. General MIDI's hand drums: bongos (60 Hi Bongo, 61 Low Bongo), congas (62 Mute Hi
Conga, 63 Open Hi Conga, 64 Low Conga), timbales (65 High Timbale, 66 Low Timbale). In the user's MIDI
files they are the drums most played on stand-ins: about 13 000 notes in 26 files (Latin jazz, bossa,
the bushgrafts trio arrangements). Until now they went to `melodic-tom`, a circuit drum.

Marks: **[paper]**, **[web]** a source below; **[calc]** worked out here; **[memory]** general
knowledge, to be checked; **[design]** a choice of this plan.

## 1. Short answer

- **All three are single heads over an open-bottomed shell.** The head's air does not sit in a
  closed box (as on a tom or a kick). The shell is a pipe closed by the head and open at the foot.
  - A conga's 76 cm barrel is a long pipe: its first resonance, near 100 Hz, is the "bass tone" a
    palm in the middle of the head brings out.
  - A bongo's shell is short (15 cm): its pipe sits near the small drum's own note.
  - A timbale's steel shell (16 cm, open) adds its own metallic ring, struck directly on the side
    for the cáscara.
- **The pitch drummers tune by is the open tone:** four fingers near the rim **[web, Wikipedia;
  mycongaplace]**. Struck there, the (0,1) and the (1,1) family both speak. At a microphone the
  (0,1) carries: it moves the air as a whole, while the (1,1) pairs are dipoles. So each drum's
  note is put on its (0,1) as the air loads it. (The plan first said (1,1); the first renders
  showed the (0,1) as the strongest low peak, §6.)
- **The hand is a soft, heavy, lossy mallet that may stay on the head.**
  - Open tone: the fingers bounce off within ~1–2 ms.
  - Mute (closed) tone: they stay pressed and damp the head where they lie.
  - Slap: the cupped fingertips snap at the head, short and hard. The other hand or the heel of the
    hand on the head takes the low ring out.
- **A Membrane (src/membrane.h) carries the head.** What is new here: the pipe under it (a few
  acoustic modes, driven by the head's volume), the hand's contact (Hunt–Crossley flesh, staying for
  the mute), the timbale's steel shell (a few ring modes) and a stick. The sound is a microphone
  above the head plus the pipe's open foot.

## 2. The drums [memory unless marked]

| Drum | Head Ø | Shell | Head | GM key | Tuning used here (open tone) |
|---|---|---|---|---|---|
| quinto / conga (hi) | 11"–11.75" | 76 cm barrel, foot ~23 cm | rawhide 1–2 mm (or synthetic) | 62 mute, 63 open | C4, 262 Hz |
| tumba (low) | 12"–12.5" | 76 cm | rawhide | 64 open | G3, 196 Hz |
| bongo macho (hi) | 7" | 15 cm, open | thin rawhide ~0.8 mm | 60 | C5, 523 Hz |
| bongo hembra (low) | 8.5" | 15 cm, open | thin rawhide | 61 | G4, 392 Hz |
| timbal high | 14" | 16.5 cm steel, open | Mylar ~7–10 mil | 65 | E4, 330 Hz |
| timbal low | 15" | 16.5 cm steel, open | Mylar | 66 | B3, 247 Hz |

- **Conga tunings are taste.** Common practice is fourths (quinto a fourth over conga, tumba a
  fourth or a minor third under). Mongo Santamaría and Poncho Sánchez tuned to an inverted C major
  chord (E–G–C), and other drummers use tumba F3, conga C4, quinto G4 **[web, mycongaplace;
  Wikipedia]**. The defaults above are a fifth apart, C4 over G3, and the knobs move them.
- **Bongos are tuned high and tight, the pair about a fourth or fifth apart.** One player's macho is
  F5 over a hembra D4 **[web, forum]**. They are played with the fingers' tips and pads near the
  rim (the *martillo* pattern).
- **Timbales:** steel shells ring more and brighter than brass ones **[web, MasterClass; LP]**.
  - Heads are thin plastic, tuned high and ringing.
  - A pair, 14" and 15", is played with thin sticks.
  - Strokes: on the head; the rimshot (head and rim together); the paila or cáscara on the shell's
    side **[web, MasterClass]**.
- **Rawhide against plastic:** rawhide is heavier and much lossier (an animal skin, irregular) than
  Mylar. It sounds warm and short, with the highs gone in tens of ms; plastic rings longer and
  brighter.

## 3. Physics [calc unless marked]

### 3.1 The head

- An ideal membrane's modes (Membrane): (0,1), (1,1), (2,1), (0,2) … at 2.405, 3.832, 5.136,
  5.520 × c/(2πR), with c = √(T/σ).
- **Tension for the open tone on (1,1):** c = 2πR f₁₁ / 3.832, T = σ c².

| Drum | R (m) | σ (kg/m²) | f₁₁ (Hz) | c (m/s) | T (N/m) | ideal (0,1) (Hz) |
|---|---|---|---|---|---|---|
| hi conga | 0.149 | 1.6 | 262 | 64 | 6600 | 164 |
| low conga | 0.159 | 1.6 | 196 | 51 | 4200 | 123 |
| macho | 0.089 | 1.0 | 523 | 76 | 5800 | 328 |
| hembra | 0.108 | 1.0 | 392 | 69 | 4800 | 246 |
| high timbal | 0.178 | 0.22 | 330 | 96 | 2000 | 207 |
| low timbal | 0.190 | 0.22 | 247 | 77 | 1300 | 155 |

  σ: rawhide ~1100–1300 kg/m³ × 1.2–1.4 mm (congas), × 0.8 mm (bongos); Mylar 1380 kg/m³ ×
  0.18 mm plus a coat **[memory]**.
- **Air loading** lowers the (0,1) (Membrane's airMass, the piston's 8/3 ρR³), most on the large
  light timbale heads, least on heavy rawhide.
- **Damping:** rawhide 2–5 % of critical for the low modes and rising fast with frequency; plastic
  ~1 % (as the snare's batter **[jd]**).

### 3.2 The pipe under the head

- The shell is a pipe closed at the head and open at the foot. Its modes are quarter waves:
  f_n = (2n − 1) c / (4 L'), with L' = L + 0.6 a (a: the foot's radius):

| Drum | L (m) | foot a (m) | L' (m) | f₁, f₂, f₃ (Hz) |
|---|---|---|---|---|
| conga | 0.76 | 0.115 | 0.83 | 103, 310, 517 |
| bongo | 0.15 | 0.045 | 0.18 | 484, 1450 |
| timbal | 0.165 | 0.18 | 0.27 | 315, 950 |

- **Coupling:** the head's (0,n) modes sweep a volume V = Σ vol_k q_k (Membrane::volume). The pipe's
  modal pressure p_n obeys

      p_n'' + γ_n p_n' + ω_n² p_n = −(2 ρc² / (A L')) V''      (A: the pipe's section at the head)

  and their sum, p = Σ p_n, pushes back on the head (f_k += p vol_k, as DrumAir). In the low limit
  a closed box would give p = −ρc²/V_box · V; the open foot lets it out except near the pipe's
  resonances.
- **The bass tone:** a palm in the middle drives the (0,1) and so the head's volume, which rings
  the pipe's first mode. On the conga that is near 100 Hz, below the head's own (0,1).
- **The open foot radiates.** Mass conservation gives the foot's outflow as U_foot = U_head −
  (A L'/ρc²) d p̄/dt (p̄: the pipe's mean pressure). The head's top and the foot are opposite
  ends of the same air, and below the pipe's resonance they nearly cancel (a dipole). At the
  resonance the foot sings.
- **Losses:** wall and radiation losses of the pipe, Q ≈ 10–20 **[memory]**; the timbale's short
  wide shell is barely a pipe (L ≈ R), so its coupling is weak.

### 3.3 The hand

- **Mass:** the fingers striking near the rim, ~20–40 g effective (the hand's wrist leads, the
  fingers flex); the palm or heel, ~100–200 g **[memory]**.
- **Contact:** flesh over bone is soft and lossy: a Hunt–Crossley contact, k·δ^1.5 (1 + λ δ'), with
  k ~ 1e6 N/m^1.5 and λ ~ 1–3 s/m. A 30 g finger mass at 3 m/s then stays 1–2 ms (the open tone's
  click), a palm 4–8 ms (the bass tone) [calc, to fit].
- **Where:** an open tone lands 1–3 cm in from the rim, the fingers across a 6–8 cm band of the
  rim (averaged shapes, as the kick's felt). The bass tone lands at the centre, the slap 2–4 cm in.
- **Mute (closed) tone:** the fingers stay pressed (a few N) and damp. The flesh is a lossy spring
  on the patch (−c·v_patch, c ~ 20–60 N s/m), damping the modes that move under it. It is shorter
  and drier, with the pitch still there.
- **Slap (an option on the hi conga):** the fingertips, cupped, hit stiffer and shorter (~0.5 ms),
  while the heel of the hand lies on the head near the rim and takes out the low modes.

### 3.4 The timbale

- **Stick:** a thin timbale stick (~20 g at the tip), hickory on Mylar: contact 0.5–1 ms.
- **Steel shell:** the side rings when struck (the cáscara) and a little under every head stroke
  (the rim carries it). Its ring modes are a few, high and long, from ~1.5 kHz up **[memory, to be
  tuned by ear]**; a thin steel shell rings "sharp and clear" **[web, LP]**.
- **The rimshot:** head and rim at once, the head hit near the edge, the shell modes hard.

### 3.5 Published measurements

- **None found for congas, bongos or timbales** in three searches. There are a review of
  percussion acoustics (Rossing, holographic modal analysis), a banjo-head study (Politzer) showing
  the shell's air coupling to the head's up-and-down mode, and a hand-drum (udu) laser-vibrometry
  study **[paper]**.
- So the numbers here are calculated, and the sounds are to be judged by ear.

## 4. GM keys

| Key | Name | Played as |
|---|---|---|
| 60 | Hi Bongo | macho, open finger stroke near the rim |
| 61 | Low Bongo | hembra, the same |
| 62 | Mute Hi Conga | hi conga, closed tone: the fingers stay |
| 63 | Open Hi Conga | hi conga, open tone |
| 64 | Low Conga | tumba, open tone |
| 65 | High Timbale | high timbal, stick on the head |
| 66 | Low Timbale | low timbal, stick on the head |

The pair of each family is one instrument: two drums in one model. (The bass tone, a palm in
the middle, and the slap have no GM key; not built, §6.)

## 5. Plan (2026-11-15)

1. `src/handdrum.h/.cc`:
   - `HandDrum`: one head (Membrane), the pipe (3 quarter-wave modes), an optional steel shell
     (4 ring modes), a striker (hand or stick: mass, Hunt–Crossley contact, a return spring), the
     hand's stay (mute) as a lossy spring on the patch, and a microphone above the head (Rayleigh
     sum as kick.cc) plus the pipe's foot as a point source.
   - `HandDrumModel` (Tube): two drums. `HandDrumPlayer`: the GM keys.
2. Instruments `conga`, `bongo`, `timbale` (kPlucked rows, family "drum"), voice.cc lines, level
   trims at −28.2 LUFS (tools/levels.py convention).
3. Measure:
   - each drum's open-tone pitch against its tuning;
   - the conga's bass tone near the pipe's 100 Hz;
   - open against mute (decay at 0.1 s, at least 10 dB shorter);
   - velocity 16–127 at about 30 dB;
   - fuzz stability;
   - cost per drum.
4. A test function in tools/test.py; a README paragraph; the drumFor lines for 60–66 to the drum
   kit's owner.

## 6. What was built and measured (2026-11-16)

- **Files:** `src/handdrum.h/.cc` (HandDrum, HandDrumModel, HandDrumPlayer). Instruments `conga`,
  `bongo`, `timbale` (kPlucked, family "drum"). Test `hand_drums()` in tools/test.py.
- **Changes from the plan, found in the renders:**
  - **The note on the (0,1), not the (1,1).** The (0,1) as the air loads it is the strongest low
    peak at the microphone. The tension comes from one trial head (the air's share of the mass
    does not depend on the tension, so f ∝ √T exactly).
  - **Only the conga has a pipe.** On the bongos (15 cm shell, as wide) and the timbales (16.5 cm,
    36 cm across) the "pipe's" first mode, 484 and 315 Hz, sat on the drum's own note and pulled it
    a third away (523 → 425 Hz, 330 → 224 Hz). A shell that short and wide is not a pipe: its air
    is mostly a mass on the head, which the head's air loading stands for (`PJ_BONGO_PIPE`,
    `PJ_TIMBAL_PIPE` put it back).
  - **The conga's note with its pipe.** The barrel pulls the (0,1) too (−53 c on the hi conga,
    +38 c on the low, from below and above the pipe's 100 and 310 Hz). The tension is solved for
    the coupled note: for one head mode and the pipe's modes, w0² = w²(1 + κ Σ 1/(w_n² − w²)),
    κ = 2ρc² vol₀²/(A L′). That leaves both on pitch within 1 c.
  - **The striker's contact is implicit** (the dent solved against the head's and the striker's
    give in one sample, as hihat.cc's stick). The explicit form blew up on the light timbale
    head under a hard stick (1570 N in one sample: the head jumped 12 mm), and the head's
    stretch made it diverge at once. The membrane itself is stable (checked alone with the
    stretch at 0.9 J).
  - **Mute:** a damper on the patch to the arm (implicit), from 2 ms after the blow, for 0.2 s.
    The first form added the damper to the contact force and made the closed tone louder.
- **Measured** (velocity 100, `-seq`, microphone 0.35–0.5 m over the head):

| Drum | Note (Hz) | Measured | Strongest peaks (Hz, dB) | Level 100–150 ms against 0–50 ms |
|---|---|---|---|---|
| hi conga, open (63) | 262 | 262 (−1 c) | 254 → tuned 262, its (1,1) 431 strongest, pipe 100 at −27 | −23 dB |
| hi conga, closed (62) | 262 | | | −39 dB (16 dB shorter at 0.1 s) |
| low conga (64) | 196 | 196 (−1 c) | 200 → 196, 324 −3, pipe 97 at −14 | −32 dB |
| bongo macho (60) | 523 | 523 (−1 c) | 523 −8, 858 0 | −54 dB (short, bright) |
| bongo hembra (61) | 392 | 392 (−1 c) | 392 −16, 647 0 | −33 dB |
| high timbal (65) | 330 | 330 (0 c) | 330 0, 658 −11, 883 −25 | −38 dB |
| low timbal (66) | 247 | 247 (−1 c) | 247 0, 668 −24 | −29 dB |

  - Velocity 16 to 127 (fast max): conga 35.4, bongo 37.5, timbale 32.9 dB (GM: ~30).
  - Fuzz 10 s each: stable. Cost: at most 306 ns/sample, 1.5 % of a core.
  - Level trims to the common −28.2 LUFS: conga +3.0, bongo +7.9, timbale −4.9 dB. After them,
    −28.2 each.
- **Not done:**
  - the bass tone (the palm in the middle) and the slap: GM has no key for them; a key or
    velocity zone could choose them;
  - the cáscara (stick on the timbale's shell) and the rimshot, likewise;
  - the timbale shell's modes, which are guesses to be tuned by ear;
  - the room's pressure on the heads (as the kick has);
  - the bongos' decay, very short (−54 dB at 0.1 s on the macho), to be judged by ear.

## 7. Sources

- **[web]** Wikipedia, "Conga": https://en.wikipedia.org/wiki/Conga. Tuning practice, the open tone.
- **[web]** My Conga Place forum, tuning threads: https://mycongaplace.com/forum/eng/viewtopic.php?p=4231
  and https://www.mycongaplace.com/forum/eng/viewtopic.php?p=87713. Fourths; E–G–C (Mongo, Poncho);
  example pitches.
- **[web]** Thomann, congas, first steps (open and closed slap, the other hand on the skin):
  https://www.thomann.fr/onlineexpert_page_congas_premiere_etape.html
- **[web]** MasterClass, guide to timbales (cáscara, rimshot, shell materials):
  https://www.masterclass.com/articles/guide-to-timbales
- **[web]** LP stainless steel timbales (steel shells: sustain, brightness):
  https://www.target.com/p/lp-prestige-series-stainless-steel-timbales/-/A-76117943
- **[paper]** T. D. Rossing, "Acoustics of percussion instruments: recent progress", Acoust. Sci. & Tech.
  22 (2001): https://typeset.io/pdf/acoustics-of-percussion-instruments-recent-progress-e5bc5xd7kk.pdf
- **[paper]** D. Politzer, "Banjo drum physics": https://arxiv.org/pdf/1806.08857 (the air in the shell
  coupling to the head's up-and-down mode).
- **[paper]** BYU, udu modal patterns by scanning laser vibrometry: https://physics.byu.edu/docs/publication/1551
- **[jd]** research/jazz_drums.md; **[toms]** research/toms.md (two-head drums, the stick, the glide).

## The edge slap and the palm's bass tone (as built)

Two strokes besides the open and the closed tone (src/handdrum.cc, HandDrumPlayer::noteOn):

- **The slap.** Loose fingers whipped onto the head just inside the rim, at 0.06 of the radius further in than an open tone. Light (0.45 × the hand's mass), hard (2.2 × K: bone under little flesh), narrow (0.55 × the fingers' width), snapped straight off (lift 2 ×). Its crack is mostly the contact's own: the air squeezed out between the cupped fingers and the skin (the click path, × `PJ_HD_SLAPCLICK`, 8). Pressing the fingers on (the model's mute) kept them on the head and damped exactly the highs that make the crack, so the slap is left open.
- **The bass tone.** The flat palm near the middle (0.08–0.2 of the radius): heavy (2.6 ×), soft (0.45 × K), wide (2.2 ×), lifted at once.

Measured (`-inst`, dry, velocity 100, the first 30 ms attack included; `PJ_HD_STROKE` 0/1/2 forces the stroke):

| | open tone | slap | palm |
|---|---|---|---|
| conga high (63) | 291 Hz, 0 % over 1.5 kHz | 792 Hz, 11 % | 283 Hz, 0 % |
| conga low (64) | 229 Hz, 1 % | 780 Hz, 14 % | 209 Hz, 0 % |
| bongo high (60) | 538 Hz, 1 % | 1003 Hz, 8 % | 674 Hz, 2 % |
| bongo low (61) | 416 Hz, 1 % | 1022 Hz, 11 % | 462 Hz, 1 % |

The palm gives a conga its bass tone (darker, its energy near the fundamental), but on a bongo's small head it gives none, so the bongo player does not use it. In the model the head's own high modes add little at a distance: a membrane's waves are far slower than sound, so its high modes mostly cancel at the microphone. Raising the heads' mode range to 5–6 kHz, or their loss knee, changed the brightness by under 1 % (it would cost 3 × the conga's CPU).

Which stroke, in `-play` (General MIDI has no keys for them): an accent (12 over the drum's recent velocity, or 105 and up) on beat two or four, mostly on the high drum, becomes a slap (60 % of them on a conga, 35 % on a bongo, × 0.4 on the low drum): the tumbao's slap. A soft stroke (10 under) on beat one or three on a conga is a bass tone 40 % of the time.
