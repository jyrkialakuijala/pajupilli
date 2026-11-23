# GM 37, Slap Bass 1 (`-inst slap-bass-1`): research and build plan

Status: research only, nothing built. Today `slap-bass-1` falls back to `double-bass`
(`src/midifile.cc:192`). The amp preset is ready (`GuitarAmp::slapDi()`, `src/guitaramp.cc:251`;
`research/bass_amps.md` 10.4). The two-pickup bass it plays on is built (`src/ebass.*`, GM 34). Slap
Bass 2 (GM 38) is being researched in parallel in `research/slap_bass_2.md`; this file covers it
only to say how the two differ.

This file adds to `research/bass_section.md` §6.3–6.4 (Rank & Kubin, Issanchou, IDMT, the
first proposal) and `research/bass_amps.md` §2.2, 7, 10.4 (the chain, FluidR3 levels). It
does not repeat them.

How each fact is marked:
- **[web, source]**: read on the web; the sources are listed at the end.
- **[paper]**: from the cited paper (its abstract, or the part read).
- **[memory]**: general knowledge, not looked up. Check it before relying on it.
- **[T]**: tested here. Either read from the SF2 files with a small parser, rendered with
  TiMidity++ and measured, scanned from the user's MIDI files, or simulated with a scratch modal
  model (Appendix A).

---

## 0. Summary: what GM 37 is, and how it differs from GM 38

- **GM itself names the two programs and nothing more** ("Slap Bass 1", "Slap Bass 2")
  [web, polyphone]. Each maker defines the sound.
- **The names are one maker's, older than GM.**
  - The MT-32 (1987) already had "Slap Bass 1" (described as "nice slap") and "Slap Bass 2"
    ("variation") as presets 69/70 [web, synthmania MT-32].
  - The SC-88Pro's CM-64 map keeps them at bank 127, programs 69/70. The CM-32P's PCM tones,
    at bank 126, also hold a "Slap Bass 1" (programs 16–19) and a "Slap Bass 2" (20–23)
    [web, SC-88Pro manual].
  - one maker's 1988–89 PCM slap tones are key-split multisamples. On the sample-based module, every slap tone is
    "Thumped, pulled, harmonics" over key ranges, roughly E2/B2 and F#4/C#4 in one maker naming,
    i.e. MIDI 40/47 and 66/61. They come in pairs: "SINGLE", and "DETUNE" (two voices). There
    are also "V-SW" tones that switch from thumped to pulled at velocity 100 [web, U-20 manual].
- **Sound Canvas.**
  - Slap Bass 1 takes **one voice** in every map (early GS module, later GS module, SC-88Pro).
  - Slap Bass 2 takes **one voice in the early GS module map and two in the later GS module and SC-88Pro maps**
    [web, SC-88Pro manual]. So the 38 of the later GS module era is a layered or doubled sound; the 37 is a
    single sample set.
  - GS variations under 37 (bank:program, counted from 0) [web, SC-88Pro manual; polyphone]:
    - "Slap Pop" (1 voice);
    - "Reso Slap" (bank 8, 1 voice);
    - "Unison Slap" (bank 9, 2 voices).
  - Under 38 there is only "FM Slap" (bank 8, 2 voices).
- **XG.**
  - Variations of 37: "ResoSlap" (bank 27) and **"PunchThm" (Punch Thumb, bank 32)**.
  - Variation of 38: **"VeloSlap"** (bank 43, a velocity switch) [web, polyphone].
  - So in XG the *thumb* variation hangs on 37, and the *velocity-switched thumb/pop* hangs on
    38.
- **FluidR3_GM, the most-used free set, is explicit** [T]:
  - preset 37 is named **"Slap Bass"** and holds only *thumb* samples;
  - preset 38 is named **"Pop Bass"** and holds only *popped* samples.
  - Both are recorded on the same 9 string/fret spots (E0 E2 E4 A1 A3 D1 D3 G0 G5).
  - The thumb samples peak 4–7 ms after their start; the pop samples peak at 19–42 ms (the
    finger's hook comes before the snap). The pops are 4–14 dB brighter above 2 kHz (§1.3).
- **TimGM6mb and Jnsgm2** [T]:
  - Both build 38 from a lower filter base (580 Hz against 1.1 kHz) with a larger filter-envelope
    sweep (+4162 against +3037 cents).
  - The result is brighter than 37 from 30 ms on, and stays bright longer; in Jnsgm2 the 38 filter
    decays over 4.5 s.
  - In all three sets 38 is the brighter program [T].
- **The one written description** of the pair says Slap Bass 2 is "the same slap bass technique,
  but … a more compressed, squashed tone", with velocity driving the filter [web, Soundhouse; via
  bass_section.md §6.4]. It agrees with all of the above.
- **Velocity.** None of the three SF2s has velocity layers on 37 or 38: there are no `velRange`
  zones [T]. Through TiMidity, velocity is level only: +20 dB from 30 to 127, with an identical
  spectrum [T]. FluidSynth adds the SF2 default velocity-to-filter modulator, which TiMidity
  does not [memory].

**What this gives for pajupilli** (a proposal; the user decides by ear):
- **GM 37, Slap Bass 1: the thumb-led slap.** A two-pickup bass, both pickups full, DI-dominant through
  `slapDi`. Most notes are thumbed (with fret clank); pops only where a player must pop (octave
  and upward leaps onto the D/G strings, accented high notes). Ghost notes and hammer-ons as the
  line asks. The compressor is moderate.
- **GM 38, Slap Bass 2: pop-forward and squashed.** More notes popped, a brighter top, and the
  heavier compression and grit of `slapSquash`. The details belong to the parallel file;
  `research/slap_bass_2.md` decides.
- Both programs share one physical bass, player and fret-contact model. Only the player's
  thumb/pop bias and the chain differ. That matches the reference sets: FluidR3 splits the two by
  technique, one maker and the SoundFonts by brightness and compression.

---

## 1. The GM 37 patch in the reference implementations

### 1.1 one maker

- **MT-32 (LA synthesis, 1987).** "Slap Bass 1" (#69, "Nice slap") and "Slap Bass 2" (#70,
  "Variation") [web, synthmania MT-32]. These are the CM-64 map's bank 127 programs 69/70 on the
  SC-88Pro [web, SC-88Pro manual]. Whether the MT-32 used a PCM slap attack under an LA sustain
  is [memory, unverified]; the MT-32's PCM ROM holds short attack samples, and a slap transient
  is the kind of sample it holds.
