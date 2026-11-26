#include "bars.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2, kC = 343.0;
// A bar free at both ends: its bending modes' beta L.
const double kBeta[5] = {4.730041, 7.853205, 10.995608, 14.137165, 17.278760};
// Mode n's shape at x / L (the mean of its square along the bar is 1: at a free end it is 2).
double beam(int n, double u) {
  // cosh b + cos b - sigma (sinh b + sin b), sigma = (cosh B - cos B) / (sinh B - sin B),
  // written so that the two huge terms never meet: cosh b sinh B - sinh b cosh B = sinh(B - b).
  const double B = kBeta[n], b = B * u, den = std::sinh(B) - std::sin(B);
  const double sigma = (std::cosh(B) - std::cos(B)) / den;
  const double hyp = (std::sinh(B - b) - std::cosh(b) * std::sin(B) + std::sinh(b) * std::cos(B)) / den;
  return hyp + std::cos(b) - sigma * std::sin(b);
}
double urand(uint32_t& s) {
  s = s * 1664525u + 1013904223u;
  return (s >> 8) * (1.0 / 16777216.0);
}
double logLerp(double a, double b, double t) { return a * fastPow(b / a, std::clamp(t, 0.0, 1.0)); }
double pitch(int midi, double a4 = 440) { return a4 * std::exp2((midi - 69) / 12.0); }

// A bar's bending modes into a key: its shapes under a mallet at `at` (a
// share of the length; `scatter`: how far the mallet strays, as a share),
// on a support at the first mode's nodes (supportLoss: 1/s for a mode moving
// there as at a free end), its own loss (a share of critical, growing by
// `lossRise` for each mode up), and its own sound: a strip much smaller than
// the wavelength whose two faces cancel but for k w, the modes whose halves
// move against each other but for k L / 2 as well. `sound`: x that.
void bendingModes(BarKey* k, int n, const double* ratio, double mass, double L, double w, double at, double scatter,
                  double zeta, double lossRise, double supportLoss, double sound, double rMic, bool lobes = true) {
  const double nrm = 1 / std::sqrt(mass);
  for (int m = 0; m < n; ++m) {
    const int i = k->modes++;
    const double f = k->f1 * ratio[m], wr = 2 * kPi * f;
    k->ratio[i] = ratio[m];
    k->hit[i] = beam(m, at) * nrm;
    k->hitVary[i] = 0.5 * (beam(m, at + scatter) - beam(m, at - scatter)) * nrm;
    const double sup = beam(m, 0.2242) / 2;
    k->loss[i] = zeta * (1 + lossRise * m) * wr + supportLoss * sup * sup;
    // (A free bar's modes move no air on the whole: what goes up in the
    // middle comes down at the ends. What is heard is the middle part,
    // between a quarter and three quarters of the length, less what the
    // ends take back while the bar is short against the wavelength; a mode
    // whose two halves move against each other: one half, likewise.)
    double mid = 0;
    for (int j = 0; j < 16; ++j) mid += beam(m, 0.25 + 0.5 * (j + 0.5) / 16) / 16;
    double area = L * w * mid * (lobes ? std::min(1.0, 2 * kPi * f / kC * L / 3) : 1.0);
    if (m % 2) {
      double half = 0;
      for (int j = 0; j < 16; ++j) half += beam(m, 0.5 * (j + 0.5) / 16) / 16;
      area = 0.5 * L * w * half * std::min(1.0, 2 * kPi * f / kC * L / 2);
    }
    k->rad[i] = sound * kRho / (4 * kPi * rMic) * area * nrm * std::min(1.0, 2 * kPi * f / kC * w);
  }
}

// A tuned air column or box under a bar: its one mode at (nearly) the
// bar's pitch, coupled to the bar's first mode so that the bar's loss
// becomes `ringLoss` (1/s) in all: bar's own + kappa^2 w / (4 x the air's).
void resonator(BarKey* k, double detune, double zetaAir, double ringLoss, double mouthArea, double airMass, double rMic, double tilt) {
  const double w1 = 2 * kPi * k->f1;
  const double zetaWant = ringLoss / w1, zetaOwn = k->loss[0] / w1;
  const double kappa = std::sqrt(std::max(4 * zetaAir * (zetaWant - zetaOwn), 0.0));
  k->resonator = true;
  k->resF = k->f1 * (1 + detune);
  k->resLoss = zetaAir * 2 * kPi * k->resF;
  k->couple[0] = kappa * w1 * w1;
  k->resRad = kRho / (4 * kPi * rMic) * mouthArea / std::sqrt(airMass) * tilt;
}
}  // namespace

