# The player's airway, and the MPK mini mk3 MIDI map

Research for REQUIREMENTS.md R10–R12 (how much of the player to model) and R17 (the controller).
Date: 2026-11-11. Every claim has a source tag like [F75]; the list is at the end of each part.
**(est.)** marks my own estimates and **(guess)** marks unverified claims.

---

## PART A — How much of the human player must be modelled?

### A1. Short answer

On a flute the player's airway is upstream of a high-resistance lip slit, and the jet leaves the
lips into **open air**. It does not enter a closed mouthpiece as it does on a reed or brass
instrument. The bore therefore cannot "push back" on the mouth through a valve in series. Tract
resonances couple only weakly.

- Measured pitch effect: ≤ ~10 cents [C73, A16].
- Measured timbre effect: up to ~5 dB on the even harmonics, and only in extreme cases [A16].

The parts of the player that matter audibly are:

1. the **blowing pressure** and its slow modulation (vibrato) [F75];
2. the **lip slit**: area, aspect ratio, jet length and coverage. These are the main tone controls
   [F75];
3. the **onset** set by the tongue valve and the mouth plenum (tonguing, attack shape);
4. **turbulence** (breath noise).

A full vocal tract, the vocal folds and the trachea are optional extras for special effects:
singing-while-playing, growl, and "vowel" colouring.

### A2. Why the coupling is weak: the numbers

**Blowing pressure.** Fletcher measured four professional flutists [F75]:

- median mouth pressure p ≈ 0.008·f mbar (f in Hz), within about ±50 %;
- that gives C4 ≈ 2 mbar = 210 Pa, C5 ≈ 420 Pa, C6 ≈ 840 Pa and C7 ≈ 1.7 kPa;
- some players hold p constant from pp to ff, while others vary it by about 2×.

**Lip slit.**

- The slit is an oval with an axial ratio of 10:1 to 20:1 [F75].
- It is up to ~12 mm wide (low and loud) and ~5 mm wide (high and soft) [F75].
- So its height h ≈ 0.3–1.2 mm and its area A ≈ 2–14 mm².
- Every player makes forte by **enlarging the lip opening**, not mainly by raising the pressure
  [F75].

**Jet length.** l ≈ 1.8 + 100·f^(−1/2) mm (±20 %), with little change with dynamics [F75]. That is
about 8 mm at C4 and 3.5 mm at C7. Coltman's uncovered embouchure width runs from 6 mm (C4) to
3.5 mm (C7) [F75 citing Coltman].

**Lip-channel impedance (est.).** Take A = 10 mm² and p = 400 Pa (≈ C5):

| Quantity | Value |
|---|---|
| Bernoulli jet speed v = √(2p/ρ) | ≈ 26 m/s |
| Flow U | ≈ 0.26 L/s |
| Differential resistance R = dp/dU = 2p/U = ρv/A | ≈ **3 MPa·s/m³** |
| Inertance of a ~3 mm effective neck, L = ρℓ/A | ≈ 360 kg/m⁴ |
| ωL at 1 kHz | ≈ 2.3 MPa·s/m³ |

The resistance grows at high notes, as A shrinks and p rises.

**Tract impedance seen from the lips (est.).** A 3 cm² tube has ρc/A ≈ 1.4 MPa·s/m³, and the
impedance peaks are a few times that. UNSW gives "tens of MPa·s/m³" for reed and brass **bore**
peaks, against which the tract is "a short circuit" in the first-order model. The flute was
explicitly set aside in that statement [W10].

**Topology, which is the decisive part.** In reed and brass instruments, the tract and the bore act
**in series** on one valve: ΔP = U·(Z_mouth + Z_bore) [W10]. A tract peak of the same size as a
bore peak can then take over the playing frequency, as in saxophone altissimo or clarinet bends
[W10].

In the flute it works differently:

- The jet speed is set by p_mouth − p_outside_lips.
- The outside pressure at the lips is the near field of an **open** embouchure (a pressure node of
  the bore), so it is a small fraction of the bore's acoustic pressure.
- The tract feels the bore only through that small outside pressure. The divider
  Z_tract / (Z_tract + Z_lip) then shrinks it further.
- Auvray et al. state that the reed-instrument "series impedance" approach "cannot be applied to
  flute-like instruments" [A16].

