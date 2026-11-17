# Each pipe its own connection to the church: design, prototype, measurements

The user: *"the individual pipes could have more distinct connection to the room."* This builds on
research/church_organ.md §7a (positions, two-way coupling), §7b.2 (the church on one core), §7d.4
(images to second order, band-shaped, discrete) and §8a (what is built). Tags as there: **[meas]**
measured here, **[calc]**, **[design]**.

Prototype (standalone, not the project's code): `proto.cc` + `part1.inc` (geometry, path tracer,
filters, a late FDN), `encl2.h` (a copy of src/organbox.h with one switch added), `ana.py`
(metrics, WAVs), in the session's scratchpad `roomcoupling/`. It links the real `FluePipe`
(src/organpipe.h) read-only, with organ.cc's Principal and Gedackt voicings (knob defaults), and
organ.cc's enclosures and chest layout.

## 1. Short answer

- **Today** every pipe of a division reaches the microphones only through its enclosure's single
  output; the Mic stage then sends that output to both channels (below 300 Hz identically, above it
  a few dB to its side). Two pipes in one enclosure differ only in the enclosure's mode gains and
  their distance to its front, so their early responses are practically identical: **cross-
  correlation 1.00** for two neighbouring pipes' mouths, **0.99** for the same key in the front and
  back rows. L/R correlation of a dry render 0.93; the time difference between the channels is 0 for
  every pipe **[meas]**.
