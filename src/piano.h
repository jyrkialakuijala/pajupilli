// A concert grand (research/piano.md): 243 strings on 88 keys, each string a
// bank of its bending shapes (modes) in two directions, the hammers, the
// dampers, and the soundboard's resonances (tools/pianoboard.py), all meeting
// at the bridge every sample.
//
//   - A string: its modes n = 1, 2, ... up to 10 kHz (the bass) .. 20 kHz
//     (the treble), each at n f0 sqrt(1 + B n^2) (the wire's stiffness: B
//     from its diameter, tension and length), swinging up-and-down (the
//     hammer's direction) and sideways. Its end on the bridge moves with the
//     soundboard and pushes it: the string loses its sound to the board, and
//     hears every other string through it. Sideways it holds the bridge
//     loosely (a fifth as much): that swing dies slowly, the long aftersound.
//   - A key's two or three strings are tuned a fraction of a cent apart:
//     they beat, and pass their energy between each other through the bridge.
//   - A hammer: a mass thrown at the strings, its felt a spring that hardens
//     as it is pressed (force = K x^p, more while being pressed than while
//     springing back). It meets each string at one place, about an eighth
//     along it; the strings' answer shapes the blow. A harder blow presses
//     the felt deeper, where it is stiffer: shorter, brighter.
//   - A damper: felt on the strings beside the hammer; every mode loses by
//     how much it moves there. The top 18 keys have none.
//   - The soundboard: 256 resonances of a ribbed spruce plate and 96 broad
//     ones above them, each with its share at every key's bridge point.
//   - The sound: the board in patches, each moving the air over it; a
//     crossed pair of figure-of-eight microphones over the strings (one
//     turned to the bass side, one to the treble side) hears every patch
//     after its own distance and by its angle.
#pragma once

#include <cstdint>
#include <vector>

