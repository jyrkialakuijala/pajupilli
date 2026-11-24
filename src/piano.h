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
//     how much it moves there. The top 17 keys (G#6 up) have none, as on a 274 cm concert grand.
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
  double levelDb = 0;              // its sound at the bridge, dB: the keys evened (the voicer's work)
  double rear = 0;                 // the string from the bridge to the plate, a share of the speaking length (0: none)
  // The hammer: mass (kg); the felt F = K x^p as it is at a gentle blow, and
  // from the force `pivot` (N a string) up an exponent higher by `colour`.
  double mh = 0, K = 0, p = 0, pivot = 2, colour = 0;
  // The felt's nap: a loose outer layer `nap` thick (m; 0: none) lying on
  // the felt proper, a spring of napK (N/m a string) that stiffens without
  // limit as it is pressed flat. The softest blows never get through it: a
  // long, gentle push, the fundamental and little else; a little harder
  // and it is flat within the blow, and the felt under it shapes the rest.
  double nap = 0, napK = 0;
  // Not a hammer but a plectrum (pluckK > 0): a springy tongue (N/m) on a
  // jack that rises at the key's speed, catches the string from below and
  // bends until its force passes pluckLimit (N), where the string slips off
  // its tip and flies: that let-go is the sound. The string leaves upward
  // and, by the plectrum's slope, sideways (pluckSide of the upward force).
  double pluckK = 0, pluckLimit = 0, pluckSide = 0;
  double top = 0;                  // this key's modes up to here (Hz; 0: the design's rule by key number)
  double loss = 0, b2 = 0, b3 = 0; // the wire's own loss: loss + b2 f + b3 f^2 (1/s)
  bool hasDamper = true;
  double damper = 0, damperLen = 0, damperRate = 0;  // its place (share), felt length (m), rate (1/s)
  double damperMass = 0;  // the head, its wire and lever as they land on the strings (kg; 0: no landing force)
};