// --- The celesta (research/celesta.md; fitted to a recording, section 12).

BarDesign celestaDesign() {
  BarDesign d;
  d.name = "celesta";
  d.keys = 66;
  d.lowMidi = 48;
  d.dampers = true;
  d.speedLo = 0.4; d.speedHi = 3.0;
  const double a4 = knob("PJ_CEL_A4", 440), feltLoss = knob("PJ_CEL_FELTLOSS", 35), steel = knob("PJ_CEL_STEELLOSS", 1e-4);
  const double boxLoss = knob("PJ_CEL_BOXLOSS", 0.02), boxDetune = knob("PJ_CEL_BOXDETUNE", 0.003), ringK = knob("PJ_CEL_RING", 1.0);
  const double hard = knob("PJ_CEL_HARD", 1.0), off = knob("PJ_CEL_OFFCENTRE", 2e-3), plate = knob("PJ_CEL_PLATE", 11.0);
  const double damper = knob("PJ_CEL_DAMPER", 46), boxTilt = knob("PJ_CEL_BOXTILT", 0.5), flat = knob("PJ_CEL_OVERTONEFLAT", 0.99);
  const double ring[6] = {12, 11, 9, 6, 3, 1.0};  // s to -60 dB at C3, C4 .. C8
  const double E = 206e9, rho = 7850, nu = 0.29, cb = std::sqrt(E / rho), ct = std::sqrt(E / (2 * (1 + nu)) / rho);
  uint32_t rng = 88;
  for (int i = 0; i < d.keys; ++i) {
    BarKey& k = d.key[i];
    const int midi = d.lowMidi + i;
    k.f1 = pitch(midi, a4);
    const double t = midi < 84 ? 2.0e-3 : 2.5e-3, w = midi < 60 ? 0.03175 : 0.025;
    const double L = std::sqrt(1.0279 * t * cb / k.f1), mass = rho * L * w * t;
    const double offX = off * (2 * urand(rng) - 1), offY = off * (2 * urand(rng) - 1), uh = 0.5 + offX / L;
    // Five bending modes (the 2nd to 4th a little under the plain strip's:
    // the holes and the felt at the nodes; a recording's 2.73 and 5.35).
    double r[5];
    for (int m = 0; m < 5; ++m) r[m] = kBeta[m] * kBeta[m] / (kBeta[0] * kBeta[0]) * (m >= 1 && m <= 3 ? flat : 1.0);
    bendingModes(&k, 5, r, mass, L, w, uh, 0, steel, 0, feltLoss, plate, d.micDist, false);
    k.rad[0] /= plate;        // (the fundamental's sound is the box's)
    k.rad[1] *= 0.16; k.rad[3] *= 0.16;  // (fitted: the 2.73 partial 45 dB under)
    // Two twisting modes: the angle cos(n pi x / L), the edge's height y x that.
    for (int nt = 1; nt <= 2; ++nt) {
      const int m = k.modes++;
      const double f = nt * (2 * t / w) * ct / (2 * L);
      k.ratio[m] = f / k.f1;
      const double nrm = 1 / std::sqrt(rho * t * (w * w * w / 12) * L);
      k.hit[m] = offY * std::cos(nt * kPi * uh) * std::sqrt(2.0) * nrm;
      const double sup = 0.006 * std::cos(nt * kPi * 0.2242) * std::sqrt(2.0) / (w / 2);
      k.loss[m] = steel * 2 * kPi * f + feltLoss * sup * sup;
    }
    for (int m = 0; m < k.modes; ++m) {
      k.damp[m] = damper * (m < 5 ? 1.0 : 0.3);
      if (k.f1 * k.ratio[m] > 21000) { k.hit[m] = 0; k.rad[m] = 0; }
    }
    // The box: a bottle in the bass, a shallow tube at the top.
    const double oct = (midi - 48) / 12.0, ro = std::clamp(oct, 0.0, 5.0);
    const int ri = std::min(int(ro), 4);
    const double rg = ringK * ring[ri] * fastPow(ring[ri + 1] / ring[ri], ro - ri) * (oct > 5 ? std::exp2(-(oct - 5) * 1.3) : 1);
    const double mouthA = 6e-4 * std::exp2(-0.15 * oct), neck = 0.046 * std::exp2(-0.2 * oct);
    resonator(&k, boxDetune * (2 * urand(rng) - 1), boxLoss, 6.91 / rg, mouthA, kRho * mouthA * neck, d.micDist, std::exp2(-boxTilt * (oct - 1)));
    // (The other modes that move the air over the mouth one way push it too.)
    {
      double mean[5];
      for (int m = 0; m < 5; ++m) {
        mean[m] = 0;
        for (int j = 0; j < 16; ++j) mean[m] += beam(m, 0.25 + 0.5 * (j + 0.5) / 16) / 16;
      }
      for (int m = 2; m < 5; m += 2) k.couple[m] = k.couple[0] * mean[m] / mean[0];
    }
    // The hammer: felt, soft and heavy in the bass, hard and light at the top.
    const double hx = midi <= 72 ? (midi - 48) / 24.0 : 1 + (midi - 72) / 29.0;
    const double hm[3] = {8e-3, 5e-3, 1.5e-3}, hk[3] = {3e8, 1e9, 1e11};
    const int hi = hx < 1 ? 0 : 1;
    k.mallet = hm[hi] * fastPow(hm[hi + 1] / hm[hi], std::clamp(hx, 0.0, 2.0) - hi);
    k.K = hard * hk[hi] * fastPow(hk[hi + 1] / hk[hi], std::clamp(hx, 0.0, 2.0) - hi);
    k.p = 2.5;
    k.hyst = 0.25;
    k.x = 1.2 * ((i + 0.5) / d.keys - 0.5);
  }
  // The case: the blows go through the felt and the frame into the cabinet's
  // wood (a recording's knock near 70, 150, 370 and 560 Hz, 35 dB under the note).
  const double kn = knob("PJ_CEL_KNOCK", 1.25);
  const double f[4] = {72, 150, 370, 560}, t60[4] = {0.14, 0.11, 0.07, 0.05}, g[4] = {1.0, 0.8, 0.9, 0.7};
  for (int i = 0; i < 4; ++i) { d.knockF[i] = f[i]; d.knockT[i] = t60[i]; d.knockG[i] = kn * g[i]; }
  return d;
}

