#include "guitar.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <type_traits>

#include "knob.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
double urand(uint32_t& s) {
  s = s * 1664525u + 1013904223u;
  return (s >> 8) * (1.0 / 16777216.0);
}
}  // namespace

GuitarSpec classicalGuitar() {
  GuitarSpec g;
  g.name = "nylon-guitar";
  g.scale = 0.650;
  g.frets = 19;
  // The strings (research/guitars.md 1.1): three of plain nylon, three of
  // silvered copper wound on nylon floss. Nylon loses more than steel, the
  // high overtones most; the wound basses ring longer.
  const double lossT = knob("PJ_GUITAR_LOSS", 1.0), lossHigh = knob("PJ_GUITAR_LOSSHIGH", 1.0);
  const double f[6] = {82.41, 110.0, 146.83, 196.0, 246.94, 329.63};
  const double T[6] = {62, 68, 70, 61, 62, 83};
  const double B[6] = {1.0e-5, 1.0e-5, 1.0e-5, 9.3e-5, 3.7e-5, 1.6e-5};
  const double l0[6] = {1.1, 1.2, 1.3, 1.6, 1.8, 2.0}, l1[6] = {0.10, 0.11, 0.12, 0.30, 0.33, 0.36};
  for (int i = 0; i < 6; ++i) g.str[i] = {f[i], T[i], B[i], lossT * l0[i], lossT * lossHigh * l1[i]};
  // The body (3.1): the air with the top, the top's own, the back's, the
  // next plate and air modes; then the plates' many more, each lighter in
  // the sound and heavier under the bridge the higher it lies.
  const double massK = knob("PJ_GUITAR_BODYMASS", 1.0);
  g.body = {{100, 25, 0.9 * massK, 0.75, 0.05, 0.05},  {200, 22, 0.30 * massK, 1.00, -0.08, 0.08}, {236, 30, 1.1 * massK, 0.25, 0.10, -0.30},
            {290, 28, 0.60 * massK, 0.40, 0.20, 0.45}, {400, 32, 0.90 * massK, 0.40, -0.25, -0.20}};
  uint32_t rng = 1905;
  double fq = 455;
  for (int i = 0; i < 23; ++i) {
    const double t = i / 22.0;
    g.body.push_back({fq * (1 + 0.04 * (urand(rng) - 0.5)), 30 + 25 * urand(rng), massK * (0.56 + 1.8 * t) * (0.7 + 0.6 * urand(rng)),
                      (urand(rng) < 0.5 ? -1 : 1) * (0.5 - 0.32 * t) * (0.5 + urand(rng)), (urand(rng) - 0.5) * 0.6,
                      (urand(rng) < 0.5 ? -1 : 1) * (0.1 + 0.5 * urand(rng))});
    fq *= 1.092;  // (to 3.2 kHz)
  }
  g.alongDetune = knob("PJ_GUITAR_ALONGDETUNE", 0.0015);
  g.ring = knob("PJ_GUITAR_RING", 0.8);
  g.ringOvertones = knob("PJ_GUITAR_RING23", 0.55);
  g.ringOvertones45 = knob("PJ_GUITAR_RING45", 0.78);
  g.radiate = 9.6e-3 * knob("PJ_GUITAR_OUT", 1.0);
  return g;
}

// The Stratocaster (research/electric_guitars.md 4, 6): 648 mm, 21 frets, D'Addario 10-46. GM 28
// ("clean-guitar") and GM 29 ("muted-guitar": the same guitar, the palm on the strings) differ
// only in the pickups selected and how they are played.
GuitarSpec electricGuitar(const char* name) {
  GuitarSpec g;
  g.name = name;
  g.scale = 0.648;
  g.frets = 21;
  // The strings (4.1): tensions from the maker's chart, B from the wire (the plain G the most
  // inharmonic). How long they ring (4.2) [E: until fitted]: the fundamental 14 s on the low E
  // to 5 s on the high one; the overtones shorter, more so on the high strings (air friction).
  const double lossT = knob("PJ_EGTR_LOSS", 1.0), lossHigh = knob("PJ_EGTR_LOSSHIGH", 1.0);
  const double f[6] = {82.41, 110.0, 146.83, 196.0, 246.94, 329.63};
  const double T[6] = {75.2, 84.5, 81.8, 73.8, 68.5, 72.1};
  const double B[6] = {1.3e-4, 5.8e-5, 2.4e-5, 1.1e-4, 4.0e-5, 1.3e-5};
  const double t60[6] = {14, 12, 10, 7, 6, 5};
  for (int i = 0; i < 6; ++i)
    g.str[i] = {f[i], T[i], B[i], lossT * 6.91 / t60[i], lossT * lossHigh * 0.06 * std::pow(f[i] / 82.41, 0.8)};
  g.ring = g.ringOvertones = g.ringOvertones45 = 1;
  g.modes = 96;
  g.topHz = 9000;  // (above, the coil's resonance and the speaker leave nothing)
  g.alongDetune = knob("PJ_EGTR_SPLIT", 0.0007);
  g.alongJitter = 0.5;
  g.tilt = knob("PJ_EGTR_TILT", 0.35);
  g.tiltJitter = 0.3;
  g.towardsLoss = 1.15;
  // The neck (5, 4.2): two of its resonances under the strings' ends; a note on one rings
  // shorter (the dead spots, often near G3-B3). Heavy: they take little; nothing is radiated.
  const double neck = knob("PJ_EGTR_NECK", 1.0);
  g.body = {{knob("PJ_EGTR_DEADHZ", 212), 25, 0.12 / neck, 0, 0, 0.15}, {338, 30, 0.2 / neck, 0, 0, -0.1}};
  g.radiate = 0;
  // The pickups (6.1, measured on a Stratocaster; low E first): from the saddle; the gaps
  // [E] (set lower under the treble strings, the neck pickup further off); a single coil
  // 2.2 H, 6.5 kOhm, 110 pF (6.2); its field ~20 mm wide.
  const double volts = 0.15 * knob("PJ_EGTR_VOLTS", 1.0), width = knob("PJ_EGTR_WIDTH", 6e-3);
  g.pickups = {
      {"neck", {0.158, 0.160, 0.159, 0.160, 0.158, 0.157}, {3.8e-3, 3.7e-3, 3.6e-3, 3.5e-3, 3.3e-3, 3.2e-3}, width, 0, 2.2, 6.5e3, 110e-12, volts},
      {"middle", {0.100, 0.102, 0.101, 0.102, 0.100, 0.099}, {3.4e-3, 3.3e-3, 3.2e-3, 3.1e-3, 3.0e-3, 2.9e-3}, width, 0, 2.2, 6.5e3, 110e-12, volts},
      {"bridge", {0.049, 0.049, 0.046, 0.045, 0.041, 0.038}, {3.0e-3, 2.9e-3, 2.8e-3, 2.7e-3, 2.6e-3, 2.5e-3}, width, 0, 2.2, 6.5e3, 110e-12, volts}};
  // The steel in each string's section (plain: all of it; wound: ~60 %, the nickel-plated
  // wrap's gaps and a core), against the D string's: the thin plain strings would otherwise be
  // the loudest by far, their swing as wide and quicker (6.1 [E]).
  {
    const double d[6] = {0.046, 0.036, 0.026, 0.017, 0.013, 0.010};
    const bool wound[6] = {true, true, true, false, false, false};
    const double ref = 0.6 * 0.026 * 0.026;
    for (int i = 0; i < 6; ++i) g.steel[i] = std::pow((wound[i] ? 0.6 : 1.0) * d[i] * d[i] / ref, knob("PJ_EGTR_STEELEXP", 0.8));
  }
  // The five-way selector: 1 bridge, 2 bridge + middle, 3 middle, 4 middle + neck, 5 neck.
  // GM 28's: 4, the in-between "quack" (2.2, 13); GM 29's: 2, bridge and middle (the funk
  // player's, 13).
  static const int kSel[5][2] = {{2, -1}, {2, 1}, {1, -1}, {1, 0}, {0, -1}};
  const bool muted = std::strcmp(name, "muted-guitar") == 0;
  // GM 32's (guitar-harmonics): 3, the middle alone: the neck pickup lies on the open strings'
  // 4th-harmonic node (the 5th fret's harmonic would be gone), the bridge's hears the low
  // harmonics thinly; the middle has no node near harmonics 2-5.
  const bool harmonics = std::strcmp(name, "guitar-harmonics") == 0;
  const int pos = std::clamp(int(knob("PJ_EGTR_PICKUP", muted ? 2 : harmonics ? 3 : 4)), 1, 5) - 1;
  g.select[0] = kSel[pos][0];
  g.select[1] = kSel[pos][1];
  g.pots = 125e3;  // (250 k volume and 250 k tone, both full up)
  g.palmSat = knob("PJ_EGTR_PALMSAT", 30.0);
  if (std::strcmp(name, "overdriven-guitar") == 0 || std::strcmp(name, "distortion-guitar") == 0) {
    // A Les Paul (GM 30 and 31, 2.4, 13; distortion_guitar.md 10.7: the same hot bridge pickup): 628 mm, 22 frets; the same strings 6 % slacker and 13 % more
    // inharmonic; two humbuckers (Les Paul: bridge ~40 mm, neck ~155 mm from the saddle [M]),
    // each two coils 18 mm apart in series: twice a coil's signal, and the pair's own comb. The
    // bridge one hot (a 498T: 8.0 H, 6.2 [T]); 500 k pots.
    const double r = 0.648 / 0.628;
    g.scale = 0.628;
    g.frets = 22;
    // (Its overtones ring longer: the Les Paul's sustain, the mahogany and the set neck [L];
    // half the Strat's estimated loss above the fundamental.)
    const double lpHigh = knob("PJ_EGTR_LPRING", 0.5);
    for (GuitarString& st : g.str) { st.tension /= r * r; st.B *= r * r; st.loss1 *= lpHigh; }
    const double hv = 0.15 * 2.6 * knob("PJ_EGTR_VOLTS", 1.0), hw = knob("PJ_EGTR_WIDTH", 5e-3);
    g.pickups = {
        {"neck", {0.155, 0.155, 0.155, 0.155, 0.155, 0.155}, {3.2e-3, 3.1e-3, 3.0e-3, 2.9e-3, 2.8e-3, 2.7e-3}, hw, 0.018, 4.2, 7.5e3, 120e-12, 0.8 * hv},
        {"bridge", {0.044, 0.043, 0.042, 0.040, 0.039, 0.038}, {2.8e-3, 2.7e-3, 2.6e-3, 2.5e-3, 2.4e-3, 2.4e-3}, hw, 0.018, 8.0, 13.5e3, 150e-12, hv}};
    // The three-way switch: 1 bridge, 2 both, 3 neck. GM 30's: the bridge.
    static const int kLp[3][2] = {{1, -1}, {1, 0}, {0, -1}};
    const int lp = std::clamp(int(knob("PJ_EGTR_PICKUP", 1)), 1, 3) - 1;
    g.select[0] = kLp[lp][0];
    g.select[1] = kLp[lp][1];
    g.pots = 250e3;  // (500 k volume and tone)
  }
  return g;
}

