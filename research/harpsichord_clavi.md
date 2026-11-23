# The harpsichord (GM 7) and the clavi D6 (GM 8): research and model plan

Status: 2026-11-15, research only; nothing is built. SI units. Pitches by MIDI-style names (C4 =
middle C); the harpsichord at A4 = 415 Hz unless stated.

Both are strings struck or plucked near one end. They should reuse the concert grand's string
banks (`src/piano.h`, `research/piano.md`): modes in two directions, an exact exchange with a
bridge, per-mode dampers, strings asleep when quiet. The harpsichord keeps the board too, made
lighter. The clavi swaps the board for two magnetic pickups and a small amplifier.

Tags: **[web]** read in a source in section 7 (the key follows in brackets); **[calc]** worked out
here from those numbers (the working is given or obvious); **[memory]** from the general
literature as remembered, not checked for this file.

## 1. Short answer

**Harpsichord.**
- A Franco-Flemish double: F1–F6 (61 keys), two 8' choirs and one 4'. The 8' strings are 1.76 m
  (F1) to 0.36 m (C5) on the Taskin 1769 scale **[web, Taskin]**. Brass in the bass, iron from
  about C3 up. The iron in the treble runs at 0.9–1.0 GPa, close to breaking **[calc]**. Tensions
  are 20–80 N per string, about a tenth of a piano string's **[calc]**.
- **The string's impedance is tiny** (0.06–0.3 kg/s against the piano's 5–10). At C4 the board is
  about 600 times stiffer than the string, against about 100 times for the piano **[calc]**. So
  the sustain is long (T60 of several seconds to 20 s in the bass) even on a 3 mm board.
- **The pluck is nearly fixed.** A quill or Delrin tongue drags the string up 1–3 mm, slowly
  (10–100 ms), then lets go in well under a millisecond. Key speed hardly changes the string's
  amplitude **[web, Giordano & Winans; Perng 2011]**. Recorded ff and pp tones differ by 0–5 dB in
  the low partials, and listeners hear about 1.7 dB **[web, Penttinen 2006]**.
  - What does change with force is a **knock**: the key and action thump into the case and board.
    The body modes below 300 Hz rise about 10 dB from pp to mf to ff **[web, Penttinen 2006]**.
- **Colour comes from the plucking point:** about 1/8–1/7 of the string in the bass, up to 1/3–1/2
  at the top **[web, Välimäki 2004; Ruckers GG 170/1332 mm]**. The two 8' rows pluck 1–2 cm apart,
  which gives a "back" (round) and a "front" (nasal) colour **[memory]**.
- **The board's reverberant tail** comes from the after-lengths behind the bridge (6–20 cm,
  inharmonic) and the undamped strings. Struck at the bridge, the board rings with T60 0.5 s
  below 350 Hz and 4.5 s from 350 Hz to 1 kHz **[web, Välimäki 2004]**.
- **Model:** the grand's machinery almost unchanged. 183 strings (3 × 61) as modal banks, about
  11 000 modes per direction to 12 kHz **[calc]**. A plectrum contact (a spring that lets go) in
  place of the hammer, cloth dampers on the jacks, a leather buff stop. A 3 mm board from
  `tools/pianoboard.py`, and a key-knock force into the board.
  - Cost: like the piano when everything rings. In normal playing, with most strings damped and
    asleep, about 5–15 % of a core **[calc]**.

**clavi.**
- 60 keys F1–E6, one steel string each, the 23 lowest wound **[web, Gabrielli 2013]**. A rubber
  tangent strikes the string and holds it against a metal anvil for as long as the key is down. The
  tangent's end of the string becomes the speaking length's end.
  - The short piece beyond it, towards the tuning pin, is wrapped in yarn.
  - On release the two parts join: the pitch drops by three semitones for a moment and the yarn
    kills the note **[web, Gabrielli 2013]**. That makes the remaining nonspeaking piece 0.19 of the
    speaking length **[calc]**.
- **No soundboard.** Two single-coil pickups near the far end (the bridge pickup 4 cm from the
  termination; the centre pickup 18.5 cm in the bass to 6.5 cm in the treble) **[web, Remaggi
  2012]**. The pickups sense velocity and are nonlinear in distance.
  - Two switches give: centre alone (warm), bridge alone (bright), both in phase (full), both out
    of phase (thin, the fundamental cancelled) **[web, till.com]**.
  - After them come four switched tone filters (Brilliant and Treble high-pass, Medium and Soft
    low-pass) and a transistor amplifier with 1 % THD **[web, Gabrielli 2013; Wikipedia]**.
- **The funk comes from four things:**
  - an excitation at the very end of the string (a moving boundary, all partials ~1/n), which
    makes the 2nd and 3rd partials louder than the 1st **[web, Gabrielli 2013]**;
  - the pickup comb near the end;
  - the long sustain (T60 ≥ 20 s low and middle **[web]**) cut short by a fast release with a pitch
    drop;
  - the key-down and key-up knocks, which reach the pickup through the string.
- **Model:** modal strings (as the grand's, no board) driven by a moving end. Two pickup taps per
  string, the polynomial nonlinearity, the switch matrix, the tone stack and the amplifier
  shelves. Only sounding keys are computed (the strings are isolated).
  - Cost: about 1500–2500 oscillators for ten voices, a few percent of a core **[calc]**.

## 2. The harpsichord

### 2.1 The instrument

- **Layout:** 1–2 manuals, 2–4 choirs. The usual 18th-century French double has two 8' choirs and
  one 4' over FF–f''' (F1–F6, 61 keys) **[web, Taskin page; Välimäki 2004]**.
  - The two 8' choirs differ in plucking point. The one plucked further from the nut is the
    "back" 8', the one nearer the nut the "front" 8' (more nasal) **[web, Välimäki 2004]**.
  - The 4' sounds an octave higher on its own bridge, and its strings hitch on a rail glued to the
    board **[web, arXiv 2511.09037]**.
  - In a French double the lower manual plays back 8' + 4' and the upper manual the front 8',
    with a coupler **[memory]**.
- **Buff (often wrongly called the "lute") stop:** a sliding batten near the nut carries leather
  (or cloth) pads that press against one 8' choir. The tone is muted and pizzicato-like and dies
  in "a few seconds". Narrow leather gives a "singing" buff, wider pads a "damping" one **[web,
  braybaroque buff; Britannica]**.
  - A true lute register is a separate jack row plucking very near the nut: brassy and nasal
    **[web, Britannica]**.
- **Measured instruments in the literature:**
  - Välimäki et al.: Knif & Pelto 2000, Italian/South German, 56 keys G1–D6, 8' back + 8' front +
    4', all brass, A = 415 Hz Vallotti, 2.40 × 0.85 m. No dampers in the top octave of the 4'
    **[web, Välimäki 2004]**.
  - The arXiv FDTD study: Dulcken 1755, 52 keys, 2 × 8' + 4' (156 strings), a' = 392 Hz. 8'
    strings 0.5→0.2 mm and 4' strings 0.3→0.2 mm. Total string pull 3938 N **[web, arXiv
    2511.09037]**.

