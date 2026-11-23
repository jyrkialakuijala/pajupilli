# PianoDesign: one piano class, several instruments (proposal)

Status: 2026-11-15, a proposal only; nothing in `src/piano.*` is changed. Written for the session
that owns the grand (`src/piano.*`), at its request, to plan the refactor after the grand's tone is
settled. Targets: the concert grand (the default, bit-identical), the electric grand
(`research/electricgrand.md`), the upright and the honky-tonk, the bright piano (`research/piano_variants.md`).

The idea: everything `Piano::build()` takes from `scaleOf()`, `pianoboard::*` and the `PJ_PIANO_*`
knobs today becomes data in one struct, filled by a function per instrument. The knobs stay, as
overrides applied by `grandDesign()` (so today's experiments keep working). The per-sample code
does not change, except for two branches chosen once at build: the bridge (board or rigid) and the
output (microphones or pickup).

## 1. The struct

```cpp
// One string's (and its key's) build data: today's Scale, made public.
struct PianoKey {
  int strings = 3;                  // 1, 2, 3 per key
  double len, tension, mu, B;       // per string: m, N, kg/m, inharmonicity
  double f1 = 0;                    // the first partial as tuned (Hz); 0: from the tuning law
  double strike;                    // the hammer's place, a share of the length
  // The hammer: mass, felt stiffness and exponent as fitted at a gentle blow,
  // and the hardening above the pivot force (today: kColour, kPivot).
  double mh, K, p;
  double pivot = 2.0, colour = 2.0; // N per string; added exponent above it (0: none)
  // Losses: the wire's own, sigma(f) = loss + b2 f + b3 f^2 (1/s).
  double loss, b2 = 3.5e-4, b3 = 3e-8;
  // The damper: place (share), felt length (m), its rate (1/s, today
  // lerp(16, 38, k / 69) / 0.2), or none (the top keys).
  bool hasDamper = true;
  double damper, damperLen, damperRate;
};

struct PianoDesign {
  const char* name = "grand-piano";
  int keys = 88, lowMidi = 21;               // smaller electric grand: 73 keys from E1
  std::vector<PianoKey> key;                 // [keys]

  // Tuning.
  enum class Tuning { kGrandStretch, kEqual, kByF1 } tuning = Tuning::kGrandStretch;
  // (kGrandStretch: today's octave-by-partials law; kByF1: the f1 given per key.)
  double unisonCents[2] = {0.08, 0.03};      // spread bass .. treble (x 0.7..1.3 random), today's
  // Honky-tonk: on top of the spread, a per-key detune of the outer strings
  // (cents, +- this, random sign), and a share of strings left flat by up to
  // `flatMax` cents (a neglected piano).
  double honkyCents = 0, flatShare = 0, flatMax = 0;
  uint32_t seed = 4242;                      // the random tilts, spreads, side tunings

  // The sideways swing (today PJ_PIANO_TILT, _SIDETUNE, _SIDE).
  double tilt = 0.4, sideTuneCents = 1.0, sideLoad = 0.4;

  // Modes per string: up to `top` Hz in the bass, `topTreble` from ~key 40 up.
  double top = 10000, topTreble = 20000;

  // The bridge.
  enum class Bridge { kBoard, kRigid } bridge = Bridge::kBoard;
  // kBoard: the soundboard's resonances (tools/pianoboard.py output): their
  // frequencies, losses (pianoboard::kLoss), the shapes at each key's bridge
  // point, and the radiating patches. The damped strings' load on the board
  // (today loadR_/loadX_, from the damper's loss) is computed from `key`
  // as now, scaled by `stringLoad` (PJ_PIANO_STRINGLOAD).
  struct Board {
    int n = 0, patches = 0;
    const float *freq, *loss, *phi /*[keys][n]*/, *patchVol /*[n][patches]*/, *patchPos /*[patches][2]*/;
  } board;
  double stringLoad = 1.0;
  // kRigid (electric grand stage 1): no board. Each string's end on a heavy, lossy
  // support: a resistance R (kg/s) folded into every mode's decay, as the
  // energy a mode loses through its end per period (sigma_n = R * 2 / (mu L)
  // to first order; fit R to recordings, start 2e4). Optional later: a 1-D
  // bridge beam on sprung mounts (32..64 modes) in place of the board's
  // resonances, so the two strings of a key and the sympathetic strings
  // still meet (stage 2: the same code path as kBoard with beam data).
  double supportR = 2e4;

  // The output.
  enum class Output { kMics, kPickup } output = Output::kMics;
  struct Mic { double x, y, z, angleDeg, pattern; };   // today: two crossed figure-of-eights
  Mic mic[2] = {{0.75, 0.70 - 0.085, 0.65, -45, 1}, {0.75, 0.70 + 0.085, 0.65, 45, 1}};
  // kPickup (electric grand): the vertical force of each key's strings on the bridge
  // (the in_/out_ exchange's force, summed over the key's strings, y only;
  // the sideways swing reaches it by `pickupSide`, ~0.05), times a per-key
  // gain (the 88 piezos in parallel on one bus: equal, except the top six
  // keys about -3.5 dB through the 0.022 uF capacitor), summed: one mono
  // signal (side() = 0). The circuit after it is the instrument's own
  // (src/electricgrand*.cc: tone stack, brilliance, opto tremolo: research/electricgrand.md 3.3).
  std::vector<float> pickupGain;             // [keys]
  double pickupSide = 0.05;
};

PianoDesign grandDesign();     // today's grand: scaleOf(), pianoboard::*, the knobs (bit-identical)
PianoDesign uprightDesign();   // later: U1/U3 scale, an upright board (tools/pianoboard.py run on it)
PianoDesign honkyDesign();     // uprightDesign() + honkyCents 4..8, flatShare, harder worn hammers
PianoDesign brightDesign();    // grandDesign() + felt x2.5 from key 30, mics 0.25 m over the hammers
PianoDesign electricGrandDesign();      // 88 keys, rigid bridge, pickup (below)

class Piano {
 public:
  explicit Piano(double fs, const PianoDesign& d = grandDesign());
  ...
};
```

## 2. What each field replaces in today's `build()`

| Today | Field |
|---|---|
| `scaleOf()`: strings per key, len, tension, mu, B, strike, mh, K, p, loss, damper, damperLen | `key[k]` (scaleOf becomes the body of `grandDesign()`) |
| the felt's hardening above `PJ_PIANO_PIVOT` with `PJ_PIANO_COLOUR` | `key[k].pivot`, `.colour` (applied in build, not in scaleOf) |
| `PJ_PIANO_LOSS2/3` | `key[k].b2`, `.b3` |
| `rate0 = dampRate * lerp(16, 38, k / 69) / 0.2`, `hasDamper = k < 70` | `key[k].damperRate`, `.hasDamper` |
| the tuner's octave law at the end of `scaleOf()` | `tuning` (`kGrandStretch`), or `f1` per key |
| `PJ_PIANO_MISTUNE`, the spread `lerp(0.08, 0.03)` | `unisonCents`, plus `honkyCents`, `flatShare`, `flatMax` |
| `PJ_PIANO_TILT`, `_SIDETUNE`, `_SIDE` | `tilt`, `sideTuneCents`, `sideLoad` |
| `PJ_PIANO_TOP`, `_TOPTREBLE` | `top`, `topTreble` |
| `pianoboard::kFreq`, `kLoss`, `kPhi`, `kPatchVol`, `kPatchPos` | `board` |
| the damped strings' load (`loadR_`, `loadX_`), `PJ_PIANO_STRINGLOAD` | computed from `key` as now; `stringLoad` |
| `PJ_PIANO_MIC*` | `mic[2]` |
| `kKeys = 88`, A0 | `keys`, `lowMidi` (arrays sized at build; `key_` a vector) |
| the rng seed 4242 | `seed` |

`kKeys` as a compile-time constant is used for `key_[kKeys]`, `activeKeys_[kKeys]` and the `phi_`
layout: these become sized by `d.keys` at build (a vector, or `kMaxKeys = 88` with `keys <= 88`,
which is simpler and keeps the arrays fixed).

## 3. The electric grand design, as data (from research/electricgrand.md)

- `keys = 88`, `lowMidi = 21`; `strings = 1` for MIDI 21–42 (22 wound), 2 from G2 (MIDI 43) up.
- Lengths: smaller electric grand bass string 0.679 m (the only published one); the treble as the grand's plain
  strings (the CP's top is a grand's top); in between a smooth scale from 0.68 m at the bottom to
  meet the grand's length near key 40. B from the lengths and diameters (~1–2e-3 in the bass;
  the open-source electricgrand-model's fit: 2.07e-3 at the bottom). Gauges unknown: scale from the grand's,
  fit B per key from a recording.
- Hammers: urethane plus leather (manual) or felt (a technician): start with the grand's felt law
  ×1.5 stiffer, `colour` as the grand; fit brightness against velocity.
- Unisons: `unisonCents = {2.0, 0.4}` (electricgrand-model fit 0.4–2.0 c), `tuning = kGrandStretch` (fit
  later), `sideTuneCents` as the grand.
- `bridge = kRigid`, `supportR = 2e4` (fit); the sideways swing's share at the pickup `pickupSide
  0.05`.
- Dampers: none on the top ~8 keys (GSi's model), `damperRate` as the grand.
- `output = kPickup`, `pickupGain = 1` for keys 1–82, ≈ 0.67 (−3.5 dB) for 83–88 (the mixing
  capacitor; the element capacitance is unknown: fit).
- Cost: ~4 200 modes per direction to 10 kHz, no board: ~2–4 % of a core a note.

## 4. Steps (for the owner of src/piano.*)

1. `PianoKey` = today's `Scale` + pivot/colour/b2/b3/damperRate/hasDamper; `grandDesign()` =
   `scaleOf()` + today's knobs; `Piano(fs, grandDesign())` must render bit-identically to today
   (a `-note` diff at −300 dB, as the choir check did).
2. `Bridge::kRigid` and `Output::kPickup` (small: two branches at build, one in `sample()`).
3. `electricGrandDesign()` and the CP's circuit in its own file (`src/electricgrand.*`, owned by the electric grand work),
   wired in `pianoinst.cc` as `-inst electric-grand`.
4. Upright/honky-tonk/bright designs as their research settles.
