# Guiro, cuica, vibraslap and whistle: research, plan, what was built

Status: 2026-11-15. General MIDI's drum keys 58 (vibraslap), 71/72 (short and long whistle),
73/74 (short and long guiro) and 78/79 (mute and open cuica) were silent. Of the user's MIDI files,
the guiro keys carry about 1500 notes, the cuica 200, the whistles 25 and the vibraslap 5.

Marks: **[paper]**, **[web]** a source below; **[calc]** worked out here; **[memory]** general
knowledge, to be checked; **[design]** a choice of this plan.

## 1. Guiro (73 short, 74 long)

### 1.1 The instrument

- **Body:** a hollow gourd (or wood or fibreglass), about 25–35 cm long and 8–10 cm across, with
  parallel notches cut across one side. It has one or two sound holes on the back, where the
  fingers hold it **[memory]**.
- **Scraper:** a thin wooden stick, or a metal comb in the Dominican güira.
  - It is drawn along the notches: down (away from the player) in a long stroke and up in a short
    one.
  - The patterns, "chk-chik-chik" or long-short-short, are the rhythm **[memory]**.

### 1.2 What sounds

- **Each notch is a small impact.** The stick's tip climbs the notch's slope and drops off its edge
  onto the next. So the scrape is a train of impacts at a rate **f_r = v/λ**: the stroke speed over
  the notch spacing **[web, VT]**.
  - The train is quasi-periodic, so a fast stroke has a pitch. "Two principal weak resonances" of
    the body shape it **[web, PhISEM via GR]**.
- **Numbers [calc, memory]:**
  - Notch spacing λ ≈ 3–5 mm.
  - Stroke speed v ≈ 0.2–1.5 m/s: a long stroke covers 15–20 cm in 0.15–0.4 s, a short one 3–5 cm
    in 30–60 ms.
  - So f_r ≈ 50–500 Hz. A slow long stroke is a "rrrr" of separate clicks; a fast one is a buzzing
    "zzip" with a pitch.