### 2.2 String scale

Taskin 1769 (Edinburgh) **[web, Taskin page]**: case 2319 mm. 8' scaling FF 1764, C 1617, c
1178, c' 711, c'' 357 mm. 4' scaling c 605, c' 337, c'' 163 mm. The treble doubles per octave
("Pythagorean"); the bass is foreshortened. Ruckers 1639 (later rescaled): GG 8' 1332 mm, plucked
at 170 mm **[web, V&A]**.

Table **[calc]**, built as follows:
- lengths interpolated in log length between Taskin's points, with the 4' bass foreshortened to
  1.0 m at F1 by guess **[memory]**;
- diameters from typical modern stringing lists **[memory]**: 8' from 0.55 mm brass to 0.20 mm
  iron, 4' from 0.35 to 0.18 mm, matching the Dulcken's 0.5→0.2 and 0.3→0.2 **[web]**;
- brass at ρ = 8500 kg/m³, E = 100 GPa; iron at 7850 kg/m³, 190 GPa **[memory]**;
- σ = ρ(2Lf)², T = σA, B = π³Ed⁴/(64TL²), Z = √(Tμ).

| Choir | Key | f0 (Hz) | L (mm) | d (mm) | wire | σ (MPa) | T (N) | B | Z (kg/s) | modes < 12 kHz |
|---|---|---|---|---|---|---|---|---|---|---|
| 8' | F1 | 41.2 | 1764 | 0.55 | brass | 179 | 43 | 3.3e-5 | 0.29 | 193 |
| 8' | C2 | 61.7 | 1617 | 0.49 | brass | 338 | 64 | 1.7e-5 | 0.32 | 162 |
| 8' | C3 | 123.4 | 1178 | 0.40 | iron | 663 | 83 | 2.0e-5 | 0.29 | 90 |
| 8' | C4 | 246.8 | 711 | 0.30 | iron | 967 | 68 | 2.2e-5 | 0.20 | 47 |
| 8' | C5 | 493.5 | 357 | 0.25 | iron | 975 | 48 | 5.9e-5 | 0.14 | 23 |
| 8' | C6 | 987 | 179 | 0.21 | iron | 983 | 35 | 1.7e-4 | 0.10 | 12 |
| 8' | F6 | 1318 | 135 | 0.20 | iron | 986 | 31 | 2.6e-4 | 0.09 | 9 |
| 4' | F1 key (F2) | 82.3 | 1000 | 0.35 | brass | 231 | 22 | 3.3e-5 | 0.14 | 120 |
| 4' | C3 key (C4) | 246.8 | 605 | 0.28 | iron | 700 | 43 | 3.6e-5 | 0.14 | 46 |
| 4' | C5 key (C6) | 987 | 163 | 0.22 | iron | 813 | 31 | 2.6e-4 | 0.10 | 11 |
| 4' | F6 key (F7) | 2635 | 58 | 0.18 | iron | 740 | 19 | 1.5e-3 | 0.06 | 4 |