namespace pajupilli {

// One key's strings, hammer and damper, as built.
struct PianoKey {
  int strings = 3;                 // 1, 2 or 3
  double len = 0, tension = 0, mu = 0, B = 0;  // per string: m, N, kg/m, stiffness (partial n at n f0 sqrt(1 + B n^2))
  double f1 = 0;                   // the first partial as tuned (Hz)
  double strike = 0.122;           // the hammer's place, a share of the length
  // The hammer: mass (kg); the felt F = K x^p as it is at a gentle blow, and
  // from the force `pivot` (N a string) up an exponent higher by `colour`.
  double mh = 0, K = 0, p = 0, pivot = 2, colour = 0;
  // The felt's nap: a loose outer layer `nap` thick (m; 0: none) lying on
  // the felt proper, a spring of napK (N/m a string) that stiffens without
  // limit as it is pressed flat. The softest blows never get through it: a
  // long, gentle push, the fundamental and little else; a little harder
  // and it is flat within the blow, and the felt under it shapes the rest.
  double nap = 0, napK = 0;
  double loss = 0, b2 = 0, b3 = 0; // the wire's own loss: loss + b2 f + b3 f^2 (1/s)
  bool hasDamper = true;
  double damper = 0, damperLen = 0, damperRate = 0;  // its place (share), felt length (m), rate (1/s)
};

// A piano as data: the grand (grandDesign()), and what is built on the same
// strings and hammers (research/piano_design.md): another scale, another or
// no soundboard, microphones or a pickup.
struct PianoDesign {
  static constexpr int kMaxKeys = 88;
  const char* name = "grand-piano";
  int keys = 88, lowMidi = 21;
  PianoKey key[kMaxKeys];
  // The unisons: a key's strings this far apart (cents: at the lowest key,
  // at the highest; each x 0.7 .. 1.3). An out-of-tune piano: the outer
  // strings a further +- honkyCents, and a share of all strings flat by up
  // to flatMax cents.
  double unisonCents[2] = {0.08, 0.03};
  double honkyCents = 0, flatShare = 0, flatMax = 0;
  uint32_t seed = 4242;
  // The sideways swing: how far off the vertical the blow sets a string
  // swinging, how far sideways is tuned from up-and-down (cents), and the
  // bridge's share of it.
  double tilt = 0.4, sideTuneCents = 1.0, sideLoad = 0.4;
  double top = 10000, topTreble = 20000;  // modes up to here (Hz): the bass, from about key 40 up
  double hammerWidth = 0.010;             // the felt's width on the string (m)
  // The bridge: a soundboard's resonances (tools/pianoboard.py), loaded by
  // the damped strings standing on it; or a heavy lossy support that does
  // not move (each string's end gives to a resistance of supportR kg/s: a
  // decay of tension / (length x supportR) for every mode).
  enum class Bridge { kBoard, kRigid } bridge = Bridge::kBoard;
  struct Board {
    int n = 0, patches = 0;
    const float* freq = nullptr;      // [n] Hz
    const float* loss = nullptr;      // [n] loss factors
    const float* phi = nullptr;       // [keys][n]
    const float* patchVol = nullptr;  // [patches][n]
    const float* patchPos = nullptr;  // [patches][2] m
  } board;
  double boardLoss = 1.0, stringLoad = 1.0;  // (x the board's own loss; x the damped strings' load)
  double supportR = 2e4;
  double supportFc = 0;  // (a sprung support: it gives by 1 / (1 + (supportFc / f)^2) of that; 0: at every pitch alike)
  // The sound: two microphones over the board (place in m: x from the belly
  // rail, y from the spine, z over the board; its axis turned angleDeg from
  // straight down towards +y; pattern 1 figure-of-eight .. 0.5 cardioid ..
  // 0 all round); or a pickup under each key's strings: their up-and-down
  // force on the bridge (N; the sideways swing's by pickupSide), each key
  // times its gain, summed (side() is then 0).
  enum class Output { kMics, kPickup } output = Output::kMics;
  struct Mic { double x, y, z, angleDeg, pattern; };
  Mic mic[2] = {{0.5, 0.615, 0.65, -60, 1}, {0.5, 0.785, 0.65, 60, 1}};
  float pickupGain[kMaxKeys];
  double pickupSide = 0.05;

  PianoDesign() { for (float& g : pickupGain) g = 1; }
};

// The concert grand (research/piano.md), with the PJ_PIANO_* knobs.
PianoDesign grandDesign();
// A tuner's work on a design's keys (their B set): equal temperament on keys
// [lo, hi] (numbered from 1, A4 = key 49 = 440 Hz), then octave by octave, a
// partial of the lower note against one of the upper: 2 with 1 going up,
// 6 with 3 going down.
void tuneByPartials(PianoDesign* d, int lo = 43, int hi = 54);

class Piano {
 public:
  static constexpr int kKeys = PianoDesign::kMaxKeys, kMaxStr = 3;
  explicit Piano(double fs, const PianoDesign& design = grandDesign());
  ~Piano();
  Piano(const Piano&) = delete;
  Piano& operator=(const Piano&) = delete;

  int keys() const { return nKeys_; }
  // Key 0 .. keys() - 1 (the grand: A0 .. C8). The hammer leaves the action at `speed` (m/s).
  void strike(int key, double speed);
  // The key's damper: on the strings, or lifted.
  void damper(int key, bool down);
  // The felt's hardness (1: as voiced; the stiffness K times this).
  void setVoicing(double hardness) { voicing_ = float(hardness); }
  // One sample: the two microphones' mean (Pa), side(): half their difference;
  // or the pickup (N).
  float sample(float testForce = 0);
  float side() const { return side_; }
  void reset();
  bool asleep() const { return asleep_; }