// --- The glockenspiel: G5 .. C8 as sounding (written two octaves lower).

BarDesign glockenspielDesign() {
  BarDesign d;
  d.name = "glockenspiel";
  d.keys = 30;
  d.lowMidi = 79;
  d.speedLo = 0.3; d.speedHi = 3.5;
  const double E = 206e9, rho = 7850, nu = 0.29, cb = std::sqrt(E / rho), ct = std::sqrt(E / (2 * (1 + nu)) / rho);
  const double zeta = knob("PJ_GLOCK_LOSS", 2e-4), rail = knob("PJ_GLOCK_RAIL", 3.0), hard = knob("PJ_GLOCK_HARD", 1.0);
  uint32_t rng = 31;
  for (int i = 0; i < d.keys; ++i) {
    BarKey& k = d.key[i];
    const int midi = d.lowMidi + i;
    k.f1 = pitch(midi);
    // Bars 9.5 mm thick, 32 mm wide (25 at the top), their lengths for their pitch.
    const double t = 9.5e-3, w = 0.032 - 0.007 * i / (d.keys - 1.0);
    const double L = std::sqrt(1.0279 * t * cb / k.f1), mass = rho * L * w * t;
    double r[4];
    for (int m = 0; m < 4; ++m) r[m] = kBeta[m] * kBeta[m] / (kBeta[0] * kBeta[0]);
    // Struck at the middle, give or take a few millimetres; lying on a felted rail at its nodes.
    bendingModes(&k, 4, r, mass, L, w, 0.5 + 0.01 * (2 * urand(rng) - 1), 0.004 / L, zeta, 0.15, rail, 1.0, d.micDist);
    // The first twisting mode, for the strokes that land off the bar's centre line.
    {
      const int m = k.modes++;
      const double f = (2 * t / w) * ct / (2 * L);
      k.ratio[m] = f / k.f1;
      const double nrm = 1 / std::sqrt(rho * t * (w * w * w / 12) * L);
      k.hitVary[m] = 0.004 * std::sqrt(2.0) * nrm * 0.5;
      k.loss[m] = zeta * 2 * kPi * f + rail * 0.2;
      k.rad[m] = 0.2 * kRho / (4 * kPi * d.micDist) * L * w * (w / 4) * nrm * std::min(1.0, 2 * kPi * f / kC * w);
    }
    for (int m = 0; m < k.modes; ++m)
      if (k.f1 * k.ratio[m] > 21000) { k.hit[m] = k.hitVary[m] = 0; k.rad[m] = 0; }
    // A hard mallet head (brass or hard plastic) on a thin shaft.
    k.mallet = 0.022;
    k.K = hard * logLerp(1.5e9, 4e9, i / (d.keys - 1.0));
    k.p = 1.5;
    k.x = 0.7 * ((i + 0.5) / d.keys - 0.5);
  }
  return d;
}

