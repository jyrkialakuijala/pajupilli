# pajupilli code review: software engineering, performance, real-time safety, testing

Reviewer: Claude (code/perf/RT/testing). Date: 2026-11-11, commit 701a84a.
Machine: Ryzen 9 9950X3D (Zen 5, full-width AVX-512), g++ 15.2, clang 21.
All experiments ran in a scratch copy. Live mode and `-selftest` were **not** run.
Note: the machine was heavily loaded during the review (load average ~60 on 32 threads),
so absolute timings vary by ±30 %. The ratios and the min-of-runs numbers hold.

## Summary

- **Vectorisation works.** Both per-sample loops in `Bore::step` compile to clean 512-bit
  code: 8 iterations of zmm `vfmadd/vfmsub`, all aligned except the two shifted loads
  (`p[i+1]`, `u[i-1]`). There are no scalar remainders, no runtime alias-check versioning
  and no gathers. Clang also vectorises them at width 16.
- **Performance is not a problem.** A steady note costs ≈170 ns/sample, which is **0.8 % of a
  core** (README says 0.5 %). Slurs cost ≈250 ns/sample. The worst 64-frame period during key
  motion was ≈70 µs out of a 1333 µs budget on a quiet core. R27 (<10 %) holds with about 12×
  headroom. The bore is only ~38 % of the time. The scalar jet/player is ~29 % and the
  radiation loop ~17 %.
- **Numerically robust under fuzzing.** 8 seeds × 60 s of random notes and controls gave no
  NaN or Inf. Peaks reached 0.999 before the volume knob. ASan and UBSan are clean on
  `-demo`, `-cal`, `-imp` and the fuzzer. `-Wall -Wextra -Wpedantic -Wshadow` is clean.
- **But a single NaN is permanent.** If one NaN gets in, the output stays NaN (silence in
  live, because of the guard) until restart. There is no recovery.
- **Scalar parity (R22) holds.** Single notes match to 3e-6 relative RMS, and `-cal` gives
  byte-identical tables. Long phrases drift in phase, so R22 should be tested on features,
  not samples.
- **FTZ/DAZ are on, but only by accident** (gcc/clang link `crtfastmath.o` with
  `-ffast-math`). The scalar build has them off, and there `Player::lung_` becomes a
  subnormal double ~22 s after a release.
- **Live-path issues:** an RT busy-loop on an unrecoverable PCM error, no MIDI hot-replug,
  xrun recovery without refill, and fprintf/seq scans in the audio thread.
- **Startup calibration takes 1.7 s** (README says 0.5 s). It parallelises to 0.28 s.
- **No automated tests.** A concrete `make test` is proposed below. It runs in ~10 s.

## Measurements

### Vectorisation (`-fopt-info-vec`, objdump of `Bore::step`)
```
bore.cc:136: loop vectorized using 64 byte vectors   (momentum + loss sections)
bore.cc:156: loop vectorized using 64 byte vectors   (3x3 node update)
Bore::step: 52 zmm instrs; the only xmm/ss code is the horn termination + ec bookkeeping
```
Loop 1 is 11 loads, 4 stores and 9 FMA/mul per vector. Loop 2 is 19 loads and 3 stores per
vector. The whole working set (state ≈4 KB + coefficients ≈9 KB) fits in L1.

`make asm` counts zmm in the whole file (354), so it would still pass if one loop stopped
vectorising. See test T1 for a stricter check.

### Time per sample (vector build, `bench/` harness, A4 steady unless noted)

| What | ns/sample | Note |
|---|---|---|
| `Bore::step` alone | 57 | 16 zmm iterations ≈ 300 cycles |
| `Player::render`, steady note | 170 | 0.8 % of a core |
| ... slurs every 20 ms (keys moving) | 250 | +50 %: full `Bore::update` every 16 samples |
| `Bore::update()` (128 × 3×3 inversions) | 1035 ns/call | runs once per 16-sample block while any pad moves |
| ... with per-cell dirty flags (tested) | 116 ns/call | 9× less. Slurs drop to ≈207 ns/sample. Output bit-identical |
| Scalar build (`-O2`, no AVX) steady | 360 | bore 265 ns: the vector bore is 4.7× faster |

Steady-note breakdown (rdtsc probes, ±5 % probe overhead):

| Part | Share |
|---|---|
| Jet/player scalar part (double `tanh`, `cosh`, `exp`, `sqrt`, `floor`, 2× noise) | 29 % |
| `Bore::step` | 38 % |
| Embouchure branch (Cramer) + jet history | 5 % |
| Radiation loop (18 radiators, branches, scatter into the ring) | 17 % |
| DC blocker / out | 4 % |
| `controlStep` / `Player` block | 1 % / 2 % |

