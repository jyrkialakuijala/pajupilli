# Review: player model, articulation, expressivity, live controller

Reviewer scope: `src/player.*`, the mouth/glottis/tongue/jet head of `Flute::sample`,
`src/live.cc`, research/playing_techniques.md §5. Date 2026-11-11, repo at `701a84a`.

How I measured: I wrote offline drivers against the `Player`/`Flute` API in a scratch copy
(`/tmp/claude-1000/-home-jyrki-pajupilli/b0250b6e-5038-4aaa-b429-7d4c393762a5/scratchpad/review_player/rv/`:
`drive.cc` for the scripted performances, `ana.py`, `onset.py`, `slur.py`, `notes.py`, `dyn.py` for the
analysis, and `experiment.patch` for the knobs I tried). All runs are calibrated (`calibrate(440)`),
dyn 0.5, no vibrato unless stated. Pitch is frame-wise YIN (12–25 ms windows). "Wrong
register" means more than 200 cents from the target while the level is within 20 dB of the
steady level. The live mode was **not** started. The MPK was only listed (`aconnect -l` →
client 32 "MPK mini 3", port 0).

## Summary

- **The model is fast and real-time clean enough.** It takes 11–19 µs per 64-frame period
  (≈1 % of the 1.33 ms budget), including periods where keys are moving.
- **Slurs across registers are the weakest part.** An upward octave slur takes 100–240 ms to
  reach the new note. For that time the tone stays on the old register, or on a sharp version
  of it. D4→D5 never gets there: it locks at 438 Hz. A real flutist does this in tens of ms. A
  simple "register kick" fixes most of it (median ≈170 → ≈50 ms). See F1.
- **The second octave F#5–C6 is fragile and 18–22 dB quieter than A4.** Its operating point is
  on the edge of the lower regime: +10 % W or −20 % pressure drops A5 an octave. So tongued
  repeated A5 at 6 notes/s gives the wrong octave in 10 of 12 notes. G6 never sounds; it plays
  1205 Hz at every dynamic. The calibration accepts the first pressure that gives the right
  pitch, so it finds marginal points. See F2.
- **Expressive range is narrow.**
  - pp→ff changes the level by only 3 dB at C4, 14 dB at A4 and 2 dB at C6.
  - Pitch drifts −24…+12 cents over that range, with no intonation reflex.
  - C6 falls to C5 at dyn ≤ 0.4.
  - Full joystick roll bends only −9/+6 cents at C4 and −28/+21 at A4.
  - Accents are inaudible: the peak is 1.07–1.08× steady on A4, because the 40 ms overshoot is
    over before the tone has built up (60–70 ms).
- **Articulation rules work, but the syllables are barely different.** "tu" and "du" release
  over the same ≈8 ms, against 1–3 ms vs 4–8 ms in the research. Tongue settings hardly change
  the onset at all (F5). The MPK's velocity range (mostly < 64, max ≈ 100) makes accents
  (> 90) and pedal re-tonguing (≥ 100) nearly unreachable.
- **Live code problems:**
  - no MIDI hotplug after the first connection;
  - port rescans and verbose `fprintf` inside the audio loop;
  - a ≈1.5 s calibration on every start;
  - knob defaults that do not sit at a common knob position, so the first touch jumps;
  - an unramped volume step;
  - a "breath pad" that cannot bring the breath to zero.

## Findings, ranked by impact

### F1. Register-change slurs are slow or fail (impact: high; effort S; risk low)

Evidence (`slur.py`; the key changes at t = 0; settle = within ±30 cents and held for 40 ms):

| slur | settle (ms) | notes |
|---|---|---|
| C4→C5 (same harmonic) | 84 | pitch arrives in 25 ms, then the level drops −28 dB for ≈60 ms (dropout) |
| E4→E5 / F4→F5 | 102 / 56 | |
| G4→G5 / A4→A5 / B4→B5 | 172 / 184 / 174 | stays on a sharp lower register (A4 fingering at 478 Hz) for 180 ms, then flips |
| D4→D5 | never | locks at 438 Hz, a −506 cent non-note |
| D5→D6 | 238 | |
| G5→G6 | never | (G6 is broken anyway, F2) |
| C5→C4 | 20 | downward slurs are mostly fine |
| A5→A4 / C6→C4 | 124 / 118 | down through a −13 to −20 dB dip |