// The steel-string guitar (research/steel_guitar_build.md 8.1, 2.2): a dreadnought, 645 mm,
// 20 frets, phosphor-bronze 12-53: the strings twice as taut as nylon and losing far less (the
// overtones ring for seconds: the shimmer), on a stiffer, heavier top that takes little from
// them; the wound strings' loss is their wrap's, the plain ones' the air's.
GuitarSpec steelGuitar() {
  GuitarSpec g;
  g.name = "steel-guitar";
  g.scale = 0.645;
  g.frets = 20;
  const double lossT = knob("PJ_STEEL_LOSS", 1.0), lossHigh = knob("PJ_STEEL_LOSSHIGH", 1.0);
  const double f[6] = {82.41, 110.0, 146.83, 196.0, 246.94, 329.63};
  const double T[6] = {111.0, 128.7, 133.1, 133.7, 103.7, 103.9};
  const double B[6] = {1.1e-4, 4.9e-5, 2.1e-5, 0.9e-5, 6.1e-5, 1.9e-5};
  const double l0[6] = {0.30, 0.40, 0.40, 0.50, 0.65, 0.85}, l1[6] = {0.040, 0.030, 0.038, 0.055, 0.075, 0.10};
  for (int i = 0; i < 6; ++i) g.str[i] = {f[i], T[i], B[i], lossT * l0[i], lossT * lossHigh * l1[i]};
  g.ring = g.ringOvertones = g.ringOvertones45 = 1;
  g.modes = 96;
  g.topHz = 10000;
  g.towardsLoss = 1;
  g.tilt = knob("PJ_STEEL_TILT", 0.2);
  g.tiltJitter = 0.3;
  g.alongDetune = knob("PJ_STEEL_SPLIT", 0.0007);
  g.alongJitter = 0.5;
  // The body (2.2): the breathing triad (air and top, top and air, the back), the two dipoles
  // (which radiate little), then the plates to 3.6 kHz, their masses set to the measured mean
  // mobility at the bridge (-53 dB re 1 m/(s N) to 2 kHz, -56 dB above). An OM (PJ_STEEL_BODY=1):
  // the first five 8 % higher and 15 % lighter.
  const double massK = knob("PJ_STEEL_BODYMASS", 1.0);
  const bool om = knob("PJ_STEEL_BODY", 0) > 0.5;
  const double fk = om ? 1.08 : 1.0, mk = massK * (om ? 0.85 : 1.0);
  g.body = {{98 * fk, 35, 1.10 * mk, 0.75, 0.05, 0.05},  {183 * fk, 23, 0.22 * mk, 1.00, -0.08, 0.08}, {204 * fk, 30, 1.0 * mk, 0.55, 0.10, -0.15},
            {350 * fk, 40, 0.60 * mk, 0.20, 0.25, 0.45}, {436 * fk, 40, 0.80 * mk, 0.15, -0.25, -0.30}};
  uint32_t rng = 1833;
  double fq = 470;
  // (Above 3.6 kHz nothing is measured; the plates go on as they were, to 10 kHz: the steel
  // strings' overtones up there are the instrument's shimmer and must come out of the body.)
  const double radHigh = knob("PJ_STEEL_RADHIGH", 0.2);
  for (int i = 0; fq < 10000; ++i) {
    const double t = std::min(1.0, i / 27.0);
    const double G = fq < 2000 ? 0.0022 : 0.0022 - 0.0007 * std::min(1.0, (fq - 2000) / 1500);
    g.body.push_back({fq * (1 + 0.04 * (urand(rng) - 0.5)), 30 + 25 * urand(rng), massK / (0.3 * fq * G) * (0.7 + 0.6 * urand(rng)),
                      (urand(rng) < 0.5 ? -1 : 1) * (0.5 - (0.5 - radHigh) * t) * (0.5 + urand(rng)), (urand(rng) - 0.5) * 0.6,
                      (urand(rng) < 0.5 ? -1 : 1) * (0.1 + 0.5 * urand(rng))});
    fq *= 1.075;
  }
  g.radiate = 9.6e-3 * knob("PJ_STEEL_OUT", 1.0);
  g.click = knob("PJ_STEEL_CLICK", 1.0) * 0.035;  // (the first 5 ms above 5 kHz at -11 dB of the whole: FluidR3's, build note 1.3)
  g.wound = 4;
  return g;
}

