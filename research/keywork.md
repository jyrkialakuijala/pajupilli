# Boehm flute geometry and keywork: numbers for the pajupilli model

Research note for REQUIREMENTS.md §2.2 (R4–R8). Metric units throughout. Confidence:
**H** = measured or primary source, **M** = secondary or consistent across sources,
**L** = single weak source, **G** = my guess or derivation (marked as such).

## 0. Sources (cited as [D], [R], …)

- **[D]** P. Dickens, *Flute acoustics: measurement, modelling and design*, PhD thesis, UNSW 2007.
  The thesis code, with its measured geometry file `xml/ModernFlute.xml` ("Pearl flute PF-661 open
  hole", C foot plus optional B foot), is at https://github.com/buha/flute-thesis. **This is the
  primary data set used below.** All lengths in the file are in mm, and hole and key sizes are
  radii. Hole formulas: `Woodwind.c`, `Woodwind.h`.
- **[R]** J.H. Rindel, "Acoustical aspects of the transverse flute from 1700 to present", Table 1:
  Jupiter SFL-711 silver flute, B foot, open holes, A=440. Positions are measured from the
  embouchure-hole centre.
  https://vmcollectables.com/wp-content/uploads/2022/10/Acoustical-aspects-of-the-transverse-flute-from-1700-to-present.pdf
- **[W]** Wolfe, Smith, Tann, Fletcher, "Acoustic impedance spectra of classical and modern flutes",
  J. Sound Vib. 243 (2001). https://www.phys.unsw.edu.au/jw/JSV.pdf . Also the UNSW pages
  https://newt.phys.unsw.edu.au/jw/fluteacoustics.html and
  https://newt.phys.unsw.edu.au/music/flute/anatomy.html
- **[M]** McGee, "Boehm's Schema" (quoting Boehm, and Miller's measurements of Boehm flutes).
  https://www.mcgee-flutes.com/Boehm's%20Schema.htm . Also the lengths table at
  https://mcgee-flutes.com/flutelengths.htm
- **[F]** C. Forster, *Musical Mathematics*, ch. 8 (flute tone holes; Armstrong flute).
  https://chrysalis-foundation.org/flute_tone_holes.htm
- **[F75]** N.H. Fletcher, "Acoustical correlates of flute performance technique", JASA 57 (1975) 233.
  https://openresearch-repository.anu.edu.au/bitstream/1885/125582/1/Acoustical%20correlates%20of%20flute%20performance%20technique.pdf
- **[F82]** Fletcher, Strong, Silk, "Acoustical characterization of flute head joints", JASA 71 (1982).
  https://www.phys.unsw.edu.au/music/people/publications/Fletcheretal1982.pdf
- **[A]** Almeida, Chow, Smith, Wolfe, "The kinetics and acoustics of fingering and note transitions on
  the flute", JASA 126 (2009). https://arxiv.org/pdf/0903.4012
- **[WFG]** Woodwind Fingering Guide, basic flute charts.
  https://www.wfg.woodwind.org/flute/fl_bas_1.html , `fl_bas_2.html`, `fl_bas_3.html`, `fl_fing.html`
- **[JLS]** JL Smith flute pad height gauge notes.
  https://www.jlsmithco.com/wp-content/uploads/2020/11/227049-Flute-Pad-Height-Gauge.pdf
- **[Y]** Yamaha YFL-2xx "technical info" key heights, relayed second-hand on a Sax-on-the-Web
  thread ("flute key height", https://www.saxontheweb.net/threads/flute-key-height.210726/). I saw
  them only as search-engine excerpts, so this is unverified (**L**).
- **[Wiki]** https://en.wikipedia.org/wiki/Western_concert_flute
- **[Dal01]** Dalmont, Nederveen, Dubos, Ollivier, Méserette, te Sligte, "Radiation impedance of tubes
  with different flanges…", JSV 244 (2001) 505.
- **[Dal02]** Dalmont, Nederveen, Joly, "Experimental determination of the equivalent circuit of an open
  side hole: linear and non linear behaviour", Acta Acustica 88 (2002).
- **[Ned98]** Nederveen, Jansen, van Hassel, "Corrections for woodwind tone-hole calculations", Acustica
  84 (1998).
- **[Dub99]** Dubos, Kergomard, Khettabi, Dalmont, Keefe, Nederveen, "Theory of sound propagation in a
  duct with a branched tube using modal decomposition", Acustica 85 (1999).
- **[K90]** D.H. Keefe, "Woodwind air column models", JASA 88 (1990) 35.
- **[LS12]** Lefebvre & Scavone, "Characterization of woodwind instrument toneholes with the finite
  element method", JASA 131 (2012) 3153.

## 1. Bore

All positions x are in mm along the axis from the **embouchure-hole centre** (x=0), positive toward
the foot. The cork face is at x = −17.5 mm.

| Quantity | Value | Source / confidence |
|---|---|---|
| Body bore diameter | 19.0 mm (Pearl); 18.85 mm (Jupiter) | [D] H, [R] H, [W] "19 mm" |
| Bore at cork face | 16.8 mm (Pearl); 16.65 mm (Jupiter) | [D] H, [R] H |
| Bore at embouchure centre | 17.07 mm (Pearl); 17.4 mm (Armstrong) | [D] H, [F] M |
| Head taper, Pearl, as modelled | linear Ø17.07 → Ø18.8 over x = 0…113.5, then cylinder Ø18.8 over x = 113.5…150.5 | [D] H (piecewise linear fit) |
| Head/body joint (socket gap) | Ø19.8 over x = 150.5…154.5 (4 mm step), then body Ø19.0 from x = 154.5 | [D] H |
| Cork to embouchure centre | 17.5 mm (Pearl, and Wolfe's measurements); "≈17 mm" standard; 17.3 mm (Wikipedia, factory) | [D] [W] H; [F82] [Wiki] M |
| Embouchure centre to foot end, C foot | 600.4 mm (Pearl); 598 mm (Powell Cooper #4434); 594 mm (Jupiter, to its C hole) | [D] H, [M] M |
| Embouchure centre to foot end, B foot | 633.3 mm (Pearl); 638 mm (Jupiter) | [D] H, [R] H |
| Cork face to foot end, C foot | ≈ 618 mm, i.e. 84.6 grid cells at dx = 7.3 mm (B foot: ≈ 651 mm, 89.2 cells) | derived from [D]; Boehm gives 618.5 mm for A435 [M] |
| Overall length, C foot | ≈ 665–672 mm (Powell 670) | [M] M |
| Head / body / foot joint lengths | head ≈ 220–230 mm including crown and tenon; foot (C) ≈ 100–125 mm; body = remainder (≈ 330–400 mm, depending on how tenons are counted) | **G**. The only clean data found are for a Rudall Carte wooden flute (231 / 333 / 127 mm). Acoustically irrelevant apart from the 4 mm socket step. |
| Wall thickness, silver tube | 0.36 / 0.41 / 0.46 mm (0.014″ / 0.016″ / 0.018″; 0.016″ most common) | Powell and Burkart specifications (M) |
| Head taper shape | Boehm called it "parabolic" ("inaccurately", [W]); measured profiles are close to a conical or linear taper ending ≈ 2 mm narrower at the cork | [W] M |

Model recommendation (G): use [D]'s piecewise-linear area profile as given. The parabola
d(x) = 19.0 − 2.2·((150 − x)/167.5)² (x from −17.5 to 150) differs from it by < 0.2 mm; both are fine.

## 2. Embouchure hole

| Quantity | Value | Source / conf. |
|---|---|---|
| Outer opening | 11.6 × 10.3 mm (Jupiter); about 12 × 10 mm is typical; a maker's starting point is 11.8 × 10.2 mm | [R] H; tippleflutes / Saxontheweb (L) |
| Long axis | across the flute, i.e. along the bore axis; the jet crosses the short (~10 mm) dimension | [F75] M ("lip opening max width ≈ 12 mm = width of the embouchure hole") |
| Area | ≈ 90 mm² | [W] H |
| Equivalent radii, Pearl (for a circular-hole model) | r_in = 6.4 mm at the bore (undercut, so wider at the bottom); r_out = 5.7 mm at the top | [D] H |
| Chimney (riser + lip plate + tube wall) | 4.4 mm (Pearl); 4.3 mm (Armstrong); riser alone "generally 5 mm" (?) | [D] H, [F] M; tippleflutes L |
| Shape | rounded rectangle ("oval"); walls undercut (wider at the bore) and overcut (bevelled at the top), with sharp strike edge | fluteworx headjoint article (qualitative only); [D] r_in > r_out |
| Lip coverage | Coltman: the uncovered width (along the jet) is **6 mm at C4, falling to 3.5 mm at A6**, i.e. about 40 % covered (low) to 65 % covered (high) of a 10.3 mm hole | [F75] quoting Coltman H; the percentages are G |
| Coverage plus face baffle, as an effective factor | Wolfe: h·√g = 0.3, where g is the open fraction of the hole and h the open fraction of the solid angle | [W] M |
| Jet length vs note | l [mm] ≈ 1.8 + 100/√f [Hz], ±20 %; uncovered width ≈ jet length − 1…2 mm | [F75] H |

Pearl modelling detail: [D] models the riser as a cone of length 4.4 + t_m whose outer radius is
`entryratio·r_out`, so lip coverage is the parameter `entryratio`. [D] adds a series resistance
Z0·6.9e-6·f and a shunt conductance 1.3e-4·f/Z0 at the embouchure (empirical), and an extra
inner end correction 0.5·b·(b/a)² on top of the standard tone-hole t_i (eq. in §6).

## 3. Tone holes (Pearl PF-661 measured [D], cross-checked with Jupiter [R] and Boehm/Miller [M])

**Naming:** each hole is named by the note it sounds when it is the highest open hole (the
acoustic name used by [D] and [R]). Technicians name a key one semitone lower, by the note sounded
when that key and everything above it are closed. Example: RH1 is the "F key", and it covers the F#
hole.

x is measured from the embouchure centre. "From foot" is 600.4 − x (C foot). "Cell" is
(x + 17.5)/7.3, the grid index counted from the cork. Ø is the hole diameter. The hole "length"
in [D] is 1.9 mm for every hole (tube wall plus drawn chimney; the key data give a chimney rim
height of 1.5 mm and a chimney wall thickness of 1.0 mm). Pad h is the key opening, i.e. pad height
above the rim, from [D]. Pad Ø is the key-cup diameter from [D]. Perf. is the diameter of the
open-hole (French) perforation.

| # | Hole (acoustic) | Operated by (technician name) | Normal state | x [D] | x [R] | from foot | cell | Ø [D] | Ø [R] | pad h [D] | pad Ø | perf. Ø |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | D#6 trill hole | D# trill key (RH2 trill lever) | closed | 200.7 | 202 | 399.7 | 29.9 | 7.4 | 6.5 | 1.6 | 13 | – |
| 2 | D6 trill hole | D trill key (RH1 trill lever) | closed | 218.0 | 218 | 382.4 | 32.3 | 7.8 | 6.5 | 1.6 | 13 | – |
| 3 | C#5 (register vent) | LH1 ("C key") | open | 234.4 | 236 | 366.0 | 34.5 | 7.0 | 6.5 | 1.9 | 13 | – |
| 4 | C5 | thumb, either lever ("B key") | open | 266.9 | 267 | 333.5 | 39.0 | 13.2 | 13 | 2.6 | 19 | – |
| 5 | B4 | "Bb key": no touch, closed by thumb Bb lever, LH2, RH1, or Bb side lever | open | 286.1 | 286 | 314.3 | 41.6 | 13.2 | 13 | 2.2 | 19 | – |
| 6 | Bb4 | LH2 ("A key") | open | 307.1 | 309 | 293.3 | 44.5 | 13.2 | 13 | 2.5 | 19 | 6.8 |
| 7 | A4 | LH3 ("G key") | open | 330.3 | 331 | 270.1 | 47.6 | 13.2 | 13 | 2.4 | 19 | 6.8 |
| 8 | G#4 | G# key (LH4) | **closed** | 352.0 | 354 | 248.4 | 50.6 | 13.4 | 13 | 2.4 | 19 | – |
| 9 | G#4 duplicate | "second G key": no touch, closed with LH3 (and by RH2 with split E) | open | 352.5 | – | 247.9 | 50.7 | 13.4 | – | 2.5 | 19 | – |
| 10 | G4 | "F# key": no touch, closed by RH1, RH2 or RH3 | open | 376.9 | 378 | 223.5 | 54.0 | 14.2 | 13 | 2.1 | 19 | – |
| 11 | F#4 | RH1 ("F key") | open | 402.6 | 404 | 197.8 | 57.5 | 14.2 | 13 | 2.3 | 19 | 6.8 |
| 12 | F4 | RH2 ("E key") | open | 429.9 | 430 | 170.5 | 61.3 | 14.2 | 13 | 2.5 | 19 | 6.8 |
| 13 | E4 | RH3 ("D key") | open | 458.7 | 460 | 141.7 | 65.2 | 14.2 | 13 | 2.4 | 19 | 6.8 |
| 14 | D#4 | Eb key (RH4) | **closed** | 490.3 | 490 | 110.1 | 69.6 | 15.4 | 15 | 2.8 | 20.6 | – |
| 15 | D4 | low C# key (RH4 roller) | open | 524.3 | 524 | 76.1 | 74.2 | 15.4 | 15 | 2.8 | 20.6 | – |
| 16 | C#4 | low C key (RH4 roller); also closes #15 | open | 557.3 | 558 | 43.1 | 78.7 | 15.4 | 15 | 2.8 | 20.6 | – |
| end | C4 (C foot) | – | – | 600.4 | – | 0 | 84.6 | 19.0 | – | – | – | – |
| B1 | C4 hole (B foot only) | low B-foot C key | open | 592.3 | 594 | – | 83.5 | 15.4 | 15 | 2.8 | 20.6 | – |
| endB | B3 (B foot) | – | – | 633.3 | 638 | – | 89.2 | 19.0 | – | – | – | – |

Cross-checks:
- **Trill and C# holes on Boehm's flutes [M] (Miller):** D# trill Ø7.6 at 216.3 mm from the cork
  (≈ 199 mm from the embouchure); D trill Ø7.6 at 233.4 mm (≈ 216); C# Ø6.6 at 253.5 mm (≈ 236).
- **Main-hole diameters [M]:** Boehm's practical maximum was 13.5 mm in silver and 13.0 mm in wood.
  Miller measured "most silver body and foot holes 13.2 mm, largest 13.4 mm". Some makers grade
  them, e.g. Heindl 11.4 → 13.6 mm in 0.2 mm steps.
- **Armstrong flute [F]:** C#4 hole at ≈ 559 mm (Pearl 557.3).
- **Spacing and grid:** holes #8 and #9 share one grid cell, as do #1/#2 within ±1 cell. The
  spacing ratio follows 2^(1/12), except hole #3, which is moved up to serve as the D5/D#5/D6
  register vent [R][W].
- **Optional extra keys, not in [D]:** C# trill key (beside the Bb trill lever, [WFG]; position
  unknown); B-foot "gizmo" (a lever on the B key only, no extra hole); D# roller (mechanical only).
  **Gap:** no position or Ø found for the C# trill hole.
- **Chimney heights:** I found no per-hole data beyond [D]'s uniform 1.9 mm (wall plus chimney) and
  1.5 mm rim. Drawn or soldered chimneys on silver flutes are ~1.5–2 mm (M). The thin wall
  (0.4 mm) means the chimney is what gives the hole its depth [R].

## 4. Keys, pads, coupling, travel

### 4.1 Normal states (no fingers)
Closed: #1, #2 (trill keys), #8 (G#), #14 (Eb). All others are open ([W]: "the trill keys, the home
key for the left fourth finger and the home key for the right fourth finger are closed when not
touched"; [WFG]). The Eb key is held open by RH4 for almost every note except D4, D5, Bb6, B6 and
C7 ([WFG]).

### 4.2 Coupling rules: hole closed if any listed control is down
These reproduce the 16-character hole strings in §5. They were derived from the hole naming in [D]
and [R] together with the [WFG] fingerings, and are consistent with every basic fingering (M).

```
#3  C#      : LH1
#4  C       : ThumbB | ThumbBb
#5  B(Bb key): ThumbBb | LH2 | RH1 | BbSideLever      ("1-and-1" Bb uses RH1)
#6  Bb      : LH2
#7  A       : LH3
#8  G#      : closed unless LH4(G#) pressed             (normally closed)
#9  G#dup   : LH3 | (splitE && RH2)
#10 G       : RH1 | RH2 | RH3                           (F# clutch; RH3 alone gives F#4)
#11 F#      : RH1
#12 F       : RH2
#13 E       : RH3
#14 D#      : closed unless RH4 Eb pressed              (normally closed)
#15 D       : C#roller | Croller
#16 C#      : Croller
#1  D#trill : open only if RH2 trill lever ; #2 Dtrill: open only if RH1 trill lever
```

- **Split E (Wiki):** RH2 also closes the second G (#9), so E6 vents only through #7. Without it, E6
  vents through #7 and #9.
- **Brossa F# lever, and the Bb trill/side lever [WFG]:** the side lever closes #5. Brossa: not modelled.
- **F# clutch** (forum, and patent US4353281): the F# key (#10) is closed indirectly whenever the
  F, E or D key (RH1/2/3) is closed.
- **Open-G# flutes** (Boehm's original) invert #8: open, closed by LH4. Not needed for a standard
  instrument.

### 4.3 Key opening (pad height above the hole rim)

| Key group | Pearl PF-661 measured [D] (H) | Yamaha 2xx spec [Y] (L) | Notes |
|---|---|---|---|
| Trill keys (#1, #2) | 1.6 | 2.40 | [JLS]: trills and LH C get the smallest step |
| LH1 C# hole (#3) | 1.9 | 2.55 ("C") | |
| Thumb (#4) | 2.6 | 2.75 | |
| Bb key (#5) | 2.2 | – | |
| LH2, LH3 (#6, #7) | 2.5, 2.4 | 3.10 (A, Bb, G) | |
| G#, G# dup (#8, #9) | 2.4, 2.5 | 3.45 (G#) | [JLS]: G# often set level with the stack, or lower |
| F# key, RH1–3 (#10–13) | 2.1, 2.3, 2.5, 2.4 | 3.15 (D, E, F, F#) | [JLS]: the stack is set from the F key |
| Foot Eb, C#, C (#14–16) | 2.8 | 3.35 | [JLS]: foot keys are highest; D# sometimes lower |

- **Rule of thumb [M]:** "pad rise ≈ 3 mm for 13.2 mm holes", i.e. h/Ø ≈ 0.2–0.25.
- **[JLS]:** makers fall into "open" and "close" camps, so ±0.3 mm across instruments is normal.
- **Model default (G):** stack 2.5 mm, foot 3.0 mm, trills 1.8 mm, thumb 2.6 mm. Make it a
  per-key parameter.

### 4.4 Pads
- **Cup diameters [D]:** 13 mm (trills, C#), 19 mm (main), 20.6 mm (foot). These are key-cup radii
  6.5 / 9.5 / 10.3 mm. Pad (cup) thickness used in [D]'s formula: 3.6–4.2 mm.
- **Pad set sizes (Dawkes, older Yamaha):** 12, 16.5, 18, 19.5 mm (M). Pad thickness ≈ 2–2.8 mm
  (Lucien pads: 2.8 mm standard, 2.0 mm thin).
- **French-model perforation:** Ø6.8 mm (radius 3.4) on LH2, LH3, RH1, RH2, RH3 (five keys [Wiki],
  [D] H). Half-holing a perforation is a valid extra control. [WFG] lists ¼, ½, ¾ ring coverings.

### 4.5 Timing
- **Key open/close time [A] (H):** ≈ **10 ms when pushed by a finger, ≈ 16 ms when moved by a
  spring** (e.g. lifting a finger off a normally-open key, or releasing the G#/Eb keys).
- **Finger asynchrony in multi-finger changes:** "tens of ms", smaller for professionals [A].
- **Suggested openness slew:** smoothstep over 10 ms (finger) or 16 ms (spring). Add random
  per-finger offsets of 0–20 ms for realism (G).

## 5. Standard fingering chart C4–C7 (C foot) as hole states

Columns 1→16 follow the §3 hole numbering (embouchure → foot): `o` = open, `x` = closed.
Fingerings are from [WFG]. The hole states were computed with the §4.2 rules. "T" = thumb B
lever, "Tb" = thumb Bb lever, L1–L3 / R1–R3 = finger keys, G# / Eb / C#r / Cr = little-finger
keys, Dtr / D#tr = trill keys.

| Note | Fingers down | holes 1…16 | open holes |
|---|---|---|---|
| C4 | T L1 L2 L3 R1 R2 R3 Cr | `xxxxxxxxxxxxxxxx` | – (foot end) |
| C#4 | T L123 R123 C#r | `xxxxxxxxxxxxxxxo` | 16 |
| D4 | T L123 R123 (Eb up) | `xxxxxxxxxxxxxxoo` | 15 16 |
| D#4 | T L123 R123 Eb | `xxxxxxxxxxxxxooo` | 14–16 |
| E4 | T L123 R1 R2 Eb | `xxxxxxxxxxxxoooo` | 13–16 |
| F4 | T L123 R1 Eb | `xxxxxxxxxxxooooo` | 12–16 |
| F#4 | T L123 R3 Eb | `xxxxxxxxxxooxooo` | 11 12 14 15 16 |
| G4 | T L123 Eb | `xxxxxxxxxooooooo` | 10–16 |
| G#4 | T L123 G# Eb | `xxxxxxxoxooooooo` | 8 10–16 |
| A4 | T L1 L2 Eb | `xxxxxxoxoooooooo` | 7 9 10–16 |
| Bb4 (thumb) | Tb L1 Eb | `xxxxxooxoooooooo` | 6 7 9 10–16 |
| Bb4 (1-and-1) | T L1 R1 Eb | `xxxxxooxoxxooooo` | 6 7 9 12–16 |
| B4 | T L1 Eb | `xxxxoooxoooooooo` | 5 6 7 9 10–16 |
| C5 | L1 Eb | `xxxooooxoooooooo` | 4–7 9 10–16 |
| C#5 | Eb | `xxoooooxoooooooo` | 3–7 9 10–16 |
| D5 | T L2 L3 R123 (Eb up) | `xxoxxxxxxxxxxxoo` | **3** (vent) 15 16 |
| D#5 | T L2 L3 R123 Eb | `xxoxxxxxxxxxxooo` | **3** (vent) 14 15 16 |
| E5 … C#6 | same as E4 … C#5 (overblown, 2nd register) | | |
| D6 | T L2 L3 Eb | `xxoxxxxxxooooooo` | 3 (vent), 10–16 |
| D#6 | T L123 G# R123 Eb | `xxxxxxxoxxxxxooo` | 8 (vent), 14–16 |
| E6 | T L1 L2 R1 R2 Eb | `xxxxxxoxoxxxoooo` | 7 9 13–16 (split E: 9 closed → `xxxxxxoxxxxxoooo`) |
| F6 | T L1 L3 R1 Eb | `xxxxxoxxxxxooooo` | 6 12–16 |
| F#6 | T L1 L3 R3 Eb | `xxxxooxxxxooxooo` | 5 6 11 12 14 15 16 |
| G6 | L1 L2 L3 Eb | `xxxoxxxxxooooooo` | 4 10–16 |
| G#6 | L2 L3 G# Eb | `xxooxxxoxooooooo` | 3 4 8 10–16 |
| A6 | T L2 R1 Eb | `xxoxxxoxoxxooooo` | 3 7 9 12–16 |
| Bb6 | T R1 Dtr (Eb up) | `xooxxooxoxxooxoo` | 2 3 6 7 9 12 13 15 16 |
| B6 | T L1 L3 D#tr (Eb up) | `oxxxooxxxooooxoo` | 1 5 6 10–13 15 16 |
| C7 | L1 L2 L3 G# R1 (C foot; B foot adds the gizmo) | `xxxoxxxoxxxooxoo` | 4 8 12 13 15 16 |

The second octave (E5–C#6) uses the first-octave fingerings and is selected by the jet (higher
pressure, shorter W). D5/D#5 use #3 as a register vent. D6 is the G4 fingering with #3 vented, and
plays as the 3rd harmonic [W]. [D]'s template marks MIDI 60/61/62 as `XXXXXXXXXXXXXXXX` /
`…XO` / `…XOO`, which matches the first three rows.

## 6. Acoustic modelling of open and closed holes

Notation: a = bore radius (9.5 mm), b = hole radius, δ = b/a, t = hole height (wall + chimney,
1.9 mm), S_h = πb², Z_h = ρc/S_h, k = ω/c. A hole is a T-junction: a shunt impedance Z_s, plus two
series impedances Z_a/2 (negative length, usually small) on each side.

### 6.1 Length corrections (all lengths in metres or mm consistently)
- **Matching volume** [Dal02 eq.6], [Ned98 eq.37]:
  `t_m = b·δ·(1 + 0.207·δ³)/8`
- **Inner end correction** [Dal02 eq.4], [Ned98 eq.40]:
  `t_i = b·(0.82 − 1.4·δ² + 0.75·δ^2.7)`.
  Lefebvre–Scavone fit [LS12]: `t_i = b(0.822 − 0.095δ − 1.566δ² + 2.138δ³ − 1.640δ⁴ + 0.502δ⁵)`.
- **Series (anti-symmetric) length, open hole** [Dub99 eq.74]:
  `t_a(open) = −b·δ² / (1.78·coth(1.84·t/b) + 0.940 + 0.540·δ + 0.285·δ²)`.
  [D] drops the coth term (→ 1), which keeps it t-independent. Keefe 1990 form:
  `t_a = −0.47·b·δ⁴/(coth(1.84 t/b) + 0.62δ² + 0.64δ)`.
- **Series length, closed hole:** the same Dubos formula with t replaced by the closed height.
  [D] uses `1.78/tanh(1.84·t/b)`.
- **Outer radiation end correction, chimney tube** (low frequency): unflanged 0.6133·b, infinite
  flange 0.8216·b [Dal01]. A thin chimney wall (1 mm) is close to unflanged. For a hole in a
  cylinder wall without a chimney, [D] uses `0.8216·b − 0.47·b·(b/(a+t))^0.8`.
- **Frequency dependence and resistance:** radiation resistance ≈ Z_h·(kb)²/4 (unflanged). [D]
  adds an empirical resistance `0.4·Z_h·(kb)²` to keyed open holes. Wall losses in the chimney follow
  the usual viscothermal law. The Helmholtz peak at "several kHz" [W] is beyond the cut-off (~2 kHz).

### 6.2 Open hole with a pad at height h (Dalmont et al. 2001, as implemented in [D])
Extra length from a disk (pad) of radius D held at height h above a tube of radius b with wall
thickness w:

```
Δ_pad = b / ( 3.5·(h/b)^0.8·(h/b + 3w/b)^(−0.4) + 30·(h/D)^2.6 )      [Dal01 eqs 48,51,52]
if perforated (perforation radius q, cup thickness e):
  (Δe/b) = 1.64·b/q − 0.15·b/D − 1.1 + e·b/q²
  Δ_pad  = Δ_pad / (1 + 5·(Δe/b)^(−1.35)·(h/b)^(−0.2))
```
[D] adds `+0.1·b` empirically (`CORR_OPEN_KEYED_HOLE_LENGTH`).

**Simple physical cross-check (G, my derivation):** radial slot of height h between the pad and the
rim gives `Δ_slot = b²·ln(D/b)/(2h)`. For b = 7.1, D = 9.5 it agrees with Dalmont within 5 % for
h ≤ 2.5 mm:

| h [mm] | 0.5 | 1 | 1.5 | 2 | 2.5 | 3 | 4 | 5 | 6 |
|---|---|---|---|---|---|---|---|---|---|
| Δ_pad Dalmont (b=7.1, D=9.5) | 12.5 | 7.1 | 4.9 | 3.6 | 2.7 | 2.1 | 1.3 | 0.9 | 0.6 |
| Δ_slot simple | 14.7 | 7.3 | 4.9 | 3.7 | 2.9 | 2.5 | 1.8 | 1.5 | 1.2 |

So at the real pad heights (2.1–2.8 mm) the pad adds ≈ 2.5–3.5 mm to the hole's acoustic height,
compared with ≈ 10 mm without the pad. This flattens the note slightly. Halving the opening
(h ≈ 1.2 mm) roughly doubles the hole's effective length, which models venting and half-holing.

**Total open-hole shunt** (low frequency, lossless):

```
Z_s(open) ≈ jωρ·t_e/S_h,   t_e = t + t_m + t_i + t_r + Δ_pad(h)     (+ R terms)
```

Computed for the Pearl holes with [D]'s data (mm; t_r = 0.6133 b; this is my own calculation, G):

| holes | b | t_i | t_m | t_a open | t_a closed | Δ_pad | t_e open (with pad) | t_e no pad | V closed [mm³] |
|---|---|---|---|---|---|---|---|---|---|
| 1, 2, 3 (small) | 3.5–3.9 | 2.4–2.5 | 0.2 | −0.2 | −0.15 | 1.1–1.6 | 7.7–8.7 | 6.6–7.0 | 80–100 |
| 4–7 (Ø13.2) | 6.6 | 2.80 | 0.61 | −0.99 | −0.62 | 2.4–2.9 (perforated 1.4) | 10.7–12.3 | 9.4 | 344 |
| 8, 9 (Ø13.4) | 6.7 | 2.79 | 0.63 | −1.03 | −0.64 | 2.5–2.7 | 12.0 | 9.4 | 357 |
| 10–13 (Ø14.2) | 7.1 | 2.70 | 0.72 | −1.21 | −0.73 | 3.4 (perforated 1.6–1.7) | 11.2–13.1 | 9.7 | 415 |
| 14–16 (Ø15.4) | 7.7 | 2.51 | 0.87 | −1.51 | −0.88 | 2.8 | 12.8 | 10.0 | 515 |

### 6.3 Closed hole (pad down)
- **Chimney cavity compliance:** `C = V/(ρc²)` with `V = S_h·(t + t_m) + V_pad`. Here V_pad is the
  dent or recess left by the pad (≈ 0; [D] uses `t_keypad = −0.05·b`, i.e. a slightly bulging pad
  that reduces the volume).
- **Exact form** [K90], [D]: `Z_s(closed) = −j·Z_h·cot(k·(t + t_m + t_pad)) + jωρ·t_i/S_h`.
  At flute frequencies kt ≪ 1, so this is a pure compliance in series with the small inner mass.
- **Size of the effect:** sum of the 16 closed volumes ≈ 5–6 cm³, against ≈ 170 cm³ of bore. This
  lowers the low notes by a few cents and shifts the hole spacing (G).
- **Series term:** keep `t_a(closed)` (≈ −0.6…−0.9 mm), or drop it (it is a small sharpening).

### 6.4 Unified model for a moving pad (recommended for R8; G, built from the above)
One hole = bore node → inner mass `L_i = ρ(t_i + t_m)/S_h` → cavity node with compliance
`C_c = (S_h·t + πD²·h)/(ρc²)` → slot mass `L_s(h) = ρ·Δ_pad(h)/S_h` in series with the slot viscous
resistance `R_s(h) = 6μ·ln(D/b)/(π·h³)` (radial Poiseuille flow between disks) → radiation mass
`L_r = ρ·0.61·b/S_h` plus radiation resistance → atmosphere.

- h → 0: L_s and R_s → ∞, leaving the pure closed-cavity compliance (§6.3).
- h ≈ 2.5 mm: the result matches §6.2.
- This needs one extra state (cavity pressure) per hole, which fits a staggered leapfrog scheme.
- Map key openness s ∈ [0,1] → h = s·h_max. Pin h_min ≈ 0.05 mm so that 1/h³ stays finite. Use
  s = 0 → exactly closed.
- The Helmholtz frequency of the chimney is ≈ 10 kHz or more. If it is stiff at 48 kHz, substep
  it, or use the 2-element form (mass + compliance) only when s < 0.2.
- **Non-linear loss at open holes at high amplitude** [Dal02]: add
  `R_nl ≈ K·ρ·|u|/(2·S_h²)` with K ≈ 0.6–1 (G for flute holes). This matters mostly at ff and for
  small trill or vent holes.

## 7. Gaps and caveats
- Hole-by-hole chimney heights were not found; only [D]'s uniform 1.9 mm (1.5 mm rim) exists.
  Pad heights differ between Pearl (measured, 1.6–2.8 mm) and the Yamaha spec relayed on a forum
  (2.4–3.45 mm; unverified).
- The joint lengths (head/body/foot) are estimates. No authoritative modern spec was found. They
  are irrelevant acoustically.
- No published Cooper or Bennett scale table (full positions and diameters) was found online. The
  Pearl ([D]) and Jupiter ([R]) data agree within ≤ 2.5 mm, which is well below dx = 7.3 mm.
- C# trill hole: position and Ø unknown. The gizmo and Brossa levers are not modelled.
- Lip coverage is known only as the uncovered width (6 → 3.5 mm, [F75]/Coltman) or the lumped
  h√g = 0.3 [W]. A coverage-vs-note curve beyond that is a guess.
- Key travel 10/16 ms is from one study [A]. The pad-height-vs-time profile within that interval
  is not known (assume smoothstep).
