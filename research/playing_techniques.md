# Flute playing techniques as physical recipes — research notes

Status: draft, 2026-11-11. Purpose: list the sounds a flutist makes (articulations, dynamics,
ornaments, extended techniques), say how each is made with air, tongue, lips and fingers, and turn
that into settings for the pajupilli player model (REQUIREMENTS.md R10–R17). Section 5 is a
**proposal** for the MPK mini mk3 mapping.

Notation: **[E]** = my estimate (no measurement found; set it by ear and expose it as a knob, R23).
Source keys are in brackets and listed in §6. Most numeric articulation data in the literature is
for the **clarinet** or the **recorder**, not the flute. I flag where I carry it over.

## 0. Model variables used below

| Symbol | Meaning (see player_and_controller.md, jet_and_losses.md) |
|---|---|
| p_L | lung pressure set-point (slewed), the "support". Vibrato modulates it. |
| p_m | mouth pressure (plenum behind the lips, τ ≈ 1.5 ms) |
| a_T | tongue valve opening 0…1 between mouth and lips. 0 = tongue sealed. |
| A_lip | lip-slit area, which sets jet thickness h and flow. The main dynamics control [F75]. |
| W | jet length, lip to edge. Shorter W → higher regime and sharper. |
| c | lip coverage of the embouchure hole. More c → flatter and darker [UF][WA18]. |
| y0 | jet offset/angle relative to the edge. It sets the even/odd harmonic balance and regime stability [DC08]. |
| σ_n | breath-noise gain |
| o_k | openness of tone hole k, 0…1. It slews over 8–17 ms when pressed, 9–22 ms when released [AL09]. |
| "normal" | p_ref(note), A_ref, W_ref(note), c_ref ≈ 0.5, y0_ref, for a mezzo-forte steady tone |

Three facts set the time scale:

- A flute's perceptual attack time is about 10 ms [AL09, citing Gordon 1987].
- Pitch can be identified after about 4 periods, which is under 10 ms for flute notes [AL09].
- The organ-pipe (flute-like) transient lasts **160 / 45 / 26 ms** for slow, medium and fast
  pressure rise. A fast rise lets higher regimes and edge-tone components appear briefly ("as with
  tonguing") [CA04].

So the attack *shape* is decided in the first 5–50 ms. That is the region the tongue valve and
the p_L envelope must get right.

## 1. Building blocks (what a player can do)

### 1.1 Air (lungs + abdomen + larynx)

- **Support level**: a steady p_L. Fletcher's four players differed on dynamics. Two kept the
  blowing pressure nearly constant from pp to ff and changed the lip opening instead; two varied
  the pressure more [F75 via ANU abstract]. The model should therefore do dynamics mostly with
  A_lip, plus a moderate p_L change (R10/R13 already says this).
- **Pressure shaping inside a note**: a swell, a decay, or a push (accent). On the clarinet,
  measured mouth pressure for an *accented* or *sforzando* note rises to a **higher peak than a
  normal note and is then reduced**. A *normal* note is released at the **lowest** pressure, with
  a slower rise [LI16]. I assume the flute is the same in kind **[E]**.
- **Breath ("h") onset and endings**: the glottis opens or closes, or the abdomen relaxes. Ramps
  take tens of ms **[E: 20–80 ms]**.
- **Vibrato**: p_L modulated at about 5 Hz by ±10 % [F75], with the abdomen and/or the larynx
  [HT00][BB21]. The sound is amplitude and timbre modulation of the upper partials, with FM under
  1 % [F75].
- **Voice**: singing while playing modulates p_m at the voice's f0 (player_and_controller.md A3).

### 1.2 Tongue (consonants)

The tongue gates the flow "in an almost binary fashion" [WS03]. Pedagogy uses syllables [TW][TC]:

| Syllable | Where the tongue is | Valve model (a_T) **[E]** | Character |
|---|---|---|---|
| **t / tu** | tip at the back of the upper teeth or gum | full seal; release 1–3 ms | crisp, clear front, a short noise burst |
| **d / du** | same place, lighter, flatter contact | seal or near-seal (residual 0–15 %); release 4–8 ms | softer; used for legato tonguing and détaché |
| **k / ku** | back of the tongue on the soft palate | full seal; release 2–5 ms; a larger downstream volume (the whole mouth) gives a slightly slower p_m ramp | slightly duller than t; the 2nd syllable of double tonguing |
| **g / gu** | soft version of k | release 5–10 ms, partial seal | partner of du in du-gu |
| **l / dl ("doodle")** | tongue tip touches lightly, lateral air escape | dip of a_T to 0.3–0.6, 5–15 ms | barely articulated; jazz/baroque legato |
| **h** (breath) | no tongue | a_T = 1; p_L ramps from 0 over 20–80 ms | soft, airy, slow-speaking start |
| **p / pu** (lip) | lips closed, then burst | A_lip = 0 during the closure, then opens in 2–5 ms with p_m already at p_L | percussive "pop"; used in pizzicato |
| **(none)** | slur | a_T stays 1 | only the fingers change the note |

- **Clarinet data** [LI16]: more tongue force, or a sudden release, makes the note start earlier
  and makes the **higher harmonics grow faster** in the transient. It does **not** change the
  exponential growth rate of the fundamental.
- **Recorder data** [EF17]: novices use longer mouth-pressure rise times than experienced players.
- **Release noise**: inject turbulence at about 2× σ_n for 10–30 ms at the release
  (jet_and_losses.md §3) **[E]**.
- **Speed limits** **[E]**: single tonguing up to about 7–9 notes/s; double (t-k) and triple
  tonguing up to about 12–16 notes/s. In fast multiple tonguing the k is weaker than the t, so
  players practise evening it out [TW][TC].

### 1.3 Lips (embouchure)

- **A_lip (aperture)**: bigger → more flow → louder and fuller. Flutists are taught to reduce the
  aperture for high notes [UF].
- **W (jet length)**: shorter W, faster jet, or a smaller aperture → a higher regime. "You can play
  the first seven or eight [harmonics] by closing all the tone holes and blowing successively
  harder (or by narrowing the lip aperture)" [UF]. Coltman calls jet length perhaps the most
  important variable of playing technique [via DC08].
- **Coverage / rolling in and out**:
  - Rolling in, or covering more, lowers the pitch "significantly", because the end effect
    grows [WA18].
  - Rolling out, or pushing the jaw forward, raises it [UF].
  - Lipping is easier on short-tube (high) fingerings [UF].
  - Range **[E]**: about −50…−100 cents down and about +20…+40 cents up on middle-register notes.
    It is less on low notes, where the impedance minima are deep.
- **Jet angle / offset y0**:
  - Aiming the air lower (into the hole) flattens the note; aiming higher sharpens it.
  - "If you just simply blow harder without compensating at all, the pitch will go up". The fix is
    to direct the air more into the flute [FS][TI-sharp].
  - Offset changes the H2/H3 balance as much as flow rate does (Acta Acustica 2021 study,
    [AA21]).

### 1.4 Fingers

- Key times [AL09]:
  - pressed by a finger: 8–17 ms, about 10 ms typical;
  - released by the spring: 9–22 ms, about 16 ms typical;
  - per finger, pressed / released: L-thumb 8.7/9.2, L-ring 17.0/22.2, R-ring 8.3/16.7 ms.
- **Finger asynchrony** in multi-finger changes [AL09]:
  - professionals: 13 ± 9 ms with three fingers, 21 ± 11 ms with four;
  - amateurs: 25 ± 16 and 34 ± 14 ms.
- **What the gap produces** [AL09]:
  - "Safe" orders give little audible change.
  - "Unsafe" orders give a **detour**: a brief wrong note, e.g. a near-G5 between E5 and F#5.
  - Or a **dropout**: the tone stops for tens of ms and a broadband component near 12 kHz appears.
  - Or the jet is **trapped** on the old note.
  - Professionals chose safe orders 57–97 % of the time, depending on the passage.
- **Partial venting**: sliding a finger off an open-hole (French) key ring, or half-holing. This
  gives continuous pitch slides [FG]. R8's continuous o_k already covers it.
- **Alternate, trill, microtone and multiphonic fingerings**: the UNSW Virtual Flute catalogue
  [UF-VF].
- **Cross fingerings** have weaker high harmonics, so they sound darker and quieter [UF].

## 2. Catalogue: conventional articulations and dynamics

Values are relative to "normal". IOI = inter-onset interval. Note length is a fraction of the IOI.
All numbers are **[E]** unless a source is given. The direction of each effect is from the cited
pedagogy and acoustics.

| Technique | Air (p_L envelope) | Tongue | Lips | Fingers | What you hear |
|---|---|---|---|---|---|
| **Legato / slur** | continuous; 0–10 % push on each new note in expressive playing; for an upward register jump raise p_L ~10–30 % within ~10–30 ms of the key change | none (a_T = 1) | shorten W / narrow A_lip on upward register jumps, reverse downward | change only; asynchrony 10–35 ms [AL09] | smooth connection; fast changes may show a short detour or dropout [AL09] |
| **Tongued legato / détaché** | continuous, unbroken support | soft "du" per note: closure 10–30 ms, release 4–8 ms | unchanged | fingers change during the closure | each note separated by a tiny gap; length ~90 % |
| **Tenuto / portato (louré)** | full value, slight swell on each note (+5–10 %) | very light "du"/"l", maybe incomplete seal | — | — | notes touching, gently pulsed |
| **Staccato** | support kept (p_L stays up). The note ends with the air (a quick p_L dip or glottal stop, 5–20 ms) or the tongue. Pedagogy favours starting with the tongue and not ending with it ("Tah not Tut") [UI]. | crisp "tu", release 1–3 ms | — | — | detached, light; length ~40–60 % of IOI. Staccato marks shortness, not a heavier tongue [TI-clar]. |
| **Staccatissimo** | as staccato, shorter; a small overshoot (+10 %) | "t" closure ends the note (tongue stop) | — | — | very short (20–35 % of IOI); the bore rings down by itself |
| **Accent (>)** | peak +20–50 % over target, decaying over 50–150 ms to the sustain level [LI16, clarinet] | "t", release at the higher pressure | A_lip slightly larger at onset | — | strong start, then softer |
| **Sforzando / fp** | sfz: peak +50–80 %, decay 80–200 ms. fp: drop to the piano level within 50–100 ms and hold. | hard "t" | **cover / aim down** at the peak (else sharp); uncover / lips forward when dropping to p (else flat) | — | explosive onset; fp = forte bite then immediate piano |
| **Marcato (^)** | accent + separation (length ~70 %) | heavy "t"; may end with the air | — | — | heavy, separated accent |
| **Double tonguing** | steady support; air must stay "on" between notes | alternating t-k (crisp) or du-gu (legato) [TW][TC]; closure ~20–40 % of IOI | — | — | fast repeated or scale notes, 10–16 per s; k slightly weaker |
| **Triple tonguing** | as above | t-k-t t-k-t or t-t-k t-t-k (du-gu-du…) [TC] | — | — | triplet figures at speed |
| **Breath (h) attack** | p_L rises from 0 over 20–80 ms, no overshoot | none | — | — | soft, "fluffy" start; more breath noise; slower speaking, risks lower regime or edge tone briefly [CA04] |
| **Crescendo** | p_L up moderately (e.g. ×1.0→1.5), **A_lip up** (×1→2) [F75] | — | roll in / aim down progressively to hold pitch | — | louder and brighter: higher partials grow [UF][F75] |
| **Diminuendo / pp** | reverse; jet speed must stay above threshold | — | **lips forward (shorter W), uncover, aim up** to fight flatness; smaller A_lip | — | softer, purer (fundamental dominates [UF]); tends flat; more relative breath noise |
| **Note end: breath release** | p_L taper 30–150 ms to 0 (or below threshold) | open | — | — | natural diminuendo tail; "air ending" |
| **Note end: tongue stop** | p_L stays up, then falls during the closure | "t" closure in 2–5 ms (a "tut") | — | — | abrupt end, then exponential ring-down of the bore (~10–40 ms) [LI16 mechanism] |
| **Phrase breath** | taper 50–150 ms, silence 200–600 ms (quick "catch breath") to 1–2 s, optional inhalation noise | — | lips relax | — | phrase separation; new note starts with t or h |

**Pitch vs dynamics.** Blowing harder raises the jet speed, and the flute goes sharp. Flutists go
sharp when loud and flat when soft, and compensate with jet direction and coverage [TI-sharp][FS].
I found no measured cents figure. Estimate **[E]**: uncompensated, about +20…+40 cents from mf to
ff and −20…−40 cents mf to pp. Section 5.4 proposes an "intonation reflex" to model the
compensation.

## 3. Catalogue: ornaments, colour and extended techniques

| Technique | Air | Tongue / throat | Lips | Fingers | Sound / model notes |
|---|---|---|---|---|---|
| **Vibrato (diaphragm/abdominal)** | p_L × (1 + d·sin 2πf t). f ≈ 5 Hz (4.5–6.5, style-dependent; Meyer 5–8 Hz), d ≈ ±5–15 % [F75] | — | — | — | mostly amplitude and timbre pulsing of the upper partials; pitch FM < 1 % [F75][FT]. Onset often delayed 150–400 ms on long notes **[E]**; rate and depth rise towards climaxes |
| **Vibrato (throat/laryngeal)** | same, generated at the glottis; can be faster and narrower (6–7 Hz) **[E]** | glottal aperture oscillates; swing 7.4 mm loud vs 1.7 mm soft [HT00] | — | — | Fletcher calls throat vibrato "unusual" [F75], but endoscopy shows the larynx active in most vibrato [HT00][BB21]. Model both as p_L modulation; a "throat" variant can add a slight noise modulation **[E]** |
| **Flutter-tongue (alveolar)** | normal or slightly higher p_L, "fast, open air" [FG] | tip trill "rrr" (Spanish r): a_T oscillates ≈ 25–30 Hz between ~0.1 and 1 **[E]** | — | — | buzzy "frrr", clear tone underneath |
| **Flutter (uvular / "growl" r)** | same | uvula trill (French/German r), upstream of the mouth: modulate p_m (not a_T), 25–35 Hz, irregular, depth 20–50 % **[E]** | — | — | rougher and smoother at once; preferred in the low register or when faster [FG][WP-FT]. Sources disagree on whether this is the jazz "growl" [WP-FT] |
| **Growl (voice)** | — | hum or sing a low pitch while playing (see sing-and-play) | — | — | rough beating timbre |
| **Trill** | constant air | none (slurred), or start with t | — | alternate one key (or trill key) at 5–8 cycles/s (10–16 notes/s) **[E]**; press ≈ 10 ms, spring release ≈ 16 ms → asymmetric [AL09] | key-only alternation; trill fingerings may sacrifice intonation or timbre [AL09] |
| **Tremolo (wide interval)** | constant; may need a register "split" | none | — | multi-finger alternation, slower (4–8 notes/s **[E]**), asynchrony glitches audible | |
| **Glissando / pitch bend (lip)** | — | — | roll in or out (c), jaw (W), jet angle (y0) [UF][WA18][FG] | — | continuous pitch change, ~−1 semitone / +¼ tone **[E]**; rolling in also darkens and softens |
| **Glissando (finger)** | constant | — | — | slide off open-hole rings, half-hole: o_k continuous 0…1 [FG] | smooth slide up to about a semitone per hole; chain several holes for longer glissandi |
| **Harmonics (overblown fingering)** | raise p_L as needed for the partial; jet speed is the key variable | none | shorter W and smaller A_lip for higher partials [UF] | hold a low fingering (e.g. C4 or B3) | thin, pure, softer tone; out of tune: 7th partial between A6 and A#6 [UF] |
| **Whistle tones** | very slow but "extremely focused" air; p_L ~5–20 % of normal **[E]** | — | lips further forward than normal (short W), very small A_lip [FG] | any; easiest in high register; can whistle the series above a low fingering | faint, sine-like whistles at high partials, may wander between partials. Model: very thin jet + tiny p; experimental. Edge-tone frequency ∝ U_j/W, so a slow jet needs a short W to reach high regimes |
| **Jet whistle** | very hard blowing *into* the tube | — | mouth covers the whole embouchure hole (c → 1), jet aimed at the far inner wall [FG] | any; a fingering change sweeps the colour | loud rushing whistle/whoosh with rising or falling resonances. Model: no edge jet; turbulent noise injected into the bore at the embouchure, closed upper end |
| **Air sounds / aeolian / residual tone** | normal to low p_L | vowel shapes colour the noise | big A_lip (wide diffuse jet) or y0 far off the edge; below oscillation threshold | fingering selects the resonance colour | breath noise filtered by the bore, faint pitch. Model: σ_n ↑, p below threshold, or jet gain ↓ |
| **Tongue ram** | forceful air, firm seal [FG] | tongue thrust into the embouchure hole: a short volume-velocity pulse (~5–10 ms) **[E]** | lips fully encircle the hole (c = 1) [FG] | any (low register works best) | dull "thunk" at about a major 7th below the written note: the embouchure end becomes closed, so the tube acts as a closed-open pipe [FC] (sources disagree on the name). Model: closed-end reflection at the embouchure + impulse |
| **Pizzicato / tongue slap** | short burst, little or no sustained lung air [FC] | heavy "spit" t, or p (lip), 5–20 ms burst [FG] | normal or near-closed | any fingering | percussive pop with a hint of the fingered pitch, "closer to a key click than a note" [FG]. Model: one tongue/lip pulse with p_L off afterwards |
| **Key clicks (percussive)** | none or minimal [FC][FG] | — | normal; or the hole covered by the mouth → about a 7th lower [FG] | slap keys hard: impulse at hole k on closure, ∝ finger velocity | quiet clicks with a pitch; best in the 1st octave [FC]. Also present at low level during normal fast fingering **[E]** |
| **Multiphonics** | p (jet speed) set between two regimes | — | precise W, c, y0; the vowel helps [FG] | special fingerings with non-harmonic resonances [UF][WA18] | 2+ pitches, quiet, hollow, unstable [FG][WA18]. Model: needs those fingerings in the key table; the jet then chooses or mixes regimes |
| **Sing and play** | p_m gets a voice component at f_v, depth ~10–30 % **[E]** | vocal folds phonate | — | — | two pitches plus sum and difference tones, beating, buzz near unison. Model: add a glottal-pulse oscillator to p_m (player_and_controller.md A3) |
| **Timbre: dark** | — | "oh/oo" vowel; maybe a little more mouth turbulence [NB25] | more coverage (c ↑), aim lower, thicker/wider jet (A_lip ↑, h ↑) | cross or alternate fingerings are darker [UF] | fewer high partials, softer, flatter (compensate with W or y0) |
| **Timbre: bright** | — | "ee" vowel | less coverage, thinner and faster jet, y0 near the edge | — | more upper partials; sharper |
| **Breath noise** | — | — | larger aperture, less focus, wider jet → more turbulence | — | σ_n relative to the tone; higher at pp and in pop/jazz/"breathy" styles |

## 4. Key findings for implementation

1. **Articulation = envelope of p_L × tongue valve × fingering timing.** The tongue decides
   *when*, and the pressure *at release* and its overshoot decide *how hard* [LI16].
   Velocity → (release time, pressure at release, overshoot) is physically justified.
2. **Slur = keys only.** Detours and dropouts in multi-finger slurs are real flute behaviour
   [AL09]. Model them with per-finger timing (keywork.md), not by masking.
3. **Dynamics move pitch.** Without a model of the player's lip compensation, crescendi go sharp.
   Real players fix it with coverage and angle in a few hundred ms. The fix is imperfect, which
   is part of the character.
4. **Note endings are two different sounds.** An air taper gives a soft tail. A tongue stop gives
   an abrupt cut plus a bore ring-down. Both are needed.
5. **Several extended techniques need model features beyond R9**:
   - a closed embouchure end (tongue ram, key clicks with the hole covered, jet whistle);
   - bore noise injection (jet whistle, air sounds);
   - impulse injection at tone holes (key clicks);
   - a voice oscillator (sing and play, growl);
   - the multiphonic fingering table.

## 5. PROPOSAL: real-time mapping to the MPK mini mk3 (< 20 ms)

This is my proposal, not a requirement. Hardware facts are from player_and_controller.md B2. Keys
have velocity but no aftertouch. Pads have velocity and channel or poly pressure. The joystick X
sends pitch bend; Y sends CC(s) and springs back. There are 8 knobs (CC) and a sustain pedal
(CC64).

### 5.1 Zero-look-ahead rules (decided at the MIDI event, latency = audio period only)

| Event | Decision |
|---|---|
| Note-on while another key is held (overlap) | **Slur**: change fingering only, with per-finger timing. If velocity > V_retongue (knob 6), add a soft "du" re-articulation (tongued legato). |
| Note-on with no key held, note sounding (inside the release tail) | **Détaché**: "du" if the gap from note-off was < ~80 ms **[E]**, otherwise "tu". The gap is known at note-on, so this costs no latency. |
| Note-on from silence | **Tongued attack**. Velocity bands **[E]**: < 35 → "h" breath attack (no tongue, 40 ms ramp); 35–90 → "tu" with no overshoot, release 3→1 ms as velocity rises; > 90 → accent "t", overshoot 20–60 % decaying over 100 ms. |
| Fast repeated attacks | **Auto double tonguing**: if the IOI is < ~110 ms (> 9 notes/s), alternate t/k (or du/gu when soft). A pad latch forces t-k or a triple pattern. |
| Note-off, last key | Start the ending chosen by knob 7: air taper 20–150 ms, or tongue stop. Short keypresses (< ~120 ms held) automatically use a tongue/air "staccato" ending **[E]**. |
| Sustain pedal down | **Breath held**: key-up does not stop the air. The last fingering stays, or an open fingering if chosen. Every new key is a slur unless velocity > V_retongue. Pedal up → breath release. |

The rules give these results, with no extra control:

- held overlap → legato;
- small gap → détaché;
- large gap → staccato or separate notes;
- velocity → articulation strength.

### 5.2 Continuous controls

| Control | Proposed function |
|---|---|
| Pad pressure (aftertouch) on a held "breath pad" | Continuous **breath**: dynamics via A_lip (mainly) and p_L, as a breath controller. Lets the player swell held notes (fp, crescendo, diminuendo). Preferred over the joystick for sustained dynamics because it does not spring back. |
| Joystick Y+ / Y− | Y+: momentary **dynamic push / accent swell** (p_L up to +50 %). Y−: **diminuendo / subito piano**. Or vibrato depth, selectable. |
| Joystick X | **Lip pitch bend**: X− = roll in (c ↑, y0 down, flatter, darker), X+ = roll out (c ↓, shorter W, sharper, brighter). Large deflection adds partial venting of the next open hole, for glissandi beyond about ±50 cents. |
| Knob 1 | Base dynamics (A_lip and p_L set-point) |
| Knob 2 | Brightness: c and y0 together (dark ↔ bright) |
| Knob 3 | Breath noise σ_n |
| Knob 4 | Vibrato depth (0 … ±15 % p_L) |
| Knob 5 | Vibrato rate 4–7 Hz (+ onset delay 0–400 ms on the shifted bank or as a fixed setting) |
| Knob 6 | Attack hardness / consonant bias (du ↔ tu ↔ accent) and V_retongue |
| Knob 7 | Note ending: air taper time ↔ tongue stop |
| Knob 8 | Intonation-reflex strength (§5.4), or W offset / mouth volume |

### 5.3 Pads (Bank A: momentary effects, velocity + pressure; Bank B: latches)

| Pad | Bank A | Bank B (latch) |
|---|---|---|
| 1 | Breath pad: pressure = continuous breath | Double-tongue t-k |
| 2 | Flutter (alveolar): pressure = depth | Triple-tongue t-k-t |
| 3 | Flutter (uvular/growl) | du-gu (soft multiple) |
| 4 | Harmonic: overblow the held fingering one partial up per hit (pressure = jet push) | Whistle-tone mode |
| 5 | Trill: hold the pad + key = trill between the sounding note and the key; pressure = rate (5–8 Hz) | Alternate/trill fingering set |
| 6 | Tongue ram (velocity = force) | Multiphonic fingering set |
| 7 | Pizzicato / slap (velocity) | Key-click mode (keys click, little air) |
| 8 | Jet whistle (pressure = blowing force) | Sing-and-play (voice at an interval, Q) |

A **catch breath** gesture could be added: tap the sustain pedal twice. It gives a 250 ms gap
with inhalation noise.

### 5.4 Intonation reflex (model of the player's ear) — proposal

The player listens to the pitch error relative to the target note and corrects it with c and y0.
Model this as a feedback loop:

- a first-order loop with time constant 100–300 ms **[E]** and limited authority (±40 cents);
- it uses the *estimated* playing frequency (bore period from the impedance-minimum table, plus a
  correction from the jet delay), not FFT pitch tracking;
- set the gain to 0 for a "naïve" player.

Effects:

- fast dynamic changes produce realistic small pitch excursions (sharp crescendo, sagging
  diminuendo) that settle;
- slow ones stay in tune;
- the joystick bend adds to the reflex target, so intentional bends are not "corrected away".

### 5.5 Optional look-ahead "expressive mode" (30–60 ms delay)

With 30–60 ms of look-ahead the engine knows the next event before it must commit. It can:

1. **Slur or tongue by context**: if the next onset is within the look-ahead and the current key
   is still held or just released, choose a slur, or a "du" when detached. This fixes ragged
   overlaps from keyboard playing.
2. **Prepare register jumps**: start the p_L and W change for an upward octave 10–30 ms before the
   key change, as players do. This avoids cracked slurs while keeping the real physics.
3. **Choose finger order**: pick a "safe" multi-finger order [AL09] for slurs. Or deliberately
   allow unsafe detours in a "sloppy" setting.
4. **Choose the articulation pattern**:
   - detect IOI groupings (pairs → t-k, threes → t-k-t);
   - shorten the last note before a rest (phrase ending);
   - add accent to metrically strong or long notes (small, ≤ 10 %).
5. **Shape endings**: taper the last note before a long gap (> 300 ms) with an air release. Use a
   tongue stop before short gaps.
6. **Decide where to breathe**: insert a catch breath at gaps > 250 ms after a long phrase (breath
   budget model), with inhalation noise.
7. **Shape vibrato**: delay the onset on long notes (known from the gap to the next event); no
   vibrato on short notes.

## 6. Sources

Book references (Wye, Moyse *De la sonorité*, Kujala *The Flutist's Vade Mecum*, Dick *The Other
Flute*) were not consulted directly in this session. No claim above is attributed to them.

- [F75] N. H. Fletcher, "Acoustical correlates of flute performance technique," JASA 57(1) (1975).
  ANU repository: https://openresearch-repository.anu.edu.au/handle/1885/125582 (also summarised in
  player_and_controller.md).