  // For tests and tools.
  double frequency(int key) const { return key_[key].f1; }
  double inharmonicity(int key) const { return key_[key].B; }
  double length(int key) const { return key_[key].len; }
  int strings(int key) const { return key_[key].ns; }
  int modes(int key) const { return str_[key_[key].s0].n; }
  int totalModes() const { return total_; }
  double hammerForce(int key) const { return key_[key].force; }  // the last sample's (N)
  double hammerVelocity(int key) const { return key_[key].vh; }  // (m/s, towards the strings)
  int activeStrings() const;

 private:
  struct Str {
    int key, off, n;      // its modes: [off, off + n) of the arrays
    float tanH;           // the hammer meets it a little off the vertical
    float comply;         // its displacement under the hammer per unit force, one sample on (m/N)
    float dPrev = 0;      // the felt's compression at the last sample (m)
    bool active = false;
  };
  struct Key {
    int s0, ns;
    double f1, B, len;
    float mh, K, p;       // the hammer: mass (kg), felt stiffness (N/m^p), exponent
    float nap = 0, napK = 0;  // the felt's nap: thickness (m), stiffness (N/m a string)
    float eta = 0, vh = 0, force = 0;  // its place (m, from the strings' rest) and speed
    bool hammerOn = false;
    bool hasDamper = true, damperDown = true;
    float engage = 1;     // the damper's felt: 0 lifted .. 1 resting on the strings
    float acc = 0;        // how far the bridge moved at its strings in the last sample (m)
    bool active = false;
  };
  void build();
  void hammer(Key& k);
  void wake();
  void boardNow();
  void census();

  PianoDesign d_;
  int nKeys_ = 0, nB_ = 0, nP_ = 0;   // keys, the board's resonances, its patches
  bool pickup_ = false;
  float kSide_ = 0, pickSide_ = 0;
  double fs_;
  float dt_;
  float voicing_ = 1;
  std::vector<Str> str_;
  Key key_[kKeys];
  int total_ = 0;
  // Per mode, all strings in a row (64-byte aligned): the state, in the two
  // directions (x = displacement * omega, v = velocity), and what is fixed.
  float* mem_ = nullptr;
  float *x_[2], *v_[2];
  float *cs_, *sn_;       // the turn per sample, the decay folded in
  float *cs1_, *sn1_;     // the same for the sideways swing, tuned a little apart
  // The string's end rides on the bridge: the bridge moving shifts every
  // mode (in_), and what a mode's velocity gains or loses in its swing the
  // bridge loses or gains (out_): the two halves of one exchange, each exact,
  // so the strings and the board together neither gain nor lose by it.
  float *in_;             // the bridge's displacement into the mode's x
  float *out_;            // the mode's share of the momentum passed to the bridge (kg)
  float *damp_;           // the damper resting on it: what is left per sample
  float *hk_;             // the hammer's force into the mode's velocity (x dt / mass)
  float *hc_, *hs_;       // the string's place under the hammer one sample on, from x and v
  // The soundboard.
  float *bx_, *bv_, *bcs_, *bsn_, *bf_, *ba_, *binv_;
  std::vector<float> loadR_, loadX_;  // [key][resonance]: a damped key's strings as a load on it (1/s)
  bool boardStale_ = false;           // a damper moved: the board's resonances are to be set again
  float *phi_;            // [key][mode]
  // The air: how much each patch of the board moves it per resonance, the
  // patches' last 10 ms, and each microphone's distance from each patch.
  static constexpr int kRing = 512, kMaxPatches = 64;
  float *bacc_, *pvol_, *ring_;
  int ringPos_ = 0, quiet_ = 0;
  int tapI_[2][kMaxPatches];
  float tapF_[2][kMaxPatches], tapG_[2][kMaxPatches];
  float side_ = 0;
  int activeKeys_[kKeys], nActive_ = 0;
  bool asleep_ = true;
  int clock_ = 0;
  uint32_t rng_ = 4242;
  float pick_ = 0;        // the pickup's signal, this sample
};

}  // namespace pajupilli
