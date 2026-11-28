# Tom-toms as physical models, and brushes on them

Status: 2026-11-15. Research and a model plan; nothing is built. It extends `jazz_drums.md` (§5 the
snare's heads, air and brushes; §6 the toms' row; §8.4 the snare model) and borrows the taiko's
two-head work (`steelpan_taiko.md` §2). The toms now are `melodic-tom`, a circuit (an analog drum
voice), through `src/drumkit.cc`.

Marks: **[paper]**, **[web]** a source below; **[jd]**, **[st]** from jazz_drums.md or
steelpan_taiko.md; **[calc]** worked out here (the script is in §7); **[memory]** general knowledge,
to be checked; **[design]** a choice of this plan.

## 1. Short answer

- **A tom is two heads on a cylinder of air.** Both are struck as one instrument: the batter is hit,
  the resonant head answers through the air and the shell.
  - The drum's fundamental is the two heads' (0,1) modes moving **in phase**, carried by the enclosed
    air. It always couples strongly, whatever the drum's depth **[paper, WB15]**. The mode sits well
    below either head's own (0,1).
  - The modes with nodal diameters, (1,1), (2,1) and so on, couple weakly. The coupling falls with the
    drum's depth and with the number of diameters m. At 55 cm the (1,1) of a 12" drum is gone from the
    far head **[paper, WB15]**.
- **A tom glides; a brushed tom hardly does.** A hard stroke stretches the head, so the pitch starts
  sharp and falls as the vibration decays. That is the tom's "boww".
  - For jazz toms, at 10 mil: about 15–25 cents at 1 mm of head motion, 50–90 cents at 2 mm, and
    1–2 semitones at 3 mm (a forte stick) **[calc]**.
  - A brush moves the head 0.1–0.5 mm: 0–3 cents **[calc]**. The glide must therefore come from the
    amplitude, as `ModalSet`'s `glide` already does, not from a fixed pitch envelope per stroke.
- **Why brushes on a modelled tom usually sound wrong, and what fixes it:**
  1. **A soft stick is not a brush.** A light, slow mallet gives a clean, pure "boom" at low level,
     an 808 tom played quietly. A brush is two things at once:
     - the fan: a soft, heavy spring pushed by the hand. Its contact lasts 7–40 ms **[calc]**, longer
       than the tom's period (6–14 ms), so it gives the fundamental a soft bloom and the head's higher
       modes almost nothing;
     - the wire tips: a hundred tiny, light, scattered impacts spread over 2–5 ms **[jd]**. They give
       the "tss" that says brush, broadband and partly radiated by the wires themselves.
     Model both; neither alone is a brush.
  2. **On a tom the brush jumps straight back up** (§4.2). The hand lifts as it lands, the contact is
     over in 1–3 ms (§4.5), and the tom rings freely after it. A second bounce is a second MIDI
     key, never the model's own. Only at the lowest velocities does the brush stay down and draw a
     longer hiss, damping the ring a little while it lies there (§4.3). The snare is different: there
     the brush is often pressed or left down.
  3. **The sweep through a tom is a pitched whoosh.** The texture noise at the moving contact point
     drives the low modes, which ring at their own pitch. On a tom, unlike the snare, the fundamental
     sings faintly under the noise, and its colour changes as the brush circles (§4.4).
  4. **No two strokes alike.** The tips' arrival pattern, the footprint and the place differ every
     time **[design]**.
- **The engine has most of it.** `ModalSet` has modes with their shapes, a mallet that lands on the
  head where it is now (so a stroke meets the ringing, as the kit should), contact patches, the
  amplitude glide, linear couplings and damping. Missing: more linear couplings, many small contacts
  (the tips), a moving excitation point, and noise forcing (§5).

## 2. The drums: sizes, tunings, tensions

### 2.1 What jazz drummers use

- **Kit:** a 12" rack tom and a 14" floor tom, the bebop kit with an 18" kick. Some kits add a 13" or a
  16" floor tom **[jd, memory]**.
- **Tuning:** high and open; both heads on; coated single-ply batters (10 mil) with clear or coated
  resonant heads; little or no muffling **[memory]**.
- **The tom's pitch.** A tuning guide gives fundamentals, the in-phase (0,1) mode, by size and set
  **[web, OL12]**:

| Size | Typical fundamentals (Hz), over ten set tunings |
|---|---|
| 8" | 165–208 |
| 10" | 131–165 |
| 12" | 98–131 |
| 13" | 87–104 |
| 14" | 82–98 |
| 16" | 65–73 |

  - Those are rock and pop tunings. Jazz toms sit a few semitones higher: a 12" near 130–165 Hz and a
    14" near 98–110 Hz **[memory, to be measured]**.
  - The toms of a set are tuned in fourths, fifths or the notes of a chord **[web, OL12]**.
