# GM 38 Slap Bass 2 (`-inst slap-bass-2`): research

Status: research 2026-11-20. Nothing built. `src/midifile.cc:192` maps `slap-bass-2` to
`double-bass`. `src/guitaramp.cc` has a "slap-squash" preset (`GuitarAmp::slapSquash()`, from
`research/bass_amps.md` 10.5), but no slap bass exists to use it. This note adds to
`research/bass_section.md` (the electric bass, §6.3–6.4 slap) and `research/bass_amps.md` (§2.2,
7, 10.4–10.5 slap chains), and does not repeat them. Slap Bass 1 (GM 37) is researched in parallel
in `research/slap_bass_1.md`.

How each fact is marked:
- **[T]** tested or measured here: SF2 data parsed from the files, notes rendered through TiMidity++
  and FluidSynth and measured, MIDI files scanned, or a primary PDF read for this note.
- **[paper]** from a paper (or its abstract, where that is said).
- **[web, source]** from a web page: a maker, retailer, magazine, forum or manual.
- **[memory]** general knowledge, not looked up. Check before relying on it.
- **[E]** my estimate or arithmetic from cited values.

---

## 0. Short answer

**What GM 38 is, in the references.**
- GM gives only the name. The name is older than GM: "Slap Bass 1" and "Slap Bass 2" are
  programs 69 and 70 of the MT-32/CM-64 list. The SC-88 Pro manual's CM-64 map lists them as
  "069 127 Slap Bass 1, 070 127 Slap Bass 2" **[T, SC-88Pro OM p.161]**.
- **Roland GS.**
  - Slap Bass 2 uses **2 voices** in the SC-88 map and the native SC-88 Pro map, but **1 voice**
    in the SC-55 map. Slap Bass 1 uses 1 voice in all three **[T, SC-88Pro OM tone list]**. From
    the SC-88 on, Roland built 38 as a two-partial tone, a layer or a velocity switch.
  - The GS variation of 38 is **"FM Slap"** (bank 8, SC-88 Pro). The variations of 37 are "Slap
    Pop", "Reso Slap" and "Unison Slap" **[T, same]**.
- **Yamaha XG.** The variation of 38 is "VeloSlap" (Velocity Switch Slap, bank 43). The
  variations of 37 are "ResoSlap" (27) and "PunchThm" (Punch Thumb, 32)
  **[web, polyphone.io MIDI standards]**. XG files the thumb under 37 and a velocity-switched
  thumb/pop under 38. What the switch does is **[memory]**.
- **SoundFonts** (presets parsed and rendered here **[T]**):
  - **FluidR3:** program 37 is **"Slap Bass"** and 38 is **"Pop Bass"**. They are two sample sets
    from the same bass, named by string and fret ("Slap Bass E0…G5" against "Pop E0…G5"). So
    **38 = the popping technique, 37 = the thumb, same instrument.**
  - **TimGM6mb:** short single-cycle loops through a filter envelope. Slap 2 starts darker
    (579 Hz) and sweeps further (+3.5 oct, to 6.4 kHz), against Slap 1's 1.1 kHz + 2.5 oct.
  - **Jnsgm2:** 38 uses the same samples as TimGM6mb (the measured spectra are identical), with
    a velocity-to-filter modulator.
  - **SGM-V2.01:** separate multisample sets, 10 zones each.
- **What is the same everywhere [T]:**
  - 38 is **brighter than 37 from E2 up**: the >2 kHz share in 0.3–0.8 s is on average 5–15 dB
    higher over E2–E3 (single keys +2 to +20 dB).
  - 38 is **not more compressed**: its crest factor is equal or higher, and it **decays as fast or
    faster** in its first second.
  - So the "more compressed, squashed" description in bass_amps.md 10.5 (one Soundhouse snippet)
    is **not supported by any SoundFont measured here**. Whether it fits the SC-55 itself is
    unknown (no hardware or ROM here).

**What GM 38 should be, physically [E].**
- The same Jazz Bass and player as 37, **pop-forward**:
  - every note that can lie on the D or G string is **popped** (the finger hooks the string,
    pulls it 2–6 mm away from the body, and lets it slam onto the frets);
  - only notes below D2 are thumbed.
- **Active electronics:** a buffered preamp at the coils, Marcus Miller's '77 J with a Bartolini
  TCT. Buffering takes the cable's ~700 pF off the coils, which moves the J pickups' resonance
  from ~4.4 kHz to ~8–9 kHz. Add treble and bass boost.
- **New stainless roundwounds.**
- A clean DI-led chain with the same compression as 37, not more.
- What makes 38 sound like 38 is the **sustained zing of popped strings** (bright for 0.3–0.8 s,
  not just a click) and their **harder fret slap**. It is not squash.

---

## 1. The GM 38 patch, reference by reference

### 1.1 Roland: MT-32, SC-55, SC-88, SC-88 Pro

- **Name lineage [T].** The SC-88 Pro's CM-64 map lists the MT-32 tones (bank 127) with
  "069 Slap Bass 1, 070 Slap Bass 2". It also lists CM-32P tones (bank 126): "016–019 Slap Bass 1,
  020–023 Slap Bass 2". GM Level 1 (1991) took its program names largely from Roland's lists
  **[memory]**. So "Slap Bass 2" started as a Roland preset name. It does not describe a standard
  playing technique.
- **The MT-32's Slap Bass 1/2** were LA-synthesis tones: a short PCM attack spliced onto
  synthesised partials **[memory]**. That is one root of the "synth-like slap" side of the
  family. The other is the DX7 "E.BASS 1" family; see bass_section.md §8.
- **Voices [T, SC-88Pro OM pp. 154–161]:**