- **The body:**
  - **Cavity:** a Helmholtz resonance. For V ≈ 1 L and a 2 cm hole in a 5 mm wall,
    f_H = c/2π · √(A/(V L_eff)) ≈ 55 · √(3.1e-4/(1e-3 · 0.022)) ≈ 210 Hz; smaller gourds and two
    holes give 250–450 Hz **[calc]**.
  - **Shell:** a few resonances in 1–5 kHz. A gourd is thin and lossy, so Q ≈ 10–30
    **[memory; PhISEM's guiro filters sit near 2.5 and 4 kHz]**.
  - **The tip's own click:** what is heard most, 2–8 kHz.
- **Stroke speed sets the loudness:** a harder, faster stroke means deeper drops and bigger
  impacts. **The notch shape sets the colour:** sharp notches give brighter clicks, rounded ones a
  mellower tone **[web, VT]**.
- **Published synthesis:**
  - Cook's PhISEM: random impulses through two resonances. Statistical, not mechanical
    **[web, GR]**.
  - Source-filter scraping (a contact force convolved with the object's response; Agarwal et
    al., DAFx 2020) **[web]**.
  - None measures a guiro.

## 2. Cuica (78 mute, 79 open)

### 2.1 The instrument

- **Build:**
  - a drum with one head (goat skin, ~0.3 mm), 15–25 cm across (6–10"), on a metal cylinder;
  - **a thin bamboo stick tied to the head's centre**, inside;
  - the player rubs the stick lengthwise with a damp cloth;
  - the other hand's thumb presses the head from outside, near the centre, to raise the pitch
    **[paper, SCK15]**.
- **The mechanism is stick-slip,** as a violin bow **[paper, SCK15; paper, W02]**. The rubbed
  stick moves in a sawtooth, which pulls the head's centre: "a primitive saw-toothed excitation,
  similar to a bowed violin string". **The body contributes little** **[paper, W02, abstract]**.
- **The head:**
  - Statsenko et al. modelled a 15 cm one cymbal maker cuica: goat skin 0.3 mm, a pre-tension of 4470 N/m,
    the stick 5 mm across, 1.4 mm off centre **[paper, SCK15]**.
  - Their measured operating shapes are near the clamped membrane's modes: 220, 820 and 942 Hz
    are shown.
  - Off centre and asymmetric, the shapes grow more asymmetric with force.
- **Pitch [calc]:**
  - Unloaded, the 15 cm head's (0,1) would be 2.405/(2π · 0.075) · √(4470/0.39) ≈ 550 Hz.
  - The stick (a few grams) at the centre loads it down to the 220 Hz they measured.
  - Thumb pressure stiffens the head near the centre and raises the pitch by up to about an octave
    **[web, SCK15: "pressure applied to the membrane changes the pitch"; memory]**.
  - Played, the cuica's "voice" glides between roughly 250 and 900 Hz.
- **Mute against open (GM 78/79):**
  - Mute: a short, higher squeak, the thumb pressing (higher, damped).
  - Open: a longer, lower "hoo", the head free, with the up-and-down of the stroke: the pitch
    rises on the pull and drops back **[memory]**.

### 2.2 Friction model

The project's bowed strings use a stick-slip friction curve between the bow and the string's
admittance at the contact point (ModalSet::bow). The cuica is the same with:
- the "string" the head's modes seen at its centre plus the stick's mass;
- the "bow" the cloth, its speed the hand's stroke (0.1–0.5 m/s), its force the grip (1–5 N);
- friction from a wet cloth on bamboo: μs ≈ 0.6, μd ≈ 0.3 [design].

## 3. Vibraslap (58)

- **Build:** a stiff bent steel wire (the spring). At one end is a wooden ball, at the other a small
  hollow wooden box with **loose metal pins (teeth, rivets) in it**. The player holds the wire's
  middle and slaps the ball with the other palm **[web, WP]**.
- **What sounds:**
  - The wire swings the box at its own slow rate. Every swing throws the pins against the box's
    walls: a buzzing rattle with the box as resonator, dying over about a second as the swing
    decays **[web, WP; memory]**.
  - The rattle's rate follows the swing, about 15–30 Hz, with several pin impacts per half swing
    [calc].
  - The pins ring a little themselves (small steel, 3–8 kHz).
  - The box: a few wood resonances, 0.5–3 kHz.
- **Model [design]:**
  - the wire and box a damped oscillator (25 Hz, T60 1.2 s), started by the slap;
  - N = 6 pins, each a free mass in a gap of 1–2 mm, colliding with the walls with restitution
    ~0.5;
  - every collision an impulse into the box's modes and the pin's ring.
  - The rattle's density and its fading come out of the motion.

## 4. Whistles (71 short, 72 long)

- **GM 71/72** are a whistle blown short and long: the samba "apito" or a referee's pea whistle
  **[memory]**.
- **The pea whistle:**
  - a jet across an edge (as a flute's) drives a small cavity;
  - pea-type whistles resonate at 3.3–3.5 kHz, set by the chamber's size **[web, P]**;
  - the pea is blown round the chamber and periodically disturbs the jet and the volume. That
    gives the trill, a few tens of Hz, mostly frequency modulation, seen most in the 2nd and 3rd
    harmonics, with a little amplitude modulation **[web, WT]**;
  - about 103 dB at 1 m **[web, WT: 106 dB(A) at 70 cm]**.
- **Model [design]:** a cavity resonance (3.2 kHz, Q ≈ 25) driven through a jet delay, the jet's
  deflection saturating (tanh): the flute's edge-tone loop in miniature. The pea modulates the
  cavity's frequency (±2–3 %) and the jet (±20 %) at its rotation rate, 25–40 Hz, which rises with
  the breath. The breath is velocity.

## 5. Plan (2026-11-15)

1. **One file pair, `src/scrapers.h/.cc`.** Four Tube models: GuiroModel, CuicaModel,
   VibraslapModel, WhistleModel. Self-contained (their own small resonator banks), so no other
   session's header is a dependency. One Performer each. Instruments:
   `guiro` (keys 73, 74), `cuica` (78, 79), `vibraslap` (58), `whistle` (71, 72), family "drum"
   (the main room, not gated).
2. **Guiro:**
   - **Stick tip:** a mass (2 g) held down by the hand's spring (1 N), dragged at v over a sawtooth
     profile (λ = 4 mm, notch depth 1 mm), with a Hunt–Crossley contact.
   - **Contact force:** drives the body (a Helmholtz mode near 320 Hz and five shell modes,
     1.1–5.5 kHz) and, high-passed, the tip's own click.
   - **Strokes:**
     - 74: long, 0.25 s over 16 cm, direction alternating per stroke;
     - 73: short, 0.06 s over 4 cm;
     - the hand's speed profile a raised cosine;
     - velocity sets the speed (and the press).
   - **Checks:** impact rate = v/λ within 10 %; velocity range near GM's 30 dB.
3. **Cuica:**
   - **Head:** 8 modes (clamped membrane, 0,n and 1,n) at the centre with the stick's mass, f01 at
     about 330 Hz open.
   - **Cloth:** a friction curve, the stroke speed in a raised cosine (in on the push, out on the
     pull), solved each sample against the head's admittance (Newton on the scalar force).
   - **Notes:**
     - 79 open: one push-pull, 0.35 s, the thumb free and then pressing a little at the end (an
       "oo-wah" upward glide of about 5 semitones);
     - 78 mute: 0.12 s, the thumb pressing (an octave up), more damping.
   - **Checks:** it sounds (stick-slip, not silent), the pitch glide range, stability.
4. **Vibraslap:** as §3. **Checks:** the rattle lasts about 1 s; there are impacts and no blow-up.
5. **Whistle:** as §4. 72 long (0.5 s), 71 short (0.12 s). **Checks:** pitch near 3.2 kHz, the
   trill rate, stability at the softest and hardest blow.
6. **Tests** in `tools/test.py`; **trims** in `src/levels_data.h`; **drumFor lines** to the owner
   of `src/drumkit.cc`.

## 6. Sources

- **[SCK15]** M. Statsenko, V. Chatziioannou, W. Kausel, "Investigation of the vibrational
  behaviour of the cuíca using time-averaged ESPI", Proc. Third Vienna Talk on Music Acoustics,
  2015, pp. 153–157. https://viennatalk2015.mdw.ac.at/proceedings/ViennaTalk2015_submission_29.pdf
- **[W02]** An acoustic study of the Brazilian cuica, JASA 112, 2266 (2002) (abstract only).
  https://ui.adsabs.harvard.edu/abs/2002ASAJ..112.2266W/abstract
- **[GR]** Csound's guiro (Cook's PhISEM): https://csound.com/manual/opcodes/guiro/
- **[VT]** "Stochastic percussion mechanics" (impact rate = stroke speed / notch spacing; sharp
  against rounded notches): https://vectree.io/c/stochastic-percussion-mechanics
- **[WP]** Vibraslap: https://en.wikipedia.org/wiki/Vibraslap
- **[P]** Pea whistle patent (pea whistles' fundamental 3.3–3.5 kHz): https://patents.google.com/patent/US5816186
- **[WT]** O. Räisänen, "Pea whistle steganography", 2015 (FM in the 2nd and 3rd harmonics; 106
  dB(A) at 70 cm): https://www.windytan.com/2015/10/pea-whistle-steganography.html

## 7. What was built (2026-11-16)

`src/scrapers.h/.cc`, instruments `guiro`, `cuica`, `vibraslap`, `pea-whistle` (family "drum";
the name `whistle` was taken by the tin whistle), the test `scrapers()` in `tools/test.py`. As
planned (§5), with these findings on the way:

- **Guiro.** The clicks repeat at v/λ: 172 /s at 0.69 m/s and 286 /s at 1.15 m/s, against 173 and
  287 expected (autocorrelation of the 2–9 kHz envelope mid-stroke). The tip also bounces once
  after landing on each notch, so counting envelope peaks gives about two per notch.
  - **The velocity range needed two physical changes:**
    - The hand's grip damps the tip near critically (450/s). A free tip launched off each notch
      edge clattered as loudly at velocity 16 as at 127.
    - The hand's height is set from the ridge tops, not the notch bottoms. A light press skims
      the tops, and the hand's spring alone pulled 0.3 N over the notch depth, more than a light
      press.
  - The range is now 25 dB (long stroke) and 24 dB (short).
- **Cuica.**
  - **Pitch:** the open stroke glides 267 → 361 Hz (+5 semitones) with the thumb; the mute sits an
    octave over the open's start.
  - **The grip** must grow with the cloth's speed (a bow's Schelleng limit). With a fixed grip a
    fast stroke double-slipped (an octave jump on the pull) or slid without catching (the mute
    was inverted: louder when softer).
  - **The cloth's reversal** in the open stroke leaves a short dip, as a real one does.
  - Range 23 dB (open), 27 dB (mute).
- **Vibraslap.**
  - The rattle lasts 1.46 s to −30 dB.
  - A second slap keeps only 30 % of the swing it meets (the palm stops the rest), so fast
    repeated slaps do not pile up.
  - Its rattle is all peaks (single pin clicks), so it is trimmed 6 dB under the common loudness;
    random playing peaks at 1.7.
  - Range 27 dB.
- **Pea whistle.**
  - Pitch 3283 Hz, the pea trilling at 36 Hz with 11 % AM.
  - The saturating jet alone choked its own start (a tiny breath meant a tiny saturation level
    that damped the seed away). The jet's turbulence now drives the cavity too.
  - Range 29 dB.
- **Levels:** all at −28.2 LUFS at velocity 90, except the vibraslap (−34.2).
- **Cost:** at most 94 ns/sample (0.5 % of a core).

Not done:
- the guiro's notch shape per instrument (gourd against fibreglass);
- the güira (metal);
- the cuica's off-centre stick (its asymmetric shapes, SCK15);
- the samba apito's three tones;
- fitting to recordings. None of these instruments is in the separated stems.