In `applyNote(kSlur)` only the fingering and the targets W, p (∝ f) change. The jet's
hysteresis keeps the old regime. Players overcome this with a short over-push and lips
forward (playing_techniques.md §2: "raise p_L 10–30 % within 10–30 ms, shorten W").

Experiment (`experiment.patch`, knobs `RV_KICKP/RV_KICKW/RV_KICKT`): when a slur raises the
harmonic number, multiply p by 1 + 0.3·e^(−t/40 ms) and W by 1 − 0.2·e^(−t/40 ms).

- Settle times: G4→G5 172→52, A4→A5 184→52, B4→B5 174→38, C5→C6 116→34, D5→D6 238→20,
  D4→D5 never→102, G5→G6 never→60 ms.
- One small regression: F4→F5 56→78 ms.
- A downward kick did not help. Gliding the embouchure (τ 8–15 ms) made things slightly worse.
- C4→C5 (both harmonic 1) is not covered by the kick. Its dropout is in the key-travel and jet
  interplay; the acoustics/keywork reviewers should look at it.

Fix: add the kick for a harmonic increase as a knob, and also scale it by the interval for
same-harmonic leaps. Later, let the look-ahead mode start it 10–20 ms before the keys (F9).

### F2. A fragile second octave and a broken G6; calibration chooses marginal points (high; M; low)

Evidence (`./flute -scale`, steady rms):

- F#5–C6 are at 0.0066–0.011, against A4 0.078 and C#5 0.086. That is −18 to −22 dB. A real
  flute's 2nd octave is as loud as the 1st or louder.
- G5 from silence flips between G5 and G4 every 40–80 ms for the whole note (onset table: "wrong"
  100–150 ms, steady level 0.005).
- A5's margin:

  | change | result |
  |---|---|
  | PJ_W = 1.1 | plays A4 (467 Hz) |
  | PJ_P = 0.8 | plays A4 |
  | PJ_P = 2 | correct, 5× louder, +54 cents |

- Repeated tongued A5: correct in 2 of 12 notes at 6 notes/s and 7 of 12 at 14 notes/s.
- Trills G5–A5: correct in 9 of 17 notes at 12 notes/s and 5 of 17 at 16 notes/s.
- Fast tongued scales: 4–5 of 30 notes in the wrong octave, all in the 2nd octave.
- G6 (91) plays 1205 Hz (−455 cents) at every dynamic from silence. It does play correctly when
  reached by a kicked slur, so the regime exists. The calibration reports "+18 cents" because
  its 0.35 s test and its "clarity > 0.6, |c| < 150" check happened to pass. The player then
  uses that point without checking it.
- `calibrate.cc` tries pressures in the order 1.0, 1.2, … and stops at the first that gives the
  right pitch within 3 cents. It never checks how far it is from the regime boundary, the level,
  or other dynamics and attack types.

Fix: make calibration choose robust points.

- For each note, find the pressure (or W) interval in which the register holds. Approach it
  from both sides, because of hysteresis.
- Take the middle of that interval, then tune with coverage.
- Check that the note holds at dyn 0.2, 0.5 and 0.9 and with v = 20 / 64 / 110 attacks.
- Optionally target a level curve (2nd octave ≈ 0 to +3 dB over the 1st).

That takes about 5–10× more search, so cache the result (F11). Expected effect: no octave drops
in tongued 2nd-octave passages, and even dynamics. Part of the root cause may be in the jet
model, and the jet reviewer should see these numbers. One example: with no calibration,
**lower** pressure jumps **up** an octave (C4 at PJ_P 0.6 plays C5; A4 at dyn 0 plays A5). That
is the opposite of the usual τ/T regime selection.

### F3. Dynamics: small level range, pitch drift, no intonation reflex, register loss at pp (high; M; med)

Evidence (`dyn.py`; note held, dyn stepped 0→1):

| note | cents at dyn 0 → 1 | rms at dyn 0 → 1 | range |
|---|---|---|---|
| C4 | −18 → +10 | 0.025 → 0.035 | 3 dB |
| A4 | −24 → +12 | 0.017 → 0.089 | 14 dB |
| E5 | | | 7 dB; at dyn 0 it drops an octave |
| C6 | | | in C5 at dyn ≤ 0.4, then 2 dB |

The mapping is `p ∝ 0.7 + 0.6·dyn`, `lipH 0.5–1 mm`, `lipWidth 7–11 mm`, with W and y0
independent of dynamics.