- [AL09] A. Almeida, R. Chow, J. Smith, J. Wolfe, "The kinetics and acoustics of fingering and note
  transitions on the flute," JASA 126(3) (2009). https://arxiv.org/abs/0903.4012
- [LI16] W. Li, A. Almeida, L. Inwood, J. Smith, J. Wolfe, "Science of articulation" (clarinet),
  Clarinet & Saxophone (2018), summarising Li et al. JASA 2016:
  https://newt.phys.unsw.edu.au/jw/reprints/Articulation.pdf ; https://newt.phys.unsw.edu.au/jw/articulations.html
- [CA04] M. Castellengo, transient pressure rise time, ISMA 2004:
  https://www.lam.jussieu.fr/Membres/Castellengo/publications/2004c-Transient_Pressure%20rise%20time-ISMA.pdf
- [EF17] A. Ernoult, B. Fabre, "Temporal characterization of experimental recorder attack
  transients," JASA (2017): https://hal.sorbonne-universite.fr/hal-01468377v1
- [DC08] P. de la Cuadra, B. Fabre, N. Montgermont, "Analysis of flute control parameters: a
  comparison between a novice and an experienced flautist" (2008):
  https://repositorio-az.uc.cl/dspace/items/c534ff0c-0225-42d7-9220-fc246dfb4af1
