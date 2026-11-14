# Tinkle bell, agogo, woodblock, reverse cymbal: modal models for four GM percussive programs — research notes

Status: 2026-11-14. Research only. Nothing here is implemented.
**[G]** marks my estimates or guesses, not published values. **[D]** marks values I derived from published
formulas or data (the derivation is given). **[U]** marks a claim taken from a secondary source, an abstract or a
synthesis program's tuned-by-ear table, which I could not check in a primary measurement. References are in brackets;
the list is at the end.

The four programs are GM 113 Tinkle Bell, 114 Agogo, 116 Woodblock and 120 Reverse Cymbal, all in the GM
"Percussive" group [WGM]. GM2 adds one variation among them: 116 bank 1 = Castanets [WGM2]. All four are *melodic*
programs: every key sounds, and the pitch follows the key. This differs from the drum channel, where key 67/68 are
high/low agogo and 76/77 high/low woodblock [PGM].

Short answer:
- **Tinkle Bell → a small tuned English handbell.** The (2,0) mode is the note. The (3,0) mode is tuned to 3:1, a
  twelfth above. Above those is an inharmonic cluster at 4.8–15× [RP via H13]. The doublets beat slowly. The
  clapper is hard. Handbells really exist across most of the keyboard, so the model is physical on every key.
- **Agogo → two thin conical iron shells**, with Rayleigh-like (m,0) ratios of about 1 : 2.8 : 5.4 : 8.8 [D]. The
  mouth is oval, so the doublet splits are wide, and that gives the agogo its rough clank. The stick contact is hard
  and short, ~0.1 ms [D].
- **Woodblock → one cross-grain wooden lip (a cantilever plate) over a Helmholtz cavity.** Both decay in tens of
  ms. Wood loss factor η ≈ 0.01 gives Q ≈ 100 [G]. Measured temple blocks are 425–831 Hz [W11].
- **Reverse cymbal → a modal cymbal bank run with *negative* damping**: a time-reversed exponential is a growing
  exponential. The bank starts from tiny random amplitudes, swells for T seconds and is cut. An offline render
  played backwards is the alternative.

---

## 0. Common machinery

All four are modal banks: a sum of two-pole resonators, each struck through a contact force. The drum plan's mallet
model (Hunt–Crossley, `drums.md` §4.4) serves all of them.

| Quantity | Formula | Note |
|---|---|---|
| Decay of mode k | τ_k = Q_k/(π f_k) (amplitude, 1/e); T60 = 6.91 τ | Q = 1/η for a material loss factor η |
| Hertz contact time (sphere on a half-space) | t_c ≈ 2.87 (m²/(R E*² v))^{1/5} | m = effective striker mass, R = tip radius, v = speed, 1/E* = Σ(1−ν²)/E |
| Contact spectrum | force pulse ≈ half-sine of length t_c; flat to ~1/(2 t_c), falls fast above ~1/t_c | Shorter contact = brighter |
| Strike-point weight | a_k ∝ φ_k(x_strike) · φ_k(x_mic or radiation) | For rings/shells, cos(mθ) and sin(mθ) for the two members of a doublet |
| Doublet | each (m,n) mode with m ≥ 1 is a pair at f(1 ± δ/2) when the ring is not perfectly round | The two members excited as cos²(mθ_s), sin²(mθ_s) of the strike angle relative to the pair's axes [KT] |

Hertz contact times for typical strikers, computed from the formula above **[D]**. They hold for a rigid half-space.
A thin shell adds local compliance, so real contacts are longer, perhaps by up to 2× **[G]**.

| Striker → target | m (kg) | R (mm) | E* (GPa) | v (m/s) | t_c |
|---|---|---|---|---|---|
| Hickory stick tip → iron (agogo) | 0.015 | 6 | 12.4 | 2 | **0.12 ms** |
| Stick tip → bronze cymbal | 0.015 | 6 | 11.9 | 3 | **0.11 ms** |
| Stick tip → wood, cross-grain (block) | 0.015 | 6 | 1.5 | 2 | **0.28 ms** |
| Brass/bronze ball clapper → bronze bell | 0.010 | 6 | 59 | 0.5 | **0.07 ms** |
| Hard plastic clapper → bronze bell | 0.020 | 12 | 3.4 | 1 | **0.22 ms** |

Materials used: wood E = 12 GPa along the grain, ~1.5 GPa across it; iron E = 200 GPa; bronze E = 110 GPa,
ρ = 8600 kg/m³ **[G]** (handbook-typical values). These times grow slowly with softness, as t_c ∝ v^−1/5. A velocity
range of 0.3–5 m/s changes t_c by a factor of 1.8 **[D]**. Most of the velocity → brightness change therefore comes
from mallet *hardness* (stiffening with compression, Hunt–Crossley exponent α ≈ 1.5–3 **[G]**). Speed matters less.

---

## 1. GM 113 Tinkle Bell

### 1.1 What it is in GM sets
- GM lists it simply as "Tinkle Bell" (Percussive group) [WGM]. Sound On Sound's survey of GM/GS/XG exotic sounds
  dismisses it ("Tinkly bells are no great shakes") and suggests the Javanese bonang/gender as better bells [SOS-ex].
- I found no description of the SC-55 sample's timbre. From memory of GM modules, it is a bright, small struck
  bell with a long ring, a high register and a strong inharmonic shimmer: a small handbell or tuned bell-plate rather
  than sleigh bells **[G]**. Sleigh bells (jingles) are a GM drum-map sound (key 83 in GS/GM2), not this program **[U]**.
