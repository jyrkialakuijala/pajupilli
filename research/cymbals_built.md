# The cymbals as built

Status: 2026-11-16 (second round: §4). What `src/cymbal.h/.cc` is, against `cymbal_plan.md` steps 2, 3 and 5. Step 4
(the nonlinearity) is not built. Numbers are measured with `-inst ... -seq` renders (scripts in the
session's scratchpad). `tools/test.py cymbals()` checks the main ones.

## 1. What is built

- **Six instruments, one object each, living through the song:**

| Instrument | Cymbal | Keys |
|---|---|---|
| `ride-cymbal` | 22" thin jazz ride, ~1600 modes to 16 kHz | 51 bow, 53 bell |
| `ride-cymbal-2` | 20" medium, drier ride | 59 |
| `crash-cymbal` | 16" thin crash | 49 |
| `crash-cymbal-2` | 18" thin crash | 57 |
| `splash-cymbal` | 10" splash | 55 |
| `china-cymbal` | 18" china | 52 |

  - A brush stroke is the same key 24 higher (73–83). `PJ_CYM_BRUSH=1` brushes everything.
  - Other keys (live): the tip, from the edge (low keys) to the bell (high).
- **The plate is the other session's `Plate`:** free round plate modes, the dome as one sphere's
  stiffness, hammered pairs split. Per cymbal: radius, mean thickness (jazz_drums.md §3.1), dome,
  split, losses.
- **Additions in `cymbal.cc`:**
  - **The body on the stand:** height on the felt, two tilts on the tilter, ~1.5 Hz. It is the give
    behind the stick, not a sound.
  - **The bell:** six modes of its own (a ride's from 820 Hz, T60 4.5–2 s), driven by what strikes
    inside its rim, heard as a small piston.
  - **The stick:** copied from `hihat.cc`. A tip of 12 g or a shoulder of 30 g on a Hertz contact,
    solved against the plate's own give each sample. Its three bending modes (324, 892, 1749 Hz)
    ring for ~40 ms.
  - **The microphones:** copied from `hihat.cc`. A Rayleigh sum over the plate, with 2 m_max + 16
    angles so that no diameter count aliases, times min(1, √(f / coincidence)).
  - **The choke:** a loss on the modes by their motion at 0.95 R, at least 0.3 of the middle
    mode's share (a hand covers more than a ring). The cymbal falls 45 dB in 100 ms and 69 dB in
    150 ms.
- **The player:**
  - **Ride bow:** the place drifts by phrases (stroke_placement.md §7.2: held 8–16 strokes, then
    moved, two times in three to the other side). Between moves it wanders, so no two strokes
    land on the same spot: a few mm to a cm in r, a few degrees round.
  - **Ride brush strokes:** they go continuously from the slowed tip to the thrown fan by a "lean",
    0–1. The lean is the velocity and, in a song (SongClock: 4/4 from beat 0), the beat: 1 and 3
    leaned on, 2 and 4 light, offbeats between.
    - Share of wire ends 0.4–1.
    - Arrival speed 0.6–1× the hand's.
    - Follow-through 0–1 mm.
    - In a song, 1 and 3 are also up to 20 % faster and 2 and 4 slower.
    - Live, the velocity alone.
  - **Crash, splash, china:** the shoulder at 0.9 R. A soft stroke (velocity < 48) is the tip at
    0.9 R.
  - **The choke:** the breath (channel pressure) is the hand on the edge. With `PJ_CYM_NOTEOFF=1`
    a note-off grabs and holds until the next stroke.

## 2. Measured

| Quantity | Built | Target |
|---|---|---|
| Ride T60, 500/1k/2k/4k/8k/12.5k Hz | 5.6 / 4.5 / 2.5 / 2.3 / 1.7 / 1.3 s | FluidR3's ride: 3.8 / 3.9 / 3.9 / 2.6 / 1.65 / 1.06 s |
| Ride at 250 Hz | almost no energy (below its ~440 Hz dome pitch only the weakly radiating (m,0) modes remain) | 2.9 s |
| Crash 16"/18" T60, 1k/4k/12.5k | 3.1 / 1.6–1.7 / 0.9 s | ~3 s / 1.5–2.5 s / < 1 s (jd §3.3) |
| Splash T60, 1k/12.5k | 1.7 / 0.6 s | 1–2 s |
| Ride time 200 bpm: the wash before a stroke, bar 5 over bar 1 | +3.9 dB (1 kHz), +7.0 (2 kHz), +1.7 (4 kHz) | builds |
| Wash at the end over one stroke's tail of the same age (1 kHz) | +25 dB | — |
| Ping over wash, 200 bpm | +4–5 dB (1–8 kHz) | Francis ride 12–19 dB |
| Ping over wash, Bönner's tempo (quarters 0.55 s, skip notes) | +5.6 / 6.3 / 6.7 / 11.1 dB at 1/2/4/8 kHz | 12–19 dB |
| Ride stroke centroid, 0–0.1 s / 0.3–1 s | 13.3 / 6.2 kHz | FluidR3: 9.8 → 3.5 kHz |
| Bell centroid 0.3–1 s; level against the bow at the same velocity | 2.2 kHz; +6.7 dB | pitched, cutting |
| Velocity 16 → 127 (LAFmax, A): ride stick / crash / splash / brushed ride | 30.1 / 30.2 / 32.3 / 33 dB | GM ~30 |
| Brushed ride against a stick stroke, velocity 80 | −12 to −15 dB | quiet |
| Crash centroid, first 0.1 s (v120) | 8.5 kHz (16"), 9.1 kHz (18") | brightest 3–8 kHz |
| Choke | −45 dB at 100 ms, −69 dB at 150 ms | 50–100 ms to silence |
| Level at velocity 90 (BS.1770, with the trims) | −28.2 LUFS each | the common level |
| Cost, random playing (`tools/test.py`) | ride 1150 ns/sample (5.5 % of a core), brushed 1490 (7.1 %), 18" crash 1220 (5.8 %) | < 10 % |
| Stability | fuzz 10 s on all six (and brushed), no resets | — |

## 3. What is missing or wrong

- **A simple bloom only.** Step 4's nonlinear core is not built. Since 2026-11-27 the crashes,
  splash and china have step 4c's cascade in a simple form (`PlateSpec::bloomRate`, the user's
  "sharp burst"): over a threshold (each plate's core energy at about velocity 70) the modes
  under 2.5 kHz are drained down to the geometric mean of the threshold and the stroke's own
  energy, 8 % of it kicked into 2.5–12 kHz (falling as 1/f) and those modes damped while it
  runs. At velocity 110 on the 16" crash: +5.6 dB in 3–8 kHz over the first 0.1 s, the tail
  2.7 dB lower at its own slope (8.6 s). The 3–8 kHz band flares at once, not 0.1–0.3 s later,
  and a hard ride stroke still does not wash up (the rides stay linear).
- **Too little ping over the wash** (+5–7 dB against the recording's 12–19).
  - With the measured band decays (~4 s at 1–2 kHz), a linear plate struck 2–5 times a second
    holds a wash within a few dB of each stroke's energy, by energy balance alone.
  - A stronger stick ring (`PJ_CYM_WOOD`) did not help.
  - What a real ride has that this lacks: an early, faster decay (a two-slope decay), or a ping
    carried by something that dies within tens of ms. The nonlinearity, or the true profile's
    mode shapes, may give it; to be measured on the stems' rides (cymbal_plan step 0).
- **Brighter than the recorded ride** by about half an octave in centroid.
- **The 2 kHz band decays too fast** (2.5 against 3.9 s); 500 Hz too slowly (5.6 against 3.8 s).
- **The dome pile-up (a Plate request).**
  - Every mode with a nodal circle whose flat pitch is below the dome's frequency is lifted to
    just above it. Dozens pile up there.
  - On an edge stroke their round members, monopoles, drown the rest: the crash's 400–800 Hz
    octave held all but 0.7 dB of the energy, the splash's 800–1600 Hz all but 0.1 dB.
  - Until Plate has the true profile (step 1), the crashes, splash and china take a dome a quarter
    of their real height (`PJ_CYM_DOME`, 0.25) and twice the felt.
- **The china** is a crash plate with wide-split pairs (4 %), a flatter dome and more loss up high.
  No flange.
- **No rivets** (no sizzle ride), no stick-shoulder accent sound beyond the heavier mass, no
  rocking heard.
- **The brushed ride's body** (600–1200 Hz) against its highs does not move with the lean on this
  plate (−42 to −52 dB, no trend). On the hi-hat the other session measured 11 dB between the
  slowed tip and the thrown fan.
- **Not checked end to end:** the beat emphasis (SongClock) needs `-play` with the cymbal keys
  routed in `drumFor`.

## 4. Second round (2026-11-16)

- **The brush on bronze:** 48 groups of wire ends, 2.5e-5 kg each, contact 1.2e4 N/m, `together`
  0.1 (each end bounces on its own), `tipLoss` 0.15.
  - The brush feels only the modes below 7 kHz (`BrushSpec::modes`, `PJ_CYM_BRUSHTOP`).
  - The brushed level: the brush's pushes reach the plate +20 dB (`PJ_CYM_BRUSHDB`) through an
    adapter surface. The level is in the stroke, so nothing already ringing jumps.
  - A brushed ride at velocity 80 is 11.5 dB(A) under a stick stroke; the brushed range over
    velocity is 28 dB.
- **The ride's brush strokes, landings** (summed force; bumps standing 15 % over the dip):

| Stroke | Before (24 tied ends, 6e4 N/m) | Now |
|---|---|---|
| Slowed tip, v40 | 3 landings over 2.3 ms, peak 1.9 N | 5 over 2.5 ms, 0.6 N |
| v80 | 2 over 3.1 ms, 5.2 N | 7 over 3.4 ms, 2.1 N |
| Thrown fan, v120 | 2 over 3.3 ms, 13 N | 3 over 3.8 ms, 7.2 N |
| Crash sweep, v90 | 5 over 39 ms, 15 N | 2 over 40 ms, 23 N |

  - The fan stroke's follow-through is 1–1.5 mm, not the 3–5 mm asked. At 3–5 mm the hand keeps the
    fan pressed and the landings run together: one bump over 6–8 ms, peak 5 N (v80) and 17 N
    (v120).
  - The hi-hat's 6–8 landings over 11–26 ms are not reached on the ride. The hand stops one
    follow-through after the first touch, so a wider spread only means fewer ends land.
  - The crash's sweep keeps 3–5 mm (a sweep, one long press).
- **The ride's decays:** each mode's loss times a factor by pitch (`Plate::setLoss`, `retune`).
  T60 at 500/1k/2k/4k/8k/12.5k Hz is now 3.7/3.8/4.1/2.6/1.7/1.1 s (before 5.6/4.5/2.5/2.3/1.7/1.3;
  FluidR3 3.8/3.9/3.9/2.6/1.65/1.06).
- **The ping did not move:** +6.1/4.9/6.8/10.8 dB at 1/2/4/8 kHz at Bönner's tempo (before
  5.6/6.3/6.7/11.1), against the recording's 12–19.
- **`domeTaper` does not remove the pile-up.** At the real dome height, any taper from 0 to 1.6
  leaves the crash's 400–800 Hz octave with all but 0.1–0.2 dB of the energy, and the splash's
  800–1600 Hz with all but 0.1. At a given height the taper moves the bands by about 1–2 dB. The
  crashes, splash and china keep a quarter of the real height (taper 1.4 on), felt back to 1×.
- **The beat, in a song:** the player learns where the bar is from the ride's own strokes: the
  quarters' phase, and 2 and 4 as the beats a skip note leads into (a song may start with a
  pickup).
  - On a synthetic Brush-kit spang-a-lang at equal velocities, beats 1 and 3 come out 3.0 dB louder
    and 2 and 4 2.0 dB softer than with the emphasis off (`PJ_CYM_BEAT=0`). The same holds with a
    one-beat pickup.
  - goodbait.mid selects the Brush kit only for its first 132 beats (then the Jazz, then the
    Standard kit), with five ride strokes in that part. Its brushed ride is too short to measure.
- **Cost:** ride 1060 ns/sample (5.1 %), brushed 1280 (6.1 %), 18" crash 1280 (6.2 %).