// The jazz guitar (GM 27; research/jazz_guitar.md 9.1): a Gibson ES-175. 628 mm, 20 frets.
//   - Flatwounds 12-52 (3.1: TI JS112 / D'Addario ECG25 at 628 mm): ~590 N in all, 1.4x the
//     Strat's set; the G wound (B ~1e-5: a wound G against the plain G's 1.1e-4), the wound strings'
//     B 1.75x their core's (the flat ribbon carries some of the bending). Their loss above the
//     fundamental steep (k^2: the ribbon's turns rubbing), fitted to Rau17's one measured
//     flatwound (3.2): the fundamental of the open low E 15 dB down in ~5.5 s, its 3rd-5th
//     harmonics in ~1 s (loss1 0.036-0.054 k^2). The plain strings as the Strat's, their loss
//     above 1.1x.
//   - The body (2.3): the neck's three modes (85, 175, 332 Hz: Rau17), the air through the
//     f-holes (125), the arched top's first (240), the bridge rocking on its two feet (420), the
//     plates to 2.2 kHz; laminated maple: Q x0.8, rad x0.5. The floating bridge drains the swing
//     towards the top twice as fast as the one along it: the punch, then the long tail.
//   - The neck humbucker, its two pole rows 160 and 178 mm from the saddle (the comb's first
//     notch at the open strings' 3.7th harmonic), 4.4 H, 7.8 kOhm, 120 pF, low under the heavy
//     strings (3.8-3.4 mm); mounted in the top: it moves with it (mount 0.4) and hears the body.
//     A bridge humbucker too (PJ_JAZZ_PICKUP: 1 bridge, 2 both, 3 neck, the default).
//   - 500 k volume (full up); the tone pot and its 22 nF are the amp's input (guitaramp.h Source).
GuitarSpec jazzGuitar() {
  GuitarSpec g;
  g.name = "jazz-guitar";
  g.scale = 0.628;
  g.frets = 20;
  const double lossT = knob("PJ_JAZZ_LOSS", 1.0), lossHigh = knob("PJ_JAZZ_LOSSHIGH", 1.0);
  const double f[6] = {82.41, 110.0, 146.83, 196.0, 246.94, 329.63};
  const double T[6] = {98, 96, 101, 103, 95, 96};
  const double B[6] = {3.5e-4, 1.1e-4, 3e-5, 1e-5, 7.0e-5, 2.2e-5};
  // (The research's loss0 0.30-0.90 x0.6: the swing along the top rings on, the one towards it
  // dies 2x as fast (towardsLoss): the archtop's punch, then the long tail.)
  const double l0[6] = {0.18, 0.21, 0.27, 0.33, 0.42, 0.54};
  // (wound: k^2; plain: k^1.2, 1.1x the Strat's 0.06 (f / 82.41)^0.8)
  const double l1[6] = {0.036, 0.0405, 0.045, 0.054, 1.125 * 0.06 * std::pow(246.94 / 82.41, 0.8), 1.125 * 0.06 * std::pow(329.63 / 82.41, 0.8)};
  const double flatExp = knob("PJ_JAZZ_FLATEXP", 2.0);
  for (int i = 0; i < 6; ++i) {
    g.str[i] = {f[i], T[i], B[i], lossT * l0[i], lossT * lossHigh * l1[i]};
    g.lossExp[i] = i < 4 ? flatExp : 1.2;
  }
  g.ring = g.ringOvertones = g.ringOvertones45 = 1;
  g.modes = 64;
  g.topHz = 6000;  // (nothing above survives the tone control and the 15" speaker)
  g.alongDetune = 0.0007;
  g.alongJitter = 0.5;
  g.tilt = knob("PJ_JAZZ_TILT", 0.35);
  g.tiltJitter = 0.3;
  g.towardsLoss = knob("PJ_JAZZ_TOWARDS", 2.0);
  // The body: f, Q, mass under the bridge (kg), its sound, its side, its share of the along-pull.
  const double lam = 0.8, rad = 0.5, mk = knob("PJ_JAZZ_BODYMASS", 1.0);
  // (The masses: the research's table (2.3) x8 for the box, x100 for the neck's three modes, fitted
  // with the losses above to Rau17's open low E through the pickup: harmonics 1-5 15 dB down in 5.7,
  // 3.4, 0.5, 1.2, 1.0 s against 5.45, 4.49, 0.86, 0.87, 1.12. The table's own drained the
  // fundamental in 0.4 s: Rau sees the neck's modes at the bridge, but they take little.)
  const double nk = 100 * mk, bx = 8 * mk;
  g.body = {{85, 25 * lam, 0.25 * nk, 0, 0, 0.1},           {125, 20 * lam, 1.2 * bx, 0.6 * rad, 0.05, 0.05},
            {175, 28 * lam, 0.35 * nk, 0, 0, 0.1},           {240, 20 * lam, 0.45 * bx, 1.0 * rad, -0.08, 0.3},
            {332, 30 * lam, 0.5 * nk, 0.2 * rad, 0.1, -0.2}, {420, 25 * lam, 0.8 * bx, 0.35 * rad, 0.2, 0.6}};
  uint32_t rng = 175;
  double fq = 500;
  for (int i = 0; i < 15; ++i) {
    const double t = i / 14.0;
    g.body.push_back({fq * (1 + 0.04 * (urand(rng) - 0.5)), lam * (25 + 20 * urand(rng)), bx * (1 + 2 * t) * (0.7 + 0.6 * urand(rng)),
                      rad * (urand(rng) < 0.5 ? -1 : 1) * (0.3 - 0.25 * t) * (0.5 + urand(rng)), (urand(rng) - 0.5) * 0.6,
                      (urand(rng) < 0.5 ? -1 : 1) * 0.3 * urand(rng)});
    fq *= 1.11;
  }
  g.radiate = 9.6e-3;
  g.mount = knob("PJ_JAZZ_MOUNT", 0.4);
  const double hv = 0.8 * 2.6 * 0.15 * knob("PJ_JAZZ_VOLTS", 1.0), hw = 5e-3;
  g.pickups = {
      {"neck", {0.169, 0.169, 0.169, 0.169, 0.169, 0.169}, {3.8e-3, 3.7e-3, 3.6e-3, 3.6e-3, 3.5e-3, 3.4e-3}, hw, 0.018, 4.4, 7.8e3, 120e-12, hv},
      {"bridge", {0.046, 0.045, 0.044, 0.043, 0.042, 0.041}, {3.0e-3, 2.9e-3, 2.8e-3, 2.8e-3, 2.7e-3, 2.6e-3}, hw, 0.018, 4.4, 7.8e3, 120e-12, hv}};
  static const int kSel[3][2] = {{1, -1}, {1, 0}, {0, -1}};
  const int pos = std::clamp(int(knob("PJ_JAZZ_PICKUP", 3)), 1, 3) - 1;
  g.select[0] = kSel[pos][0];
  g.select[1] = kSel[pos][1];
  g.pots = 500e3;  // (the volume pot full up; the tone branch is the amp's Source)
  g.palmSat = 30;
  // The steel in the field: the wound flats nearly solid (0.9 of the diameter's area; round
  // wound 0.6), against the D string's.
  {
    const double d[6] = {0.052, 0.042, 0.032, 0.024, 0.016, 0.012};
    const double ref = 0.9 * 0.032 * 0.032;
    for (int i = 0; i < 6; ++i) g.steel[i] = std::pow((i < 4 ? 0.9 : 1.0) * d[i] * d[i] / ref, 0.8);
  }
  return g;
}

Guitar::Guitar(double fs, const GuitarSpec& spec) : s_(spec), fs_(fs), dt_(1 / fs) {
  arch_ = !s_.pickups.empty() && (s_.mount != 0 || s_.radiate > 0);
  {
    // (the pick's click: a band of noise round 4.5 kHz, 2 to 8 kHz)
    const double w = 2 * kPi * 4500 / fs, alpha = std::sin(w) / (2 * 0.7);
    clickB0_ = float(alpha / (1 + alpha));
    clickA1_ = float(-2 * std::cos(w) / (1 + alpha));
    clickA2_ = float((1 - alpha) / (1 + alpha));
  }
  for (const GuitarSpec::Mode& m : s_.body) {
    Body b;
    const double w0 = 2 * kPi * m.f, a = w0 / (2 * m.q), w = std::sqrt(w0 * w0 - a * a);
    const double g = std::exp(-a * dt_), c = std::cos(w * dt_), sn = std::sin(w * dt_);
    b.a11 = float(g * (c + a / w * sn));
    b.a12 = float(g * sn / w);
    b.a21 = float(-g * (w0 * w0 / w) * sn);
    b.a22 = float(g * (c - a / w * sn));
    b.invMass = float(1 / m.mass);
    b.rad = float(s_.radiate * m.rad);
    b.side = float(s_.radiate * m.side);
    b.along = float(m.along);
    b.mic = float(s_.mount * m.rad);  // (the pickup in the top: as the top moves under it there)
    body_.push_back(b);
  }
  // Each mode's axes and its split: fixed for the instrument (its saddle and nut).
  uint32_t rng = 4049;
  for (int i = 0; i < kStrings; ++i)
    for (int k = 0; k < kMaxModes; ++k) {
      str_[i].tilt[k] = float(s_.tilt + s_.tiltJitter * (urand(rng) - 0.5) * 2);
      str_[i].split[k] = float(s_.alongDetune * (1 + s_.alongJitter * (urand(rng) - 0.5) * 2));
      str_[i].ct[k] = std::cos(str_[i].tilt[k]);
      str_[i].st[k] = std::sin(str_[i].tilt[k]);
    }
  // The magnets' small-swing slope, to put the EMF in V per m/s.
  for (int p = 0; p < 2; ++p)
    for (int i = 0; i < kStrings; ++i) {
      const int sel = s_.select[p];
      if (sel < 0 || sel >= int(s_.pickups.size())) continue;
      const double g = s_.pickups[sel].gap[i], h = 1e-6;
      fieldSlope_[p][i] = (field(sel, g - h) - field(sel, g + h)) / (2 * h);
    }
  for (int i = 0; i < kStrings; ++i) tune(str_[i], i);
  if (s_.fretContact) {
    // The frets' crowns (research/slap_bass_1.md 5.2): on a plane, the relief a parabola between
    // the first fret and the last, each crown its own; the open string from the saddle to the nut
    // set by its height over the 17th fret and over the first.
    fretA_.assign(size_t(kStrings) * kMaxFrets * 2 * kMaxModes, 0.0f);
    fretK_.assign(size_t(kStrings) * 4 * kMaxModes, 0.0f);
    const int F = std::min(s_.frets, kMaxFrets);
    const double L = s_.scale, x1 = L * std::exp2(-1 / 12.0), xF = L * std::exp2(-F / 12.0);
    uint32_t r = 7177;
    for (int i = 0; i < kStrings; ++i) {
      for (int q = 1; q <= F; ++q) {
        const double x = L * std::exp2(-q / 12.0), u = (x1 - x) / (x1 - xF);
        crown_[i][q] = -s_.relief * 4 * u * (1 - u) + s_.fretJitter * 2 * (urand(r) - 0.5);
      }
      const int qa = std::min(17, F);
      const double a1 = x1 / L, aa = L * std::exp2(-qa / 12.0) / L;
      const double c1 = s_.nutClear + crown_[i][1], ca = s_.action[i] + crown_[i][qa];
      // Hs (1 - a) + Nn a = c at the two frets.
      const double det = (1 - a1) * aa - (1 - aa) * a1;
      saddleH_[i] = (c1 * aa - ca * a1) / det;
      placeFrets(i);
    }
  }
}