- **Choice: a small English handbell (bronze), struck by its internal clapper.** Reasons: (a) it is tuned, so it has
  a clear note on every key; (b) its mode set is measured [RP via H13]; (c) handbells really span most of the
  keyboard, with bass handbells around C2–C3 [RM93], so low keys stay physical. Alternatives are the church-bell set
  (§1.3) and a glass-bell/bell-plate (§1.6).

### 1.2 Handbell modes (measured, C5 handbell; Rossing, Perrin, Sathoff & Peterson 1984)
The (m,n) labels: m = nodal meridians/2 (the ring's circumferential order), n = nodal circles. Frequencies are
relative to (2,0). Holograms and ratios are reproduced in Hansen 2013, Fig. 2 [H13, from RP87].

| Group | Mode | f / f(2,0) | Note |
|---|---|---|---|
| 0 | (2,0) | **1.0** [U] | the note (the "fundamental", tuned) |
| 0 | (3,0) | **3.0** [U] | tuned to a twelfth (English tuning) [RS80, RM93] |
| I | (3,1) | **4.8** [U] | |
| I | (4,1) | **5.4** [U] | (4,0) is missing in handbells: replaced by (4,1) [H13] |
| II | (4,1#) | **5.9** [U] | circle very near the mouth |
| I | (2,1) | **7.4** [U] | |
| I | (5,1) | **7.6** [U] | |
| II | (5,1#) | **9.0** [U] | |
| I | (6,1) | **10.2** [U] | |
| III | (4,2) | **10.4** [U] | |
| III | (3,2) | **11.7** [U] | |
| III | (5,2) | **12.0** [U] | |
| II | (6,1#) | **12.6** [U] | |
| III | (6,2) | **15.2** [U] | |
| III | (2,2) | **15.3** [U] | |

- Some makers tune the second mode to **2.4×** (a minor-third bell) instead of 3× [US6915756].
- The (2,0) radiates a strong **second harmonic** (2 f₁), and the (3,0) a strong sixth harmonic (6 f₁ = 2 f(3,0)). This
  is radiation at twice each mode's frequency, attributed to parametric/nonlinear radiation [RS80, RM93 abstracts, U].
  Model: add a small term ∝ x_k² per strong mode at the output (amplitude ~ −20 to −30 dB re. the mode at mf **[G]**),
  or skip it.
- Small bells radiate (2,0) poorly: a quadrupole source, small compared with λ [Mansell thesis abstract, U]. So weight
  the radiated amplitude ∝ (ka)^m approximately, or more simply ∝ f² below ~1 kHz **[G]**.

### 1.3 Church-bell alternative (if a "bell" colour is wanted)
| Partial | Hum | Prime (strike) | Tierce | Quint | Nominal | Superquint | Octave nominal |
|---|---|---|---|---|---|---|---|
| f / prime | 0.5 | 1 | 1.2 (minor third) | 1.5 | 2 | ~3 | ~4 |
| Mode | (2,0) | (2,1#) | (3,1) | (3,1#) | (4,1) | (5,1) | (6,1) |

Ratios: the Hemony design 1:2:2.4:3:4 relative to the hum [HB-p2381, U]; upper partials and mode labels from the
church-bell literature [PCD82, U]. The strike note is perceived near the prime/nominal/2, from the nominal and the
upper partials. Decay order: the hum and prime ring longest, the upper partials die within ~1 s [HB-p2381, U].

### 1.4 Doublets and beating
- A bell that is not perfectly round splits every m ≥ 1 mode into two, at slightly different frequencies [HB-dbl].
- Measured on a 19.25-in church bell (1855): hum beats at **1 Hz**, tierce at **2 Hz**, prime at **4.5 Hz**
  (851.5 / 856 Hz, i.e. a split of 0.53 %) [KT, U].
- The strike angle sets the mix. At four positions 90° apart (for m = 2) one member is at maximum and the other at
  zero. 45° away the roles swap; in between, both sound and beat [KT].
- **Handbell recipe [G]:** tuned handbells are lathe-turned, so they are rounder than cast church bells. Use a split
  δ = 0.05–0.2 % for (2,0) and (3,0) (beats of 0.3–3 Hz at C5–C6) and 0.2–1 % for the upper cluster. Draw them
  randomly per voice instance (each "bell" in the instrument is a different bell), fixed per key. The strike angle
  relative to the doublet axes is random per note in a small range, because a ringer's clapper hits nearly the same
  spot.

### 1.5 Decay and excitation
| Mode | Q | τ at C6 (1047 Hz) | Note |
|---|---|---|---|
| (2,0) | 5000–10000 **[G]** | 1.5–3 s **[D from Q]** | the long hum of the note |
| (3,0) | 3000–6000 **[G]** | 0.3–0.6 s | |
| (m,1), (m,1#) | 1500–3000 **[G]** | 0.05–0.25 s | the "clang" after the strike |
| (m,2) | 1000–2000 **[G]** | < 0.05 s | |

- The handbell ratios are measured. The Q values are not; I found no published handbell decay times. The values are
  set so that a C5–C6 handbell rings audibly for ~5–10 s (T60 of (2,0)) **[G]**, as a handbell does when it is not
  damped. Bronze's material loss factor is ~1e-4 or less **[G]**, so radiation and the handle/mounting dominate.
- **Scaling with key:** a geometric scale-down by s multiplies all f by 1/s and keeps the Q values, so τ ∝ 1/f
  **[D]**. Above ~C7, use a floor of τ(2,0) ≥ 0.6 s **[G]** so that the top keys still "tinkle".
- **Excitation:** the clapper head (hard plastic, rubber or felt-covered on modern handbells **[G]**) strikes the
  rim (soundbow) from inside. Use t_c ≈ 0.1–0.3 ms (§0). A hard clapper excites up to ~5 kHz, so the 5.4–15× cluster
  is strong at C5 and above ~C7 is above hearing **[D]**. A rim strike excites every (m,n) at its antinode, so the
  amplitudes are set mostly by the modal masses and radiation.
- **Starting amplitudes at the rim, for the radiated sound [G]:** (2,0) 1.0, (3,0) 0.5, (3,1) 0.15, (4,1) 0.25,
  (4,1#) 0.12, (2,1) 0.08, (5,1) 0.12, (5,1#) 0.06, (6,1) 0.06, the rest 0.03 each. The (2,0) is attenuated by
  the radiation weight in §1.2 for keys below ~C5.
- A real handbell is struck by a sweeping arm, and the ringer may "echo" or "martellato" (hit on the table pad: a
  quickly damped note). The note-off can model the pad or hand damping: multiply all damping by 30–100× **[G]**.

### 1.6 Glass-bell / bell-plate option
If the patch should be more "glassy": a free flat plate or a thin tube. The plate's modes are inharmonic (around
1 : 1.5 : 1.9 : 2.6 for the low free-plate modes **[G]**). This is no better grounded than the handbell, so the
handbell is the recommendation.

### 1.7 Key mapping
- Key k → f(2,0) = 440·2^((k−69)/12) **[G]**. It is reasonable for GM files, which use Tinkle Bell for melodies in
  the 4th–6th octaves. Some modules may sound an octave higher **[G]**; check by ear against a GM file.
- The whole bell scales with the key (§1.5), and the mode ratios are kept. Bass handbells (C2–C3) radiate the
  fundamental poorly and have strong 2nd/6th harmonics [RM93]. Keep that physics for low keys.

---

## 2. GM 114 Agogo

### 2.1 The object
- **Agogô:** two (sometimes three) conical, black iron bells joined by a bent iron rod. African in origin; the larger
  forerunner is the gankogui [SOS-ex]. The GM module guide lists it as C4–C5 in pitch, usually in pairs of one large
  and one small bell [SH, U].
- Each bell is a thin welded sheet-iron cone, open at the wide mouth. The mouth is often flattened to an oval. The
  stick hits near the mouth, on the outside **[G]**.
- **Interval between the bells:** a fourth to a major sixth in practice; typical pairs are near a fourth or fifth
  **[G]**. I found no measurements of real agogos (frequencies or decays). The numbers below are derived or guessed.

### 2.2 Modes
A thin conical shell with a closed apex, open mouth: the low modes are the inextensional (m,0) ring-like modes.
Rayleigh's inextensional ring ratio f ∝ m(m²−1)/√(m²+1) gives:

| Mode | (2,0) | (3,0) | (4,0) | (5,0) | (6,0) | (7,0) |
|---|---|---|---|---|---|---|
| f/f₁, inextensional ring **[D]** | 1 | 2.83 | 5.42 | 8.77 | 12.87 | 17.71 |
| Handbell measured, for comparison [H13] | 1 | 3.0 (tuned) | 5.4 (4,1) | 7.6 (5,1) | 10.2 (6,1) | — |
| STK `ModalBar` "Agogo" preset (synthesis, by ear) [STK] | 1 | 4.08 | 6.669 | fixed 3725 Hz | | |
| CR-8000 cowbell (analog synth reference) [SOS-cb] | 587 Hz | 845 Hz (×1.44) | | | | |

- The handbell's measured ratios follow the inextensional law closely (2.83 vs 3.0, 5.42 vs 5.4). An untuned
  cone of near-uniform wall should do the same, so **use 1 : 2.83 : 5.42 : 8.77 : 12.9** plus (m,1) modes at roughly
  1.5–2.5 × the (m,0) of the same m **[D/G]**.
- The STK preset (1 : 4.08 : 6.67 plus a fixed 3725 Hz partial) is tuned by ear [STK, U]. It is a useful sanity
  check: it sounds like an agogo, with sparse inharmonic partials and a fixed high "clank" partial that does not
  follow the pitch.
- The 2-oscillator cowbell (1 : 1.44) suggests that a cowbell's first two strong partials are close [SOS-cb, U]. For
  a rectangular cowbell these are probably the two plates' modes. For an oval-mouth cone they are the two members of
  a widely split (2,0) doublet **[G]**. So: **an oval mouth splits (2,0) strongly.**

### 2.3 Doublets (the clank)
| Mode | Split δ | Beat at f₁ = 800 Hz | Note |
|---|---|---|---|
| (2,0) | 2–8 % **[G]** | 16–64 Hz → roughness, not beats | oval mouth; a large split (up to 1.44×, two-tone) for a flattened cowbell-like mouth |
| (3,0) | 0.5–2 % **[G]** | 11–45 Hz | |
| (m ≥ 4) | 0.3–1 % **[G]** | | the seam and hand-forged irregularity |

Splits above ~15 Hz are heard as roughness. That is the main difference between an agogo and a handbell. The strike
direction relative to the oval sets the mix, cos²(mθ) and sin²(mθ). A player hits roughly the same spot, so use a
fixed θ_s ≈ 20–30° **[G]**. Both members then sound, the one on the hit axis louder.

### 2.4 Decay and damping
| Mode | Q free **[G]** | τ, high bell f₁ = 900 Hz | τ, low bell f₁ = 650 Hz |
|---|---|---|---|
| (2,0) | 600–1200 | 0.2–0.4 s | 0.3–0.6 s |
| (3,0) | 500–1000 | 60–120 ms | 85–170 ms |
| (4,0)+ | 300–600 | 20–40 ms | 25–55 ms |

- The welded seam, the rod joint and the thin sheet give a lower Q than cast bronze **[G]**. STK uses pole radius
  0.999 at 44.1 kHz, i.e. τ = 1/(f_s(1−r)) = 23 ms for every mode **[D from STK]**. That is shorter than a free bell,
  but STK's agogo is a "struck and held" sound **[G]**.
- **Hand damping:** the player holds the rod, so the bells ring freely by default. Players mute by pressing a bell
  against the body or squeezing the two together, which also gives a "click" stroke **[G]**. Model as a damping
  multiplier ×10–30 on all modes (τ → 10–30 ms), driven by a controller (aftertouch or CC) **[G]**.

### 2.5 Excitation
- A wooden stick (hickory or bamboo) on iron: t_c ≈ 0.1–0.2 ms (§0) **[D]**, with a force spectrum flat to ~3–5 kHz.
  At ff the contact gets shorter and the high modes louder.
- Strike near the mouth: the (m,0) modes have antinodes there, so all are excited. Starting amplitudes **[G]**:
  (2,0) 1.0, (3,0) 0.6, (4,0) 0.4, (5,0) 0.25, (6,0) 0.15, (m,1) 0.1 each; a fixed "clank" partial near 3.5–4 kHz
  at 0.2 (cf. STK's 3725 Hz) that does *not* scale with the key above C5 **[G]**.

### 2.6 Key mapping
- GM sets play one agogo bell pitched across the keyboard (C4–C5 nominal range) [SH, U]. Physical choice: key k sets
  the struck bell's f₁ = 440·2^((k−69)/12) **[G]**. The size scales with the key, τ ∝ 1/f (§1.5), and the fixed
  clank partial is kept below Nyquist.
- An option closer to a real agogô: odd/even keys or a split point alternate high/low bells of a fixed pair a fifth
  apart **[G]**. Not GM practice.

---

## 3. GM 116 Woodblock

### 3.1 The objects
| Object | Shape | Sounding parts |
|---|---|---|
| Orchestral (Chinese) woodblock | a rectangular hardwood block (~18 × 7 × 5 cm **[G]**) with a deep slot sawn along its length under the top face, on one or both sides | the thin **lip** above the slot (bends across the grain) + the **slot cavity** (Helmholtz) |
| Temple block (mokugyo, "wooden fish") | a hollow, roughly spherical carved block with a wide slit mouth | the shell + the internal **cavity** (Helmholtz), coupled like a bass-reflex loudspeaker [Y05] |

### 3.2 Measured data
- **Mokugyo (Yuasa et al. 2005, 176 instruments, Ø 7.5–120 cm):** two peaks close together, average ratio
  **F2/F1 = 1.15**. The best-rated timbres are near 1.15. The mokugyo is modelled as a bass-reflex loudspeaker (shell
  = driver, slit = port): the two peaks are the coupled shell–cavity resonances. The drumstick force lasts
  **1–6 ms**, depending on the mass and stiffness of the material wrapped round the stick tip [Y05, U: abstract].
- **Temple blocks, a set of five (Wang et al. 2011, Taiwan):** fundamentals and decay constants measured with a
  microphone and FFT [W11]:

| Block | f₁ (Hz) | Ratio to block 1 | σ (1/s) | τ = 1/σ | Plastic block f₁ (Hz) | σ (1/s) |
|---|---|---|---|---|---|---|
| 1 | 425 | 1.00 | 18.7 | 54 ms | 581 | 14.6 |
| 2 | 525 | 1.24 | 24.8 | 40 ms | 738 | 33.9 |
| 3 | 606 | 1.43 | 27.7 | 36 ms | 781 | 36.6 |
| 4 | 647 | 1.52 | 27.0 | 37 ms | 788 | 32.8 |
| 5 | 831 | 1.96 | 14.8 | 68 ms | 2491 | 28.1 |

  Upper peaks: block 2 has a partial at 2103 Hz (4.0 × f₁), block 3 at 2213, 3234, 4797 Hz (3.65, 5.34, 7.91 × f₁)
  [W11]. σ is the paper's mean "decay rate". Its unit is not stated. I read it as amplitude 1/s, because the same
  paper gives σ = 4.3 for a xylophone C6, i.e. τ = 0.23 s, which is realistic **[U]**. Temple-block T60 = 6.91/σ ≈
  0.25–0.47 s **[D]**. The paper notes that the blocks have no clear harmonic peaks and that their pitches do not
  match a scale [W11].
- I found no published mode table for the rectangular orchestral woodblock.

### 3.3 Derived model of the slotted woodblock
**Lip (cantilever plate, clamped along the slot bottom, bending across the grain) [D]:**
f₁ = (1.875²/2π) · (t/L²) · √(E_T/(12ρ)).
With lip thickness t = 10 mm, free length (slot depth) L = 45 mm, ρ = 800 kg/m³ and cross-grain E_T ≈ 1 GPa **[G]**:
f₁ ≈ **890 Hz** **[D]**. With E = 12 GPa (along the grain) it would be 3.1 kHz. So the lip's cross-grain bending
is what places woodblocks in the 0.5–1.5 kHz range **[D/G]**.

| Lip mode | f/f₁ | Source |
|---|---|---|
| 1st bending (cantilever) | 1 | |
| 1st twisting / 1st bending along the length (uses E_L, the stiff direction) | 1.3–2.0 | **[G]** depends on block length |
| 2nd cantilever | 6.27 | Euler–Bernoulli **[D]** |
| 3rd cantilever | 17.5 | **[D]** |
| Block body (free-free bar along the grain, E_L) | 1 : 2.76 : 5.40 relative to its own f ≈ 1.5–3 kHz for 18 cm | free-bar ratios **[D]**, f **[G]** |

Measured temple-block partials (3.65, 5.34, 7.91) and the STK "Wood1" set (1 : 2.777 : 7.378 : 15.377) [STK] are
both in this range. STK's "Wood1" is plainly a free-bar series (2.777 ≈ 2.756).

**Cavity (Helmholtz) [D]:** f_H = (c/2π) √(A/(V L_eff)), L_eff = t + 1.7 √(A/π), c = 343 m/s.

| Size | V | Slit area A | Wall t | f_H | Q_rad = 2π √(V (L_eff/A)³) |
|---|---|---|---|---|---|
| Small temple block | 60 cm³ | 2.5 cm² | 8 mm | 730 Hz | 43 |
| Medium temple block | 150 cm³ | 4 cm² | 12 mm | 505 Hz | 53 |
| Large temple block | 400 cm³ | 8 cm² | 15 mm | 376 Hz | 48 |
| Very large | 1000 cm³ | 12 cm² | 20 mm | 259 Hz | 59 |

The volumes and slits are guesses **[G]**. The frequencies match the 425–831 Hz blocks measured [W11]. Viscous loss
at the slit lowers Q to ~15–30 **[G]**. In a slotted woodblock the slot is a long, narrow cavity whose mouth is the
whole slot, so its Helmholtz Q is low (~5–15) **[G]**. It is a "body" colour more than a ring.

### 3.4 Recipe
| Component | f (rel. to key frequency) | Q | τ at f₁ = 800 Hz | Amplitude, centre strike over the slot |
|---|---|---|---|---|
| Lip bending 1 (the pitch) | 1.0 | 60–100 **[G]** | 24–40 ms | 1.0 |
| Cavity (Helmholtz), coupled | 0.87 (so that F2/F1 = 1.15 [Y05]) | 20–30 **[G]** | 8–12 ms (alone) | 0.6 |
| Lip twist / length bending | 1.5 **[G]** | 60 | 15 ms | 0.3 |
| Body bar 1 | 3.6 **[U from W11 peaks]** | 80 | 9 ms | 0.25 |
| Body / lip 2 | 5.3 [W11] | 80 | 6 ms | 0.15 |
| Lip 2 / bar 3 | 7.9 [W11] | 100 | 5 ms | 0.08 |

- Q from wood: η(longitudinal) ≈ 0.006–0.01 for dense hardwoods (rosewood, maple) **[G]**; cross-grain bending
  loses more (η ≈ 0.01–0.02 **[G]**). τ = Q/(πf). This gives the "tens of ms" decay: ~25–60 ms for the pitch
  partial, in line with the temple-block τ of 36–68 ms [W11].
- **Coupling the cavity:** couple the lip mode and the Helmholtz mode with a gyrator-like term (piston area ×
  pressure), like a bass-reflex port. Two resonators with coupling κ split around their mean into F1, F2. Choose κ
  to give F2/F1 ≈ 1.15 [Y05]. Simpler: two independent resonators at that ratio.
- **No doublets:** a block has no rotational symmetry. Wood irregularity only detunes modes **[G]**.
- **Excitation:** a hard stick (rubber or plastic for orchestral blocks, wood for "woodblock" in pop): t_c ≈
  0.2–0.3 ms on cross-grain wood (§0) **[D]**. Temple-block mallets (rubber, yarn) or a wrapped mokugyo stick:
  1–6 ms [Y05]. Add a broadband "click": the contact force itself radiated through a high-pass, about 2–4 ms long
  **[G]**. Much of a woodblock's identity is that click.
- **Damping:** a hand on the block or a held stick shortens the lip modes by 2–5× **[G]**. Optional.

### 3.5 Key mapping
- GM woodblock samples cover the whole keyboard by repitching **[G]**. Real blocks: temple blocks 250–900 Hz
  [W11, D], woodblocks ~0.5–1.5 kHz **[G]**. Use f₁ = 440·2^((k−69)/12) (key 69 = 440 Hz, roughly a medium
  temple block) **[G]**.
- Physical scaling: scaling the block by 1/s multiplies all f by s and keeps Q, so τ ∝ 1/f. Low keys (f₁ < 200 Hz)
  turn into slit drums (log drums; that is physical too). High keys (> 2 kHz) become clicks: there, clamp τ ≥ 3
  cycles **[G]**.
- GM2 bank 1 Castanets [WGM2]: two shells clapped. The same recipe at f₁ ≈ 2–4 kHz, Q ≈ 20, with a double strike
  (2 hits 5–15 ms apart) **[G]**.

---

## 4. GM 120 Reverse Cymbal

### 4.1 What it is
A crash cymbal recorded and played backwards: the decay tail comes first, quiet and dark (the low modes ring
longest). The sound then swells, gets brighter, and ends abruptly at the moment of the strike. GM2 has no variation
of it [WGM2]. It is not a physical sound, so a model only needs a forward physical cymbal and a way to reverse it.

### 4.2 Physics of a 16–18 in crash
| Quantity | Value | Source |
|---|---|---|
| Low modes, 18 in cymbal | (3,0) 64 Hz, (4,0) 99 Hz, (5,0) 145 Hz, (6,0) 188 Hz | [ROL, U: search excerpt] |
| Same scaled to 16 in (f ∝ h/d², same h) | (3,0) 81, (4,0) 125, (5,0) 184, (6,0) 238 Hz | **[D]** |
| 12 in cymbal | (2,0) 62, (3,0) 135, (5,0) 459 Hz | [ROL, U] |
| Mode shapes | the first ~6 like a flat plate fixed at the centre; higher modes mix | [Rossing via HH13, AT22] |
| Modal density (Kirchhoff plate, n = A√3/(h c_L), c_L = √(E/(ρ(1−ν²))) = 3800 m/s) | 16 in, h = 0.8 mm: **0.070 /Hz (14 Hz spacing), ~1400 modes below 20 kHz**; h = 1.0 mm: 18 Hz, ~1100 modes | **[D]** (counts both members of each degenerate pair) |
| Spectrum after the strike | < 700 Hz dominates at once; strong 700–1000 Hz peaks build in 10–20 ms; energy in 3–5 kHz within ~100 ms | Rossing & Shepherd 1983 via [HH13] **[U]** |
| Aftersound | the 3–5 kHz range dominates the sound a second or so after the strike: the shimmer | [HH13, U] |
| Level envelope (recorded crashes) | peak SPL 0.15–0.2 s after the strike; 3–8 kHz is the loudest band for an 18 in | [HTHF, U: reviews with spectra] |
| Late tail | high modes die first; the last energy is in the few-hundred-Hz modes | [SS37, U] |
| Mechanism | geometric (von Kármán) nonlinearity: low modes → a cascade of energy to high frequencies (wave turbulence) | [CTT05, DT15, Bao19] |
| Taper | the thinner edge strengthens the nonlinearity and gives an almost immediate broadband spectrum (crash/splash) | [Bao19, U: search excerpt] |

**Decay by band.** A modal damping law measured on a thin plate (Humbert, h = 0.5 mm) is ξ = 8·10⁻³ ω^−0.4
[DAFx15]. Evaluated **[D]**:

| f | 100 Hz | 300 Hz | 1 kHz | 3 kHz | 5 kHz | 10 kHz |
|---|---|---|---|---|---|---|
| Q = 1/(2ξ) | 820 | 1280 | 2070 | 3200 | 3900 | 5200 |
| τ (amplitude) | 2.6 s | 1.35 s | 0.66 s | 0.34 s | 0.25 s | 0.17 s |
| T60 | 18 s | 9.4 s | 4.5 s | 2.4 s | 1.7 s | 1.1 s |

That law is for a bare lab plate. A crash on a stand (felt, sleeve) loses more at low frequencies **[G]**. Target
T60s for a 16–18 in crash: ~4–8 s in 100–500 Hz, ~3 s at 1 kHz, ~1.5–2.5 s at 3–5 kHz, < 1 s above 10 kHz
**[G]**. In the forward sound the highs keep being fed by the cascade for the first ~0.1–0.3 s, so their apparent
decay starts later than their own τ suggests.

**Excitation.** A raised-cosine force with half-width T_wid = 0.8–1 ms and peak 100–300 N drives a plate from weakly
nonlinear (w ~ h) to turbulent (w ~ 2h) [DAFx15]. Hertz gives t_c ≈ 0.11 ms for a stick tip on a rigid half-space
(§0) **[D]**. The cymbal's edge is soft locally, and crashes are hit with the shoulder of the stick, so 0.5–2 ms is
realistic **[G]**.

**Cascade, cheaply.** Ducceschi & Touzé simulate it with ~100 transverse modes up to 9 kHz and 50 in-plane modes
(21 × 18 cm × 1 mm plate). The nonlinear term costs O(N³) per step and needs f_s above audio rate [DAFx15]. That is
too costly for real time with a full cymbal. Real-time options **[G]**:
1. ~1000 linear modes (random-phase, density from the table above). A "cascade" source feeds the high modes in
   proportion to the low modes' energy: E_hi(t) driven by E_lo(t)² with a time constant of ~20–100 ms. A
   phenomenological wave-turbulence model (cf. Ducceschi & Touzé's augmented model [DT15, U]).
2. A few dozen nonlinear low modes + filtered noise bands for the wash (as `drums.md` §6 anticipates).

### 4.3 Making it reversed
The sound is not causal (the swell anticipates a strike), so three ways:

| Method | How | Pros | Cons |
|---|---|---|---|
| **A. Anti-damped modal bank (recommended)** | Reversed, a decaying mode A e^{−t/τ} sin(ωt+φ) becomes A e^{−(T−t)/τ} sin(ω(T−t)+φ): a *growing* sinusoid. Run each resonator with pole radius r = e^{+1/(τ f_s)} (> 1), started at amplitude A e^{−T/τ} with a random phase, and cut at t = T. | Real time; no buffer; any T; pitch by scaling f. The low modes (long τ) are audible from the start, the highs grow in late and steeply: exactly the reversed band envelope | A finite-time instability: it must be stopped at T (and at note-off). Double precision for long T or very small starting amplitudes |
| B. Offline forward render, played backwards | At note-on, render a full crash (physical model with cascade) for T seconds into a buffer and play it backwards | It *is* a reversed physical cymbal, with the reverse cascade at the end (the brief "bloom" from 3–5 kHz back to < 700 Hz in the last ~100 ms) | The first output sample is the render's *last*, so the whole render must finish before sound starts: 2 s × 1000 modes ≈ 10⁸ mode-updates, ~0.1–0.3 s CPU **[G]**. Pre-render per velocity layer at program load and cache it (model output, not recorded audio) |
| C. Band-envelope swell | 20–30 noise bands (or a modal bank with frozen amplitudes), each with gain e^{−(T−t)/τ_b} | Cheapest | Least "cymbal-like": no modal fine structure |

A hybrid of A and B: method A for the body (the swell) plus a short cached reversed cascade transient (the last
~150 ms: the reversed hit with its 700–1000 Hz and < 700 Hz content and the reversed contact click) **[G]**.

**Length T and the key:**
- GM modules play a fixed sample, so T is fixed per key and shrinks at higher keys: rate = 2^((k−60)/12), T ≈
  1.5–2.5 s at key 60 **[G]**. The note-off before T releases quickly (~50–100 ms) **[G]**.
- "Reversed over the note's length" cannot be known at note-on. Options: (a) T fixed per key (GM-compatible);
  (b) T from a controller or tempo (a "reverse to the bar line" setting); (c) grow until note-off and then play the
  cached reversed hit, which ends the sound at the release. Option (c) needs the growth capped: hold the bank at
  its final amplitude by switching r → 1 when the level reaches the target. It is the most playable live **[G]**.
- Pitch: scale all mode frequencies by 2^((k−60)/12) and τ by the inverse (a smaller cymbal) **[D]**, or keep τ (pure
  repitch, like a sample) **[G]**.

**A physical alternative** that needs no reversal: a soft-mallet *roll* with a crescendo on the forward cymbal model,
which is how orchestras make the sound live. Swelling mallet hits (yarn mallets, t_c ≈ 2–5 ms **[G]**), 15–25
strokes/s **[G]**, ending in a choke. It is physical, causal and fits the drum kit's cymbal model.

---

## 5. Summary table (starting values)

| | Tinkle Bell (handbell) | Agogo | Woodblock | Reverse cymbal |
|---|---|---|---|---|
| Mode ratios | 1, 3.0, 4.8, 5.4, 5.9, 7.4, 7.6, 9.0, 10.2 … [H13] | 1, 2.83, 5.42, 8.77, 12.9 [D]; + fixed ~3.7 kHz [STK, G] | 1 (lip), 0.87 (cavity, coupled), 1.5, 3.6, 5.3, 7.9 [W11, Y05, G] | 16 in: (3,0)…(6,0) ≈ 81, 125, 184, 238 Hz [D]; then ~14 Hz spacing to 20 kHz [D] |
| Q | 5000–10000 (2,0), 1500–3000 cluster [G] | 300–1200 [G] | 20–100 [G]; τ ≈ 36–68 ms measured [W11] | 800–5000 by band [DAFx15] |
| Doublet split | 0.05–0.2 % low, 0.2–1 % high [G]; church bell prime 0.53 % [KT] | 2–8 % (2,0), 0.5–2 % (3,0) [G] | none | random pairs, small [G] |
| Contact | 0.07–0.3 ms [D] | 0.1–0.2 ms [D] | 0.2–0.3 ms stick [D]; 1–6 ms wrapped [Y05] | 0.8–1 ms raised cosine, 100–300 N [DAFx15]; swell via anti-damping |
| Key → pitch | f(2,0) = key frequency [G] | f₁ = key frequency [G] | f₁ = key frequency [G] | repitch about key 60; T ∝ 1/rate [G] |
| Decay vs key | τ ∝ 1/f (scaled object) [D] | τ ∝ 1/f [D] | τ ∝ 1/f, floor 3 cycles [G] | n/a |
| Cost | ~15 doublets = 30 resonators | ~12 resonators ×2 bells | ~6–8 resonators + click | 300–1000 resonators, or 20–30 bands |

## 6. What to validate
1. Tinkle Bell against GM files with bell melodies: the register (is key = (2,0) right, or an octave lower?). The
   beat rates should be "alive but not wobbly".
2. Agogo: the (2,0) split. Sweep δ from 0 to 10 % and listen for the change from "bell" to "agogo/cowbell".
   Compare against measured pairs (§3 of `drums.md`: the Brazilian tracks) for the f₁ of the high/low bells and the
   decays. They are guesses here.
3. Woodblock: the F2/F1 = 1.15 coupling and the click. Fit τ against woodblock hits in the Latin tracks.
4. Reverse cymbal: method A against a reversed render of the forward model (method B) offline. They should be close
   except for the last ~150 ms.

## 7. Main gaps
- No measured agogo or cowbell mode tables or decay times were found online. Rossing's *Science of Percussion
  Instruments* ch. 12 ("Handbells, Choirchimes, Crotals and Cowbells") has cowbells [RB00] and is worth getting.
- Handbell decay times: Rossing & Sathoff 1980 [RS80] probably has them. Not read.
- Rectangular woodblock modes: none published that I could find. The lip/cavity numbers here are derived.
- Cymbal low-mode frequencies are from a search excerpt of the Rollins paper [ROL], not the paper itself.

---

## References

[AT22] E. K. Ellington Scott, A. Morrison, "The acoustics of the modern jazz drum kit", Acoustics Today (2022). https://acousticstoday.org/wp-content/uploads/2022/11/The-Acoustics-of-the-Modern-Jazz-Drum-Kit-E.-K.-Ellington-Scott-and-Andrew-Morrison.pdf
[Bao19] (Bao, Chaigne, Touzé) "Tapered cymbals", JASA (2019). https://perso.ensta-paris.fr/~touze/PDF/2019_BaoCT_TaperedCymbals_JASA.pdf (server refused the connection; search excerpt only)
[CTT05] A. Chaigne, C. Touzé, O. Thomas, "Nonlinear vibrations and chaos in gongs and cymbals", Acoust. Sci. & Tech. 26(5), 403–409 (2005), doi:10.1250/ast.26.403. https://www.jstage.jst.go.jp/article/ast/26/5/26_5_403/_article
[DAFx15] M. Ducceschi, C. Touzé, "Simulations of nonlinear plate dynamics: an accurate and efficient modal algorithm", Proc. DAFx-15, Trondheim (2015). https://dafx.de/paper-archive/2015/DAFx-15_submission_22.pdf (raised-cosine strike, Humbert damping law ξ = 8e-3 ω^−0.4, 100 modes to 9 kHz)
[DT15] M. Ducceschi, C. Touzé, "Modal approach for nonlinear vibrations of damped impacted plates: application to sound synthesis of gongs and cymbals", J. Sound Vib. 344, 313–331 (2015). https://hal-ensta-paris.archives-ouvertes.fr/hal-01134639 ; sound examples https://perso.ensta-paris.fr/~touze/modalsynthesis.html (not read)
[H13] U. J. Hansen, "Structural vibrations: normal modes in a hand-bell", Proc. Indiana Acad. Sci. 122(2), 118–125 (2013). https://journals.indianapolis.iu.edu/index.php/ias/article/download/22464/21956/34173 (Fig. 2: the mode chart with ratios, from [RP87])
[HB-dbl] B. Hibbert, "Doublets or warble in bells". https://www.hibberts.co.uk/doublets-or-warble-in-bells/
[HB-p2381] B. Hibbert, bell partials pages. https://www.hibberts.co.uk/?p=2381 ; https://www.hibberts.co.uk/identifying-bell-partials/
[HH13] R. Harrison, A. Hill, "A scientific approach to microphone placement for cymbals in live sound", Proc. IOA 35(2) (2013). https://adamjhill.com/AJHILL/wp-content/uploads/2017/05/Harrison-Hill-RS2013.pdf (cites Rossing & Shepherd 1983 and Rossing 2000)
[HTHF] Secrets of Home Theater and High Fidelity, cymbal reviews with spectra (Paiste 18" 2002 crash; Zildjian 20" A Custom Rezo crash). https://hometheaterhifi.com/latest-reviews/cymbal-reviews/paiste-18q-2002-crash-cymbal/
[KT] Keltek Trust, "The sound of bells" (doublets of a 19.25 in bell). https://keltektrust.org.uk/sob09.html
[PCD82] R. Perrin, T. Charnley, J. DePont, "Normal modes of the modern English church bell", J. Sound Vib. 90 (1983). https://www.hibberts.co.uk/wp-content/uploads/2022/01/perrin_charnley_depont_1982.pdf (not read)
[PGM] PG Music, General MIDI instrument and drum kit lists. https://pgmusic.com/tutorial_gm.htm
[RB00] T. D. Rossing, *Science of Percussion Instruments*, World Scientific (2000). https://www.worldscientific.com/worldscibooks/10.1142/4294 (not read)
[RM93] T. D. Rossing, (Mansell), ASA 125th meeting abstract on bass handbells (1993). https://auditory.org/asamtgs/asa93ott/4pMU/4pMU10.html ; ASA 127th abstract https://auditory.org/asamtgs/asa94mit/3pMU/3pMU4.html
[ROL] "The normal modes of cymbals", Rollins College student–faculty paper. https://scholarship.rollins.edu/cgi/viewcontent.cgi?article=1020&context=stud_fac (403; values from a search excerpt)
[RP87] T. D. Rossing, R. Perrin, "Vibrations of bells", Applied Acoustics 20, 41–70 (1987). Via [H13].
[RS80] T. D. Rossing, H. J. Sathoff, "Modes of vibration and sound radiation from tuned handbells", JASA 68(6), 1600–1607 (1980), doi:10.1121/1.385214. (abstract only)
[RPSP84] T. D. Rossing, R. Perrin, H. J. Sathoff, R. W. Peterson, "Vibrational modes of a tuned handbell", JASA 76, 1263–1267 (1984). Via [H13].
[SH] Sound House column on GM tones (agogo C4–C5, woodblock pitches). https://www.soundhouse.co.jp/en/contents/column/index?post=2903 (via search excerpt; page timed out)
[SOS-cb] G. Reid, "Synth Secrets: synthesizing cowbells & claves", Sound On Sound (Oct 2002). https://www.soundonsound.com/techniques/synthesizing-cowbells-claves
[SOS-ex] "Exploring exotic instruments in GM/XG/GS sound sets", Sound On Sound. https://www.soundonsound.com/techniques/exploring-exotic-instruments-gmxggs-sound-sets
[SS37] G. Reid, "Synth Secrets 37: practical cymbal synthesis" (mirror). https://github.com/micjamking/synth-secrets/blob/master/part-37.md
[STK] P. R. Cook, G. P. Scavone, STK `ModalBar.cpp` presets (Agogo, Wood1, Wood2, …). https://github.com/thestk/stk/blob/master/src/ModalBar.cpp
[US6915756] US patent 6,915,756 B1, "Bells" (handbell tuning, 3:1 and 2.4:1). https://patents.google.com/patent/US6915756
[W11] (Wang et al.), "不同打擊樂器聲音特性探討" (sound characteristics of different percussion instruments), Proc. 24th Acoustical Society of the ROC conference (2011). https://val.npust.edu.tw/wp-content/uploads/2021/11/248-不同打擊樂器聲音特性探討.pdf (Tables 2–4)
[WGM] Wikipedia, "General MIDI". https://en.wikipedia.org/wiki/General_MIDI
[WGM2] Wikipedia, "General MIDI Level 2". https://en.wikipedia.org/wiki/General_MIDI_Level_2
[Y05] K. Yuasa, N. Yanagisawa, M. Sunohara, F. Asano, Y. Furihata, "Acoustical characteristics of mokugyo" (title approximate), JASA 117, 2247–2258 (2005). Record: https://katalog.lib.cas.cz/KNAV/EdsRecord/edsair,edsair.doi.dedup.....167e3ca1eaaae0df4560cad748ed917d (abstract only)
