# Pan flute: what the model lacks, and what the physics says — research notes

Status: 2026-11-27. SI units. **[G]** marks my estimates or guesses, not published values. **[D]** marks values I
derived from published formulas. **[M]** marks measurements of our own model. References are numbered at the end.

The question: where can the pan flute's sound improve? The candidates are the player's mouth and vocal tract; where
the air is blown, at what angle and width; laminar versus turbulent jets; attacks (p, t, d, k, h; ta-ga, da-da-ga);
and glissandi across the pipes.

---

## 0. The model today

- `src/designs.cc` `kPanflutePipes`: 30 stopped cylinders, chromatic G4..C7. The bore runs 15.0 mm at 205 mm down to 6.5 mm at 30 mm.
- `src/flute.cc`: the pipe is chosen per note (`selectPipe`, `buildPipe`). Changing pipe **zeroes the bore**, so
  the new pipe starts from rest and the old one stops dead. The jet is the transverse flute's
  (Verge / Fabre / de la Cuadra, research/jet_and_losses.md). The mouth is one 60 cm³ compliance fed through a
  glottis, with a tongue valve (50 mm²) and a lip slit. **There is no vocal-tract resonance** (horn.cc and reed.cc
  have one, `Embouchure::tract/tractHz`).
- `src/player.cc` drives it with the **transverse flute's rules**:
  - Fletcher 1975 sets the jet length and pressure.
  - Coltman's coverage sets the uncovered width.
  - `y0 = 0.8 mm · (261.6/f)` sets the jet offset: the player's aim, smaller for higher notes.
  - The roll (pitch bend) changes only the uncovered width.
- `src/performer.cc`: glides are switched off for pipes ("a pan flute's pipes cannot").
- The attacks are h (breath), du, tu and accent. All of them use the same tongue valve at the same place.

**[M]** Steady mf notes, dry room, harmonic levels in dB re the fundamental:

| note | H2 | H3 | H4 | H5 | H6 | noise between H1 and H2 |
|---|---|---|---|---|---|---|
| G4 | −38.5 | −7.3 | −32.8 | −27.9 | −36.4 | −72 |
| G5 | −37.6 | −9.9 | −33.1 | −27.4 | −37.3 | −68 |
| G6 | −35.5 | −14.3 | −33.6 | −38.5 | −37.1 | −56 |

Compare Fletcher's player [1]:

| range | even harmonics, below their odd neighbours |
|---|---|
| low note | 20–50 dB |
| mid-range | 10–20 dB |
| higher notes | "approaching" the odd harmonics |

Our even harmonics sit about 35 dB down **at every pitch**. The trend is missing, and the cause is §1. The noise floor
is also very clean (−72 dB at G4). Fletcher's spectra show a breath-noise background peaked at the pipe's odd
resonances, even in normal playing.

---

## 1. The stopped pipe sets the jet's offset itself (Fletcher 2005) — the biggest finding

This is the main physical difference from the flute. Steady flow cannot enter a stopped pipe: its impedance at zero
frequency is infinite. The jet entrains air out of the pipe as it crosses it, and that outflow must be balanced. So
**the mean centre-line of the jet is fixed by aerodynamics, not by the player's lips** [1, §III–IV]. This is also why
the pan flute is easy to sound compared with the shakuhachi.

The derivation, in 2D [1, eqs. 2–6, 9]:

- **Profile relaxation.** The jet leaves the lips (aperture height 2b0) with a top-hat or parabolic profile. Within a
  short distance δ it becomes a Bickley jet, V(z) = V0 sech²(z/b1), with constant momentum flux J = ∫V² dz.
  - Top-hat: b1/b0 = 3/2, and the volume flow becomes U = 1.5 U0.
  - Parabolic: b1/b0 = 4/5, and U = 1.2 U0.
  - Half of the extra entrained flow comes from inside the pipe: β U0, with 0.1 < β < 0.25.
- **Spreading.** The jet then spreads linearly: `b = b1 + ε (l − δ)`, with ε ≈ 0.1 (a half-angle of about 6°) [1, eq. 9].
  - With b grown by α = b/b1, the centre velocity becomes α^(−1/2) V0 and the volume flow α^(1/2) U0.
- **Entrainment from the pipe.** The total flow entrained from inside the pipe is
  `U' = [(α^(1/2) − 1)/2 + α^(1/2) β] U0 = γ U0`, with γ ≈ 0.4 [1, eq. 4].
- **Offset.** To return that flow, the jet's mid-plane is deflected **out of the pipe** by h0:
  `V b [1 − tanh(h0/b)] = U'`, so **`tanh(h0/b) = 1 − 2 γ α^(−1/2)`** [1, eqs. 5, 6].
  - With α ≈ 2 and γ ≈ 0.4: h0 ≈ 0.5 b, about 0.5 mm at the low end.
- **Typical values.**
  - Jet length l: 5–10 mm.
  - Lip half-width: 0.5–1 mm.
  - Half-width at the rim: 1.5–2 mm.
  - V0: 20–40 m/s.
  - cp ≈ 0.4 U0.

Why the even harmonics grow with pitch [1, §V–VI]:

- High notes have a shorter jet (narrower pipe, and players shorten it on purpose) and a faster jet (higher pressure).
- A faster jet also takes longer to relax (a larger δ), which leaves less spreading length l − δ.
- So α is smaller, and **h0/b is larger for high notes**.
- The even harmonics in the injected flow become large once h0 ≳ 0.5 b, with a swing comparable to b
  (Fletcher and Douglas 1980 [2]).
- Against this, players narrow the lip opening on high notes, so players differ in their harmonic balance.

**[D]** How h0/b rises as α falls. γ comes from eq. 4 with β = 0.175; the rows are evaluated from eq. 6.

| α | γ | h0/b |
|---|---|---|
| 2.0 | 0.45 | 0.37 |
| 1.4 | 0.30 | 0.54 |
| 1.1 | 0.21 | 0.70 |

**For our model:**

- The stopped pipe should derive y0 from l, b0 and Uj by eq. 6, instead of the flute player's `0.8 mm · 261.6/f`,
  which runs the wrong way.
- The player's aim and angle become a small trim on top (§2).
- **Check the sign**: Fletcher's offset is outward. Our `y0` is the "jet offset from the labium edge", and the
  sign of `kEtaSign_` must be checked against it.
- A dynamic version may be more faithful: integrate the mean flow into the pipe (the mean of the flow split minus
  the entrainment) and let it push the jet out. The offset then sets itself and follows changes in breath and lips
  with a short lag. That lag could be part of the attack.
- Bore: Fletcher's European instrument follows `d = 1.32 L^0.43` (mm) [1, eq. 1]. Ours has the same exponent (0.44)
  but runs about 15 % wider (15.0 mm against 13.0 mm at 205 mm). The tops are lightly bevelled inside. Leave this as is.
- Wall loss: `σ ≈ 1.2·10⁻⁵ ω^(1/2) / r` [1, eq. 8].

## 2. Where and how the air is blown: angle, width, coverage, and the curved rim

- **Angle.** Flutists blow down at about 25–40°: shallower on low notes and when loud (Fletcher 1975 [3], as quoted
  in [4]). On a flute the angle and offset both change the harmonics strongly, and the lip-to-edge distance less so
  (Acta Acustica 2021 [5]; the full text refused automated fetching, so it is unread).
  - On a stopped pipe, §1 says the aerodynamics take over most of the offset. The angle then acts mainly through
    (a) the jet length to the far rim, (b) the share of the jet that enters, and so the entrainment γ, and (c) the
    coverage. **[G]**