// --- The marimba (C2 .. C7) and the xylophone (F4 .. C8 as sounding).

static BarDesign woodBars(const char* name, int lowMidi, int keys, const double* ratio, double lenLow, double lenHigh,
                          double widthLow, double widthHigh, double thick, double zeta, double ringShorter,
                          double malletMass, double kLow, double kHigh, double at, double scatter, double span, double tubeTilt, double overtone) {
  BarDesign d;
  d.name = name;
  d.keys = keys;
  d.lowMidi = lowMidi;
  d.speedLo = 0.3; d.speedHi = 4.0;
  const double rho = 900;  // Honduras rosewood, kg/m^3
  uint32_t rng = 57 + lowMidi;
  for (int i = 0; i < keys; ++i) {
    BarKey& k = d.key[i];
    const double t = i / (keys - 1.0);
    k.f1 = pitch(lowMidi + i);
    const double L = logLerp(lenLow, lenHigh, t), w = lenLow > 0 ? widthLow + (widthHigh - widthLow) * t : widthLow;
    const double mass = 0.8 * rho * L * w * thick;  // (the arch carved out of its underside)
    // Struck off the middle (the second mode has a node there), a little
    // elsewhere each time; hung on a cord through its nodes.
    bendingModes(&k, 4, ratio, mass, L, w, at, scatter, zeta, 0.5, 2.0, 1.0, d.micDist);
    for (int m = 0; m < k.modes; ++m)
      if (k.f1 * k.ratio[m] > 21000) { k.hit[m] = k.hitVary[m] = 0; k.rad[m] = 0; }
    // The tube: closed below, a quarter of the wavelength long, its mouth
    // under the bar's middle. It sounds the fundamental and shortens it.
    const double rad = 0.027 - 0.012 * t, area = kPi * rad * rad, tube = kC / (4 * k.f1);
    resonator(&k, 0.004 * (2 * urand(rng) - 1), 0.012, k.loss[0] * ringShorter, area, kRho * area * tube / 2, d.micDist,
              std::exp2(-tubeTilt * (lowMidi + i - 60) / 12.0));
    // (The bar's own sound: the overtones, which no tube helps.)
    for (int m = 1; m < k.modes; ++m) k.rad[m] *= overtone;
    k.mallet = malletMass;
    k.K = logLerp(kLow, kHigh, t);
    k.p = 1.5;
    k.hyst = 0.1;
    k.x = span * ((i + 0.5) / keys - 0.5);
  }
  return d;
}