| Map | 37 Slap Bass 1 | 38 Slap Bass 2 | Variations |
|---|---|---|---|
| SC-55 | 1 voice | **1 voice** | |
| SC-88 | 1 | **2** (marked "#" for the native map: the same as the SC-88's) | 37: Reso Slap (bank 8) |
| SC-88 Pro native | 1 | **2** | 37: Slap Pop (1), Reso Slap (8), Unison Slap (9, 2 voices); **38: FM Slap (8, 2 voices)** |

- **What this says:**
  - From the SC-88 on, Roland gave 38 two partials, a layered or velocity-switched tone. 37 kept
    one **[T for the counts; E for the reading]**.
  - Roland's only named relative of 38 is a *synthetic* slap (FM Slap). The "Slap Pop" variation
    sits under 37, so Roland did not treat 38 as "the pop" **[T for the names; E for the
    reading]**.
- **Not found:** any Roland text describing how Slap Bass 2 sounds, or the content of its samples.
  The SC-55's waves were not available (no ROM dump or hardware here). This is the largest gap
  (§9).

### 1.2 Yamaha XG

- Bank 0 program 38 "SlapBas2". Variation bank 43 "VeloSlap" (Velocity Switch Slap). Under 37:
  bank 27 "ResoSlap" and bank 32 "PunchThm" **[web, polyphone.io]**. Yamaha's DD-65 and PSR
  manuals list them in the order "Slap Bass 1, Punch Thumb Bass, Slap Bass 2, Velocity Switch
  Slap" **[web, manualslib snippets]**.
- **Reading [E]:** Yamaha files the thumb-centred variant under 37. Under 38 it files one that
  switches samples by velocity. That points to 38 as the patch where the pop appears. The
  switch's direction (pop on hard notes) is **[memory]**: no Yamaha data list was readable here
  (their PDFs refused the download).

### 1.3 The SoundFonts, from their SF2 data [T]

The data were parsed with a small reader (`phdr/pbag/pgen/inst/ibag/igen/shdr`, plus the
modulator lists). SGM is SF3: the sdta chunk has an odd size, so the pdta list was found by
searching.

**FluidR3_GM** (`/usr/share/sounds/sf2/FluidR3_GM.sf2`):
- **37 "Slap Bass"** (instrument 3):
  - 9 zones, samples "Slap Bass E0, E2, E4, A1, A3, D1, D3, G0, G5". That is string and fret: E
    string at frets 0/2/4, A at 1/3, D at 1/3, G at 0/5. The key ranges run 0–28, 29–30, …,
    46–84.
  - 22.05 kHz, about 5 s each, a short loop at the end.
  - Volume envelope: hold 3.0 s (1902 tc), decay 36–40 s to −100 dB (6386 → 5502 tc, shorter up
    the keyboard).
  - **No filter, no modulators.**
- **38 "Pop Bass"** (instrument 8):
  - 9 zones, "Pop E0, E2, E4, A1, A3, D1, D3, G0, G5": **the same strings and frets, popped**.
  - The same key splits and envelope, plus extra hold on the two lowest zones (3392 and 2786 tc).
  - Reverb send 20 on both.
- So the sample sets are two recordings of one bass, one per technique. **FluidR3's GM 38 is the
  pop.**

**TimGM6mb** (`/usr/share/sounds/sf2/TimGM6mb.sf2`):
- **37 "Slap Bass 1":**
  - 6 zones on 3 sample sets (A0, D1, D4, C6) of 433–8150 samples at 44.1 kHz: an attack and a
    loop.
  - Filter Fc 8502 abs-cents = **1.11 kHz**, modEnv→Fc +2.9–3.0 k cents (to **6.4 kHz** at the
    peak), modEnv decay 0.43–0.61 s to 29 % (≈ 2.0 kHz).
  - Attenuation 1.6–2.0 dB; release 0.07 s.
- **38 "Slap Bass 2":**
  - 6 zones, samples "Slap Bass 2 A0, F1, C4, C6", only **2327 / 2475 / 1041 / 179 samples**
    (53 ms and less): a sampled click and a looped single cycle.
  - Fc 7375 = **579 Hz**, modEnv→Fc +3.6–4.2 k cents (peak **6.4 kHz**), modEnv decay
    0.36–0.53 s to 37 % (≈ 1.4 kHz).
  - Attenuation 0.3 dB; volume decay 5.6–19.8 s (scaled by key); release 0.15 s.
- **The difference:** 38 sweeps a filter further and faster from a darker base. That is a synth
  envelope on a sampled attack, and it is the most "synth-like" 38 of the four.

**Jnsgm2** (`~/soundfonts/Jnsgm2.sf2`):
- **37 "Slap Bass 1":** "SLPBS31/43/84" at 22.05 kHz (other recordings), filter 730 Hz +3600
  cents (5.8 kHz) decaying 0.71 s.
- **38 "Slap Bass 2":** **the TimGM6mb samples** by name, key ranges, envelopes and LFOs, with
  longer copies (14 283 samples). The rendered octave-band spectra of the two 38s agree to the dB
  **[T]**. The filter: 9322 abs-cents (1.8 kHz) + 2475 cents.
- **Preset modulators on both:** a velocity→Fc of −4684 cents (linear, max→min). The default
  velocity→Fc modulator is switched off at instrument level. **Jnsgm2 is the only set here where
  velocity changes the timbre.**

**SGM-V2.01** (`~/soundfonts/SGM-V2.01.sf3`):
- Two separate 10-zone multisample sets at 44.1 kHz, named by pitch: "Slap Bass 1 E1 … B3" and
  "Slap Bass 2 E1, G1, A1, C2, D#2, F#2, A#2, D3, F#3, B3".
- No filter. 38's preset gain is 4.7 dB above 37's (−91 against −44 cB). The default
  velocity→filter modulator is cancelled.

**Is 38 a different instrument, a different technique, or the same samples processed?**

| Set | Answer |
|---|---|
| FluidR3 | **Different technique, same bass** (pop against thumb) **[T]** |
| TimGM6mb / Jnsgm2 | Different samples, and a deeper and faster filter sweep: **processed, synth-like** **[T]** |
| SGM | Different samples (technique not named) **[T]** |
| SC-55 | Unknown. 1 voice, like 37; becomes 2 voices on the SC-88 **[T]** |

### 1.4 Rendered and measured [T]

**Method.**
- Keys E1, A1, E2, A2, E3, A3 (28, 33, 40, 45, 52, 57) at velocities 32/64/96/127, each held
  1.6 s.
- Renderers:
  - TiMidity++ (`-EFreverb=d -EFchorus=d`, all programs remapped to the font as in
    `tools/synthcompare.py`);
  - FluidSynth 2.x (libfluidsynth through ctypes, reverb and chorus off), which applies the SF2
    filters, modulation envelopes and modulators.
- TiMidity removes the 0.5 s of leading silence, so onsets were found in the audio.
- The scripts are in the scratchpad (`slap2_render.py`, `slap2_fluid.py`, `slap2_sf2.py`).
- The FluidSynth figures are given below. TiMidity agrees for FluidR3 and SGM (no filters there).
  For TimGM6mb and Jnsgm2, TiMidity ignores the filter envelope, so those renders are brighter
  than intended.

**Brightness of the attack: centroid of the first 100 ms (Hz) at v127**

| Key | FluidR3 37 / 38 | TimGM6mb 37 / 38 | Jnsgm2 37 / 38 | SGM 37 / 38 |
|---|---|---|---|---|
| E1 28 | 302 / 396 | 173 / 232 | 592 / 237 | 246 / 165 |
| A1 33 | 313 / 460 | 182 / 301 | 570 / 312 | 171 / 204 |
| E2 40 | 307 / **870** | 169 / **366** | 323 / **401** | 253 / **406** |
| A2 45 | 519 / **1671** | 192 / **474** | 376 / **538** | 385 / **698** |
| E3 52 | 422 / **1939** | 253 / **550** | 477 / **656** | 280 / **1042** |
| A3 57 | 472 / **2421** | 324 / **627** | 596 / **778** | 276 / 328 |

**The sustained brightness: >2 kHz share of the energy (dB) in 0–30 / 30–100 / 100–300 / 300–800 ms, v127**

| Key | FluidR3 37 | FluidR3 38 | TimGM6mb 37 | TimGM6mb 38 | Jnsgm2 37 | Jnsgm2 38 | SGM 37 | SGM 38 |
|---|---|---|---|---|---|---|---|---|
| E1 | −18/−19/−29/−42 | −14/−14/−23/−37 | −11/−15/−22/−36 | −17/−17/−22/−30 | −8/−9/−16/−31 | −17/−17/−23/−32 | −15/−24/−37/−54 | −25/−27/−42/−54 |
| E2 | −12/−16/−19/−25 | −12/**−8/−13/−18** | −15/−21/−28/−33 | −16/−14/−20/−28 | −14/−16/−25/−36 | −16/−13/−17/−30 | −14/−18/−28/−44 | −16/−16/−22/−31 |
| A2 | −13/−11/−13/−17 | −7/−5/−9/−15 | −18/−23/−28/−35 | −12/−11/−14/−21 | −15/−18/−27/−39 | −12/−10/−14/−21 | −13/−15/−20/−32 | −10/−11/−18/−29 |
| E3 | −12/−17/−26/−38 | −9/−6/**−11/−19** | −20/−26/−29/−36 | −12/−12/−17/−26 | −17/−21/−27/−38 | −12/−10/−14/−18 | −18/−18/−24/−37 | −11/−10/−18/−31 |
| A3 | −12/−19/−28/−37 | −8/−6/−11/−17 | −22/−26/−29/−35 | −13/−13/−20/−28 | −20/−23/−26/−37 | −12/−11/−16/−19 | −19/−20/−26/−39 | −20/−20/−27/−43 |

- **From E2 up, 38 is brighter in all four sets.** By 0.3–0.8 s the gap averages +7 to +15 dB
  over E2, A2 and E3 in each set (single keys +2 to +20; SGM's A3 alone is 4 dB the other way):
  the brightness *lasts*, which a bright click alone would not do. Below E2 the sets disagree:
  TimGM6mb, Jnsgm2 and SGM make 38's lowest notes darker than 37's.
- **3–6 kHz in the first 30 ms** (the fret clatter band, Issanchou's "reinforced zone around
  4.5 kHz"): FluidR3 38 is −8 to −19 dB of the total, against 37's −12 to −20. In the others the
  two are within a few dB. The *click* is not what separates them.

**Dynamics and level (v127, FluidSynth)**

| | Crest (peak over RMS 0–0.3 s), dB | Level at 1.1–1.3 s against the first 100 ms, dB | Peak, dBFS |
|---|---|---|---|
| FluidR3 37 / 38 | 11.1–16.2 / 11.7–15.2 | −6.5…−14.3 / −8.6…−12.3 | −5.8…−6.3 / −5.8…−6.0 |
| TimGM6mb 37 / 38 | 9.1–9.8 / **13.4–18.1** | −6.3…−7.9 / **−11.3…−16.1** | −6.9…−8.9 / −6.6…−7.0 |
| Jnsgm2 37 / 38 | 12.6–17.6 / 13.6–17.8 | −7.7…−9.7 / **−11.0…−14.1** | −6.3…−10.4 / −7.0…−7.6 |
| SGM 37 / 38 | 7.3–11.9 / 9.1–14.0 | −5.8…−11.2 / −6.0…−12.0 | −4.3…−6.7 / −2.4…−9.0 |

- **38 is not more compressed.** Its crest factor is the same (FluidR3) or 4–8 dB higher
  (TimGM6mb), and its tail falls as fast or faster (by 1.2 s, 5–8 dB more fall in TimGM6mb and
  3–5 dB more in Jnsgm2).
- The two have the same loudness to within about ±2 dB, with the same peaks. bass_amps.md 7.1
  found the same with TiMidity: Slap 2 is 0.7 dB louder at the peak.
- **Velocity:**
  - In FluidR3, TimGM6mb and SGM, velocity changes only the level: 12 dB from v32 to v64, 7 dB
    to v96, 5 dB to v127, with an identical spectrum.
  - In Jnsgm2 (its preset modulator), 38's E3 >2 kHz share goes −34 → −23 → −14 → −10.5 dB over
    v32/64/96/127, a 24 dB rise.
  - The physical model should behave like Jnsgm2 here: harder pops reach the frets harder.
- **bass_amps.md 7.1** measured FluidR3's 38 attack centroid at 406–2140 Hz against 37's
  307–560 (TiMidity). The table above agrees.

### 1.5 The FluidR3 samples themselves (no envelope or filter in the way) [T]

Raw samples taken from the `smpl` chunk; the >2 kHz share and the 3–6 kHz share per window (dB):

| Sample | 0–30 ms | 30–100 ms | 100–300 ms | 300–800 ms | 1.5–2.5 s |
|---|---|---|---|---|---|
| Slap Bass E0 (E1 open, thumb) | −17 / −21 | −19 / −24 | −29 / −38 | −42 / −59 | −68 / −78 |
| Pop E0 | −13 / −18 | −14 / −18 | −24 / −34 | −37 / −54 | −66 / −74 |
| Slap Bass G0 (G2 open) | −14 / −18 | −11 / −14 | −13 / −17 | −18 / −25 | −29 / −49 |
| Pop G0 | **−7 / −9** | **−5 / −7** | **−8 / −10** | **−15 / −17** | −27 / −39 |
| Slap Bass D3 | −11 / −16 | −15 / −18 | −19 / −22 | −25 / −32 | −47 / −63 |
| Pop D3 | −9 / −13 | −7 / −11 | −13 / −16 | −19 / −24 | −35 / −51 |
| Slap Bass G5 | −12 / −14 | −15 / −16 | −24 / −26 | −35 / −42 | −63 / −76 |
| Pop G5 | −9 / −11 | **−5 / −7** | −10 / −11 | −18 / −21 | −42 / −52 |

- **Harmonics of the open G** (0.1–0.4 s, dB under the strongest):
  - Pop G0: k = 8, 9, 10 at **−3, −6, −3**;
  - thumb: **−19, −14, −24**.
  - On the open E (k = 2…5) the pop is 2–6 dB stronger than the thumb.
- **Attack against body:**
  - thumb: the first 30 ms is 8–14 dB over the 0.1–0.3 s level (a spike, then the body);
  - pop: 3–7 dB over (its body is as bright as its attack).
- **Reading [E]:** a pop leaves the string with a large initial *displacement* with a sharp
  corner. The modes keep that 1/k² displacement (1/k at the velocity pickup) for as long as the
  string rings. A thumb is a soft, wide *velocity* pulse whose own spectrum is dark, so its
  brightness is mostly the fret clank. This matches Rank & Kubin's two excitations
  (bass_section.md §6.3).

---

## 2. The physics that makes 38 distinct

### 2.1 Popping

- **What happens [paper, Rank97 via bass_section.md §6.3; L]:**
  - the index or middle finger hooks under the D or G string near the end of the neck;
  - it pulls the string *away* from the body, several mm;
  - the string slips off the fingertip.
- It swings back through rest and on towards the fingerboard by about the pull's amplitude. With
  an action of ~2–2.5 mm it **hits the frets during its first half-period**, then on several
  following passes.
- **Issanchou et al.'s experiment is a pop** **[paper, Issanchou18 JASA §IV.B; Issanchou et al.
  2018 Applied Acoustics]**:
  - the G string of an electric bass (Tab. IV: 191.6 N, 6.69 g/m, 0.863 m) pulled to
    **3.6 mm at 64 cm from the nut (≈22 cm from the bridge)** and released, against the measured
    20-fret neck;
  - contacts in **0–0.09 s**, "substantial high frequency content from the very first
    collisions", a reinforced spectral zone **near 4.5 kHz**;
  - the coefficient of restitution hardly matters ("dominated by waves going back and forth in
    inter-fret intervals").
  - The authors call pop and slap "a peculiar bright and percussive sound" from "numerous
    contacts ... intentionally provoked in the transient attack".
- **Rank & Kubin** model the pop as a large triangular initial displacement with rounded corners,
  and the frets as a one-sided limit that must act at least during the first period at the
  body-end frets **[paper, Rank97 via bass_section.md]**.
- **Trautmann & Rabenstein** simulate fret–string contact in the functional transformation
  method (modal). A multirate version cuts the cost about 80 % for six strings **[paper,
  EURASIP JASP 2004, abstract]**.
- **Bilbao & Torin**, "the fretboard": distributed collision with the board and the individual
  frets, with tension modulation, free and finger-stopped **[paper, DAFx-14, abstract]**.
  **Bilbao, Torin & Chatziioannou** give energy-conserving collision schemes **[paper, Acta Acust.
  2015, abstract]**.
- **Ducceschi, Bilbao, Willemsen & Serafin** (JASA 149(5), 2021): collisions by energy
  quadratisation. The update needs one linear solve, with no Newton iteration, and "generalise[s]
  naturally" to modal discretisations. **Bilbao, Ducceschi & Zama** (JCP 2023) make it fully
  explicit **[paper, abstracts]**. This is the real-time candidate for 20 frets on a modal string
  (§6).

### 2.2 Pop against thumb, in numbers [E unless marked]

| | Thumb (37's default) | Pop (38's default) |
|---|---|---|
| Strings | E, A (and D) | D, G (A sometimes) |
| Excitation | velocity pulse, thumb flesh 15–25 mm wide, contact 1–3 ms, thumb comes back (returning contact) | displacement 1.5–6 mm *away* from the body; release off the fingertip 0.2–0.5 ms; hook ~3 mm wide |
| Where | 22–27 cm from the bridge (end of the neck) [bass_section.md] | 18–24 cm from the bridge, same region [L] |
| String spectrum after release | dark: the pulse's width and duration cut above ~0.3–1 kHz | bright: 1/k² displacement, 1/k velocity, sinc of a 3 mm hook to well above 10 kHz |
| Speed at first fret contact | ~0.5–1.5 m/s (the thumb's speed) | ω·A: G2 open, 3.6 mm → 2π·98·0.0036 ≈ **2.2 m/s**; D2 → 1.7 m/s |
| Fret contacts | mostly the last frets, first 1–2 periods | all frets under the string's swing, 0–90 ms **[paper, Issanchou]** |
| At the pickup | moderate | apex 3.6 mm at 22 cm gives 2.5 mm at the J neck coil (156 mm) and 1.0 mm at the bridge coil (64 mm). With a 2–3 mm pole clearance the neck coil sees the string nearly reach its poles: strong field nonlinearity, so even harmonics and bark |

- **Popped notes are usually the octave or tenth above a thumbed note**, played on the D/G
  strings. Rhythmically they land on off-beats or backbeats **[L]**.
- **The double pop:** index and middle in quick succession on two strings (Mark King, Louis
  Johnson) **[L/memory]**.
- **The left-hand slap:** fretting fingers slapped onto the strings as a percussive ghost
  **[web, guitarworld on Mark King]**.

### 2.3 Active electronics and bright strings

- **Buffering is the main physical cause of the "active" top [E].**
  - A passive J coil pair (1.65 H in parallel) loaded by ~800 pF of cable and pots resonates at
    **4.4 kHz** (bass_section.md §3.3).
  - With an onboard preamp at the coils, only the coils' own ~180 pF and the wiring load them
    (~200 pF): **≈ 8.8 kHz**. A single J coil goes from 3.2 to ~9 kHz, near the published
    unloaded "J about 9 kHz" **[web, via bass_section.md §3.2]**.
  - The pop's string spectrum reaches well above 5 kHz (§2.2), so this octave of extra pickup
    bandwidth is where 38's sustained zing lives.
- **Marcus Miller's 1977 Jazz Bass:**
  - Roger Sadowsky fitted a Stars Guitars preamp in 1979;
  - after it was destroyed in 1983 (a DI surge at Media Sound) he fitted a **Bartolini TCT**;
  - plus a Badass bridge. It is the bass on *The Man with the Horn* **[web, guitarworld;
    premierguitar]**.
  - Both pickups up and the preamp's treble boost give the canonical "J slap" **[L]**.
- **Music Man StingRay (1976):**
  - One humbucker whose coils sit **75 and 99 mm from the saddle** (33.3/35.7 cm from the 12th
    fret on a 34″ bass; single-coil midpoints 34.3–34.8 cm) **[web, pickup-position-db]**.
  - A 9 V active preamp, 2-band originally. Treble about +20/−16 dB at ~10 kHz and bass about
    +14/−7 dB at ~40 Hz. These are clone makers' measurements; Ernie Ball publishes no 2-band
    specification **[web, reverb listings; Ernie Ball forum]**.
  - Louis Johnson's "Thunder Thumbs" bass with the Brothers Johnson **[memory]**.
- **Pickup combs** (open E harmonics k = 1…16, dB under the strongest, velocity taps at points)
  **[E]**:

| Pickup | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| StingRay HB (75 + 99 mm, series) | −9 | −4 | −1 | 0 | 0 | 0 | −2 | −5 | −11 | **−33** | −10 | −5 | −2 | −1 | −1 | −2 |
| J both ('60s, 155 + 64 mm) | −6 | −1 | 0 | 0 | −2 | −6 | −15 | **−37** | −27 | −22 | −10 | −6 | −4 | −5 | −10 | −20 |

  - The StingRay has no scoop at the 7th–9th harmonics. It is flat from the 3rd to the 7th and
    weaker at the fundamental: a punchier, more mid-forward slap than the scooped J.
- **Strings:**
  - New roundwounds are much brighter than old ones. Stainless (Rotosound Swing Bass 66, the
    1970s–80s slap string) is brighter than nickel **[memory/L]**.
  - Model new strings as **a lower high-frequency loss**: loss1 30–40 % below the GM 34 strings,
    which puts T60 at 8 kHz near 0.6 s rather than Kemp's 0.4 s **[E]**.
  - No measured stainless-vs-nickel loss data were found.

### 2.4 Synth-like or processed character

- **TimGM6mb/Jnsgm2's 38** is a sampled click plus a looped cycle through a filter envelope
  (579 Hz → 6.4 kHz → 1.4 kHz in 0.4–0.5 s). It has a synth's wah-like sweep and is the same on
  every note **[T]**.
- **Roland put "FM Slap" under 38 [T].** In the 80s the DX7 bass (bass_section.md §8: a harmonic
  burst at 13–19 f₀, identical on every note) was the "processed" slap of records.
- **FluidR3 and SGM are plain recordings.** The pop's own brightness is enough, with no sweep
  **[T]**.
- **Decision [E]:** keep 38 physical (a real pop, real frets). The DX7 E.BASS 1 already has its
  own place (bass_section.md §10) and is the natural home for an "FM Slap". **Do not put a filter
  sweep on the physical pop.**

---

## 3. Players and style: where the 38 character comes from

- **Larry Graham** (Sly & the Family Stone, Graham Central Station) developed "thumpin' and
  pluckin'" around 1966–70: the thumb standing in for the kick, the pluck for the snare
  **[memory]**.
- **Louis Johnson** (Brothers Johnson; on Michael Jackson's *Off the Wall* and *Thriller*
  sessions):
  - a StingRay, hard thumb and pop, very bright, percussive octaves **[memory]**.
- **Stanley Clarke:**
  - Alembic basses with active electronics, bright, a lot of popping and up-the-neck lines (*School
    Days*, 1976) **[memory]**.
- **Marcus Miller:**
  - the '77 J + Bartolini (§2.3), both pickups, treble boosted, scooped;
  - pops and thumbs in tight 16ths, many ghost notes **[web; L]**.
- **Mark King** (Level 42, from 1980):
  - a Jaydee Supernatural from about 1981, later Status graphite and Alembic;
  - Trace Elliot amps (an AH500 head into two 4×10s);
  - "continuous 16th-note slap grooves played with a combination of slaps, pops, hammer-ons and
    fretting-hand slaps", the "machine-gun" thumb **[web, guitarworld; Wikipedia]**.
- **What this means [E]:** the 1980s pop sound GM's slap programs imitate is bright, pop-forward,
  active and DI'd. 37 and 38 are two halves of one technique: thumb for the low notes and the
  beat, pops for the high notes and the backbeat. FluidR3 splits them exactly that way.
- **For interpreting MIDI:**
  - a GM 38 track is a request for the pop-forward half;
  - it is played by a slap player, so low roots are still thumbed (they cannot be popped well);
  - nearly everything from D2 up is popped.

### 3.1 GM 38 in the user's files [T]

- Scanned: `~/midi` (44 files), `~/Music` (309) and `~/Downloads` (44); `~/Musici` does not exist.
  Program 38: **one file, 361 notes**:
  - `~/Music/midi/bushgrafts/grelots.mid` ("Michel Petrucciani's Latin song"), channel 2, bank 0;
  - keys 29–55 (F1–G3), median 43 (G2); velocities 18–118, median 85;
  - durations median 0.47 s; the line is legato (duration/IOI median 0.97);
  - **68 % of the notes at D2 or above**, 51 % at G2 or above; 11 % shorter than 80 ms (grace
    notes and slides, e.g. D3 then D#3 v40 for 30 ms);
  - **octave jumps up within 0.4 s: 1 %**; 8 near-simultaneous pairs; a short pitch-bend gesture
    at 81 s.
- For comparison, program 37: "Gaviota trio 2" (510 notes, velocity median 110) and "louisiana"
  (723 notes, 31 % shorter than 80 ms), 1233 notes in all. Program 36 (fretless): 26 636.
- **What the line is:** a tumbao-like latin/jazz trio line (roots, fifths, octaves on half and
  dotted beats). It is *not written as slap* (no octave-pop figures, no ghost-note 16ths). The
  file's author chose 38 for its sound on their module.
- **What this means [E]:**
  - The player must turn an ordinary legato line into a slap performance without chopping it.
    Thumb the roots below D2; pop everything else, about 2/3 of this file.
  - Keep the duration: a popped note rings until its note-off, as in GM 34.
  - Do not insert ghost notes.
  - Velocity 85 typical: a mezzo pop with clear fret contact.

---

## 4. The chain

- **What 1980s slap records used [web/L/memory]:**
  - active bass → DI (often the DI alone) → compression (1176 or dbx 160), plus a clean
    solid-state head into 4×10s live (Trace Elliot, GK);
  - Trace Elliot's graphic EQ was set in a "smile" (lows and highs up, 400–800 Hz down);
  - sometimes chorus on 1980s pop records; exciters (Aphex) in some mixes **[memory]**.
- **What the GM references show [T]:** no chorus or modulation (only reverb sends), no
  distortion, no extra squash on 38.
- **Proposal for "slap-squash" (GuitarAmp), revising bass_amps.md 10.5 [E]:**
  - **Keep 37's 1176 settings or nearly**: 4:1, **attack ≥ 1 ms** (0.8 ms is 37's), release
    120–150 ms, 4–6 dB on pops.
    - The planned 8:1 with 0.2 ms attack would flatten the pop's first 5 ms and lower its crest
      factor. Every reference keeps or raises the crest factor of 38.
  - **The stomp compressor in front: off by default** (or 2:1, ≤ 2 dB). It was there only for
    "squashed" (Soundhouse), which the measurements do not support.
  - **SansAmp-like drive: off by default** (or ≤ 10 % blend).
  - **The brightness comes from the bass, not the amp:** the active buffer (§2.3), and the
    preamp's EQ on the bass (treble +6 dB shelf at 4 kHz, bass +4 dB at 50 Hz, flat mids).
  - **DI path** at least as bright as 37's (VPF at noon: +4 dB at 35 Hz, −6 dB at 380 Hz, +4 dB
    at 10 kHz).
  - **Blend** DI 80 % and amp 20 %, the amp a GK-like head into the 4×10 + horn (cabinet 5).
  - Output level matched to 37 within ±1 dB.
  - **Optional, off by default:** a chorus (two delays of 6–12 ms modulated at 0.5–0.8 Hz,
    15–25 % wet, above 200 Hz only), a "1980s pop" switch.

---

## 5. What 38 shares with 37 (the other agent's plan) and what must differ

### 5.1 Shared [E]

- **The instrument:** `ElectricBassModel` on `Guitar`, the J spec (`jazzBass()`), the
  EXL170-like strings, neck dead spots, pickups and the coil network.
- **The fret collision:**
  - 20 one-sided contacts at the fret positions (crowns ~1 mm over the board), the action and
    the relief;
  - energy-conserving (IEQ/SAV as Ducceschi 2021, or `Bass`'s penalty springs `kBoardK`,
    stiffened and tested), active while the string is near the frets (first 0.1–0.2 s);
  - a stopped string's part behind the finger is damped.
- **More modes:** carry modes to 10 kHz while contacts are active (about 240 on an open E1, 100
  on G2). `Guitar::kMaxModes` = 128 must grow or become per-string.
- **The thumb:** the velocity pulse with the returning thumb contact.
- **The pop excitation:** a displacement held by a hook of width w, released over a slip time.
  Both programs need it; 37 uses it less.
- **The fingering solver**, ghost notes and left-hand damping.
- **The amp chain base** (slap-di).

### 5.2 What differs for 38 [E], with numbers

| Parameter | 37 Slap Bass 1 | **38 Slap Bass 2** | Basis |
|---|---|---|---|
| Technique choice | thumb by default; pop only for jumps up ≥ 10 semitones within 0.4 s or notes ≥ C3 (48) | **pop by default for every note the hand can take on the D or G string (in practice ≥ D2, 38); thumb below D2**; a thumbed note directly before a pop a 10th or octave up stays thumbed | FluidR3 37 = thumb, 38 = pop; refs' 38 differs from E2 up (§1.4) |
| Share of pops on grelots.mid | ~10–20 % | **~65 %** | §3.1 |
| Fingering bias | low position (frets 0–5), open strings welcome | **position up the neck, frets 3–9**: notes from D2 to G2 on the D/G strings, not the A string, so they can be popped | |
| Pop depth (perpendicular, away) | 1.5–4 mm | **2–6 mm: v40 → 2 mm, v85 → 3.6 mm (Issanchou's case), v127 → 6 mm** | §2.1–2.2 |
| Pop place | 20–24 cm from bridge | **18–22 cm** (a little nearer the bridge: tighter, brighter) | [L] |
| Pop release | 0.3–0.5 ms | **0.2–0.3 ms**, hook 3 mm | |
| Electronics | passive J into the DI (coil resonance 4.4 kHz) | **active: buffered at the coils (resonance ≈ 8.8 kHz, Q ≈ 1.5), onboard EQ treble +6 dB shelf at 4 kHz, bass +4 dB at 50 Hz** | §2.3 |
| Strings | EXL170 nickel, loss as GM 34 | **new stainless roundwounds: loss1 −35 %** | §2.3 |
| Compressor | 4:1, 0.8 ms, 150 ms | **4:1, 1–2 ms, 120 ms; no stomp, no drive by default** | §1.4, §4 |
| Loudness at v100 | reference | **within ±1 dB of 37** | §1.4 |
| Alternative spec (by ear, a knob) | | `PJ_SLAP2_BASS=stingray`: one humbucker at 75 + 99 mm in series, 2-band preamp (treble +8 dB at 8–10 kHz, bass +5 dB at 40 Hz) | §2.3 |

### 5.3 The chain for 38

- J (active, §5.2) → splitter 0.9 MΩ (the preamp's output is low-impedance, so the splitter's
  load no longer matters).
- DI path: the VPF at noon.
- Amp path: the GK-like head and cabinet 5.
- Blend DI 0.8, then the 1176 at 4:1, 1.5 ms / 120 ms.
- Room reverb as for the other basses.
- Optional chorus, off.

---

## 6. Build plan for 38 (after 37's core)

1. **The pop-forward player:**
   - the technique rule and fingering bias of §5.2 in `ElectricBassPlayer` (or a slap player
     shared with 37, with a `popBias` parameter: 37 = 0.15, 38 = 0.85);
   - velocity maps to pop depth 2–6 mm.
2. **The active bass:**
   - a `GuitarSpec` flag `active`: the coil network loaded by ~200 pF and 1 MΩ, not the cable;
   - an onboard 2-band EQ (shelves);
   - for 38, stainless strings (loss1 × 0.65).
   - A StingRay spec (one series humbucker at 75/99 mm) behind a knob, for an A/B by ear.
3. **The pop's pickup nonlinearity:** the field model (Novak, already in `Guitar`) must see the
   2.5 mm swing at the neck coil. Check that a 6 mm pop does not push the string "through" the
   pole (clamp the field term at ≥ 0.5 mm clearance).
4. **Revise "slap-squash"** to §4 (milder than now). Keep the stomp and drive behind knobs.
5. **`midifile.cc`:** map `slap-bass-2` to the new instrument.
6. **Cost:**
   - with 240 modes × 2 polarisations and 20 contacts for 0.15 s after each pop, then the
     contacts asleep, about 1.5× GM 34's cost per note **[E]**;
   - the IEQ update is one linear solve per contact set; with the modal string it is a 20×20
     system while active.

---

## 7. Tests

**Physics (shared with 37, pop-specific limits):**
1. **Fret contact happens, then stops.**
   - A G2 open pop at v85 (3.6 mm, 22 cm): the first contact within 6 ms (half a period, 5.1 ms,
     plus margin), contacts at ≥ 5 different frets, the last before 0.12 s
     (Issanchou: 0–0.09 s).
   - A 1.5 mm pop at v32: few or no contacts.
2. **Energy:** with the contacts' restitution at 1 and no losses, total energy constant to 1e-6
   over 0.2 s. With losses, never rising.
3. **The pop's spectrum:** the same note (G2 open, v100), pop against thumb.
   - The pop's >2 kHz share in 0.3–0.8 s ≥ 5 dB above the thumb's (FluidR3 raw samples: +3…+17).
   - Harmonics 8–10 of the open G within 8 dB of the strongest for the pop, ≥ 12 dB under for
     the thumb.
4. **The 4.5 kHz zone:** a v100 pop's 0–90 ms spectrum has more energy in 3.5–5.5 kHz than the
   same pop with the contacts disabled, by ≥ 6 dB (Issanchou's reinforced zone).
5. **Active pickup:** with the buffer, the J-both network peaks at 8–9.5 kHz; passive (37) at
   4.0–4.6 kHz. Below 1 kHz the two are within 0.5 dB.

**The test that tells 37 and 38 apart** (`tools/test.py`: render the probe on both, FluidR3's
probe keys and velocities as in §1.4, plus the first 32 notes of grelots.mid). Pass when all hold:
- (a) **Attack centroid** (first 100 ms, v100) for keys ≥ E2: 38 ≥ 1.3 × 37 on every key.
  References: FluidR3 2.8–5.1×, TimGM6mb 1.9–2.5×, Jnsgm2 1.2–1.4×, SGM 1.6–3.7× (except A3).
- (b) **Sustained brightness:** the >2 kHz share in 0.3–0.8 s, averaged over E2, A2 and E3: 38 ≥
  37 + 5 dB, and none of E2, A2, E3 more than 2 dB darker than 37. References: averages
  +7 (SGM) to +15 dB (Jnsgm2); single keys +2 to +20 dB.
- (c) **Not squashed:** the crest factor (peak over RMS of 0–0.3 s) of 38 ≥ 37's − 1 dB. Level
  at 1.2 s against the first 100 ms: 38 ≤ 37's + 1 dB (38 decays at least as fast).
- (d) **Same loudness:** the v100 integrated loudness of the probe within ±1.5 dB.
- (e) **Technique counts on grelots.mid:** pops ≥ 55 % of the notes for 38 and ≤ 25 % for 37.
  Thumbed notes on 38 all below D2.
- (f) **Velocity opens the pop:** on 38, E3's >2 kHz share (0–100 ms) rises ≥ 10 dB from v32 to
  v127. Jnsgm2 rises 24 dB; the plain sample sets do not rise at all.
- (g) **A guard against a 38 that is just a brighter EQ of 37:** on keys ≥ E2 the difference
  between them at 0–30 ms must be smaller than at 0.3–0.8 s. That is, 38's extra brightness lasts
  (the string, the pop), and is not only in the click (an EQ would brighten both equally).

**Chain:**
- The 1176 on 38: pops get 4–6 dB of gain reduction at v100, and the first 2 ms of a pop's
  waveform keep their peak within 1 dB of the uncompressed signal (attack ≥ 1 ms).
- No THD > 1 % on a held E1 at v100.

---

## 8. What a listener would notice if 38 were wrong

1. **38 = 37 + treble EQ:** the click gets brighter but the note behind it stays as dull as a
   thumb. A pop's body is bright for half a second (§1.5).
2. **"Squashed" 38:** a flat, lifeless pop with no snap (0.2 ms attack, 8:1). The references are
   peaky.
3. **Low roots popped:** a pop on E1 sounds thin and flappy. Real players thumb the low roots.
4. **A synth sweep:** a filter opening and closing on every note sounds like TimGM6mb, the DX7,
   or a keyboard patch, not a bass. Keep that for "FM Slap" or the DX7.
5. **Every pop identical:** the depth, the fret clatter and which frets the string strikes vary
   from note to note.
6. **Legato lines chopped:** grelots.mid's notes are written legato. A slap player's pop rings
   until the next note.

---

## 9. Not found, or uncertain

- **The SC-55's Slap Bass 2 samples and envelope**, and any Roland text describing it. This is
  the most important gap: the GM files were mostly made on SC-55/SC-88s **[memory]**. A Sound
  Canvas VA or SC-55 recording of E1–A3 at four velocities, measured as in §1.4, would settle
  whether Roland's 38 is pop-forward, compressed, or synthetic.
- **Yamaha's XG 38** sample content and what "Velocity Switch Slap" switches between.
- **The Soundhouse column** (bass_amps.md's only source for "squashed") could not be loaded
  (timeouts). Its claim stands unverified and is contradicted by every SoundFont measured.
- **Pop depths, places and release times** measured on players: only the one controlled case
  (3.6 mm at 22 cm). The IDMT-SMT-BASS "slap-pluck" notes would give real pop spectra to fit.
- **The StingRay 2-band preamp's real curves:** clone makers' numbers only.
- **Stainless against nickel string losses:** no measurement found.
- **Issanchou et al., Applied Acoustics 129 (2018)**, "String/frets contacts in the electric bass
  sound": only the abstract was read (HAL and core.ac.uk blocked the download). The ISMA 2017
  version read is an abstract page.

---

## 10. Sources

**Manuals and lists**
- Roland, *SC-88 Pro Owner's Manual*, tone list pp. 154–160 (voices per map, variations), CM-64
  map p. 161 **[T]**. https://dosdays.co.uk/media/midi/SC-88PRO_OM.pdf
- Polyphone documentation, MIDI standards annex (GS and XG variation names and banks) **[web]**.
  https://www.polyphone.io/en/documentation/manual/annexes/midi-standards
- Yamaha DD-65 / PSR voice lists (XG names, snippets) **[web]**.
  https://www.manualslib.com/manual/196702/Yamaha-Dd-65.html?page=46
- Soundhouse column on GM programs (cited by bass_amps.md; not loadable here).
  https://www.soundhouse.co.jp/en/contents/column/index?post=2827

**SoundFonts (parsed and rendered) [T]**
- `/usr/share/sounds/sf2/FluidR3_GM.sf2` (presets 36 "Slap Bass", 37 "Pop Bass").
- `/usr/share/sounds/sf2/TimGM6mb.sf2` (36 "Slap Bass 1", 37 "Slap Bass 2").
- `~/soundfonts/Jnsgm2.sf2` (36, 37). `~/soundfonts/SGM-V2.01.sf3` (36, 37).
- TiMidity++ and libfluidsynth 3 (FluidSynth 2.x).

**Papers**
- C. Issanchou, V. Acary, F. Pérignon, C. Touzé, J.-L. Le Carrou, "Nonsmooth contact dynamics for
  the numerical simulation of collisions in musical string instruments", JASA 143(5), 3195
  (2018), §IV.B (the electric bass G string, 3.6 mm at 64 cm from the nut, contacts 0–0.09 s,
  4.5 kHz) **[paper, read]**.
  https://www.lam.jussieu.fr/Membres/LeCarrou/Articles/A24_Issanchou_NonsmoothContactDynamics.pdf
- C. Issanchou, J.-L. Le Carrou, C. Touzé, B. Fabre, O. Doaré, "String/frets contacts in the
  electric bass sound: Simulations and experiments", Applied Acoustics 129, 217–228 (2018)
  **[paper, abstract]**. https://hal-ensta.archives-ouvertes.fr/hal-01582578 ; ISMA 2017 version:
  https://perso.ensta-paris.fr/~touze/PDF/ISMA_2017_paper_15.pdf
- C. Issanchou, S. Bilbao, J.-L. Le Carrou, C. Touzé, O. Doaré, "A modal-based approach to the
  nonlinear vibration of strings against a unilateral obstacle: simulations and experiments in
  the pointwise case", JSV 393, 229–251 (2017) **[paper, cited by the above]**.
  https://perso.ensta-paris.fr/~doare/files/issanchou_jsv2017.pdf
- E. Rank, G. Kubin, "A waveguide model for slapbass synthesis", ICASSP 1997 **[paper, via
  bass_section.md]**. https://www2.spsc.tugraz.at/people/franklyn/ICASSP97/pdf/author/ic970443.pdf
- L. Trautmann, R. Rabenstein, "Multirate simulations of string vibrations including nonlinear
  fret-string interactions using the functional transformation method", EURASIP JASP 2004
  **[paper, abstract]**. https://dx.doi.org/10.1155/S1110865704312059
- S. Bilbao, A. Torin, "Numerical simulation of string/barrier collisions: the fretboard",
  DAFx-14 **[paper, abstract]**.
  https://www.dafx14.fau.de/papers/dafx14_stefan_bilbao_numerical_simulation_of_s.pdf
- S. Bilbao, A. Torin, V. Chatziioannou, "Numerical modeling of collisions in musical
  instruments", Acta Acustica united with Acustica (2015) **[paper, abstract]**.
  https://www.ness.music.ed.ac.uk/wp-content/uploads/2015/01/bilbao_acustica_r2.pdf
- M. Ducceschi, S. Bilbao, S. Willemsen, S. Serafin, "Linearly-implicit schemes for collisions in
  musical acoustics based on energy quadratisation", JASA 149(5) (2021) **[paper, abstract]**.
  https://www.pubmed.gov/34241147/
- S. Bilbao, M. Ducceschi, F. Zama, "Explicit exactly energy-conserving methods for Hamiltonian
  systems", J. Comput. Phys. (2023) **[paper, abstract]**. https://arxiv.org/pdf/2206.12391
- J. Abeßer et al., DAFx-13 (IDMT-SMT-BASS: slap-thumb and slap-pluck classes) **[paper, via
  bass_section.md]**.

**Instruments and players**
- Pickup positions (StingRay, Status, Spector), cm from the 12th fret **[web]**.
  https://agschaid.github.io/pickup-position-db
- StingRay 2-band preamp clone specifications **[web]**.
  https://reverb.com/item/92115761-bqfs-vintage-70s-pre-eb-musicman-stingray-2-band-preamp-1-year-warranty-2026 ;
  Ernie Ball forum on the 2-band: https://forums.ernieball.com/goto/post?id=918583
- Marcus Miller's 1977 Jazz Bass (Stars preamp 1979, Bartolini TCT 1983) **[web]**.
  https://guitarworld.com/features/the-story-behind-marcus-millers-1977-fender-jazz-bass ;
  https://www.premierguitar.com/roger-sadowsky-a-vintage-man-of-modern-means
- Mark King's basses, Trace Elliot rig, technique **[web]**.
  https://www.guitarworld.com/features/level-42-mark-kings-5-most-famous-bass-guitars ;
  https://en.wikipedia.org/wiki/Mark_King_(musician)

**Project files**
- `research/bass_section.md` (§2–3 strings and pickups, §6.3–6.4 slap, §8 DX7 E.BASS 1),
  `research/bass_amps.md` (§2.2, 7.1, 10.4–10.6), `src/guitaramp.cc` (`slapDi`, `slapSquash`),
  `src/ebass.h/.cc`, `src/guitar.h`, `src/bass.cc` (`kBoard` contacts), `src/midifile.cc:192`.
- User MIDI: `~/Music/midi/bushgrafts/grelots.mid` (the only GM 38 file).