// The field on a cylindrical magnet's axis, `gap` from its face [NLSB18's form].
double Guitar::field(int p, double gap) const {
  (void)p;
  const double r = s_.magnetR, L = s_.magnetL, z = std::max(gap, 0.3e-3);
  return (z + L) / std::sqrt(r * r + (z + L) * (z + L)) - z / std::sqrt(r * r + z * z);
}

double Guitar::frequency(int s) const { return str_[s].f1; }

void Guitar::tune(Str& st, int index, bool place) {
  const GuitarString& d = s_.str[index];
  st.len = s_.scale * std::exp2(-st.fret / 12.0);
  st.f1 = d.f0 * s_.scale / st.len * std::exp2(st.bend / 1200);
  const double mu = d.tension / std::pow(2 * s_.scale * d.f0, 2);
  // (A fretted string ends on metal with a finger behind it: a little more
  // loss than on the nut.)
  const double end = st.fret > 0 ? 1.15 : 1.0;
  const double top = std::min(0.45 * fs_, s_.topHz > 0 ? s_.topHz : 1e9);
  int n = 0;
  for (int k = 1; k <= std::min(s_.modes, int(kMaxModes)); ++k) {
    const int i = k - 1;
    const double f = k * st.f1 * std::sqrt(1 + d.B * k * k);
    const bool on = f < top;
    if (on) n = k;
    for (int dir = 0; dir < 2; ++dir) {
      const double w = 2 * kPi * f * (dir ? 1 + (s_.alongJitter > 0 ? st.split[i] : s_.alongDetune) : 1.0);
      st.w[dir][i] = float(w);
      st.cs[dir][i] = float(std::cos(w * dt_));
      st.sn[dir][i] = float(std::sin(w * dt_));
    }
    // (By ear: the notes ring a little longer than the first numbers gave,
    // the first and second overtones most.)
    const double loss = (d.loss0 + d.loss1 * std::pow(s_.lossByFrequency ? k * st.f1 / d.f0 : double(k), s_.lossExp[index])) * end * s_.ring *
                        (k == 2 || k == 3 ? s_.ringOvertones : k == 4 || k == 5 ? s_.ringOvertones45 : 1.0);
    // (Along the top the string's own loss is the same; it only keeps more
    // of its sound from the body.)
    // (The palm: in both directions alike. Soft flesh takes only so much: what it takes of the
    // higher modes saturates at palmSat times the fundamental's. Against FluidR3's recorded palm-
    // muted guitar: the fundamental to -30 dB in 0.3-0.5 s, the 4th harmonic in 30-270 ms.)
    double palm = 0;
    if (st.muteDepth > 0) {
      const double r = underPalm(k, st.muteAt, st.len) / underPalm(1, st.muteAt, s_.scale);
      palm = st.muteDepth * r / (1 + r / s_.palmSat);
    }
    // (A harmonic's fingertip at 1/n of the sounding length: each mode by its shape squared
    // there, over the fingertip's width.)
    if (st.touchN > 1) palm += st.touchRate * underFinger(k, st.len / st.touchN + st.touchOff, st.touchWidth, st.len);
    st.decay[0][i] = on ? float(std::exp(-(loss * s_.towardsLoss + palm) * dt_)) : 0.0f;
    st.decay[1][i] = on ? float(std::exp(-(loss + palm) * dt_)) : 0.0f;
    st.slope[i] = float(d.tension * k * kPi / st.len * ((k & 1) ? -1.0 : 1.0));   // its pull on the bridge per unit swing (N/m)
    st.bridgeIn[i] = float(2.0 / (k * kPi) * ((k & 1) ? -1.0 : 1.0));              // the bridge's acceleration into it
  }
  // (Modes above the last that sounds: still.)
  for (int i = n; i < kMaxModes; ++i)
    for (int dir = 0; dir < 2; ++dir) st.q[dir][i] = st.v[dir][i] = 0;
  st.n = std::max(n, 1);
  st.invMass = float(2 / (mu * st.len));
  if (!place || !electric()) return;
  // The pickups' ears: each mode's shape where the pickup is, over the field's width (a
  // Gaussian), a humbucker's two coils; what of each direction lies towards the magnet (and
  // the little it hears across it).
  for (int p = 0; p < 2; ++p) {
    const int sel = s_.select[p];
    if (sel < 0 || sel >= int(s_.pickups.size())) { std::fill(st.ear[p][0], st.ear[p][0] + kMaxModes, 0.0f); std::fill(st.ear[p][1], st.ear[p][1] + kMaxModes, 0.0f); continue; }
    const GuitarSpec::Pickup& pu = s_.pickups[sel];
    const double x = std::clamp(1 - pu.at[index] / st.len, 0.02, 0.98);
    for (int k = 1; k <= kMaxModes; ++k) {
      const int i = k - 1;
      const double a = k * kPi * pu.width / st.len;
      const double e = i < st.n ? std::sin(k * kPi * x) * std::exp(-0.5 * a * a) * std::cos(k * kPi * pu.apart / (2 * st.len)) : 0.0;
      const double c = std::cos(st.tilt[i]), sn = std::sin(st.tilt[i]);
      st.ear[p][0][i] = float(e * (c + s_.sideways * sn));
      st.ear[p][1][i] = float(e * (-sn + s_.sideways * c));
    }
    // (What it hears now, with these ears: no jump in the flux at a new fret.)
    double y = 0;
    for (int i = 0; i < st.n; ++i) y += st.ear[p][0][i] * st.q[0][i] + st.ear[p][1][i] * st.q[1][i];
    st.flux[p] = field(sel, pu.gap[index] - y);
  }
}

void Guitar::shapeAt(Str& st, float* shape, double fromBridge, double width, double apart) {
  // (apart: the string held at two places that far from each other, each `width` wide.)
  const double x = std::clamp(1 - fromBridge / st.len, 0.02, 0.98);
  for (int k = 1; k <= kMaxModes; ++k) {
    const double a = k * kPi * width / (2 * st.len);
    shape[k - 1] = float(std::sin(k * kPi * x) * (a > 1e-6 ? std::sin(a) / a : 1.0) * std::cos(k * kPi * apart / (2 * st.len)));
  }
}

// A stroke's force into each mode's two directions: the shape, split by the angle against each
// mode's own axes (rad from the top's plane; the axes tilted: a push along the top reaches the
// direction towards it too).
void Guitar::push(Str& st, const float* shape, float (*to)[kMaxModes], double angle) {
  for (int i = 0; i < kMaxModes; ++i) {
    const double a = angle + st.tilt[i];
    to[0][i] = float(shape[i] * std::sin(a));
    to[1][i] = float(shape[i] * std::cos(a));
  }
}

void Guitar::fret(int s, int fret) {
  Str& st = str_[s];
  fret = std::clamp(fret, 0, s_.frets);
  if (fret == st.fret) return;
  st.fret = fret;
  st.bend = 0;
  tune(st, s);
  if (s_.fretContact) placeFrets(s);
}

void Guitar::bend(int s, double cents) {
  Str& st = str_[s];
  if (std::fabs(cents - st.bend) < 0.05) return;
  st.bend = cents;
  tune(st, s, false);
}