**Measured coupling gain.** This is the mouth-pressure fluctuation relative to the bore acoustic
pressure, G_p = p′/p_ac [A16]:

| Condition | G_p |
|---|---|
| Recorder player, normal (large) mouth, C6 = 1047 Hz | ≈ −60 dB |
| Same player after deliberately shrinking the mouth | ≈ −20 dB, phase moving 0 → π |
| Another player opening the velum | ≈ −30 dB max |

**Effects at that gain** [A16]:

- At −20 dB the even harmonics rise by about 5 dB and the odd ones stay flat or fall. The change
  is not systematic across players.
- On an artificial mouth at −14 dB:
  - the pitch moves about 7 cents with the coupling phase;
  - the regime-change thresholds move by about ±7 %;
  - the odd/even balance moves by up to 30 dB over all phases. This is a lab extreme.
- Mechanism: a **symmetric (varicose)** jet perturbation from the pulsating mouth flow, on top of
  the antisymmetric deflection by the bore flow [A16].

**Coltman 1973.** On an artificial mouth, changing the mouth volume shifted the fundamental by
about 10 cents, with no observed change in spectral content [C73, as summarised in A16].

**Conclusion.** Expect the tract to be inaudible in normal playing: a −40…−60 dB coupling. It
becomes a small but real timbre and stability control when a player deliberately narrows the
mouth on high notes: −20 dB, ±5 dB on the even partials, ≤ 10 cents.

### A3. The individual parts

**Lungs and abdomen.**

- Lung and chest compliance is huge compared with anything acoustic, so the lungs are an ideal
  **slow pressure source** (est.).
- Fletcher explains the near-universal ~5 Hz vibrato rate as a resonance of the lung volume
  (~10⁻² m³) loaded by the diaphragm and abdominal mass (~10 kg). The resonance comes out near
  5 Hz. Neuromuscular-loop timing is an alternative explanation [F75].
- Model them as p_lung(t): a set-point with a 10–50 ms slew, plus vibrato.

**Larynx (glottis).** The evidence on vibrato is mixed:

- Fletcher: "nearly all" flutists vibrate with the diaphragm, at ~5 Hz and ±10 % of the blowing
  pressure. Throat vibrato is "unusual" [F75].
- Hwang & Thorpe, nasendoscopy of six expert flutists [HT00]:
  - The glottal aperture oscillates in step with the vibrato.
  - The swing averages 7.42 mm when loud and 1.73 mm when soft.
  - The authors conclude the larynx modulates the airflow, and so the mouth pressure.
- A Japanese laryngo-fiberscope study saw the glottis open and narrow 4–6 times per second during
  vibrato [MK].
- Boysen & Brown (videostroboscopy) found "active vocal fold motion in all rapid vibratos" [BB21].
- The Sydney video-nasendoscopy thesis finds the larynx and pharynx active in tone and dynamics,
  not just in vibrato [FIO].
- I could not locate Gärtner's EMG study directly. It is only cited second-hand, as "larynx always
  active in vibrato" **(unverified)**.

Acoustically, both mechanisms end up as a modulation of the mouth pressure. The open glottis
(~1–2 cm²) has far less resistance than the lip slit (~0.1 cm²): the ratio scales as (A_lip/A_g)²,
about 1 %. Its resistance only matters when the glottis narrows strongly (est.).

**What vibrato sounds like** [F75]:

- frequency modulation < 1 %;
- strong amplitude modulation of the **upper partials**, a "timbre vibrato" that emerges from the
  nonlinear jet;
- so the vibrato does not need to be painted on: a modulated p_lung is enough.

**Vocal folds vibrating (singing while playing, growl).** The folds then chop the flow at the voice
f0, so the mouth pressure carries a large fluctuation at f0. That modulates the jet and produces
the characteristic beating, sum and difference tones. I found no quantitative flute study, only
informal sources **(guess on the magnitudes)**. This is a special effect, not part of normal tone.

**Tongue.**

- On a very short time scale the tongue gates the flow "in an almost binary fashion" (ta, da, ka,
  la) [WS03].
- This is the articulation that matters most.
- Flutter tonguing is an alveolar trill or a uvular trill, at roughly 25–35 Hz, which is the
  speech trill rate **(guess, from phonetics, not a flute measurement)**.
