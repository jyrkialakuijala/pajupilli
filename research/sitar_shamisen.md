# Sitar (GM 105) and shamisen (GM 107) as modal string models — research notes

Status: research only (2026-11-14). Nothing here is implemented. SI units.
**[G]** marks my estimates or guesses, not published values. **[D]** marks values I derived from published
formulas or data, with the derivation shown or obvious. **[U]** marks a claim taken from a secondary source,
a forum, an abstract or a figure, which I could not check in a primary text. Untagged numbers are quoted from the
primary source cited next to them. References are keyed; the list is at the end.

Both instruments fit the plucked double bass's frame (`src/bass.h`, `research/double_bass.md`): strings as sums
of modes in two polarisations, a bridge, body modes, a plucking excitation. What is new, in order of importance:
1. **A one-sided obstacle at a string end**, which is the instrument's sound, not an extra. On the sitar it is a
   wide, gently curved bridge the string wraps onto (*jawari*, distributed contact over ~10–20 mm). On the
   shamisen it is a small ridge (*sawari-yama*) about 10 mm from the nut under the first string only (nearly a point
   contact). The bass's fingerboard contact (`kBoard = 5` points, an explicit penalty spring) is the right template,
   but it needs (a) much stiffer contact, (b) points concentrated within 20 mm of one end, (c) many more modes (the
   buzz lives at 1–8 kHz), and (d) an energy-stable update (§3.4).