While keys move, `controlStep` (which runs `updateHoles` and the full `Bore::update`) takes
33 % of the time.

### Other measurements

- **Denormals.** The vector binary starts with MXCSR=0x9fc0 (FTZ+DAZ). That comes from
  `crtfastmath.o`; no line in the code sets it.
  - In 80 s of silence after a note there were no float-subnormal bore states.
  - In the scalar build, `Player::lung_` decays through the double subnormal range after
    ≈22 s and stays at 2.2e-322 forever. That code is per-block, so it is cheap, but it is a
    latent trap.
- **Parity, vector vs scalar.** A4 and C7 over 1 s: max difference 1.8e-6 and 4.2e-6, rel
  RMS ≤ 1e-5. In the demo, all 29 scale notes have equal pitch and RMS. The vibrato phrase
  (11–13 s) drifts to 20–27 % sample difference, which is phase drift of a self-oscillator
  and not an error. `-cal` output is identical.
- **NaN.** One NaN injected in `p[40]` left 95861 of the next 96000 samples non-finite. The
  model never recovers.
- **Calibration time.** `./flute -cal` takes 1.6–1.7 s serially; the slowest note takes
  0.19 s. 37 `std::thread`s take 0.28 s. Output is identical because the notes are independent.
- **Sanitizers.** `-fsanitize=address,undefined` is clean on `-demo`, `-imp 69`, `-note 74`,
  `-cal` (60 s) and a 20 s fuzz.

## Findings, ranked by impact

**1. A single NaN or Inf is permanent (robustness). Effort S, risk low.**
- Evidence: the injection above.
- The live guard (`live.cc`, `if (!std::isfinite(y)) y = 0`) hides it as permanent silence.
  The player then has to restart the program, and recalibration takes 1.7 s.
- Fix: add `Flute::reset()` (memset `bore_.s`; zero `eq_ el_ ef_ pm_ uj_ xi_ jl* noise*
  dc* outRing_ vHist_ radPrev_` and `termL_`). Call it from `Flute::process` when the block's
  last output is not finite. One `isfinite` check per block is enough because a NaN reaches
  the output within the radiation delay.
- Count resets and report them in the live summary.
- While there, add a soft limiter (`tanh` or a knee) before the hard clip. Fuzz peaks reach
  0.999 before the volume knob, which allows 2×.

**2. Live loop: an unrecoverable error spins at SCHED_FIFO (RT safety). Effort S, risk low.**
- `snd_pcm_recover()`'s return value is ignored. On -ENODEV (USB unplug, PipeWire restart),
  `writei` fails immediately every iteration. The SCHED_FIFO thread then spins at 100 %
  (until RT throttling) and `frames` races ahead.
- Fix: if `recover` returns < 0, print once and exit (or reopen).
- After a successful recover, write `periods-1` periods of silence again before continuing.
  Today the stream restarts with one period buffered, which invites cascaded xruns.
- Handle short writes (`0 <= w < period`) by looping on the remainder.

**3. Live loop: MIDI hot-replug is missing, and the rescan runs in the audio thread (RT, UX).
Effort M, risk low.**
- `midi.connected` never goes back to false. If the MPK is unplugged and replugged, nothing
  reconnects it.
- The once-a-second rescan (`connectSources`, seq ioctls, and fprintf with `verbose=true`
  hard-coded) runs between `render` and `writei`.
- Fix: subscribe to `SND_SEQ_CLIENT_SYSTEM:SND_SEQ_PORT_SYSTEM_ANNOUNCE`, connect on
  `SND_SEQ_EVENT_PORT_START`, and drop the polling. These are cheap and event-driven.

**4. fprintf in the audio loop (`-v`) (RT). Effort S.**
- `cc`, `pad` and `xrun` messages go to unbuffered stderr from the RT thread. A slow terminal
  then causes the xruns being logged.
- Fix: push small event structs into a fixed ring and print them from a low-priority thread
  (or every N periods).
- Also print the SCHED_FIFO failure and the `mlockall` failure always, not only with `-v`.
  Latency depends on them.

**5. Startup calibration: 1.7 s on every `-live` (UX). Effort S, risk low.**
- Fix: run notes in parallel with `std::thread` (measured 0.28 s), or cache to
  `~/.cache/pajupilli/cal-<hash of PJ_* env + git rev>.txt`.
- Fix the README figure ("~0.5 s").
- Keep `setenv(PIPEWIRE_LATENCY)` after the threads are joined. `setenv` is not thread-safe.

**6. `Bore::update` rebuilds all 128 cells when any pad moves (perf). Effort S, risk very low.**
- Per-cell dirty flags (`setBranch` marks the cell; `update` rebuilds marked cells) cut the
  update from 1035 to 116 ns/call and slur cost by ~20 %, with bit-identical output.