- [AA21] Acta Acustica 5 (2021), jet offset and angle vs harmonics:
  https://acta-acustica.edpsciences.org/10.1051/aacus/2021003
- [UF] J. Wolfe, "Flute acoustics: an introduction": https://newt.phys.unsw.edu.au/jw/fluteacoustics.html
- [UF-VF] UNSW Flute Acoustics / Virtual Flute: https://newt.phys.unsw.edu.au/music/flute/
- [WA18] J. Wolfe, "The acoustics of woodwind musical instruments," Acoustics Today (2018):
  https://www.phys.unsw.edu.au/jw/reprints/Wolfe_AT2018.pdf
- [WS03], [HT00], [BB21], [NB25]: see research/player_and_controller.md.
- [FG] L. Riner, "A Field Guide to Extended Techniques for Beginners" (2017):
  https://rinerflute.blogspot.com/2017/11/a-field-guide-to-extended-techniques.html
- [FC] Flute Center of NY, "All about extended technique":
  https://flutecenter.com/blogs/all-about-the-flute-center/all-about-extended-technique-on-flute ;
  E. Ferguson, https://www.emiferguson.com/flutes-extendedtechniques
- [FT] flutetunes.com, "Flute vibrato": https://www.flutetunes.com/articles/vibrato/
- [FS] The Flute Show (F. Nerrière), loud/soft:
  https://www.virtualsheetmusic.com/experts/fluteshow/loud-soft-flute
