# Review: instrument geometry, tone holes, fingerings, intonation

Reviewer specialty: bore and hole geometry, the hole branch model, fingerings, tuning.
Date: 2026-11-11. Repo state: 701a84a. I did not modify the source tree. All experiments were run on a
copy in the session scratchpad:
- `…/scratchpad/review_geometry/`: repo copy and build.
- `…/scratchpad/exp/tmm2.py`: tools/tmm.py with switches for t_a, pad formula, closed volume, c,
  the embouchure extra length, and the fingering table read from player.cc.
- `…/scratchpad/dk/`: Dickens' thesis code (github.com/buha/flute-thesis), built with a 5-line GSL shim
  and an env-var patch. It runs his **PlayedImpedance** model of this very Pearl PF-661 (C-foot XML).

Notation: "passive" = admittance peak seen from the window (`-imp`, tmm), in cents against ET A440.
"played" = `PJ_NOCAL=1 ./flute -scale`.

## Summary

1. **The steep first-octave slope and the wide octaves are mostly real physics of the passive flute with
   a constant embouchure, not a tone-hole bug.** Dickens' own model of the same flute, with a constant
   face/lip load and pajupilli's c = 347 m/s, gives C4 −43 → C#5 +26 c (a 69 c slope) and an E4→E5
   octave of +33 c. pajupilli's TMM gives 76 c and +37 c; its waveguide gives 95 c and +35 c. The
   body and holes agree with Dickens to within about 7 c of slope. The rest comes from the waveguide's
   discretisation and from the embouchure.
2. Real players remove this with an **embouchure load that grows with pitch**. Dickens fitted it
   explicitly as `faceZ ∝ (2.937 ln midi − 11.63)`, ×4.5 from C4 to C7, and the Coltman schedule
   does the same job. Without the face term, Dickens' model has a 119 c slope; with it, 45–50 c.
   So even the reference model leaves ~50 c that the jet/player must absorb. A ±2 c "passive in tune"
   target is unrealistic.
3. **The calibration's extremes are not physical.** At C4 it uses 9.7 mm uncovered: 94 % of the
   10.3 mm hole, and wider than the model's own jet length (6.2 mm at C4). Pressure ×1.7 at C4 and
   ×2.0 in the third octave are tuning crutches. They stand in for two model offsets:
   (a) the played pitch sits **20–35 c below** the passive peak, whereas Fletcher et al. 1982 state
   that the sounded mode "normally lies rather above" the passive one; and (b) +7…13 c grid errors at
   C4, G4 and C5.
4. The fingering table matches the WFG basic charts for C4–C7 (all 37 checked hole by hole). The
   `register` field is unused and partly wrong.

## Findings ranked by impact

### F1. Played pitch is ~24 c *below* the passive resonance. Literature says at or above. (High)
- **Evidence.** The played table (no calibration) minus the TMM with the same Coltman coverage
  schedule gives −21…−34 c in the first octave and −6…−26 c in the second.
  Example: C4 passive −28 → played −62; A4 −6 → −29; E5 +7 → −14.
  - Fletcher, Strong & Silk 1982 (JASA 71, p. 1255): the sounded lowest mode frequency is "normally
    lying rather above the corresponding passive mode frequency".
  - Wolfe et al. 2001 fixed their 6 mm embouchure load "so that the impedance minima correspond to
    the played notes", which also puts played at or near passive.
- **Why it matters.** This uniform flattening is the main reason the low register needs a nearly
  uncovered hole and ×1.2–1.7 pressure. If played sat ~0…+10 c above passive, C4 would need about
  6–7 mm uncovered: Coltman's range.
- **Fix (owner: the jet/excitation reviewer, flagged from the tuning side).** The flattening comes
  from the jet phase at the operating point, which sets how far the oscillation is pulled off the
  passive peak.
  - Suspects: `PJ_CONV = 0.175` (cp/Uj; literature 0.3–0.5 for the convection speed of jet waves),
    together with `jetLen = 100/√f`. These two set the jet transit phase.
  - Test: sweep `PJ_CONV` and `PJ_W` with `PJ_NOCAL=1 ./flute -scale`. Aim for played − passive ≈
    0…+10 c across C4–C6 at mf.
- **Effort:** M. **Risk:** M (timbre and register selection depend on the same phase).

