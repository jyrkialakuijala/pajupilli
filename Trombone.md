# Trombone code review (2026-11-18)

A read-through of the trombone path as a computational scientist and software engineer. Nothing was built, run
or measured: every finding below comes from reading the code and doing the arithmetic by hand, so each one names
the check that would confirm it. The review stopped before `hornplayer.cc`, `tools/brass.py`, `calibrate.cc`
and the tests were read.

Files read: `src/brass.cc` (the trombone's bore, valve, slide, lips, notes), `src/horn.cc` and `src/horn.h`
(lips, valves, slide, brassiness), `src/bore.cc` and `src/bore.h` (the waveguide), the `LipParams` struct in
`src/instrument.h`, and `Tube::setGeometry`, `addRadiators`, `radiate` in `src/tube.cc`.

## What is sound

- **The cup and lip-flow solve** (`horn.cc:396-414`). The trapezoidal cup with the Bernoulli square root in
  closed form is derived correctly: with D = pm - pmid and u = A sqrt(D), the quadratic s^2 + beta A s - R = 0
  and its root match the code, as do the reverse-flow branch and p_new = 2 pmid - pOld.
- **The slide's length bookkeeping** (`horn.cc:113-118`, `267-272`, `bore.cc:140`). 133 cells are stretched
  (cells 6..138 at dx = 11.2 mm). The interfaces take the mean of their two cells' stretch, so the mass adds up
  to exactly the same 133 (s - 1) dx as the volume. The added tube equals the requested millimetres whatever
  the truncation of the slide's ends to whole cells.
- **Stability under stretch.** Stretching only lowers the local Courant number (0.975 / s), as the comment in
  `bore.h:132-134` says. Valve-loop stretch is clamped to >= 1 for the same reason (`horn.cc:190-191`).
- **Brassiness constant** (`horn.cc:122`). The time advance L beta p+ / (rho c^3) with beta = 1.2 is right.
  At p+ = 10 kPa and L = 2 m it is 15 samples at 32 kHz, well inside the 48-sample look-ahead.
- **Position and trigger parsing** (`horn.cc:157-165`). '1'..'7' and 'A'..'G' index `kTromboneSlide[0..13]`
  correctly, and the note table's F-side entries (C2 "Fx" = 1242.5 mm, and so on) agree with the comment.
- **Viscothermal factor** (`bore.cc:99-103`). 1.47 = 1 + (gamma - 1) / sqrt(Pr), and the fit is of the whole
  sqrt(s), so the dispersion part of the wall loss is included, not only the damping.

## Findings

### 1. A moving slide changes the stored energy with the wrong sign (medium)

`Bore::setStretch` (`bore.h:135-141`) scales a cell's compliance and its interfaces' mass by s but leaves p
and u as they are. The energy stored in the slide, C p^2 / 2 + L u^2 / 2, is therefore multiplied by
s_new / s_old at every control block: it grows as the slide goes out and shrinks as it comes in.

For a slowly lengthening resonator the invariant is E / omega, so the energy should fall as the pitch falls.
The model does the opposite. Going from 1st to 7th (s = 1 to about 1.8) it adds up to about 2.5 dB to the
slide's share of the field where physics removes about 1.5 dB over that tritone; coming in, the reverse.

The lips restore the steady state within some tens of milliseconds, which is about as long as the slide move
itself, so the error would show as a level bump on outward slurs and glissandi and a dip on inward ones.

- **Fix:** conserve the charge and the flux when the stretch changes. Multiply p[i] by s_old / s_new in the
  stretched cells and u[i] by the same ratio of the interface stretch. Then E scales as 1 / s, which is the
  adiabatic result.
- **Check:** a slow glissando 1 -> 7 -> 1 at constant breath; compare the level against held notes at the
  same positions.

### 2. The bell's radiation load reflects about 20 % at high frequencies (medium)

The open end is a resistance 1.505 rho c / S in parallel with the mass of 0.6133 a (`bore.cc:104-113`). That
gives the right low-ka value, Re Z = rho c / S (ka)^2 / 4. Above ka of about 2 the load tends to
1.505 rho c / S, not rho c / S, so the end reflects (1.505 - 1) / (1.505 + 1) = 0.20 of the wave where a real
bell lets it out.

On the flute's foot (a = 9.5 mm) ka = 1 is at 5.7 kHz and this does not matter. On the trombone's bell
(a = 107 mm) ka = 1 is at about 510 Hz, so everything above about 1 kHz sees the mismatch. The expected
symptoms are weak spurious resonances above the bell's cut-off and a ripple in the loud, bright spectrum.

- **Fix:** a second-order radiation model that has both limits right, for the brass only (the flute family
  was fitted with the present one).
- **Check:** the input impedance above 1 kHz against Bilbao & Chick's measured curve. Their peaks 1-16 are met
  within 1 % by the transfer matrices, but the peak heights above cut-off are what this affects.

### 3. Brassiness does not grow with the slide (medium-low)

`nlK_` is fixed from `nlLen` = 2.0 m (`horn.cc:122`, `brass.cc:261-263`). In 7th position the cylinder is
1.18 m longer (1.5 m on the F side), so the steepening length should be about 60 % larger there. As written,
a loud low E in 7th is no brassier than a B-flat in 1st at the same pressure.

- **Fix:** nlK proportional to (nlLen + slideNow_ * 1e-3), updated in `setSlide`.

### 4. The slide is about twice as fast as an arm (medium-low; plausibility, not measured)

`slideDur_ = (0.03 + 0.06 sqrt(reach)) / sqrt(fingerSpeed)` (`horn.cc:171`) gives 90 ms from 1st to 7th. That
is 592 mm of travel at a mean 6.6 m/s, with a peak near 10 m/s under the smoothstep. I remember measured
slide speeds as a few metres per second at most; I did not look the figure up, so check it against
`research/trombone.md` before changing anything.

If the real figure is 200-250 ms for the full reach, legato over large shifts now sounds more like a valve
than a slide.

The comment above the line says "~60 ms from 1st to 7th"; the formula gives 90 ms. One of the two is stale.

### 5. The bore and the design tool may disagree up high (low)

In the stretched slide the grid is effectively up to 2 dx (22 mm) at Courant about 0.49. The leapfrog scheme's
phase error there is about (k dx)^2 / 24 x (1 - lambda^2). That is roughly 1 cent at partial 8 (negligible)
but about 0.5 % at 1 kHz and 4-5 % at 3 kHz in the slide's part of the tube.

The positions were fitted with `tools/brass.py`'s transfer matrices, which have no such dispersion. The
calibration absorbs the note's own pitch, but the alignment of the upper harmonics with the bore's resonances
differs between 1st and 7th position in a way the real instrument's does not.

- **Check:** the bore's impulse response in 7th against brass.py's peaks for the same geometry.

### 6. The crook is stretched with the legs (low)

All cells between 62.6 and 1567.4 mm are stretched, including the crook's bulge at 737.7 mm (14.79 mm bore),
which does not telescope. On the instrument the added tube is the outer slide's bore, a little wider than the
13.6 mm inner legs. The effect is small and the position fit absorbs it. It matters only if brass.py models
the slide differently from `setSlide`; "keep the two in step" (`brass.cc:3-4`) is the thing to verify.

### 7. Shock handling takes the overtaking sample (low)

In `Horn::brassy` (`horn.cc:277-293`) arrival times are kept monotonic with a running maximum, and when two
samples arrive together the later one's value is used (`f = 1`). That puts the shock at its earliest possible
place. The equal-area rule would put it between the two and make it weaker. With `PJ_BRASSY` and the two
low-passes after it this is tunable, but the knob then also compensates for this bias.

The steepened wave is only added to the radiated sound; nothing is fed back into the bore. The bore therefore
loses no energy to the shock, and the loud playing regime is the linear bore's.

### 8. Smaller points

- **Lip sweep on the mouth side.** The lips' swept flow enters the cup (`horn.cc:398`) but is not taken from
  the mouth: `pm_` is drained by `u_` only (`horn.cc:373`, `413`). With a 60 cm^3 mouth the effect is small.
- **Explicit lip coupling.** The lips are driven by the last sample's pressure difference (`horn.cc:349`).
  That is fine at 32 kHz for lip resonances below 2 kHz, but it is the first place to look if a high note
  misbehaves at a lower sample rate.
- **First note without `settle()`.** `slideTo_` starts at 0, so a first note in another position slides
  there during the attack unless `settle()` runs first. `hornplayer.cc` was not read to see whether it does.
- **Positional `LipParams` initialisers** (`brass.cc:261-263`). 24 unnamed values; the trombone's are in the
  right order today. Designated initialisers would make the next added field safe.
- **Rebuild cost while sliding.** `setStretch` rebuilds each stretched node at once (133 3x3 inversions per
  16-sample block) and then `buildU` runs over all 512 interfaces. It is not a hot spot, but the nodes'
  matrices depend on the stretch only through the compliance, so one inversion per distinct value would do.

## Not reviewed

`src/hornplayer.cc` (breath, slurs, how positions and alternates are chosen), `tools/brass.py` (the fit),
`src/calibrate.cc`, `tools/test.py` and `research/trombone.md` beyond what the code comments quote from it.
Findings 4, 5 and 6 depend partly on what those say.

## What was changed after the review (2026-11-18)

Built and measured in a separate worktree (old build against new, same sequences), then applied to `src/bore.cc`,
`src/bore.h`, `src/horn.cc`, `src/horn.h`, `src/hornplayer.cc`. Not committed; `./pajupilli` was not rebuilt
(another session was using it). `tools/test.py`'s `brass()` passes on the new build as on the old (all four
instruments; trombone waveguide vs brass.py worst 8.1 / 7.4 / 6.9 cents, unchanged).