- **Coverage and tilt.** The lower lip covers part of the opening. Tilting the instrument (the pan flute's "roll")
  changes three things together:
  1. The uncovered area S_m, which sets the end correction and the pitch.
  2. The jet length W, because the lips move relative to the far rim.
  3. The jet's angle.

  This gives the coupled pitch-and-timbre bend, roughly 100–150 cents down by covering **[G]**. Today our roll
  changes only (1). A real nai is **diatonic**: its accidentals are made by tilting the pipe about a semitone
  down, so they sound darker and breathier than the open notes **[G; widely described pedagogy]**.
- **Geometry check.** The jet must cross the uncovered part of the opening, so `W ≈ uncovered chord + lip setback`.
  We set them independently. At G4 the player gives W ≈ 5 mm and an uncovered width of about 5.4 mm on a 15 mm
  pipe. They agree only by accident, so they should be tied together.
- **Width and the curved rim (a new idea, [G]).**
  - The edge is a circle. A jet of width H (the lip opening, 7–11 mm in our player) meets the far rim at different
    distances across its span: longest at the centre, shorter towards the sides. It also meets it with a different
    effective offset.
  - Each strip of the jet has its own convection delay. Their sum smears the phase, which lowers the gain and adds a
    natural impurity. A jet wider than the pipe spills air outside, adding edge hiss and wasting flow.
  - Cheap to try: 2–3 spanwise strips with their own W and y0, reading the same displacement history at different
    delays.

## 3. Laminar or turbulent: it depends on the Reynolds number, not on a fixed noise knob

- **Definitions.** `Re = Uj h / ν` with ν ≈ 1.5·10⁻⁵ m²/s.
  - The transition is at roughly Re 2500–3000. It is hard to predict exactly [6][7].
  - Flutes reach Re ≈ 10⁴ and turn turbulent soon after the lips [7].
  - Edge-tone simulations: no sustained oscillation at Re ≈ 500, consistent at about 1000 (2D) [8].
- **Pan flute.** V0 = 20–40 m/s, with a lip aperture of 2b0 = 1–2 mm, gives **Re ≈ 1300–5300 [D]**. The pan flute
  lives across the transition: low and soft notes are laminar, high and loud ones turbulent.
  - Our player (h = 0.75 mm, Uj = 23–53 m/s) gives Re ≈ 1150–2650: never quite turbulent.
- **How turbulence changes the jet** (de la Cuadra, Vergez, Fabre: schlieren, slit jet h = 1 mm [7]):

  | jet | cp/Ub | peak displacement | centre-line velocity decay |
  |---|---|---|---|
  | laminar, Re 500 | ≈ 0.4 | η ≈ 0.8 h | x^(−0.33) |
  | turbulent, Re 3000 | ≈ 0.3 | η ≈ 0.3 h | x^(−0.53), so it spreads faster |

  - Laminar roll-up appears at St ≈ 0.36 and varicose modes at St ≈ 0.68.
  - A short channel with a square exit (like lips) convects faster than a long one, because of thinner boundary layers.
- **The Andean percussive style** [1, §V–VI]. The jet comes in short bursts at high pressure and turns turbulent
  before the rim.
  - **Only the fundamental sounds as a tone.** The rest is noise shaped by the pipe's impedance, with peaks at the
    odd resonances, broadened by the turbulence's dissipation.
  - The same noise background is present in normal playing at a much lower level. Skilled players can reduce it further.
- **For our model** **[G]**:
  - Compute a turbulence share from Re, with a smooth step over about 1800–3500.
  - Blend toward turbulent behaviour as it rises: cp down from 0.4 to 0.3 of Uj, less growth, faster spreading ε,
    more jet turbulence, and a broader pipe Q (the jet's loss).
  - A "golpe" (a hard Andean burst) then needs no special case: it is simply a high-Re attack.
  - Our turbulence already passes through the jet into the pipe (`kTurbIn_`), which is right. It is only too weak
    for this instrument (§0).

## 4. The mouth and vocal tract

- **The mechanism** (Auvray, Ernoult, Terrien, Lagrée, Fabre, Vergez 2015, recorder [9]):
  - The sound reaches the mouth and makes its pressure fluctuate: p′ is 0.1–10 % of the steady pressure. Measured
    p′/p_ac ran from −60 dB up to −20 dB (shrinking the mouth volume raised it).
  - p′ modulates the jet velocity, `u′/U = p′/2P` (Bernoulli), and drives the jet's **symmetric (varicose) mode**:
    cp(s) ≈ 0.8 U and α(s) ≈ 0.2/h, against 0.4 U and 0.3/h for the antisymmetric mode.
  - The antisymmetric mode is driven by the pipe's acoustics; the symmetric one is driven by the tract.
  - The two impedances are not in series.
- **The effects, set by the coupling's phase φ more than its gain:**
  - **The even harmonics change by up to about 20 dB.** Their share against the odd ones spans about 30 dB across φ.
    They rise near φ ≈ π.
  - Pitch moves by ±7 cents. Coltman found about 10 cents by changing an artificial mouth's volume (1973 [10]).
  - The register thresholds move by about ±7 %. φ ≈ π widens the hysteresis.
  - Players do it by changing the mouth volume, opening the velum, or setting the teeth ("gritting").
  - "Not systematic": the result depends on the instrument's own harmonic balance.
- **Why it matters more on a pan flute [G].** The lips are not sealed to a duct, and the pipe's mouth radiates right
  at the lips, so the coupling path is short. Its harmonics are also unbalanced (odd ≫ even), which is the case where
  a tract can move the even ones most.
- **The audible payoff is probably the breath noise [G].** The "h" from the glottis and the turbulence in the mouth
  pass through the tract before reaching the lips. So "huu", "hoo" and "hii" breath have different colours, and that
  is the vowel you hear in a breathy pan flute.
  - Today our glottal hiss is white noise low-passed once, entering the mouth as a pressure, with no formant.
- **For our model:**
  - Reuse horn.cc's tract (the throat's inertance on the mouth's volume, tuned to tractHz with Q) between the
    glottis and the lip slit.
  - Feed it from the window's pressure through the lip slit's acoustic impedance (the p_ac → p′ path), not just from
    the jet's draw.
  - Let the jet velocity follow the mouth's p′. That gives the symmetric-mode drive in its simplest form: a
    modulation of Uj with its own delay at 0.8 U.
  - Set the vowel per syllable (§5).
- **The pasted model's suggestion** (a band-pass resonator on p_m before Bernoulli) is a reasonable first sketch.
  Coupling it to the window pressure is what makes the phase φ, and so the harmonics, depend on the vowel.

## 5. Attacks: the consonant, the vowel, and fast tonguing

Today there is one tongue valve at one place, with timing variants (h, du, tu, accent). Real attacks differ in
**where the valve is**, **which volume sits behind and in front of it**, and **what the vowel is**. The brief
pedagogy: "poo" for soft attacks, where the lips close, trap the air and let it out cleanly [11]; tu-ku double
tonguing [12]; siku players use flutter tonguing and overblowing [13].

| syllable | valve | what happens **[G]** | what to expect |
|---|---|---|---|
| **p** ("puuh") | the lips themselves | The mouth fills to full pressure behind closed lips. The **aperture h grows from 0** over about 5–15 ms. A thin jet at full speed, then a widening one. Plosive puff: a short broadband burst plus a low "pop" of flow onto the rim. | A thin, fast first jet: a high-θ chiff, possibly a flash of the upper register. A softer, rounder body. No tongue click. |
| **t** | tongue tip at the teeth or ridge, sealed | The lip aperture is already set. A fast release (2 ms). A small volume in front of the valve. | Sharp and immediate (as today's tu). |
| **d** | tongue tip, a lighter seal | A slower release (about 8 ms), less overpressure. | Today's du. |
| **k / g** ("ga") | the tongue's back on the soft palate | **The whole oral cavity lies in front of the valve**, so at release it must fill before the lips see the pressure: slower and softer, with a slight lag. The tongue is low and back, which gives a different tract (§4). | Softer, slightly late, a darker vowel. This difference is what makes ta-ga uneven unless practised. |
| **h** | the glottis | As today, but the hiss should be tract-coloured. | Airy start. |
| **f / fu** | nearly closed lips | Turbulent from the first moment (high Re, narrow slit). | A breathy entry. |
| **golpe** (Andean) | the chest or diaphragm | A pressure pulse over the tongue release. | Turbulent burst: the fundamental plus pipe-coloured noise (§3). |
| **scoop** (nai) | tilt | Start covered, uncover over 50–150 ms. | The pitch rises into the note, darker at first. |

- **The vowel after the consonant** ("duuh" or "diih", "puuh" or "paah") sets the tract for the note: its resonance,
  and so the coupling's phase (§4) and the breath noise's colour. The syllable is a (valve, vowel) pair.
- **Fast repetition.** Players choose the pattern from the tempo and the grouping:
  - **ta-ga-ta-ga** for 8ths and 16ths;
  - **da-da-ga** (or ta-ta-ka) for triplets;
  - tu-ku in general.

  The performer can pick it from inter-onset times of about 60–150 ms and from the grouping it hears. The **"ga"
  strokes then really are different** (back valve, more volume in front, a darker vowel): slightly softer, later and
  darker. A practised player evens this out; `practised_` / gesture can set how much.
- Variety: draw each attack's timing, overpressure and vowel around the syllable's values. The ±15 % wander exists
  already.

## 5b. Vibrato and expression

These items come from notes the user pasted, without sources. Treat them as practice to check by ear. They are
mapped here onto what the model has.

| technique | what the player does | in the model today | to do |
|---|---|---|---|
| Diaphragm vibrato, 4–6 Hz | Lung pressure pulses: amplitude and brightness, a little pitch. | Present: `vib_`, the lung share (`1 − kVibThroat_`). | Nothing beyond tuning the rate and depth for the pan flute. |
| Throat or glottal vibrato, 6–8 Hz | The glottis pulses: narrower, faster, a husky edge. | Present: the throat share narrows the glottis, whose hiss is `kGlotNoise_`. | Tract-coloured hiss (§4) gives the "husky" its vowel. A faster rate when the throat share dominates. |
| Hand or wrist vibrato | The instrument rocks: W, the angle and the coverage oscillate. Mostly timbre and pitch, little level. | Missing. | Modulate the tilt control (§2 / §8 #2). Do it per player style (nai: yes). |
| Semitone by tilt, 100–150 c | Rolling the rims in covers the bore and shortens the jet. | Roll changes only the uncovered width. | §2: tilt drives coverage, W and angle. With a diatonic layout the accidentals are tilted notes. |
| Scoop and fall | Start tilted and roll upright; roll in at a phrase end. | Missing. | An attack (§5, scoop) and a release gesture (a tilt ramp of 100–300 ms as the breath ends). |
| Subtone and core | A dropped jaw and a wide aperture scatter the jet into breath; a tight oval gives a ringing, focused tone. | Partly present: `air_` widens the lip opening and spreads the jet. | Make it reach turbulent Re and colour the noise by the pipe (§3) and the tract (§4). |
| Overblowing to H3 (a twelfth) or H5 | A pressure spike with a shorter, thinner jet. | The jet model can, but the player never asks for it. | Check with `-ramp` that the stopped pipes jump to their 3rd and 5th. Use it as a climax effect or a performer gesture. |
| Ghosted scales and harp-like runs | A continuous airstream sweeps across pipes; intermediate pipes are brushed in wide leaps. | Missing: pipes cannot glide. | §6. |

## 6. Moving between pipes: legato and glissando

Physically, the player slides the instrument (or the head) along the row. The jet's target moves from pipe A across
the wall between the pipes to pipe B.

- **The overlap.**
  - While the lip jet (7–11 mm wide) spans two pipe mouths, **both pipes are blown**, each in proportion to its share
    of the jet.
  - The wall between them (about 1 mm of bamboo, 2–3 mm of wood on a nai **[G]**) takes the jet's middle briefly:
    a dip in the tone and a puff of edge noise.
  - Each pipe has its own jet length, because the far rim sits at a different place in each.
- **The old pipe rings on.** A pipe of Q about 30 decays in Q/(π f), roughly 25 ms at G4 **[D]**. That is audible as
  the overlap of a legato. Today `selectPipe` zeroes the bore, so there is a click of silence and no overlap.
- **A glissando run.** Sliding past several pipes without tonguing sounds each one briefly: a scale in the
  instrument's tuning (the classic nai run). Sliding on one pipe bends the pitch by breath and coverage [11]. Players
  sometimes articulate a second instead [11].
- **For our model:**
  1. Keep 2–3 pipe bores alive, each with its own window branch.
  2. Make the jet's lateral position x(t) a player state.
  3. Split the jet's flow (and its displacement drive) between the pipes by overlap: the jet's width against each
     mouth, minus the wall.
  4. A slur moves x over 15–40 ms. A glide gesture (the performer's soft overlapped key) runs x through the
     intervening pipes at a speed set by the interval. Pipes the jet leaves ring down on their own and still radiate.
  5. Allow glides for pipes in `performer.cc`.
- With a diatonic layout (§2), a chromatic glide becomes a diatonic run, as on the real instrument. Whether to offer
  that layout is a design choice, below.

## 7. The pasted model (closed waveguide plus a Fabre–Hirschberg–Verge jet)

The pasted description and ours share most of the structure: Bernoulli Uj, delay plus growth, a Bickley tanh split,
vortex losses with α_vc 0.6, and a stopped quarter-wave pipe. Where they differ:

- **Its jet offset is the player's aim**, and "η0 = 0 gives purely odd harmonics". For a stopped pipe, §1 says the
  offset sets itself at about 0.5 b outward. This is the most important correction to it, and to us.
- Its source is `½ρUj² (h w_rim/S_p)(tanh …)`, a momentum-flux form. Our Verge dipole source with δd = (4/π)√(2hW)
  is better grounded; keep ours.
- The vocal tract as a band-pass on p_m is a fine start (§4), but the coupling's phase to the window matters.
- **Worth taking: one tilt control mapped to W, S_m and δL together** (§2). Its 100–150 c bend range is plausible.
- The waveguide and its lumped loss filter: our finite-difference bore with wall losses and radiators is already
  more detailed.

## 7b. Decisions (2026-11-27)

- **One instrument for both styles**, the Romanian nai and the Andean siku. The player's style picks the
  behaviour: smooth and vibrato-rich, or breathy, percussive and golpe.
- **Bass pipes are welcome**, with a shakuhachi or bass-flute feel. The range now runs C3..C7 (49 pipes). The bore
  follows the same law below G4, up to 26 mm and 64 cm at C3, as on a bass nai or toyo. The lips and the uncovered rim
  grow with the bore, x (d / 15 mm)^0.7 (`PJ_PIPELIPS`; tools/design.py mirrors it, so the pipes are cut for it).
- **Vibrato is planned by the director, and the flutist chooses its kind** (as for the singer). The director's cues
  stay as they are: `kCueVibDepth`, `kCueVibDelay`, `kCueVibRate`. The pan flute player turns them into a mix of:
  - **diaphragm** (the lungs, 4–6 Hz);
  - **throat** (the glottis, 6–8 Hz, husky);
  - **hand** (tilting the instrument: coverage, W and angle; pitch and timbre, little level).

  The player chooses by the note's length, register and style, and by how deep the cue asks. For example: a long
  nai note at a climax gets the hand plus the diaphragm; a short note, the throat; a siku, mostly the diaphragm. The
  hand vibrato waits for the tilt control (§8, #2).

## 7c. Done so far

- **The jet's offset on a stopped pipe** (§1) is in `Flute::jetCoefs`. `PJ_PAN_AERO` (1) turns it on;
  `PJ_PAN_AIM` (0.25) is the share of the player's own aim kept on top. Pipes only: not the bottles, which are
  Helmholtz resonators. **[M]** Steady mf, dry, the even harmonics below their odd neighbours:

  | note | H2, before | H2, after | H4, after |
  |---|---|---|---|
  | C3 | — | 39 dB | 22 dB |
  | C4 | 34 dB | 37 dB | 14 dB |
  | G4 | 35 dB | 34 dB | 9 dB |
  | G5 | 33 dB | 22 dB | 4 dB |
  | G6 | 28 dB | 13 dB | −5 dB |

  H2 at G6 is now level with H3 (−24 against −23 dB), the trend Fletcher measured. The level wobble also fell
  (G4 from 5 % to 3 %).
- **The bass.** In tune within 1–2 c. C3 sits 13 dB under G4. Calibration still covers its rim more than the
  geometry suggests (3.9 mm of 26 mm): the jet pulls the long pipes sharp. Breath noise is very low there (−79 dB).
  That is §3's job, and where the bass flute's feel will come from.

- **The vibrato** (2026-11-27). In `-play`, a wind's vibrato comes only from the director's cues; without them its
  depth is 0. A performed file (for example "Just Friends Trio", whose event times don't match its ticks) got none,
  so the pan flute played with no vibrato at all.
  - Now the director plans the vibrato for every note, whatever the file has of its own (`Direction::vib`,
    `PJ_DIRECTOR_VIBALL`).
  - The pan flute player (`Player::direct`) makes the vibrato x1.6 a flute's (`PJ_PAN_VIB`) and chooses its kind:
    - the hand, 0.35 of it (`PJ_PAN_HAND`), more for a deeper cue and half on the top notes. It rocks the
      rim's coverage: log uncovered width ±3 per unit (`PJ_PAN_HANDK`).
    - the throat: 0.1, plus up to 0.3 for a quicker cue.
    - the diaphragm: the rest.
  - **[M]** Single notes at a cue of 1 (`PJ_NOTE_VIB=1`), rms pitch wobble: C4 2 → about 5 c, C5 2 → 7 c,
    C6 3 → 10 c (peaks about 1.4 x). The level wobble also grows, to about 20 %, because covering the rim
    costs level.
  - The hand still moves only the coverage. The tilt control (#2) will add the jet length and angle.

- **Phrase ends** (2026-11-27, by request). The last note of a phrase, but not of the piece's last phrase, is blown
  softer and more diffuse.
  - The director marks it: `kCuePhraseEnd` is 1, or 2 for the piece's last phrase. It reaches a performed file
    too, through the vibrato-only cue (`kCueOnly`; the singer ignores phrase ends there).
  - The player, over about 0.15 s (`endAir_`):
    - dynamics −0.12 (`PJ_PAN_ENDDYN`);
    - air +0.3, a wider, less focused lip opening (`PJ_PAN_ENDAIR`);
    - breath x0.6 (`PJ_PAN_ENDP` 0.4);
    - the jet x√ of the breath, so the lips come forward and its travel time holds;
    - the rim uncovered by `0.17 · √(261.6/f)` (log), to cancel the airy lips' flattening (`PJ_PAN_ENDTUNE`).
  - The top pipes are left alone: none at C7, fading out from G#6.
  - **[M]** C4–G6: −1 to −6 c (C3 −13 c), 1–1.6 dB softer, breath noise 2–4 dB more against the tone.
  - Without the jet shortened, 25 % less breath went 25–40 c flat, and 40 % less broke the high notes.
- **The blown bottles get everything above** (the user: "a pan flute with glass bottles"):
  - the self-set jet offset; it steadies them, C4's level wobble 9 % → 3 %;
  - the vibrato kinds and the phrase ends;
  - the lips scaled by the neck, the rim, at 20 → 10 mm. tools/design.py recuts the bottles for it;
  - +3 dB.
- Also: the pan flute +3 dB by ear (`src/levels_data.h`).
- **The bottles C2..C8** (the user: "two octaves at both ends"). The C4..C6 bottles kept, the same geometric
  run on: a 196 mm jug (neck 144 × 40 mm) at C2, a 9 mm vial at C8. The vials' necks narrow at half the rate
  above C6, to about 7 mm at C8. At 5 mm, C#7..C8 needed twice a player's breath and the rim at its limit
  to tune.
  - **[M]** Calibrated: 73/73 within 2 c except C2 (+1 c, 1.45 × breath).
  - Radiation grows about 9 dB an octave out there, 5 dB in the middle. The player trims it
    (`PJ_JUGTRIM` 6.5, `PJ_VIALTRIM` 5 dB an octave below C4 and above C6), glided over 40 ms.
    Applied at once, a vial's last ring got a jug's gain: the fuzz peak was 3.1–4.9.
    **[M]** C2 −36, C4 −30, C6 −20, C8 −9 dB.
- **No lip closing at a pipe's release** (the user: "a click at the end of C2"). The lips closing over 12 ms
  on the offset jet let the ringing pipe flap the slowing jet wholly in and out: a square wave, +49 dB of
  highs on C2, +35 dB on the pan flute's C3. Pipes have no lower register to fall into, so the player only
  stops the breath. **[M]** C2 +5 dB (the breath's end), the rest under 3 dB; dry decays smooth (C2 60 dB
  in 0.35 s).
- **Fast runs double- and triple-tongued** (the user: "ta-ga-ta-ga" for 8ths and 16ths, "da-da-ga" or
  "da-ga-ga" for triplets).
  - The director's `kCueTongue`, `PJ_DIRECTOR_TONGUE` 1. On any flute part, a run of three or more joined notes
    under 0.2 s each gets each note's place in a pair or a three, from the run's own note value on the
    grid. Threes are triplets, or a compound beat's eighths. It reaches performed files with the vibrato
    (`kCueOnly`).
  - The pipe player's syllables: pairs ta-ga (a "d" on a key under 50, a "k" on an accent); threes da-da-ga,
    or da-ga-ga by habit (`PJ_PAN_TKK`; a seeded player one time in three). "g" is the back of the tongue,
    a 3 ms seal and 14 ms release; "k" the same firmer. A tongued legato elsewhere is the tip's "l": 0.8 ms,
    the air barely broken.

- **Long notes shaped by the player itself** (2026-11-28, by request: "even if the director is not giving
  instructions, long notes should either have expansion (if there is plenty of other activity), or at least slight
  decrescendo if there is no activity, … with the end of the note when vibrato starts with no or less decrescendo —
  or if the long note is a ramp up to a higher activity and key velocity, … a crescendo towards the end").
  - **What the player hears** (`src/midifile.cc`, after the director; `kCueOwnLen`, `kCueOwnBusy`, `kCueOwnRamp` in
    `src/director.h`): the note's length, the other parts' note-ons per second while it sounds (a drum stroke
    counting half), and whether it leads up. Leading up means the part's next note comes within 0.6 s, at least 2
    semitones higher (0.4) or 6 velocity harder (0.3), with the band busier after it (+0.3).
  - The pan flute's and the bottles' notes without a cue get one carrying only that, even with `-director off`.
  - **When:** the cue gives no shape: a vibrato-only cue, or no swell and the level moving under 0.05 through
    the note. From 0.6 s long; full from 1.2 s; deeper for the longest (× up to 1.3); ±20 % per note.
  - **What** (`Player::shapeOwn`, in dynamics, and heard as loudness at `PJ_PAN_OWNDB` 16 dB per unit, as the
    director's):
    - **expansion** over a busy band (3 note-ons a second or more; blended from 1): a grow to +0.12 by 60 % of the
      note, easing 30 % over its last 15 % (`PJ_PAN_EXPAND`);
    - **sinking** over a quiet one: −0.1 across the note; from 40–70 % of it on, a quarter as fast once the vibrato
      has come in (`PJ_PAN_DECR`). Not on a phrase's last note, which tapers already;
    - **crescendo** into the higher, harder note: up to +0.18 from 40 % of the note to its end (`PJ_PAN_CRESC`).
      Where the director already gives one (its level rising through the note), the director's is kept.
  - **[M]** `research/gm_compare/work/panflute-long/long.mid` (90 bpm, 4.7 s notes, the pan flute alone, level
    against the note's start at 30 / 55 / 75 / 90 % of it, dB):

    | | busy band | quiet band |
    |---|---|---|
    | without (`PJ_PAN_EXPAND=0 PJ_PAN_CRESC=0 PJ_PAN_DECR=0`) | +1.9 +0.6 +0.8 +0.2 | +1.0 +1.0 +0.9 +0.3 |
    | with | +2.9 +2.6 +2.8 +2.1 | +0.6 +0.0 −0.2 −0.9 |

- **A long last note of a solo line does not fade while the band plays on** (2026-11-28, by request: "long
  solistic last note should not easily fade away if others are still playing — and likely it should have vibrato
  in the end", for any wind or string).
  - **Which notes** (`src/midifile.cc`, the same pass): any wind (not the bagpipes) or solo string; 1.2 s or longer;
    a phrase's last note, or the part's last, or followed by over 1 s of rest; no other note of its own part under
    it. Others must play on: note-ons under it at 1 a second or more, or a note sounding through its end.
  - **What:**
    - no phrase-end taper (`kCuePhraseEnd` cleared);
    - its level not falling through it, nor the director's level curve under its start;
    - vibrato if it had little (depth 1, coming in after 45 % of the note, at most 1.5 s in), or its delay
      brought to half the note at most.
  - `PJ_HOLD_LAST=0`: as before. The file player prints how many it held.
  - **[M]** The test file's last note (4 beats, the band on 8ths under it): +0.2 −0.1 +0.7 +0.6 dB before, +1.4 +2.4
    +3.2 +2.9 dB now (held, and expanding over the busy band).
  - Listening: `long-new.wav`; `long-noown.wav` (the player's own shapes off, the held last note still on); `long-before.wav` (both off).

- **The other winds and the solo strings shape their long notes too** (2026-11-28, by request: "give it there --
  probably the long notes should be very considerate about their surroundings so that they don't become comical").
  - **The same shape** (`src/longnote.h`, shared with the pan flute), smaller: expansion 0.08, crescendo 0.12,
    sinking 0.06 (`PJ_OWN_EXPAND`, `PJ_OWN_CRESC`, `PJ_OWN_DECR`). It goes through the part's level, as the
    director's swells do (`ownLevel` in `src/midifile.cc`): heard as loudness at 16 dB per unit, with only 0.4 of it
    in the breath (`PJ_OWN_BREATH`). With the whole shape in the breath, a clarinet expanded +3.5 dB and sank −3.6 dB:
    its level follows its breath strongly.
  - Uncued notes too: `-director off`, or a part the director leaves alone.
  - **Considerate** (in the score pass; the pan flute gets the same, through `kCueOwnSize`):
    - nothing for a part that plays chords (its notes overlapping more than 15 % of the time);
    - no growing for a pad's note (two other parts or more beginning notes of 0.6 × its length with it);
    - no expansion under a busy melody (1.5 or more note-ons a second over 2 semitones above it), and half the
      crescendo;
    - a crescendo only into a clear rise (its lead-up 0.6 or more);
    - a run of long notes (1.2 s or more, under 1 s apart): the second shaped 0.7, the third on 0.35.
  - **[M]** `long-<instrument>.mid` (the test file, its lead played by each), the lead alone, against the same with
    the player's shapes and the held last note off (dB against the note's start at 30 / 55 / 75 / 90 %):

    | | busy band, before → now | quiet band, before → now |
    |---|---|---|
    | flute | +1.1 +0.9 +0.7 +0.2 → +2.1 +2.8 +2.6 +1.7 | +2.2 +1.0 +1.8 +1.7 → +2.0 +0.5 +1.2 +1.1 |
    | clarinet | +1.3 +2.6 +1.0 +1.7 → +2.3 +5.1 +3.6 +4.1 | +1.4 +0.6 −0.1 −0.9 → +1.2 +0.0 −1.2 −2.6 |
    | trumpet | +0.5 +0.4 −0.4 −1.0 → +2.1 +2.9 +2.4 +1.2 | +1.1 +0.1 −1.9 −3.0 → +0.8 −0.5 −2.6 −3.7 |
    | oboe | +0.2 +0.4 −0.8 +0.0 → +1.2 +2.3 +1.1 +1.6 | +0.7 +0.2 +0.1 −0.3 → +0.4 −0.3 −0.6 −1.0 |

    The violin's level wanders ±4 dB between the two renders: no reading.
  - The ramp note is the same in both: the director's own crescendo there is kept.
  - `consid.mid` (`mk2.py`): a clarinet long note under a busy flute melody is unchanged (no expansion). A chord
    held by clarinet, flute and oboe: none of them grows.
  - Listening: `long-<instrument>-new.wav` against `-before.wav`; `consid-new.wav` against `consid-before.wav`.

## 8. Proposed order of work, with checks

| # | change | check |
|---|---|---|
| 1 | The stopped pipe's aerodynamic offset (§1), with the player's trim on top; verify the sign. | Measure even and odd harmonics: low 20–50 dB apart, mid 10–20, high nearly equal [1]. Then by ear. |
| 2 | Tie W to the uncovered chord; one tilt control moves coverage, W and angle (§2). | The bend range and how its timbre changes. |
| 3 | A turbulence share from Re: cp, growth, spread, noise, Q (§3). | The noise floor rises at the odd resonances; a loud or golpe note goes "fundamental plus coloured noise". |
| 4 | The vocal tract between the glottis and the lips, coupled to the window; the vowel per syllable (§4). | Hiss colour by vowel; the even harmonics move with the vowel; ±5–10 c. |
| 5 | The attack table: p, t, d, k/g, h, f, golpe, scoop; ta-ga and da-da-ga chosen by tempo (§5). | Listen to repeated 16ths and triplets. |
| 6 | Several live pipes, the jet sliding over them, the old pipe ringing, diatonic runs (§6). | Legato scales and glissandi by ear. |
| 7 | Expression (§5b): the hand-tilt vibrato, the scoop and fall, overblown climaxes. | By ear. `-ramp` for the registers. |

Smaller experiment: spanwise strips of the jet on the curved rim (§2).

Still unread: the jet-angle paper [5] (publisher blocks fetches); de la Cuadra's thesis [6] (over 10 MB); Czyzewski
and Kostek's waveguide panpipes [14]; Verge and Hirschberg on turbulence noise [15].

## References

1. N. H. Fletcher, "Stopped-pipe wind instruments: Acoustics of the panpipes", JASA 117(1), 370–374 (2005).
   https://newt.phys.unsw.edu.au/music/people/publications/Fletcher2005.pdf
2. N. H. Fletcher, L. M. Douglas, "Harmonic generation in organ pipes, recorders and flutes", JASA 68, 767–771 (1980).
3. N. H. Fletcher, "Acoustical correlates of flute performance technique", JASA 57, 233–237 (1975).
4. "Acoustical principles of flute technique" (secondary copy): https://www.scribd.com/document/234058417/Acoustical-Principals-of-Flute-Technique
5. "Effects of jet angle on harmonic structure of sound radiating from the flute", Acta Acustica (2021). https://doi.org/10.1051/aacus/2021003
6. P. de la Cuadra, "The sound of oscillating air jets: physics, modeling and simulation in flute-like instruments", thesis (Stanford, 2005).
   https://ccrma.stanford.edu/~pdelac/research/MyPublishedPapers/Thesis.pdf
7. P. de la Cuadra, C. Vergez, B. Fabre, "Visualization and analysis of jet oscillation under transverse acoustic perturbation", arXiv:0811.0333.
   https://arxiv.org/html/0811.0333v1
8. A. N. Janjua, "Flow instabilities in flute-like instruments", thesis (U. Houston). https://uh-ir.tdl.org/bitstreams/f8c20dc9-b9b6-4e30-a626-a478755f67a3/download
9. R. Auvray, A. Ernoult, S. Terrien, P.-Y. Lagrée, B. Fabre, C. Vergez, "Effect of changing the vocal tract shape on the sound
   production of the recorder", Acta Acustica u. Acustica 101, 317–330 (2015). https://arxiv.org/abs/1601.05545
10. J. W. Coltman, "Mouth resonance effects in the flute", JASA 54, 417 (1973). https://ccrma.stanford.edu/marl/Coltman/Papers.html
11. L. Garrison, performance guide to Donjon's "Pan!". http://leonardgarrisonflute.blogspot.com/2014/03/performance-guide-to-johannesdonjons.html
12. Pan flute (Grokipedia; unvetted): https://grokipedia.com/page/Pan_flute
13. R. Ball, "Tips and tricks for playing the siku" (NFA). https://www.nfaonline.org/docs/default-source/committees-documents/global-flutes-committee/siku.pdf
14. A. Czyzewski, B. Kostek, "Waveguide modelling of the panpipes", SMAC-03, 259–262 (2003).
15. M.-P. Verge, A. Hirschberg, "Turbulence noise in flue instruments", ISMA 1995, 94–99.