BarDesign marimbaDesign() {
  // Tuned 1 : 4 : 10 (as measured bars come out: 3.92, 9.24, 16.3); yarn mallets, softer in the bass.
  const double r[4] = {1, 3.92, 9.24, 16.27};
  const double hard = knob("PJ_MARIMBA_HARD", 1.0);
  return woodBars("marimba", 36, 61, r, 0.60, 0.19, 0.068, 0.040, 0.022, knob("PJ_MARIMBA_LOSS", 0.0025),
                  knob("PJ_MARIMBA_TUBE", 1.7), 0.030, hard * 3e6, hard * 4e7, 0.38, 0.05, 2.4, 1.4, knob("PJ_MARIMBA_OVERTONE", 1.0));
}

BarDesign xylophoneDesign() {
  // Tuned 1 : 3 : 6; hard mallets; the notes short.
  const double r[4] = {1, 3.0, 6.1, 9.9};
  const double hard = knob("PJ_XYLO_HARD", 1.0);
  return woodBars("xylophone", 65, 44, r, 0.38, 0.13, 0.040, 0.036, 0.019, knob("PJ_XYLO_LOSS", 0.0045),
                  knob("PJ_XYLO_TUBE", 1.3), 0.020, hard * 3e8, hard * 8e8, 0.36, 0.04, 1.4, 1.1, knob("PJ_XYLO_OVERTONE", 1.0));
}

// --- Tubular bells: C4 .. F5.

BarDesign tubularBellsDesign() {
  BarDesign d;
  d.name = "tubular-bells";
  d.keys = 18;
  d.lowMidi = 60;
  d.speedLo = 0.5; d.speedHi = 3.0;
  d.micDist = 1.5;
  const double E = 105e9, rho = 8500, dia = 0.038, wall = 1.5e-3;  // brass
  const double c = std::sqrt(E / rho), gyr = dia / (2 * std::sqrt(2.0));
  const double lossLow = knob("PJ_BELLS_LOSS", 0.35), lossRise = knob("PJ_BELLS_LOSSRISE", 0.5), hard = knob("PJ_BELLS_HARD", 1.0);
  for (int i = 0; i < d.keys; ++i) {
    BarKey& k = d.key[i];
    k.f1 = pitch(d.lowMidi + i);  // the note heard: an octave under the fourth mode
    // A free tube's bending modes lie nearly as the odd squares, 9 : 25 : 49 : 81 : 121 : 169 ...;
    // the fourth is tuned to twice the note.
    const double f4 = 2 * k.f1, L = std::sqrt(kPi / 8 * 81 * gyr * c / f4);
    const double mass = rho * kPi * dia * wall * L, nrm = 1 / std::sqrt(mass);
    for (int n = 1; n <= 9; ++n) {
      const int m = k.modes++;
      const double f = f4 * (2 * n + 1) * (2 * n + 1) / 81.0;
      k.ratio[m] = f / k.f1;
      k.hit[m] = 2 * nrm;  // struck on its top end, where every mode swings fully
      k.hitVary[m] = 0;
      k.loss[m] = lossLow + lossRise * f / 1000;
      // A thin tube sounds as a line of small sources, its two sides against
      // each other: poorly while it is thinner than the wavelength (the low
      // modes are hardly heard), better the higher.
      const double kd = 2 * kPi * f / kC * dia;
      k.rad[m] = kRho / (4 * kPi * d.micDist) * L * dia * 0.6 * nrm * std::min(1.0, kd);
      if (f > 21000) { k.hit[m] = 0; k.rad[m] = 0; }
    }
    // A rawhide (or plastic) hammer.
    k.mallet = 0.2;
    k.K = hard * 4e7;
    k.p = 1.5;
    k.hyst = 0.3;
    k.x = 0.9 * ((i + 0.5) / d.keys - 0.5);
  }
  return d;
}

// --- The music box: C4 .. C8.