| finding | done | measured |
|---|---|---|
| 1. slide energy | `Bore::moveStretch`: the slide's cells keep their charge C p and their interfaces their flux L u when the stretch changes | Held notes: pitch and level as before. A same-partial slur B-flat 3 <-> F-sharp 3: level within 1 dB of the old build, the pitch settles within 12 ms of it. The effect is small, as the lips restore the level quickly; the fuzz test's peak rose 6.5 -> 8.6 (a fast inward slide now raises the pressure, as it should) |
| 3. brassiness with the slide | `nlK_` scales with (nlLen + slide extension) | ff notes in 7th: the 2-8 kHz share moved by 0.1 dB or less. The steepening term adds almost nothing at these pressures, so this is consistent but not audible; whether `PJ_BRASSY` is doing enough at ff is a separate question |
| new: the slide in a rest | `Horn::slideAhead`, called from `HornPlayer::applyNote`: the arm has moved during the rest | Found while measuring: every tongued note started with the slide still travelling from the last position. E2 (7th) after a 0.5 s rest: the old build wobbled between -7 and +16 cents for 300 ms; the new one is at +6..+8 from 60 ms on |
| 4. slide speed | **reverted**: the review was wrong for slurs | At 150 ms for the whole reach a down-slur fell up to 245 cents under the note for ~100 ms: the lips retune at once and the bore lags. `research/trombone.md` 6 says the arm hurries in a slur (<= 40..60 ms). The formula is as it was; only the stale "~60 ms" comment is corrected (it gives 90 ms) |
| 2. bell radiation | done, see below | |
| 5-8 | not done | 5 and 6 need `tools/brass.py` read first; the lip sweep on the mouth side (8) would shift the tuning of all brass |

