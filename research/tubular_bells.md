# Tubular bells (orchestral chimes): research and model plan

Status: 2026-11-27. Built as `tubularBellsDesign()` in `src/bars.cc` (the player in
`src/barsinst.cc`): church bells C2–B3, tubes C4–G5, carillon bells G♯5–C8 (section 12), the
tubes' modes from a Timoshenko finite-element model with the plug, doublets, the longitudinal
mode, the two-faced hammer chosen by velocity, the ring times of 3.5, and jyrki-spectral tuning
per stroke (section 9). Not yet: hand damping, the swing, the
director's rules (section 8). See section 10a for what was measured on the result. GM program 15 "Tubular Bells". This note
replaces that section's guesses with measurements and sources, and puts the weight where the
user asked: how the bells are **played** (which part of the tube, at what angle, with which face
of the hammer), how they **blend** with the orchestra, and how they **tune** (jyrki-spectral).

Where each number comes from: **[web]** a source in section 13; **[meas]** measured here from the
recorded tubular-bell samples of four GM fonts (FluidR3, TimGM6mb, Jnsgm2, and the pajupilli
model itself), scripts in the session scratchpad; **[calc]** computed here (a Timoshenko beam
finite-element model of the tube with its plug; Hertz contact); **[memory]** remembered, not
looked up; **[est]** a working guess, to be fitted. Pitch names are sounding (C4 = 261.6 Hz).

## 1. Short answer

- **No mode sounds at the note heard.** The strike note is a virtual pitch an octave under the
  4th bending mode, made by the 4th, 5th and 6th (and 7th) partials lying near 2 : 3 : 4 (: 5)
  [web, Hibbert; Fletcher & Rossing via HyperPhysics]. The 1st and 2nd modes are nearly silent;
  the 3rd (a flat major third above the strike note, 1.21–1.25×) is the "bell" colour.
- **Real orchestral tubes are not the odd squares.** Measured on three recorded sets: P5 = 2.93–
  2.97×, P6 = 3.99–4.11×, P7 = 5.16–5.40×, P8 = 6.4–6.8× the strike note [meas]; the model
  has 2.99, 4.17, 5.56, 7.14 (from 60 to 190 c sharp at P6–P8). The cause is the tube's shear
  and rotary inertia (a Timoshenko beam), which grows with diameter / length: the short high
  tubes are stretched least, the church tubes (Hibbert: 2.86, 3.81) most [calc]. The end plug
  matters little on a heavy 1½" tube but a good deal on a light ¾" one (P5–P9 +8…+30 c,
  section 3.2) [calc].
- **The first model's upper keys sounded an octave (or a sixth) low** (before 2026-11-27). It
  let the 0.62× and 1.21× modes come out as loud as the strike-note octave (F5: P2 −2 dB, P3
  0 dB); the recordings have them 12–38 dB down [meas]. The pitch tracker heard its F5 as A4
  (−780 c). The model as built now has them where the recordings do (section 10a).
- **The hammer has two faces: rawhide and wood (or acrylic).** Rawhide: full, round, more
  "fundamental" (the 2 : 3 : 4 group). Wood or plastic: articulate, piercing, more upper
  partials [web]. Players use the hard face for loud articulate passages and the soft one for
  quiet legato [web, Becker]. Contact: rawhide ≈ 0.7–1.5 ms, hard wood ≈ 0.3–0.4 ms [calc].
- **Where and at what angle:** always the top cap, never the tube lower down. The blow goes
  horizontally into the side of the cap with the handle parallel to the tube (most sources), or
  into the rounded corner at about 45° (some orchestral players). Never downward onto the top:
  "thin", "inferior", "far out of tune" [web]. The physics agrees: a downward component drives
  the tube's longitudinal mode, which lands right on P6 at C4 and on P5 at C5 [calc]. A blow off
  the tube's plane drives both members of each doublet, and they beat ("movement") [web, calc].
- **In the orchestra:** few notes, slow, on the tonic and fifth or deliberately outside the
  harmony. At most two at once. The pedal is held through a harmony and cleared at changes; the
  last bell is left to ring past the cutoff. They cut through at p–mf and in thin textures and
  are swamped at fff tutti unless doubled (horns and trombones on the strike, low piano octaves
  or tam-tam for a hum, harp or glockenspiel an octave up) [web].