### F2. The embouchure model: one constant "+2 mm" trades slope against octave stretch. (High)
- **Evidence.** `tmm2.py` with a fixed 6 mm uncovered width, varying the PJ_EMB_LEN equivalent:

  | extra (mm) | slope C4→C#5 | octave E4→E5 | mean, 1st octave |
  |---|---|---|---|
  | 0 | +93 c | +34 c | +25 c |
  | 2 (current) | +76 | +37 | −2 |
  | 6 | +41 | +42 | −53 |
  | 10 | +19 | +49 | −100 |

  - Any constant inertance flattens the slope only by making octaves wider and everything flat.
  - Wolfe's measured equivalent load (h√g = 0.3, i.e. L_c/S_c ≈ 167 m⁻¹ of radiation inertance) is
    about 2× the model's at 6 mm (55 m⁻¹ radiation + 29 m⁻¹ from the 2 mm extra).
  - Dickens instead uses an entry cone (outer radius 3.9 mm vs r_out 5.7 mm), an extra inner end
    correction 0.5 b δ² (+0.8 mm here), a negative series t_a at the embouchure, an empirical
    R = Z0·6.9e-6 f and G = 1.3e-4 f/Z0, and the pitch-dependent face term.
  - My mass-only approximation of Dickens' embouchure in tmm2 gave a +18 c slope against Dickens'
    own +45 c. **Details of the embouchure riser move the slope by ±25 c**: it is the most sensitive
    part of the instrument model.
- **Fix.**
  1. Replace PJ_EMB_LEN by a per-note face/lip inertance, Λ_face(m) = k·(2.937 ln m − 11.63)·0.82 r_e/(π r_e²)
     with r_e ≈ 3.9 mm, or by an equivalent `extra(m)`. Keep Coltman's width schedule as the lip
     coverage part.
  2. Model the riser as the undercut cone (r_in 6.4 → r_top 5.7 mm, 4.4 mm + t_m), not as
     `4.3/√(sw·sbot)`, and add the inner t_a (≈ −1.2 mm in bore units).
  3. Fit k so that the calibration's coverage trims come out near 1.