- The research (§2, §1.3) says that at pp a player brings the lips forward (shorter W) and aims
  up, and at ff covers more. Without that, pp loses the register on high notes and the pitch
  sags.
- The ±12–24 cent drift is the "naïve player" behaviour described in §5.4.

Fixes:

1. Make W depend on dyn (W × (1 − 0.15·(0.5 − dyn))) and y0 too. That keeps high notes in
   register at pp. Effort S.
2. Add an intonation reflex as a **learned static map plus lag**. Calibrate the cover trim at
   dyn {0.1, 0.5, 0.9} per note, interpolate, and let the applied cover follow with τ ≈ 150 ms.
   Crescendi then go briefly sharp and settle, as §5.4 describes, at no runtime cost. Knob 8
   (or a PJ_ knob) sets the reflex strength. Effort M.
3. The 3 dB range at C4 is too small: real flutes span ≈ 10–20 dB at low notes. It probably
   needs a larger lip-area span, or a jet-saturation change in the acoustics. Flag it to the jet
   reviewer.

### F4. Accents and attack types barely differ (medium; S; low)

Evidence (onset table, attack from silence; "first" = −40 dB re steady, then −20 / −6 dB, and
pitch held within ±30 cents):

| note | breath v20 (−20 / −6 / pitch) | tu v64 | accent v110 | accent peak / steady |
|---|---|---|---|---|
| C4 | 12 / 82 / 82 | 11 / 78 / 72 | 9 / 73 / 66 | 1.16 |
| A4 | 57 / 76 / 58 | 52 / 70 / 54 | 45 / 59 / 44 | 1.08 |
| C5 | 44 / 55 / 46 | 36 / 47 / 44 | 31 / 40 / 30 | 1.19 |

- The first audible noise comes at 3–9 ms, which is good for R16a.
- The tone build-up dominates: 40–80 ms to −6 dB. Changing the tongue release (1.5 ms) or the
  closure (4–10 ms) changes this by ≤ 6 ms. That agrees with LI16: the tongue does not change
  the growth rate.
- The accent overshoot (+22 %, τ 40 ms) is over before the tone is up, so it adds ≤ 0.7 dB.
  The research gives +20–50 % over 50–150 ms.
- A bigger accent (+50 %, τ 100 ms) overblows C4 to C5. This is the low-note headroom problem
  of F2.
- tu and du have the same tongue release (≈ 8 ms at v64). The table in §1.2 says t 1–3 ms and
  du 4–8 ms.
- The "h" attack differs from tu by only ≈ 6 ms.

Fixes:

- Accent: let the overshoot follow the tone. Use τ ≈ 80–120 ms, and clamp it per note to the
  calibrated register headroom from F2.
- Separate the syllables: t release 1.5–3 ms with closure 6–10 ms (the mouth fully charged); du
  release 6 ms with closure 2 ms and a 10 % residual leak.
- Lengthen the h ramp to 60–80 ms with more glottal noise.

The audible differences will be mostly in the noise transient and the speed of the upper
partials, as in LI16.

### F5. Velocity rules do not fit the MPK mini mk3's velocities (medium; S; low)

- research/player_and_controller.md B2: the keys usually send < 64 and at most ≈ 100.
- Accent needs v > 90 at the default hardness, and pedal re-tonguing needs v ≥ 100. Both are
  practically unreachable.
- The breath attack (v < 35) catches many normal soft presses.
- With the pedal down, a hard hit gives a *soft* "du". The `blowing_` branch maps it to kDu,
  which is the opposite of what the README promises.

Fix: add a velocity curve in `Controls`, e.g. v' = 127·(v/100)^0.7, clamped. Make the
thresholds relative to it, and hit hard → "tu"/accent under the pedal.

### F6. Live: MIDI hotplug, work in the audio loop, startup (medium; S–M; low)

- **Hotplug:**
  - `connectSources` runs only while `!midi.connected`. After an unplug and replug the
    subscription is gone and the flag stays true, so the device is never reconnected.
  - Fix: subscribe to `SND_SEQ_CLIENT_SYSTEM:SND_SEQ_PORT_SYSTEM_ANNOUNCE`, and on
    `SND_SEQ_EVENT_PORT_START` connect that port if its name matches. This removes the rescan
    poll too.
- **Audio-loop work:**
  - The rescan does seq ioctls in the audio thread once a second while disconnected.
  - With `-v`, `fprintf` runs for every CC, pad and xrun.
  - Move printing to a lock-free ring drained by a low-priority thread, or at least keep it off
    by default during play.
