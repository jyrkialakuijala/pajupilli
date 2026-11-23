// A guitar (research/guitars.md): six strings over frets, a bridge on a
// body, the fingers of the right hand. The first of them: the classical
// guitar, nylon strung (classicalGuitar()).
//
//   - Each string: its lowest 48 modes in each of two directions (towards
//     the top plate and along it). A fretted string ends on its fret: the
//     same string, shorter.
//   - All six end on one bridge, and the bridge stands on the body: its
//     motion moves every string's end (the strings not played ring along),
//     and takes a string's sound into the body. Towards the top the bridge
//     gives readily: that part of a note is loud and soon over. Along the
//     top it gives little: that part rings on quietly. How a string is
//     plucked sets the mix (a rest stroke goes into the top; a free stroke
//     along it): the note's two-stage decay.
//   - The body: the air in the box breathing through the soundhole with the
//     top (near 100 Hz), the top's own first resonance (near 200), the
//     back's, the next air mode, and some twenty more of the plates up to
//     3 kHz, each a mass under the bridge with its own share of the sound.
//   - A pluck: a finger takes the string where it stands (stopping what it
//     still sounded), pulls it aside, and lets it go: first off the flesh
//     (wide and slow: the round part of the tone), then off the nail's edge
//     (narrow and quick: the bright part). Where along the string, how
//     deep, at what angle, how much nail.
//   - Damping: a hand or a lifting finger laid on a string.
//
// The electric guitars (research/electric_guitars.md; electricGuitar(): the three-pickup solid-body for
// GM 28) are the same strings in steel, on a solid body that hardly moves, heard not through a
// body but through magnetic pickups (below), and picked:
//   - Steel strings, 10-46 (4.1): they ring 10-20 s on the low E, 3-6 s on the high one; their
//     modes up to 8 kHz (96 of them on the low E). Each string's two directions are not exactly
//     towards the top and along it: the saddle and the nut set them a little askew, each mode
//     its own way (the axes tilted ~20 degrees), and a little apart in pitch. The pickup hears
//     the swing towards it; a pick throws the string mostly along the top: what the pickup hears
//     is the part of each direction that lies towards it, beating slowly against each other: the
//     note blooms and breathes.
//   - The pick: a narrow edge (0.8 mm) pushing the string aside over a millisecond and letting
//     it slip off in 0.15 ms; down strokes push along the top and a little into it, up strokes
//     the other way and a little out. It passes: what the string was sounding goes on.
//   - The neck: two resonances of the neck under the string's end (dead spots: a note on one
//     rings shorter); otherwise the body is still: no sound of its own.
//   - A pickup: a magnet under each string, a coil round it. It hears the string's velocity
//     towards the magnet, at its place along the string (the bridge pickup near the saddle: the
//     first notch of its comb high, at the 13th-16th harmonic; the neck one at the 4th), over a
//     width (the field is ~20 mm wide: the highest harmonics, fretted high, fade), through the
//     magnet's field (stronger as the string comes nearer: on-axis field of a cylindrical
//     magnet, Novak et al.: a hard pluck grows an even harmonic). Two pickups together (the
//     three-pickup solid-body's in-between positions) are two sources in parallel: their mean, through half the coil.
//     What comes out is the coil's EMF (V); the coil itself, and what it rings with, belong to
//     the amp's input (guitaramp.h).
#pragma once

#include <cstdint>
#include <vector>