- **Benefit:** calibration trims shrink to ±1–2 mm; octave stretch falls from ~+30 c to below ~10 c
  (Dickens' face model: E4 +9 c, E5 +16 c).
- **Effort:** M (tmm first, then the window branch in flute.cc). **Risk:** L–M.

### F3. Calibration ranges are physically implausible. Constrain them and report the residual. (High, for honesty and for the "player" feel)
- **Evidence.** `./flute -cal`:
  - C4–A4 uncovered 7.3–9.7 mm with pressure ×1.0–1.7.
  - D#6–C7: 2.0–2.8 mm with ×1.45–2.0, still +9…+30 c on F#6, G6, G#6, A6 and B6.
  - Coverage is non-monotonic between neighbours (F4 8.1, F#4 9.3, G4 9.6, G#4 8.2), because the
    pressure steps and the grid errors (F4) are absorbed into coverage.
- **What real players do.**
  - Coltman (via Fletcher 1975): uncovered width 6 mm at C4 → 3.5 mm at A6, with the lip-to-edge
    distance l = 1.8 + 100/√f mm. The uncovered width cannot exceed about l − 1 mm
    (≈ 7 mm at C4, ≈ 3.3 mm at A6).
  - Rolling bends pitch by about ±20–40 c, and blowing pressure at a given note varies about 2–3× from
    pp to ff (both approximate figures from the general literature, not verified here). Players
    *compensate* the pitch effect of pressure with coverage rather than using pressure to
    tune.
  - Plausible tuning trims: coverage ±1.5 mm around the schedule, pressure ×0.85–1.25.
- **Fix.**
  - Clamp the uncovered width to ≤ min(10.3, jetLen + 1 mm) and ≥ 2.5 mm.
  - Limit the pressure trim to ×0.85–1.25.
  - Couple rolling to jet length (rolling in shortens W).
  - Log the residual and fix the model (F1, F2, F4) instead of widening the search.
- **Effort:** S. **Risk:** L. The tuning table will look worse until F1/F2 land, but it will be truthful.

### F4. Grid errors: the foot end and the embouchure cell are not compensated; holes up to ±13 c. (Medium)
- **Evidence.** Waveguide `-imp` minus TMM, same embouchure: C4 −13, G4 −7, C5 +13, A4 +6, C6 +6,
  Bb6 −8; all others within ±5 c.
  - `setBore` rounds the foot to 85 cells (620.6 mm) instead of 617.9 mm. In the TMM, a 2.7 mm
    longer foot gives exactly −6.7 c on C4 and 0 on everything else.
  - `ec_ = int(17.5/7.30) = 2` puts the window at 18.25 mm from the cork: the cavity is 0.75 mm
    longer than measured.
  - The hole compensation d·S_h/S (chimney length) is a first-order fix. For the thumb hole (moved
    3.4 mm) it leaves +13 c on C5.
- **Fix.**
  1. Subtract the rounding (foot_·dx − footX) from the horn throat `horn0_` (0.6133 a → ~3.1 mm).
  2. Correct the cork cavity by scaling the cells' areas above the window, so that volume and
     inertance match 17.5 mm.
  3. For holes, compensate with a weighted split across the two neighbouring cells, or keep a
     per-hole residual table fitted against the TMM (as a hidden constant, not as player
     calibration).
- **Benefit:** removes the ±7–13 c note-to-note jitter that the calibration now hides.
- **Effort:** S (foot, cork); M (holes). **Risk:** L.

### F5. Missing series t_a (negative inertance) on open *and* closed holes. (Medium-low)
- **Evidence.** Dubos 1999 eq. 74, as used by Dickens, applied in tmm2: C4 +11 c, D4 +15, A4 +14,
  C#5 +6, second octave +13 c, third +9 c.
  - Closed holes contribute about −0.7 mm (big holes, t = 1.9 mm) each. Low notes have many closed
    holes above them, so they gain the most.
  - The slope falls only 4 c (76 → 72), but the overall level rises 10–15 c. Together with F6 this
    removes the "everything flat" pressure on coverage.
- **Fix.** Add the per-cell series correction −t_a/S_bore to the momentum equation's inertance at the
  hole cells. It is static for closed holes; interpolate it with openness.
- **Effort:** M (bore.cc update of `bu`). **Risk:** L.

### F6. Speed of sound. (Low-medium, uniform)
- The model uses c = 347 m/s (≈ 26.5 °C dry). Dickens (Cramer formula) uses 30.3 °C at the
  embouchure, saturated air, 2.5 % CO₂ (exhaled breath), and −7.7 °C/m: c = 349.5 m/s at the
  embouchure.
- Effect: **+12 c uniform** (the gradient adds about −3 c at C4 relative to C#5).
- Wolfe measured at 25 °C and states that played notes are about 30 c above those minima.
- **Fix:** kC = 349.5 (it changes dx, so re-check the hole cells), or keep dx and scale with a knob.
- **Effort:** S. **Risk:** L.

### F7. Smaller hole-model items. (Low)
- **Pad formula.** The slot formula b² ln(D/b)/(2h) against Dalmont 2001 plus Dickens' +0.1 b:
  ≤ ±2 c in the first two octaves, +5 c on C#5. Keep the slot (it extrapolates sanely to h → 0).
- **Closed volume.** Adding t_m − 0.05 b to the closed chimney length: −3 c on the low notes. Add it
  for consistency.
- **Radiation.** R = 1.5 ρc/S_h ∥ L(0.6133 b) gives Re → ρc/S·(kb)²/4: correct unflanged
  low-frequency loss. Dickens adds an empirical 0.4 Z_h (kb)² for keyed holes (more damping, no
  pitch effect).
- **Inner correction.** The t_i polynomial (Dubos form) and Dickens' 0.82 − 1.4δ² + 0.75δ^2.7 differ
  by < 0.05 mm at δ = 0.7. Negligible.
- **Merged holes 8 and 9** (0.5 mm apart): fine at dx = 7.3 mm. The parallel combination already
  conserves the inertance.

### F8. Third octave stays sharp: consistent with the reference, so do not over-fit. (Medium)
- **Evidence.** Passive (tmm2, Coltman schedule) mean +26 c in the third octave, against −9 c in the
  first. Dickens' played model: third octave +13…+49 c over its first octave at the same
  temperature.
- Real flutes are known to run sharp up there. Players cover more, blow slower and "direct the air
  down" (a smaller y0 offset and a shorter W).
- **Fix.** Let the schedule continue its coverage trend above C6 (Coltman's 3.5 mm at A6 →
  ~3 mm at C7), and once F1 lands, use a lower jet velocity for the third octave rather than ×2
  pressure. The calibration's ×2 pressure there is probably finding the only setting that stays in
  the register, not the in-tune one.

## Fingering table (src/player.cc)

All 37 hole strings were checked against WFG `fl_bas_1/2/3` and the coupling rules in
keywork.md §4.2: **all correct**, including:
- D6 `T-23|---Eb`, D#6 `T123G#|123Eb`, E6 `T12-|12-Eb`, F6 `T1-3|1--Eb`, F#6 `B1-3|--3Eb`;
- G6 `-123|---Eb`, G#6 `-23G#|---Eb`, A6 `T-2-|1--Eb`, Bb6 `T---|1 Dtr`, B6 `B1-3|D#tr`;
- C7 `123G#|1--` (C foot, no Eb).

Issues:
- The `register` column is unused (grep), and wrong for several third-octave notes:
  - D#6 sounds the 4th mode of a D#4-like fingering (311 × 4 = 1244 Hz);
  - A6 is a cross-fingered upper mode;
  - C7 is labelled 4.

  Either drop the column or use it, e.g. to seed the jet length (W) for register selection.

Alternatives worth adding (passive effect from tmm2, same coverage):

| Use | Fingering | Holes | Effect |
|---|---|---|---|
| Split E (mechanism) | E6 with #9 closed | `xxxxxxoxxxxxoooo` | −2 c; real benefit is response/stability (the Wolfe/UNSW "split E" pages) |
| Bb4/5 "1-and-1" | T1--\|1-- Eb | `xxxxxooxoxxooooo` | same pitch as thumb Bb; needed for chromatic passages |
| Bb side lever | T1--\|--- + lever | as thumb Bb | mechanical only |
| F#4/5 middle-finger alternate | T123\|-2- Eb | `xxxxxxxxxxoxoooo` | −10 c (flatter, sometimes used to correct sharp F#) |
| F#6 RH2 alternate | B1-3\|-2- Eb | `xxxxooxxxxoxoooo` | −8 c vs standard |
| C#5/C#6 "stabiliser" | add RH2, RH3 | `xxoooooxoxoxxooo` | 0 c passive; real players use it for response, so model value is low |
| Trills | D-trill / D#-trill keys over C#–D, C–D, D–E, … | already modelled holes 1, 2 | needs the trill-lever coupling in player |
| Harmonics | low fingerings overblown (C4 → C5, G5, C6, E6) | existing strings + a register target | requires a per-note "mode" target in the player (see F3/F1) |
| Eb key on D5 (some players) | D5 + Eb | `xxoxxxxxxxxxxooo` | brighter/sharper D5 |

## Bore and hole data
- The bore profile and the hole table match Dickens' XML exactly (positions, radii, pad heights,
  cup radii, perforation radius 3.4 mm, wall 1.9 mm; cork radius 8.4, emb 8.534, 9.4 mm cylinder,
  9.9 mm socket, 9.5 mm body).
- One note: the XML file, as published, concatenates the C-foot tail **and** the B-foot section.
  Anyone re-running Dickens' code must delete the B-foot block (I did).
- Pearl's production scale is reportedly A = 442 (not verified for PF-661). If so, expect a uniform
  ~+8 c at A440.

## Recommended order
1. **F4 foot/cork compensation and F6 c (S, ~1 h).** Removes 7 c of C4 error and adds 12 c of
   uniform sharpening. Re-run `-cal`.
2. **F3 calibration constraints (S).** Makes the remaining model error visible.
3. **F1 jet operating point (M, with the jet reviewer).** Expected to remove the ~25 c uniform
   flattening that drives the 9.7 mm coverage.
4. **F2 embouchure riser cone + per-note face inertance (M).** Removes most of the octave stretch;
   fit against tmm2 and Dickens' PlayedImpedance.
5. **F5 t_a series terms (M).** Correctness, +10 c low end.
6. Fingering alternatives (S each). Drop or use the `register` column.

## Open questions
- **Does the played pitch really sit above passive on a modern flute at mf, and by how much per
  register?** The basis is Fletcher 1982's statement and Coltman's work. Numbers are needed:
  Coltman 1966/1968 JASA or Dickens' thesis ch. 6 (the PDF was not retrievable here).
- What was Dickens' `faceZ(midi)` fitted to: played notes on this Pearl at what temperature and
  pitch standard? His model still shows a 45 c slope at 26.5 °C. Either the jet offset is
  note-dependent, or the PF-661 itself has a rising scale (a student flute; C#5 is notoriously
  sharp on real flutes).
- How much of the low-register flatness is the time-domain window branch (mass + R∥L) versus a
  distributed riser? A tmm-vs-waveguide comparison at several window widths would separate them.
- The UNSW virtual-flute tables (`notes.php?…&order=intonation`) returned 404. The JSV+ data files
  would give measured minima per fingering for a direct check of F2.