- **Recommended:** every sounding pipe's mouth (and an open pipe's top, a reed's resonator top) gets
  its **own direct path and its own image sources to second order** (side walls, west wall, vault,
  nave floor, gallery floor, its case's back panel), traced in the church's geometry with the case
  around it (open front, the Rückpositiv's shadow, diffraction at the impost). Each path is one
  **integer-delayed copy** with its 1/r, its surfaces' loss, the microphone's pattern and the pipe's
  directivity. It lands in one of 30 **class rings** (2 microphones x 5 classes x 3 shadow levels).
  Each ring has one short minimum-phase filter: the image band of §7d.4 for its order, the air at
  20 °C and 58 % RH for its path length, and the shadow. The filters are LTI and shared, so summing
  first and filtering after is exactly the same as filtering every copy on its own. The pipes write
  their 16-sample blocks with one vector add per path. The enclosures keep their near field (mode
  shapes, `heard()`, the shutters); only their per-pipe direct sound is switched off, and their
  field leaves as one source per enclosure with its own images.
- **Result [meas, 15 m into the nave, 2 m AB pair]:** two neighbouring mouths' early responses
  correlate 0.58 (today 1.00), front against back row 0.46 (today 0.99). Across the 305 pipes the
  first arrival's channel time difference spans -1.65..+1.04 ms (today ±0.02) and the level
  difference -9..+15 dB. A dry render's L/R correlation is 0.05-0.11 (today 0.93). There are 15
  discrete arrivals above -20 dB in the first 80 ms (today 3).
- **Cost [meas]:** **7.4 ns per sounding pipe-sample** (44 paths for both microphones, mouth and
  top), about 18 % of a pipe measured in the same run (42 ns, an open principal). A 300-pipe full
  organ needs 2.2 µs a sample, **11 % of the church's core**. Rings 1.4 MB. Cheaper fallbacks: to
  first order per pipe 3.4 ns, direct only 1.3 ns. Every pipe can have it; no per-region fallback is
  needed in the budget of §7b.

## 2. What the current scheme does [read: src/organ.cc, src/organbox.h]

- `OrganModel::renderEnclosure(e)`: each awake pipe of enclosure e hears `enc.heard(src)` (the box's
  4 lines weighted by its mode shapes at the mouth) plus 4 neighbours, then adds its mouth (and top)
  into the enclosure (`Enclosure::add`: the 4 lines by its mode gains, and `direct_` delayed by its
  distance to the front, gain 0.5/dist). `Enclosure::step(church)` returns direct + field, through
  the shutters for a swell box, delayed by `outDelay`.
- `OrganModel::sample()`: the Mic stage. Each enclosure's output passes a 300 Hz crossover; the
  lows go to both channels alike, the highs with gL = 1 - p, gR = 1 + p (p = pan x 0.5), with no
  time difference (`PJ_ORGAN_ITD` 0).
- Nothing about the church: in `-play -room church` the whole organ is one source of the shared
  `Room`, with its 24 images and its FDN.

So a pipe's place shows only in which of the 10 enclosures it stands and in its small direct delay
inside it. A chromatic run alternates between two enclosures (C side / C# side), but within a side
every pipe sounds from the same point.

## 3. The design [design; prototyped]

### 3.1 The church and the case, as geometry

| Item | Prototype values (placeholders, per organ) |
|---|---|
| Nave | 50 x 20 x 20 m (x across, north +; y up; z east from the west wall) |
| West gallery | floor y = 7 m, from the west wall to z = 5.5 m: a slab that blocks; its top a finite wooden reflector |
| Main case (Hauptwerk + impost) | x ±2 m, y 7-13.3 m, z 1.6-3.0 m; open front from y = 10.3 m (the impost) up; back panel at z = 1.6 m |
| Façade | in front of it (z 3.05-3.45 m), mouths at y = 9.5 m; behind it the main case's front (a wooden reflector) |
| Swell boxes | x ±(1.0-3.6) m, y 13.4-15.8 m, z 1.3-2.9 m; open front through the shutters |
| Rückpositiv | x ±1.15 m, y 7-9.3 m, z 4.65-5.35 m (on its stand at the rail); small: it **shadows** (diffraction) rather than blocks |
| Pedal towers | x ±(2.7-4.5) m, y 7-13.2 m, z 1.1-2.9 m |
| Listener | two omni-to-cardioid microphones (a + (1-a) cos, a = 0.6), 2 m apart, turned ±55° out (the convention of src/room.cc), ears 1.2 m high, 2 m south of the axis, **15 m** (default here) or 27 m from the west wall |

Every pipe's mouth sits at its enclosure's centre plus its `Pipe::at` (organ.cc's chest layout,
unchanged). An open pipe's top is 0.95 λ/2 above its mouth. A reed sounds from its resonator's top,
which organ.cc already puts in `at`.

### 3.2 A pipe's paths

For each source (mouth, top) and each microphone, every sequence of up to two reflections off
{south, north, west walls; nave floor; vault; gallery floor; its own back panel} is traced (mirror
images, then back-traced specular points). A path is valid when:
- each specular point lies on its surface (the gallery floor and the back panel are finite);
- no leg passes through the gallery slab or another case;
- the first leg leaving a case exits through the case's open front. If it crosses the front within
  0.6 m below the aperture's lower edge (the impost) or above its upper edge, it is **diffracted**:
  -3 dB and one shadow step, two steps beyond 0.3 m. A leg through the Rückpositiv is likewise
  shadowed;
- its delay is within 80-95 ms of the source's first arrival. Later images (the east wall at
  131-288 ms) are left to the late field, as §7d.4 decided.

Per path: delay r/c (integer samples), gain = 10^(surfaces' loss/20) / r x the microphone's pattern
toward the arriving direction, and a **shadow** in [0, 1] quantized to 0 / ½ / 1:
- the flue mouth's directivity: the pipe's body hides its back, so 0.8 x (1 - cos θ)/2 with θ
  between the leaving direction and the case's front normal. An open top or a region source is
  omni;
- the rows in front of it: 0.12 per chest row (the façade and the front row 0);
- 0.7 per shadow step (the Positiv, the impost edge).

The surfaces' broadband losses at 1 kHz are those of §7d.4: side walls -1.3 dB, west wall -1.5,
vault -1.2, floor with pews -4.2, gallery floor and back panel (wood) -1.5 / -1.0.

At 15 m a Hauptwerk pipe has ~9 valid paths per microphone (mouth), at 27 m ~15: the case cuts the
steep downward ones. For principal8 C4 (C side) at the left microphone **[meas]**: direct;
floor +4.6 ms -5.0 dB; south wall +18.2 ms -4.2 dB at -51°; south-floor +21.6; north +39.3 at
+63°; vault +43.6; ... south-north +75.7 at -72°. Its C# neighbour on the other side reaches the left
microphone through the Positiv's shadow (-3 dB, darker), its south wall image 5 ms later.

13 of the 305 prototype pipes have no direct path at 15 m: the Gedackt's bass octave along the swell
box's back wall, below the shutters' line. They are heard through the box's field, as in a real
swell **[meas]**.

### 3.3 Rendering: class rings by blocks

- **Classes** by order and path length: direct; first order near / far; second order near / far
  (split at the median path, 20 / 30 m at 15 m). Each has its mean path length for the air.
- **Rings:** 2 mics x 5 classes x 3 shadow levels = 30 float rings of 4096 samples (paths cut at 80
  ms) or 8192 (95 ms), all delays less a common offset D0 (the earliest arrival).
- **Scatter:** a pipe's block of 16 samples (its gain applied) is added into each of its paths' rings
  at `w + delay`: one contiguous 16-float multiply-add per path, which vectorizes (one AVX-512 lane
  set).
- **Readout:** at the block's end each ring's 16 samples at `w` go through the ring's filter chain
  and are summed per microphone. The swell boxes' pipes write into their box's own ring set,
  read through that box's shutter (the same two-pole low-pass, gain and wall leak as
  `Enclosure::step`, at the box's current opening) before joining.
- **The class filters** (minimum-phase first-order sections): a 300 Hz one-pole high-pass per
  reflection order, and above 1 kHz a one-pole low-pass plus a first-order shelf, fitted to the
  §7d.4 band (revised: -10 / -20 dB at 16 kHz) plus the air at 20 °C / 58 % (ISO 9613-1) over the
  class's mean path. A half / full shadow adds a -4 / -8 dB shelf above 1.5 kHz. Measured responses
  (dB) at 15 m:

| class (mean path) | 125 | 250 | 500 | 1 k | 2 k | 4 k | 8 k | 12 k | 16 k |
|---|---|---|---|---|---|---|---|---|---|
| direct (16.5 m) | 0 | 0 | 0 | 0 | -0.1 | -0.5 | -1.6 | -3.2 | -5.4 |
| 1st, near (20 m) | -8.3 | -3.9 | -1.4 | -0.5 | -0.5 | -1.4 | -4.9 | -9.9 | -16.5 |
| 2nd, far (39 m) | -16.6 | -7.8 | -2.8 | -1.1 | -1.4 | -4.2 | -11.7 | -19.3 | -27.7 |

- Each image is thus a discrete, delayed copy with a short minimum-phase filter (§7d.4: the spread
  under 0.1 ms), as the user specified. Sharing a filter across a class is exact (linear), not an
  approximation. The approximations are the class's mean air length (±2.5 dB at 16 kHz across a
  class) and the shadow's three steps.
- **Integer delays are enough [meas]:** 2-tap fractional delays (twice the taps) changed the
  neighbours' cross-correlation from 0.57 to 0.55. The difference between two neighbouring pipes'
  paths is many samples, not a fraction of one: 8 cm moves a lateral image by up to 0.2 ms.

### 3.4 The enclosures stay: their near field, their field into the church

- `Enclosure` keeps everything that is near: the pipe's mouth exciting the box's modes by its place,
  `heard()` (the jet hearing the box at its mouth), the neighbours, the shutters, the walls' leak.
- Its **per-pipe direct sound** (`direct_`, the 0.5/dist copy to its front) is switched off
  (`directOut = 0`): the pipe's own paths replace it.
- Its **field** (the 4 lines' sum x `field`, through the shutters) leaves as one **region source**
  at the enclosure's opening with its own direct path and images (omni, unshadowed): the case's
  reverberation radiated from its mouth.
- The swell's per-pipe paths pass the shutters (§3.3), so closing the box darkens and quietens both
  the pipes' direct sound and the box's field, as before.

### 3.5 The room back into each pipe [design; not prototyped]

What each pipe hears now: its enclosure's field at its mouth (per pipe through the mode shapes),
4 neighbours (sample-exact), and the church's pressure `pRoom_`, one signal for the whole organ fed
into every enclosure's lines (`Enclosure::step(church)`).

- **Per region:** each enclosure gets its own church input, `church_[e][s]`:
  - the nave's late field at the enclosure, a 16-tap projection of the room's FDN with a direction
    vector per region (16 MAC x 10 regions = 160 MAC a sample);
  - the other regions' direct sound: each enclosure's output delayed by its distance (2-7 m: 6-20
    ms, longer than a block) and 1/r, 10 x 9 taps a sample.
- **Per pipe:** in a diffuse field the coherence between two points is sinc(kr) **[calc]**. Pipes
  0.1-0.5 m apart hear the same field below ~300 Hz and independent fields above ~1 kHz. Give each
  pipe a fixed mix of two of 4 decorrelated projections of its region's field, chosen by its
  position: `heard += pp.hearRoom * (a * f[e][k1] + b * f[e][k2])`, 2 multiply-adds a pipe-sample
  (~0.3 ns).
- **Its strength [calc]:** the late field at any pipe is 16π/A ≈ -10 dB against each sounding
  pipe's level at 1 m (A = 537 m² at 6 s). With 300 pipes sounding that is +15 dB over one pipe at
  1 m. A pipe's own field at its mouth (2-5 cm) is +26..+34 dB, so the room is 10-20 dB under it,
  reaching the jet through `kHear` (0.3) and FluePipe's 0.05: a small pull, as §7a.2 expects
  (Mitnahme by the room is weak against the neighbours').
- Reeds stay deaf to it (`PJ_REED_HEAR` 0, §8a: their loud sound howled through their enclosure).

### 3.6 The late field and the shared `Room`

The organ's own stage now produces the direct sound and the early images per pipe. The late field
(§7b.2: organ-9.wav's T60s, one core) belongs to the church. In `-play`, src/room.* would add its
own 24 images for the organ as a single source, so the organ needs to be a **late-only source**
there (its early taps skipped, its FDN input kept, ideally with the onset ramp of §7d.4) and to get
the room's pressure per region (§3.5). That interface is for the room's owner to agree; until then
the organ renders its early part alone (as the prototype's B) and `-room church` adds the late field
(and, without the flag, a second set of images).

## 4. Measurements [meas]

Setup: the pipes of five ranks, 305 in all: Hauptwerk Principal 8′ (bass C2-F3 in the façade),
Octave 4′, Quinte 2⅔′, Superoctave 2′ (rows 0-3 of the HW chest), and the Schwellwerk's Gedackt 8′
(two boxes, shutters ¾ open). Three schemes from the same pipe signals:
- **A0**, today: enclosure (direct + field) → Mic stage;
- **A1**, per region (§7a.4's plan): each enclosure's output as one source at its opening, with its
  own images;
- **B**, per pipe (§3).

Impulse responses per pipe (mouth + top, 150 ms) at both microphones. B is physical (1/r re 1 m);
A0 and A1 are scaled to B's mean early energy. "Mouth only" leaves out the open pipes' tops.
Shared late field: a 16-line FDN fitted to organ-9.wav (prototype accuracy: 63 Hz 7.7 s, 125 8.9,
250 7.1, 500 6.0, 1 k 5.8, 2 k 4.9, 4 k 3.3, 8 k 1.9 s against 6.3 / 9.5 / 6.1 / 5.6 / 6.2 / 4.1 /
3.6 / 1.6), level by diffuse-field theory (16π/A, the pattern's diffuse power).

### 4.1 Each pipe at the two microphones

| at 15 m (27 m) | A0 today | A1 per region | B per pipe |
|---|---|---|---|
| IACC, 0-80 ms, ±1 ms, mean [range] | 1.00 [0.99-1.00] | 0.30 [0.05-0.59] (0.48) | 0.38 [0.11-0.83] (0.51) |
| first arrival R-L time, range | ±0.02 ms | -1.35..+0.10 ms | **-1.65..+1.04 ms** (-1.08..+1.00) |
| first arrival R-L level (2-8 kHz), range (sd) | -6.4..+6.4 dB (4.1) | -6.1..+3.5 (3.7) | -9.4..+15.1 (3.8) |
| discrete arrivals > -20 dB in 80 ms | 3.3 | 16.3 | 15.3 |
| normalised echo density, 0-80 ms | 0.42 | 0.27 | 0.30 |
| dry render's L/R correlation, 0.3-8 kHz (chord / scale) | 0.93 / 0.93 | -0.16 / -0.12 | **0.11 / 0.04** |

- A0's channels are copies of each other, with a level difference set by the enclosure alone.
- B's IACC is higher than A1's: a pipe inside its case sees fewer images (its own case blocks the
  steep and backward paths) than an enclosure's opening does, so its direct sound weighs more. Both
  lie in a good hall's range (IACC_E 0.3-0.6).
- Echo density: both A1 and B are sparse in 0-80 ms (NED 0.3: discrete images, as wanted). The
  density comes from the late field, which fills in from ~50 ms. A0's 0.42 is the enclosure's short
  reverberation, not the church.

### 4.2 How distinct two pipes are (early responses, 0-80 ms)

Max normalised cross-correlation over ±2 ms (1 = the same response), mean over keys 56-91.
Mouth only:

| pair, at 15 m (27 m) | A0 today | A1 per region | B per pipe |
|---|---|---|---|
| whole tone, same side (8 cm apart) | 1.00 | 1.00 | **0.58** (0.62) |
| same key, front / back row (Principal 8′ / Octave 4′) | 0.99 | 0.99 | **0.46** (0.52) |
| semitone (C side / C# side) | 1.00 | 0.33 | 0.32 (0.51) |
| same key, Hauptwerk / swell | 0.01 | 0.09 | 0.11 |
| whole tone in a swell box | 1.00 | 1.00 | 0.93 (0.78) |

With the open pipes' tops (a second source λ/2 above the mouth) the whole tone falls to 0.40 in B
(A0 0.63, A1 0.81) and the front / back row to 0.30 (A0 0.54, A1 0.56). The region scheme A1 gains
nothing within an enclosure. That gain is what the per-pipe paths add. Inside a swell box the pipes
stay alike (0.93), because the box, not the pipe, radiates, as physics says. With the late field
added into the same 80 ms window every pair rises to 0.72-0.89 (1.00 in a swell box). At 15 m the late field's energy is
already larger than the early part's by 50 ms.

### 4.3 The balance of early to late [meas]

Steady state at 15 m, early (direct + images) against the late field: a front-row Principal 8′
-11.7 dB (theory for an unobstructed direct path at 16 m: -11.3 dB), Octave 4′ (row 1) -12.9,
Superoctave 2′ (row 3, shadowed) -16.3, the swell's Gedackt -25.4. The held chord as a whole: -18.8
dB. The per-pipe paths therefore matter most in the **onsets**: precedence, where each note seems
to come from, and the first 50 ms before the late field builds. They also set the colour of each
rank through its place (rows behind are darker and weaker in the direct sound) and the stereo image.
In a sustained chord at 15-27 m the church's late field dominates, as in the recordings.

### 4.4 Cost [meas: one pinned thread, the minimum of 6 runs, a heavily loaded machine (load ~16-22)]

| per pipe (305 pipes, 549 sources with tops) | paths per pipe (both mics) | ns per pipe-sample | against a pipe (42 ns here) |
|---|---|---|---|
| **B: per pipe to 2nd order**, 3 shadow rings | 43.7 | **7.4** | 18 % |
| per pipe to 1st order, 2nd order per region | 17.7 | 3.4 | 8 % |
| per pipe direct only, images per region | 3.5 | 1.3 | 3 % |
| (B with the shadow split into 2 taps instead) | 79.9 | 13-14 | 32 % |
| (B mouths only, no tops) | 44.8 / 305 sources | ~15 / 2 = 7.5 per source | |

- A pipe here is an open principal with 16 modes plus `top()`: 42 ns in the same loaded run (the
  project's ~30 ns is an average with stopped pipes on a quieter machine).
- **Full organ:** 300 sounding pipes x 7.4 ns = **2.2 µs per sample**, 11 % of the church's core
  (20.8 µs); 500 pipes 3.7 µs. Fixed per sample: the rings' readout (30 rings x 3 sets) and 30
  filter chains of 3-5 sections, ~0.3 µs.
- **Memory:** a ring set is 30 x 4096 x 4 B = 480 KB. The church's set plus the two swell boxes'
  is 1.4 MB (L2/L3-resident). Path tables: ~45 x 12 B per pipe, ~1.4 MB for all 2 600 pipes, built
  once per listener position (the whole build, tracing and fitting the filters: 38 ms for 305 pipes
  [meas], so ~0.3 s for all 2 600 [calc]).
- Against the earlier estimates: §7b.2's 48 MAC per pipe and 7-12 µs for 300-500 pipes, and §7d.4's
  memory-bound 15 ns per source (42 MB of per-source rings). Scattering by blocks into shared class
  rings is ~2x cheaper per source and needs ~30x less memory. Ring length (4096 / 8192) made no
  measurable difference.

### 4.5 The A/B demos (15 m, 48 kHz stereo, 16-bit)

Chord: C major (C3 E3 G3 C4 E4 G4 C5, arpeggiated over 0.7 s, held to 3.4 s), then F major (F2 F3
A3 C4 F4 A4 C5, to 6.0 s), and the decay. Stops: Principal 8′ + Octave 4′ + Quinte 2⅔′ +
Superoctave 2′ + the swell's Gedackt 8′. Scale: chromatic C4-C5-C4 on Principal 8′ + Octave 4′,
0.22 s a note. Each set is loudness-matched (RMS) with one common peak. "_early" = without the late
field (the difference is clearest there); the full mixes include the shared late field at its
physical level. In the scratchpad `roomcoupling/`:
`demo_chord_A0.wav` / `demo_chord_B.wav` (full), `demo_chord_A0_early.wav` /
`demo_chord_B_early.wav`, the same for `demo_scale_*`, and `*_A1*` (per region) for reference.

## 5. Integration plan (a few hundred lines)

### 5.1 New: `src/organroom.h` (~300 lines, from the prototype)

```cpp
class OrganRoom {
 public:
  struct Listener { double z = 15, x = -2, y = 1.2, spacing = 2, angleDeg = 55, pattern = 0.6; };
  void setup(double fs, const Listener& l);                      // the nave, the gallery, the listener
  int addCase(Box box, double apLoY, double apHiY, double backZ, bool shadowOnly);
  int addRingSet();                                              // 0: the church; one per swell box
  // A radiating point: its paths traced now (to 2nd order), its taps kept. Returns its index.
  int addSource(Vec3 at, int caseId, const Plane* back, float occl, float dirDark, int set);
  void finish();                                                 // D0, class splits, the class filters
  void scatter(int src, const float* blk);                       // one block (kBlock samples, gain applied)
  void setShutter(int set, float a, float gain, float leak);     // a swell box's opening, per block
  void endBlock(float* l, float* r);                             // read, filter, sum; w += kBlock
  void reset();
};
```

It contains the prototype's `paths()` tracer, `Tap`/`RingSet`/`scatter()`, `airDbPerM()` (ISO
9613-1), the first-order sections and `fitTop()`.

### 5.2 `src/organbox.h`

- `Enclosure::Params`: add `double directOut = 1;` (0: the pipes' direct sound leaves by their own
  paths).
- `Enclosure::add()`: `direct_[(dw_ + s.delay) & dmask_] += float(p_.directOut) * s.gain * y;`
  (one line; the lines' input unchanged).
- Add the accessors `float shutterA() const { return a_; }`, `float shutterGain() const { return
  gainOpen_; }`, `double leak() const { return p_.leak; }`, so OrganRoom's swell ring sets use the
  box's own shutter state.
- `step()`, `addSource()`, `heard()`: unchanged.

### 5.3 `src/organ.cc` / `src/organ.h`

- **Data per `Pipe`:** `int rsrc = -1, rtop = -1;` (its sources in OrganRoom) and
  `float blk[2][kBlock], blkTop[2][kBlock];` (its last two blocks, ~260 B). Its church position is
  computed when it is placed (`kPlace[enc].centre + at`); its row (shadow) comes from `st.row`.
- **`OrganModel::OrganModel`:** a table `kPlace[kEnclosures]` (each enclosure's centre in the church
  and its case: §3.1). `room_.setup(fs, listener)` (knobs `PJ_ORGAN_LISTENER` = 15 m,
  `PJ_ORGAN_PATHS` = 1), `addCase` for the main case, swell boxes, Positiv (shadowOnly) and towers;
  `addRingSet` per swell box; each enclosure's `directOut = 0` when the paths are on; after
  `addRank`s and `calibrate()`, one region source per enclosure at its opening; `room_.finish()`.
  (Calibration then hears the same enclosure field as before: the pitch is unaffected.)
- **`OrganModel::addRank`:** after `pp.at = at;`: `pp.rsrc = room_.addSource(world(at), caseOf(enc),
  backOf(enc), 0.12f * st.row (0 in the façade), 0.8f, setOf(enc))`. For an open flue also
  `pp.rtop` at its top (omni, half the occlusion). A reed's source is its resonator's top (`at`
  already), omni.
- **`OrganModel::renderEnclosure(e)`:** besides `enc.add(...)`, store `pp.blk[par][s] = pp.gain *
  pp.y` and the top's sample. A pipe that falls asleep mid-block writes zeros for the rest.
- **`OrganModel::renderBlock()`:** `pool_->run(n + 1, ...)`. Job n is the room: for every pipe awake
  in the **previous** block, `room_.scatter(pp.rsrc, pp.blk[par ^ 1])` (and the top); each
  enclosure's previous output as its region source; `room_.setShutter` from the swell boxes;
  `room_.endBlock(roomL_, roomR_)`. One block (0.33 ms) of latency; at ~35 µs per block for 300
  pipes the job balances against a Hauptwerk half's ~50 µs and adds no wall time. The per-region
  church input `church_[e][s]` (§3.5) replaces the single `church_[s]`.
- **`OrganModel::sample()`:** with the paths on, `l = roomL_[bp]; r = roomR_[bp];` replaces the Mic
  loop (kept for `PJ_ORGAN_PATHS=0`, the A/B). Mid/side and `takeSide()` as now.
- **`OrganModel::resetAir()`:** `room_.reset()`.
- **`setSwell`:** nothing new (the room reads each box's shutter per block).

### 5.4 Outside the organ (the room's owner)

`src/room.*` / the Ensemble: a source flag "early supplied by the instrument" (skip its `early_`
taps, keep its FDN input, with §7d.4's onset ramp), and the room's pressure per region of the organ
(§3.5). Not needed for the organ to sound right on its own.

### 5.5 Checks for tools/test.py

- The neighbours' early cross-correlation < 0.7, the front / back rows' < 0.6 (this prototype: 0.58,
  0.46); a dry render's L/R correlation < 0.3 (0.05-0.11); the first arrival's channel time
  difference spans > 1.5 ms across the case (2.7 ms).
- The paths' cost < 25 % of the pipes' in a full-organ render.
- The swell closed: its pipes' direct paths ~12 dB down and dark (the shutters on their rings).

## 6. Open

- The geometry is placeholders: the case's depth, the impost's height above the chest (it decides
  which back-row pipes the nave sees directly), whether the case has a roof, the Rückpositiv's size.
  Also the listener: 15 m in the nave, 27 m, or the console (on the gallery, where the case's
  inside and the near field dominate).
- The microphones: a 2 m AB pair (as src/room.cc) gives real time differences (up to 1.6 ms). Organ
  recordings often use a spaced pair high in the nave. A binaural listener (each path's direction
  is known) is a later option (§7e).
- The pedal towers and the Positiv were laid out but not measured with pipes. The pipes' near-field
  coupling (Mitnahme) stays in the enclosures unchanged.
- The late field fades in on its own (the FDN's first outputs at ~50 ms). §7d.4's explicit ramp
  (35 % at 20-30 ms, all from 60-70 ms) is for the room's owner.