BarDesign musicBoxDesign() {
  BarDesign d;
  d.name = "music-box";
  d.keys = 49;
  d.lowMidi = 60;
  d.micDist = 0.5;
  d.micApart = 0.2;
  d.speedLo = 0.5; d.speedHi = 1.5;  // (a pin bends every tooth much alike)
  const double zeta = knob("PJ_MUSICBOX_LOSS", 8e-4), boxCorner = knob("PJ_MUSICBOX_BOX", 300), slip = knob("PJ_MUSICBOX_SLIP", 0.5e-3);
  // A tooth clamped at its root: 1 : 6.27 : 17.5 : 34.4.
  const double r[4] = {1, 6.267, 17.55, 34.39};
  for (int i = 0; i < d.keys; ++i) {
    BarKey& k = d.key[i];
    const double t = i / (d.keys - 1.0);
    k.f1 = pitch(d.lowMidi + i);
    const double mass = logLerp(2.5e-3, 0.25e-3, t), nrm = 1 / std::sqrt(mass);  // (the bass teeth leaded)
    for (int m = 0; m < 4; ++m) {
      const int j = k.modes++;
      const double f = k.f1 * r[m];
      k.ratio[j] = r[m];
      // The tooth's tip; the pin does not let go all at once (it slides
      // off over `slip` s), which the high modes are too quick to feel fully.
      k.hit[j] = 2 * (m % 2 ? -1 : 1) * nrm / std::sqrt(1 + (kPi * f * slip) * (kPi * f * slip));
      k.loss[j] = zeta * (1 + 1.5 * m) * 2 * kPi * f;
      // The tooth itself is far too small to be heard: its root shakes the
      // comb (each mode by how much of the tooth it moves one way: 0.78,
      // 0.43, 0.25, 0.18), the comb the box's wood, which sounds what lies
      // in its range.
      const double share[4] = {0.783, 0.434, 0.254, 0.182};
      const double box = (f * f) / (f * f + boxCorner * boxCorner) / (1 + (f / 7000) * (f / 7000));
      k.rad[j] = 2.0e-4 * nrm * share[m] * box;
      if (f > 21000) { k.hit[j] = 0; k.rad[j] = 0; }
    }
    k.pluck = true;
    k.K = 0.6;  // N per m/s: how far the pin bends the tooth before it slips
    k.x = 0.15 * ((i + 0.5) / d.keys - 0.5);
  }
  // The pin slipping off and the comb's snap into the box.
  const double f[4] = {420, 900, 1700, 3100}, t60[4] = {0.05, 0.04, 0.03, 0.02}, g[4] = {0.6, 0.8, 0.6, 0.4};
  const double kn = knob("PJ_MUSICBOX_TICK", 1.0);
  for (int i = 0; i < 4; ++i) { d.knockF[i] = f[i]; d.knockT[i] = t60[i]; d.knockG[i] = kn * g[i]; }
  return d;
}

bool barDesignFor(const char* name, BarDesign* d) {
  if (!std::strcmp(name, "celesta")) *d = celestaDesign();
  else if (!std::strcmp(name, "glockenspiel")) *d = glockenspielDesign();
  else if (!std::strcmp(name, "marimba")) *d = marimbaDesign();
  else if (!std::strcmp(name, "xylophone")) *d = xylophoneDesign();
  else if (!std::strcmp(name, "tubular-bells")) *d = tubularBellsDesign();
  else if (!std::strcmp(name, "music-box")) *d = musicBoxDesign();
  else return false;
  return true;
}

// --- The engine.

Bars::Bars(double fs, const BarDesign& design) : d_(design), fs_(fs), dt_(1 / fs) {
  n_ = std::min(d_.keys, kMaxKeys);
  for (int i = 0; i < 4; ++i) {
    if (d_.knockF[i] <= 0) continue;
    const double w = 2 * kPi * d_.knockF[i], a = 6.91 / d_.knockT[i], gd = std::exp(-a * dt_), c = std::cos(w * dt_), sn = std::sin(w * dt_);
    Case k;
    k.a11 = float(gd * c); k.a12 = float(gd * sn / w); k.a21 = float(-gd * w * sn); k.a22 = float(gd * c);
    k.gain = float(d_.knockG[i]);
    case_.push_back(k);
  }
  for (int i = 0; i < n_; ++i) {
    Key& k = key_[i];
    const BarKey& b = d_.key[i];
    k.nm = std::min(b.modes, kModes);
    for (int m = 0; m < k.nm; ++m) {
      const double wr = 2 * kPi * b.f1 * b.ratio[m];
      k.w2[m] = float(wr * wr);
      k.hit[m] = float(b.hit[m]);
      k.damp[m] = float(b.damp[m]);
      k.couple[m] = float(b.couple[m]);
      k.rad[m] = float(b.rad[m]);
    }
    if (b.resonator) {
      const double wr = 2 * kPi * b.resF;
      k.w2[kModes] = float(wr * wr);
      k.rad[kModes] = float(b.resRad);
    }
    k.damperDown = d_.dampers;
    k.engage = d_.dampers ? 1 : 0;
    const double dl = std::hypot(b.x + d_.micApart / 2, d_.micDist), dr = std::hypot(b.x - d_.micApart / 2, d_.micDist);
    k.gainL = float(d_.micDist / dl);
    k.gainR = float(d_.micDist / dr);
    coefs(k, b);
  }
}