- [TI-sharp] The Instrumentalist, flute intonation / sharp when loud: https://theinstrumentalist.com/?p=5926 ;
  vibrato exercises: https://theinstrumentalist.com/october-2009-flute-talk/10-exercises-for-vibrato-control/
- [TI-clar] The Instrumentalist, "Clarity and Articulation": https://theinstrumentalist.com/?p=4169
- [TW] Wikipedia, "Tonguing": https://en.wikipedia.org/wiki/Tonguing
- [TC] flutecentric, "Notes: On Articulation": https://flutecentric.com/notes-flute-topics-playing/flute-articulation/
- [UI] Univ. of Idaho woodwind fundamentals handout ("Tah not Tut"):
  https://www.webpages.uidaho.edu/~leonardg/fundamentals.pdf
- [WP-FT] Wikipedia, "Flutter-tonguing" (mirror): https://kiwix.lema.org/wikipedia_en_all_mini_2024-02/A/Flutter-tonguing

## 7. Gaps

- I found no flute study of tongue timing together with mouth pressure. The envelopes above carry
  over clarinet [LI16] and recorder [EF17] data.
- No flute measurement of the flutter-tongue rate; 25–30 Hz is from phonetics.
- No measured cents figure for pitch vs dynamics, or for the range of lip bending.
- Whistle-tone and multiphonic physics in a 1-D jet-drive model is uncertain. Treat these as
  experiments.
