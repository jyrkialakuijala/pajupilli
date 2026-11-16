# The chromatic percussion (General MIDI 9–16): what was built, and on what numbers

Status: 2026-11-16. Built: `-inst celesta`, `glockenspiel`, `music-box`, `marimba`, `xylophone`,
`tubular-bells` on one engine (`src/bars.h/.cc`, `src/barsinst.*`), and `dulcimer` on the piano's
strings (`src/dulcimer.cc`). The vibraphone (GM 12) was built earlier (`research/vibraphone.md`,
`src/modalinst.cc`). The celesta has its own research file (`research/celesta.md`) and is fitted to a
recording. **The other six are not:** no literature search was made for them and nothing was
measured. Their numbers below are marked **[memory]** (general acoustics knowledge, to be checked
before it is relied on), **[calc]** (computed here from those) or **[est]** (a working guess, set so
the result measures plausibly). They are first forms, to be corrected by ear and by recordings.

## 1. The engine

One sounding piece per key: up to nine modes, each with its share under the mallet, how that share
changes as the mallet strays (new for every stroke), its loss, its sound at two microphones, and
(some) its push on a resonator's single air mode and a damper. The mallet is a mass on a contact
F = K δ^p, its force found each sample against the piece's own give, so a second stroke meets the
piece where it is.

- **Checked on the celesta:** a second blow 0.2 s after the first, moved through one period of the
  fundamental, changes the fundamental from +4.9 to −9.9 dB against a single blow, and the overtones'
  share by up to +11 dB **[calc, the model]**. Repeated notes therefore differ in spectrum.
- **A free bar moves no air on the whole** (its modes are orthogonal to the bar moving as a body)
  **[calc]**. What it radiates is its middle part against its ends: weak while the bar is short
  against the wavelength. The model takes the middle half's motion × min(1, k L / 3) for the
  symmetric modes, one half's × min(1, k L / 2) for the antisymmetric, and × min(1, k w) for the
  two faces **[est]**.

## 2. Glockenspiel (GM 10): G5–C8 as sounding

- Steel bars 9.5 mm thick, 32 mm wide narrowing to 25 mm **[memory]**; lengths from the uniform
  bar's f = 1.028 t √(E/ρ) / L²: 246 mm (G5) to 106 mm (C8) **[calc]**.
- Modes: the plain free bar's, 1 : 2.757 : 5.404 : 8.933, and the first twisting mode for strokes
  off the centre line (4.9 × f at G5, 2.1 × at C8) **[calc]**. No tuning of overtones and no
  resonators **[memory]**.
- Loss: 2·10⁻⁴ of critical, a little more per mode, plus the rail's felt **[est]**: the model
  rings 5.9 s at G5, 1.8 s at G7.
- Mallet: 22 g, hard (K 1.5–4·10⁹ N/m^1.5) **[est]**. No dampers.

## 3. Marimba (GM 13): C2–C7

- Rosewood (900 kg/m³ **[memory]**), bars 60 cm × 68 mm to 19 cm × 40 mm, 22 mm thick **[memory]**,
  carved underneath so the second mode is two octaves over the first: modelled ratios 1 : 3.92 :
  9.24 : 16.3 **[memory]**. The shapes used are still the uniform bar's **[est]**.
- Struck at 0.38 of the length, ± 5 % from stroke to stroke **[est]**: the second mode has a node
  at the middle, so the place decides how much of the "fourth harmonic" sounds.
- Loss: 0.0025 of critical (Q ≈ 200), rising per mode **[est]**. A closed quarter-wave tube under
  each bar, loss 0.012 of critical, coupled so the fundamental dies 1.7 times faster than the bare
  bar's **[est]**: 6 s at C2, 0.9 s at C4, 0.2–0.3 s at the top.
- Mallets: 30 g yarn, K 3·10⁶ (bass) to 4·10⁷ (top) N/m^1.5 **[est]**.

## 4. Xylophone (GM 14): F4–C8 as sounding

