# Making the models faster: what has worked, what has not

2026-11-25. Collected from speed work in pajupilli (the pads review, the pow conversion, the
piano and synth-strings passes) and in puhuja (LEARNINGS.md "Speed", OPEN_TECHNICAL.md section 5).
Every number was measured in the place named; none is a general promise. The same file is kept
in both repos (`~/pajupilli/SPEED.md`, `~/puhuja/SPEED.md`): add a finding to both.

## 1. Measuring

- **Profile first.** Guesses are wrong. In puhuja the wave-propagation loops, the obvious kernel,
  are under 5 % of a render; the fold model is 26-28 %, the per-sample control function 14 %. In
  pajupilli's pads the cost was in things recomputed around the filter, not in the filter.
- **Count instructions, not time, on a loaded machine.** `perf stat -e instructions:u` repeats to
  a few parts per million; cycles moved 23-31 G for one binary (puhuja). Wall time with nine
  agents running was good to about ±5 % even as the minimum of several runs (pajupilli).
- Instructions miss cache misses and store-forwarding stalls (section 4), so confirm a win in
  time once on a quiet machine.
- **A unit to compare by:** ns per sample per voice (pajupilli's pads: 177-500), G instructions
  for a fixed render (puhuja's Kennedy: 266.7 -> 246.3; pajupilli's 34 s piano sequence:
  331 -> 181).
- **For live playing the worst block counts, not the mean.** A 64-frame block has 1.33 ms; the
  halo pad at 32 voices has a mean of 90 % of real time and a p99 of 1.50 ms: over.
- **Keep the binary from before the change** and compare the output with it: identical, or the
  difference in dB. A microbenchmark of one function says little about a render: pow ~16 ns
  alone, 45 ns on the loaded machine.
- Two test runs at once share temporary files in pajupilli (`tools/.swirl.mid`,
  `tools/.fretlessboard`) and fail each other. One at a time.

## 2. The cheapest wins: do it less often

These gave the largest gains and most keep every bit.

| What | Where | Gain | Output |
|---|---|---|---|
| Constants of a mode recomputed every 16 samples, cached per strike | pad-metallic `controlStep` | 381 -> 266 ns (-28 %) | identical |
| Coefficients recomputed every block, skipped when nothing changed | pad-choir `Singer::coefs()` | 358 -> 305 ns (-15 %) | identical |
| A slow gain (0.1 s glide) computed per sample with pow and cbrt, moved to every 16 samples | pad-halo's compressor | 542 -> 360 ns (-32 %) | -68 dB |
| A system that has become linear treated as linear | the piano's seated dampers (`PJ_PIANO_DLIN`) | 331 -> 181 G | -46 dB |
| Several sums over the same data in one pass | the piano's `feltSums` | (part of the above) | exact |

- Ask of each line in the sample loop: how fast does its input change? Envelope, wander, a
  knob: control rate. A note's pitch and build: once per note.
- When a control-rate value steps, slew it, or the step is a click (the pads' resonance wander
  steps the gain once a block; puhuja's clicks are the same fault).
- Voices that are silent should cost nothing. Pajupilli's circuit voices run idle and released,
  on one thread: not fixed yet.
- No allocation in the audio path (`Ensemble::block` allocates every block; `Lead::side_` grows
  by `push_back` per sample: not fixed yet).

## 3. Mathematical functions

Measured alone on this machine: `pow` ~16 ns, `exp(y * log(x))` ~10 ns, `cbrt` ~14 ns,
`exp` ~5 ns.

- **`pow` with a variable base is the slow one.** Pajupilli: `fastPow(x, y)` in `src/vmath.h`
  (`exp(y * log(x))` for x > 0, `std::pow` otherwise), within ~1e-15 of `pow`; renders equal in
  float32 but for the violin (-102 dB).
- **What the compiler already does** (GCC, `-ffast-math`): a constant base becomes `exp` or
  `exp2`; exponents that are multiples of 1/4 (2, 3, 1.5, 0.25, -0.5) become multiplies and
  square roots. Rewriting those by hand wins nothing. Without `-ffast-math` (puhuja) do not
  assume it: look at the assembly.
- **Converting 372 pows did not speed up a render** in pajupilli: nearly all sit at note-on or
  control rate. Find the pows in the sample loop first (puhuja: `powf` in `setAreas` was 4 % of
  the time; four square roots replaced it).
- Use the fast forms that exist: `std::tanh` where `fastTanh` would do cost 8 % on two pads.
- An `exp` that the model needs per sample is a floor: the ladder filter's 11 `exp` are 52 % of
  a lead voice, and inlining `exp` did not help.
- Divisions: 3.5 % of a puhuja render, most divisors changing every step. Little there.
- `-ffast-math` lets glibc swap `exp` / `log` / `pow` in vectorised loops for libmvec, so an
  unrelated edit can change the rounding. Puhuja builds without it for that reason (and `-O3`
  without it was faster there: 1.34 against 1.70 s). Pajupilli builds with it.

## 4. Vector code

- **Check, do not assume:** `-fopt-info-vec-missed`. A `bool` test on the loop index kept a
  16-lane loop scalar; computing every lane and blending the special ones to zero vectorised it
  (puhuja).
- **Store-forwarding:** a scalar loop reading single floats from a row just stored as a whole
  vector waits for the store. One such line was 7 % of a puhuja render. Keep whole vectors (a
  lane shift inside the register).
- Denormals: checked in pajupilli's pads, fine in both builds.

## 5. Keeping the bits

- **Bit-identical changes first, in their own commits.** They need no listening.
- What keeps the bits: caching a value computed by the same expression; hoisting a quotient with
  a constant divisor, written the same way. What changes the last bit: `x * (1/c)` for `x / c`;
  regrouping a product of constants; `x * x` or `sqrtf` for `powf` (differs for about 0.07 % of
  floats); `exp(y log x)` for `pow`.
- **In a chaotic model a last bit is a new performance.** Puhuja's folds decorrelate within a
  second, so every such change is a new draw of the same reading and has to pass the gate.
  Collect them and gate once. In pajupilli most instruments stay within -100 dB and a comparison
  of the renders is enough.
- Stored reference binaries depend on the system's libm: link it statically or record the
  toolchain (puhuja, open).

## 6. Build flags

- Profile-guided optimisation: 17 % slower (puhuja). Link-time optimisation: 3 % faster, and the
  build faster.
- Pajupilli: `-O3 -march=native -mprefer-vector-width=512 -ffast-math -fno-finite-math-only`,
  and a scalar `-O2` build to compare with.
- Puhuja: `-O3 -march=native -fno-math-errno -fno-signed-zeros -fno-trapping-math
  -ffp-contract=off`.

## 7. Open

- Pajupilli: the three pad wins of section 2 are measured, not applied (`review/pads.md`);
  idle voices; allocation in `Ensemble::block`; `fastTanh` in `lead.cc`.
- Pajupilli's synth strings went from 1856 to 1105 instructions a sample (within -146 dB) in
  another session; what did it is not written up here.
- Puhuja: the fold model (`StringFolds::step`, 28 %) and the tract substeps (17 %) are what is
  left; `make bench` on instructions beside output md5s.
- The click guards for puhuja cost about +3.6 % (`CLICKS.md`).