- Tongue position and shape (vowel) influence the **broadband** component and, weakly, the first
  ~10 harmonics. Chen et al. found this on the recorder, with no tuning of the tract to the note
  [CL07, via A16].

**Lips.**

- These are the most important "player" element after pressure [F75].
- The lips set:
  - the slit area and aspect ratio (jet thickness and flow);
  - the jet length l;
  - the coverage of the embouchure hole (tuning and timbre);
  - the jet angle and offset y0.
- Flutists "control lip aperture size and geometry" [WS03].
- The jet cross-section strongly affects the upper partials [F75].

**Mouth as a plenum (est.).**

- Volume ≈ 50–100 cm³ gives a compliance C = V/(ρc²) ≈ 5×10⁻¹⁰ m³/Pa.
- With the lip R ≈ 3 MPa·s/m³: τ = RC ≈ 1.5 ms.
- With the lip L ≈ 360 kg/m⁴, the Helmholtz frequency is ≈ 375 Hz at Q ≈ 0.3, so it is
  **overdamped**.
- After a tongue release the mouth discharges through the lips in about 1–2 ms. This shapes the
  jet's onset, not its steady tone.
- The real cavity also connects through the pharynx (its inertance is of the same order as the
  lips'). A lumped plenum is wrong in detail above a few hundred Hz. It doesn't matter much, given
  the weak coupling in A2.

**Breath noise.**

- The jet is transitional or turbulent: Re = v·h/ν ≈ 26 m/s × 1 mm / 1.5×10⁻⁵ ≈ 1700 (est.).
- 2-D simulations of flute-like jets sustain oscillation at Re ≈ 1000–2000 and become chaotic
  above that [DFD20].
- The main noise source is the turbulent jet hitting the labium edge. Its strength is
  pressure-dependent and it is spectrally shaped by the bore resonances.
- Turbulence generated **in the mouth**, behind a front-of-mouth constriction, de-correlates the
  jet and lowers the harmonics. That is a possible mechanism for "vowel" timbre (2-D CFD on a
  recorder) [NB25].
- So the tract shape can change the noise and colour a little, but there is no flute measurement
  of how large the effect is.

### A4. Recommendation for pajupilli

Model the following, in order of realism gained per unit of effort:

1. **p_lung(t)**: a slewed set-point (10–50 ms) plus vibrato. Vibrato is a 4.5–6 Hz modulation of
   ±0–15 % of the pressure, optionally also driving the glottal valve. This alone gives the
   correct timbre vibrato [F75]. *Biggest gain.*
2. **Lip slit as a nonlinear resistance plus inertance.** U obeys
   L·dU/dt = p_mouth − p_out − ρU|U|/(2A²), with A = slit area as a player control.
   - Jet speed v = U/A feeds the jet-drive model.
   - Jet height h = A/width.
   - Forte = larger A, with p roughly constant [F75].
   - W (jet length) and coverage are separate controls; seed them from l(f) and from Coltman's
     uncovered widths.
3. **Tongue valve** between the mouth and the lips: an area 0…1 that closes instantly and opens
   over about 2–10 ms **(est.)**.
   - Pressure builds behind it, so the release gives a natural overshoot.
   - Attack strength comes from MIDI velocity.
   - Flutter tonguing = the valve modulated at about 25–30 Hz with partial closure.
4. **Mouth plenum**: a single compliance of 50–100 cm³ between the tongue valve or glottis and the
   lips. It costs one state variable and gives the 1–2 ms onset and discharge dynamics.
5. **Turbulence noise** injected at the jet and labium: an amplitude ∝ (Re² − Re_c²) above a
   threshold, the same law puhuja uses. Add a small "mouth-turbulence" knob that also lowers jet
   coherence [NB25].
6. **(Optional) glottal valve**: a variable resistance between the lungs and the mouth, for the
   larynx-vibrato variant [HT00] and for breathy, constricted attacks. It is cheap.
7. **(Optional, later) symmetric jet modulation from the mouth**: feed back a fraction G_p (−60…−20
   dB, with an adjustable phase) of the embouchure acoustic pressure into p_mouth. This is the
   recorder result's mechanism [A16] and gives the ±5 dB even-harmonic and ≤ 10-cent effect
   cheaply.
   - The physical version is a short (~16-section) tract line between the plenum and the lips,
     with the lip outlet pressure taken from the embouchure near field.
   - Do this only if the simple version sounds right.

**Skip:**

- a full articulated vocal tract for normal tone;
- the nasal tract;
- the trachea as a waveguide;
- vocal-fold oscillation, except as an optional "sing/growl" effect later.

All of these are below the −40 dB coupling level in normal playing [A16, C73].

### A5. What to reuse from puhuja

**Reuse.**

- **`line.h` / `line.cc` `TransmissionLine`** is the same formalism as R1:
  - staggered leapfrog p/u;
  - an area per section;
  - float32 aligned in blocks of 16 for AVX-512;
  - viscous and wall loss;
  - a Bernoulli orifice loss at area steps (the `oriInv_` term).

  It is the natural template for the **bore**, so adapt it rather than rewrite it. It needs:
  - SI units, or the CGS conversion handled once (it uses CGS units);
  - Courant 0.99 (it uses 0.9);
  - tone-hole side branches.

  If item 7 above is ever done physically, a 16-section instance of it is the tract.
- **The turbulence-noise law**, Re-based and from puhuja's TRACT_MODEL §5, carries over directly to
  breath noise.
- **The lung model** ("a slow resistive source" at the far end of the trachea) is the right idea
  for p_lung.

**Not now.**

- `string.cc` / `folds.cc` (2 × 16-lane vocal-fold string): useful only for the singing-while-
  playing or growl effect. The interface takes subglottal and supraglottal pressure, so it could
  be dropped between p_lung and the mouth plenum later.
- The vowel and consonant shape machinery (`tract.cc`), nasal line and walls: not needed.

---

## PART B — Akai MPK mini mk3: default MIDI behaviour

Akai publishes **no MIDI implementation chart** for this device [IN22], so some defaults below
come from third-party sources. **Verify on the unit** with `aseqdump -p "MPK mini 3"` when it is
plugged in. It was not connected to this machine when I checked `aconnect -l`.

### B1. Linux / ALSA

- **USB class-compliant.** The ALSA sequencer client is named **"MPK mini 3"**, with port
  **"MPK mini 3 MIDI 1"**.
  - tsmetana/mpk3-settings finds the device by the name prefix "MPK mini 3" [TS].
  - An Ardour user selects "MPK mini 3 MIDI 1 (In)" [ARD].
  - `aseqdump -p "MPK mini 3"` is used in [YD].
- Auto-connect by matching the client-name prefix "MPK mini 3".

### B2. Controls (default program: Program 1, aligned to MPC Beats [FAQ])

**Keys (25).**

- Velocity-sensitive, with a 10-octave range via Octave −/+ [QS].
- **No aftertouch on the keys** [SOS].
- Typical velocities are low: often < 64, and about 100 at most. Full Level does not affect the
  keys [IN22]. (The SOS review says Full Level works on both pads and keys [SOS]; trust [IN22]
  and test.)
- A hidden velocity-curve mode exists: hold Full Level for about 5 s, then K1–K4 set the curve
  points and K5 sets the black-key balance [IN22].
- So pajupilli should apply its own velocity curve.

**MIDI channel.**

- Keys and knobs ("Keybed & Controls") and pads have **separate channels** in each program [TS].
- The usual setup is keys on channel 1 and pads on channel 10. Akai's support article tells users
  to set exactly that, keys 1 and pads 10 [AK-ch]. [YD] shows the same.
- That this is the factory default for Program 1 is **(likely, verify)**.
- Listen on all channels and route by event type.

**Octave and transpose.**

- Octave −/+ shifts by up to ±4 octaves. Pressing both buttons resets [QS].
- Each program also stores a transpose of ±12 semitones [TS].
- These are applied inside the device: the note numbers sent are already shifted.

**Knobs K1–K8.**

- 360° endless encoders that send CC [QS].
- **CC 70–77** by default:
  - Knob 1 = CC70 and Knob 2 = CC71 [OMNI];
  - K1–K8 = 70–77 [YD].
- Per program, each knob stores a mode (absolute or relative), a CC, a min and max, and a name
  [TS, SOS].
- Default mode is **absolute, 0–127 (likely, verify)**.
- In absolute mode the knob keeps an internal value and sends 0–127, so there is no pickup jump
  from the controller's side.
- **Note:** while Arp or Note Repeat is held, the knobs edit those settings instead [QS].

**Joystick (4-way thumbstick).**

- Defaults: left/right = **pitch bend** (14-bit, centre 8192, both directions); up/down = a "mod
  wheel effect" [QS].
- Each axis can be set to one of three modes [FAQ, TS]:

  | Mode | Behaviour |
  |---|---|
  | Pitch bend | Pitch bend in both directions |
  | Single CC | One CC per axis, centre = 64 |
  | Dual CC | Two CCs per axis, centre = 0, one CC per direction, each 0–127 |

- The program stores the CC numbers for the negative and positive directions of each axis [TS].
- My reading of "mod wheel effect" is that up sends CC1 0→127 and down sends nothing or a second
  CC (dual mode) **(guess, verify)**.
- The stick springs back to centre, so it is not a "hold" control.

**Pads (8, two banks: A red, B green).**

- Velocity- and **pressure**-sensitive [QS].
- Pad pressure is **off, channel aftertouch or polyphonic aftertouch**, set per program [TS, SOS].
  The factory default is **(unknown, verify)**.
- Modes [QS]:
  - Note (default);
  - **CC** button: pads send CC instead of Note On;
  - **Prog Change** button: pads send Program Change.
- Default numbers, **(likely, verify)**:

  | Mode | Bank A | Bank B | Source |
  |---|---|---|---|
  | Note | 36–43 (C1–G1, Akai labels "C2") | 44–51 | Akai support says pad 1 of bank A = note 36 [AK-pad]; mk1 factory preset uses 36–43 [GLJ] |
  | CC | 16–23 | 24–31 | [YD] |
  | Program Change | 0–7 | 8–15 | (guess) |

- **Full Level:** the pads always send velocity 127 [QS].
- **Note Repeat:** a held pad retriggers at the tempo and time division [QS].
- **Prog Select + pad:** loads program 1–8. Each program is a stored settings set [QS, FAQ].

**Sustain pedal.**

- 1/4" TS jack [QS]. Expected to send **CC64** (0/127) on the keybed channel **(standard, verify)**.
- Whether the device auto-detects pedal polarity is unknown.

**Arpeggiator, tap tempo and clock.**

- Arp On/Off; hold it with a knob or key to edit [QS].
- Program fields [TS]:
  - mode: up, down, exclusive, inclusive, order, random;
  - division: 1/4 to 1/32T;
  - octave 1–4;
  - swing 50–75 %;
  - latch;
  - tempo 60–240 BPM (taps 2–4);
  - clock internal or external.
- The arp generates the Note On/Off messages itself. pajupilli should just tell users to keep it
  **off**. A monophonic flute will follow the arp notes like any others.

**SysEx (for a later "load pajupilli program" feature).**

- Manufacturer 0x47, product 0x49, command 0x64 write / 0x66 query / 0x67 reply.
- 254-byte program dump; program 0 = RAM.
- The offsets are in tsmetana's `message.h` [TS].

### B3. Suggested mapping for pajupilli (suggestion, not a requirement)

| Control | Use |
|---|---|
| Keys | Fingering: a new key = fingering change (slur) while sounding, or a tongued attack if no other key is held. Velocity = tongue-release speed and pressure overshoot. |
| Joystick Y (up) | Breath pressure offset (expression), or vibrato depth |
| Joystick X | Pitch bend, realised as lip coverage / jet angle (a flutist's "rolling in/out"), not as a frequency shift |
| Knobs | Base pressure, lip-slit area (dynamics), W, coverage, y0, noise, vibrato rate and depth, mouth volume / G_p |
| Pads | Articulation modes (single tongue, flutter, breath attack ("ha"), key clicks, overblow harmonic). Use poly or channel pressure as continuous breath where enabled. |
| Sustain | Legato hold: keep the air on between notes |

### Sources

- **[F75]** N. H. Fletcher, "Acoustical correlates of flute performance technique," JASA 57(1),
  233–237 (1975).
  https://openresearch-repository.anu.edu.au/bitstream/1885/125582/1/Acoustical%20correlates%20of%20flute%20performance%20technique.pdf
- **[C73]** J. W. Coltman, "Mouth resonance effects in the flute," JASA 54, 417–420 (1973). Read via
  the summary in [A16], not the full text. Listing: https://ccrma.stanford.edu/marl/Coltman/Papers.html
- **[A16]** R. Auvray, A. Ernoult, S. Terrien, P.-Y. Lagrée, B. Fabre, C. Vergez, "Effect of changing
  the vocal tract shape on the sound production of the recorder: an experimental and theoretical
  study," arXiv:1601.05545 (2016). https://arxiv.org/abs/1601.05545
- **[CL07]** J.-M. Chen, D. Laurin, J. Smith, J. Wolfe, "Vocal tract interactions in recorder
  performance," Proc. 19th ICA, Madrid (2007). Cited via [A16].
- **[W10]** J. Wolfe, J.-M. Chen, J. Smith, "The acoustics of wind instruments – and of the musicians
  who play them," Proc. ICA 2010, Sydney.
  https://www.phys.unsw.edu.au/jw/reprints/WolfePlenaryICA2010.pdf
- **[WS03]** J. Wolfe, A. Almeida, J.-M. Chen, D. George, N. Hanna, J. Smith, "The player–wind
  instrument interaction," SMAC 03. https://www.phys.unsw.edu.au/jw/reprints/SMACinteraction.pdf
- **[HT00]** J.-H. B. Hwang, C. W. Thorpe, "Laryngeal configuration and acoustical properties during
  flute vibrato," JASA 108(5 Suppl.), 2591 (2000).
  https://pubs.aip.org/asa/jasa/article/108/5_Supplement/2591/552895
- **[MK]** "Laryngeal movements during wind instruments play" (Japanese, laryngo-fiberscope).
  https://www.researchgate.net/publication/20410664 (authors not checked)
- **[BB21]** Boysen & Brown, "Laryngeal vibrato production in flutists," Flutist Quarterly (2021),
  via Gale. https://go.gale.com/ps/i.do?id=GALE%7CA676446175
- **[FIO]** "The Flute Inside-Out: Tracking internal movements in flute playing," Univ. Sydney
  thesis. https://ses.library.usyd.edu.au/handle/2123/17992
- **[NB25]** N. Nordblom, R. Bader, "Vocal tract turbulence and recorder sound – a CFD approach,"
  Forum Acusticum 2025. https://dael.euracoustics.org/confs/fa2025/data/articles/000964.pdf
- **[DFD20]** APS DFD 2020 abstract, "Flow instabilities in flute-like instruments."
  https://meetings-archive.aps.org/dfd/2020/p19/4/
- **[QS]** Akai Professional, MPK mini (mk3) Quickstart Guide v1.3. Copy hosted by Amazon:
  https://m.media-amazon.com/images/I/B1SIdD1oPFL.pdf
- **[FAQ]** Akai Pro, MPK mini mk3 FAQ.
  https://support.akaipro.com/support/solutions/articles/69000798861
- **[AK-ch]** Akai Pro support, "How to play pads and keys at the same time in any DAW."
  https://support.akaipro.com/support/solutions/articles/69000876799
- **[AK-pad]** Akai Pro support, "How to program MPK mini pads in a DAW."
  https://support.akaipro.com/support/solutions/articles/69000859755
- **[SOS]** Sound On Sound, review of the Akai MPK Mini MkIII.
  https://www.soundonsound.com/reviews/akai-mpk-mini-mkiii
- **[TS]** tsmetana/mpk3-settings (Linux editor), `src/message.h` and `src/device.c`.
  https://github.com/tsmetana/mpk3-settings
- **[GLJ]** gljubojevic/akai-mpk-mini-editor README (original MPK mini SysEx and factory presets).
  https://github.com/gljubojevic/akai-mpk-mini-editor
- **[IN22]** incenp.org, "Akai MPK mini3 velocity" (2022).
  https://incenp.org/notes/2022/akai-mpk-mini3-velocity.html
- **[YD]** yduf, "Akai MPK mini MK3" notes. https://yduf.github.io/akai-mpk
- **[OMNI]** Spectrasonics, "Setting up the MPK mini mk3."
  https://support.spectrasonics.net/manual/Omnisphere3HW/3/en/topic/setting-up-the-mpk-mini-mk3
- **[ARD]** Ardour forum, "How can I map/remap pads from a MIDI controller."
  https://discourse.ardour.org/t/how-can-i-map-remap-pads-from-a-midi-controler/108701