void Bars::coefs(Key& k, const BarKey& b) {
  for (int m = 0; m <= kModes; ++m) {
    if (m >= k.nm && m < kModes) continue;
    if (m == kModes && !b.resonator) continue;
    const double own = m < kModes ? b.loss[m] : b.resLoss;
    const double a = own + (m < kModes ? k.engage * k.damp[m] : 0), w0 = std::sqrt(double(k.w2[m]));
    const double w = std::sqrt(std::max(w0 * w0 - a * a, 1.0));
    const double g = std::exp(-a * dt_), c = std::cos(w * dt_), s = std::sin(w * dt_);
    k.a11[m] = float(g * (c + a / w * s));
    k.a12[m] = float(g * s / w);
    k.a21[m] = float(-g * (w0 * w0 / w) * s);
    k.a22[m] = float(g * (c - a / w * s));
  }
}

void Bars::reset() {
  for (int i = 0; i < n_; ++i) {
    Key& k = key_[i];
    std::fill(k.q, k.q + kModes + 1, 0.0f);
    std::fill(k.v, k.v + kModes + 1, 0.0f);
    k.hammerOn = false;
    k.active = false;
    k.engage = k.damperDown ? 1 : 0;
    coefs(k, d_.key[i]);
  }
  nActive_ = 0;
  prevL_ = prevR_ = side_ = 0;
  knockIn_ = 0;
  for (Case& c : case_) c.x = c.v = 0;
}

void Bars::strike(int key, double speed) {
  if (key < 0 || key >= n_) return;
  Key& k = key_[key];
  const BarKey& b = d_.key[key];
  // Where the mallet lands this time.
  const double stray = 2 * urand(rng_) - 1;
  double g = 0;
  for (int m = 0; m < k.nm; ++m) {
    k.hit[m] = float(b.hit[m] + stray * b.hitVary[m]);
    g += double(k.hit[m]) * k.hit[m];
  }
  if (!k.active) { k.active = true; k.quiet = 0; active_[nActive_++] = key; }
  if (b.pluck) {
    // Bent by the pin and let go: each mode starts from where the bending
    // force held it (on top of whatever it was doing).
    const double force = b.K * speed;
    for (int m = 0; m < k.nm; ++m) k.q[m] += float(k.hit[m] * force / k.w2[m]);
    knockIn_ += float(force * fs_ * 0.02);
    return;
  }
  k.G = float(dt_ * dt_ * (g + 1 / b.mallet));
  double h = 0;
  for (int m = 0; m < k.nm; ++m) h += double(k.hit[m]) * k.q[m];
  k.hz = h + 1e-5;  // from above, a hair over the piece as it is now
  k.hv = -speed;
  k.hammerOn = true;
  k.dPrev = 0;
}

void Bars::damper(int key, bool down) {
  if (key < 0 || key >= n_ || !d_.dampers) return;
  Key& k = key_[key];
  k.damperDown = down;
  if (!k.active && down != (k.engage > 0.5f)) { k.engage = down ? 1 : 0; coefs(k, d_.key[key]); }
}