- **Stress:** the iron sits at 0.9–1.0 GPa from C4 up, against a tensile strength of 0.8–1.2 GPa
  for iron and 0.7–1.0 GPa for yellow brass **[web, mircat JAMIS 2012]**.
  - This is the old makers' rule: each string as long as the wire allows at that pitch. At
    A = 400 (Taskin's own pitch, about A = 409 on his 1784 fork **[web, Taskin page]**) the stress
    is 8 % lower **[calc]**.
  - Brass at C3 would be 0.73 GPa, at its limit. That is why the change to iron sits near there
    **[calc]**.
- **Total pull:** about 9 kN for the three choirs **[calc]**, against 3.9 kN measured on the
  smaller, lower-pitched Dulcken **[web]**. Either the diameters above are too heavy or the
  Taskin pulls more; the true value is within a factor of two. It matters for B and for each
  string's damping by the bridge, both of which are to be fitted (section 6).
- **Inharmonicity:** measured on the Knif instrument, B is about 1e-5 for the 19 lowest keys
  (audible) and above the mean audibility threshold for all 56 keys. Above 140 Hz it lies within
  the threshold's confidence band **[web, Välimäki 2004]**. The table's 1.3–3e-5 in the bass and
  1e-4–1e-3 in the top octave agree in order **[calc]**. Inharmonicity is much less than the
  piano's (piano C4 3e-4) because the wire is thin.
- **After-lengths:** behind the bridge each string runs on to its hitch pin, 6–20 cm,
  "very inharmonic", ringing long and undamped **[web, Välimäki 2004]**. Their first modes run
  from 360 Hz (bass, 20 cm) to 2.9 kHz (treble, 6 cm), 4–33 modes each below 12 kHz **[calc]**.
  Further, the bridge pins kink the string about 0.1 rad with a 400 g sideways pull
  **[web, Sankey string]**.
- **Bridge pin geometry splits the polarisations:** on a northern c' string the vertical
  frequency is 0.8 % below the horizontal **[web, Sankey string]**. This is a strong source of
  beating and of a two-stage decay; it was the bass model's "aliveness" and the piano's sideways
  direction.

### 2.3 The plucking point

- Knif instrument: 12 % of the length in the bass to about 50 % in the treble. The treble is then
  "round" (weak even partials) **[web, Välimäki 2004]**.
- Ruckers GG: 170/1332 mm = 12.8 % **[web, V&A]**. Sankey's northern c': 1/7 **[web, Sankey]**.
- Northern doubles in general **[memory]**: about 1/8–1/7 in the bass, 1/4–1/3 in the treble. The
  nut runs at an angle to the jack rows, so the distance in millimetres grows slowly while the
  ratio rises. The second 8' row plucks one slot (about 13–16 mm) further from the nut. On a C5
  string of 357 mm that is about a 4 % difference in ratio **[calc]**.
- Mode n starts at an amplitude ∝ sin(nπβ)/n² for an ideal pluck. At β = 1/7 every 7th partial
  is missing (Sankey's measured c' table: A1..A14 = 0.72, 0.32, 0.18, 0.10, 0.05, 0.02, 0.00,
  0.01, ...) **[web, Sankey string]**.

### 2.4 The plectrum and the pluck

- **Mechanics:** the key lifts the jack, guided in two registers. A tongue pivoting in the jack
  carries the plectrum (bird quill, now mostly Delrin). The plectrum's underside catches the
  string, bends, and the string slides along it and slips off its tip **[web, Perng 2010]**.
  - On the way down a spring (bristle or brass) lets the tongue swing back, so the plectrum
    passes the string without replucking. Then the cloth damper in the jack's top lands on the
    string **[web, Välimäki 2004; memory for the spring material]**.
- **One plectrum, measured and modelled [web, Perng 2010/2011]:** Delrin, 6 mm long, 2–4 mm wide,
  0.5 mm thick, I = 0.029 mm⁴, E = 3–5 GPa. String: steel, 0.37 mm, 0.5 m, 92–135 N.
  - Treated as a large-deflection cantilever with no friction. It deflects up to 45°, and the
    string moves both vertically and sideways before release.
  - Release amplitude against plucking ratio at a jack speed of 1 m/s: β = 1/2: 2.22 mm, 1/3: 1.87,
    1/4: 1.69, 1/5: 1.53, 1/6: 1.37, 1/7: 1.26 mm.
  - Plucked at the midpoint it is about 3 mm vertical and 1 mm horizontal at 0.02 m/s. At the
    slip-off there is a sharp sideways "kick".
  - Playing jack speeds are 0.02–0.1 m/s (the 2010 paper quotes 0.1–10 m/s from earlier work).
    Over these the release amplitude changes less than 10 %. It peaks near 2 m/s, outside
    playing.
- **What that implies [calc]:**
  - Cantilever stiffness 3EI/L³ ≈ 1200 N/m.
  - Force at release for β = 1/6, 1.37 mm, 92 N, 0.5 m: F = Ty/(β(1−β)L) ≈ 1.8 N.
  - Energy in the string about 1.2 mJ (Sankey measured 0.9 mJ, 9000 erg, into a c' string
    **[web]**).
  - The drag lasts y/v_jack = 10–100 ms, and the string barely moves until the slip.
- **Key speed:** "the amplitude of the resulting string vibration is approximately independent of
  the velocity with which the key is depressed" **[web, Giordano & Winans 1999]**.
  - Penttinen measured Knif's 8' back register at C2–C6, pp/mf/ff. At 0.4 s the ff tone's
    harmonics 1/2/3 are higher by 5/3/0.5 dB (C4) and 0/2/1 dB (C6). Decay is unchanged.
  - Listeners equalised loudness at pp→ff 1.68 dB (s.d. 0.37), pp→mf 1.08, mf→ff about 0.6
    **[web, Penttinen 2006]**.
- **Smith's pluck element [web, PASP]:** the plectrum as a spring (stiffness k, damping μ) at the
  string point, rigidly held. It engages when the jack point passes the string and lets go when
  the force passes f_max. Its force reflectance is (k/2R)/(s + k/2R).
  - The harpsichord picks one way only. After a release it stays off until the jack is below the
    string's swing.
  - To soften engage and release: f = k·y^p with p = 2 ("feathering").

### 2.5 Release: the jack falling back

- **On key-up:**
  - the jack falls;
  - the plectrum's top touches the string and the tongue swings back, a soft scrape and tick;
  - the damper cloth lands, the jack seats on the key, the key on its rail cloth.
  Välimäki et al. play a recorded release sample (recorded after the string had died) **[web,
  Välimäki 2004]**; Penttinen proposes modal synthesis of these knocks **[web, Penttinen 2006
  ref.]**.
- **Not found:** time constants for the damper. Plan: the grand's per-mode damper with an
  engage time of about 20–50 ms **[memory]**, fitted from recorded note ends.
- The top octave of the 4' often has no dampers **[web, Välimäki 2004]**. The board's 4.5 s
  ringing at 350 Hz–1 kHz is mainly the after-lengths and undamped strings **[web]**.

### 2.6 Soundboard, case and radiation

- **Board:** spruce 2–4 mm, about 5 mm in the bass to a little over 2 mm in the treble **[web,
  early-keyboard, arXiv 2511.09037: 2.4–5.7 mm, up to 7.5]**, against 8–10 mm on a piano.
  - Few ribs. Ruckers had none between the case side and the 4' hitch-pin rail. A cut-off bar
    crosses the board **[web, hpschd.nu]**.
  - Bridges 1.2–1.9 cm high; ribs and the cut-off bar about 2 cm thick; the 4' rail adds 1 cm
    **[web, arXiv 2511.09037]**.
  - Spruce E_L = 11 GPa, E_L/E_R = 8, ρ = 430 kg/m³ (as assumed there).
- **At one bridge point (c') [web, Sankey energy]:**
  - deflection 0.126 mm under 1 kg, so a stiffness of 7.8e4 N/m;
  - first resonance (tap tone) 110 Hz, acoustic mass 0.163 kg;
  - 0.21 g/cm² (60 % board, 34 % bridge, 6 % strings);
  - radiating area 776 cm², about 10 % of the board;
  - efficiency 0.4 %, about 70 dB(A) at 1 m.
  - Characteristic impedance √(km) ≈ 113 kg/s **[calc]**, a tenth of the piano's 1000.
  - With a C4 string of 0.2 kg/s that gives a bridge-loss decay rate 2f0·Z/R ≈ 0.85 1/s, T60 ≈
    8 s **[calc]**. Sankey measured the c' envelope at 2.6/s, with viscous loss 0.86/s and the
    rest to the board.
- **Modes:** one per 16.8 Hz (Savage) **[web, via Sankey]**; 36 modes from 0 to 600 Hz **[web,
  Kottick abstract]**, the same density. Fletcher: the northern barring gives an even response
  from the first resonance to about 4 kHz. The radiated response is "reasonably flat" from 50 Hz
  to 2 kHz **[web, Välimäki 2004 citing ref. 11]**.
  - So 256 board modes reach about 4.3 kHz on a harpsichord, against 2.3 kHz on the grand
    **[calc]**.
- **Air in the case:** the case is closed by a bottom board and often a rose. Savage fitted the
  air modes as those of a wedge-shaped sector (Bessel J0, J9 solutions) **[web, Savage via
  Sankey]**.
  - Helmholtz resonance: "usually below 100 Hz" **[web, Välimäki 2004]**; 37 Hz on the Dulcken,
    measured and computed **[web, arXiv 2511.09037]**.
  - Unlike the grand, the box is part of the sound: a second, air system coupled to the board.
- **Board decay:** at the 8' bridge the measured T60 was 235 ± 6 ms (Dulcken; band unstated)
  **[web, arXiv]**.
  - On the Knif instrument the impulse response has T60 about 0.5 s below 350 Hz and 4.5 s from
    350 Hz to 1 kHz (the strings ringing on) **[web, Välimäki 2004]**.
  - Their reverb fit: T60 6.0 s at DC, the ratio at fs/2 0.13.

### 2.7 Decay, beating, the ripple in decay rates

- Single tones last 10–25 s to silence, the bass longest **[web, Välimäki 2004]**.
- One tone (197 Hz): loop gain 0.996 gives τ = −1/(f0 ln g) = 1.27 s, a T60 of 8.8 s for its low
  partials **[calc from web]**.
- The partials' decay rates are uneven ("ripply") from one partial to the next. In the example
  above, the 6th partial rings longest and the 2nd outlasts the 1st and 3rd **[web, Välimäki
  2004]**.
  - This is the mark of the bridge (each partial meets a different board admittance) and of the
    two polarisations. A modal model with a real board gets it for free; a waveguide needs the
    "ripple filter".
- **Beating:** partial 10 of the 197 Hz tone beats with a period of about 1.5 s. Offsets range
  from about 0 to a few Hz **[web, Välimäki 2004]**. Two 8' choirs together also beat with each
  other (unison mistuning) **[memory]**.

### 2.8 Published models

| Model | String | Excitation | Body | Cost |
|---|---|---|---|---|
| Välimäki, Penttinen, Knif, Laurson, Erkut 2004 | waveguide, 1st-order allpass fraction, dispersion allpass, one-pole + ripple loss | recorded excitation 20 000 samples (0.45 s), windowed | 8-line FDN with comb allpasses, tone corrector; release sample; one beating resonator | per string under 13 mults; board 4× a string; G4 800 MHz: 15 strings, 10 with board |
| Penttinen 2006 | as above | + gain and timbre against velocity | + parallel knock filter for loud notes | small |
| Perng, Smith, Rossing 2010/2011 | waveguide at 100 kHz (2011) | cantilever plectrum with large deflection, two polarisations | rigid nut, bridge filter | offline |
| Smith, PASP | waveguide | spring plectrum with f_max release, one-filter scattering junction | — | one biquad |
| arXiv 2511.09037 (2025) | — | — | FDTD board on a 1 cm grid, 72 × 178, measured thickness | offline |

## 3. The clavi

### 3.1 The instrument

- **Keys and strings:** 1971–1982 (clavis 1964–1982) **[web, Wikipedia]**. 60 keys F1–E6, one
  steel string per key, the first 23 wound, a small timbre break at the change **[web, Gabrielli
  2013]**. The harp lies diagonally under the keys **[web, Wikipedia]**.
- **Mechanism [web, Gabrielli 2013]:**
  - The key is a class-2 lever. Its rubber tip, the tangent, strikes the string and traps it
    against a metal stud (the anvil) for the whole note.
  - The tangent splits the string into a speaking part (towards the tailpiece and the pickups)
    and a nonspeaking part (towards the tuning pin, wound with yarn).
  - Key speed averages 1–4 m/s.
- **Mute slider:** a mechanical slider with felt pads pressed onto the strings near the end
  **[web, Wikipedia; Gabrielli fig. 1]**. Pad position and pressure are not published.
- **Gauges:** one replacement-string chart **[web, Vintage Vibe]** gives .032" (0.81 mm) ×10,
  .028" ×5 and .022" ×9 (wound), then .011" (0.28 mm) ×3 and .009" (0.23 mm) ×35 (plain). Its note
  labels are garbled; the counts fit 24 wound, 38 plain. A restringer mentions .008" at the top
  **[web, clavinet.com]**.
- **String lengths:** only one is published, 67.8 cm at 161 Hz (about E3) **[web, Remaggi
  2012]**. The rest must be measured or estimated:
  - At E6 (1318.5 Hz) a plain steel string must be under about 0.20 m to stay below about 2 GPa
    **[calc]**.
  - Gabrielli's B = 8e-5 at E6 with a .009" string gives L ≈ 0.19 m, using B = π²Ed²/(64ρL⁴f²)
    **[calc]**.
  - The E3 string runs at only about 0.37 GPa if plain .011" (T ≈ 23 N) **[calc]**. clavi
    strings are slack compared with a harpsichord's, and the mid-range wound/plain split is
    uncertain.
- **Inharmonicity used in the model [web, Gabrielli 2013, table 1]:** F1 5e-4, A#1 2e-4, D3 9e-5,
  D#3 1e-4, F#5 9e-5, E6 8e-5. Audible in the bass, below the threshold at the top. There is a
  visible jump between keys 23 and 24 (about 150 Hz).
- **Pitch** is stable over a note to 1–2 cents **[web, Gabrielli 2013]**. Finger pressure after
  the strike bends the pitch slightly, a vibrato as on the clavichord **[web, Wikipedia
  mirrors]**. No size is published; it follows from the string's extra stretch as the tangent
  presses it further past the anvil line **[calc, mechanism only]**.

### 3.2 The excitation: a moving end

- **Waveforms [web, Gabrielli 2013]:**
  - At the pickup (time derivative of displacement), a low or middle tone's first period is a
    clean positive pulse with reflections and fast ripples (D3). Higher up it is a smooth
    periodic wave (A4).
  - The model's pulse is a 6th-order polynomial ramp joined to its mirror image. Its length is
    N = fs·d/v (d: the gap between tangent and string, v: key speed 1–4 m/s); d itself is not
    published.
- **Physics [calc]:** the tangent's tip is the string's end, and it moves by d in a time
  τ = d/v. Write the string as u(t)(1 − x/L) plus modes. Each mode q_n is then driven by −ü(t)
  with weight 2/(nπ): a 1/n spectrum (a sawtooth's, the "buzzy" colour) shaped by the spectrum of
  the tip's acceleration.
  - A gap of a few millimetres **[memory]** at 1–4 m/s gives τ ≈ 0.5–3 ms. That rolls off the
    upper partials more for soft notes: the dynamics in timbre.
  - The rubber's compliance rounds the end of the stroke, and the tip may bounce once on the anvil
    **[memory]**.
  - This explains why the 2nd partial is always at least 3 dB above the 1st and often the 3rd above
    the 2nd (A#2) **[web]**: a 1/n source, seen through a velocity pickup (+n), near the end
    (sin(nπd/L) ∝ n for low n) rises with n until the comb's first null **[calc]**.
- **Knock:** the tangent hits the anvil, and through the string and the stud some of it reaches
  the pickups. It is clearly audible on high notes and has most of its energy below 1.2 kHz
  **[web, Gabrielli 2013]**. Gabrielli plays one sample cut from E6 for every key, with a slightly
  random low-pass.

### 3.3 The yarn and the release

- **While the key is down:** the tangent is a node. The yarn on the nonspeaking part damps
  nothing of the speaking part, which sustains for a long time: T60 can exceed 20 s in the low and
  middle ranges. The only losses are at the ends (the tailpiece bar and the rubber tip) and to the
  pickup and air **[web, Gabrielli 2013]**.
- **On release:** the tangent lifts and the two parts become one string, the yarn at one end.
  - The pitch falls three semitones for the whole keyboard. So L_total/L_speaking = 2^(3/12) =
    1.19, and the nonspeaking part is 0.19 L_s **[web; calc]**.
  - The note dies fast with "an effective yarn damper", a sound typical of the instrument (the
    release "thunk") **[web]**. No decay time is published.
  - The yarn is a distributed loss over a few centimetres. It takes most from the modes that move
    there: the high ones, and all of them because the yarn sits at the open end of the joined
    string **[calc]**.
  - Worn yarn rings on: worn instruments sound different here **[memory]**.
  - Gabrielli models the release by lengthening the delay line by the nonspeaking part and
    lowering the loop gain.

### 3.4 Pickups, switches, electronics

- **Geometry [web, Remaggi 2012]:**
  - Two pickups, electrically identical, each a row of bar coils under epoxy: six bars × ten
    strings (Remaggi), or ten bars × six strings (Gabrielli 2013). Bars 0.5 cm wide, 3.7 cm long
    (centre) and 3.3 cm long (bridge).
  - Centre pickup: from 18.5 cm (lowest string) to 6.5 cm (highest) from the string's end.
  - Bridge pickup: a constant 4 cm, tilted about 30°.
  - The two papers disagree on which pickup lies above and which below the strings; Wikipedia
    says "above and below".
- **The comb [web; calc]:** a pickup at distance d from the end reads mode n ∝ sin(nπd/L).
  - On the 67.8 cm string the bridge pickup's first null is at partial 17 (about 2.7 kHz). A
    centre pickup at about L/5 nulls every 5th partial, as in Gabrielli's A#2 (notches about
    544 Hz apart).
  - Remaggi quotes comb delays of 200 samples (centre) and 84 (bridge) for this string. Their
    ratio fits the distances (about 9.5 cm against 4 cm), but their size does not fit the wave
    speed 2Lf0 = 218 m/s: they are about five times too long **[calc]**. Use the geometry, not
    those delays.
- **Transduction [web, Gabrielli 2013; Remaggi 2012]:**
  - Velocity sensing with a flat electrical response (within 1 dB).
  - The horizontal polarisation is about 25 dB below the vertical. The largest string swing is
    1 mm peak-to-peak.
  - The flux falls exponentially with distance. A 4th-order polynomial in displacement:
    p0 = 0.7951, p1 = −1.544, p2 = 181.8, p3 = −9508, p4 = 1.817e5 (in its fitted units: integrate,
    apply, differentiate).
- **Switches [web, till.com]:** left switch C/D, right switch A/B.
  - Both down: centre ("neck") pickup alone, warm.
  - Left down, right up: bridge pickup alone, bright.
  - Both up: both in phase, full.
  - Left up, right down: both out of phase, "the fundamental cancels somewhat", thin.
  - Letterings differ between revisions and sources **[web, Chicago Electric Piano]**.
  - Out of phase on the E3 string with the pickups at 4 and about 9.5 cm, the fundamental is
    about 8 dB under the in-phase sum and the 2nd about 9 dB; the highs above the centre pickup's
    first null stay **[calc]**.
- **Tone stack and amplifier [web, Gabrielli 2013]:**
  - Four switched filters, each an impedance in a parallel network: Soft R = 30 kΩ, C = 0.1 µF;
    Medium R = 10 kΩ, C = 15 nF; Treble L = 2 H, C = 4.7 nF; Brilliant L = 0.6 H. Wikipedia: Brilliant
    and Treble high-pass, Medium and Soft low-pass.
  - The elements' own frequencies are 53 Hz (Soft RC), 1.06 kHz (Medium RC) and 1.64 kHz (Treble
    LC) **[calc]**; the network's corners depend on the rest of the circuit.
  - Without the stack the amplifier is nearly flat: −3 dB low shelf at 130 Hz, +3 dB high shelf at
    4 kHz.
  - Two BJT stages: 1 % THD at 400 mV (1 kHz), 3.6 % on rare fortissimo chord peaks, judged
    masked. Unshielded coils pick up hum and noise.

### 3.5 Decay and beating

- Most partials decay in a straight line in dB, with uneven T60 from partial to partial (a ripple
  with a period of 2–3 partials). The lowest 2–4 partials last much longer **[web, Gabrielli 2013]**.
- **Beating** is seen only below E4: 0.5–2 Hz, up to 15 dB peak-to-peak, not on every note,
  slightly more with key speed. It shows in both microphone and pickup. The suggested cause is a
  yielding termination; it could also be the two polarisations seen through a pickup that reads
  only one **[web; calc for the alternative]**.

### 3.6 Published models

- **Gabrielli, Välimäki, Penttinen, Squartini, Bilbao 2013** (from ICMC 2010, with Bilbao's FDTD
  exploration before it) **[web]**:
  - The string: a waveguide with ripple loss (R_rate 1/2–1/3, r from −0.006 to −0.001, drawn at
    random per note) and up to an 8th-order dispersion allpass.
  - Beating: three "beating equalizers", |cos| gain modulation.
  - The rest: pulse excitation, release delay, knock sample, pickup comb + polynomial, tone
    stack, shelves.
  - Cost: 82 multiplications and 64 additions per string-sample (the tone stack 21, the dispersion
    20). At most 923 delay samples. Ten voices on a BeagleBoard at 97 % CPU, 5.8 ms latency.
  - Listening tests: discrimination d = 53 % on average (58 % for clavi-savvy musicians; 50 % is
    chance).
- **Remaggi et al. 2012:** the pickup model above.

## 4. Model plan: harpsichord (`-inst harpsichord`)

**Strings.** The grand's banks as they are: modes in two directions, x/v rotation, decay folded in,
`in_`/`out_` exchange at the bridge, `damp_` per mode.
- 3 choirs × 61 = 183 strings. Modes to 12 kHz: 4316 per 8' choir, 2283 for the 4', 10 900 per
  direction in all **[calc]**. 16 kHz costs about 1.3 times that.
- **Scale:** section 2.2's table as a first guess, tuned by partials like the piano's
  `research/piano.md` §11 (B from the wire, then fitted).
- **Polarisations:** vertical (the pluck's, towards the board) and horizontal (the slip-off kick).
  - They hold the bridge differently: the pins' 0.8 % split **[web]** plus a looser sideways hold,
    as in the piano.
  - Two 8' choirs on one bridge, slightly mistuned, beat through it. That should replace
    Välimäki's beating resonator.
- **After-lengths:** each string's piece behind the bridge as a short bank (4–33 modes, 6–20 cm)
  on the same bridge point, undamped. The piano left "lengths behind the bridge" open; here they
  are the reverb tail **[web]** and come first, not later. About 2000 modes in all **[calc]**.

**Plectrum.** In place of the hammer's felt solve:
- A contact at the plucking point β·L. The string's place there comes from the existing
  `hc_`/`hs_` rows.
- The plectrum is a spring k ≈ 1000–1500 N/m on a jack moving up at 0.02–0.1 m/s (from key
  velocity; the effect is small). It engages when the tip reaches the string and lets go when the
  string's deflection along the tongue passes its free length.
  - In Smith's form: when the force passes f_max ≈ 1–2 N in the treble, more in the bass
    **[calc]**.
- Feathered (f ∝ y²) at the start and end to avoid clicks. The force is applied vertically, plus
  a sideways component growing with the plectrum's bend (Perng's kick, a fraction of the vertical
  release: 1 mm against 3 mm at the midpoint **[web]**).
- The release is the step: the string springs back from 1–3 mm in under 0.1 ms. The rise of the
  release force is set by the tip's width (about 1 mm of contact), so a smoothing over 2–5
  samples is enough **[calc]**.
- Per choir: β, plectrum stiffness (quill harder and brighter, Delrin softer **[memory]**) and
  f_max as voicing parameters per key.
- **Dynamics (Penttinen):** the key-speed range maps to ±2 dB of string level, and to a key
  "knock" into the board that grows about 10 dB per dynamic step.

**Release and dampers.**
- Key up: after 10–30 ms (the jack's fall) the plectrum brushes past the string. This is a weak
  second contact with a small k for a few ms, one-way off, since the tongue yields **[memory;
  shape from PASP]**.
- Then the damper cloth: the grand's `engage` ramp over 20–50 ms at a point a few mm from the
  plucking point, on the same choir.
- The jack seating on the key: a small force pulse into the board at the key's bridge end
  (fixed shape, scaled by release speed).
- The top 4' octave optionally undamped (Knif).

**Buff stop.** A fixed damper per mode on one 8' choir, at about 1–2 % of L from the nut. It
weights each mode by sin²(nπx/L) at the pad, so it takes more from the high modes. Fitted so the
note dies in about 2–3 s ("a few seconds" **[web]**). Cost: nothing new.

**Board and box.** `tools/pianoboard.py` run on a harpsichord:
- 3 mm spruce (5→2.4 mm), wedge outline about 2.2 × 0.9 m (Taskin case 2319 mm);
- the 8' and 4' bridges as beams, the 4' hitch-pin rail, a cut-off bar, almost no ribs.
Then:
- **Board resonances:** 256 to about 4.3 kHz (density 1/16.8 Hz **[web]**), plus 64–96 broad ones
  to 12 kHz.
  - Check: the point stiffness at the c' bridge point (7.8e4 N/m **[web]**), the first
    resonance near 110 Hz and the bridge impedance near 100 kg/s **[calc]**.
- **The air box:** a few cavity modes as resonators coupled to the board's volume velocity. The
  Helmholtz mode at 37–100 Hz **[web]** first, then the wedge modes.
- **Radiation:** reuse the piano's patches and two microphones, about 1 m over the board, as in
  the Helsinki recordings **[web]**.
- The damped strings load the board, as already found on the grand (piano.md §11). Here the
  string/board impedance ratio is six times smaller, so the effect is weaker **[calc]**.

**Reuse from the grand.** `Piano` is close to a general "keyboard of strings on a board".
Factor it rather than copy it:
- string table builder, modal arrays and SIMD loops, the bridge exchange, dampers, sleep and
  wake, board, patches and microphones: reuse;
- the hammer: becomes one of two exciters (felt hammer, plectrum);
- new: after-length banks; a second key→string map (one key, 1–3 choirs by register switch); an
  air-cavity block; the key and jack knock forces.

**Cost [calc].**
- Everything ringing (all dampers lifted, as never happens): about 22 000 string oscillators
  plus 4000 after-length and 350 board oscillators, the piano's size, about 30 % of a core
  (piano.md §7a: 0.45 ms per 64 samples for 21 000).
- Playing: dampers down on most strings, so 10–40 strings awake plus all the undamped
  after-lengths: about 5–15 %.

**Validation.**
1. One string, one choir: pluck spectra against β (the sin(nπβ)/n² nulls), release amplitudes
   against Perng's table, a level change of about 2 dB from slowest to fastest key.
2. Board: point stiffness and first mode; T60 of the bridge impulse response, 0.5 s below 350 Hz
   and 4.5 s at 350 Hz–1 kHz with the after-lengths **[web]**.
3. Recordings in `~/Music` (harpsichord tracks, if any), analysed with the `tools/pianofit.py`
   method:
   - B per register (target about 1e-5 in the bass **[web]**);
   - decay per partial (T60 several s to 20 s; ripple from partial to partial);
   - colour per register;
   - 8'+8' beating;
   - the sound at note end (damper time, release tick).
4. Listening: 8' back, 8' front, 4', buff, and the combinations against recordings. Välimäki's
   d-factor listening test is the reference method.

## 5. Model plan: clavi (`-inst clavi`)

**Strings.** 60 modal banks, two directions, modes to about 12–16 kHz. The velocity pickup lifts
the highs, so 16 kHz is worth it here.
- F1 needs about 275 modes to 12 kHz **[calc]**; a typical voice 50–150.
- Only sounding strings are computed: no board couples them. Ten voices are about 1500–2500
  oscillators **[calc]**.
- The scale: B from Gabrielli's table. Lengths: E3 at 67.8 cm **[web]**, the top about 0.15–0.20
  m **[calc]**, the rest to be measured (photos, an owner) or fitted from the pickup comb in
  recordings (the null spacing gives d/L).
- Ends:
  - the tailpiece end is nearly rigid;
  - the tangent end is the rubber: a lossy, slightly compliant support (a per-mode loss rising
    with frequency), with a small difference between the polarisations.
  - That makes the 2–3-partial ripple in T60 and the 0.5–2 Hz beating below E4. Check it against
    the 15 dB peak-to-peak **[web]** before adding anything ad hoc.

**Excitation.** The moving end (section 3.2):
- The tip's displacement u(t) rises over τ = d/v with a rounded end (the rubber), then holds.
- Each mode gets −ü(t)·2/(nπ) into its velocity. This is the same row of the force table as a
  point force at the end, so one table per key, scaled.
- Key velocity sets v (1–4 m/s) and so τ and level. A rubber stiffness of about 10⁴–10⁵ N/m
  **[memory]** sets the bounce, if one is wanted.
- Aftertouch (channel pressure): tension +ΔT, so every mode's frequency × √(1+ΔT/T). A few cents
  at full pressure (to be heard and set; nothing published).

**Release.**
- Key up: project the speaking string's mode state (x, v) onto the modes of the full string
  (1.19 L_s). This is a fixed n_s × n_f matrix per key computed at build time, e.g. 100 × 120 =
  12 000 multiply-adds once per release **[calc]**: free.
- Then damp the full string's modes by the yarn (a band at the far end, weight ∫ sin² over the
  yarn length). This gives the three-semitone drop and the fast decay with no special case.
- Plus a release thunk: the tangent leaving and the key hitting its rest, a short force pulse
  into the string near the anvil.
- The mute slider: a fixed per-mode damper near the tailpiece end, like the buff stop.

**Knock.** The tangent hitting the anvil: a short force pulse into the string at the anvil. It
reaches the pickups through the string's modes, so it is pitched and differs per key, unlike
Gabrielli's single sample. Its energy below 1.2 kHz **[web]** sets the pulse width (about 0.5 ms)
**[calc]**.

**Pickups and electronics.**
- Per string and pickup, the displacement at the pickup: one dot product per direction (vertical
  weight 1, horizontal −25 dB).
- Then the polynomial in displacement (Gabrielli's coefficients scaled to the gap, with a gap
  parameter per pickup), then a first-difference (velocity).
- Sum per pickup, switch matrix (C, B, C+B, C−B), the four tone filters as Gabrielli's bilinear
  sections, the two shelves, an optional soft clip (1 % THD at nominal level), and hum and noise
  at a low level.
- Then the project's amp, rotary or room as for the synth leads.

**Reuse.** The grand's modal arrays, SIMD loops and dampers. No board, no bridge exchange: the
string's far end is fixed. The tangent's end-drive replaces the hammer. The pickup and filters
are new, small classes; the tone stack and shelves may exist in `src/lead.cc` or `src/room.cc`
already (to be checked).

**Cost [calc].** Ten voices of 150–250 modes in two directions is 3000–5000 oscillators: under
0.3 µs a sample from the piano's measured rate (0.33 ns per oscillator-sample). The pickups (two
dot products per voice) and filters (a few dozen multiplies) are noise. About 2–3 % of a core;
lower than Gabrielli's waveguide in absolute terms only because the machine is faster.

**Validation.**
1. The spectrum shape of a single note: 2nd partial ≥ 1st + 3 dB, the comb nulls per pickup
   setting (every 5th partial on A#2 with the centre pickup **[web]**), the out-of-phase setting's
   lost fundamental.
2. Release: pitch drop exactly three semitones, decay time against recordings.
3. T60 per partial against Gabrielli's E4 figure (long 1st–2nd partials, ripple).
4. Beating amplitude and rate below E4.
5. Funk tracks in `~/Music` (if any): onset spectra, release behaviour and knock level, analysed
   like the piano fit. Gabrielli's d-factor test against isolated recorded notes, if a sample
   library or owner's recordings can be had.

## 6. Open questions

- **Harpsichord:**
  - Real diameters and pull (9 kN computed against 3.9 kN on a smaller instrument).
  - Damper engage time; the jack's fall time.
  - Measured plectrum force at release on quill against Delrin. Giordano & Winans have
    measurements, not read here (paywalled).
  - The board's modal data for a Franco-Flemish instrument: none found; model it.
- **clavi:**
  - String lengths, the tangent gap d, the tangent rubber's stiffness and loss.
  - The yarn's decay time.
  - Which pickup is above the strings.
  - The size of the pressure vibrato.
  - The tone network's full circuit (the schematic is public **[web, Chicago Electric Piano]**;
    simulate it once offline).

## 7. Sources

- V. Välimäki, H. Penttinen, J. Knif, M. Laurson, C. Erkut, "Sound synthesis of the harpsichord
  using a computationally efficient physical model", EURASIP J. Applied Signal Processing 2004(7),
  934–948. https://www.ee.columbia.edu/~dpwe/e6820/papers/ValPK04-harpsi.pdf ;
  https://link.springer.com/article/10.1155/S111086570440211X
- H. Penttinen, "On the dynamics of the harpsichord and its synthesis", DAFx-06, 115–120.
  https://dafx.de/paper-archive/2006/papers/p_115.pdf
- C.-Y. J. Perng, J. O. Smith, T. Rossing, "Physical modeling of the harpsichord plectrum-string
  interaction", DAFx-10. https://www.dafx.de/paper-archive/2010/DAFx10/PerngSmithRossing_DAFx10_P93.pdf
- C.-Y. J. Perng, J. O. Smith, T. Rossing, "Harpsichord sound synthesis using a physical plectrum
  model interfaced with the digital waveguide", DAFx-11. https://dafx.de/paper-archive/2011/Papers/24_e.pdf
- N. Giordano, J. P. Winans II, "Plucked strings and the harpsichord", J. Sound Vib. 224 (1999)
  455 (abstract via https://www.physics.purdue.edu/piano/articles.shtml)
- J. O. Smith, Physical Audio Signal Processing, "Pluck modeling".
  https://www.dsprelated.com/freebooks/pasp/Pluck_Modeling.html
- J. Sankey, "The physics of harpsichord sound" and "Bridge pins of harpsichords".
  https://johnsankey.ca/energy.html ; https://johnsankey.ca/string.html
- "Taskin harpsichord scalings and stringings revisited". https://www.braybaroque.ie/harps/Taskin_stringing.htm
- "Harpsichord - fitting a singing buff". https://braybaroque.ie/harps/buff.htm
- Britannica, keyboard instruments, special effects. https://www.britannica.com/art/keyboard-instrument/Special-effects
- V&A, Ioannes Ruckers 1639 (scaling, plucking point). https://collections.vam.ac.uk/item/O169594/lid-prop-ruckers-ioannes
- Iron and brass wire strengths (JAMIS 2012). https://amis.mircat.org/jamis/2012_078.pdf
- Harpsichord soundboard FDTD and FEM, Dulcken 1755 (arXiv 2511.09037, 2025). https://arxiv.org/html/2511.09037v1
- Soundboard thickness, construction. https://www.gb.early-keyboard.com/guide.htm ; https://hpschd.nu/tech/sre/sb-crack.html
- W. R. Savage, E. L. Kottick, T. J. Hendrickson, K. D. Marshall, "Air and structural modes of a
  harpsichord", JASA 91(4), 2180–2189, 1992 (cited via Sankey; not read).
- N. H. Fletcher, "Analysis of the design and performance of harpsichords", Acustica 37 (1977)
  139–147 (cited; not read). https://openresearch-repository.anu.edu.au/items/c22ee166-22ba-4134-8986-073b1fd2efa3
- L. Gabrielli, V. Välimäki, H. Penttinen, S. Squartini, S. Bilbao, "A digital waveguide-based
  approach for Clavinet modeling and synthesis", EURASIP J. Adv. Signal Process. 2013:103.
  https://asp-eurasipjournals.springeropen.com/articles/10.1186/1687-6180-2013-103 (provisional PDF:
  https://www.ness.music.ed.ac.uk/wp-content/uploads/2013/06/Gabrielli.pdf)
- L. Gabrielli, V. Välimäki, S. Bilbao, "Real-time emulation of the Clavinet", ICMC 2010.
  https://www.research.ed.ac.uk/en/publications/real-time-emulation-of-the-clavinet-an-electromechanical-keyboard/
- L. Remaggi, L. Gabrielli, R. C. D. de Paiva, V. Välimäki, S. Squartini, "A pickup model for the
  Clavinet", DAFx-12. https://dafx.de/paper-archive/2012/papers/dafx12_submission_62.pdf
- Clavinet pickup switches (till.com, 1999). https://till.com/articles/Clavinet/
- Clavinet schematic and controls (Chicago Electric Piano). https://chicagoelectricpiano.com/clavinet/hohner-clavinet-schematic-tone-controls/
- Clavinet, Wikipedia. https://en.wikipedia.org/wiki/Clavinet
- Clavinet strings: Vintage Vibe chart https://vintagevibe.com/products/clavinet-strings ;
  restringing diary https://clavinet.com/chronicles.php