- **sample-based module / CM-32P / sample-based module (PCM, 1988–89).**
  - The sample-based module's factory tones include "a clean electric bass played with slap technique" [web,
    search snippet of the sample-based module pages].
  - The CM-32P (the sample-based module's ROM in a CM box) provides the bank 126 "Slap Bass 1" and
    "Slap Bass 2" [web, SC-88Pro manual; the U-110 link is [memory]].
  - The sample-based module's slap tones: "SLAP 1 SINGLE: Thumped, pulled, harmonics (E2, F#4)" and "SLAP 2
    DETUNE: …". "SLAP 5 V-SW: Thumped/pulled (v = 100), harmonics (F#4)" [web, sample-based module manual;
    OCR-damaged, so the key names are approximate].
  - So one maker's PCM slap is **thumb at the bottom, popped ("pulled") notes above a split, and
    harmonics at the top**. "2" is the **detuned (two-voice) copy** of "1".
  - **[Inference]** The early GS module's GM 37 most likely descends from this family: one voice, thumb
    low and pop high by key split. The later GS module map's two-voice Slap Bass 2 continues the
    SINGLE/DETUNE pairing. The early GS module was not heard or measured here.
- **early GS module / later GS module / SC-88Pro.** For the voice counts and the GS variations, see §0 [web, SC-88Pro
  manual]. "Slap Pop" as a variation of 37 shows that one maker treated the pop as a *variant*: the
  main 37 is not pop-dominated.
- **Not found:** an early GS module recording or ROM analysis showing where 37's split lies and whether it
  switches by velocity. One maker's sample ROMs are not documented per tone.

### 1.2 XG

- 37 bank 0 "SlapBas1" (MSB 0, LSB 0, program 37 [web, Yamaha CLP data list]); bank 27
  "ResoSlap"; bank 32 "PunchThm" [web, polyphone].
- 38 bank 0 "SlapBas2"; bank 43 "VeloSlap", a velocity switch [web, polyphone]. One maker's bank 43
  is the "Velo-Switch" bank [web, Yamaha QY100 list via search].
- How many elements the MU-series SlapBas1/2 have, and which way they switch, was not read
  [not found].

### 1.3 The three SoundFonts, read from the files [T]

`/usr/share/sounds/sf2/FluidR3_GM.sf2`, `/usr/share/sounds/sf2/TimGM6mb.sf2`,
`~/soundfonts/Jnsgm2.sf2` (as in fx_sounds.md). The parser is
`…/scratchpad/slap1_sf2.py`; the sample analysis is `slap1_samp.py`.

**FluidR3_GM, preset 0:36 "Slap Bass" (GM 37)** [T]
- One instrument, "Slap Bass", one layer, **no velocity splits**. Nine key zones, each with its
  own sample and root:

| keys | sample (string, fret) | root |
|---|---|---|
| 0–28 | Slap Bass E0 | 28 (E1) |
| 29–30 | E2 | 30 |
| 31–32 | E4 | 32 |
| 33–35 | A1 | 35 (sic: A string fret 1 is 34) |
| 36–37 | A3 | 36 |
| 38–39 | D1 | 39 |
| 40–42 | D3 | 41 |
| 43–45 | G0 | 43 |
| **46–84** | G5 | 48 (C3), stretched up three octaves |

- **Nothing sounds above key 84.**
- The samples are 22.05 kHz mono, 4.9–5.9 s long, with a short loop at the end. Their names give
  the string and fret actually played: the set was sampled from a real bass at nine positions.
- Envelope:
  - a global hold of 3.0 s (1902 tc) with full sustain, then a decay of 40 s (keys ≤ 28) to 24 s
    (keys ≥ 46) to −100 dB;
  - a release of 0.20 s;
  - so the recorded decay itself is what is heard for the first 3 s;
  - reverb send 2 %.
- **Preset 0:37 "Pop Bass" (GM 38):** the same zone plan and envelopes, with samples "Pop E0 …
  Pop G5". Two zones add a hold of 7.1 s and 5.0 s.

The raw samples (first 10/30/100 ms, and the harmonic levels in 30–250 ms) [T]:

| sample | peak at | centroid 0–30 ms | 30–100 ms | 300–600 ms | energy > 2 kHz, 30–100 ms |
|---|---|---|---|---|---|
| Slap E0 (E1) | 7.0 ms | 202 Hz | 283 | 116 | −18.5 dB |
| Pop E0 | 18.9 ms | 247 | 433 | 188 | −14.0 |
| Slap A1 | 6.3 ms | 224 | 298 | 209 | −19.1 |
| Pop A1 | 42.3 ms | 399 | 433 | 231 | −12.5 |
| Slap D1 | 5.7 ms | 523 | 342 | 217 | −13.5 |
| Pop D1 | 37.2 ms | 532 | 1075 | 463 | −6.6 |
| Slap G0 (G2) | 3.8 ms | 288 | 534 | 308 | −10.9 |
| Pop G0 | 30.2 ms | 1185 | 1743 | 677 | −5.0 |
| Slap G5 (C3) | 5.1 ms | 499 | 333 | 159 | −15.0 |
| Pop G5 | 30.5 ms | 666 | 1833 | 480 | −5.3 |

- **Thumb:** the peak comes 4–7 ms after the sample starts. The first 10 ms are the loudest
  (rms −2 dB, falling to −10 by 20 ms): a thud followed by a falling, mid-bright body.
  Harmonics 4–10 of A1 lie 22–48 dB under the fundamental, a deep comb notch from where the thumb
  struck. Above that the 11th–15th partials come back to −9 to −12 dB: the fret clank's band.
- **Pop:** the first 10 ms are quiet (rms −10…−17 dB, almost nothing above 2 kHz: the finger
  hooking the string). The snap comes 19–42 ms in. After it, 2–4 kHz carries 5–10 dB more than in
  the thumb, and the upper partials ring longer (at 0.8–1.2 s the 7th–10th partials of Pop G0 are
  still at −4…−11 dB).
- **Use for pajupilli:** pops need *lead time*. The hook and pull before the release take about
  15–40 ms, and the note's onset is the release. A player model must start a pop that much early
  (the `noteLead` mechanism, `src/instrument.h:216`), or the pops will be late against the thumbs.

**TimGM6mb, preset 0:36 "Slap Bass 1"** [T]
- One layer, six key zones, no velocity splits.
- Samples (44.1 kHz) are very short. "Slap Bass 1 A0" is 0.18 s (root 43, keys 0–37); "D1" is
  0.10 s (root 60, keys 38–71). Both are an attack plus a few-cycle loop. "D4" and "C6" are
  single cycles at the top.
- The thumb's thud is in the sample: the peak comes at 0.7–1.1 ms, and the 0–10 ms centroid is
  1.3–1.9 kHz.
- **Filter** (all zones): a base of 8502 abs cents = **1110 Hz**; the mod envelope adds +2700 to
  +3037 cents (peak ≈ **6.4 kHz**), decays in 0.5–0.6 s and sustains at 30 % (≈ 1.9 kHz).
- **Volume:** a hold of 0.14 s, then a decay of 20 s (low keys) to 7 s (high keys) towards a
  sustain 33.8 dB down; the release is 0.07 s; −1.6…−2.0 dB of attenuation.
- **Preset 0:37 "Slap Bass 2":** its own samples. The base is **579 Hz** and the sweep +3600…+4162
  cents (peak ≈ 6.4 kHz), decaying in 0.53 s to a 37 % sustain (≈ 1.4 kHz). The release is
  0.15 s.

**Jnsgm2, preset 0:36 "Slap Bass 1"** [T]
- Three zones: "SLPBS31" (keys 0–38, −7.5 dB), "SLPBS43" (39–67) and "SLPBS84" (68–127). 22 kHz,
  0.25–0.34 s attack and loop.
- Filter: a base of 729 Hz, a sweep of +3600 cents (5.8 kHz), decaying in 0.71 s to a 22 %
  sustain.
- Volume: a decay of 20 s to −27 dB; the release is 0.02 s.
- **Jnsgm2's 38 is TimGM6mb's 38 with longer samples.** The base is 1.8 kHz, the mod sustain 100 %,
  and the filter decays over 1.6–4.5 s, so its brightness lasts.

### 1.4 Rendered through TiMidity and measured [T]

Keys 28, 33, 40, 45, 52, 59 at velocities 30/60/90/127, each held 2 s; reverb and chorus off;
44.1 kHz mono. The script is `slap1_render.py` and the full output `slap1_render.txt`. "Bands" are
the octave bands 20–60 … 8–16 kHz of the first 30 ms, in dB under the strongest. "Decay" is from
0.3 to 1.5 s.

| set, program | centroid 0–30 ms (keys 28→59) | 30–100 ms | 300–600 ms | decay | velocity 30→127 |
|---|---|---|---|---|---|
| FluidR3 37 (thumb) | 231–1002 Hz | 283–593 | 116–347 | 3.6–6.1 dB/s | +20.8 dB, spectrum identical |
| FluidR3 38 (pop) | 275–2286 | 414–2311 | 187–810 | 3.7–6.1 | +20.8 dB (0–30 ms centroid falls a little with velocity) |
| TimGM6mb 37 | 362–509 | 165–375 | 59–373 | 1.6–2.4 | +20.8 dB |
| TimGM6mb 38 | 240–906 | 247–985 | 220–987 | 1.6–3.7 | +20.8 dB |
| Jnsgm2 37 | 423–1302 | 334–686 | 106–679 | 1.3–1.4 | +20.8 dB |
| Jnsgm2 38 | 238–1067 | 246–1004 | 165–649 | 1.6–2.8 | +20.8 dB |

- **37 against 38, by set** [T]:
  - **FluidR3:** 38's 30–100 ms centroid is 1.4–3.5× 37's (pop against thumb).
  - **TimGM6mb:** 38 is 1.5–2.6× brighter at 30–100 ms and 2.6–3.7× at 300–600 ms.
  - **Jnsgm2:** 38 is brighter from key 45 up; below that 37's thud is brighter in the first
    30 ms.
- **The thumb's character in the measurements** [T]: TimGM6mb's and Jnsgm2's 37 is brightest
  *at the very start* and goes dark quickly (centroid 500 → 166 → 60 Hz at E1). That is a thud
  and click, then a round body. 38 rises or holds.
- **The level** is the same for 37 and 38 within 0.3–1.5 dB at equal velocity, in every set [T].
  bass_amps.md 7.1 had found FluidR3's slaps 7.7–8.4 dB louder than its finger bass.
- **The decay** of 37 is slow (1.3–6 dB/s): neither set gates the note short. A real slap note's
  length comes from the player damping it (§2.4).

---

## 2. The physics of slap

### 2.1 The thumb

- **Where:** on the E and A strings "near the base of the bass's neck" [web, Wikipedia],
  bouncing off the string with the bony side of the thumb joint [web, Wikipedia; bass_section.md
  §6.3]. On a J (34" scale) the neck ends at the 20th fret, 272 mm from the saddle [E: 864 mm ×
  2^(−20/12)]. The thumb strikes at about 240–290 mm from the saddle, over or just past the neck's
  end [memory, lore].
- **What it is mechanically:** a hammer that strikes and rebounds. Rank & Kubin model it as a
  velocity pulse and note that the thumb is a second contact that comes back, which they did not
  model [paper, Rank97; bass_section.md §6.3].
- **The rebound stroke** (thumb bounces off) and the **rest stroke** (thumb follows through and
  stops on the next string) are both used [memory, lore]. The rest stroke also mutes the next
  string.
- **What the scratch model shows** (Appendix A) [T]:
  - with a 20 g thumb at 1.5–5 m/s and a flesh spring of 3·10⁴ N/m, 15 mm wide, at 260 mm on
    the open E, the thumb drives the string onto the **20th fret within 1.1–3.7 ms**;
  - it **pins it there for most of the contact**: the thumb touches for 4.2 ms at 3–5 m/s and
    9.6 ms at 1.5 m/s, and the fret is in contact in 43–58 % of the first 10 ms;
  - then the string's rebound rattles on the low frets (1–3 on an open string, the frets just
    above the stopping fret on a fretted one) for 100–300 ms, sparsely;
  - the peak force is 13 N at 1.5 m/s, 37 N at 3 m/s and 67 N at 5 m/s;
  - **without frets** the same blow gives a dull pluck: the 30–100 ms centroid is 164 Hz, the
    2–4 kHz band −34 dB and 4–8 kHz −58 dB. **With frets:** 1.7 kHz, −12 dB and −13 dB.
  - The fret contact *is* the slap's brightness, as Rank & Kubin and Issanchou say.