void Guitar::pluck(int s, double depth, double fromBridge, double angle, double nail, int strum, double clack) {
  Str& st = str_[s];
  st.plant = strum ? 0.0015 : kPlant_;
  st.flesh = strum == 2 ? 0.0038 : strum ? 0.0022 : kFlesh_;
  st.nailSlip = strum == 2 ? 0.0014 : strum ? 0.0008 : kNail_;
  // (What the stroke takes of what the string still sounded: a picking
  // finger stops it; the fingers' backs going down hardly touch it; the pad
  // coming up lies on it a moment and eats some.)
  st.hold = strum == 2 ? 200 : strum ? 25 : 300;
  st.wFlesh = strum == 2 ? 0.016 : strum ? 0.011 : 0.009;   // the pad going up; the fingers' backs going down
  st.wNail = strum == 2 ? 0.006 : strum ? 0.003 : 0.0015;   // the nail's face, not its edge
  st.fromBridge = std::clamp(fromBridge, 0.02, 0.5 * st.len);
  // The force that holds the string that far aside there.
  st.force = s_.str[s].tension * depth * st.len / (st.fromBridge * (st.len - st.fromBridge));
  st.nail = std::clamp(nail, 0.0, 1.0);
  // (A strumming hand never knocks. A nail is curved: picking, the string
  // meets its hollow side and its edge, stiff as an arch; strumming, the
  // string meets its rounded back, which flexes and slides.)
  st.clack = strum ? 0 : std::clamp(clack, 0.0, 1.0);
  float shape[kMaxModes];
  shapeAt(st, shape, st.fromBridge, st.wFlesh);
  push(st, shape, st.pushF, angle);
  // Picking, the string lies in the nail's hollow side: the nail's curve
  // holds it sharply at two places a few millimetres apart. Strumming, it
  // meets the nail's rounded back: one broad, soft place.
  shapeAt(st, shape, st.fromBridge, st.wNail, strum ? 0 : 0.003);
  push(st, shape, st.pushN, angle);
  st.clock = 0;
  st.dampRate = 0;
  st.active = true;
  asleep_ = false;
  quiet_ = 0;
}

void Guitar::pick(int s, double depth, double fromBridge, double angle, double rise, double width) {
  Str& st = str_[s];
  // The pick's push: one narrow edge (0.8 mm) from first touch to the slip, no flesh; it hardly
  // stops the string as it passes (research/steel_guitar.md 4.1).
  st.plant = std::clamp(rise, 0.0003, 0.004);
  st.flesh = 0;
  st.nailSlip = 0.00015;
  st.hold = 40;
  st.fromBridge = std::clamp(fromBridge, 0.02, 0.5 * st.len);
  st.force = s_.str[s].tension * depth * st.len / (st.fromBridge * (st.len - st.fromBridge));
  st.nail = 1;
  st.clack = 0;
  float shape[kMaxModes];
  shapeAt(st, shape, st.fromBridge, width);
  push(st, shape, st.pushF, angle);
  if (s_.click > 0) st.clickT = 0;
  st.fbMode[0] = st.fbMode[1] = -1;
  st.holdN = 0;
  std::copy(&st.pushF[0][0], &st.pushF[0][0] + 2 * kMaxModes, &st.pushN[0][0]);
  st.clock = 0;
  st.dampRate = 0;
  st.active = true;
  asleep_ = false;
  quiet_ = 0;
}

double Guitar::underPalm(double k, double at, double len) {
  // (Its mean over the hand's 12 mm, eight points.)
  double m = 0;
  for (int j = 0; j < 8; ++j) {
    const double y = std::max(0.0, at + 0.012 * ((j + 0.5) / 8 - 0.5));
    const double sn = std::sin(k * kPi * y / len);
    m += sn * sn / 8;
  }
  return m;
}

double Guitar::underFinger(double k, double at, double width, double len) {
  // (Its mean over the fingertip, eight points.)
  double m = 0;
  for (int j = 0; j < 8; ++j) {
    const double sn = std::sin(k * kPi * (at + width * ((j + 0.5) / 8 - 0.5)) / len);
    m += sn * sn / 8;
  }
  return m;
}

void Guitar::touch(int s, int n, double secs, double rate, double width, double off) {
  Str& st = str_[s];
  st.touchN = n > 1 && secs > 0 ? n : 0;
  st.touchLeft = secs;
  st.touchRate = rate;
  st.touchWidth = width;
  st.touchOff = off;
  tune(st, s, false);
}

void Guitar::mute(int s, double depth, double at) {
  Str& st = str_[s];
  depth = std::max(0.0, depth);
  if (depth == st.muteDepth && at == st.muteAt) return;
  st.muteDepth = depth;
  st.muteAt = at;
  tune(st, s, false);
}

double Guitar::modeAmp(int s, int k) const {
  const Str& st = str_[s];
  const int i = k - 1;
  if (i < 0 || i >= st.n) return 0;
  double e = 0;
  for (int d = 0; d < 2; ++d) e += double(st.v[d][i]) * st.v[d][i] + double(st.w[d][i] * st.q[d][i]) * (st.w[d][i] * st.q[d][i]);
  return std::sqrt(e);
}

void Guitar::sustain(int s, int k, int ref, double level, double rate, int slot) {
  Str& st = str_[s];
  slot = std::clamp(slot, 0, 1);
  if (k <= 0 || k > st.n) { st.fbMode[slot] = -1; return; }
  st.fbMode[slot] = k - 1;
  st.fbTarget[slot] = level * modeAmp(s, ref);
  st.fbRate[slot] = rate;
}

void Guitar::hold(int s, double hz, double level, double rate) {
  Str& st = str_[s];
  st.holdN = 0;
  if (level <= 0) return;
  for (int i = 0; i < st.n && st.w[0][i] < 2 * M_PI * hz; ++i) {
    st.holdTarget[i] = float(level * modeAmp(s, i + 1));
    st.holdOwn[i] = float(-std::log(std::max(1e-9f, std::min(st.decay[0][i], st.decay[1][i]))) * fs_);
    st.holdN = i + 1;
  }
  st.holdRate = rate;
}

void Guitar::pluckPad(int s, double depth, double fromBridge, double angle, double width, double rise, double slip) {
  Str& st = str_[s];
  st.plant = std::clamp(rise, 0.0005, 0.01);
  st.flesh = std::clamp(slip, 0.0002, 0.006);
  st.nailSlip = 0.0001;
  st.hold = 250;  // (the finger takes the string: what it sounded dies under it)
  st.wFlesh = width;
  st.fromBridge = std::clamp(fromBridge, 0.02, 0.5 * st.len);
  st.force = s_.str[s].tension * depth * st.len / (st.fromBridge * (st.len - st.fromBridge));
  st.nail = 0;
  st.clack = 0;
  float shape[kMaxModes];
  shapeAt(st, shape, st.fromBridge, width);
  push(st, shape, st.pushF, angle);
  std::copy(&st.pushF[0][0], &st.pushF[0][0] + 2 * kMaxModes, &st.pushN[0][0]);
  st.fbMode[0] = st.fbMode[1] = -1;
  st.holdN = 0;
  st.clock = 0;
  st.dampRate = 0;
  st.active = true;
  asleep_ = false;
  quiet_ = 0;
}

void Guitar::pickContact(int s, double depth, double fromBridge, double angle, double pushT, double slip, double stiffness,
                         double bendMax, double width) {
  Str& st = str_[s];
  // The edge (0.8 mm) where it meets the string: the direction it pushes in is also the one it
  // feels the string's place in (a spring between the two: passive, whatever its numbers).
  st.fromBridge = std::clamp(fromBridge, 0.02, 0.5 * st.len);
  float shape[kMaxModes];
  shapeAt(st, shape, st.fromBridge, width);
  push(st, shape, st.pushF, angle);
  st.pkDepth = std::clamp(depth, 0.0, 0.01);
  st.pkPush = std::clamp(pushT, 0.0005, 0.03);
  st.pkSlipT = std::clamp(slip, 0.00005, 0.002);
  st.pkK = std::clamp(stiffness, 500.0, 50000.0);
  st.pkRelease = bendMax > 0 ? st.pkK * bendMax : 1e9;
  st.pkT = 0;
  st.pkF = 0;
  st.pkSlip = false;
  st.pkOn = true;
  st.clock = 1e9;  // (no finger)
  st.fbMode[0] = st.fbMode[1] = -1;
  st.holdN = 0;
  st.dampRate = 0;
  st.active = true;
  asleep_ = false;
  quiet_ = 0;
}

float Guitar::contact(Str& st, float& hold) {
  const double t = st.pkT;
  st.pkT += dt_;
  if (!st.pkSlip) {
    // The hand's place for the tip; the string's under the edge, along the stroke; the tip's
    // spring between them, pushing only.
    const double u = std::min(t / st.pkPush, 1.0);
    const double yp = st.pkDepth * (0.5 - 0.5 * std::cos(kPi * u));
    double ys = 0;
    for (int i = 0; i < st.n; ++i) ys += double(st.pushF[0][i]) * st.q[0][i] + double(st.pushF[1][i]) * st.q[1][i];
    const double f = std::max(0.0, st.pkK * (yp - ys));
    // (While it lies on the edge, the string's other motion is a little held: as pick()'s.)
    if (f > 0) hold = float(std::exp(-40 * dt_));
    if (u >= 1 || f >= st.pkRelease) {
      st.pkSlip = true;
      st.pkF = f;
      st.pkT = 0;
    }
    return float(f);
  }
  const double u = t / st.pkSlipT;
  if (u >= 1) {
    st.pkOn = false;
    return 0;
  }
  return float(st.pkF * (0.5 + 0.5 * std::cos(kPi * u)));
}