- `mlockall`/sched failures are silent unless `-v`; report them once.
- **Startup:** calibration of all 37 notes takes 1.5 s, measured with `./flute -cal`. F2 makes it
  5–10× longer. Cache it in `~/.cache/pajupilli/cal-<hash of PJ_* env and source version>`.
- **CPU:** 11.5 µs mean per period steady, 17–19 µs while keys move (`drive cpu`). The worst
  cases of 0.3–2 ms appeared only in some runs on this non-RT desktop. They are most likely
  scheduling noise, but worth checking under SCHED_FIFO with `cyclictest`-style logging of the
  render time.

### F7. Knob defaults, ranges and zipper (medium; S; low)

The MPK's encoders keep their own value, so the first turn sends wherever the device was.
The code's defaults correspond to different knob positions:

| knob | default | its position |
|---|---|---|
| K1 dyn | 0.5 | 64 |
| K2 brightness | 1 | ≈40 |
| K3 noise | 1 | ≈42 |
| K4 vibrato | 0.05 (live) | ≈42 |
| K5 rate | 5 Hz | ≈42 |
| K6 hardness | 1 | ≈42 |
| K7 release | 30 ms | ≈10 |
| K8 volume | 1 | ≈90 |

So the first touch of any knob jumps.

Fixes:

- Remap so that every default sits at 64, using exponential maps around the default.
- Optionally use soft takeover, or SysEx-load a "pajupilli" program into the MPK (B2 gives the
  message format).
- Volume (`gain_ = 2x²`) is applied per period with no ramp: steps of up to 0.3 dB at
  ≈ 750 Hz. Ramp it inside the period.
- Roll (joystick X) is too small: ±1 gives −9/+6 cents at C4, −28/+21 at A4 and −28/+35 at C6.
  The research says −50…−100 / +20…+40. Increase the `exp(0.3·roll)` span asymmetrically, and
  add y0 (aim) and W, which also move the timbre.

### F8. The breath pad is not a breath controller (medium; M; low)

- Pad pressure maps to `breathCtl = −0.5 + v/127`, which is added to dyn. At zero pressure dyn
  only clamps to 0, so the flute still blows at pp. Notes still start from keys.
- Swells from nothing, fp and phrasing need breath that can go to zero.

Proposal: "breath mode" (latched with pad 1 of bank B):

- the lung target = f(pad pressure), 0 → silence;
- keys only finger (always slur);
- a pressure rise from silence chooses h, du or tu from its slope.

This is how a wind controller works. Also check whether the unit sends channel or poly
aftertouch. Its factory default is unknown (B2), and `padPressure(-1, …)` guesses.

### F9. Expressive look-ahead mode (R16b): proposed design (high value; M–L)

Architecture:

- Put a fixed delay D (knob, 20–60 ms, default 40) between MIDI input and the `Player`.
- Events go into a timestamped queue. The `Player` consumes them D later. It also has a
  `peek(window)` that sees everything in the next D.
- The audio is not delayed: only control events are. So the cost is exactly D of extra latency.
- Everything below is a decision made at the delayed time from the visible future, so it stays
  deterministic and can be tested offline with the same drivers.

Decisions, in order of value:

1. **Register preparation.** If the next note is within D and its harmonic is higher, or it is
   ≥ 7 semitones up, start the F1 kick (p +20–30 %, W −15–20 %) 10–20 ms *before* the keys
   move. This is the biggest audible win: slurs then settle in ≈ 30 ms instead of 50–180.
2. **Slur / détaché repair.** Keyboard players leave 5–30 ms gaps between legato notes. If the
   next note-on follows a note-off within ≈ 40 ms, treat it as a slur, or as "du" if the gap is
   ≥ 25 ms or the velocity is high. Accidental tonguing of slurs goes away.
3. **Endings.** If no note follows within D after a note-off, and the note was held < 150 ms,
   use a tongue stop (a fast closure that keeps the pressure). Otherwise use an air taper whose
   length grows with the time to the next note (unknown beyond D, so use a running estimate of
   the IOI).
4. **Auto multiple tonguing.** If the IOI of repeated or tongued notes is < 110 ms, alternate
   t/k (k: closure 20 % longer, release 1.5× slower, 10 % less pressure at release). This works
   with zero look-ahead too.