- Rosewood, 38 cm to 13 cm, about 38 mm wide, 19 mm thick **[memory]**, tuned for a twelfth:
  1 : 3 : 6.1 : 9.9 **[memory]**. Struck at 0.36 of the length **[est]**.
- Loss 0.0045 of critical; short tubes (coupled for 1.3 times faster) **[est]**: 0.5 s at F4,
  0.2–0.4 s above. Mallets 20 g, hard: K 3–8·10⁸ **[est]**.
- Not modelled: the tube's own third harmonic, which lies on the bar's twelfth and is the usual
  reason given for this tuning **[memory]**.

## 5. Tubular bells (GM 15): C4–F5

- Brass tubes 38 mm across, 1.5 mm wall **[memory]**. A free tube's bending modes lie nearly as the
  odd squares 9 : 25 : 49 : 81 : 121 : 169 … **[memory]**. No mode is at the pitch heard: the fourth,
  fifth and sixth lie as 2 : 2.99 : 4.17 and the ear hears the note an octave under the fourth
  **[memory]**. The model tunes the fourth mode to twice the key's pitch and takes nine modes
  (0.22, 0.62, 1.21, 2, 2.99, 4.17, 5.56, 7.14, 8.91 × the note). Lengths: 1.66 m (C4) to 1.02 m
  (F5) **[calc]**.
- Struck on the top end, where every mode moves fully; a 200 g rawhide hammer, K 4·10⁷ **[est]**.
- Loss 0.35 /s plus 0.5 /s per kHz **[est]**: about 8 s. Radiation rising with the tube's width
  against the wavelength, so the lowest modes are hardly heard **[est]**.
- Not modelled: the end plug's mass (which pulls 4 : 5 : 6 nearer 2 : 3 : 4), the damper pedal.

## 6. Music box (GM 11): C4–C8

- A steel comb's teeth, clamped at the root: 1 : 6.267 : 17.55 : 34.39 **[calc, the cantilever]**.
  Tooth masses 2.5 g (leaded bass) to 0.25 g **[est]**.
- Plucked, not struck: the pin bends the tooth and lets it go, so each mode starts from where the
  bending force held it; the pin slides off over about 0.5 ms, which the high modes are too quick
  to feel fully **[est]**.
- The tooth is far too small to be heard. Its root shakes the comb (each mode by its share of the
  tooth's mass moving one way: 0.78, 0.43, 0.25, 0.18 **[calc]**) and the comb the box, modelled
  as a rise to 300 Hz and a fall above 7 kHz **[est]**, plus four short box resonances for the
  pin's tick.
- Loss 8·10⁻⁴ of critical, more per mode **[est]**: 5 s at C4, 0.7 s at C7.

## 7. Hammered dulcimer (GM 16): G2–A6

- On the piano's engine (`PianoDesign`): courses of two strings (below G3) or three, plain steel
  0.5 mm thickening to 0.9 mm in the bass, 166 mm at D6 and 1.7 times longer per octave down (to
  1.2 m), the lowest overspun to hold 80 N **[est]**; struck at 0.14 of the length **[memory]**.
- Hammers: 2–3.5 g of wood faced with leather: the grand's felt stiffness × 6, no soft layer
  **[est]**. No dampers: every note rings into the next. Unisons 0.8–1.2 cents apart: the shimmer
  **[est]**.
- **The soundboard is still the grand's.** A dulcimer's small trapezoid of spruce and its two
  bridges are not modelled; this is the largest known fault.

## 8. Checks (`tools/test.py`, celesta())

In tune within 10 cents (the bells: their fourth mode an octave over the note, within 5); 10 s of
random playing stable; the bars 0.5–0.9 % of a core.

## 9. What is owed

A literature and recordings pass for each of the six, as the celesta had: measured bar dimensions
and mode ratios (marimba and xylophone bars are not uniform, so their shapes and masses are
wrong in detail), decay times, mallet contact times, and the levels across each compass, which are
set here only so that the registers measure roughly even.
