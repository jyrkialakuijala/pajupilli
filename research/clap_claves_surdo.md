# The hand clap, claves, wood blocks, surdo, bell tree (and XG's low bass drum)

Status: 2026-11-16. Research, a plan and what was built. These are the drum keys still on
stand-ins or silent after the kit's other pieces were built:

| Key | Sound | Use in the user's files |
|---|---|---|
| 39 | Hand Clap (Jazz/Standard kits; the Brush kit's 39 is the brush slap) | 2 874 notes / 48 files |
| 75, 76, 77 | Claves, Hi Wood Block, Low Wood Block | 2 900 / 9 |
| 84 | Bell Tree (GM2) | none |
| 86, 87 | Mute Surdo, Open Surdo (GM2); XG's 13, 14 | none |
| XG 33 | Bass Drum L | a few |

Marks: **[paper]**, **[web]** a source below; **[ref]** measured here on FluidR3's General MIDI
samples (timidity, `/etc/timidity/fluidr3_gm.cfg`): recordings of real instruments, cut short;
**[calc]** worked out here; **[memory]** general knowledge, to be checked; **[design]** a choice.

## 1. The hand clap

- **Physics.** Two hands meet. The air cupped between the palms is squeezed out through the
  gaps between the fingers and the heels of the hands. It rings as a Helmholtz resonator: the
  cavity a spring, the air in the openings a mass. Its pitch is set by the cavity's volume and the
  openings: cupped hands sound low, flat hands high. The hands' soft tissue damps it within a few
  cycles **[paper, FKLZJ25]**.
  - Material elasticity changes the pitch little but the sound's evolution in time a lot; the
    frequency changes by an order of magnitude with the cavity **[paper, FKLZJ25; web, APS23]**.
- **Numbers [calc].** f = (c / 2π) √(A / (V L′)), with L′ about 1.7 times the opening's radius.
  - A cupped clap: V ≈ 50 cm³, A ≈ 2 cm² → about 0.9 kHz.
  - A flat clap: V ≈ 10 cm³ → about 2 kHz.
- **The recorded GM clap [ref, key 39]:**
  - spectral centre 2.7 kHz, with broad peaks 0.7–1.6 kHz;
  - about ten bursts 2–3 ms apart over 25 ms;
  - energy spread evenly over the first 100 ms.

  This is a group clap: several people clapping together, each clap itself two contacts (the
  fingers, then the palms).
- **Model [design].**
  - Four clappers, each with their own cavity (0.7–2.2 kHz, Q 3–6) and openings.
  - Each clap is two contacts 1–3 ms apart. The cavity's collapse pushes the neck's air (a
    half-sine squeeze of 0.5–1.5 ms), which rings in the resonator; the sound is its volume flow's
    derivative (a monopole). Under it, the jet's turbulent noise (2–6 kHz) while the hands close,
    and the palms' soft thud (100–300 Hz).
  - The clappers are spread over 0–20 ms. Everything is drawn afresh each stroke, so no two claps
    are alike.

## 2. Claves

- **The instrument.** Two hardwood bars (rosewood, grenadilla), about 20 cm × 2.5–3 cm.
  - One lies in the cupped hand, supported at the fingertips and the heel of the hand, near its
    free-free nodes. The cupped hand under it is a resonator.
  - It is struck at its middle by the other, held loosely at one end.
- **Measured [paper, EM23]:** a pair of rosewood claves, 20 cm × 3.0 cm, 160 g each (ρ ≈ 1130
  kg/m³), E 13 GPa, rang at 2540–2570 Hz. A bar this size, free-free, would be 2260 Hz
  [calc: 22.37 / (2π L²) × (d/4) √(E/ρ)]. The grip shortens the vibrating length, so 2555 Hz.
- **The recorded GM claves [ref, key 75]:** one strong partial at 2156 Hz, with the others 6–16
  dB under within ±4 %; about 126 ms to −30 dB (T60 ≈ 0.25 s); spectral centre 2.2 kHz.
- **Model [design].**
  - Two free-free bars (1 : 2.756 : 5.404 : 8.933): the held one at 2.4 kHz, ringing longest; the
    striker about 6 % lower, damped by the grip (T60 ≈ 0.12 s). Each stroke's pitches a hair off.
  - The cupped hand: the held bar's fundamental radiated about 2× (a resonance tuned to it), its
    upper modes T60 ≈ 0.05–0.1 s.
  - A hard wood-on-wood contact (about 0.1 ms).

## 3. Wood blocks

- **The instrument.** A hardwood block hollowed out behind a slot: a thin lip over a cavity. The
  lip's bending, coupled to the cavity's air, gives a few strong, nearly harmonic partials; the
  rest die fast **[memory, Fletcher & Rossing ch. 19]**.
- **The recorded GM pair [ref]:**
  - Hi Wood Block (76): 746 Hz with a cluster 671–758 Hz, and 1615–1638 Hz (×2.17–2.2);
    about 297 ms to −30 dB.
  - Low Wood Block (77): 502 Hz (cluster 472–561), 1108 and 1230 Hz (×2.2, ×2.45);
    about 353 ms to −30 dB.
- **Model [design].** One block per key, five modes each:
  - the lip's main mode, with a neighbour 4 % off (the cluster);
  - the second at 2.2× and a third near 3.6×;
  - the cavity's breathing just below the main mode, which helps it radiate.

  T60s: 0.35 s main, 0.15 s second, under 0.06 s above. A hickory stick, about 0.3 ms.

## 4. The surdo

- **The instrument** **[web, Wikipedia; Virada]:**
  - a large, deep two-headed bass drum of samba: aluminium or wood shell, goatskin or plastic
    heads;
  - primeira 22–26" (the lowest), segunda 20–22", terceira 14–18"; Rio drums are about 60 cm
    deep, northeastern ones 50 cm;
  - played with a padded mallet (felt or leather) in one hand while the other hand mutes the head.
- **GM2's two keys:** 86 Mute Surdo, the dead stroke with the hand on the head; 87 Open Surdo.
  The recorded pair [ref] sit at 92 Hz (mute, yet T60 1.6 s) and 136 Hz (open, T60 2.5 s).
  That is not a samba surdo's sound, and is not followed.
- **Tuning [design]:** no published pitches were found. A 22" × 55 cm primeira tuned low and
  booming: 62 Hz (≈ B1). Heavy heads (goat or 14 mil plastic: σ 0.32 kg/m²).
- **Model [design].**
  - The two-headed drum of the toms (src/tom.cc, used as it is), with the surdo's sizes. The
    mallet is a 0.15 kg felt head (Hertz, K ≈ 1.5e6 N/m^1.5: a 3–6 ms contact), with little
    click.
  - The mute: the other hand on the batter. Every mode is damped (setDamping ~12) from the stroke
    until an open stroke lifts it. A mute stroke also stops what still rings.

## 5. The bell tree

- **The instrument.** 10–25 nested brass bells, cupped, on a vertical rod, from the largest at the
  bottom to the smallest at the top. A brass beater is drawn down (or up) the stack, so a
  glissando of strikes rings out together **[memory]**.
- **The recorded GM bell tree [ref, key 84]:** energy at 8–15 kHz (peaks 8.0, 9.0, 10.9, 13.1,
  14.8 kHz), centre 9.7 kHz, longer than its 0.4 s sample.
- **Model [design].**
  - 16 bells, fundamentals from 3.2 kHz (the largest) to 9 kHz, each with three cup modes
    (1, 1.6, 2.4×) and a T60 of 1.5–2.5 s.
  - The run goes from the top down (high to low) over 0.25–0.5 s; a harder key is a faster sweep.
    Each bell is struck by the sliding beater: steel on brass, about 0.05 ms.

## 6. XG's 33, the low bass drum (Bass Drum L)

- XG's map has a second, lower bass drum below the standard 35/36. The jazz kick's 20" variant
  (`jazz-kick-20`: 59.6 Hz, T60 0.97 s; research/kick.md) is that: larger, lower, a little longer.
  **{"jazz-kick-20", 36}** (36: struck nearer the middle).

## 7. Plan (2026-11-16)

1. `src/clap.h/.cc`: `-inst hand-clap`, the group clap of §1. Check the spectral centre (target
   about 2.7 kHz), the burst structure (bursts within ~25 ms), no two claps alike (correlation),
   velocity ~30 dB.
2. `src/woodperc.h/.cc`: `-inst claves`, `-inst wood-blocks` (76 hi, 77 low) on ModalSet
   (modal.h, read only). Check the claves at 2.4 kHz with T60 about 0.25 s, the blocks at 746
   and 502 Hz.
3. `src/surdo.h/.cc`: `-inst surdo` on Tom with a surdo TomSpec (read only). Check the note
   62 Hz, the open T60, the mute at least 20 dB down at 0.2 s.
4. `src/belltree.h/.cc`: `-inst bell-tree` on ModalSet. Check the descending run (the spectral
   centre falls over the run), the band 3–15 kHz.
5. Registration (rows, voice lines, trims to the common −28.2 LUFS at velocity 90), a test, a
   README paragraph, and the drumFor lines for the coordinator.

## 7.1 Built (2026-11-16)

- **Files:** `src/clap.h/.cc`, `src/woodperc.h/.cc` (claves, wood blocks), `src/surdo.h/.cc` (on
  `Tom`, read only), `src/belltree.h/.cc` (on `ModalSet`, read only). The test is
  `clap_claves_surdo()` in `tools/test.py`.
- **Measured:**

| Check | Result | Target |
|---|---|---|
| Clap: spectral centre, per clap | 1.6–4.8 kHz, 2.8 kHz mean | 2.7 kHz [ref] |
| Clap: bursts | 6 over 17 ms | ~10 over 25 ms [ref] |
| Clap: likeness of two claps | max correlation 0.37–0.41 (mean 0.31) | none alike |
| Claves | 2450 Hz, T60 0.29 s; the striker's 2303 Hz at −13 dB | 2.5 kHz [paper], T60 ~0.25 s [ref] |
| Wood block 76 | 746 Hz (0), 672 Hz (−4), 1638 Hz (−18), T60 0.58 s | 746 Hz [ref] |
| Wood block 77 | 502 Hz (0), 524, 561 Hz (−11 to −16), T60 0.67 s | 502 Hz [ref] |
| Wood blocks, 16 strokes at equal velocity (place scattered ±10 % of width and length) | each partial's level against the strongest: sd 0–5 dB (the main 0, its neighbours and the top mode 3.5–5.2); mean 2.3 (76), 2.0 dB (77); with no scatter 0.00; the peak level's sd 0.4 dB | roughly the same place, never quite the same stroke |
| Surdo | 62 Hz; open T60 2.9 s; muted T60 0.36 s, 25–34 dB under open at 0.2 s | 62 Hz [design] |
| Surdo glide | 63.0 → 61.9 Hz at velocity 127 | |
| Bell tree | spectral centre 11.0 → 9.0 → 7.4 → 6.5 kHz over 0.3 s | a falling run |
| Velocity 16 to 127 (fast peak) | clap 32, claves 28.7, blocks 30, surdo 25.8–31.7, bell tree 30.9 dB | ~30 dB |
| Cost | at most 514 ns/sample (2.5 % of a core) | |
| Fuzz, 10 s each | no resets; peaks 1.1–1.5, the surdo 6.1 (random ff strokes in phase on a long-ringing 22" drum; one stroke at velocity 127 peaks 1.09) | |

- **Levels:**
  - surdo and bell tree at the common −28.2 LUFS (trims −0.2 and −4.5 dB);
  - claves +1.0 dB (−30.1 LUFS, a peak ~0.8);
  - the clap and the wood blocks set by their peaks (~0.6 and ~0.8 at velocity 100; −37.9 and
    −37.4 LUFS): short sounds, which would peak at 1.7–2.3 if brought to the common loudness.
- **Not done:**
  - no recording fitted beyond FluidR3's samples;
  - the clap's openings are not modelled per hand (one resonance a clapper);
  - the claves' cupped-hand resonance is a fixed radiation gain;
  - the surdo's pitch is a design value (no published tuning found), and its leather mallet and
    the rim strokes are not built;
  - the bell tree sweeps only downward.

## 8. Sources

- **[FKLZJ25]** Y. Fu, A. Kiyama, G. Liu, L. Zhang, S. Jung, "Revealing the sound, flow
  excitation, and collision dynamics of human handclaps", Physical Review Research 7, 013259
  (2025). https://doaj.org/article/af9371cadb5641ceb8bf93cf79193324; Cornell's summary:
  https://news.cornell.edu/node/330469
- **[APS23]** Y. Fu et al., "Sound of a handclap: An elastic Helmholtz resonator", APS March
  Meeting 2023. https://archive.aps.org/mar/2023/b08/7
- **[EM23]** E. Evans, C. McComb, "Pitch Imperfect: Designing 3D Printed Claves to Mimic
  Traditional Rosewood Claves", Rapid Prototyping Journal (2023); preprint:
  https://utw10945.utweb.utexas.edu/sites/default/files/2021/012%20Pitch%20Imperfect_%20Designing%203D%20Printed%20Claves%20to%20Mi.pdf
- **[Wikipedia]** Surdo. https://en.wikipedia.org/wiki/Surdo
- **[Virada]** Surdo guide. https://viradadrums.com/surdo-guide/
- Fletcher & Rossing, *The Physics of Musical Instruments* (bars, blocks):
  https://csclub.uwaterloo.ca/~pbarfuss/The_Physics_of_Musical_Instruments.pdf