- **The user's notes (2026-11-28)** **[user]**, the jazz ranges above confirmed:
  - 12" × 8" rack tom C3–F3 (130–175 Hz); Max Roach and Philly Joe Jones pushed it up near E3–F3, a
    taut, bongo-like snap over the bass and piano. 14" × 14" floor tom G2–C3 (98–130 Hz), well above
    the upright bass's low register. A 16" floor tom E2–G2 (82–98 Hz), tight enough to speak with
    brushes.
  - Intervals matter more than absolute pitch: a **fourth** (floor G2, rack C3) is the traditional,
    open, conversational one; a **fifth** (F2, C3) wider and more dramatic; a **major third**
    (Bill Stewart, Brian Blade) a tight, vocal cluster.
  - Heads: **equal tension** top and bottom for the most sustain and a round fundamental with little
    glide; the **resonant head higher** (up to a minor third) gives an upward swell on the stroke
    and a shorter decay, which keeps fast bebop articulate in live rooms. **No muffling**: no gels,
    tape or internal dampers.
  - The model (`src/tom.cc`) as of this date: 12" 164.8 Hz (E3), 14" 110 Hz (A2, a fifth under), 16"
    92.5 Hz (F#2), the low 16" 73.4 Hz (D2: GM's low floor tom, under the jazz range); both heads at
    one lug tension (`PJ_TOM_RESO` 1.0), light damping only. All within the ranges. Not tried yet: a
    fourth (the 14" up to B2, 123 Hz) and the resonant head a little higher (`PJ_TOM_RESO` above 1).

### 2.2 Lug pitch and the drum's pitch

- **The lug pitch** is a head tapped lightly near one lug. It is that head's own (1,1) mode, about 1.59
  times its own (0,1) **[web, OL12; memory]**.
- **With both heads at the same lug pitch, the drum's fundamental is the lug pitch ÷ 1.6–1.9**, about
  ÷ 1.75 **[web, OL12]**. Against the ideal membrane's 1.593, that means the in-phase (0,1) pair sits
  **0–20 % below** each head's own (0,1). The enclosed air adds mass to the in-phase motion, as in the
  taiko, where the in-phase mode sits at ×0.975 **[st]**.
  - So a tom's (1,1)/(0,1) ratio is about 1.6–1.9, not 1.59. That ratio is a large part of its
    character, and a target for the model **[calc]**.
- **Resonance (sustain) depends on how the heads are tuned against each other** **[web, OL12]**:

| Setting | Batter lug ÷ fundamental | Resonant lug ÷ fundamental | Sound |
|---|---|---|---|
| maximum | 1.75 | 1.75 | longest sustain |
| high | 1.5 | 1.85 | |
| medium | 1.4 | 2.0 | |
| low | 1.2 | 2.3 | short, "controlled"; good for recording |

  - Heads tuned alike share their energy back and forth. Heads far apart detune the pair and the
    drum dies sooner.
  - With the resonant head higher the sound is more controlled. With the batter higher the attack is
    stronger **[web, OL12]**.
  - The model gets this for free once both heads are there with their own tensions: a "resonance"
    knob is the resonant head's tension.

### 2.3 Tension and the glide [calc]

- **Head:** coated 10 mil Mylar, ρ_s ≈ 0.26 kg/m² **[jd]**; E h ≈ 4.5 GPa × 0.254 mm = 1.1 MN/m.
- **Tension:** from each head's own (0,1) = lug ÷ 1.593, with both heads at maximum resonance. The
  glide follows the Berger-type average tension, as worked out for the taiko **[st §2.4]**:
  f ≈ f₀ √(1 + β a²/2), β = 0.857 E h/(R² T₀), where a is the (0,1) amplitude at the centre.

| Drum | Fundamental (Hz) | Lug (Hz) | T₀ (N/m) | β (m⁻²) | 0.3 mm | 1 mm | 2 mm | 3 mm |
|---|---|---|---|---|---|---|---|---|
| 12", jazz | 165 | 289 | 1350 | 31 000 | 1 c | 13 c | 52 c | 114 c |
| 12", rock | 124 | 217 | 770 | 55 000 | 2 c | 24 c | 91 c | 192 c |
| 14", jazz | 110 | 192 | 820 | 38 000 | 1 c | 16 c | 63 c | 136 c |
| 14", rock | 98 | 172 | 650 | 48 000 | 2 c | 20 c | 79 c | 168 c |
| 16" | 73 | 128 | 470 | 50 000 | 2 c | 22 c | 83 c | 177 c |

- **How far a stroke moves the head** **[calc]**:
  - The head's (0,1) modal mass is small: σ π R² × 0.27 ≈ 5 g (12") and 7 g (14").
  - A 40 g stick is far heavier, so the head moves off at about the stick's speed: a ≈ v/ω. That is
    3–4 mm at 3 m/s (forte), 1 mm at 1 m/s and 0.5 mm for a ghost note.
  - So a forte stick starts a jazz tom 1–2 semitones sharp. That is large, and the air loading and the
    resonant head (which shares the motion) bring it down: **fit it**, and expect a semitone or less
    on a tight jazz tom.
- **The glide's time constant is half the (0,1) amplitude's** (a² decays twice as fast) **[st]**.
- The literature on synthesizing this: the energy-based tension modulation of Avanzini, Marogna and
  Bank **[paper, AMB10; AMB12]**. The short-time average tension is nearly proportional to the
  vibration's energy, so the glide costs almost nothing. That is what `ModalSet`'s `glide` does.

### 2.4 Modes and damping

- **One head alone:** the ideal membrane's ratios to about the first eight modes (1, 1.593, 2.136,
  2.295, 2.653, 2.917, 3.156, 3.50). A taiko's measured head fits them within 1.6 % **[st]**. A thin
  Mylar head has no stiffness to speak of.
- **Air loading** of each head lowers its low modes, the (0,1) most. With the coupling, that gives
  the 1.6–1.9 ratio of §2.2.
- **The pairs**, as two-mass models; Rossing finds the snare's six lowest resonances form two pairs
  **[jd]**:
  - **(0,1):** in phase (heads move the same way, carrying the air; the fundamental, low) and out of
    phase (the air a spring between them; a few to 15 % higher). On the taiko the pair is ×0.975 and
    ×1.059 with equal heads, and it widens as the heads are tuned apart **[st]**.
  - **(0,2):** the same, weaker.
  - **(1,1) and higher diameter modes:** weakly coupled. Each head mostly keeps its own, beating
    against the other's when they are tuned close. That is part of a tom's "ring".
  - **Split degenerate modes:** uneven tension around the lugs lifts the degeneracy of each (m,n)
    with m > 0 into a close pair **[paper, WB15]**. A few Hz apart, it beats slowly. Give each m > 0
    mode two members, 0.3–1 % apart **[design]**.
- **Damping [memory, to be measured]:**
  - Open jazz toms ring: the fundamental's T60 is about 0.6–1.5 s, longer on the floor tom.
  - The snare's (0,1) is damped at 5 % of critical by radiation **[jd]**. A tom's in-phase (0,1)
    radiates as a dipole (both heads moving the same way) and keeps its energy longer. That is why a
    tom sustains and a snare does not **[memory]**.
  - The higher modes, about 1 % of critical: 0.1–0.3 s at 300–1000 Hz.
- **Shell:** maple or birch, thin and damped. Weak in the sound, except where it carries the (1,1)
  coupling **[paper, WB15; memory]**. A rack tom's mount (suspension or a post into the shell) damps
  it too: a post through the shell shortens the sustain. That is why suspension mounts exist
  **[memory]**.
- **Sympathy:** a tom rings when the kick or another tom near its pitch plays, and it buzzes the
  snare wires **[jd]**. That comes later, with the kit.

## 3. The stick on a tom

- The engine's mallet: mass, speed, Hunt–Crossley contact (K, α), landing on the head where it is
  now. A stick: m ≈ 40 g at the tip, hickory on coating (K, α as the snare's), contact 0.5–1.5 ms
  **[jd]**.
- **Strike position:** the centre gives a fat, short, mostly-fundamental sound. Off centre (5–10 cm)
  brings in the (1,1) family and the ring. The taiko players' "good" sound is about 10 % off centre
  **[st]**.
- **Rim shot on a tom:** rare in jazz; the snare's hoop model when it exists.
- **A stroke meets the ringing.** The mallet lands on the head's present position and speed, so a
  stroke on a ringing head adds to or cancels its motion depending on the phase. A stick resting a
  moment after the stroke ("dead stroke") damps it. Both come out of the contact as it is.

## 4. Brushes on toms

### 4.1 The tool and what it does to a head [calc unless marked]

- **A wire:** steel, 0.3 mm thick, 10 cm out of the handle.
  - Mass 0.055 g; as a cantilever its tip carries ≈ 0.013 g.
  - Bending stiffness at the tip 3 E I/L³ ≈ 0.24 N/m: a single wire is almost nothing.
- **The fan:** 100–200 wires, spread 8–12 cm **[jd]**. Pushed by the hand into the head, the wires bend
  and slide and the fan acts as a soft spring:
  - stiffness of order 100–1000 N/m, with the handle's 5–15 g behind it [estimate];
  - half-period π√(m/K) = **7–40 ms**, a soft, damped push.
- **What the head gets:**
  - **The push** lasts longer than the tom's period (6 ms at 165 Hz, 14 ms at 73 Hz). Its force
    spectrum falls off above about 1/(2 t_c) = 12–70 Hz, so it gives the fundamental a gentle bloom
    and the higher modes little.
    - The stiffer and quicker the fan (a short fan, a "slap" with the wrists), the more it gives.
      Pressing the fan flat in ("slappy") gives the most **[web, MD]**.
  - **The tips**, many tiny impacts spread over 2–5 ms (the fan's tilt × the hand's speed) and over a
    footprint of about 4 × 8 cm **[jd]**. Each tip is light, but together they reach the high head
    modes and the air: a "tss". Averaged over the footprint, the head's high modes partly cancel
    (the footprint filter) **[jd]**. Much of the "tss" is the wires' own radiation, not the head's.
  - **Head amplitude:** a brush at 1 m/s moves the head about 0.1–0.5 mm, so the glide is 0–3 cents.
    The brushed tom keeps its pitch.
- **The result**, in words: a soft, dark thump with the drum's pitch clear and steady, a breathy "tss"
  on top, and the drum ringing freely after it.

### 4.2 How a tom is played with brushes (the user's account)

- **A tom is struck so that the brush jumps back up.** The hand lifts as the brush lands (a
  "pull-out" stroke). The contact ends quickly and **the tom rings freely after the brush hit**.
  - That is the normal stroke. It is unlike the snare, where the brush is often pressed or left down.
- **No bounce of its own.** A second touch, if wanted, is the drummer's: each bounce is its own MIDI
  key. The model must not add a second stroke the file did not ask for. The hand's path takes the
  brush away, so it cannot fall back (§4.5).
- **At the lowest velocities the brush may keep the contact** and drag lightly over the coating, for
  a longer hiss. The drum still sounds under it, a little damped while the brush lies on it.
- **So, per key:**

| Velocity | Stroke | Contact | After |
|---|---|---|---|
| ordinary (about 25–127) | pull-out: down at the key's speed, the hand turning up at contact | 0.7–3 ms: 1–3 wire-end touches (§4.5) | the head rings free |
| lowest (about 1–25) | placed and drawn: the brush is set down lightly and drawn ~1–3 cm before it lifts | 30–150 ms, longer the softer [design] | a hiss with the drum's pitch in it (§4.4), the ring a little damped (§4.3), then free |

  The threshold and the drag's length are to be set by ear [design]. The pressed slap and the dead
  stroke of §4.3 stay possible (a slap kit key, a held note) but are not what a tom key does.

### 4.3 The brush resting on a moving head: damping and sizzle

(On a tom only during the low-velocity drag of §4.2, or for a slap. A normal tom stroke never
rests.)

- **Damping.** The fan's wires lie on the coating with friction. Each mode whose shape moves under the
  footprint loses energy to them: `ModalSet::damp`, but weighted by each mode's shape under the fan.
  A per-mode rate ∝ (shape²) × the fan's pressure [design]. The (0,1) under a centre brush dies
  fastest.
- **Sizzle.** A wire resting on the head is a snare wire in small. The head moving down by more than
  the tip's own sag lets the tip fall, and the head coming back up hits it **[jd, the snare wires]**.
  - It needs the head's speed at the footprint to exceed what gravity and the wire's spring let the
    tip follow: so only while the head moves fast, in the loud first few tens of ms.
  - Model: 2–4 contact patches under the fan (the engine's `cShape`/`cHeight`), each a light tip
    mass. Their contact forces, high-passed, go straight to the output too, as the wires radiate it
    **[jd §8.4, as the snare wires]**.

### 4.4 The sweep and the swirl on a tom

- The snare's plan carries over **[jd §5.6]**:
  - bristle groups at a moving point;
  - a texture height s(x) read at the brush's speed (2–30 kHz per tip);
  - a contact force k_c (s − z_head) under the hand's pressure;
  - the modes driven through their shapes J_m(k r) cos(m θ) at the point.
- **On a tom the difference is the low modes:**
  - The force noise excites the (0,1) pair and the (1,1) family, which ring on, so the sweep has the
    drum's pitch in it.
  - As the brush circles at radius r, the (0,1) is driven by J₀(2.405 r/R) and the (1,1) by
    J₁(3.832 r/R) cos θ. The pitch content changes along the circle, once per turn (the patent's
    1/rev variation **[jd]**), and the (1,1) pair's two orientations take turns.
- **The resting fan's damping** keeps the ring short and the sweep from building up into a drone
  **[design]**.
- **The low-velocity drag of §4.2** is the same exciter over a short straight path, 1–3 cm at
  0.1–0.3 m/s under a light load (0.1–0.5 N) [design]: a short hiss through the tom's modes.
- Brushes are swept on the toms less than on the snare; a tom sweep is an effect, a fill or a ballad
  colour **[web, MD; memory]**.

### 4.5 The rebound [calc: `tools/brushbounce.py`]

A brush is not a fixed force pulse. It is a small mechanism thrown at a head that moves:
- the **hand** moves a target down and back up, or holds it pressed;
- the **handle** (10–30 g) follows the hand through the **grip**, a spring: none when the drummer
  throws a relaxed tap, about 2000 N/m when holding it firmly;
- the **fan** is a spring with friction (the wires rubbing on each other and sliding on the coating)
  from the handle to the **wire ends** (~1 g together);
- the wire ends meet the head through a stiff contact.

The tool drops it at 1 m/s onto a tom's centre modes: the coupled (0,1) pair, (0,2) and (0,3). What
comes out:

- **The stroke is three things in time:**
  1. The wire ends land and bounce: 1–3 short contacts of 0.5 ms each, about 1 ms apart. That is
     the start of the "tss", and with the fan's loss it is 1–2 bounces.
  2. The fan compresses and pushes for 10–40 ms. That is the bloom.
  3. The fan throws the handle back: the rebound.
- **Rebound.** With little loss in the fan (damping ratio 0.3) a brush comes back at e ≈ 0.8. That is
  too lively: drummers find brushes give little rebound and must be lifted **[memory]**.
  - At a loss of 1.5–3 the rebound is e ≈ 0.35–0.5, against 0.9–0.96 for a stick. Fit it there.
  - The loss also takes the wire ends' bounces from 3 down to 1–2.
- **The drum changes the stroke.** On the same throw a loose floor tom moves 2.5× as far as a tight
  12": 0.72 against 0.29 mm at the centre. The softer head gives way, which takes the stroke's energy
  into the head, not back into the brush.
- **Fan and handle:**

| Fan (N/m) | Handle (g) | Wire-end bounces | Fan contact (ms) | (0,1) amplitude, 14" |
|---|---|---|---|---|
| 150 | 10–30 | 4 | 20–40 | 0.24–0.35 mm |
| 400 | 10–30 | 2 | 15–27 | 0.29–0.50 mm |
| 1000 | 10–30 | 1 | 11–20 | 0.51–0.68 mm |

  - A short, stiff fan (wires pulled into the handle) plays louder and with fewer bounces.
  - A long, soft fan gives more chatter and a softer bloom. That matches what drummers do with
    retractable brushes **[memory]**.
- **The grip:**
  - With a firm grip the fan's contact is shorter (7.5 against 21 ms): the hand lifts the brush.
  - Held down (a slap or a dead stroke) it never leaves.
- **A stroke onto a ringing head takes some of the ring away and mixes in its own.** Take a 14" ringing
  at 0.5 m/s (0.7 mm, a stick stroke just before). The (0,1) energy 60 ms later, against no brush
  stroke:

| Drum | Tap (thrown) | Dead stroke (held) |
|---|---|---|
| 12" | −10.5 / −10.8 dB | −8.7 / −9.9 dB |
| 14" | −5.8 / −7.1 dB | −8.2 / −7.6 dB |
| 16", loose | −3.1 / −7.8 dB | −3.7 / −13.7 dB |

  The two numbers are two phases of the ringing head as the brush lands: moving up into it, or down
  away from it. On the floor tom the phase decides it, 3 against 8 dB, 4 against 14 dB. So repeated
  brush strokes on a ringing tom never come out the same, and a quick second stroke partly chokes the
  first. That is the "dampen some of it and mix the rest" of the kit, from the mechanism alone.
- **Limits of the tool:**
  - only centre modes: no (1,1) family, so off-centre strokes are not covered;
  - one lumped wire end, not 8–16 groups;
  - no gravity, which would bring a relaxed brush back down for a second, soft touch on a slow
    hand;
  - the stick in the same tool stays on 10–19 ms, because the head's higher modes, which make it let
    go sooner, are missing.
  The numbers are for the shape of the behaviour, to be fitted by ear and to recordings.

**The tom stroke as played (§4.2): pull-out.** The grip is firm (2000 N/m) and the hand turns up as
the brush lands (`pullout()` in the tool):

| Drum | 0.2 m/s | 0.5 m/s | 1 m/s | 2 m/s |
|---|---|---|---|---|
| 12" | 1.1 ms (2 touches) | 1.4 ms (1) | 2.2 ms (1) | 3.3 ms (2) |
| 14" | 0.7 ms (1) | 1.2 ms (2) | 1.7 ms (3) | 3.0 ms (2) |
| 16" | 0.7 ms (1) | 0.6 ms (1) | 1.3 ms (2) | 2.2 ms (2) |

- **The brush is gone within 1–3 ms.** The touches are the wire ends' bounces inside the one stroke,
  under a millisecond apart: they are the start of the "tss", not a second stroke.
- **After that the head rings freely.** Its (0,1) energy follows the stroke's speed squared: +6 dB per
  doubling (−41, −35 dB at 1 and 2 m/s on the 12").
- **So the long fan push of the thrown stroke above (10–40 ms) is not a tom stroke.** It is what a
  relaxed brush does when the hand does not take it away. The bloom of the tom's note is then the
  free ring itself, not a push.
- **The stroke onto a ringing head** still meets the head where it is. With a 1–3 ms contact it takes
  less of the ring than the long push: the phase decides whether the new stroke adds to the ring or
  partly cancels it.

### 4.6 What would make it sound bad (a checklist for listening)

| Fault | Cause | Fix |
|---|---|---|
| An "808" tom: pure, smooth | only the push, no tips | the tip impacts and their direct radiation |
| A paper "tss" with no drum | only the tips, the push too short | the push's K and mass: the bloom of the (0,1) |
| A glide on a soft stroke | a fixed pitch envelope | the glide from the amplitude only (§2.3) |
| A hiss-free swirl or a drone | noise not shaped, no damping | the texture, its speed law, the resting damping |
| Every stroke the same | fixed tip pattern | random arrival times, places and footprints, per stroke |
| A brush that "bounces like a stick" | a lossless fan, a prescribed pulse | the fan's friction (e ≈ 0.35–0.5, §4.5) |
| Repeated strokes pile up the ring | the stroke adds force but takes nothing | the brush as a mechanism touching the moving head (§4.5) |
| The brush clings, a muffled tom | a long push or a resting fan on an ordinary stroke | the pull-out: the hand lifts at contact, the tom rings free (§4.2) |
| A double stroke the file did not ask for | the brush falling back | one key, one stroke: the hand's path never returns |
| A flat-pitched "box" | the (1,1)/(0,1) ratio at 1.59 | the air coupling to 1.6–1.9 (§2.2) |

## 5. Model plan

### 5.1 One tom = one `ModalSet` vibrator, both heads in it

- **Modes:** both heads' modes, 2 × ~32, to about 2.5 kHz (the snare's choice, 64 modes in all
  **[jd §8.4]**).
  - Each head is the ideal membrane at its own tension. Every m > 0 mode is a split pair (§2.4).
  - Air loading lowers each head's (0,1) and (0,2).
- **Air coupling:**
  - linear couplings between the heads' (0,1)s and (0,2)s, strong;
  - weaker ones for (1,1), (2,1);
  - each g fitted so that the in-phase (0,1) lands at lug ÷ 1.75 with equal heads, the out-of-phase
    one 5–15 % above, as on the taiko;
  - `Def::kLin` grows from 2 to about 8 [design].
- **Radiation:** per mode, `rad`; the (0,n) pairs as dipole (in phase) and monopole (out of phase).
  Above the modes (2.5 kHz up), the contact's own high-passed force is the sound, as with the snare
  wires.
- **Glide:** `glide` on the (0,1) amplitude, β from §2.3, scaled to the fit.
- **Damping:** per mode: the T60s of §2.4, then fitted.
- **Three drums** from one table of sizes: 12" (rack), 14" and 16" (floor), with their tunings.
  - The keys: 41/43 floor, 45/47 mid, 48/50 rack, as GM's low/high pairs. The second key of each pair
    is the same drum tuned a little higher (a 13" between the 12" and 14") **[design]**.
  - Or a kit of three: 50 and 48 rack, 47 and 45 the 13", 43 and 41 floor. Six GM toms on three
    drums, the pairs a third apart, as real kits do **[design]**.
- **One drum = one instrument living through the song** **[design, as the cymbal plan]**: strokes on
  the same drum go into the same ringing heads. Not a note pool.

### 5.2 The exciters

- **Stick:** the existing mallet, at a place on the head (centre to 0.6 R). The shape vector comes
  from the Bessel table (§5.3).
- **Brush strokes: the mechanism of §4.5**, integrated per sample against the head:
  - a hand target, the stroke's path by velocity (§4.2): a pull-out, down at the key's speed and
    turning up at contact; at the lowest velocities, placed and drawn (the hiss), then lifted; for a
    slap key, held. One key is one stroke: the path never returns for a second touch;
  - the grip (a spring; relaxed: none) to the handle, 10–30 g;
  - the fan, a spring-damper with friction (K 150–1000 N/m, damping ratio 1.5–3), to N_b = 8–16 wire-end
    groups;
  - each group a tiny mass (0.05–0.2 g) on its own share of the fan, with a small random offset in
    height and place within the footprint. They land over 1–5 ms, each through a stiff contact with
    the head at its own point (its shape vector).
  - Their summed contact force, high-passed above ~2 kHz, is added to the output (the wires).
  - The rebound, the bounces, the bloom, and what the stroke takes from a ringing head all come out
    of it (§4.5). A stroke is never a stored pulse.
  - The engine's mallet is one per vibrator. The groups need a small pool, up to 16 at once [design].
- **Brush resting:** a damp rate per mode, ∝ its shape² at the fan, plus 2–4 contact patches with
  light tips for the sizzle.
- **Sweep:** a moving point (its shape vector recomputed every 16 samples) and a texture noise force
  under a normal load **[jd §8.4]**.

### 5.3 Engine additions [design]

| What | Why | Cost |
|---|---|---|
| `kLin` 2 → 8 | the two heads' pairs | 8 MAC/sample |
| Mallet pool (16 small ones) + the brush's handle, grip and fan | brush tips; the rebound | per tip, while touching: 64 × 2 MAC/sample |
| A force at a moving point | the sweep; the tips' scattered places | a shape vector per 16 samples |
| Per-mode damping by shape | the resting brush | 64 multiplies/sample while resting |
| Bessel table (m 0–6, n 1–4, 64 radii) | a shape at any (r, θ) | none at run time |
| A direct output of the contact force | the wires' radiation | trivial |

- **Cost per tom:** 64 modes ≈ the snare head's, about 0.02–0.04 of a core. Three toms ringing ≈ 0.1
  [calc, the snare's estimate].
- **The engine is shared** (`src/modal.*`). The pool, the moving point and the shaped damping are
  wanted by the snare, the ride (brush on cymbals) and the hi-hat as well. Build them once, for the
  toms first.

### 5.4 Order of building, and the checks

1. **Heads and air, struck with a stick:**
   - the pair frequencies against the lug ÷ 1.75 rule;
   - the resonance settings of §2.2 (sustain falls as the resonant head moves away);
   - the glide against §2.3, then by ear;
   - **A/B with the separated jazz stems:** find tom fills (`drums.md`'s tools), measure the
     fundamental, its T60 and the glide. The fits come from there.
2. **Brush strokes** (the pull-out, and the drag at the lowest velocities):
   - check that the contact is over in a few ms and the head then rings free;
   - check that no stroke touches twice beyond the wire ends' bounces;
   - check that the brushed tom's glide stays under 5 cents;
   - the bloom's level against the stick's, at the same velocity;
   - the "tss" share;
   - A/B on brush passages in the stems (*Some Cats Know*, 1:01–1:10, has brushes on the snare
     **[drums.md]**; tom strokes with brushes need finding, likely in ballads).
3. **Sweep on a tom:** by ear first; then measure the pitch in the noise against the drum's
   fundamental.
4. **Into the kit:** `drumkit.cc`'s tom keys (41, 43, 45, 47, 48, 50), the Brush kit's toms with the
   brush exciter, sympathy with the kick and snare later.

## 6. Gaps

- **No published measurements of a two-headed tom's modes** were found in two searches: only the
  coupling's form **[paper, WB15]** and the tuning guide's ratios **[web, OL12]**. Rossing's book
  (Science of Percussion Instruments; Fletcher & Rossing ch. 18) is the place to check.
- **No measurement of brushes on toms**, nor a model of them, was found; neither for the snare
  **[jd]**. FluidR3's Brush kit uses the stick tom samples (key 45 differs by under 5 % from the
  Standard kit's) [measured here], so it is no reference.
- **Damping and glide amounts** are estimates until fitted to recordings.

## 7. Sources and the calculation

- **[WB15]** R. Worland, B. Boe, "Using a dual electronic speckle-pattern interferometer to study
  coupled vibrations in drumheads", Proc. Third Vienna Talk on Music Acoustics, 2015, pp. 148–151.
  https://viennatalk2015.mdw.ac.at/proceedings/ViennaTalk2015_submission_43.pdf
  - A 12" tom cut and rejoined at 2–110 cm.
  - The (0,1) couples strongly at all lengths, in phase.
  - Diameter shapes couple less with length and with m.
  - Uneven tension splits degenerate modes.
- **[OL12]** Overtone Labs, "Drum-Set Tuning Guide" (Tune-Bot), 2012.
  https://tune-bot.com/tunebottuningguide.pdf. The tom fundamentals by size; the lug pitch 1.6–1.9 ×
  the fundamental; the resonance settings.
- **[AMB10]** M. Marogna, F. Avanzini, B. Bank, "Energy based synthesis of tension modulation in
  membranes", DAFx 2010. **[AMB12]** F. Avanzini, R. Marogna, B. Bank, "Efficient synthesis of tension
  modulation in strings and membranes based on energy estimation", JASA 131(1), 2012.
  https://air.unimi.it/handle/2434/654153
- **[MD]** Modern Drummer, "Jazz Drummer's Workshop: Focus on Brushes" (S. Fidyk):
  https://moderndrummer.com/?p=28808. The "slappy" pressed stroke and the "swish".
- **§4.5:** `tools/brushbounce.py` (semi-implicit Euler at 2 µs; head modes as §2.4 with the in-phase
  and out-of-phase (0,1) at twice one head's modal mass).
- **The calculation of §2.3 and §4.1:**
  - ρ_s = 0.26 kg/m²; E h = 1.14 MN/m;
  - each head's own (0,1) = lug/1.593, lug = 1.75 × fundamental;
  - T = ρ_s (2π R f₀₁/2.405)²; β = 0.857 E h/(R² T);
  - cents = 1200 log₂ √(1 + β a²/2);
  - head modal mass σ π R² 0.27;
  - wire 0.3 mm × 10 cm steel: k = 3 E I/L³, tip mass 0.24 m;
  - fan half-period π √(m/K).

## 9. Build plan (2026-11-15)

What exists now and changes §5: the other session's `Membrane` (two heads with a Bessel shape table,
air mass on the m = 0 modes, the stretch glide), `DrumAir` (the cavity spring with its vent) and
`Brush` (hand, grip, handle, fan, wire-end groups; tap, slap, sweep) cover §5.1–5.3; nothing in
`src/modal.*` is needed. `src/kick.*` (the jazz kick on the same parts) is the template.

1. **`src/tom.h/.cc`:** a `Tom` (batter + resonant head + `DrumAir` + one brush + a stick),
   `TomModel` (Tube) and `TomPlayer` (Performer), as the kick's.
   - Six drums, one instrument each, living through the song: `tom-10` (50), `tom-12` (48),
     `tom-13` (47), `tom-14` (45), `tom-16` (43), `tom-16-low` (41). Each key keeps its own pitch,
     as General MIDI expects, even though a jazz kit has two or three of them.
   - Jazz tuning, both heads at the same lug pitch (§2.2, maximum resonance): the drum's note at
     196, 165, 147, 110, 92.5, 73.4 Hz. The tension is solved for at construction: the pair is
     simulated briefly and both heads retuned until the in-phase (0,1) lands on the note.
   - Modes: about 250 on the batter and 120 on the resonant head, the top set by that budget.
     Above it, the contact's own sound (the stick's click, the wire ends' "tss") is added
     directly, as the kick and snare do.
2. **Strokes:**
   - **Stick:** a mass (25 g at the tip) on a Hunt–Crossley contact, thrown from just over the head
     where it is now. The hand lifts it again; one key, one stroke.
   - **Brush (the key + 24):** the pull-out stroke of §4.2 (`Brush::strike` with no dwell, a firm
     grip): the contact ends within a few ms and the drum rings free.
   - **Lowest velocities (under 24) brushed:** the drag, 1–3 cm over 30–150 ms at a light load,
     then lifted.
   - Each stroke lands a little elsewhere, around 0.35 R (the phrase-held drift of
     stroke_placement.md §7.2 is left for the song context).
3. **The room** presses on both heads as on the kick (strength 0.3).
4. **Checks** (`tools/test.py`, `toms()`):
   - each drum's note within 4 % of its target;
   - the out-of-phase partner above it;
   - glide: ff stick > 20 c, feathered < 6 c, brushed < 5 c;
   - brush contact < 5 ms;
   - velocity range 16..127 about 30 dB;
   - fuzz and real time.
   Then trims to the common loudness, demos, and the `drumFor` lines for the other session.

### 9.1 Built (2026-11-16)

- **Files:** `src/tom.h/.cc`. Six instruments (`tom-10` … `tom-16-low`), registered in kPlucked,
  `voice.cc` and `levels_data.h`. A test, `toms()`, in `tools/test.py`.
- **Notes:** the self-tuning puts each drum's in-phase pair on its target within 0.1 %. The batter's
  (1,1) then lies 1.89–2.05 × the note, against the tuning guide's 1.6–1.9 for the perceived lug
  pitch. The model's air loading on the m = 0 modes is a little stronger than the guide implies;
  left as the membrane model has it.
- **Sustain (T60 of the note):** 0.51 s (10") to 1.44 s (low 16"), as §2.4 expected.
- **Glide:**

| Drum | Stick at 4 m/s (velocity 127) | Feathered (velocity 30) | Brushed (velocity 90) |
|---|---|---|---|
| 10" | +2 c | 0–1 c | 0–1 c |
| 12" | +10 c | 0–1 c | 0–1 c |
| 13" | +18 c | 0–1 c | 0–1 c |
| 14" | +58 c | 0–1 c | 0–1 c |
| 16" | +60 c | 0–1 c | 0–1 c |
| 16" low | +147 c | 6 c | 6 c |

  The low 16"'s 6 c is the estimator's step.
- **Contact:**
  - stick 6.5–8.7 ms: a 15 g stick on a head whose (0,1) carries 5–9 g rides it down for half
    a period of the loaded mode, as §2.3 expected. Shorter than that needs a lighter stick or the
    hand's pull, by ear;
  - brush pull-out 3.4–4.2 ms;
  - the soft drag about 95 ms.
- **Velocity:**
  - stick, 16 → 127: 27.5–33.3 dB;
  - brush, 30 → 127: 18–25 dB, and 6–13 dB under a stick at the same velocity;
  - the drag 23–28 dB under a velocity-90 brush stroke.
- **Cost:** about 330 ns/sample per ringing drum (≈ 1.6 % of a core), 248 + 107 modes; asleep when
  silent. Making one drum, with its tuning, takes 60–120 ms (remembered per kind).
- **Not done:**
  - the phrase-held placement drift (stroke_placement.md §7.2);
  - a stick rim shot on a tom;
  - sympathy with the kick and snare (it needs a shared room, §2.4);
  - fitting the damping and glide to recordings of real tom fills.