namespace pajupilli {

struct GuitarString {
  double f0;        // open (Hz)
  double tension;   // N
  double B;         // inharmonicity: f_k = k f0 sqrt(1 + B k^2)
  double loss0, loss1;  // 1/s: loss0 + loss1 k^1.2
};

struct GuitarSpec {
  const char* name = "nylon-guitar";
  double scale = 0.650;        // nut to saddle (m)
  int frets = 19;
  GuitarString str[6];         // low E first
  // The body's resonances under the bridge: Hz, quality, the mass the bridge
  // feels (kg), its share of the sound (+-; 1: the top's first resonance),
  // and of what differs between the two microphones.
  // along: how much of a string's motion along the top this resonance takes
  // (and gives back), against its motion towards the top: the breathing
  // modes hardly any, the ones that rock the bridge much, either way.
  struct Mode { double f, q, mass, rad, side, along; };
  std::vector<Mode> body;
  // The strings' own losses: all (x); the 2nd and 3rd partial's (x again); the 4th and 5th's (x again, less).
  double ring = 0.8, ringOvertones = 0.55, ringOvertones45 = 0.78;
  double alongDetune = 0.0015; // along the top a string sounds this much sharper (its ends give less)
  double radiate = 4.8e-3;     // Pa at the microphones per m/s^2 of the top's first resonance
  // How many modes a string has (at most Guitar::kMaxModes), and none above topHz (0: the
  // sample rate's limit).
  int modes = 48;
  double topHz = 0;
  int strings = 6;             // (an electric bass: 4, the first four of str[])
  // The two directions askew: their axes tilted from towards-the-top (rad), each mode its own
  // a little (+-tiltJitter); their pitch apart by alongDetune, each mode +-alongJitter of it.
  double tilt = 0, tiltJitter = 0, alongJitter = 0;
  double towardsLoss = 1;      // the direction towards the top loses this much more
  // The electric's pickups (none: an acoustic). at, gap: from the saddle and from the magnet
  // to the string at rest (m), per string; width: the field's width along the string (m, a
  // Gaussian's sigma); apart: a humbucker's two coils (m; 0: one coil); its coil L (H), R
  // (Ohm), C (F); volts: EMF per m/s of the string at small swings.
  struct Pickup { const char* name; double at[6], gap[6], width, apart, L, R, C, volts; };
  std::vector<Pickup> pickups;
  int select[2] = {0, -1};     // the pickups the selector puts on (-1: none)
  double pots = 125e3;         // the volume and tone pots across the pickup (Ohm)
  double magnetR = 2.45e-3, magnetL = 17e-3;  // the pole piece: radius, length (m)
  double sideways = 0.06;      // what the pickup hears of the swing across its magnet
  double palmSat = 30;          // the palm takes at most this many times the fundamental's loss
  // How much steel each string brings into the field (relative): a string is magnetised
  // across its section, so the coil hears it in proportion (to its steel's area, ^0.8 [E]).
  double steel[6] = {1, 1, 1, 1, 1, 1};
  // The pick's own sound on an acoustic (research/steel_guitar_build.md 8.1): a short burst as the
  // string leaves its edge (Pa per N of the stroke's force; 0: none), on the lowest `wound`
  // strings the edge rattling over the wrap for a millisecond or two more.
  double click = 0;
  int wound = 0;
  // The jazz archtop's (research/jazz_guitar.md 3.2, 9.1): each string's loss above the
  // fundamental as loss1 k^lossExp (flatwounds: their ribbon's turns rubbing, k^2; the rest k^1.2).
  double lossExp[6] = {1.2, 1.2, 1.2, 1.2, 1.2, 1.2};
  // A pickup mounted in the top (2.5): it moves with the body under it, `mount` of the bridge's
  // motion in each mode (as its sound's sign), and hears every string's steel come nearer or go
  // (0: a pickup that does not move).
  double mount = 0;
  // The frets as obstacles (research/slap_bass_1.md 5.2; off by default: no cost, no change): each
  // fret's crown a point the string cannot pass towards the board; struck there it is stopped at
  // once (a rigid, inelastic projection). The geometry: the string's height over the 17th fret
  // (m, per string; open), over the first fret at the nut, the neck's relief (a parabola between
  // the 1st and last fret, its deepest), each crown its own +-fretJitter (m).
  bool fretContact = false;
  double action[6] = {2.4e-3, 2.3e-3, 2.2e-3, 2.0e-3, 2.0e-3, 2.0e-3};
  double nutClear = 0.5e-3, relief = 0.3e-3, fretJitter = 0.03e-3;
  // How the string leaves a crown it struck: its speed into it given back this much (1: as it
  // came, no loss; 0: it stays on, which in 100-odd modes drains a pop of most of its energy in
  // 50 ms). 0.6: the slap's and the pop's brightness as FluidR3's recorded ones (src/ebass.cc).
  double fretBounce = 0.6;
  // A stopped string's partials losing as the open string's of the same frequency (not of the same
  // number): fretted high, its top dies as soon as the open string's would (false: as before).
  bool lossByFrequency = false;
  // The crowns' contact as seen by the modes: over this width (m; a raised cosine: the crown's
  // round, the string's own stiffness bending it over it); 0: a point.
  double fretWidth = 0;
  // The fretless bass (research/fretless_bass.md 2, 8.2; off by default: no cost, no change). The
  // board behind the stopping finger: a few points where the string, swinging towards it, lies
  // down on it every period (the mwah): a one-sided spring boardK (N/m) with a damper boardC
  // (N s/m), the double bass's; the string over the board's end (boardEnd m from the saddle)
  // boardAction m high, the neck's relief boardRelief (its curvature, 1/m). A stopped string ends
  // on a fingertip: fingerLoss times the double bass's fingertip loss (0.3 + 0.12 k + 15 (f / 4
  // kHz)^2 per s) instead of the fret's x1.15 (0: a fret).
  bool board = false;
  double boardK = 3e4, boardC = 1.5, boardEnd = 0.27, boardAction = 2.5e-3, boardRelief = 0;
  double fingerLoss = 0;
};
GuitarSpec classicalGuitar();  // with the PJ_GUITAR_* knobs
GuitarSpec electricGuitar(const char* name);  // "clean-guitar": a three-pickup solid-body
// The steel-string acoustic (GM 26, research/steel_guitar.md, steel_guitar_build.md 8.1): a
// dreadnought, phosphor-bronze 12-53, with the PJ_STEEL_* knobs.
GuitarSpec steelGuitar();
// The jazz guitar (GM 27, research/jazz_guitar.md 9.1): a laminated archtop, a laminated archtop on a
// floating bridge, flatwounds 12-52, the neck humbucker mounted in the top; PJ_JAZZ_* knobs.
GuitarSpec jazzGuitar();

class Guitar {
 public:
  static constexpr int kStrings = 6, kMaxModes = 128;
  explicit Guitar(double fs, const GuitarSpec& spec = classicalGuitar());
  const GuitarSpec& spec() const { return s_; }
  bool electric() const { return !s_.pickups.empty(); }
  int frets() const { return s_.frets; }
  double open(int s) const { return s_.str[s].f0; }
  // The left hand: string s (0: low E) stopped at a fret (0: open), at once.
  void fret(int s, int fret);
  // The right hand: pull string s aside by `depth` (m) at `fromBridge` (m),
  // `angle` from the top's plane (rad: 0 along the top, 1.57 straight into
  // it), `nail` 0 (flesh alone) .. 1 (nail alone), and let it go.
  // strum (1 down, 2 up; 0: a plucking finger): not a finger that takes the
  // string first, but a hand passing over it: flesh and nail together brush
  // the string as it is (still ringing, if it was). They are broad and
  // soft where a plucking nail's edge is narrow and quick: the string leaves
  // them in a rounded shape, with few of its high overtones: warm, thick,
  // velvety; the fingertip's pad coming up most of all.
  // clack: 0 .. 1, the nail striking the string as the finger takes it, a
  // short knock before the pull (the hardest strokes; flamenco's): it adds
  // to the upper middle of the tone, not to the very top.
  void pluck(int s, double depth, double fromBridge, double angle, double nail, int strum = 0, double clack = 0);
  // A pick: string s pushed `depth` (m) aside at `fromBridge` (m), `angle` from the top's plane
  // (rad: a down stroke ~ +0.25, along and a little in; an up stroke ~ pi + 0.12), over `rise`
  // seconds, then slipping off the edge. It passes: the string's sound goes on. width: the
  // edge's on the string (m; a thumb-pick's 1.5 mm).
  void pick(int s, double depth, double fromBridge, double angle, double rise = 0.001, double width = 0.0008);
  // A bass player's finger: the pad (`width` m wide, calloused) takes the string over `rise`
  // seconds, pulls it `depth` aside and lets it slip off in `slip` seconds; no nail.
  void pluckPad(int s, double depth, double fromBridge, double angle, double width, double rise, double slip);
  // A bassist's pop (research/slap_bass_1.md 5.4): pluckPad with a longer hook (rise up to 30 ms:
  // the finger hooks under the string and pulls it out from the body before it slips off).
  void pop(int s, double depth, double fromBridge, double angle, double width, double rise, double slip);
  // A slapping thumb (research/slap_bass_1.md 5.3): a mass `mass` (kg) on a flesh spring
  // `stiffness` (N/m), `width` (m) wide, starting `gap` (m) off string s at `fromBridge` and
  // coming at `speed` (m/s) along `angle` (rad from the top's plane: ~1.3 into the body; ~-1.3
  // an upstroke, out of it). It strikes and rebounds: from 2 ms after it first touches, the
  // forearm's turn pulls it back at `lift` (m/s^2). The flesh is lossy: `damping` (s/m), Hunt
  // and Crossley's (its force K x (1 + damping dx/dt)). What the string sounded is a little held while it lies on it.
  void thump(int s, double speed, double fromBridge, double angle, double width = 0.015, double mass = 0.02,
             double stiffness = 3e4, double lift = 30, double gap = 0.002, double damping = 0);
  // The fret contacts (GuitarSpec::fretContact): how many samples string s has touched a fret
  // since the last call (and reset), the frets touched (a bit each, 1..31).
  int fretHits(int s, uint32_t* which = nullptr);
  double energy(int s) const;  // string s's energy in its modes (J), for checking
  // A pick held by a hand (research/pick_bass.md 4.2, 9.2b): not a force but a place. The hand
  // carries the pick's tip `depth` (m) aside at `fromBridge` (m), `angle` from the top's plane,
  // over `pushT` seconds (a raised cosine: 2-20 ms); the tip is a spring of `stiffness` (N/m:
  // 2-20 kN/m from a thin pick to a heavy one) against the string where it is now (it only
  // pushes: a string swinging away is caught up with, one swinging towards it is met at once).
  // The string lets go when the hand has gone its way, or earlier when the tip has bent
  // `bendMax` (m; 0: never), and slips off the edge in `slip` seconds. Slow push, quick slip:
  // the string leaves near its static shape (the low modes whole: loud) and leaves it sharply
  // (the high ones: bright). It passes: what the string was sounding goes on.
  // width: the tip's contact along the string (m; a pointed tip 0.8 mm, a rounded heavy jazz pick
  // 2-3 mm).
  void pickContact(int s, double depth, double fromBridge, double angle, double pushT, double slip = 0.00015,
                   double stiffness = 6000, double bendMax = 0, double width = 0.0008);
  // The left hand pushes string s sideways along its fret: it is stretched,
  // and sounds this many cents sharper (0: let back).
  void bend(int s, double cents);
  // Something lying on string s: its sound dies at `rate` (1/s); 0: lifted.
  void damp(int s, double rate);
  // The palm (research/electric_guitars.md 8): the side of the picking hand lying on string s
  // `at` (m) from the saddle, over 12 mm; `depth`: what it takes of the open string's fundamental
  // (1/s); each mode loses in proportion to how much it moves under the hand (its shape squared,
  // averaged over the contact), up to GuitarSpec::palmSat times the fundamental's: the
  // overtones die within a tenth of a second, the fundamental thuds on for a third. 0: lifted.
  void mute(int s, double depth, double at = 0.012);
  // A harmonic (research/electric_guitars.md 9.1-9.2, as Bass::touch): a fingertip lying lightly
  // on string s at 1/n of its sounding length (its own place `off` m off the node, `width` m wide)
  // for `secs`, then lifted. Each mode loses `rate` (1/s) times its shape squared there, averaged
  // over the fingertip: the modes not multiples of n die under it, n, 2n, 3n ... ring on. Call
  // after fret(); n < 2: lifted at once.
  void touch(int s, int n, double secs, double rate, double width = 0.004, double off = 0);
  // Feedback, as a player finds it (research/electric_guitars.md 5): standing where the amp's
  // sound comes back in phase with mode k of string s, so that mode gets back what it loses,
  // up to a level the amp's compression sets: `level` times the amplitude mode `ref` has now.
  // It grows towards it (or holds) at up to `rate` (1/s). k 0: off (so do a pick and a damp).
  // Two at once (slot 0, 1): the fundamental held while a harmonic blooms.
  void sustain(int s, int k, int ref, double level, double rate, int slot = 0);
  double modeAmp(int s, int k) const;  // mode k's swing now (m/s, both directions)
  // The same for every mode of string s below `hz`: each held at `level` times its swing now
  // (0: let go).
  void hold(int s, double hz, double level, double rate);
  // The sound: an acoustic's at its microphones (Pa); an electric's pickup EMF (V).
  float sample();
  float side() const { return side_; }
  // An electric's own acoustic sound (its body's radiation, Pa), as sample() would give it on an
  // acoustic: the archtop heard in the room under its amp.
  float acoustic() const { return acoustic_; }
  void reset();
  bool asleep() const { return asleep_; }
  // Tests: a kick (or a NaN) into string 0's first mode on the next sample; and how many times a
  // non-finite state was caught and cleared (reset()) inside sample().
  void kick(float v) { str_[0].v[0][0] += v; str_[0].active = true; asleep_ = false; }
  int nanResets() const { return nanResets_; }
  double frequency(int s) const;  // as stopped now
  // The fretless's left hand (GuitarSpec::board; any guitar can): string s stopped `semis`
  // semitones above its open pitch, anywhere along the board (fractional; 0: open). Called again
  // each control block as the finger slides or rocks: a slow change of length keeps each mode's
  // action (its swing's amplitude; its velocity follows its frequency), no new attack.
  // fret(s, f) is stop(s, f) at once.
  void stop(int s, double semis);
  double stopped(int s) const { return str_[s].fret + str_[s].cents / 100; }
  // The board (GuitarSpec::board): how many samples string s has lain on it since the last call.
  int boardHits(int s) { const int n = boardHits_[s]; boardHits_[s] = 0; return n; }