2. **String stiffness (inharmonicity) is essential**: without it the jawari's characteristic *descending formant*
   (the "precursor" that escapes the bridge's closing) disappears [Sid12, vWBM16].
3. **Sympathetic strings** (sitar: 11–13 taraf) tuned to a raga, sharing the bridge with the played strings. The
   bass already lets undamped strings ring on a shared bridge.
4. **Tension control during a note** (sitar *meend*: a sideways pull raising the pitch up to a fifth).
5. **A plectrum that also hits the body** (shamisen *bachi*: string and skin struck at once).

---

## 1. The sitar

### 1.1 Strings

Layout of a Ravi Shankar–style (*kharaj-pancham*) sitar: 7 playing/drone strings on the main bridge plus 11–13
sympathetic (*taraf*) strings on a small second bridge under the frets [Wiki-S, LMM]. Sa (the tonic) is usually
C#, sometimes C or D [RJ]: Ravi Shankar C#, Vilayat Khan C/C#, Manilal Nag D [RJ, U].

| # | Name | Note (Sa = C#) | Pitch, Hz | Material, ⌀ | Length | Tension | B |
|---|---|---|---|---|---|---|---|
| 1 | baaj (main melody) | Ma | F#3 185.0 [D from U] | steel No. 3, 0.012" = 0.305 mm [U: LMM, Karuna set] | 0.88 m [U: RJ] | 61 N [D] | 1.8e-5 [D] |
| 2 | jod | Sa | C#3 138.6 [G] | bronze 0.014–0.016" (0.36–0.41 mm) [U] | 0.88 m | 53 N [D, 0.36 mm] | 2.2e-5 [D] |
| 3 | pancham / laraj | Pa | G#2 103.8 [G] | bronze 0.020–0.022" (0.51–0.56 mm) [U] | 0.88 m | 60 N [D, 0.51 mm] | 7.8e-5 [D] |
| 4 | kharaj | low Sa | C#2 69.3 [G] | bronze 0.028–0.032" (0.71–0.81 mm) [U] | 0.88 m | 59 N [D, 0.76 mm] | 3.9e-4 [D] |
| 5 | kharaj-pancham (some sitars) | lower Pa | G#1 51.9 [G] | bronze ~0.8 mm [G] | 0.88 m | — | — |
| 6, 7 | chikari (rhythm drones) | Sa, high Sa (sometimes also Pa) | C#4 277, C#5 554 [G] | steel 0.009–0.011" (0.23–0.28 mm) [U] | 0.45–0.6 m [G]: they leave the neck at side pegs | 50–80 N [D at those lengths] | ~1e-5 [D] |
| taraf ×11–13 | sympathetic | the raga's notes | C#4…F5 (277–700 Hz) [G] | steel 0.009" (0.23 mm) [U] | 0.6–0.7 m, graduated [G] | 36–310 N [D]: the top taraf must be short | ~1e-5 |

- The main-string pitch: one player derived **Ma = 175 Hz (F3) for Sa = C**, 0.3048 mm steel, 0.9 m, giving
  ≈ 56 N [U: IMF forum]. Siddiq's measured sitar note was c, f0 ≈ 131 Hz [Sid12]. Wikipedia: the main string is a
  perfect fourth above the tonic, the second string the tonic [Wiki-S]. I take baaj = Ma = F#3 for Sa = C# [D from U].
- Tensions use T = μ(2Lf)², μ = ρπd²/4 (steel 7850, bronze 8800 kg/m³). B = π³Ed⁴/(64TL²) with E = 200 GPa
  (steel), 110 GPa (bronze) [D]. Fretting shortens L at constant T, so B grows as (L₀/L)²: ×4 at the octave.
- Steel 0.305 mm breaks near 170–180 N [D, assuming 2.4 GPa]: a fifth's meend (×2.25 → 137 N) runs close to it,
  which matches players' reports that a meend of a fifth needs a good string and sitar [U: IMF].
- Stretch stiffness EA = 14.6 kN for the baaj [D]. EA/T ≈ 240: a hard mizrab stroke gives an audible pitch drop
  (tension modulation, as in the bass).
- **Taraf tuning [U: chandrakantha, RJ]:** to the notes of the raga, usually a run of the scale from just below
  the middle Sa upward (e.g. Pa, Sa, then Re Ga Ma Pa Dha Ni Sa' Re' Ga' for an Yaman-like raga, with Ma# etc.),
  repeated or doubled on the important notes; the order and octaves vary by player and school. For Bilawal they
  are the major scale; for Kalyan the taraf Ma is raised from F to F# (Sa = C). The sympathetic ringing lingers
  after a meend has moved off the starting note, a recognisable cue the real instrument has and simple virtual
  sitars lack [ISMIR22].

**Losses (no measured sitar decay data found).** Use the tanpura's fitted steel-string loss [vWBM16] as the
template: α_i = σ0 + σ1 β_i + σ3 β_i³, β_i = iπ/L, with σ0 = 0.6 s⁻¹, σ1 = 6.5e-3 m/s, σ3 = 5.0e-6 m³/s (L = 1 m,
0.28 mm steel, 33 N, C3). That gives T60 [D]: 11 s (130 Hz), 7.2 s (1.3 kHz), 3.1 s (2.6 kHz), 1.3 s (3.9 kHz),
0.33 s (6.5 kHz), 0.085 s (10.5 kHz). Alternative physical form (air + viscoelastic + thermoelastic) from
[Iss16]: δ_ve = 0.0045, Q_te⁻¹ = 2.03e-4, η_air = 1.8e-5 kg/(m s), ρ_air = 1.2 kg/m³. The sitar's sounding decay
is shorter than the free string's, because the jawari pumps energy into upper modes that die quickly
[Iss16]: a main-string note rings ~3–6 s audibly [G]; the taraf ring on for 5–10 s [G].

### 1.2 The jawari: the bridge obstacle

**Physics.** The main bridge (*badaa goraa*: stag horn, camel bone, ebony or synthetic [Wiki-S]) has a wide,
slightly convex top. The string rests on its back (tail-side) edge and the surface falls away beneath it towards
the nut. As the string swings down it wraps onto the surface, which shortens its speaking length periodically and
puts a "kink" in it [Raman21, Vya09]. Consequences: the Young–Helmholtz rule fails (no missing harmonics at the
pluck point's nodes), energy flows from low to high partials, and a formant descends over the note [Sid12, Iss16].
**Stiffness is required**: the high-frequency part of each reflected wave travels faster and passes the contact
point before the low part closes the gap; with EI = 0 the precursor disappears [vWBM16, Sid12].

**Geometry (numbers).**

| Quantity | Value | Tag / source |
|---|---|---|
| Profile model | parabola from the grazing point: gap g(x) = a x² (x from the contact edge towards the nut) | [Vya09, Iss16] |
| Curvature from a measured sitar bridge | a = 0.5778 (m⁻¹), radius 1/(2a) = 0.87 m | [U: quoted by Iss16 from Vya09] |
| Drop of that profile at 5 / 10 / 15 / 20 mm | 0.014 / 0.058 / 0.13 / 0.23 mm | [D] |
| Luthier's practice for the melody string | the surface curved down ≈ 1 mm (parabolic) over the bridge top, a final cut ≈ 1.5 mm below the original edge | [U: Marcotty via Godden] |
| → a for "1 mm over 15–20 mm" | 2.5–4.4 m⁻¹ | [D] |
| Bridge length along the string (the jawari area) | 10–20 mm; "open" (khula) jawari (Ravi Shankar) longest, buzziest; "closed" (band) (Vilayat Khan) shortest, cleanest | [G] for mm; styles [U: Godden] |
| Synthetic choice used by Issanchou | a = 0.0065 m⁻¹ over 19 mm (deliberately flat so the whole length touches) | [Iss16] |
| Biwa bridge (Taguti's data, used by Kartofelev) | straight slope 0.008 rad over 15 mm; or slope to 10 mm then parabola R = 10 mm | [Kart13] |
| String height over the highest fret | ≈ 8 mm finished | [U: Marcotty] |
| Bridge material E (bone/horn) | 15–20 GPa | [G] |

For the model: the string grazes at x = 0 (the back edge); beyond the jawari length the bridge ends (no contact).
Typical played amplitudes are ~0.5–2 mm at mid-string [G], i.e. 10–40 µm within 10 mm of the end, the same
order as the drop of the profile: the string touches over a moving fraction of the bridge, which is the effect
[Vya09: "any degree of wrapping may occur during normal playing" U].

**Tanpura, the closest fully parameterised relative [vWBM16, Table 1]** (one string, C3, steel r = 0.14 mm):
L = 1.0 m, ρA = 4.83e-4 kg/m, T = 33.1 N, EI = 6.03e-5 N m²; bridge contact centred x_b = 10 mm, width
w_b = 2 mm, k_b = 4.39e8 N/m, unity exponent, lossless, height 0 (grazing); cotton thread at x_c = 4–8 mm,
w_c = 1.2 mm, k_c = 1.2e5 N/m, r_c = 1.2 kg/s. Earlier distributed version [vWC14]: L = 668 mm, d = 0.3 mm,
T = 31.47 N, EI = 8.35e-5, ρA = 5.58e-4, k_b = k_c = 1e8 N/m² (per length), bridge profile measured by Guettler,
apex at 46.5 mm. The sitar has no thread; its jawari plays the thread's role by its curvature.

### 1.3 Body

**No published sitar body measurement was found** (searched JASA/JSV/Applied Acoustics, theses). Construction
[Wiki-S, RJ]: a calabash gourd (*tumba*), 40–44 in (1.0–1.1 m) in circumference, i.e. ≈ 0.33 m diameter
[D], cut and closed by a flat wooden top (*tabli*, tun or teak, a few mm thick [G]), no sound hole [U]; often a
second small gourd on the neck. The bridge stands on the tabli.

Proposed body modes until measured (all **[G]**), by analogy with the tanpura's measured high-pass bridge
response [vWBM16 Fig. 5: it radiates weakly in the string's fundamental range 65–200 Hz] and a closed-box guitar:

| Mode | f (Hz) | Q | Note |
|---|---|---|---|
| tabli–air "breathing" (A0-like, closed gourd) | 180–250 | 15 | no port: weak |
| tabli first plate mode | 300–400 | 25 | |
| gourd shell / plate | 500–700 | 30 | |
| cavity standing wave across the gourd | ≈ 500 (c/2D, D ≈ 0.33 m) [D] | 40 | |
| higher plate modes, dense | 0.8–4 kHz | 20–40 | fill as a statistical set (as `bassbody.h` does above its measured modes) |
| neck gourd | 300–500 | 20 | small |

The audible result should be a high-pass: the jawari's precursor (1–5 kHz) dominates the radiated sound, and the
sound seems to *grow* over the first ~0.5 s even though the bridge force decays [vWBM16 §4.1, for the tanpura].

### 1.4 Excitation: mizrab, da/ra, meend

- **Mizrab**: a bent steel wire worn on the right index fingertip [LMM]; a hard, narrow, fast pluck. Contact width
  ~1 mm, release in ~0.1–0.3 ms [G] (vs ~1 ms for the bass's callused finger). Plucking point: over the joint of the
  neck and the tabli, ~0.2–0.3 L from the bridge [G].
- **Da / ra**: the two stroke directions, one towards the player's palm and one away. The naming convention is
  disputed (NIME-04 eSitar paper: "dha" up, "ra" down; a tutorial says the reverse) [U: KTC04, LMM]. Da is the
  stronger, main stroke [G]. Model: alternate the pull direction (in the plane of the top) and give da ~3 dB more
  depth [G]; both strokes drive the string mostly parallel to the bridge top, and the jawari is hit hardest by the
  component towards the bridge, so tilt the pull ~20–30° into the bridge [G].
- **Chikari strokes**: the same mizrab sweeps the chikaris for rhythm (*jhala*); a separate trigger.
- **Meend** (pulling the string sideways along the fret): the main string is pulled across the curved fret by up to
  a fifth (players: "5 steps", Wikipedia 7 semitones) [U: LMM, Wiki-S]. As tension: T ∝ f², so a semitone is +12 %,
  a fourth +78 %, a fifth +125 %. The pull needed [D, steel 0.305 mm, T₀ = 61 N, EA = 14.6 kN, pulled at the
  middle: ΔL = d²(1/(2a) + 1/(2b)), ΔT = EA ΔL/L]: 14 mm for a semitone, 20 mm for a tone, 35 mm for a fourth,
  45 mm for a fifth. Model as a tension ramp: the mode frequencies scale as √(T/T₀) (with B scaling as T₀/T), the
  speaking length fixed. Friction at the fret and bridge makes the speaking part's tension lag the pull a little
  [G]. Meend also changes the string's angle over the jawari slightly; ignore it.
- **Frets**: ~20 curved brass frets, movable (set to the raga), tied on with gut or nylon [RJ, LMM]; the string is
  stopped *between* frets by the fingertip and rests on the fret (a fret termination, no fingerboard under the
  string: the left finger presses the string onto the fret from above).

### 1.5 Keyboard mapping (sitar)

- Range of the main string: ~36 semitones (3 octaves) with stringing and meend [RJ]. With Sa = C#: baaj open F#3
  (MIDI 54) up ~2 octaves on the frets to ~F#5–C#6 (MIDI 78–85), plus meend [G]. Below F#3: the jod (C#3, MIDI 49),
  then the bass strings for the low Sa/Pa drones (C#2, G#2). Playable range for the model: **MIDI 49–84**.
- Mapping [G]: each key picks the main string (or jod below F#3) and a fret position; legato (a key pressed while
  another is held, within a fifth upwards) becomes a **meend** from the held note instead of a new stroke, at a rate
  from velocity or a fixed ~150–400 ms; the pitch wheel = meend (0 to +7 semitones, upward only; downward bends
  are done by releasing a pre-bent string). Alternate da/ra on successive notes. A pedal or low keys (below MIDI 49)
  strike the chikaris. A "raga" setting (or the last N distinct pitch classes played) retunes the 13 taraf.
- Typical decay: a single stroke sounds ~3–6 s with the taraf continuing [G]; sitar phrases are mostly
  many short strokes, so the jawari attack matters more than the tail.

---

## 2. The shamisen

### 2.1 Strings

Three strings, silk (professional) or nylon/tetron (students) [Wiki-Sh]. Body (*dō*) of four kwarin/rosewood
boards with skin front and back; neck of red sandalwood or kwarin [Oe11]. Types: hosozao (nagauta), chūzao (jiuta),
futozao (gidayū, tsugaru) [Wiki-Sh].

| String | Honchōshi | Niagari | Sansagari | Fuji-ito silk set (Oe/Kokubun Table 1-1) | μ [D] | T at C3 tuning, L = 0.80 m [D] | B [D, E_silk = 6 GPa G] |
|---|---|---|---|---|---|---|---|
| ichi no ito (1st, thick) | C3 130.8 | C3 | C3 | 17-1 (min'yō): 0.90 mm, 1.0 g per 1.128 m | 0.89 g/m | 39 N | 7.7e-5 |
| ni no ito (2nd) | F3 174.6 (+4th) | G3 196.0 (+5th) | F3 | 15-2: 0.65 mm, 0.5 g / 1.132 m | 0.44 g/m | 35 N (43 N niagari) | 2e-5 |
| san no ito (3rd, thin) | C4 261.6 (+8ve) | C4 | Bb3 233.1 (−1 tone) | 13-3 silk: 0.40 mm, 0.2 g / 1.186 m; nylon 0.50 mm, 0.3 g / 1.145 m | 0.17 (silk) / 0.26 (nylon) g/m | 30 N (silk), 46 N (nylon) | 4e-6 |

- Tunings (*chōshi*) are relative: honchōshi = 4th + octave, niagari = 5th + octave, sansagari = 4th + minor 7th
  [Oe11, Wiki-Sh, Bachido]. The absolute pitch is set by *honsū* (1–12, one semitone per step) to suit the singer
  [Oe11]; 1 hon ≈ A is commonly cited [U]. C–F–C is "the most common" honchōshi [Wiki-Sh]; Oe's recorded open
  1st string had its fundamental near 150 Hz [Oe11] (≈ D3). Default: ichi = C3, adjustable ±6 semitones.
- Nagauta set: 15-1 / 14-2 / 13-3; tsugaru: 28–30 / 15–16 / 13–14 (thicker 1st string) [Oe11, U: Bachido].
  Measured tsugaru diameters by a player: 0.83–0.85, 0.60–0.64, 0.38–0.40 mm [U: Bachido forum].
- **Scale length 0.80 m**: Kokubun simulated 800 mm [Kok12]; a tsugaru review states the bridge at 80 cm and
  position 10 at 40 cm [U: Thomann]. Overall instrument ~0.98–1.03 m [U]. The masses above are given to 0.1 g, so
  μ (and T) are uncertain by ±10–25 % [D].
- Silk is lossy and slightly irregular (twisted filaments): expect much shorter decays than steel, and strong
  high-frequency damping. No measured decay rates found. Start [G]: σ0 = 1.5 s⁻¹, σ1 = 0.02 m/s, σ3 = 3e-5 m³/s
  (T60 ≈ 3.5 s at 131 Hz, ≈ 0.6 s at 2 kHz). Oe/Kokubun's spectrograms: the sawari lengthens the 500–1500 Hz and
  > 3 kHz bands' decay; without sawari the tone decays smoothly [Oe11, Kok12].

### 2.2 The sawari: the nut obstacle

**Mechanism [Oe11, Kok12, Wiki-Saw].** The 2nd and 3rd strings pass over the upper nut (*kamigoma*). The 1st
string does not: it lies directly on the neck's end (lower than the others) and grazes a small ridge, the
**sawari-yama**, very close to the kamigoma. Plucking the 1st string makes it touch the ridge and buzz. When the
2nd or 3rd string (or a stopped note) is in tune with a harmonic of the open 1st string, the 1st string resonates
sympathetically and buzzes too: players use this to check tuning and finger positions [Oe11]. Two types:
*ichimonji-zawari* (a fixed ridge) and *azuma-zawari* (height adjustable by a screw) [Oe11].

| Quantity | Value | Tag / source |
|---|---|---|
| Ridge distance from the nut (kamigoma) | best sawari at 8–12 mm (simulated 2–20 mm); reference case 10 mm | [Kok12] simulation |
| Ridge height relative to the string's rest line | grazing: −0.05 … +0.05 mm all buzz; reference case −0.21 mm in the simulation's convention | [Kok12] |
| Too high a ridge | it becomes the nut: loud beating, fast decay ("120 %" sawari) | [Oe11, Kok12] |
| Contact length along the string | ~1–2 mm (a ridge, nearly a point) | [G] |
| Ridge material | neck wood (kōki / kwarin) or a metal insert (azuma) | [Oe11] |
| Contact stiffness | k ≈ (π/4) E* w; silk on wood E* ≈ 9.8 GPa → k ≈ 7.7e6 N/m for w = 1 mm | [D, Hertz line contact as in vWBM16 §3.2, E_silk 6 GPa G, E_wood 15 GPa G] |
| Spectral effect | adds/raises peaks around 1 kHz, 2.5–4.5 kHz, 4 and 6 kHz bands, a peak near 9.5 kHz | [Oe11, Kok12] |

This is the tanpura's two-point geometry (a grazing obstacle a few mm from the termination) and the vWBM16 point
model fits it directly (§3.4). Only the 1st string has it. The biwa (the source of the sawari idea) has a buzzing
surface at every fret; Taguti's biwa string: L = 0.8 m, μ = 0.375 g/m, T = 38.4 N, f0 = 200 Hz [Kart13 quoting
Taguti].

### 2.3 Body: skin and dō

Measured on one instrument by hammer tests [Kok12] (thesis, Hosei University):

| Part | Mode | f (Hz) | Damping |
|---|---|---|---|
| whole instrument | 1st bending | 67.6 | — |
| whole instrument | torsion | 85.4 | — |
| dō (body frame) | 1st bending | 569–573 | — |
| dō | torsion | 631–672 | — |
| skin (*kawa*), front | 1st | 151.7 | ζ = 1.02 % (Q ≈ 49, T60 ≈ 0.71 s [D]) |

- The recorded 1st-string spectra have their strongest peaks near the dō's 570 Hz mode and around 1 kHz,
  stronger than the fundamental [Oe11, Kok12].
- Higher skin modes [D, ideal square membrane from the measured f11 = 151.7 Hz, ratios √(m²+n²)/√2]: 240, 303,
  339, 387, 442, 455 Hz. The real skin is loaded by the cavity and the back skin, so these are only a start; the
  back skin is driven through the cavity ("the same mechanism as a drum") and energy flows more readily from the
  body to the skin than back; the skin's loss factor is roughly constant above 500 Hz [Suz17, abstract].
- Body size (tsugaru "go-bu dai"): ≈ 0.212 × 0.194 m face [U: Bachido]; the dō is ~0.08–0.09 m deep [G]. Cavity
  modes [D]: c/(2·0.21) ≈ 830 Hz, c/(2·0.19) ≈ 900 Hz.
- Skin: cat (finer instruments), dog (most, especially tsugaru), now often synthetic [Wiki-Sh]. Tension is very
  high (skins split in dry weather) [U]. No published tension; [G]: surface density 0.2–0.4 kg/m²; the measured
  f11 gives a membrane wave speed c = √2·a·f11 ≈ 45 m/s (a = 0.21 m) [D] → tension ≈ 400–800 N/m [D from G].
- The bridge (*koma*): height 3.2–3.6 (nagauta), 2.6–2.8 (jiuta), 2.6 (tsugaru) — presumably in *bu* (3.03 mm), i.e.
  8–11 mm [U: Wiki-Sh, units inferred]. Taller koma: louder, harder to control. A light koma on a membrane:
  model the string's bridge end as moving with the skin's drive point (the skin modes' shapes at the koma, ~1/4
  of the way up from the bottom edge of the face [G]).

### 2.4 Excitation: the bachi

- The bachi is a large wooden/ivory/plastic plectrum with a thin, flexible blade tip (tortoiseshell on tsugaru);
  jiuta's is the longest and widest, gidayū's the heaviest, tsugaru's the smallest [Wiki-Sh]. No published mass;
  [G]: 60–150 g, blade 6–9 cm wide at the edge, tip ~0.5 mm.
- It **strikes downward through the string onto the skin** (*tataki*): the pluck and a drum hit in one gesture; the
  skin "snap" stays at full volume even when the string is muted [U: Bachido, Wiki-Tsugaru]. Upstroke *sukui*
  plucks only the string; *hajiki* is a left-hand pluck (no skin hit).
- Proposed impulse **[G]**: the bachi edge moves at 1–3 m/s; the string is caught and slips off in ~0.2–0.5 ms (a
  hard, narrow pluck at ~0.1–0.15 L from the koma); 0.5–2 ms later the edge hits the skin 1–3 cm above the koma: a
  half-sine force of 0.5–1 ms, peak 20–100 N for tsugaru, much less for nagauta. Skin share of the radiated attack:
  ~50 % of the first 30 ms energy in tsugaru, ~10–20 % in nagauta/jiuta [G]. The skin hit excites the skin modes
  (151 Hz up) and the dō (570 Hz) with a broad click to ~5 kHz.

### 2.5 Keyboard mapping (shamisen)

- No frets; positions (*tsubo*) numbered by players; the neck covers about two octaves on each string [G]. Range
  with ichi = C3: **C3 (MIDI 48) to ~C6 (MIDI 84)** [G]; most melody sits on the 2nd and 3rd strings.
- Mapping [G]: notes below F3 on the 1st string (the only one with sawari), F3–C4 on the 2nd, above C4 on the 3rd
  (choose the open string when the key matches one). Velocity → bachi speed (and the skin share: soft notes become
  sukui-like, hard ones tataki). A held key with a new key a whole tone or less away: a slide (*suri*) on the same
  string. A chōshi switch (honchōshi/niagari/sansagari) and a honsū transposition.
- The 1st string should stay undamped and resonate (with its buzz) when other strings play notes in its harmonic
  series: give it the shared koma coupling and let it ring.
- Decay: a short, dry tone; audible ~1–2 s, the skin's 151 Hz mode gone in 0.7 s [D/G]. Tsugaru is played fast
  and percussively; the attack is the sound.

---

## 3. Implementing the contact in a modal model

### 3.1 State and contact points

Modes y_i(t), shapes v_i(x) = sin(iπx/L) (simply supported). The contact is evaluated at a few points or patches
x_c near the end from the modes: y(x_c) = Σ g_{c,i} y_i. Using a raised-cosine patch of width w instead of a point,
g_{c,i} = (π² sin(β_i x_c) cos(β_i w/2)) / (π² − β_i² w²) (with the limit (π/4) sin(β_i x_c) at β_i = π/w)
[vWBM16 eq. 20]: the same weights distribute the force back onto the modes, which keeps the scheme conservative.

**Mode count.** The buzz lives up to ~8–10 kHz and the contact patch must be resolved: vWBM16 used 133 modes (to
20 kHz) for a C3 string. For the sitar baaj at 185 Hz: ~100 modes to 20 kHz [D]; for the shamisen 1st string at
131 Hz, ~75 modes to 10 kHz would do given silk's loss [G]. The bass's 64 is too few for the sitar at F#3 (64 ×
185 Hz = 11.8 kHz, and a half-wavelength of 14 mm, the size of the jawari) [D].

### 3.2 The contact law

- Penalty force on the string, one-sided: F = K [η]₊^α, η = (obstacle height) − y (penetration). Issanchou et al.
  K = 1e13, α = 1.3 (DAFx-16) or 1.5 (JSV 2017/JASA 2018), but they then need Fs = 1–2 MHz to converge [Iss16,
  Iss18].
- **Real-time choice: α = 1, lossless, K = 1e8–5e8 N/m** per contact patch [vWBM16, vWC14], with K justified by
  Hertz line contact between a cylinder and a flat: K ≈ (π/4) E* w, E* = [(1−ν₁²)/(2E₁) + (1−ν₂²)/(2E₂)]⁻¹. Steel
  on bone: E* ≈ 36 GPa → 2.9e10 N/m per metre of contact, i.e. 2.9e7 N/m per mm [D]; vWBM16's steel–ivory value
  4.39e8 N/m for w_b = 2 mm. Compression stays below 3e-8 m (well below the string's 0.3 mm diameter).
- For the sitar's distributed jawari use 4–8 patches along 0–20 mm, each with its own height from g(x) = a x²
  [G]; vWBM16 collapse the distributed contact into one patch at a fixed centre, which loses the moving contact
  point; their later work suggests distributed contact plus tension modulation for the real effect [vWB16 EUSIPCO].
- Contact damping is usually omitted (the string's own high-mode losses suffice); Bridges & van Walstijn add a
  sideways friction (μ_k from a fitted atan, limit ~0.3–0.7 for smooth materials) for the second polarisation
  [BvW15].

### 3.3 Two polarisations

The jawari acts on the polarisation perpendicular to the bridge top only. The other polarisation sees only
friction [BvW15] and the ordinary bridge admittance; the string's whirling moves energy between them, so a stroke
parallel to the top still buzzes after a few periods [BvW15, G]. This matches the bass's y/z split exactly.

### 3.4 Energy-stable update (vWBM16, recommended)

Per mode, with modal mass m = ρAL/2, exact frequency ω_i and decay α_i [vWBM16 eq. 33]:
R_i = exp(−α_i Δt), Ω_i = cos(ω_i Δt),
**a*_i = (1 − 2R_iΩ_i + R_i²)/(1 + 2R_iΩ_i + R_i²)**, **b*_i = 2(1 − R_i²)/(1 + 2R_iΩ_i + R_i²)**
(check: for small Δt, a* → k_iΔt²/(4m), their uncorrected a_i). With the diagonal matrices A = diag(a*), B = diag(b*),
J = I + A + B, ξ = Δt²/(2m), the update (eqs. 38–48) is:
1. Scaled momentum q_i = (Δt/2m) p_i. A per-step vector ū = J⁻¹[2(q − A y) + ξ g_e F_e] — all componentwise.
2. Project onto the contact: u = g_bᵀū, θ = g_bᵀ J⁻¹ ξ g_b (a precomputed scalar for a fixed patch).
3. The contact force from the two-point discrete gradient F = −(V(y⁺) − V(y))/(y⁺ − y), V = (K/2)[h − y]₊²: with
   α = 1 the scalar equation in the step s_b = y_b⁺ − y_b has a closed-form solution in four cases (vWBM16 Fig. 4)
   — no iteration.
4. s̄ = ū + J⁻¹ξ g_b F_b; y⁺ = s̄ + y, q⁺ = s̄ − q.
Exact modal frequencies and damping; unconditionally stable (energy can grow only through the pluck); only
requirement: no mode above Nyquist. At 44.1 kHz the bridge signal is slightly too strong above 15 kHz;
**2× oversampling (88.2 kHz) makes the difference almost inaudible** [vWBM16 §4.2]. Cost in Matlab: < 2.5 s per
second of sound for any practical string, more than half in step 1; contact solving negligible — easily real-time
in C [vWBM16 §4.3]. With several contact patches, step 3 becomes a small coupled system (a few × few matrix,
LCP-like), or treat patches with the scalar solution sequentially (not exactly conservative) [G].
The bass's explicit penalty spring would need tiny K (soft contact) to stay stable at these stiffnesses; the
vWBM16 form is the one to copy.

### 3.5 Pluck shape for gentle strokes

vWBM16 eq. 14–15: F_e(t) = a_e sin²[(π/τ_e) h_e(t − t_e)] over τ_e, with h_e(t) = ½(t + τ_e sinh(β_e t/τ_e)/sinh β_e);
β_e from 3 (smooth) to 30 (abrupt). Their test pluck: a_e = −0.4 N, τ_e = 50 ms, β_e = 30, over the whole string.
For a mizrab or bachi use τ_e of 0.2–1 ms [G] at a point patch (w ≈ 1–3 mm).

---

## 4. Published synthesis models and their numbers

| Model | Method | Instrument | Key numbers |
|---|---|---|---|
| van Walstijn, Bridges, Mehes 2016 [vWBM16] | modal, energy-stable, one bridge patch + thread spring | tanpura | Table 1 above; 133 modes; 44.1 kHz works, 88.2 kHz recommended; 4-string drone G2–C3–C3–C2; 2048-tap FIR body filters (bridge and nut) from hammer measurements |
| van Walstijn & Chatziioannou 2014 [vWC14] | FD, distributed contact | tanpura | above; Δt = 1/176.4 kHz, Δx = 3⅓ mm, Δx_b = 0.18 mm |
| Bridges & van Walstijn 2015 [BvW15] | FD, 2 polarisations, bridge friction | tanpura | L = 0.628 m, μ = 5.58e-4 kg/m, T = 31.47 N, EI = 8.35e-5; thread–bridge distance 2.5/4/5 mm compared |
| Issanchou et al. 2016 [Iss16] | modal + conservative penalty | point (tanpura), parabola (sitar) | L = 1.002 m, T = 180.5 N, μ = 1.17e-3 kg/m, B = 1.78e-5; obstacle at 6 mm; K = 1e13, α = 1.3; pluck 1–1.8 mm; needs Fs ≥ 1 MHz (2 MHz used) |
| Issanchou et al. 2017/2018 [Iss17, Iss18] | modal + penalty / nonsmooth (Moreau–Jean) | point obstacle at 6 mm; bass fretboard | K = 1e13, α = 1.5; validated against measurements |
| Siddiq 2012 [Sid12] | waveguide + FD patch at the bridge (Krishnaswamy–Smith coupling) | sitar | allpass cascade for dispersion; FD loss b = 0.97, d = 5e-5 (Karjalainen form); no geometry numbers |
| Vyasarayani, Birkett, McPhee 2009 [Vya09] | PDE with wrapping region | sitar | parabolic bridge, a = 0.5778 (quoted by Iss16) [U] |
| Mandal & Wahi 2015 [MW15] | modal analysis of a string on a boundary obstacle | sitar, veena | full text not obtained |
| Kartofelev et al. 2013, 2017 [Kart13, Kart17] | kinematic d'Alembert | biwa, generic | biwa L = 0.8 m, μ = 0.375 g/m, T = 38.4 N; slope 0.008 rad, R = 10 mm; 325 kHz |
| Taguti 2008, 2017 [Tag08, Tag17] | FD, unilateral constraint; + resonator + stick-slip pluck | biwa (sawari) | sawari as large as the real one, pluck at a specific point; numbers in the full text only |
| Kokubun 2012 [Kok12] | FD ideal string, c = 100 m/s, L = 800 mm, Δx = 2 mm, Δt = 0.01 ms | shamisen 1st string | sawari 2–20 mm from the nut, height ±0.05 mm |
| STK `Sitar` [STK] | Karplus–Strong | sitar | delay jittered ±5 % per note, loop gain 0.995 + 5e-7 f (≤ 0.9995), loop zero 0.01, envelope 1 ms / 40 ms — no buzz mechanism, only a noisy pitch |
| Bilbao & Torin 2014 [BT14] | FD, energy-conserving barrier | fretboard | the general collision framework behind the above |

---

## 5. Summary of guesses to replace by measurement

Sitar: the main string's exact octave/tension (from one forum calculation and Siddiq's 131 Hz note), chikari
and taraf lengths, the jawari length and curvature for a real bridge (one quoted value, a = 0.58 m⁻¹, and a
luthier's "1 mm drop"), **all body modes**, mizrab contact time, decay times. Shamisen: skin tension/density,
higher skin modes (only f11 measured), the bachi's mass and force, the skin share of the attack, silk losses, the
honsū reference pitch. The best next sources: Vyasarayani 2009 and Mandal & Wahi 2015 full texts (sitar bridge
geometry), Valette & Cuesta 1993, Suzuki 2017 full text (shamisen SEA loss factors), Taguti 2017 full text (biwa
sawari size), C. V. Raman 1921, and an impact-hammer measurement of a sitar bridge (as vWBM16 did for the tanpura).

---

## References

[Bachido] Bachido shamisen FAQ and forum: tunings https://bachido.com/faq-items/three-main-tunings ; string
diameters https://community.bachido.com/t/what-are-the-diameters-of-shamisen-ito-strings/8180 ; body sizes
https://community.bachido.com/t/dou-sizes/3223 ; bachi striking https://community.bachido.com/t/how-hard-do-you-strike-with-your-bachi/3939
[BT14] S. Bilbao, A. Torin, "Numerical simulation of string/barrier collisions: the fretboard", Proc. DAFx-14,
Erlangen (2014); archived at https://dafx.de/paper-archive/ (not read here)
[BvW15] J. Bridges, M. van Walstijn, "Investigation of tanpura string vibrations using a two-dimensional
time-domain model incorporating coupling and bridge friction", Proc. Third Vienna Talk (2015).
https://viennatalk2015.mdw.ac.at/proceedings/ViennaTalk2015_submission_72.pdf
[Godden] B. Godden, "How to do sitar and tamboura jawari" (excerpts, incl. W. Marcotty "Djovari").
https://pdfcoffee.com/sitar-jawari-pdf-free.html
[IMF] Indian Music Forum archive, "string tension on indian instruments". https://chandrakantha.com/archives/2/0112.html
[ISMIR22] "Differentiating the pitch bend function between the sitar as a real and virtual instrument", ISMIR 2022
LBD. https://archives.ismir.net/ismir2022/latebreaking/000022.pdf (abstract only)
[Iss16] C. Issanchou, J.-L. Le Carrou, S. Bilbao, C. Touzé, O. Doaré, "A modal approach to the numerical simulation
of a string vibrating against an obstacle: applications to sound synthesis", Proc. DAFx-16, Brno, 167–174.
https://dafx.de/paper-archive/2016/dafxpapers/24-DAFx-16_paper_44-PN.pdf
[Iss17] C. Issanchou, S. Bilbao, J.-L. Le Carrou, C. Touzé, O. Doaré, "A modal-based approach to the nonlinear
vibration of strings against a unilateral obstacle: simulations and experiments in the pointwise case", J. Sound
Vib. 393, 229–251 (2017). https://perso.ensta-paris.fr/~doare/files/issanchou_jsv2017.pdf (not reachable; numbers via Iss18)
[Iss18] C. Issanchou, V. Acary, F. Pérignon, C. Touzé, J.-L. Le Carrou, "Nonsmooth contact dynamics for the numerical
simulation of collisions in musical string instruments", JASA 143(5), 3195–3205 (2018).
https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A24_Issanchou_NonsmoothContactDynamics.pdf
[Kart13] D. Kartofelev, A. Stulov, H.-M. Lehtonen, V. Välimäki, "Modeling a vibrating string terminated against a
bridge with arbitrary geometry", Proc. SMAC 2013, 626–632. https://www.tud.ttu.ee/web/dmitri.kartofelev/publications/smacsmc2013.pdf
[Kart17] D. Kartofelev, "Kinematics of ideal string vibration against a rigid obstacle", Proc. DAFx-17.
https://www.tud.ttu.ee/web/dmitri.kartofelev/publications/DAFx17_Kartofelev.pdf
[Kok12] 國分圭介 (K. Kokubun), 三味線の振動と音響 (Vibration and acoustics of the shamisen), MSc thesis, Hosei
University (2012). https://hosei.ecats-library.jp/da/repository/00008389/%E5%9C%8B%E5%88%86%E5%9C%AD%E4%BB%8B.pdf
[KTC04] A. Kapur, A. Lazier, P. Davidson, R. S. Wilson, P. R. Cook, "The electronic sitar controller", Proc. NIME-04.
https://soundlab.cs.princeton.edu/publications/esitar_nime_2004.pdf (stroke names via search snippet only)
[LMM] Last Minute Musicians, "Learning the sitar". https://www.lastminutemusicians.com/blog/learning-the-sitar/
[MW15] A. K. Mandal, P. Wahi, "Natural frequencies, modeshapes and modal interactions for strings vibrating against
an obstacle: relevance to sitar and veena", J. Sound Vib. 338, 42–59 (2015). (not obtained)
[Oe11] 大江耕司 (K. Oe), 三味線のさわり機構による音響・振動特性 (Sound and vibrational property with sawari mechanism
of shamisen), MSc thesis, Hosei University (2011). https://hosei.ecats-library.jp/da/repository/00007716/%E5%A4%A7%E6%B1%9F%E8%80%95%E5%8F%B8.pdf
[Raman21] C. V. Raman, "On some Indian stringed instruments", Proc. Indian Assoc. Cultivation Sci. 7, 29–33 (1921). (not read)
[RJ] Ragajunglism, "Sitar". https://ragajunglism.org/ragas/instruments/sitar/
[Sid12] S. Siddiq, "A physical model of the nonlinear sitar string", Archives of Acoustics 37(1), 73–79 (2012).
https://journals.pan.pl/Content/101574/PDF/10_paper.pdf
[STK] P. R. Cook, G. P. Scavone, STK `Sitar.cpp`. https://github.com/thestk/stk/blob/master/src/Sitar.cpp
[Suz17] Y. Suzuki et al., "Vibration energy propagation analysis of shamisen by experimental statistical energy
analysis", Proc. JSME Dynamics & Design Conf. 2017, 403. https://www.jstage.jst.go.jp/article/jsmedmc/2017/0/2017_403/_article/-char/en (abstract only)
[Tag08] T. Taguti, "Dynamics of simple string subject to unilateral constraint: a model analysis of sawari
mechanism", Acoust. Sci. Tech. 29(3), 203–214 (2008). https://www.jstage.jst.go.jp/article/ast/29/3/29_3_203/_article (abstract only)
[Tag17] T. Taguti, "Numerical simulation of the sawari tone on Chikuzen biwa", Acoust. Sci. Tech. 38(5), 237–245
(2017). https://www.jstage.jst.go.jp/article/ast/38/5/38_E1633/_article/-char/en (abstract only)
[Thomann] Zenon Tsugaru 11 shamisen user review (scale 80 cm). https://www.thomann.co.uk/zenon_tsugaru_11_shamisen_reviews.htm
[Vya09] C. P. Vyasarayani, S. Birkett, J. McPhee, "Modeling the dynamics of a vibrating string with a finite
distributed unilateral constraint: application to the sitar", JASA 125(6), 3673–3682 (2009), doi:10.1121/1.3123403.
https://hal.archives-ouvertes.fr/hal-01525495 (blocked; not read)
[vWB16] M. van Walstijn, J. Bridges, "Simulation of distributed contact in string instruments: a modal expansion
approach", Proc. EUSIPCO 2016. (not read)
[vWBM16] M. van Walstijn, J. Bridges, S. Mehes, "A real-time synthesis oriented tanpura model", Proc. DAFx-16,
Brno, 175–182. https://pureadmin.qub.ac.uk/ws/files/94023894/25_DAFx_16_paper_08_PN.pdf ; sounds
https://sandormehes.github.io/dafx16.html
[vWC14] M. van Walstijn, V. Chatziioannou, "Numerical simulation of tanpura string vibrations", Proc. ISMA 2014,
Le Mans. https://pureadmin.qub.ac.uk/ws/files/10528686/isma14a.pdf
[Wiki-S] Wikipedia, "Sitar". https://en.wikipedia.org/wiki/Sitar
[Wiki-Saw] Wikipedia, "Sawari" (after L. Sakata, "The comparative analysis of sawari on the shamisen",
Ethnomusicology 10(2), 141–152, 1966). https://en.wikipedia.org/wiki/Sawari
[Wiki-Sh] Wikipedia, "Shamisen". https://en.wikipedia.org/wiki/Shamisen
[Wiki-Tsugaru] Wikipedia, "Tsugaru-jamisen". https://en.wikipedia.org/wiki/Tsugaru-jamisen
[chandrakantha] Chandrakantha, sitar tuning for beginners. https://chandrakantha.com/?p=12339