Both builds show a brief low reading (about -300 cents for one 12 ms frame) 50 ms into a same-partial
down-slur: that is in the old build too and was not looked into.

## Bell radiation (added 2026-11-18)

The unflanged pipe's reflection after Silva et al. 2009, -(1 + 0.167 s) / (1 + 1.393 s + 0.457 s^2) with
s = j ka (the constants are from memory of the paper; they reproduce the code's own 0.6133 a and 1.505, which
is a good check, but compare them with the paper). Written as a circuit it is exactly the bore's present load
(the mass of 0.613 a beside 1.505 rho c / S) with a third branch beside them: 2.979 rho c / S in series with
the compliance of 0.125 a of the opening's air. So nothing in `bore.cc` changes and the flute family is
untouched: `Horn` adds that one branch at the last cell (`src/horn.cc`, `bellCell_`), takes its flow from the
cell after each step and radiates it with the rest. `tools/brass.py`'s `bell_load` is the same circuit.

The review's "the bores would need refitting" was wrong. Measured with brass.py, old load against new:

- Every partial the bores, valve loops and slide positions were fitted to moves less than 1 cent (horn 4..12,
  trumpet 3..8, tuba 2..10, trombone 3..8 in 1st and 7th).
- Above the bell's cut-off the peaks rise in pitch and fall in height: partials 13..16 by +1..+8 cents and
  up to -1 dB (tuba, horn, trumpet most; the trombone +3 cents).
- The waveguide follows: trumpet partial 13 1519.8 -> 1523.9 Hz (+4.7 cents, brass.py +3.9), trombone partial
  16 955.9 -> 958.0 Hz (+3.7, brass.py +2.8), the low partials unchanged to 0.02 Hz.

`tools/test.py`'s `brass()` passes for all five instruments as before (notes sound and tune, held notes
steady, fuzz, NaN recovery, waveguide against brass.py: horn 10.0 / 10.5, trumpet 5.1 / 6.6, trombone
8.1 / 7.4 / 6.9, tuba 12.1 / 9.5 cents).

Radiated spectrum of held notes, new minus old (dB):

| note | total | < 0.5 k | 0.5-1 k | 1-2 k | 2-4 k | 4-8 k |
|---|---|---|---|---|---|---|
| trombone B-flat 2, mf and ff | 0.0 | 0.0 | -0.4 | -0.8 | +1.5 | +2.4 |
| trombone B-flat 3, ff | -0.1 | 0.0 | -0.2 | -0.3 | +0.1 | +0.3 |
| trumpet B-flat 4, ff | -0.3 | 0.0 | +0.1 | -0.6 | -0.5 | +0.2 |
| tuba C2, ff | -0.3 | -0.2 | -1.4 | +0.7 | +1.9 | +1.8 |
| horn F3, ff | -0.1 | 0.0 | -0.1 | -0.7 | -2.7 | +1.6 |

Limits: the branch hangs on the last cell's pressure, not behind the short tube from that cell's centre to
the rim (up to one cell, 11 mm at 32 kHz), and it uses the last cell's mean area as the bore's own load does,
not the rim's. The Harmon mute's 17 mm hole gets the branch too; there ka = 1 lies at 6.5 kHz and it does
nothing audible. Not listened to.