- **They ring longer than a piano:** a ¾" tube's strike note falls ~30 dB in 17 s (T60 ≈ 34 s),
  its colour softening as the upper partials die first (the user's listening). The first model's
  bell died in 11 s, sooner than its own grand (22 s at C4); the model as built rings 34–41 s
  [meas].
- **Tuning:** a set is fixed (A = 442 is usual in orchestras [web, VSL]), but pajupilli can do
  what no rack can. Each strike is tuned by jyrki-spectral (`bandPull`) with the bell's **own**
  partials as the listener, and the band hears the bell by its own template (`kBandChime`),
  not the harmonic `kBandBell` one (section 9).
- **Below and above the tubes** (the user's plan, built): church bells below C4 and carillon
  bells above G5, the changes sudden. The tubes are a portable carillon, the low tubes a cheap
  stand-in for church bells (section 12).

## 2. The instrument

- **Tubes:** brass, chrome-plated or plain, 1¼" (31.8 mm) or 1½" (38.1 mm) across; the big
  "symphonic" sets use 1½" [web, Yamaha, Adams, VSL]. No maker publishes a wall thickness
  (VSL's "1–2 cm" is a units error); 1.2–2 mm is typical of drawn brass tube [memory]. It hardly
  matters to the modes (section 3.2).
- **Lengths:** about 155 cm (C4) to 75 cm (F5) [web, VSL]; the model computes 1.61 m (C4) to
  0.95 m (F5) for 38 mm brass with a 40 g plug [calc]. The VSL F5 must be a thinner tube.
- **Range:** C4–F5 standard (18 tubes); some to G5; F3–F5 (25) and E♭3–G5 (29, "philharmonic")
  exist [web]. Written at the strike note; listeners disagree about the octave (section 3.4).
- **Top end:** a brass striking cap, plugged and reinforced. One museum set: a 0.3" (7.6 mm)
  thick metal cap with a ½" opening [web, Grinnell]. The bottom end is open.
- **Hanging:** each tube on a cord (plastic-covered string) through a hole near the top, from
  a rail; the accidentals' row about 20 cm higher, behind [web]. The guide boards are
  felt-lined, so the tubes do not swing into each other [web, PDISC].
- **Damper:** a felt-lined bar along the tubes. Releasing the pedal pushes each tube's side into
  the felt of the upper and lower boards. Pedal down: all ring. A latch ("damper stopper") holds
  it open. Some European sets work the other way round [web, Yamaha, PDISC, ARMY].
- **The frame:** 180–220 cm tall, 84–100 kg for 1½ octaves [web, VSL].

## 3. How a tube vibrates

### 3.1 Partials, measured

Ratios to the strike note (P4 / 2), from the recorded samples [meas]; levels in dB re the
loudest partial over the first second (the samples are 1.3–1.6 s, looped, so decays are not
trustworthy):

| partial | Jnsgm2 "C" | Jnsgm2 "G" | FluidR3 | TimGM6mb | Hibbert, 217 church tubes [web] | model now (odd squares) |
|---|---|---|---|---|---|---|
| P1 | — | 0.222 (−44) | 0.223 (−40) | 0.221 (−30) | (very quiet) | 0.222 |
| P2 | 0.615 (−38) | 0.617 (−23) | 0.623 (−20) | 0.624 (−12) | 0.65 | 0.617 |
| P3 | 1.211 (−28) | 1.216 (−16) | 1.219 (−16) | 1.224 (−9) | 1.25 | 1.210 |
| P4 | **2** (−14) | **2** (−2) | **2** (−4) | **2** (−5) | 1.99 | 2 |
| P5 | 2.970 (−12) | 2.953 (0) | 2.952 (0) | 2.932 (0) | 2.86 | 2.988 |
| P6 | 4.107 (0) | 4.058 (−2) | 4.056 (−1) | 3.990 (−6) | 3.81 | 4.173 |
| P7 | 5.397 (−5) | 5.295 (−9) | 5.293 (−13) | 5.159 (−22) | 4.86 | 5.556 |
| P8 | 6.824 (−13) | 6.64 (−21) | 6.44 (−40) | 6.38 (−31) | — | 7.136 |
| P9 | 8.373 (−19) | — | — | — | — | 8.914 |

- The strike-note group (P4–P6) carries the sound. P5 is usually the loudest partial; it lies
  20–60 c flat of 3×. P6 is −50…+30 c from 4×. That is the "slightly out-of-tune" quality the
  orchestration books mention [web, Adler].
- A **doublet** shows in several partials (Fluid: 3017 / 3093 Hz, 4204 / 4249 Hz, 5432 /
  5545 Hz; the weaker member decays 2–4 times faster) [meas]. Hibbert found nearly all partials
  doubled, "very wide" in the high ones [web]. The causes: the tube is not quite round, plus the
  suspension hole and the cap. A Musser tuner taps a tube on its two sides: "You will always
  have a straight sound on one of these sides", the other beats [web, Piotrowski].

### 3.2 Why the ratios fall: Timoshenko, and the plug on a light tube [calc]

A free tube by Euler–Bernoulli has 9 : 25 : 49 : 81 : 121 : 169 … (the model's table). With shear
and rotary inertia (a Timoshenko FE model; brass E = 105 GPa, ρ = 8500, shear factor 0.53;
38.1 mm tube; a plug of 0–60 g at the top), ratios to P4 / 2:

| tube (L) | P1 | P2 | P3 | P5 | P6 | P7 | P8 | P9 |
|---|---|---|---|---|---|---|---|---|
| Euler–Bernoulli, any | 0.224 | 0.617 | 1.210 | 2.988 | 4.173 | 5.556 | 7.136 | 8.914 |
| C4 (1.63 m) | 0.231 | 0.633 | 1.227 | 2.937 | 4.019 | 5.229 | 6.547 | 7.955 |
| F4 (1.41 m) | 0.234 | 0.638 | 1.233 | 2.920 | 3.971 | 5.131 | 6.377 | 7.688 |
| A♯4 (1.21 m) | 0.237 | 0.645 | 1.240 | 2.899 | 3.910 | 5.008 | 6.167 | 7.364 |
| F5 (0.97 m) | 0.244 | 0.659 | 1.256 | 2.858 | 3.796 | 4.784 | 5.795 | 6.807 |
| ¾" tube, C4 (1.13 m) | 0.224 | 0.619 | 1.213 | 2.972 | 4.120 | 5.434 | 6.901 | 8.508 |
| ¾" tube, C5 (0.79 m) | 0.225 | 0.623 | 1.219 | 2.951 | 4.055 | 5.293 | 6.644 | 8.088 |
| ¾" tube, F5 (0.68 m) | 0.227 | 0.627 | 1.224 | 2.937 | 4.011 | 5.200 | 6.479 | 7.826 |

- The stretch depends on diameter / length, so it grows **up** the compass. Fitting the
  diameter to the recorded ratios gives effective D/L = 0.023–0.033 (rms 6–15 c) [calc]. The
  38 mm FE values above overshoot the recordings at P6–P8, so the samples look like thinner
  tubes (below).
- The ¾" rows (1 mm wall, 20 g plug) match the FluidR3 and Jnsgm2 samples to a few cents,
  better than the 1½" rows. The tube diameter is a design choice of the model: ¾" (light,
  nearly odd squares at the bottom), 1¼" or 1½" (heavier, more stretched).
- **The plug:** on a 1½" tube 30–60 g moves P5–P9 by +1…+8 c relative to P4 and lowers the
  whole tube by ~1.5 % (the length is cut to suit) [calc]. On a light ¾" tube (0.4 kg) a 20 g
  plug is 5 % of the mass: it moves P5–P9 by +8…+30 c and P1 by −45 c, and that is what brings
  the model onto the recordings (the closed Timoshenko formula without it was 15–30 c low at
  P7–P8). So the model computes the tube with its plug by finite elements (40 Timoshenko
  elements, the plug a mass with its rotary inertia: within ~5 c of a fine mesh to P9). The
  effect is not linear in the plug's mass; no simple correction fits it. It does **not** pull
  4 : 5 : 6 towards 2 : 3 : 4. Wall thickness, 1.2–2 mm: < 3 c [calc]. Its real job is the
  striking surface, and (per the makers' folklore) more sustain in the first partials [web,
  Hite].
- Hibbert's empirical fit for church tubes, f ∝ (2n + 0.287)^1.651 [web], is the same effect in
  thick, long-pitched tubes.

### 3.3 The other motions, which the angle of the blow chooses [calc]

| key | strike | tube L | longitudinal c/2L | × strike | torsional | × strike |
|---|---|---|---|---|---|---|
| C4 | 261.6 | 1.61 m | 1090 Hz | 4.17 | 666 Hz | 2.54 |
| E4 | 329.6 | 1.43 m | 1231 Hz | 3.73 | 752 Hz | 2.28 |
| G4 | 392.0 | 1.30 m | 1349 Hz | 3.44 | 824 Hz | 2.10 |
| C5 | 523.3 | 1.12 m | 1576 Hz | 3.01 | 963 Hz | 1.84 |
| F5 | 698.5 | 0.95 m | 1848 Hz | 2.65 | 1129 Hz | 1.62 |

- **Longitudinal** (the tube stretching; its harmonics 2×, 3× that): only a force **along** the
  tube drives it, so a downward blow on the cap's top. It falls in the middle of the strike-note
  group: on P6 at C4 and on P5 at C5. Beating against them, it is the "far out of tune" and
  "predominant unwanted overtones" of a downward or angled stroke [web, ARMY]. It radiates only
  from the end faces and the plug, weakly, but it sits where the ear listens hardest.
- **Torsional:** driven by a blow that misses the tube's axis (a glancing hit on the cap's
  side). It hardly radiates from a round tube [memory]. Through the cap's unevenness it couples
  into bending, and it adds damping noise rather than a tone.
- **The doublet:** a horizontal blow straight at the axis, along one of the tube's two principal
  directions, drives one member; any other direction drives both, which beat at the split
  (a few Hz in P5–P7) [calc, web].
- **The cap ring (the rim's own ovalling, kHz):** a sharp click at the contact, stronger the
  harder the face and the more the blow lands on the rim's edge [est].

### 3.4 The note heard

- The strike note is P4 / 2, a little flat of it because P5 and P6 are flat [web, Hibbert].
- **Octave ambiguity:** the score is written at the strike note. VSL says the fundamental
  heard is an octave under it; Cook says some hear it an octave higher; Yamaha says listeners
  place it an octave lower; makers disagree on whether the lowest tube is C4 or C5 [web]. Lee
  Hite: from about C5 up, "the strike note will be the fundamental", below it more bell-like and
  more ambiguous [web]. With our ratios the group P4 : P5 : P6 = 2 : 2.95 : 4.05 is clear
  enough. The ambiguity grows when P2/P3 are loud (as in the model now), when P4 is weak against
  P5 and P6, and in the low tubes, where P4 (520 Hz at C4) is near the bottom of the
  virtual-pitch region (Hibbert: 500–1500 Hz).

### 3.5 Loss

- No written source gives decay times. The orchestral tube rings "for a long time" [web]; a tube
  on its own stand rings longer than in the rack, where the cords and the damper felt touch it
  [web, France].
- **The user's listening (2026-11-27, from a video of a real set):** the tubes ring longer than
  a piano. The video shows one note ringing for **17 s, then damped by hand**: at that point
  the strike note had fallen perhaps **30 dB**, and the colour had **softened a lot** (the
  upper partials long gone, the 2 : 3 : 4 group left) [user]. 30 dB in 17 s is a T60 of about
  **34 s** for the strike-note group [calc]. A tube is heavy, about half a kilogram by eye.
  **That set had ¾"
  (19 mm) tubes** (Gary France plays such a set [web]). Computed for ¾" brass with a 1–1.25 mm
  wall: C4 1.13 m, 0.58–0.72 kg; C5 0.79 m, 0.40–0.50 kg; F5 0.68 m, 0.34–0.43 kg. That agrees
  with "half a kilogram". The ratios for ¾" C5 are 2.951, 4.055, 5.293, 6.644. That is
  FluidR3's sample (2.952, 4.056, 5.293, 6.44) and Jnsgm2's G (2.953, 4.058, 5.295, 6.64) to a
  few cents, so the GM fonts' bells may well be thin tubes too [calc, meas]. The 1¼–1½"
  orchestral tubes are 2–5 times heavier; with the same brass they should ring at least as
  long [est]. Computed masses: a 1¼"
  tube with a 0.8–1 mm wall weighs 0.7–1.2 kg; a 1½" tube with a 1.5 mm wall weighs 1.5–2.5 kg
  [calc]. Either way it is 5–15 times the hammer, so a blow puts little energy in and the tube
  loses it slowly.
- **The model now, against that** [meas, `-secs 20`, dry]: the bell's T60 is 10.9 s at C4 and
  10.8 s at C5, while pajupilli's own grand gives 22.3 s (C4) and 13.2 s (C5). The bell dies
  **sooner** than the piano: it should be the other way round.
- **Target:** T60 ≈ 35 s for the strike note's partials (P4, about 0.5–1.4 kHz) in the middle
  of the compass, perhaps 40 s at C4 and 25 s at F5, for the ¾" tubes and at least that for the
  heavier ones [user, est]. That is a loss of 0.2 /s, a brass Q of about 1.5·10⁴ at 1 kHz,
  plausible for drawn brass [est]. The model's 0.35 /s + 0.5 /s per kHz gives T60 = 8 s at
  1 kHz and 5 s at 2 kHz: four times too lossy where the ear listens.
- **The colour softening** says the loss climbs steeply with frequency: P5 and P6 (1.5–3 kHz)
  perhaps 15–25 s, P7 and above 3–8 s, so that at 17 s the upper group is 50–80 dB down while
  P4 is 30 dB down [user, est].
- The samples' first second (unreliable: looped) shows P4 and P5 falling 0–17 dB/s and P7–P8
  falling 35–65 dB/s [meas]. That means **a much stronger rise of loss with frequency** than the
  model's, or radiation loss (which rises with ka): the upper partials are a bright strike that
  dies within a second, the 2 : 3 : 4 group sings on, and the hum (P2, P3) is quiet but
  survives [meas, web, SOS Synth Secrets: "strike, strike note, hum"]. So: P4 rings ~35 s,
  P5–P6 15–25 s, P7 and above 3–8 s, and P2/P3 at least as long as P4 [est].
- **Needed:** 25–40 s single-strike recordings, to fit each partial's own decay (section 12).

## 4. The hammer: two faces, chosen by the dynamic

### 4.1 What the hammers are [web]

- Rolled (coiled) rawhide on a wooden handle: the standard. "A full-sounding tone ... Wooden and
  acrylic hammers do not sustain as full-bodied a tone" (Cook); "allows more of the fundamental
  to be heard when compared to a harder acrylic mallet" (Deagan).
- Two-faced hammers are the norm: "a hard surface for playing those loud articulate passages and
  a softer surface for playing those legato quieter passages" (Becker); "the hard wooden side
  gives a more piercing, brighter sound" (Thomann); felt, moleskin or chamois on one face "for
  softer attacks and more mellow tone" (Army, Cook, Vic Firth).
- Masses: 130 g (Grover PM-3, 38 mm head), 180–185 g (PM-4, 44 mm: "a true fundamental
  sonority"). Heads 38–50 mm across, about as wide as the tube (Becker); handles ~30 cm [web].
  Kolberg grades plastic heads by hardness 60 / 80 / 95.
- The repertoire's extremes: Holst marks Saturn's fff "with metal striker" and the pp return
  "with felt striker" (and the felt pp is the one that reads in recordings); Oldfield's claw
  hammer cracked the tubes [web].

### 4.2 Contact [calc]

Hertz contact, F = K δ^1.5, a 150 g head on a rigid cap; the time t ≈ 3.22 (m/K)^0.4 v^−0.2,
above about 1/t the force spectrum falls:

| face | K (N/m^1.5) [est] | 0.5 m/s | 1.5 m/s | 3 m/s |
|---|---|---|---|---|
| rawhide, the model's now | 4·10⁷ | 1.6 ms / 640 Hz | 1.3 ms / 790 Hz | 1.1 ms / 910 Hz |
| rawhide, harder coil | 1.5·10⁸ | 0.9 ms / 1.1 kHz | 0.75 ms / 1.3 kHz | 0.65 ms / 1.5 kHz |
| acrylic / POM | 6·10⁸ | 0.53 ms / 1.9 kHz | 0.43 ms / 2.3 kHz | 0.37 ms / 2.7 kHz |
| hard maple | 1.2·10⁹ | 0.40 ms / 2.5 kHz | 0.32 ms / 3.1 kHz | 0.28 ms / 3.5 kHz |

- The model's rawhide (K 4·10⁷) is too soft: at C5 it cuts P5 (1.55 kHz) and leaves P2/P3
  untouched. The recordings' P5 loudest and P7 −5…−22 dB suggest a rawhide corner near
  1.3–2 kHz (K ≈ 1.5–4·10⁸) [est, to fit].
- The cap is not rigid: the rim's local give (its ovalling) is in series with the face. For
  the wood face the cap's give probably sets the contact more than the wood does [est].
- Rawhide is hysteretic (a coiled hide, layered), so it parts more slowly than it presses: the
  model's `hyst` 0.3 is in the right spirit. Wood hardly is [est].

### 4.3 Which face, when (the user's rule, 2026-11-26: velocity can choose the face)

- **Rawhide** for p–mf and legato tolling, and wherever the bell should sing and fuse;
  **wood** for f–ff, accents, marcato, and fast articulate figures that must speak through a
  texture. This follows the pedagogy above.
- **A player cannot turn the hammer between every note.** The face changes only in a gap. A
  workable rule: the face follows the velocity with hysteresis. Change to wood above about
  velocity 96, back to rawhide below about 72, and only when the gap since the last stroke
  exceeds ~0.4 s (the time to flip a hammer). With the director: the face is chosen per phrase
  from the phrase's peak dynamic and its articulation, as a vibist chooses mallets [est].
- With two hammers (fast passages, a roll), both show the same face (Vic Firth: "otherwise you
  won't have a consistent sound from hand to hand").
- The faces differ in more than brightness. Wood gives the cap's click and the upper partials;
  rawhide gives a rounder onset and leaves the hum and the 2 : 3 : 4 group. Within a face,
  velocity still brightens (Hertz), which the GM fonts lack and the model rightly has
  (gm_compare: +2.2 ERB from vel 40 to 127).

## 5. Where and how to strike: the aesthetic of the stroke

| what the player does | sources | what happens physically [calc/est] | in the model |
|---|---|---|---|
| horizontal, into the side of the cap, handle parallel to the tube | Army, Up Front, Becker, B. Jones, Instrumentalist, Gary France | a pure transverse force at the free end, where every bending mode has its antinode: the full 2 : 3 : 4 group, no longitudinal mode | the default stroke |
| into the rounded corner, ~45°, "come up as fast as I possibly can" | PDISC, McCarthy (orchestral players) | mostly transverse plus a little longitudinal: a slightly harder, more present attack; the rim edge gives more click | a "45°" variant: 15–30 % of the force axial [est] |
| "through and up" (a slightly rising stroke) | Meyer (PAS) | transverse with a little axial component away from the tube; a clean parting | as horizontal |
| downward onto the top | all: "thin", "inferior", "far out of tune" | axial force: the longitudinal mode on P5/P6, less bending | a fault, not modelled except as an option for effects |
| off the axis (a glancing hit) | Army (off-angle, "the sixth above") | both members of each doublet driven, beating; torsion | the doublets' mix by the stroke's direction, varied stroke to stroke |
| on the tube lower down | forbidden (dents); effect only (hard xylophone mallet) | lands on nodes of some modes: an uneven, "unclear" set | an effect articulation |
| scraped across the caps (glissando) | ISFEE, Up Front, Adler | many tubes struck lightly and rapidly | a glissando articulation |
| bowed at the bottom | Wikipedia | high bending partials, sustained | not planned |

- **The stroke:** "a relaxed, weighted full stroke from the arm and wrist, with a fluid rebound
  to avoid a pounded clangy tone" (Cook); "think of pulling the tone out of the tube" (Grover).
  The head should leave at once. A pressed (dead) stroke, where the hammer stays on, damps the
  tube at its antinode. That is a real articulation, but a fault in normal playing.
- **The swing:** after a strong blow the tube swings on its cord; the guide boards keep it off
  its neighbours. No source treats the audible effect (a slow amplitude and Doppler wobble of
  well under a hertz) [memory]. It is a candidate for the life that sampled chimes lack.

## 6. Damping and phrasing

- **The pedal** is a piano's sustain pedal, all tubes at once [web, VSL]. Held through a
  harmony, cleared at a change (Grover: "dampening notes of limited duration") [web]. With the
  pedal down and several notes, "the result becomes quite blurry" (ISFEE).
- **The hand:** pedal down, the free hand grabs the top of the tube just played as the next is
  struck. With one hammer this is the normal way to play a melody legato without the smear
  (Becker, Army, B. Jones). Sliding the hand along the tube gives a decrescendo (middle-school
  curriculum) [web].
- **Damped strokes:** struck with the dampers on, a short note [web, Versilian, VSL "single
  strokes (damped)"]. The release itself is a sound: Versilian recorded the dampers' cut-offs
  as velocity-dependent release samples, "a significant boost to realism" [web].
- **Let ring:** the endings. Shostakovich 11, Pärt's Cantus (struck when everything stops, so
  only the bell is left) and MacMillan (about a minute) all leave the last bell ringing [web].

## 7. Playing style

- **Few notes, slow.** "Not common for doubling quick-moving lines" (ISFEE). "Faster passages
  convey the impression of many church bells ringing at once, especially if the sustaining pedal
  is depressed" (Adler) [web]. One hammer for simple tunes; two for fast passages, played from
  memory (PDISC: "memorization is a very large part of playing tubular bells").
- **Tolling patterns:** one note repeated in fixed groups (Pärt: threes); two notes alternating
  (Berlioz C/G); a short ostinato (Parsifal C–G–A–E); festive same-pitch ringing (1812); far
  and near bells answering after a pause (Tosca); a syncopated tocsin against the tutti
  (Saturn) [web].
- **Two notes at most at once** (Adler; four-note chords need two players) [web].
- **Rolls:** two hammers alternating on one cap; "great crescendo possible", "extremely soft pp
  tremolos" (VSL); vibraphone mallets for soft rolls. Chimes "do not generally require rolls"
  (Cook) because they sustain [web].
- **Timing:** nothing says to strike early. The strike is instant, so a bell is played on the
  beat [web, memory]. Offstage bells come late in a big hall: the conductor shouts "Anticipate!"
  [web]. A synthetic offstage bell with a distance delay is moved earlier by it.

## 8. In the orchestra: blending

- **Pitch choice:** tonic, fifth, a pedal; or deliberately away from the harmony (Meyerbeer's
  bell F a tritone over the bassoons' B, "that sinister quality"; Forsyth: in the theatre, "a
  heavy bell whose tap-note is absolutely removed from the orchestral harmony") [web]. Tubes
  "blend better with other instruments than church bells" (VSL), being nearer harmonic (Hibbert)
  [web].
- **What they sit with:** glockenspiel, vibraphone, gong and tam-tam, harp, piano, timpani;
  brass and woodwind full chords ("support the strike note with sforzando attacks and the
  resonance with sustained notes", VSL) [web].
- **Balance:** the books say they prevail (Rimsky-Korsakov); practice says they are buried at
  fff tutti. Saturn's metal-striker fff is "barely audible", the felt pp "stands out clearly"
  [web]. Hence: a pp bell needs nothing; a fff bell is doubled, thinned around, or given the
  wood face.
- **Building a bigger bell than the tube** (Berlioz, Parsifal, Tosca's low E, Mahler's deep
  bells: these wanted real or very deep bells): Forsyth's and Bayreuth's recipes are a low
  brass attack on the strike note, a tam-tam (or a low piano octave) for the hum, harp or
  glockenspiel an octave up. Berlioz himself used piano low octaves. San Francisco Opera
  builds Tosca's campanone from layered tubes and samples [web]. Mahler 2 and 6 ask for
  *unpitched* deep bells; tubes are the wrong instrument there.
- **Where it stands:** at the back with the percussion; offstage in Berlioz, Mahler 6 and Tosca
  [web]. In pajupilli's stage (OrchestraConfiguration.md) the bells belong far back, with more
  room than direct sound.

**For the director** (src/director.*, Director.md), drawn from the above:

1. Give the bells tonic, fifth or pedal notes. Where the part is a melody, keep it at slow note
   rates (the strike-to-strike gap ≥ ~0.3 s) and at most two notes at once.
2. Pedal (sustain) through a harmony, cleared at each harmony change; with a single hammer in a
   legato line, hand-damp the previous note as the next is struck.
3. Choose the face by the phrase's dynamic and articulation (section 4.3).
4. Let the final bell ring past the orchestra's cutoff unless the score damps it.
5. When a score asks for *deep* bells below the tubes' range, they are church bells (section
   12). Only a stage with tubes alone needs the composite: the tube an octave up, a soft
   low-brass or piano attack on the written note, a tam-tam hum.

## 9. Tuning: jyrki-spectral

The user's direction (2026-11-26): the bells should tune with jyrki-spectral usually.

- **What exists:** `bandPull` (src/band.h) places a listener's note where its partials meet
  those of everything sounding, each pair within 36 c pulling by exp(−(Δ/20 c)²) / (higher
  harmonic number)². The koto, the fretless, the winds and the jazz singer use it; the organ's
  `jyrki-spectral` does the same by its pipes.
- **What a rack cannot do:** a real set is fixed, ET at 442 [web]. pajupilli can retune each
  strike (as the koto's ji are set by plan and pressed notes by ear). The tube's pitch is fixed
  once it rings, so the tuning is chosen **at the strike** and never glides.
- **The listener's partials must be the bell's own:** P2…P9 at the ratios of section 3.1 (per
  key, from the Timoshenko table), with their levels at that face and velocity, and "harmonic
  numbers" k = ratio (2, 2.95, 4.05, …) for the pull's weighting. Consequences:
  - P4 (the strike note's octave) carries most of the weight: it is near 2× and strong. The
    bell then lines up its octave with the band's partials, as a fixed instrument tuned to
    the chord's root would.
  - P5 is 20–60 c flat of 3× and P6 −50…+30 c from 4×: they pull in opposite directions
    against a harmonic chord, and mostly cancel. Within the 36 c window P5 pulls the bell
    sharp against a chord whose fifth is in tune. The ±15 c limit (as the singer's
    `PJ_SINGER_TUNE_LIM`) keeps that harmless.
  - P3 (the flat major third, 1.21–1.25×) meets a just major third (1.25) only loosely. It is
    the bell's colour, not something to tune by: weight it lightly or leave it out.
- **The band must hear the bell as a bell:** `bandPartial(kBandBell, …)` is harmonic (1, 0.5/k).
  For the tubes it should be the measured set (P4 = 2, P5 2.95, P6 4.05, P7 5.3, P3 1.22 weak),
  so that a wind or the singer tunes to where the bell's partials really are. A separate family
  (`kBandChime`) is cleaner than bending the bell template, which the harp, the celesta and the
  glockenspiel share [est].
- **Order of hearing:** the bells are struck, not held. As a fixed-pitch-like part (ear = 0 to
  the others, the koto's "rings on" flag) they are heard by everyone after them, and they
  themselves hear what came before (a held chord) when struck. Two bells of a chord struck
  together: the lower is heard by the higher (band.h's rule).
- **Knobs** (by the others' pattern): `PJ_BELLS_TUNING` (`jyrki-spectral` default, `tet12`),
  `PJ_BELLS_TUNE_LIM` (15 c).

## 10. The first model (before 2026-11-27), against the recordings [meas]

Rendered `-inst tubular-bells -note 60/72/77` (dry), first second:

| | P1 | P2 | P3 | P4 | P5 | P6 | P7 | P8 | P9 |
|---|---|---|---|---|---|---|---|---|---|
| C4, dB | −35 | −20 | −8 | −4 | −3 | −3 | **0** | −6 | −9 |
| C5, dB | −22 | −7 | −1 | **0** | −2 | −8 | −14 | −20 | −25 |
| F5, dB | −16 | **−2** | **0** | 0 | −6 | −14 | −20 | −26 | −31 |
| recordings (C5-ish), dB | −30…−44 | −12…−38 | −9…−28 | −2…−14 | 0 | 0…−6 | −5…−22 | −13…−40 | −19 |

- **P2/P3 too loud** in the middle and top, worst at F5: the radiation term `min(1, k·d)` and the
  flat `hit` (2/√m for every mode) do not reproduce how quiet the low modes are. Real tubes
  radiate their low modes as dipoles whose two sides cancel (k·d ≪ 1). For P1–P3, with k·d of
  0.04–0.4, the efficiency goes as (k·d)², not k·d [calc]. The pitch tracker's verdicts (C4 heard
  −1175 c, F5 −780 c) are this fault.
- **C4's P7 loudest:** the (k·d) radiation rises without the rawhide's roll-off, which is too low
  in frequency to matter there. The overall balance needs the harder rawhide (section 4.2) and
  the (k·d)² radiation together.
- **Ratios:** odd squares, 60–190 c sharp at P6–P8 (section 3.2).
- **Too short:** T60 11 s at C4 and C5, under the grand's 22 s and 13 s. A real set rings
  T60 ≈ 35 s for the strike note (section 3.5).
- **No doublets, no damper, no pedal, no hand damping, no face choice, no stroke angle, no
  band tuning.**
- gm_compare's findings (darker by 4 ERB; 57 ms attack; level floor) follow from the same: a
  soft hammer and a 9-mode ceiling at 8.9× (P10–P14 are missing; they matter for the wood
  face's shimmer).

## 10a. As built (2026-11-27) [meas]

- **Tube ratios** (C5, ¾", 20 g plug): 0.225, 0.624, 1.221, 2, 2.960, 4.073, 5.33, 6.70. The
  recordings have 0.223–0.224, 0.615–0.624, 1.21–1.22, 2, 2.95–2.97, 4.06–4.11, 5.29–5.40. The
  plug matters for a light ¾" tube: it moves P5–P9 by +8…+30 c and P1 by −45 c (the 38 mm tube
  feels it much less, 3.2). So it is in the finite-element model.
- **Levels at the stroke** (C5, rawhide, velocity 96): P2 −21, P3 −18, P4 −4, P5 −4, P6 0,
  P7 −17, P8 −22 dB. The pitch tracker hears C5 at +10 c (it heard the old model's F5 780 c low).
- **Ring:** C4: P4 41 s, P5 38 s, P6 33 s, P8 12 s. C5: P4 34 s (−31 dB at 15 s), P5 23 s, P6
  13 s, P7 7 s. G5: P4 34 s. Church B3: hum 168 s (the 150 s cap and its doublet), prime and
  tierce 50 s, nominal 23 s. Carillon G♯5: hum 39 s, nominal 7 s.
- **Loudness** (BS.1770, the levels tool's way): within ±2.5 dB of the old instrument over C3–C8.
  The church bells are 2–4 dB louder (C2: +4).
- **The face:** velocity 99 (rawhide) −16.1 LU, centroid 1.81 kHz; 100 (wood, eased) −15.3 LU,
  1.92 kHz.
- **Tuning:** under a held C major chord (strings), C5 +5 c, G4 +5 c, E5 −7 c (the flat P5
  pulls it sharp; the third toward just).

## 11. Model plan, and what was built

The `Bars` engine kept (its implicit Hertz mallet, per-mode loss, damper ramp, two microphones),
grown: `BarKey::kModes` 9 → 28, `BarDesign::kMaxKeys` 72 → 80, doublets (`pair`, `theta0`,
`thetaVary`), a second face (`mallet2`, `K2`, `p2`, `hyst2`, `knock[2]`), `Bars::retune`.
Status of each step on 2026-11-27:

1. **Modes per key — built.** 12 bending modes per tube from a finite-element model computed at
   start (`tubeModes` in `src/bars.cc`: 40 Timoshenko elements with the plug, Cholesky and
   Jacobi; ~10 ms a tube), the length found so that P4 is twice the note. The diameter is
   `PJ_BELLS_DIAMETER` (19.05 mm default; 31.8, 38.1), the wall 5 % of it, the plug 20 g ×
   (D / 19.05 mm)². Each mode a **doublet**, split 0.15 % at P4 widening to ~1 % at P8
   (× 0.5–1.5 per tube; `PJ_BELLS_SPLIT`), the weaker direction dying 1.5 × faster. A stroke at
   θ = the tube's own angle ± 10° shares each mode as cos θ / sin θ.
2. **The longitudinal mode — built**, its share of the stroke 0.1 ± 0.05 (`PJ_BELLS_AXIAL`),
   its sound −22 dB under P5's at an equal drive, T60 15 s. The 45° and downward strokes as
   articulations: not yet.
3. **Radiation — built as levels, not as a law.** Each mode's sound is set by its level under a
   hard impulse (P1…P12: −40, −26, −18, −3, 0, −2, −4, −12, −18, −24, −30, −36 dB), fitted to
   the recorded sets; the hammer's contact shapes it by face and speed. Evened over the compass
   by (f / C4)^0.63 so that the short tubes are not quieter than the recorded sets'. A
   (k·d)²-type law in place of the table: not done.
4. **Loss — built:** 0.11 /s + 0.025 /s × (f / kHz)³ (`PJ_BELLS_LOSS`, `PJ_BELLS_LOSSCUBE`);
   measured in 10a.
5. **The hammer — built:** 150 g; rawhide K 2·10⁸, hyst 0.3 (`PJ_BELLS_RAWHIDE`); wood K
   1.2·10⁹, hyst 0.05 (`PJ_BELLS_WOOD`); the cap's click (3.2, 5.1, 7.4 kHz, 10–20 ms;
   `PJ_BELLS_CLICK`), the wood's 5 × the rawhide's. The face rule of 4.3 in `BarsPlayer`, the
   wood's stroke eased to 0.75 of the speed (`PJ_BELLS_HARD_EASE`). The cap's own give in
   series with the wood: not modelled.
6. **Damper and pedal — built (2026-11-28, the user: "tubular bells need to be dampened when the
   key goes [up], unless the sustain pedal is pressed").** A key's note-off brings the felt down
   (`BarsPlayer::noteOff`); the pedal (CC 64) holds every damper off and its release damps every
   key not held (`setHold`). The felt: `PJ_BELLS_DAMPER` 20 1/s at 500 Hz, x sqrt(f / 500)
   (0.7 .. 2.5: the higher modes sooner), about 0.35 s to -60 dB on a tube's strike note; the
   cast bells beyond the tubes 0.6 of it (no damper on a real set: a hand on the heavier bell);
   the felt coming down over `PJ_BELLS_DAMPER_TIME` 0.04 s. **The half pedal** (the user: "the
   player can do a slower release by half pedaling them a bit when they need to die out"): a note
   let go gets the felt only touching, `PJ_BELLS_HALFPEDAL` 0.2 of the full damper (about 1.5 s
   to -60 dB [meas]); the next stroke clears every key let go with the full damper (the player
   moving on, no smear); `PJ_BELLS_HALFPEDAL=1`: the full stop at once. Not yet: hand damping,
   damped strokes, the damper's own release sound.
7. **Swing — not yet.**
8. **Tuning — built** (section 9): `BarsPlayer::pull` at each stroke of a key not ringing, its
   own ringing bells heard too (`hearOwn`), ±15 c; `kBandChime` and `bandRatio` in `src/band.*`
   for the others to hear the bells' inharmonic partials.
9. **Range — built:** church bells C2–B3, tubes C4–G5, carillon bells G♯5–C8 (section 12).

Cost: 73 keys, 18–25 oscillators each, only the ringing ones run; 10 s of dense random playing
renders in 0.6 s.

## 12. Below the tubes: church bells (the user's plan, 2026-11-27)

The idea: tubular bells are a portable stand-in for a carillon, and the bass tubes a cheap,
portable stand-in for church bells. So below the tubes, pajupilli plays **real church bells**
rather than folding the notes up an octave or stretching the tubes. This is also what the
repertoire asked for and seldom got (section 1 of the orchestration findings): Berlioz's C and G
"which he always had difficulty securing", Parsifal's C–G–A–E, Tosca's campanone E, Shostakovich
11's four bells, the 1812's ringing.

### 12.1 Why the tube is a church bell's approximation

Partials against the strike note: a true-harmonic (Simpson-tuned) church bell [web, Lehr 1986,
Hibbert's Glossop 4th] and an orchestral tube (section 3.1):

| church bell | ratio | tube | ratio |
|---|---|---|---|
| hum | 0.50 | P2 | 0.62 (quiet) |
| prime (the strike note itself) | 1.00 | — | none |
| tierce (minor third) | 1.20 (1.18–1.28 cast) | P3 | 1.21–1.25 (quiet) |
| quint | 1.50 (very quiet: a node near the sound bow) | — | none |
| nominal | 2.00 | P4 | 2.00 |
| superquint | 3.00 (2.96–3.15) | P5 | 2.93–2.97 |
| octave nominal | 4.00 (4.05–4.37) | P6 | 3.99–4.11 |
| upper partials | 5.3, 6.6 … | P7, P8 | 5.2–5.4, 6.4–6.8 |

- The tube keeps the bell's **strike-note group** (nominal, superquint, octave nominal: the 2 : 3 : 4
  that makes the virtual pitch in both) and even a tierce-like P3. It lacks the **hum and the
  prime**, which in a church bell are strong and ring longest. That missing low, long ring is
  why tubes sound like "a bell, but small and far", and why Forsyth's and Bayreuth's recipes add
  a tam-tam or low piano for the hum [web].
- So at the seam the two instruments share the strike-note group and the colour of the tierce.
  The church bell adds the prime and the hum underneath. A line going down from the tubes into
  the bells keeps its pitch and its kind of sound, and gains the depth.

### 12.2 The bells

- **Size and pitch** [web, Chime Master; Lehr's carillon table]: weight ∝ f^−3, halving every four
  semitones up. In scientific pitch (bell founders name an octave lower: their "C3, 4,300 lb" has
  a 262 Hz strike):

  | strike | mass | diameter |
  |---|---|---|
  | C5 | 265 kg | 0.77 m (Lehr's carillon C5) |
  | A4 | 450 kg | 0.9 m |
  | C4 | 2.0–2.2 t | 1.6 m |
  | G3 | 5.4 t | 2.1 m |
  | C3 | ~16 t (Big Ben's class) [calc from the rule] | ~3 m |

- **Partials:** Lehr's F4 bell measured (relative to the nominal): hum 0.250, prime 0.500, tierce
  0.600, quint 0.751, nominal 1, then 1.504, 2.087, 2.736, 3.416 [web]. Carillon bells come out
  with the tierce 22–56 c sharp of a just minor third and the quint ±15–57 c [web, Lehr Table III].
  Old (pre-Simpson) bells have the hum up to a whole tone sharp and the prime up to a whole tone
  flat [web, Hibbert]. That is a design choice: `true-harmonic` (default) or `old-style`.
- **Doublets:** a cast bell is less round than a tube. Measured on one 19.25" bell: hum beats at
  1 Hz, tierce at 2 Hz, prime at 4.5 Hz (a 0.53 % split) [research/bells_blocks.md §1.4]. The
  beating of the hum is the slow "wow" of a big bell.
- **Decay:** no source found gives numbers. "The hum, prime and tierce often last for quite a
  while (many seconds)" [web, Hibbert]. A tonne-class bell's hum audibly lasts a minute or more
  [memory]. As a starting point: hum T60 60–120 s, prime and tierce 30–60 s, nominal 10–20 s,
  the upper partials 1–4 s, all longer for the bigger bells [est, to fit to recordings]. The
  church-bell table already in `src/modalinst.cc` (`kChurchT60`, hum 2.5 s) was made for the
  tinkle bell's small bells and is far too short for these.
- **The stroke:** orchestral and theatre bells hang still and are struck: an iron or hard
  wooden hammer on the sound bow (Lehr's strike point H/R0 = 0.16, outside, near the mouth), or a
  clapper pulled against it (chiming) [web, memory]. The hammer is heavy (several kg for a
  tonne bell), so the contact is long and the sound starts with the nominal group and the
  hum. A wood face gives a softer, rounder start; iron gives the clang [est]. The swinging
  clapper of a ringing tower (full-circle ringing) is not an orchestral stroke; it could be an
  option for the 1812's "celebratory ringing".
- **Where:** a church bell belongs offstage or far: Berlioz, Tosca and Mahler place them away
  [web]. On pajupilli's stage the bell section is placed farther and more reverberant than the
  tubes (OrchestraConfiguration.md), and it is moved earlier by its distance delay (section 7).

### 12.3 One instrument, three kinds of bell

- **The compass** of `-inst tubular-bells` (as built, 2026-11-27):
  - **below C4: church bells**, down to C3 (or lower, as a design choice);
  - **C4–G5: tubes** (¾", 1¼" or 1½", section 3.2);
  - **above G5: carillon bells** (small cast bells, true-harmonic, at least 12 kg; Lehr's
    G6–G♯7), since the tubes are a portable carillon. The user chose this over the octave fold.
- **The seams at C4 and G♯5 are sudden** (the user's choice): the hum and prime come in at
  full strength at B3, as from one instrument to another. The nominal is at 2× the strike note
  on both sides, so the pitch carries over. Levels as built: the church bells 2–4 dB louder than
  the tubes (the hum's energy, heard as depth), the carillon bells evened by (f / G♯5)^0.7.
- **As built, the bells** (`castBellKey` in `src/bars.cc`): partials 0.5, 1, 1.205, 1.5, 2,
  3.008, 4.174, 5.472, 6.832 × the strike note, at −10, −6, −6, −22, 0, −5, −8, −12, −16 dB
  under a hard impulse; T60 at C4 90, 50, 45, 15, 22, 7, 4, 2.5, 1.8 s, × (C4 / f)^0.8, at most
  150 s (`PJ_BELLS_BELLRING`); doublets 0.1–0.4 %; mass 2 t × (f / C4)^−3, at least 12 kg; the
  hammer 1.5 kg × (M / 2 t)^⅓ (0.25–6 kg), padded K 4·10⁸ (`PJ_BELLS_PADDED`) or iron 10¹⁰
  (`PJ_BELLS_IRON`); the carillon's soft face bare iron at K 3·10⁹ (`PJ_BELLS_CARILLON`).
  `PJ_BELLS_OLDSTYLE=1`: hum +150 c, prime −120 c. Levels `PJ_BELLS_TUBELEVEL`,
  `PJ_BELLS_BELLLEVEL`.
- **Tuning (section 9) for the bells:** the listener's partials are the bell's own (hum, prime,
  tierce, quint, nominal, superquint …). The prime and the nominal carry the pull. The minor-third
  tierce is the bell's character and always clashes with a major chord ("heavy cast-bells …
  always disturb, and in their turn are disturbed by the harmonic scheme", Forsyth). Weight it
  lightly so it does not drag the bell's tuning. That is one more reason for the director to keep
  the church bells on the tonic and fifth, or deliberately apart from the harmony (section 8).
- **The director:** section 8's rule 5 (a composite deep bell from brass, tam-tam and piano)
  becomes unnecessary for the notes below C4; the church bells are the real thing. The
  composite stays as an option for a "theatre" stage that has only tubes.

## 13. What is still missing

- **For the model:** the damper and pedal, hand damping and damped strokes (6); the tube's swing
  (5); the 45°, downward, glissando and lower-on-the-tube strokes (5); the director's rules for
  bells (8): pitch choice, pedalling, the face per phrase, letting the last bell ring; the
  re-run of tools/gmcompare.py on the new instrument; the user's ear on the demo.

- **Long single-strike recordings** of an orchestral set (both faces, several velocities, pedal
  down, dry or near). The GM samples are 1.3–1.6 s and looped, so they give ratios and onset
  levels but not decays. Candidates: Freesound user sandyrb's orchestral tubular-bell
  recordings (CC BY 4.0, 48 kHz / 24 bit) and the VSCO 2 tubular bell (public domain, Freesound
  "Samulis") need a Freesound login to download; Philharmonia, University of Iowa and Wikimedia
  Commons had none in a quick search.
- **Church-bell decay times** (each partial, by the bell's size) and a recorded tonne-class bell
  struck by a hammer, not swung: section 12.2's T60s are estimates.
- **The Rabenstein–Koch–Popp model** (IEEE TASLP 18(4) 2010, a tube as a plate with periodic
  boundaries) and **Arsie & Oliver** (J. Math. Music 2019, end weighting): abstracts only; full
  texts not reachable here.
- **Fletcher & Rossing / Rossing's *Science of Percussion Instruments*** chime chapters
  (measured plug effects): not read; the plug claims above are from the FE model, which
  disagrees with the folklore that the plug pulls 4 : 5 : 6 towards 2 : 3 : 4.
- **Decay times, swing, half-pedal, roll mechanics, a hammer face's grams-per-dynamic**: no
  source found.

## 14. Sources

Acoustics
- W. Hibbert, "Tubular bells" (233 tubes; partial ratios; strike pitch from P4–P6):
  https://www.hibberts.co.uk/tubular-bells/ ; "Modes of vibration of a tubular bell" (doublets):
  https://www.hibberts.co.uk/modes-of-vibration-of-a-tubular-bell/
- HyperPhysics, "Tubular bells": http://hyperphysics.phy-astr.gsu.edu/hbase/Music/tbell.html
- R. Rabenstein, T. Koch, C. Popp, "Tubular bells: A physical and algorithmic model", IEEE TASLP
  18(4) 881–890, 2010 (abstract): https://cris.fau.de/converis/portal/Publication/106210984
- L. Hite, *Tubular Bell Chimes Design Handbook* (wind chimes: caps, doublets from uneven tube,
  the strike note's octave): https://leehite.org/documents/Tubular%20Bell%20Chimes%20Design%20Handbook.pdf
- Sound On Sound, "Synthesizing bells": https://www.soundonsound.com/techniques/synthesizing-bells

The instrument and playing
- Yamaha, "Anatomy of chimes": https://hub.yamaha.com/drums/percussion/anatomy-of-chimes
- Vienna Symphonic Library academy, "Tubular bells": https://www.vsl.co.at/academy/percussion/tubular-bells
- Wikipedia, "Tubular bells": https://en.wikipedia.org/wiki/Tubular_bells
- G. Cook, *Teaching Percussion* 3rd ed., pp. 101–103
- US Army TC 12-43 *Percussion Techniques*, ch. 4 "Chimes"
- B. Meyer, "Chime Techniques from the PAS Classroom": https://www.youtube.com/watch?v=_yJx5_kETw0
- J. Becker, "Chime Technique": https://www.youtube.com/watch?v=JY0nTum2HrY
- Brett Jones, "Chimes Tutorial": https://www.youtube.com/watch?v=9o7r_f22QJ4
- Gary France, "Learn to play Chimes / Tubular Bells": https://www.youtube.com/watch?v=mjACsPhDJDE
- The Percussion Discussion, "How to Play the Tubular Bells": https://www.youtube.com/watch?v=oKiGvc0UyWE
- Vic Firth, "Keyboard Percussion 10: Chimes": https://www.youtube.com/watch?v=nVLHmFuiBjM
- N. Grover, "Chimes In": https://groverpro.com/neil-grover-chimes-in ; B. Piotrowski on
  tuning (comment): https://groverpro.com/new-tech-talk-from-will-james
- Tapspace, *Up Front* (sample): https://tapspace.com/wp-content/uploads/2026/06/Up-Front-SAMPLE.pdf
- The Instrumentalist, "Preparing percussion for adjudication" (2018):
  https://theinstrumentalist.com/february-2018/preparing-percussion-for-adjudication/
- Percussion Clinic Adelaide: https://www.percussionclinic.com/art_tubbells07.htm
- Thomann, "Tubular bells": https://www.thomann.co.uk/onlineexpert_page_orchestral_percussion_tubular_bells.html
- Hammers: Grover PM-3/PM-4, Innovative Percussion CL-C1
  (https://www.innovativepercussion.com/products/cl-c1), Kolberg
  (https://www.kolberg.com/en/Products/Percussion-mallets/Chime-mallets-hammers/), Freer CH2
- ISFEE (Indiana), mallet percussion:
  https://timbreandorchestration.org/isfee/extreme-orchestration/percussion/mallet-percussion
- Sample libraries: VSL Synchron Bells I (https://vsl.co.at/products/synchron/bells-1);
  Soniccouture Orchestral Chimes (https://www.soundonsound.com/reviews/soniccouture-orchestral-chimes-collection);
  Versilian Tubular Bells II

Church bells
- A. Lehr, "Partial groups in the bell sound" (1986), on Hibbert's site: https://www.hibberts.co.uk/?p=1841
- W. Hibbert, "The musical sound quality of church bells":
  https://www.hibberts.co.uk/the-musical-sound-quality-of-church-bells/ ; "Building a bell sound":
  https://www.hibberts.co.uk/?p=281
- Chime Master, "Bell weights and sizes": https://help.chimemaster.com/p/Bell_Weights_and_Sizes

Orchestration and repertoire
- C. Forsyth, *Orchestration* (1914), ch. 12 "Bells": https://archive.org/details/orchestration00fors
- S. Adler, *The Study of Orchestration* 3rd ed., "Chimes", pp. 441–442
- H. Berlioz, *Treatise on Instrumentation*, "Bells": https://archive.org/details/treatiseonmodern00berl
- N. Rimsky-Korsakov, *Principles of Orchestration*
- Berlioz, Symphonie fantastique (Bärenreiter preface):
  https://www.barenreiter.co.uk/prefaces/9790006202690_Innenansicht.pdf ;
  http://www.hberlioz.com/Scores/sfantastique.htm
- Parsifal bells: https://en.wikipedia.org/wiki/Parsifal_bell ;
  https://www.steingraeber.de/en/innovationen/parsifal-bells/
- Tosca: https://www.scottishopera.org.uk/media/3799/tosca-programme-deepdive-article.pdf ;
  https://www.sfopera.com/blog/backstage-with-matthew/2018/backstage-with-matthew-the-sounds-of-rome/
- Holst, Saturn: http://www.musicweb-international.com/classrev/2019/Nov/Holst_planets_RR146.html
- Mussorgsky/Ravel: https://slso.org/pictures-at-an-exhibition/
- Mahler 2: https://www.mahlercat.org.uk/Pages/Symph2/Symph2.htm ; Mahler 6: https://mahlerfoundation.org/?p=723
- Pärt, Cantus: https://en.wikipedia.org/wiki/Cantus_in_Memoriam_Benjamin_Britten
- Offstage timing: https://interlude.hk/misplaced-musicians-part-1-orchestral-musicians-play-offstage/