float Bars::sample() {
  float l = 0, r = 0;
  const float dt = float(dt_);
  for (int a = 0; a < nActive_; ++a) {
    const int ki = active_[a];
    Key& k = key_[ki];
    const BarKey& b = d_.key[ki];
    // The damper coming down or going up.
    const float want = k.damperDown ? 1.0f : 0.0f;
    if (k.engage != want) {
      const float step = float(dt_ / d_.damperTime);
      k.engage = want > k.engage ? std::min(want, k.engage + step) : std::max(want, k.engage - 4 * step);
      if ((++k.tick & 15) == 0 || k.engage == want) coefs(k, b);
    }
    float f[kModes + 1] = {};
    // The mallet: what is between it and the piece (felt, yarn, a hard
    // head), its force found for the end of the sample.
    if (k.hammerOn) {
      float h = 0, hv = 0;
      for (int m = 0; m < k.nm; ++m) { h += k.hit[m] * k.q[m]; hv += k.hit[m] * k.v[m]; }
      const double free = (h - k.hz) + dt_ * (hv - k.hv);
      double force = 0, d = 0;
      if (free > 0) {
        double lo = 0, hi = free;
        const double hyst = 1 + b.hyst * std::clamp((free - k.dPrev) / dt_, -2.0, 6.0);
        for (int it = 0; it < 34; ++it) {
          d = 0.5 * (lo + hi);
          if (d + k.G * b.K * fastPow(d, b.p) * hyst > free) hi = d; else lo = d;
        }
        force = std::max(0.0, b.K * fastPow(d, b.p) * hyst);
      }
      k.dPrev = float(d);
      knockIn_ += float(force);
      k.hv += dt_ * force / b.mallet;
      k.hz += dt_ * k.hv;
      for (int m = 0; m < k.nm; ++m) f[m] -= k.hit[m] * float(force);
      if (force == 0 && k.hv > 0) k.hammerOn = false;  // thrown back: it is taken away
    }
    // The piece and its resonator's air: each pushes the other.
    if (b.resonator) {
      const float qa = k.q[kModes];
      float push = 0;
      for (int m = 0; m < k.nm; ++m) {
        f[m] += k.couple[m] * qa;
        push += k.couple[m] * k.q[m];
      }
      f[kModes] += push;
    }
    float y = 0;
    for (int m = 0; m < k.nm; ++m) {
      const float v = k.v[m] + dt * f[m], q = k.q[m];
      k.q[m] = k.a11[m] * q + k.a12[m] * v;
      k.v[m] = k.a21[m] * q + k.a22[m] * v;
      y += k.rad[m] * k.v[m];
    }
    if (b.resonator) {
      const int m = kModes;
      const float v = k.v[m] + dt * f[m], q = k.q[m];
      k.q[m] = k.a11[m] * q + k.a12[m] * v;
      k.v[m] = k.a21[m] * q + k.a22[m] * v;
      y += k.rad[m] * k.v[m];
    }
    l += k.gainL * y;
    r += k.gainR * y;
    // Asleep once nothing is left of it.
    if (std::fabs(y) < 1e-9f && !k.hammerOn && k.engage == want) ++k.quiet; else k.quiet = 0;
    if (k.quiet > int(0.25 * fs_)) {
      k.active = false;
      std::fill(k.q, k.q + kModes + 1, 0.0f);
      std::fill(k.v, k.v + kModes + 1, 0.0f);
      active_[a--] = active_[--nActive_];
    }
  }
  // The case or frame: its short answer to the blows.
  float knock = 0;
  if (knockIn_ != 0 || knockOn_) {
    knockOn_ = false;
    for (Case& c : case_) {
      const float v = c.v + c.gain * knockIn_ * dt, x = c.x;
      c.x = c.a11 * x + c.a12 * v;
      c.v = c.a21 * x + c.a22 * v;
      knock += c.v;
      knockOn_ |= std::fabs(c.v) > 1e-9f;
    }
    knockIn_ = 0;
  }
  // (The sources' strength is their acceleration: the sum's rate of change.)
  const float outL = (l - prevL_) * float(fs_) + knock, outR = (r - prevR_) * float(fs_) + knock;
  prevL_ = l; prevR_ = r;
  side_ = 0.5f * (outL - outR);
  const float mono = 0.5f * (outL + outR);
  if (!std::isfinite(mono)) { reset(); return 0; }
  return mono;
}

}  // namespace pajupilli