void Guitar::pop(int s, double depth, double fromBridge, double angle, double width, double rise, double slip) {
  pluckPad(s, depth, fromBridge, angle, width, rise, slip);
  str_[s].plant = std::clamp(rise, 0.0005, 0.03);
}

void Guitar::thump(int s, double speed, double fromBridge, double angle, double width, double mass, double stiffness, double lift,
                   double gap, double damping) {
  Str& st = str_[s];
  st.fromBridge = std::clamp(fromBridge, 0.02, 0.5 * st.len);
  float shape[kMaxModes];
  shapeAt(st, shape, st.fromBridge, width);
  push(st, shape, st.pushF, angle);
  std::copy(&st.pushF[0][0], &st.pushF[0][0] + 2 * kMaxModes, &st.pushN[0][0]);
  // (It starts `gap` short of where the string is now, along the stroke.)
  double ys = 0;
  for (int i = 0; i < st.n; ++i) ys += double(st.pushF[0][i]) * st.q[0][i] + double(st.pushF[1][i]) * st.q[1][i];
  st.thZ = ys - gap;
  st.thV = std::max(speed, 0.05);
  st.thT = 0;
  st.thTouch = -1;
  st.thM = std::max(mass, 1e-3);
  st.thK = std::max(stiffness, 100.0);
  st.thLift = lift;
  st.thC = std::max(damping, 0.0);
  st.thYs = ys;
  st.thOn = true;
  st.pkOn = false;
  st.clock = 1e9;
  st.fbMode[0] = st.fbMode[1] = -1;
  st.holdN = 0;
  st.dampRate = 0;
  st.active = true;
  asleep_ = false;
  quiet_ = 0;
}

float Guitar::thumbForce(Str& st, float& hold) {
  // The flesh between the thumb and the string under it (along the stroke): it only pushes. The
  // thumb a mass, explicit (the flesh against the string under 15 mm rings near 1-2 kHz: well
  // inside the step).
  double ys = 0;
  for (int i = 0; i < st.n; ++i) ys += double(st.pushF[0][i]) * st.q[0][i] + double(st.pushF[1][i]) * st.q[1][i];
  const double pen = st.thZ - ys;
  // (The flesh's loss, Hunt and Crossley's: the spring's force the more as it is pressed in
  // faster, the less as it comes out; nothing at first touch, so no knock of its own.)
  const double f = pen > 0 ? st.thK * pen * std::max(0.0, 1 + st.thC * (st.thV - (ys - st.thYs) * fs_)) : 0.0;
  st.thYs = ys;
  if (f > 0) {
    if (st.thTouch < 0) st.thTouch = st.thT;
    hold = float(std::exp(-150 * dt_));
  }
  const double lift = st.thTouch >= 0 && st.thT - st.thTouch > 0.002 ? st.thLift : 0.0;
  st.thV += dt_ * (-f / st.thM - lift);
  st.thZ += dt_ * st.thV;
  st.thT += dt_;
  if ((st.thTouch >= 0 && pen <= 0 && st.thV < 0) || st.thT > 0.1) st.thOn = false;
  return float(f);
}

int Guitar::fretHits(int s, uint32_t* which) {
  Str& st = str_[s];
  const int n = st.fretHits;
  if (which) *which = st.fretWhich;
  st.fretHits = 0;
  st.fretWhich = 0;
  return n;
}

double Guitar::energy(int s) const {
  const Str& st = str_[s];
  double e = 0;
  for (int d = 0; d < 2; ++d)
    for (int i = 0; i < st.n; ++i) e += double(st.w[d][i]) * st.w[d][i] * st.q[d][i] * st.q[d][i] + double(st.v[d][i]) * st.v[d][i];
  return 0.5 * e / std::max(1e-9f, st.invMass);
}

void Guitar::placeFrets(int s) {
  // The frets over the sounding string (above the stopping one), their clearance under the
  // string at rest (from the saddle to the stopping fret's crown, or to the nut), each mode's
  // shape there in the string's two askew directions (towards the board: as push() at pi/2).
  Str& st = str_[s];
  const int F = std::min(s_.frets, kMaxFrets);
  const double L = s_.scale, Hs = saddleH_[s];
  const double xf = st.fret > 0 ? L * std::exp2(-st.fret / 12.0) : L;
  // (The nut's height: from the open line through the first fret.)
  const double x1 = L * std::exp2(-1 / 12.0);
  const double nut = (s_.nutClear + crown_[s][1] - Hs * (1 - x1 / L)) / (x1 / L);
  const double hf = st.fret > 0 ? crown_[s][st.fret] : nut;
  st.fretFirst = st.fret + 1;
  st.fretCount = std::max(0, F - st.fret);
  float* K = &fretK_[size_t(s) * 4 * kMaxModes];
  for (int i = 0; i < kMaxModes; ++i) {
    const bool on = i < st.n;
    K[i] = on ? float(st.invMass * st.sn[0][i] / st.w[0][i]) : 0.0f;
    K[kMaxModes + i] = on ? float(st.invMass * st.sn[1][i] / st.w[1][i]) : 0.0f;
    K[2 * kMaxModes + i] = on ? float(st.invMass * st.cs[0][i]) : 0.0f;
    K[3 * kMaxModes + i] = on ? float(st.invMass * st.cs[1][i]) : 0.0f;
  }
  for (int j = 0; j < st.fretCount; ++j) {
    const int q = st.fretFirst + j;
    const double x = L * std::exp2(-q / 12.0);
    const double y = Hs + (hf - Hs) * x / xf;
    fretClear_[s][j] = float(std::max(y - crown_[s][q], 2e-5));
    float* a0 = &fretA_[((size_t(s) * kMaxFrets + j) * 2) * kMaxModes];
    float* a1 = a0 + kMaxModes;
    const double u = 1 - x / st.len;
    double g = 0;
    for (int i = 0; i < kMaxModes; ++i) {
      // (a raised cosine of width w: its transform 1 / (1 - (a/pi)^2) sinc-like, a = k pi w / (2 len))
      const double a = (i + 1) * kPi * s_.fretWidth / (2 * st.len);
      const double sm = a < 1e-9 ? 1.0 : std::fabs(a - kPi) < 1e-6 ? 0.5 : std::sin(a) / a / (1 - a * a / (kPi * kPi));
      const double ph = i < st.n ? std::sin((i + 1) * kPi * u) * sm : 0.0;
      a0[i] = float(ph * st.ct[i]);
      a1[i] = float(-ph * st.st[i]);
      g += double(a0[i]) * a0[i] * K[i] + double(a1[i]) * a1[i] * K[kMaxModes + i];
    }
    fretG_[s][j] = float(g);
  }
  fretLive_[s] = ~0u;
  fretFull_[s] = true;
}