5. **Vibrato shaping.**
   - Start the vibrato fade on notes expected to be long, i.e. no next note within D.
   - Do not reset the vibrato on tongued notes inside a phrase. Today `sinceOn_` resets at every
     tongued note, so tongued melodies never vibrate, which is wrong even without look-ahead.
     Effort S.
6. **Finger order.** For multi-finger slurs, choose per-hole delays from a safe-order table
   (AL09; this needs per-hole timing in `setHoles`). An "amateur" knob adds 10–35 ms asynchrony
   for realism.
7. **Breath budget.** Track the air used. At a gap > 250 ms after > 6 s of playing, insert a
   catch breath with inhalation noise.

Testing: replay the same MIDI files with D = 0 and D = 40. Measure slur settle time,
wrong-register ms and onset counts with `slur.py`/`notes.py`.

### F10. Smaller player issues (low–medium; S)

- **Note stack:** duplicates are kept, and keys that fold to one note (octave ≥ +2: C7 and C8
  → 96) are both removed by the first key-up; a full stack (16) drops entries silently. Count
  per note in `held_` and remove one entry at a time.
- **Release:** only an air taper (τ = `release_`, tongue open). Add the tongue-stop ending
  (research §4.4) and a release noise burst.
- **Flutter:** a pure 27 Hz sinusoid; add ±15 % rate jitter.
- **Uvular flutter, trills and harmonics pads** from §5.3 are unimplemented.
  - Harmonics are cheap: hold pad 4 so the player uses harmonic n + 1 of the current
    fingering, p × ((n + 1)/n)^≈1 and W × n/(n + 1). The F1 kick helps it speak.
  - Trill pad: alternate between the sounding note and a key at a pressure-controlled rate.
    Today slurred trills work on 6 of 9 tested pairs at 12 notes/s. They fail on C5–D5 (only
    9/17 correct: D5 lands −280 to −620 cents) and on G5–A5 (F2).
- **Vibrato:** depth 0.05 (the live default) gives 9 cents FM peak-to-peak and 1 dB AM on A4;
  0.15 gives 29 cents and 4.6 dB. FM below 1 % (17 cents) matches Fletcher at ≤ 0.1. That is
  OK, but the AM is small. Consider a throat share or a coupled y0 wobble for more timbre
  pulsing. This is speculation; decide by ear.
- **Key clicks** (missing): a small impulse ∝ closing speed into the hole's q in
  `Flute::updateHoles`. Whistle tones need a jet-model study first (research §7).

### F11. Calibration as a startup cost and a fidelity limit (low–medium; S)

Calibration uses only a v64 attack at dyn 0.5, and its pressure trims jump between neighbours
(E5 ×1.2, F5 ×1.45, F#5 ×1.0; C4 ×1.70 vs C#4 ×1.45), so adjacent notes differ audibly in
loudness and brightness. Smooth the trims across notes (penalise steps) and cache them (F6).

## Proposed order of work

1. Register kick for slurs and 2nd/3rd-register attacks (F1). [S]
2. Velocity curve and threshold fixes (F5); keep vibrato continuous across tongued notes. [S]
3. Robust calibration with a cache (F2, F11). [M]
4. W/y0 vs dynamics, plus the learned intonation reflex (F3). [M]
5. Accent and syllable timing (F4). [S]
6. Live hotplug, knob centring, volume ramp, no I/O in the loop (F6, F7). [S]
7. Breath-controller mode (F8). [M]
8. Look-ahead mode (F9), starting with register preparation and slur repair. [M–L]

## Open questions

- Is G6's failure, and the "lower pressure → higher octave" behaviour, in the jet model? Flag it
  to the acoustics/jet reviewers.
- The C4→C5 dropout (60 ms at −28 dB): is it key timing (16 ms spring opening of many holes)
  or jet adaptation? Real flutes show dropouts of "tens of ms" (AL09). This is in that range
  but long.
- The MPK unit's real settings were not checked:
  - joystick Y (CC1 only, or dual CC);
  - pad aftertouch mode;
  - pedal polarity;
  - the key velocity spread.
- They can be checked with `aseqdump -p "MPK mini 3"` while Jyrki plays. I did not send SysEx
  to the device.
- The build-up time (40–80 ms to −6 dB) is set by the jet loop gain, not by the player. Is it
  realistic for this flute? Measured flute attacks (AL09, CA04) suggest 20–50 ms for tongued
  middle-register notes. That is speculation without recordings; Jyrki's ear decides (R28).