### 2.2 The pop

- An index or middle finger hooks under the D or G string and "snaps [it] away from the body so
  [it] bounce[s] off the fretboard" [web, Wikipedia]. Rank & Kubin model it as a large triangular
  initial displacement [paper, Rank97].
- **Hook and pull:** 15–40 ms in FluidR3's pop samples (the time to the peak) [T]. The finger
  damps what the string was sounding. Release is by slipping off a callused fingertip, a fraction
  of a millisecond [design: as `Guitar::pluckPad`'s slip].
- **Depth** [memory; design]: 2–6 mm away from the body, at 180–230 mm from the saddle
  (between the end of the neck and the neck pickup).
- **What the scratch model shows** (Appendix A) [T], G or D string popped 2–4 mm at 200 mm, released at
  once:
  - the first fret contact comes 2.8–4.6 ms after release (the string swings through its rest
    position to the other side);
  - dense contacts last **40–60 ms**. Issanchou measured contacts within 0–90 ms for a 3.6 mm
    initial amplitude on a G string [paper, Issanchou18; bass_section.md §6.3];
  - the frets touched are those nearest the middle of the swing (3–5 open, 7–9 with the G
    fretted at 5);
  - 2–4 kHz goes from −19 dB (no frets) to 0 dB, and 4–8 kHz from −22 to −5 dB (G, 4 mm);
  - the 0–30 ms centroid goes from 311 Hz to 2.7 kHz.
- **The audible difference from the thumb** [T, sims and FluidR3]:
  - a pop puts its brightness in the snap and in the first ~50 ms of contacts, higher (2–8 kHz)
    and on a thinner string;
  - a thumb's brightness is a pinned clank (a few ms against the last fret) plus a low-fret rattle
    on the thick strings, 0.5–2 kHz;
  - the pop is late (the hook) and louder: popped notes in a MIDI line are 6–25 velocity steps over
    their thumbed root (§4).

### 2.3 String–fret collision: the literature

- **Rank & Kubin 1997** [paper, Rank97]. A waveguide string, with the fret contacts as conditional
  reflections. They need the contact at least at the last fret during the first period, and fret
  "noise" in the first two periods. Their real-time cost on 1997 hardware: about 800 delay units,
  contacts thinned to one per 8 sections. Details are in bass_section.md §6.3.
- **Trautmann & Rabenstein 2004** [paper, TR04], "Multirate simulations of string vibrations
  including nonlinear fret-string interactions using the functional transformation method",
  EURASIP JASP 2004(7):949–963:
  - a modal (FTM) string with the frets as a nonlinear interaction, solved at audio rate;
  - a multirate variant cuts the cost by almost 80 % for a six-string guitar, with little loss of
    accuracy and staying stable [web, DOAJ abstract];
  - it is the closest published relative of pajupilli's modal `Guitar`.
- **Issanchou, Bilbao, Le Carrou, Touzé, Doaré 2017–18** [paper, Issanchou17; Issanchou18]:
  - a modal string against point (fret) and distributed obstacles, with an unconditionally stable
    scheme (energy-conserving, iterative);
  - measured against a real electric bass: G string, contacts 0–0.09 s, high-frequency content
    from the first collision, a reinforced band near 4.5 kHz;
  - the restitution coefficient hardly matters for point contacts (bass_section.md §6.3).
- **Ducceschi, Bilbao, Willemsen, Serafin 2021** [paper, Ducceschi21], "Linearly-implicit
  schemes for collisions in musical acoustics based on energy quadratisation", JASA 149(5):
  - the collision potential is rewritten with an auxiliary variable (IEQ/SAV), so each step solves
    one linear system and no Newton iteration;
  - energy-stable;
  - it generalises to modal discretisations; its examples include strings on fretboards and
    fingers [web, abstract].
- **Bilbao, Russo, Webb, Ducceschi, DAFx 2024, "Real-time guitar synthesis"** [paper, DAFx24,
  read]:
  - an FD string with the geometric nonlinearity, a smooth fretboard, **20 point frets** (Hertzian
    F = K_F[m₀ − Ju]₊^α_F) and a finger, with the SAV update and a Woodbury/Sherman–Morrison solve;
  - one second of audio (44100 steps) costs **0.09–0.18 s on a Xeon E5, 0.06–0.13 s on an M1,
    0.04–0.11 s on an M2 Pro** per string. Real time for six strings on one core;
  - per step, for the low E: the fretboard 19 %, the frets 7.7 %, the finger 5 %, and the SAV
    update and solve 64 %.
  - So collisions with 20 frets are affordable even in FD. In pajupilli's modal form the contact
    check is a dot product per fret (§5.2).
- **Other related work** [memory]:
  - Evangelista & Eckerholm, IEEE TASLP 2010: player–string interaction and collisions in
    waveguides;
  - Chatziioannou & van Walstijn, JSV 2015: energy-conserving contact schemes;
  - van Walstijn & Bridges, EUSIPCO 2016: distributed contact in a modal string, the same
    structure as `Guitar`;
  - Kramer et al., ICASSP 2012: a waveguide bass with 11 techniques, slap included (not opened;
    bass_section.md §12).

### 2.4 Ghost notes, left-hand slaps, hammer-ons [web, Wikipedia; memory]

- **Ghost (dead) notes:** "notes played with the string damped" [web, Wikipedia]: the left hand
  lies on the string without pressing it, and the thumb or pop gives a pitchless click. IDMT
  calls them "dead notes: a percussively sounding note with almost no harmonic components"
  [paper, Abeßer13; bass_section.md §6.5].
- **The left-hand slap** [memory]: the fretting hand's fingers slap the strings flat against the
  frets, a muted, broadband "chk" between a thumb and a pop (Mark King's triplets: thumb,
  left-hand slap, pop).
- **Hammer-ons and pull-offs** raise the speed: "pull-offs and hammer-ons with the fretting hand"
  [web, Wikipedia]. A pop followed by a hammer-on a tone or minor third up is a stock figure
  [memory].
- **Double thumbing** (Wooten): the thumb down through the string and then up, using "both sides
  of the thumb" [web, Wikipedia]. That makes fast repeated thumb notes, the upstroke weaker and
  brighter [memory].
- **Note length:** a slap line is mostly short. The left hand releases or the right hand mutes, so
  the note's length is its MIDI length plus a quick damp (bass_section.md §6.5: −30 dB in
  50–150 ms).