// A piano as data: the grand (grandDesign()), and what is built on the same
// strings and hammers (research/piano_design.md): another scale, another or
// no soundboard, microphones or a pickup.
struct PianoDesign {
  static constexpr int kMaxKeys = 256;  // (a harpsichord's four choirs: each string its own key)
  const char* name = "grand-piano";
  int keys = 88, lowMidi = 21;
  PianoKey key[kMaxKeys];
  // The unisons: a key's strings this far apart (cents: at the lowest key,
  // at the highest; each x 0.7 .. 1.3). An out-of-tune piano: the outer
  // strings a further +- honkyCents, and a share of all strings flat by up
  // to flatMax cents.
  double unisonCents[2] = {0.08, 0.03};
  double honkyCents = 0, flatShare = 0, flatMax = 0;
  double honkyHz = 0;  // (or: every string of a key its own ~this far off, in hertz)
  double flatHz = 0;   // (the flat strings ~this many hertz flat instead of flatMax cents)
  // (a tacked hammer or a mandolin rail, each key its own: one or two strings met first by metal,
  // a knock of ~tackHard N per m/s of the hammer lasting tackTime s, the face uneven by up to
  // tackGap m)
  double tackHard = 0, tackGap = 0, tackTime = 1e-4;
  // The strings' stretching (0: none): a loud blow lengthens a string by the square of its
  // swing; its lengthwise modes (the first at sqrt(E A / mu) / 2 L, ~16 x the fundamental in
  // the bass) ring with the sums of its partials, pushing the bridge along the string; that
  // push rocks the bridge on the board, moving it as a force of longShare of it would at 1 kHz
  // (as the square root of the frequency). Computed after a blow until the string's swing has
  // lost 20 dB (at most longTime s), on the strings whose first lengthwise mode is under
  // longTop Hz; each mode's Q longQ.
  double longShare = 0, longTime = 4, longTop = 2500, longQ = 50;
  // The dampers and the pedal's trapwork as forces (research/piano.md 6), x pedalNoise (0: none).
  // A damper falls onto its strings at damperFall m/s (a key let go; the pedal's release gives
  // its own speed), its momentum taken up by the felt in a half sine of damperContact s, pushing
  // the strings towards damperSide (-1: down, from above, the grand's; +1: the upright's, from
  // the hammers' side), each head late by up to damperSpread m of the lift over its speed (the
  // regulation). The lift rail and the pedal's lever (railMass kg) land on their felt stops at
  // the pedal's speed in railContact s; rimShare of a knock in the case reaches the board
  // through the rim. A press: the rail meets the dampers' underlevers, railPress of the
  // release's knock.
  double pedalNoise = 0;
  double damperFall = 0.12, damperContact = 8e-3, damperSide = -1, damperSpread = 0.5e-3;
  // The pedal let go: its dampers' landings and the rail's knock x this (by ear: a little louder
  // than the landings' and the knock's own physics give; PJ_PIANO_PEDALUP).
  double pedalUp = 1.6;
  double railMass = 0.6, railContact = 8e-3, rimShare = 0.5, railPress = 0.3;
  // The key's own knock at a stroke (the key's front landing on its felt punching on the keybed,
  // the hammer's shank and the action's wood): an impulse of keyKnock N s per m/s of the hammer,
  // a half sine of keyKnockTime s, into the board at the key's own bridge point (through the key
  // frame and the case to where its strings stand). Under the strings in the middle of the
  // keyboard; at the top, where the strings' sound is short, it is much of the note: the thunk.
  double keyKnock = 0, keyKnockTime = 3e-3;
  uint32_t seed = 4242;
  // The sideways swing: how far off the vertical the blow sets a string
  // swinging, how far sideways is tuned from up-and-down (cents), and the
  // bridge's share of it.
  double tilt = 0.4, sideTuneCents = 1.0, sideLoad = 0.4;
  double top = 10000, topTreble = 20000;  // modes up to here (Hz): the bass, from about key 40 up
  double hammerWidth = 0.010;             // the felt's width on the string (m)
  // The concert grand's own voicing (designFor sets these; what is built on the grand keeps
  // the plain values): the treble bridge's admittance above ~1 kHz at the top key, the top
  // strings' own extra loss (partials above the first, the first), and how the nap gives out
  // under a blow (0: never quite, a hyperbola; else it packs down within this share of its depth).
  double topBridge = 1, topUp = 2.5, topF0 = 1.5, napPack = 0;
  // The blow's vowel: how deep (dB) the valleys between the formants the hammer speaks through
  // (Piano::voiceBlow; 0: none).
  double vowelDb = 0;
  // The bass's bloom, as a bell's: its upper partials take the blow at once but come out through
  // the bridge each at its own later time, some much later (x how deep; 0: none).
  double bloom = 0;
  // From the mid-bass up (C2 to C3 gliding in) the partials over ~600 Hz, fully from ~1.7 kHz, lose
  // their energy this much faster in the wire (x 1 + it); the lower ones keep their sustain.
  double hiDecay = 0;
  // The strings behind the bridge (PianoKey::rear): their sound on the bridge (x; 0: none), what
  // their own key's strings hand them across it, what the bridge's vibration gives them (x the
  // real coupling), and their Q: slightly resonant.
  double rearGain = 0, rearOwn = 0, rearBridge = 0, rearQ = 60;
  // The long strings' larger swing heard at the bridge: their sound x (length / C4's length)^this
  // (1: as the length, 0.5: its square root; 0: none), their sustain and timbre as they were.
  double bassLength = 0;
  // A hammer landing on a string still ringing: how much of each partial's swing into the hammer
  // it stops, at a place where that partial moves fully (0: none; Piano::restrikeStep).
  double restrike = 0;
  // How the blow's energy goes with the hammer's mass, as a power of it, for the hammers heavier
  // than A4's (1: 1/2 m v^2 alone; 2: a heavier hammer also hands its energy over better).
  double massPower = 1;
  // The pitch's rise at a loud blow: the swing stretches the string, its tension up and every
  // partial sharper until the swing has died (x the computed rise; 0: none).
  double glide = 0;
  // How much of the bridge's motion goes back into the strings (x): under 1 a stiffer, heavier
  // bridge and board, the strings handing their energy over more slowly and singing longer.
  double bridgeIn = 1, bridgeBass = 1;  // (bridgeBass: x more of it at C3 and under, gliding from C5)
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
// The hammered dulcimer (src/dulcimer.cc): its own scale, hammers and unisons.
PianoDesign dulcimerDesign();
// A large harpsichord (src/harpsichord.cc): four choirs, each string a key, plucked.
PianoDesign harpsichordDesign();
// A tuner's work on a design's keys (their B set): equal temperament on keys
// [lo, hi] (numbered from 1, A4 = key 49 = 440 Hz), then octave by octave, a
// partial of the lower note against one of the upper: 2 with 1 going up,
// 6 with 3 going down.
void tuneByPartials(PianoDesign* d, int lo = 43, int hi = 54, bool byRegister = false);
// The concert grand's own speaking lengths, key by key (the 20 wound notes, A0 to E2, keep their
// tension, their mass following; the plain wire from F2 up keeps its gauge, its tension following).
void concertScale(PianoDesign* d);

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
  // A damper just let down laid on its strings over this long, with no landing: the sound fades.
  void softLand(int key, double seconds);
  // The key's damper: on the strings, or lifted. Let down, it lands at `speed` (m/s; < 0: the
  // design's damperFall, each time a little its own).
  // `loud`: x its landing's push (the pedal let go: PianoDesign::pedalUp).
  void damper(int key, bool down, double speed = -1, double loud = 1);
  // The sustain pedal's trapwork: the lift rail rising (down: the pedal pressed) or falling back
  // onto its stop at `speed` (m/s); its knock reaches the board. (The dampers are the player's.)
  void pedal(bool down, double speed);
  bool pedalNoise() const { return noise_ > 0; }
  const PianoDesign& design() const { return d_; }
  // The felt's hardness (1: as voiced; the stiffness K times this).
  void setVoicing(double hardness) { voicing_ = float(hardness); }
  // One sample: the two microphones' mean (Pa), side(): half their difference;
  // or the pickup (N).
  float sample(float testForce = 0);
  float side() const { return side_; }
  // The board in regions (keepRegions(true) first): its patches in kRegions groups by where they
  // lie (four across, bass to treble; two deep, the keyboard's half and the tail's), each group's
  // share of sample()'s sound (the microphones' mean), and its middle on the board (m: x from the
  // belly rail along the strings, y from the spine across). Each key's sound, its hammer's thump
  // with it, comes most from where its bridge drives the board.
  static constexpr int kRegions = 8;
  void keepRegions(bool on);
  float region(int j) const { return region_[j]; }
  // The regions as the hall hears them (no microphones: PianoModel::concertHall).
  void hallSound(bool on) { regionSrc_ = on && keepRegions_; }
  bool hallSound() const { return regionSrc_; }
  // The close microphones and the hall blended (keepRegions(true) first): the close pair's sound
  // times close, the board's own sound into the room (as hallSound) times hall; hallSound wins.
  void micBlend(float close, float hall) { closeG_ = close; hallG_ = keepRegions_ ? hall : 0.0f; }
  void regionPlace(int j, double* x, double* y) const { *x = regionX_[j]; *y = regionY_[j]; }
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
    int live = 0;         // (those under the top: the rest, padding to 16, stay at rest)
    float tanH;           // the hammer meets it a little off the vertical
    float comply;         // its displacement under the hammer per unit force, one sample on (m/N)
    float dPrev = 0;      // the felt's compression at the last sample (m)
    float hard = 1, gap = 0;  // (a tacked hammer: this string's felt stiffness x, its face's distance (m))
    float tack = 0;           // (... its metal's sharp knock: peak force per m/s of the hammer (N s/m))
    float tickAmp = 0;        // (... the knock under way: its peak (N), samples left)
    float dampR = 0;          // (the damper's felt, its friction on this string pressed in (N s/m))
    int tickLeft = 0, tickN = 1;
    bool active = false;
    // Its stretching (research/piano.md 3): nl lengthwise modes, each as the transverse ones
    // (lx = U omega, lv), its turn per sample (decay folded in), the drive by the transverse
    // modes' products (x dt), its force on the bridge per lx and per product (what the mode
    // would hold at rest, taken off: x dt), and the even tension's per squared slope (x dt);
    // samples left to compute it.
    static constexpr int kLong = 8;
    int nl = 0, nlx = 0, longLeft = 0;  // (nlx: its transverse modes under 5 kHz, those stretching it)
    float lx[kLong] = {}, lv[kLong] = {}, lcs[kLong] = {}, lsn[kLong] = {}, ldrive[kLong] = {}, lout[kLong] = {};
    float lrest[kLong] = {}, ltension = 0;
    float glideK = 0, glide = 0, glideSet = 0;  // (the pitch's rise: per the swing's energy; now; as last applied)
    float longPeak = 0;       // (its swing's energy at the loudest since the blow)
  };
  struct Key {
    int s0, ns;
    double f1, B, len;
    float mh, K, p;       // the hammer: mass (kg), felt stiffness (N/m^p), exponent
    int glideN = 0;       // (samples to the next look at its strings' tension)
    float softStep = 0, softT = 0;  // (a damper let down very slowly, Piano::softLand: its pace per sample, how far)
    int restrikeT = -1;   // (samples into a restrike's stopping of the swing; -1: none)
    int bloomT = -1;      // (samples since the blow while its partials are still coming out; -1: not)
    // (the strings behind the bridge: per string, kRear partials: state, turn, coupling)
    static constexpr int kRear = 3;
    int rearN = 0;
    float rx[kMaxStr * kRear] = {}, rv[kMaxStr * kRear] = {}, rc[kMaxStr * kRear] = {}, rs[kMaxStr * kRear] = {};
    float rin[kMaxStr * kRear] = {}, rout[kMaxStr * kRear] = {}, rdrive[kMaxStr * kRear] = {};
    float lenGain = 1;    // (the long string's sound at the bridge, x; what comes back from the bridge / it)
    float inBack = 1;     // (under its damper the bridge's motion reaches the strings in full: x this)
    float nap = 0, napK = 0, napPack = 0;  // the felt's nap: thickness (m), stiffness (N/m a string), packing
    float pluckK = 0, pluckLimit = 0, pluckSide = 0;  // a plectrum in the hammer's place
    float eta = 0, vh = 0, force = 0;  // its place (m, from the strings' rest) and speed
    bool hammerOn = false;
    bool hasDamper = true, damperDown = true;
    float engage = 1;     // the damper's felt: 0 lifted .. 1 resting on the strings
    float acc = 0;        // how far the bridge moved at its strings in the last sample (m)
    bool active = false;
    // (its damper landing: the felt's push on each string (N, the peak), the samples until it
    // touches, those left of the push and its length)
    float landAmp = 0, landDelay = 0;
    int landWait = 0, landLeft = 0, landN = 1;
    // (the damper as a contact (PJ_PIANO_DCONTACT): its head's place (m, up from the strings'
    // rest: the felt's face) and speed; its mass and its weight on the strings (N))
    float hy = 0, hv = 0, dM = 0.016f, dW = 0.16f;
    bool seated = true;  // (the strings' rest is where its weight holds them: their motion counted from there)
  };
  void damperContact(Key& k);
  void build();
  void hammer(Key& k);
  void bloomStep(Key& k);
  float rearStep(Key& k, float own);
  void restrikeStep(Key& k);
  void glideStep(Key& k);
  void pluck(Key& k);
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
  std::vector<float> wdt_, cs0_, sn0_, cs10_, sn10_;  // (each mode's turn per sample at rest)
  std::vector<float> restrikeLn_;  // (each mode: the log of what a restrike leaves of its swing)
  std::vector<float> outFull_, bloomTau_, bloomFloor_;  // (each mode: its output in full, how long it takes (samples), from what)
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
  float *vw_;            // what of the hammer's push reaches each mode: this blow's vowel
  float *hc_, *hs_;       // the string's place under the hammer one sample on, from x and v
  float *dk_;             // a force on the damper's felt into the mode's velocity (x dt / mass)
  // The damper's felt as kDPts contacts along its length (each a quarter of it): per contact,
  // the mode's shape there (the mean over its quarter), / omega (the place from x), and x dt / mass.
  static constexpr int kDPts = 4;
  float *dsh_[kDPts], *dshw_[kDPts], *dkp_[kDPts];
  bool dContact_ = false;
  float dFeltK_ = 300;    // the damper felt's stiffness (N/m a string: thick, soft wool, the strings sunk ~1 mm into it)
  float dHold_ = 0;       // the drag on its head's motion (N s/m: the guide rail's bushings, the underlever)
  float *nw_;             // n / omega: the mode's x to its slope's share (n a_n, m)
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
  // The array microphone over the board (PJ_PIANO_ARRAY=1, -piano_mics array) in place of the pair:
  // each patch's paths to its mics, [patch][mic], each mic's share of left and right in its gains.
  int arrN_ = 0;
  std::vector<int> arrI_;
  std::vector<float> arrF_, arrGL_, arrGR_;
  float side_ = 0;
  bool keepRegions_ = false;
  // (the regions as sources (PJ_PIANO_PARTSRC, 1): each patch's own sound, one common delay and
  // gain (the microphones' distance straight down), the room doing the rest; 0: the two close
  // microphones' mean, as heard there)
  bool regionSrc_ = false;
  float closeG_ = 1, hallG_ = 0;  // (micBlend)
  int srcD_ = 0;
  float srcG_ = 0;
  int srcI_[kMaxPatches] = {};      // (each patch's delay toward the audience, within its region)
  float srcF_[kMaxPatches] = {};
  float srcW_[kMaxPatches] = {};      // (its gain: the keyboard's balance kept, band by band)
  int regionOf_[kMaxPatches] = {};
  float region_[kRegions] = {};
  double regionX_[kRegions] = {}, regionY_[kRegions] = {};
  int activeKeys_[kKeys], nActive_ = 0;
  bool asleep_ = true;
  bool allStrings_ = false;  // (awake: every string computed, wake())
  int clock_ = 0;
  uint32_t rng_ = 4242;
  float pick_ = 0;        // the pickup's signal, this sample
  // The dampers' and the trapwork's noise (pedalNoise): the landings under way, the rail's
  // knocks in the case (a release and a press may overlap), where they enter the board, and
  // the draws for each landing's own speed.
  float noise_ = 0;
  int landing_ = 0;
  struct Knock { float amp = 0; int left = 0, n = 1, at = -1; };  // (a half sine: peak (N), samples; at: a key's bridge point, -1 the rim)
  static constexpr int kKnocks = 8;
  Knock knock_[kKnocks];
  int knocks_ = 0;           // (those under way)
  std::vector<float> trapPhi_;
  void knock(double impulse, double T, int at = -1);
  uint32_t noiseRng_ = 1;
  void land(Key& k);
  float stretch(Str& s);
};

}  // namespace pajupilli