void Guitar::fretStep(Str& st, int s, float& y0, float& y1) {
  const int n = st.n;
  ++fretTick_[s];
  // The gate: while a stroke is on, or soon after a contact (which moves the energy between the
  // modes), every fret every sample; otherwise only the frets the string could reach with each
  // mode's swing (which, free, only falls), found again every 16 samples.
  const bool stroke = st.clock < 1e8 || st.pkOn || st.thOn;
  constexpr int kLo = 32;
  if (stroke) {
    st.fretIdle = 0;
    fretLive_[s] = ~0u;
    fretFull_[s] = true;
  } else if ((fretTick_[s] & 15) == 0) {
    uint32_t live = ~0u;
    fretFull_[s] = st.fretIdle <= 20 * dt_;
    if (!fretFull_[s]) {
      alignas(64) float m0[kMaxModes], m1[kMaxModes];
      for (int i = 0; i < n; ++i) {
        m0[i] = std::sqrt(st.q[0][i] * st.q[0][i] + st.v[0][i] * st.v[0][i] / (st.w[0][i] * st.w[0][i]));
        m1[i] = std::sqrt(st.q[1][i] * st.q[1][i] + st.v[1][i] * st.v[1][i] / (st.w[1][i] * st.w[1][i]));
      }
      live = 0;
      for (int j = 0; j < st.fretCount; ++j) {
        const float* a0 = &fretA_[((size_t(s) * kMaxFrets + j) * 2) * kMaxModes];
        const float* a1 = a0 + kMaxModes;
        float b = 0, t = 0;
        const int lo = std::min(n, kLo);
        for (int i = 0; i < lo; ++i) b += std::fabs(a0[i]) * m0[i] + std::fabs(a1[i]) * m1[i];
        for (int i = lo; i < n; ++i) t += std::fabs(a0[i]) * m0[i] + std::fabs(a1[i]) * m1[i];
        fretTail_[s][j] = 1.02f * t;
        if (1.02f * (b + t) > fretClear_[s][j]) live |= 1u << j;
      }
    }
    fretLive_[s] = live;
  }
  st.fretIdle += dt_;
  if (!fretLive_[s]) return;
  const float* K = &fretK_[size_t(s) * 4 * kMaxModes];
  const float *kq0 = K, *kq1 = K + kMaxModes, *kv0 = K + 2 * kMaxModes, *kv1 = K + 3 * kMaxModes;
  const bool elec = electric();
  for (int sweep = 0; sweep < 2; ++sweep) {
    bool hit = false;
    for (int j = 0; j < st.fretCount; ++j) {
      if (!(fretLive_[s] >> j & 1)) continue;
      const float* a0 = &fretA_[((size_t(s) * kMaxFrets + j) * 2) * kMaxModes];
      const float* a1 = a0 + kMaxModes;
      float d = 0;
      if (!fretFull_[s] && sweep == 0 && n > kLo) {
        // (The low modes first; the rest only if they could matter.)
        for (int i = 0; i < kLo; ++i) d += a0[i] * st.q[0][i] + a1[i] * st.q[1][i];
        if (d + fretTail_[s][j] <= fretClear_[s][j]) continue;
        for (int i = kLo; i < n; ++i) d += a0[i] * st.q[0][i] + a1[i] * st.q[1][i];
      } else {
        for (int i = 0; i < n; ++i) d += a0[i] * st.q[0][i] + a1[i] * st.q[1][i];
      }
      const float p = d - fretClear_[s][j];
      if (p <= 0) continue;
      // The crown turns it back: its speed into the crown (u, along the crown's shape a in the
      // modes) reflected, fretBounce of it given back; the modes' masses alike, so a reflection
      // of v along a keeps the kinetic energy (1) or takes what goes (< 1). What has gone deeper
      // than 0.01 mm is put back on the crown, by the least stretch (the shift along a / w^2:
      // mostly the low modes; it only lowers the potential energy, the string being pressed
      // into the crown).
      float u = 0, aa = 0, ab = 0;
      for (int i = 0; i < n; ++i) {
        u += a0[i] * st.v[0][i] + a1[i] * st.v[1][i];
        aa += a0[i] * a0[i] + a1[i] * a1[i];
        ab += a0[i] * a0[i] / (st.w[0][i] * st.w[0][i]) + a1[i] * a1[i] / (st.w[1][i] * st.w[1][i]);
      }
      const float dv = u > 0 && aa > 0 ? -(1 + float(s_.fretBounce)) * u / aa : 0.0f;
      const float dx = p > 1e-5f && ab > 0 ? -(p - 1e-5f) / ab : 0.0f;
      float dy0 = 0, dy1 = 0;
      for (int i = 0; i < n; ++i) {
        const float dq0 = dx * a0[i] / (st.w[0][i] * st.w[0][i]), dq1 = dx * a1[i] / (st.w[1][i] * st.w[1][i]);
        st.q[0][i] += dq0;
        st.q[1][i] += dq1;
        st.v[0][i] += dv * a0[i];
        st.v[1][i] += dv * a1[i];
        if (elec) {
          dy0 += st.ear[0][0][i] * dq0 + st.ear[0][1][i] * dq1;
          dy1 += st.ear[1][0][i] * dq0 + st.ear[1][1][i] * dq1;
        }
      }
      y0 += dy0;
      y1 += dy1;
      hit = true;
      if (sweep == 0) {
        ++st.fretHits;
        st.fretWhich |= 1u << std::min(st.fretFirst + j, 31);
      }
    }
    if (!hit) break;
    st.fretIdle = 0;
    fretFull_[s] = true;
  }
}

void Guitar::damp(int s, double rate) {
  if (rate > 0) { str_[s].fbMode[0] = str_[s].fbMode[1] = -1; str_[s].holdN = 0; } str_[s].dampRate = rate; }

void Guitar::reset() {
  for (Str& st : str_) {
    for (int d = 0; d < 2; ++d) { std::fill(st.q[d], st.q[d] + kMaxModes, 0.0f); std::fill(st.v[d], st.v[d] + kMaxModes, 0.0f); }
    st.clock = 1e9;
    st.pkOn = false;
    st.active = false;
  }
  for (int i = 0; i < kStrings; ++i)
    for (int p = 0; p < 2; ++p)
      if (s_.select[p] >= 0 && s_.select[p] < int(s_.pickups.size())) str_[i].flux[p] = field(s_.select[p], s_.pickups[s_.select[p]].gap[i]);
  for (Body& b : body_) b.x = b.v = 0;
  bridgeV_ = bridgeVz_ = 0;
  side_ = 0;
  asleep_ = true;
}