- `Flute::updateHoles` also re-sends every hole cell when any one hole moves. Send only
  holes that moved or share a cell with one.

**7. FTZ/DAZ depend on a link side effect (RT perf, portability). Effort S.**
- Set them explicitly at the start of `runLive` (`_mm_setcsr(_mm_getcsr() | 0x8040)`). The
  setting is per thread, and a future worker thread would not inherit `crtfastmath`'s setup.
- Clamp slowly decaying doubles (`lung_`, `uj_`) with `if (|x| < 1e-30) x = 0` once per block.
- Write down in the Makefile comment that `-ffast-math` implies FTZ/DAZ at link time.

**8. Fast-math safety: OK as configured. No change needed.**
- `-fno-finite-math-only` keeps `std::isfinite`/`isnan` working (the calibration's NaN
  protocol and the live guard depend on it).
- Reassociation and contraction change results by ≤4e-6 relative, below float noise.
- `-mprefer-vector-width=512` is right on Zen 5 (no downclock).
- Keep `-fno-finite-math-only` with a comment saying why. Do not switch to `-Ofast`.

**9. Crashes and silent failures in the CLI (correctness). Effort S.**
- `./flute -imp 30` segfaults: `noteInfo` returns nullptr and `impulse()` dereferences it.
- `-dump /bad/path` segfaults (unchecked `fopen`).
- `-o /bad/path` silently exits 0 (`writeWav`'s result is ignored).
- `-note 30` prints a meaningless line.
- `-imp` prints a spurious first peak "100.0" (the peak test starts at the left edge with
  `prev2=prev1=0`).
- Fix: range-check notes, check I/O, return non-zero exit codes.
- Use `strtol`/`strtod` with checks instead of `atoi`. Today `-period x` gives 0 frames.

**10. Dead and debug code in the hot path (clarity, small perf). Effort S.**
- `PJ_JETMODEL=0` is broken: A4 plays at 680 Hz (+755 cents). Delete it. That also removes
  `gain` (an `exp` per sample, unless the compiler drops it), `kJetModel_`, the
  `vHist_ = eq_/esw_` variant, and the `if` in the hot path.
- `PJ_REL` (default 0, `kRel_*xi_`), `PJ_JET_SIGN` and `PJ_ETA_SIGN` are sign-debug knobs.
  Move them into a "debug knobs" comment block or drop them.
- `PJ_RADONLY` adds a branch per radiator per sample. Keep it as a tool, but hoist the test
  out by building the radiator list in the constructor.
- `Player::held_` is written and never read.
- `Bore::fs_` is unused (clang warns).
- `flute.h` includes an unused `<string>`.
- `Flute::horn_r()` is a 1-line alias.
- In `calibrate.cc`, `best = log(bestCover)` is immediately followed by `exp(best)`.
- `testDrive` is used by `-imp`. Keep it, but note it in the API comment.

**11. Misleading comment in `bore.h` (clarity). Effort S.**
- The comment says the embouchure cell is "excluded from the vector node update". It is not:
  the loop updates it, and `Flute::sample` overwrites `p[ec]`. That is fine (`q`, `l` at `ec`
  stay 0 because the branch is empty) but surprising.
- Also document that `u[kCells-1]` from the loop is overwritten by the termination, and that
  `psi`/`ck` past the foot are 0.
- Add `static_assert`s on layout assumptions: `kCells % kVec == 0`, `kPad >= 1`,
  `kOutRing > max radDelay` (max delay is ~150 samples; assert or clamp in `addRad`), and
  `kHist` a power of 2.

**12. Hot-path micro-optimisations (perf, optional). Effort M, total gain ≈30–40 %.**
- Only worth doing if the headroom is needed for oversampling or more cells.
  - (a) Hoist per-block constants out of `sample()`: `cosh(alpha W/h)`, `b`, `dd`, `aj`,
    `cm`, `bore_.compliance(ec_)` (a double divide per sample), and the Cramer
    sub-determinants that do not depend on `eq_`.
  - (b) Build the radiation list without the `-1`/`-2` special cases. Embouchure and foot go
    outside the loop; holes go in a flat array of `q` indices. That is 17 % of the time today.
  - (c) The u[127] scalar store followed by the vector reload in loop 2 is a store-forwarding
    stall (~10–20 cycles). Write the termination into a temporary and patch `u[127]` after
    loop 2 (J at the last cell only).
- Not recommended: specialising the q/l rows to hole cells. It breaks R20's uniform loops
  for ~10 % of the bore time.

**13. Structure and header hygiene (maintainability). Effort S–M.**
- `Controls` in `live.cc` initialises `sounding_` to 0 and relies on a separate `init()`
  call (a `public:` section in the middle of the class). Initialise to -1 in the
  constructor.
- `gCal` is a global in `main.cc`. Pass it instead.
- `Flute` exposes `bore_.s`/`k` publicly, which is useful for tests. Mark it as test-only.
- `Flute` mixes geometry tables, the jet, the window solver, radiation and the player
  airway (380 lines). Consider splitting `Jet`/`Airway` (scalar, per-sample) from
  `Flute` (bore + holes + radiation) when the jet model settles.
- Knobs are scattered in two constructors. A single `knobs.h` table (name, default, help)
  would let `./flute -knobs` print them all, which helps R23 and Jyrki's A/B work.
- `tools/tmm.py` duplicates the bore and hole tables. Have `./flute -geom` print the C++
  tables as JSON and let `tmm.py` read them, so they cannot diverge.
- Makefile: there are no `test`, `san` or `all` targets. Everything recompiles together,
  which is fine at 2.3 s.

## Proposed `make test` (tests/test.cc + a 30-line Makefile addition, no ALSA)

Build `tests/test.cc` together with `src/{bore,flute,player,calibrate}.cc`. Build it twice,
with CXXFLAGS and with SCALARFLAGS. Fail on the first broken check. Measured runtime ≈10 s.

| # | Test | Threshold | Today |
|---|---|---|---|
| T1 | Vectorisation: `-fopt-info-vec-optimized` on bore.cc reports both `step` loops "using 64 byte vectors" (grep the two loop lines by a marker comment, not line numbers). `objdump` of `Bore::step` has no `vgather`. | both loops present | pass |
| T2 | Stability (R24): fuzz as in the review harness (random note/velocity/dyn/breath/roll/flutter/noise/vibrato/hold every 10–100 ms), 3 seeds × 60 s | 0 non-finite, max\|y\| < 1.5 pre-gain | pass (0.999 max) |
| T3 | NaN recovery (after fix 1): inject NaN into `p[40]` | finite again within 2 blocks (32 samples); reset counter = 1 | **fail today** |
| T4 | Tuning (R25): `calibrate(440)`, all 37 notes `ok` | \|cents\| ≤ 3 for MIDI 60–87; ≤ 35 for 88–96; plus no note moves > 5 cents vs `tests/golden_cal.txt` (update the golden file on purpose) | pass (88: +9, 90–95: up to +30) |
| T5 | Register selection: each calibrated note played 0.5 s, then YIN over [f0/2.2, f0·2.2] | within ±50 cents of target, clarity > 0.8 (catches octave errors) | to measure |
| T6 | Slur registers: the demo scale; pitch of each 0.22 s segment | each within ±50 cents of its note | check (uncalibrated D5 sounds D4) |
| T7 | Determinism: render the demo twice in one process | bit-identical | pass |
| T8 | Scalar/vector parity (R22): the two test binaries write A4/D5/C7 1 s; the script compares | rel RMS < 1e-4 over the first 0.5 s; per-note pitch equal within 1 cent and RMS within 1 % for the whole demo; identical `-cal` table | pass |
| T9 | Passive resonances (R26): `-imp` peaks for 60, 69, 74 against a golden list (or `tmm.py`) | first 3 peaks within ±10 cents of golden | pass by definition at first |
| T10 | RT margin (R27): steady render, min of 5 × 1 s runs | ≤ 400 ns/sample (≈2 % of a core) vector; also log the slur case | 170 / 250 |
| T11 | Live-path hygiene: run `Player::render` under a `malloc` hook (override `operator new`/`malloc` counters in the test binary) for 10 s of fuzz | 0 allocations after construction | expected pass |
| T12 | `make san`: ASan+UBSan build of the test binary running T2 (10 s), T7 and `-imp 69` | no reports | pass |

For the CLI, add `./flute -test` wrappers, or keep everything in `tests/`. Never call
`runLive` from tests. Make `-selftest` refuse to run unless an explicit `-dev` is given, so
that automated tools cannot play sound by accident.

## Suggested order

1. Fixes 1, 2 and 9 (≈1 h): crashes, NaN latch, RT spin.
2. `make test` with T1, T2, T4, T7, T8 and T10 (≈2 h). It locks in parity and vectorisation
   before more model work.
3. Fixes 5 and 6 (≈1 h): startup 1.7 → 0.3 s, cheaper key motion.
4. Fixes 3, 4 and 7 (≈2 h): live robustness.
5. Cleanup: fixes 10, 11 and 13. Only fix 12 if more headroom is needed.

Scratch artefacts (harnesses: bench.cc, b2.cc probe build, fuzz.cc, sub.cc, cal.cc, den.cc):
`/tmp/claude-1000/-home-jyrki-pajupilli/b0250b6e-5038-4aaa-b429-7d4c393762a5/scratchpad/review_code/bench/`