### 2.5 Contact durations and spectra: the numbers to aim at

| quantity | value | source |
|---|---|---|
| contacts after a slap or pop | 0–90 ms (G, 3.6 mm) | [paper, Issanchou18] |
| reinforced band | ~4.5 kHz | [paper, Issanchou18] |
| fret contact needed | at least the first period at the last fret | [paper, Rank97] |
| thumb on the string | 4–10 ms (1.5–5 m/s, a 20 g thumb, flesh 3·10⁴ N/m) | [T, model; the thumb's parameters are assumed] |
| first fret contact after a thumb | 1.1–3.7 ms | [T, model] |
| first fret contact after a pop | 2.8–4.6 ms | [T, model] |
| pop hook and pull before the snap | 15–40 ms | [T, FluidR3 samples] |
| pop brighter than thumb at 30–100 ms, > 2 kHz | +4…+9 dB (FluidR3, same note) | [T] |
| slap lines and low-pass | slap is the technique most hurt by a 3.5 kHz low-pass | [paper, Abeßer13 via bass_section] |

---

## 3. Players and style

### 3.1 Who, and on what [memory unless marked]

- **Larry Graham** (Sly and the Family Stone, late 1960s; Graham Central Station). He is credited
  with inventing slap on electric bass; he was "trying to emulate a drum set" before his band had
  a drummer; he calls it **"thumpin' and pluckin'"** [web, Wikipedia]. He played an American amp Jazz
  Bass, later Moon basses.
- **Louis Johnson** (The Brothers Johnson, "Thunder Thumbs"; Quincy Jones and Michael Jackson
  sessions) played a **active single-humbucker bass**: a single humbucker near the bridge and an active
  2-band EQ. It is one of the archetypal slap sounds, punchy in the upper mids.
- **Marcus Miller:** a **1977 two-pickup bass**, ash body, maple board, with a **Bartolini
  preamp** fitted by Roger one bass maker. Both pickups full, bass and treble boosted ("smile"), usually
  DI'd. This is the J slap of the 1980s and of GM's era.
- **Mark King** (Level 42): fast 16th-note thumb with left-hand slaps and pops ("machine-gun"
  triplets). Jaydee and one bass maker basses.
- **Flea** (RHCP): an aggressive thumb on active single-humbucker bass, one bass maker and Modulus basses, later American amp J.
- **Victor Wooten:** double thumbing, open-hammer patterns; Fodera.
- **Stanley Clarke** and **Bootsy Collins** too. Stanley Clarke played a one bass maker with a bright
  active tone.

**For GM 37:** the GM sets come from 1987–91 studio practice. Both the J (Miller) and the active single-humbucker bass
(Johnson) are typical. pajupilli has the J built, and bass_section.md §10 and bass_amps.md §2.2
already chose the J for 37 and 38. An active single-humbucker bass spec (one humbucker, active EQ) would be a later
variant, not needed for 37.

### 3.2 Patterns [memory, lore]

- **The octave pop:** the thumb on a root on the E or A string, the pop on the octave two strings
  up, fretted two frets higher (a span of index to ring or pinky). Often as eighths or 16ths:
  T P T P, or T g P g with ghosts.
- **Thumb on the beat, pop on the off-beat** (16th funk): T – g – P – g, accented pops.
- **Rakes and triplets:** thumb, left-hand slap, pop (King); thumb, pop, hammer.
- **Pop then hammer** (minor third or tone up), and **slide into the octave pop**.
- **Popped double stops** (two fingers popping D and G together): the 10th or the 7th over the
  thumbed root.
- **Open-string thumb pedals** (open E or A between fretted notes).

### 3.3 What this means for reading MIDI

- **Thumb:** the lowest notes, roots, notes on the E and A strings, notes on strong beats, and
  repeated notes. **Pop:** notes on the D and G strings that leap up from the last thumbed note
  (by 7–15 semitones, an octave above all), and accented high notes. Ghosts: low velocity and
  short. Hammer-ons: a short step up, legato, softer.
- Velocities can't be relied on: one of the user's files has 95 % of its slap notes at 100–127
  (§4). Register and context are the main cues. Velocity only refines them.

---

## 4. Usage in the user's GM files [T]

Scanned: `~/midi` (44 files), `~/Music` (309), `~/Downloads` (44); `~/Musici` does not exist.
The script is `slap1_scan.py`. There are only **three slap tracks**, all in `~/Music/midi/bushgrafts/`:

| file | program | notes | pitch (median) | velocity | durations | notes |
|---|---|---|---|---|---|---|
| louisiana.mid | **37** | 723 | 29–54 (38 = D2) | 20–103, median 62; 16 % < 50 | median 0.12 s; 31 % < 80 ms | 10 % ghost-like (v < 50 and < 0.1 s); IOI median 0.32 s |
| Gaviota trio 2.mid | **37** | 510 | 29–59 (41) | **95 % at 100–127** (flat) | median 0.27 s; 13 % < 80 ms | no ghosts; 7 % octave steps |
| grelots.mid | 38 | 361 | 29–55 (43) | median 85 | median 0.47 s | longer notes; 4 % ghosts |

- **louisiana** opens with the octave figure: 35/76 47/88 35/73 47/92 31/64 43/92 31/66 43/90
  (key/velocity). The upper octave is **+12…+28 velocity** over its root: thumb and pop, written
  in. Further on it has short low notes (0.03–0.08 s, v 42–60) between accented ones: ghosts and
  rakes.
- **Gaviota trio 2** is flat at v ≈ 110, with octave steps (36→48, 43→55-type) and chromatic
  approaches (39 40 41 42 43). Thumb/pop must come from register and context, not velocity.
- **Range:** F1 (29) to B3 (59). Everything fits a 4-string J at frets 0–17. The top keys use the
  G string's 12th–16th frets, where popping is natural.
- **Durations:** a third of louisiana's notes are under 80 ms, so the note-off damp matters as
  much as the attack.

---

## 5. Build plan

### 5.1 What to reuse

- **`Guitar` (`src/guitar.*`)**: the modal string with two polarisations, exact per-mode rotation,
  `kMaxModes` = 128, the J pickups with the magnet's nonlinearity, `pluckPad`, `mute`, `damp`,
  `fret`, `bend`, `touch` (harmonics).
  - With the E string's B = 2·10⁻⁴, 128 modes reach **~10.9 kHz** on the open E
    (f_k = k f₀ √(1+Bk²)), and further on the others [E, computed].
  - That covers the slap's 4–8 kHz without raising `kMaxModes`. Raise `jazzBass().topHz` from
    5 kHz to ~10 kHz for the slap programs only.
- **`jazzBass()` (`src/ebass.cc`)**: the strings, losses, neck dead spot and pickups as built for
  GM 34. Both pickups full.
- **`ElectricBassModel` / `ElectricBassPlayer`**: the fingering solver (string and fret choice,
  hand position), note-off damping, the one-voice line, and the double stops. Add a technique
  choice per note.
- **`GuitarAmp::slapDi()`**: the chain for 37 as specified in bass_amps.md 10.4. It is already
  coded.
- **`Bass`'s fingerboard contact (`src/bass.cc` ~315–330 and ~765–810)**: the pattern for
  contact points in a modal string: precomputed mode shapes at the points, clearances, a
  `boardOn` gate and a clock. Its explicit penalty with a "hold" limiter shows the stability
  issue; the frets need the implicit projection below.
- **The piano hammer (`src/piano.h`)**: the pattern for a mass with a felt-like spring, for the
  thumb.
- **`noteLead`**: lets the player start the pop's hook early so that the snap lands on time.

### 5.2 The fret-collision model (fits the real-time budget)

**Geometry** (per string; x is measured from the saddle):
- The fret crowns are a plane, with the frets at x_q = L·2^(−q/12), q = 1..20.
- The **relief** is a parabola between the nut and the 20th fret, 0.3 mm at its deepest
  [memory: Fender's ~0.3 mm].
- The **saddle height** H_s is set from the **action at the 17th fret**: 2.4 mm by default
  (`PJ_SLAP_ACTION`). Slap players set it low, ~1.8–2.4 mm [memory].
- The **open string** runs from H_s at the saddle to 0.5 mm over the first fret at the nut.
- A **fretted string** runs from H_s to the crown of its stopping fret, so the next fret up has
  only ~0.06·H_s clearance (Appendix A's endless-buzz caveat).
- Each fret crown gets its own ±0.03 mm and each instance its own relief, to vary buzz and
  dead spots per bass.

**Contact** (the frets act only on the motion towards the board):
- Let y_q = Σ_k φ_k(x_q)·(ct_k q₀_k − st_k q₁_k), the string's displacement towards the top at
  fret q, through each mode's tilted axes.
- **After each sample's free modal update** (the exact rotation already in `Guitar::sample`), for
  each fret q above the stopping fret, with penetration p = −y_q − c_q > 0:
  - apply the impulse I = p / g_q, with g_q = Σ_k φ_k(x_q)² (sn_k/ω_k)/m_k precomputed per
    stopped length;
  - write it back as q_k += I φ_k (sn_k/ω_k)/m_k and v_k += I φ_k cs_k/m_k, on the towards-top
    axis of each mode.
- This is the **rigid, inelastic projection**: the closed-form limit of the linear-implicit
  (Sherman–Morrison) solve in DAFx24 and Ducceschi21.
- Two Gauss–Seidel sweeps over the touching frets handle frets that couple through the modes.
- Optionally use a finite stiffness, F = K p/(1 + K g_q) with K ≈ 10⁶ N/m. The modal string's
  effective mass at a point is ~0.2 g with 128 modes, so K = 10⁶ puts the contact resonance near
  11 kHz.
- The scratch model (Appendix A) ran stably with the rigid projection at 48 kHz in every case tried
  [T].
- **No restitution.** Issanchou found it hardly matters [paper].

**Cost:**
- The check is 20 frets × 128 modes = 2560 MACs per string per sample (fewer: only the frets
  above the stopping fret, and the modes in use). As AVX float that is ~320 FMAs, ~0.1–0.2 µs,
  i.e. **~0.5–1 % of a core per string at 48 kHz** while it runs. Each touching fret adds two more
  dot products.
- **The gate:** every 16 samples, bound |y_q| ≤ Σ_k |q_k||φ_k(x_q)|. Skip the check while the
  bound is under the smallest clearance, or more than ~0.4 s after the last stroke when nothing
  touched in the last 50 ms.
- A finger or pick note at GM 34 levels rarely enters the check at all.

**Optional extras:**
- Apply the same contact to `ElectricBassPlayer` for GM 34 (buzz on hard finger notes),
  bass_section.md §6.5.
- A fret "tick" is not needed: the frets themselves hardly ring.

### 5.3 The thumb (new: `Guitar::thump`)

- **A mass and spring** (as the piano hammer):
  - the thumb's effective mass m_t = 15–25 g (20 by default), starting 2 mm above the string at
    speed v_t;
  - the flesh is a spring at 3·10⁴ N/m. Hertz with α = 1.5 is optional, softening light touches;
  - it acts over a Gaussian 15 mm wide (σ ≈ 4 mm) on the towards-top axis, at an angle of 1.2–1.4
    rad (mostly into the body, some along it).
- **Speed from velocity:** v_t = 1.0 + 3.5·(v/127)^1.3 m/s, about 1–4.5 m/s, scaled by the
  dynamics and ±7 % per stroke.
- **Place:** 0.24–0.29 m from the saddle, scaled by the brightness CC: nearer the neck is
  rounder. Jitter ±8 mm.
- **The rebound stroke** (default): from 2 ms after contact, a lifting acceleration of ~30 m/s²
  (the forearm's rotation), plus the flesh's own push.
- **The rest stroke** (for slow, heavy notes on the E): the thumb falls on to the next string up
  and damps it (`damp(s+1, …)`).
- **Model results** at these values [T]: on for 4–10 ms; the string pinned on the 20th fret
  through most of it; fret clank 1–4 ms in; peak 13–67 N.
- **Integration:** explicit is enough. The flesh against the string's effective mass under a
  15 mm contact sits at ≤ 2 kHz. Same scheme as `pluckPad`: the force enters as v += im·F·shape.
- **Double thumbing:** the upstroke is the same thumb from the other side, angle −1.3 (away from
  the body), at 60–70 % of the speed, using the nail side (narrower, 8 mm). It is used when
  thumbed notes come less than 110 ms apart.

### 5.4 The pop (reuse `pluckPad`)

- `pluckPad(s, depth, from, angle, width, rise, slip)`, with:
  - angle −1.45 ± 0.08 rad (away from the body);
  - depth 1.5–5.5 mm, from 1.5 + 4·(v/127)^1.2 mm;
  - from 0.18–0.23 m;
  - width 6 mm;
  - rise 12–25 ms (the hook and pull, slower on soft pops);
  - slip 0.15–0.35 ms (faster on hard pops).
- **The finger lands** on the string before the pull, stopping its sound (`hold`, as
  `pluckPad`'s plant).
- **Timing:** the release lands on the note's time, so the hook starts *rise* ms before.
  `ElectricBassPlayer` asks for `noteLead` ≥ 25 ms on the slap programs. Live (no lead), the pop
  sounds ~15 ms late, as on a real bass.
- **Model results** at 2–4 mm [T]: first fret contact 3–5 ms after release; dense contacts for
  40–60 ms; 2–4 kHz up ~19 dB against no frets.
- **Popped double stops:** two `pluckPad`s 3–8 ms apart (index and middle).

### 5.5 The player: thumb or pop, ghosts, hammers (the logic, with numbers)

A per-note decision in `ElectricBassPlayer::play()` after the string and fret are chosen; it
needs a short look-back (≈ 0.5 s):
1. **Ghost / dead note** if velocity < 40, or (velocity < 55 and duration < 90 ms when the
   length is known, otherwise IOI < 90 ms):
   - the left hand mutes the string (`mute` at the fret's position, deep: the fundamental
     loses ~200 /s);
   - then the stroke the pattern would have given: a soft thumb at 1–1.5 m/s, or a pop at
     1–1.5 mm;
   - there is no pitch to fret: keep the hand where it is.
2. **Hammer-on** when:
   - the note is 1–3 semitones above the previous one on the same string;
   - it starts < 140 ms after the previous note, which is still held (legato);
   - and its velocity is not over the previous + 10.
   Then there is no right-hand stroke: `fret()` while ringing, plus a small finger impulse at the
   new fret (a 5 g fingertip at ~1 m/s, 6 mm wide). **Pull-off** is the mirror case (a small
   pluck sideways as the finger leaves).
3. **Pop** if the note is on the D or G string, and any of:
   - it lies **7–16 semitones above the last thumbed note** within 0.6 s (the octave pop; 10ths,
     7ths);
   - it is **≥ 12 semitones above the bar's lowest note** and accented (velocity ≥ the running mean
     + 8);
   - it is a chord tone of a double stop on D and G.
   - **GM 37's bias:** pop only on these rules (`PJ_SLAP_POP` = 1: the threshold as stated).
     For GM 38, slap_bass_2.md may lower the thresholds (e.g. a 5-semitone leap, or anything
     ≥ D3).
4. **Thumb** otherwise:
   - the E and A strings always;
   - D and G notes that do not leap (a run up the neck stays thumbed);
   - fast repeated thumb notes (IOI < 110 ms) alternate down and up strokes (double thumbing).
5. **Left-hand slap** (optional, later): ghost notes that fall between a thumb and a pop within
   a 16th triplet grid become a left-hand slap. All four strings are muted under the fingers and
   pressed to the frets, with an 8–15 ms broadband contact burst from the fret model.
6. **Damping:**
   - note-off: the fretting hand lifts and the right hand lands, at 40–80 /s (shorter than finger
     style; slap lines are clipped);
   - open strings: damped by the thumb's next landing or by the palm.
7. **Variation:** speed, place and angle get a per-stroke jitter (±7 %, ±8 mm, ±0.08 rad), so no
   two slaps are the same (bass_section.md §9.4 item 7).

Checked against the user's files, the rules give these targets [T, by counting]:
- louisiana: octave figures popped at roughly 10–20 % of notes; ghost rule ~10 %;
- Gaviota: a few pops (octave steps 7 %), and the rest thumb;
- grelots on 38: more pops under 38's bias.

### 5.6 The amp chain (reuse)

- **J, both pickups full** (`PJ_EBASS_PICKUP` 2) → **`slapDi`** [bass_amps.md 10.4, coded]. That
  is:
  - 0.9 MΩ splitter;
  - the DI with the VPF at noon (+4 dB at 35 Hz, −6 dB at 380 Hz, +4 dB at 10 kHz), 75 %;
  - a GK-like clean head with Mid Contour into a 4×10 + horn, 25 %;
  - a FET compressor at 4:1, 0.8 ms / 150 ms.
- **An active preamp on the bass** (Bartolini-like, Miller): a +6 dB low shelf at 40 Hz and a
  +6 dB high shelf at 3.5 kHz before the cable, i.e. the "smile". It is already roughly the DI's
  VPF, so add it only if the ear asks (a knob, default 0).
- **Compression target:** pops within ~4 dB of thumbs after the FET compressor (bass_amps.md 10.6 check 7).
  GM 38's `slapSquash` squeezes to ≤ 2 dB and adds grit: **that is the main audible difference
  between 37 and 38 besides the thumb/pop balance.**

### 5.7 Mapping and names

- `-inst slap-bass-1`: `ElectricBassModel` with `jazzBass()` (topHz 10 kHz, the frets as
  contacts), the slap player (thumb bias), and `GuitarAmp::slapDi()`.
- `midifile.cc:192`: `{"slap-bass-1"}` without the double-bass fallback.
- Knobs:
  - `PJ_SLAP_ACTION` (the 17th-fret action, 2.4 mm), `PJ_SLAP_RELIEF` (0.3 mm);
  - `PJ_SLAP_POP` (the pop bias), `PJ_SLAP_THUMBV` (speed scale), `PJ_SLAP_THUMBAT` (place);
  - `PJ_SLAP_FRETS` = 0 to hear the slap without frets.

### 5.8 Tests (`tools/test.py`)

1. **Stability.** 2,000 random thumbs and pops (all strings, frets 0–17, the full velocity range)
   with no NaN or inf.
   - With losses off, the string's energy never increases at a contact.
   - The largest penetration after projection is < 0.02 mm.
2. **Contacts happen when and where they should** (E1 thumb v100, G2 pop v100):
   - the first contact 1–4 ms after the strike (thumb) or the release (pop);
   - the contact share over 0–10 ms ≥ 30 % for the thumb;
   - the pop's dense contacts end within 30–100 ms (Issanchou: 0–90 ms);
   - no contact on a v40 finger note (GM 34 unchanged).
3. **The frets make the slap** (`PJ_SLAP_FRETS` 0 against 1, at the DI):
   - the 30–100 ms centroid at least 4× higher with frets;
   - 2–4 kHz in 0–100 ms at least +10 dB.
4. **Thumb against pop on the same note** (D2, G2, C3; at the DI):
   - the pop's energy above 2 kHz in 30–100 ms is 4–10 dB over the thumb's (FluidR3: +4…+9 dB);
   - the thumb's level is highest in the first 10 ms;
   - the pop's snap is at the note time with `noteLead`, or 12–25 ms late without it.
5. **Against the GM sets** (after `slapDi`, keys 28–59):
   - the 30–100 ms centroid of thumbed notes is 250–700 Hz (FluidR3 37: 283–593, TimGM6mb
     165–375, Jnsgm2 334–686);
   - popped notes are 0.9–2.5 kHz (FluidR3 38: 414–2311);
   - the level at v120 is +7…+9 dB over GM 34 (bass_amps.md 7.1).
6. **Velocity is brightness as well as level**, unlike the SF2s:
   - v40 → v120 at the DI: +16…+20 dB;
   - the 0–30 ms centroid rises ≥ 1.5×;
   - after `slapDi`: 8–10 dB.
7. **Every slap different:** ten identical E1 v100 thumbs; the 0–30 ms centroid spread ≥ 5 %, the
   level spread 0.5–1.5 dB.
8. **The player's choices** (counted, no audio):
   - louisiana.mid: 8–25 % pops, octave-up figures popped, 5–15 % ghosts, the hammer-on rule
     firing only on legato steps;
   - Gaviota trio 2: ≤ 15 % pops;
   - no pop on the E or A strings.
9. **Damping:** a 0.1 s note: −30 dB within 120 ms of its note-off.
10. **Cost:** a dense 16th-note slap line at 120 bpm, rendered offline. The slap bass ≤ 1.5× the
    finger bass of GM 34; contact work ≤ 1 % of a core per ringing string.
11. **Fitting material:** the IDMT-SMT-BASS single notes, slap-thumb (ST) and slap-pluck (SP)
    (bass_section.md §6.3). Fit the thumb's speed/place and the pop's depth so that the 0–100 ms
    octave-band shapes match within 3 dB.

### 5.9 Order of work

1. The frets as contacts in `Guitar` (with the gate) and test 1–3 on plain hard plucks.
2. `thump` and the pop settings; tests 4–7.
3. The player's technique choice (ghosts, hammers, double thumb); tests 8–9.
4. Hook up `-inst slap-bass-1` with `slapDi`; tests 5, 10; listen against FluidR3 37 and
   TimGM6mb 37 with the user, then hand the shared parts to GM 38.

---

## Appendix A: the scratch modal model behind the [T] contact numbers

`…/scratchpad/slap1_sim.py`, numpy, 48 kHz:
- **The string:** one polarisation (towards the board), 128 modes, the J's E/A/D/G strings from
  `jazzBass()` (T, f₀, B, losses).
- **The frets:** the geometry above (17th-fret action 2.4 mm, relief 0.3 mm, nut 0.5 mm) with the
  rigid inelastic projection and two Gauss–Seidel sweeps.
- **The thumb:** a 20 g mass with a linear 3·10⁴ N/m flesh, Gaussian over 15 mm, at 260 mm,
  lifting at 30 m/s² from 2 ms.
- **The pop:** a released triangle at 200–210 mm.
- **The output:** the string velocity at 0.6 × the neck pickup + 0.4 × the bridge pickup. No coil,
  no amp, so its centroids are *not* comparable with the SF2 numbers. Only its with/without-frets
  differences and its timings are used above.

Results:

| case | first contact | dense contacts until | contact share 0–10 / 10–50 / 50–150 ms | frets mostly hit | thumb on, peak | centroid 0–30 / 30–100 / 300–600 ms |
|---|---|---|---|---|---|---|
| thumb E open 1.5 m/s | 3.7 ms | 290 ms | 58 / 2 / 0 % | 20, 3, 1 | 9.6 ms, 13 N | 645 / 684 / 271 Hz |
| thumb E open 3 m/s | 1.8 | 190 | 56 / 4 / 1 | 20, 1, 2 | 4.6, 37 | 769 / 1713 / 928 |
| thumb E open 3 m/s, no frets | — | — | — | — | 18.9, 16 | 157 / 164 / 140 |
| thumb E open 5 m/s | 1.1 | 600 (sparse rattle) | 55 / 6 / 3 | 20, 1, 2 | 4.2, 67 | 793 / 1675 / 952 |
| thumb A open 3 m/s | 1.8 | 130 | 48 / 5 / 2 | 20, 1, 2 | 4.5, 39 | 1003 / 2034 / 706 |
| thumb E fret 5, 3 m/s | 1.6 | 500 | 43 / 5 / 3 | 20, 6, 7 | 4.5, 38 | 1055 / 2248 / 783 |
| pop G open 2 mm | 4.6 | 50 | 2 / 2 / 0 | 4, 3, 5 | — | 794 / 903 / 227 |
| pop G open 4 mm | 3.5 | 50 | 8 / 3 / 0 | 5, 3, 4 | — | 2698 / 2206 / 276 |
| pop G open 4 mm, no frets | — | — | — | — | — | 311 / 265 / 203 |
| pop D fret 2 (E2) 3 mm | 4.5 | 60 | 4 / 4 / 1 | 4, 5, 6 | — | 1382 / 2013 / 319 |
| pop G fret 5 (C3) 3 mm | 2.8 | 40 | 5 / 3 / 0 | 8, 7, 9 | — | 2192 / 1787 / 334 |

- **Caveat: the endless buzz.** With one polarisation, no fret-crown variation and the stopping
  fret's neighbour only ~0.25 mm under a fretted string, hard thumbs on fretted notes (and an open
  E at 5 m/s) keep rattling sparsely for 0.5 s. Without relief this was worse: 600 ms on almost
  everything.
  - A real bass does sizzle after a hard slap, but less.
  - The build should have the second polarisation (half of the swing is along the board and never
    touches) and the relief, and should tune the action against the IDMT notes.
  - Keep `PJ_SLAP_ACTION` as the first knob to try.
- The contact projection was stable in every run. A Python per-sample loop is not a cost
  measurement; the cost estimate in §5.2 is arithmetic.

---

## 6. Not found, or uncertain

- **The early GS module's GM 37 itself:** where its thumb/pop key split lies (if it has one), whether it
  switches by velocity, and which bass was sampled. Only the sample-based module's tone remarks, the voice
  counts and the variation names were found.
- **The later GS module map's two-voice Slap Bass 2:** whether the second voice is a detuned copy (as the
  sample-based module's DETUNE), a pop layer, or a velocity switch.
- **one maker's SlapBas1/2 element structure** (MU80/MU100 data lists not read).
- **Measured thumb speed, mass, contact time and pop depth on a real bass.** No motion-capture
  study was found. The §5.3–5.4 numbers are assumptions; the model results follow from them.
- **The Soundhouse column's full text.** It timed out again; only the snippet is used.
- **Kramer et al. 2012** and the full texts of Issanchou 2017/18 and Trautmann & Rabenstein 2004
  were not re-read here (abstracts only, plus bass_section.md's notes).

---

## Sources

- **[web, polyphone]** Polyphone, "MIDI standards" (GM, GS, XG tables):
  https://www.polyphone.io/en/documentation/manual/annexes/midi-standards
- **[web, SC-88Pro manual]** Roland SC-88Pro Owner's Manual: the tone list with the Native,
  SC-88 and SC-55 maps and their voice counts; the CM-64 map (banks 126/127):
  https://dosdays.co.uk/media/midi/SC-88PRO_OM.pdf
- **[web, synthmania MT-32]** https://synthmania.com/mt-32.htm
- **[web, U-20 manual]** Roland U-20 Owner's Manual, Preset Tone List (p. 166):
  https://www.manualslib.com/manual/1156670/Roland-U-20.html?page=168
- **[web, U-110]** https://en.wikipedia.org/wiki/Roland_U-110 ; https://www.synthmania.com/u-110.htm
  (search snippets only)
- **[web, Yamaha]** Yamaha CLP-785 data list (SlapBass1 0/0/37, via search):
  https://it.yamaha.com/files/download/other_assets/2/1342332/clp785_en_de_fr_es_pt_ja_dl_a0.pdf
- **[web, Soundhouse]** https://www.soundhouse.co.jp/en/contents/column/index?post=2827 (snippet,
  via bass_section.md)
- **[web, Wikipedia]** "Slapping (music)": https://en.wikipedia.org/wiki/Slapping_(music)
- **[paper, Rank97]** E. Rank, G. Kubin, "A waveguide model for slapbass synthesis", ICASSP 1997:
  https://www2.spsc.tugraz.at/people/franklyn/ICASSP97/pdf/author/ic970443.pdf
- **[paper, TR04]** L. Trautmann, R. Rabenstein, "Multirate simulations of string vibrations
  including nonlinear fret-string interactions using the functional transformation method",
  EURASIP JASP 2004(7), 949–963, doi:10.1155/S1110865704312059;
  https://doaj.org/article/35062fd83fc14d96b8d3d2dceb376513
- **[paper, Issanchou17]** C. Issanchou et al., "String/frets contacts in the electric bass
  sound: simulations and experiments", ISMA 2017:
  https://perso.ensta-paris.fr/~touze/PDF/ISMA_2017_paper_15.pdf ; Applied Acoustics 2018,
  doi:10.1016/j.apacoust.2017.07.021; https://hal-ensta.archives-ouvertes.fr/hal-01582578
- **[paper, Issanchou18]** as cited in bass_section.md §6.3 (the measured G string, contacts
  0–0.09 s, 4.5 kHz).
- **[paper, Ducceschi21]** M. Ducceschi, S. Bilbao, S. Willemsen, S. Serafin, "Linearly-implicit
  schemes for collisions in musical acoustics based on energy quadratisation", JASA 149(5),
  3502–3516 (2021): https://vbn.aau.dk/ws/files/475165562/DucceschiEtal2021JASALinearlyImplicitSchemes.pdf
- **[paper, DAFx24]** S. Bilbao, R. Russo, C. Webb, M. Ducceschi, "Real-time guitar synthesis",
  DAFx24 (read: Tables 1–2, §2.3–2.4, §5):
  https://dafx.de/paper-archive/2024/papers/DAFx24_paper_50.pdf
- **[paper, Abeßer13]** J. Abeßer et al., the IDMT-SMT-BASS work (via bass_section.md §6.3, 6.5).
- **[T]** The SF2 files: `/usr/share/sounds/sf2/FluidR3_GM.sf2`, `/usr/share/sounds/sf2/TimGM6mb.sf2`,
  `~/soundfonts/Jnsgm2.sf2`; TiMidity++ renders; the user's MIDI files; scratch scripts
  `slap1_sf2.py`, `slap1_samp.py`, `slap1_render.py` (output `slap1_render.txt`), `slap1_scan.py`,
  `slap1_scan2.py`, `slap1_sim.py` in
  `/tmp/claude-1000/-home-jyrki-pajupilli/f3f9b7e6-c8bc-4bf3-a7a3-c720f3e9b3d1/scratchpad/`.