// (kArch: the archtop's path, its pickup moving with the top and its sound kept; the other
// guitars run the same code without it.)
template <bool kArch>
float Guitar::run() {
  if (asleep_) {
    side_ = 0;
    if constexpr (kArch) acoustic_ = 0;
    return 0;
  }
  const float dt = float(dt_);
  // The strings' pull on the bridge: towards the top, and along it.
  float fy = 0, fz = 0;
  for (Str& st : str_) {
    if (!st.active) continue;
    float a = 0, b = 0;
    // (Each mode's two axes askew: their swings' parts towards the top and along it.)
    for (int i = 0; i < st.n; ++i) {
      a += st.slope[i] * (st.ct[i] * st.q[0][i] - st.st[i] * st.q[1][i]);
      b += st.slope[i] * (st.st[i] * st.q[0][i] + st.ct[i] * st.q[1][i]);
    }
    fy -= a;
    fz -= b;
  }
  // The body under the bridge: each resonance takes its own mix of the two
  // pulls, and moves the bridge its own way.
  float bv = 0, bz = 0, out = 0, sd = 0;
  for (Body& b : body_) {
    const float v = b.v + dt * (fy + b.along * fz) * b.invMass, x = b.x;
    b.x = b.a11 * x + b.a12 * v;
    const float vn = b.a21 * x + b.a22 * v;
    out += b.rad * (vn - b.v);
    sd += b.side * (vn - b.v);
    b.v = vn;
    bv += vn;
    bz += b.along * vn;
  }
  const float ab = (bv - bridgeV_) * float(fs_), az = (bz - bridgeVz_) * float(fs_);  // the bridge's acceleration, both ways
  bridgeV_ = bv;
  bridgeVz_ = bz;
  // The strings.
  bool any = false;
  const bool elec = electric();
  double emf[2] = {};
  // A pickup mounted in the top (the archtop): where the top has carried it (m, towards the
  // strings), the same under all six; a string at rest is heard coming nearer too.
  const bool mounted = kArch && elec && s_.mount != 0;
  double zMic = 0;
  if (mounted)
    for (const Body& b : body_) zMic += double(b.mic) * b.x;
  for (Str& st : str_) {
    if (!st.active) {
      if (mounted) {
        const int idx = int(&st - str_);
        for (int p = 0; p < 2; ++p) {
          const int sel = s_.select[p];
          if (sel < 0) continue;
          const double f = field(sel, s_.pickups[sel].gap[idx] - zMic);
          emf[p] += s_.pickups[sel].volts * s_.steel[idx] * (f - st.flux[p]) * fs_ / fieldSlope_[p][idx];
          st.flux[p] = f;
        }
      }
      continue;
    }
    // The finger: it takes the string (what the string still sounded dies
    // under it), pulls, and lets go: off the flesh, then off the nail.
    // (fF: what the flesh holds of the string; fN: what the nail's edge holds.)
    float fF = 0, fN = 0, hold = 1;
    if (st.clock < 1e8) {
      const double t = st.clock;
      st.clock += dt_;
      const double fleshEnd = st.plant + st.flesh * (1 - 0.6 * st.nail), nailEnd = fleshEnd + st.nailSlip;
      if (t < st.plant) {
        fF = float(st.force * (0.5 - 0.5 * std::cos(kPi * t / st.plant)));
        hold = float(std::exp(-st.hold * dt_));
        // The nail's knock as it meets the string: against the pull, 0.6 ms.
        if (st.clack > 0 && t < 0.0006) fN = float(-1.6 * st.clack * st.force * std::sin(kPi * t / 0.0006));
      } else if (t < fleshEnd) {
        // The flesh gives way: the string comes to lie on the nail's edge alone.
        const double u = 0.5 - 0.5 * std::cos(kPi * (t - st.plant) / (fleshEnd - st.plant));
        fF = float(st.force * (1 - st.nail) * (1 - u) + st.force * st.nail * (1 - u));
        fN = float(st.force * st.nail * u);
        hold = float(std::exp(-0.4 * st.hold * (1 - u) * dt_));
      } else if (t < nailEnd) {
        const double u = (t - fleshEnd) / st.nailSlip;
        fN = float(st.force * st.nail * (0.5 + 0.5 * std::cos(kPi * u)));
      } else {
        st.clock = 1e9;
      }
    }
    if (st.touchN > 1 && (st.touchLeft -= dt_) <= 0) {
      // The harmonic's fingertip lifts: its hold halved every 6 ms (a sudden release would set
      // free what the bridge forces the touched modes to).
      st.touchRate *= 0.5;
      st.touchLeft = 0.006;
      if (st.touchRate < 2) st.touchN = 0;
      tune(st, int(&st - str_), false);
    }
    if (st.pkOn) fF = contact(st, hold);  // (a held pick: its force through pushF)
    if (st.thOn) fF = thumbForce(st, hold);  // (a thumb: the same)
    const float im = st.invMass * dt;
    const float lay = st.dampRate > 0 ? float(std::exp(-st.dampRate * dt_)) : 1.0f;
    const float keep = hold * lay;
    float e = 0, y0 = 0, y1 = 0;
    auto modes = [&](auto ears) {
      constexpr bool kEars = decltype(ears)::value;
      const int n = st.n;
      for (int i = 0; i < n; ++i) {
        // Towards the top.
        const float bIn = st.bridgeIn[i] * dt;
        float v = st.v[0][i] + im * (fF * st.pushF[0][i] + fN * st.pushN[0][i]) + bIn * (st.ct[i] * ab + st.st[i] * az), q = st.q[0][i];
        float qn = st.cs[0][i] * q + st.sn[0][i] / st.w[0][i] * v, vn = -st.sn[0][i] * st.w[0][i] * q + st.cs[0][i] * v;
        const float q0 = qn * st.decay[0][i] * keep;
        st.q[0][i] = q0;
        st.v[0][i] = vn * st.decay[0][i] * keep;
        // Along it.
        v = st.v[1][i] + im * (fF * st.pushF[1][i] + fN * st.pushN[1][i]) + bIn * (st.ct[i] * az - st.st[i] * ab);
        q = st.q[1][i];
        qn = st.cs[1][i] * q + st.sn[1][i] / st.w[1][i] * v;
        vn = -st.sn[1][i] * st.w[1][i] * q + st.cs[1][i] * v;
        const float q1 = qn * st.decay[1][i] * keep;
        st.q[1][i] = q1;
        st.v[1][i] = vn * st.decay[1][i] * keep;
        e += st.v[0][i] * st.v[0][i] + st.v[1][i] * st.v[1][i];
        if constexpr (kEars) {
          y0 += st.ear[0][0][i] * q0 + st.ear[0][1][i] * q1;
          y1 += st.ear[1][0][i] * q0 + st.ear[1][1][i] * q1;
        }
      }
    };
    if (elec) modes(std::true_type{}); else modes(std::false_type{});
    if (s_.fretContact) fretStep(st, int(&st - str_), y0, y1);
    for (int i = 0; i < st.holdN; ++i) {
      // Feedback: the low modes held where they were.
      if (i == st.fbMode[0] || i == st.fbMode[1]) continue;
      double a = 0;
      for (int d = 0; d < 2; ++d) a += double(st.v[d][i]) * st.v[d][i] + double(st.w[d][i] * st.q[d][i]) * (st.w[d][i] * st.q[d][i]);
      const double g = st.holdOwn[i] + st.holdRate * std::clamp(1 - std::sqrt(a) / std::max(double(st.holdTarget[i]), 1e-12), -1.0, 1.0);
      const float f = float(1 + g * dt_);
      for (int d = 0; d < 2; ++d) { st.q[d][i] *= f; st.v[d][i] *= f; }
    }
    for (int slot = 0; slot < 2; ++slot) {
      // Feedback: a held mode given back what it loses, towards its level (and no further).
      const int i = st.fbMode[slot];
      if (i < 0 || (slot == 1 && i == st.fbMode[0])) continue;
      double a = 0;
      for (int d = 0; d < 2; ++d) a += double(st.v[d][i]) * st.v[d][i] + double(st.w[d][i] * st.q[d][i]) * (st.w[d][i] * st.q[d][i]);
      a = std::sqrt(a);
      // (Its own loss made good, and more or less as it lies under or over its level.)
      const double own = -std::log(std::max(1e-9f, std::min(st.decay[0][i], st.decay[1][i]))) * fs_;
      const double g = own + st.fbRate[slot] * std::clamp(1 - a / std::max(st.fbTarget[slot], 1e-12), -1.0, 1.0);
      const float f = float(1 + g * dt_);
      for (int d = 0; d < 2; ++d) { st.q[d][i] *= f; st.v[d][i] *= f; }
    }
    if (elec) {
      // The pickups: the flux through each coil follows the string's distance from its magnet;
      // its change is the EMF.
      const int idx = int(&st - str_);
      const float y[2] = {y0, y1};
      for (int p = 0; p < 2; ++p) {
        const int sel = s_.select[p];
        if (sel < 0) continue;
        const GuitarSpec::Pickup& pu = s_.pickups[sel];
        const double f = mounted ? field(sel, pu.gap[idx] - y[p] - zMic) : field(sel, pu.gap[idx] - y[p]);
        emf[p] += pu.volts * s_.steel[idx] * (f - st.flux[p]) * fs_ / fieldSlope_[p][idx];
        st.flux[p] = f;
      }
    }
    if (e < 1e-14f && st.clock > 1e8 && !st.pkOn && !st.thOn && std::fabs(ab) <= 1e-4f) {
      st.active = false;
      for (int d = 0; d < 2; ++d) { std::fill(st.q[d], st.q[d] + kMaxModes, 0.0f); std::fill(st.v[d], st.v[d] + kMaxModes, 0.0f); }
      if (elec)
        for (int p = 0; p < 2; ++p)
          if (s_.select[p] >= 0) st.flux[p] = field(s_.select[p], s_.pickups[s_.select[p]].gap[&st - str_]);
    } else {
      any = true;
    }
  }
  // The other strings ring along: woken by the bridge.
  if (std::fabs(ab) > 1e-4f)
    for (int i = 0; i < s_.strings; ++i) str_[i].active = true;
  out *= float(fs_);
  side_ = sd * float(fs_);
  if (s_.click > 0) {
    // The pick's own sound: a burst of noise as the string leaves the edge, 0.6 ms on a plain
    // string; on a wound one the edge rattles over the wrap first (at ~3 kHz, 1.5 ms).
    float drive = 0;
    for (int i = 0; i < s_.strings; ++i) {
      Str& st = str_[i];
      if (st.clickT > 1e8) continue;
      const bool wound = i < s_.wound;
      const double t = st.clickT - st.plant + (wound ? 0.0015 : 0.0003), len = wound ? 0.0023 : 0.0009;
      st.clickT += dt_;
      if (t < 0) continue;
      if (t >= len) { st.clickT = 1e9; continue; }
      clickRng_ = clickRng_ * 1664525u + 1013904223u;
      const double noise = (clickRng_ >> 8) * (2.0 / 16777216.0) - 1;
      const double env = std::sin(kPi * t / len);
      drive += float(st.force * env * env * noise * (wound ? 0.6 + 0.4 * std::cos(2 * kPi * 3000 * t) : 1.0));
    }
    const float y = clickB0_ * (drive - clickX2_) - clickA1_ * clickY1_ - clickA2_ * clickY2_;
    clickX2_ = clickX1_;
    clickX1_ = drive;
    clickY2_ = clickY1_;
    clickY1_ = y;
    out += float(s_.click) * y;
    side_ += 0.3f * float(s_.click) * y;
  }
  if constexpr (kArch) acoustic_ = out;
  if (elec) {
    // Two pickups together: two sources in parallel, their mean.
    out = float(s_.select[1] >= 0 ? 0.5 * (emf[0] + emf[1]) : emf[0]);
    side_ = 0;
  }
  if (!std::isfinite(out)) { reset(); return 0; }
  if (!any && std::fabs(out) < 1e-7f) {
    if (++quiet_ > int(0.2 * fs_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return out;
}

float Guitar::sample() { return arch_ ? run<true>() : run<false>(); }

}  // namespace pajupilli