 private:
  struct Str {
    int fret = 0, n = 0;  // n: its modes
    double len = 0.65, f1 = 110, bend = 0;
    alignas(64) float q[2][kMaxModes] = {}, v[2][kMaxModes] = {};
    alignas(64) float cs[2][kMaxModes], sn[2][kMaxModes], w[2][kMaxModes], decay[2][kMaxModes];
    alignas(64) float slope[kMaxModes] = {}, bridgeIn[kMaxModes] = {};
    // A stroke's force on each mode, each direction: the flesh's (or the pick's), and the nail's.
    alignas(64) float pushF[2][kMaxModes] = {}, pushN[2][kMaxModes] = {};
    // The pickups' ear for each mode, each direction (the selected two).
    alignas(64) float ear[2][2][kMaxModes] = {};
    float tilt[kMaxModes] = {}, split[kMaxModes] = {};  // each mode's axes, and its pitch apart
    alignas(64) float ct[kMaxModes], st[kMaxModes];      // (the axes' cosine and sine)
    double flux[2] = {}, side[2] = {};  // the pickups' last flux and sideways swing
    float invMass = 0;
    // The finger: planted, pulling, then letting go off the flesh and off the nail.
    double clock = 1e9, force = 0, nail = 0.5, fromBridge = 0.12;
    // This stroke's rise, its slips off the flesh and off the nail (s), the
    // finger's damping while it lies on (1/s), the flesh's and the nail's width on the string (m).
    double plant = 0.005, flesh = 0.0022, nailSlip = 0.0005, hold = 300, wFlesh = 0.009, wNail = 0.0015, clack = 0;
    double dampRate = 0;
    double clickT = 1e9;              // the pick's click: s since the stroke began (idle: 1e9)
    double muteDepth = 0, muteAt = 0.012;
    int touchN = 0;                   // a harmonic's finger: at 1/touchN (0: none), how long yet
    double touchLeft = 0, touchRate = 0, touchWidth = 0.004, touchOff = 0;
    int fbMode[2] = {-1, -1};         // feedback: the modes held (-1: none), their levels, rates
    double fbTarget[2] = {}, fbRate[2] = {};
    int holdN = 0;                    // the low modes held, their levels and own losses
    double holdRate = 0;
    float holdTarget[kMaxModes] = {}, holdOwn[kMaxModes] = {};
    bool active = false;
    // A held pick (pickContact): on, slipping; time in this phase (s); how far the hand goes (m),
    // over how long, the slip (s); the tip's stiffness (N/m), the force it lets go at, the force
    // when the slip began (N).
    bool pkOn = false, pkSlip = false;
    double pkT = 0, pkDepth = 0, pkPush = 0.005, pkSlipT = 0.00015, pkK = 6000, pkRelease = 1e9, pkF = 0;
    // A thumb (thump): on; its place along the stroke (m), speed (m/s), time since it began and
    // since it first touched (s), its mass, spring, lift.
    bool thOn = false;
    double thZ = 0, thV = 0, thT = 0, thTouch = -1, thM = 0.02, thK = 3e4, thLift = 30, thC = 0, thYs = 0;
    // The frets (fretContact): the first fret over the sounding string, how many; s since a
    // stroke or a contact; contacts counted, the frets touched.
    int fretFirst = 0, fretCount = 0, fretHits = 0;
    uint32_t fretWhich = 0;
    // The fretless (stop()): cents beyond `fret`; where the modes were last fully placed (semis).
    double cents = 0, placed = 0;
    double fretIdle = 0;
  };
  void tune(Str& st, int index, bool place = true);
  static double underFinger(double k, double at, double width, double len);  // the same under a fingertip
  static double underPalm(double k, double at, double len);  // a mode's mean shape^2 under the palm  // place: the pickups' ears too (a new length)
  void shapeAt(Str& st, float* shape, double fromBridge, double width, double apart = 0);
  void push(Str& st, const float* shape, float (*to)[kMaxModes], double angle);
  double field(int p, double gap) const;  // the magnet's field on its axis (arbitrary units)
  float pickups();
  template <bool kArch>
  float run();   // sample(), the archtop's path or the rest's
  bool arch_ = false;
  float contact(Str& st, float& hold);  // a held pick's force on the string this sample (N)
  float thumbForce(Str& st, float& hold);  // a thumb's force on the string this sample (N)
  // The frets (fretContact): each fret's towards-the-board shape for each mode, both directions
  // (fretA_[string][fret][dir][mode]), its clearance and its compliance; the write-back factors.
  void placeFrets(int s);
  void fretStep(Str& st, int s, float& y0, float& y1);
  // The fretless's board (GuitarSpec::board): kBoardPts points behind the stopping finger, each
  // mode's shape there towards the board (board_[string][point][dir][mode]), their clearance and
  // their dampers' limit; s since a stroke or a contact; samples touching.
  static constexpr int kBoardPts = 5;
  void placeBoard(int s);
  void boardStep(Str& st, int s);
  std::vector<float> board_;
  float boardClear_[kStrings][kBoardPts] = {}, boardHold_[kStrings][kBoardPts] = {};
  int boardN_[kStrings] = {}, boardHits_[kStrings] = {}, boardTick_[kStrings] = {};
  uint32_t boardLive_[kStrings] = {};
  double boardClock_[kStrings] = {};
  static constexpr int kMaxFrets = 24;
  std::vector<float> fretA_, fretK_;  // kStrings*kMaxFrets*2*kMaxModes; kStrings*4*kMaxModes
  float fretClear_[kStrings][kMaxFrets] = {}, fretG_[kStrings][kMaxFrets] = {};
  double crown_[kStrings][kMaxFrets + 1] = {}, saddleH_[kStrings] = {};
  uint32_t fretLive_[kStrings] = {};  // (the gate: the frets the string could reach now)
  float fretTail_[kStrings][kMaxFrets] = {};  // (and how far the modes above the 32nd could add there)
  bool fretFull_[kStrings] = {};      // (a stroke or a contact just now: every mode, every sample)
  int fretTick_[kStrings] = {};
  double fieldSlope_[2][kStrings] = {};
  GuitarSpec s_;
  double fs_, dt_;
  Str str_[kStrings];
  struct Body { float x = 0, v = 0, a11, a12, a21, a22, invMass, rad, side, along, mic = 0; };
  std::vector<Body> body_;
  float bridgeV_ = 0, bridgeVz_ = 0, side_ = 0, acoustic_ = 0;
  uint32_t clickRng_ = 90001;        // the pick's click: its noise, a band-pass's state
  float clickX1_ = 0, clickX2_ = 0, clickY1_ = 0, clickY2_ = 0;
  float clickB0_ = 0, clickA1_ = 0, clickA2_ = 0;
  bool asleep_ = true;
  int quiet_ = 0;
  int nanResets_ = 0;
  static constexpr double kPlant_ = 0.005, kFlesh_ = 0.0022, kNail_ = 0.0005;  // s: the finger's rise; its two slips
};

}  // namespace pajupilli
