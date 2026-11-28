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

// --- Tubular bells (research/tubular_bells.md): church bells C2 .. B3, tubes C4 .. G5,
// carillon bells G#5 .. C8; the change from one to the next sudden, as from one instrument to
// another (the tubes being a portable carillon, the low tubes a church bell's cheap stand-in).

namespace {
constexpr int kTubeLow = 60, kTubeHigh = 79;  // C4, G5

// A tube's bending modes (free at both ends, the plug at the top, where it is struck) by finite
// elements: Timoshenko beam elements (shear and rotary inertia), the plug a mass with its own
// rotary inertia (research/tubular_bells.md 3.2: within ~5 c of a fine mesh to the 9th mode with
// 40 elements). Writes the first `want` frequencies (Hz) and each mode's displacement at the
// top, mass-normalised (1/sqrt(kg)). The generalized problem K x = w^2 M x by Cholesky and cyclic
// Jacobi.
void tubeModes(double L, double D, double wall, double plug, double plugLen, int want, double* f, double* top) {
  constexpr int ne = 40, n = 2 * (ne + 1);
  const double E = 105e9, rho = 8500, nu = 0.34, G = E / (2 * (1 + nu)), kap = 0.53;
  const double Di = D - 2 * wall, A = kPi / 4 * (D * D - Di * Di), I = kPi / 64 * (D * D * D * D - Di * Di * Di * Di);
  const double h = L / ne, ph = 12 * E * I / (kap * G * A * h * h), c = E * I / (h * h * h * (1 + ph));
  static thread_local double K[n][n], M[n][n], V[n][n];
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) K[i][j] = M[i][j] = 0;
  const double ke[4][4] = {{12, 6 * h, -12, 6 * h}, {6 * h, (4 + ph) * h * h, -6 * h, (2 - ph) * h * h},
                           {-12, -6 * h, 12, -6 * h}, {6 * h, (2 - ph) * h * h, -6 * h, (4 + ph) * h * h}};
  const double mt[4][4] = {{156, 22 * h, 54, -13 * h}, {22 * h, 4 * h * h, 13 * h, -3 * h * h},
                           {54, 13 * h, 156, -22 * h}, {-13 * h, -3 * h * h, -22 * h, 4 * h * h}};
  const double mr[4][4] = {{36, 3 * h, -36, 3 * h}, {3 * h, 4 * h * h, -3 * h, -h * h},
                           {-36, -3 * h, 36, -3 * h}, {3 * h, -h * h, -3 * h, 4 * h * h}};
  const double ct = rho * A * h / 420, cr = rho * I / (30 * h);
  for (int e = 0; e < ne; ++e)
    for (int i = 0; i < 4; ++i)
      for (int j = 0; j < 4; ++j) {
        K[2 * e + i][2 * e + j] += c * ke[i][j];
        M[2 * e + i][2 * e + j] += ct * mt[i][j] + cr * mr[i][j];
      }
  M[0][0] += plug;
  M[1][1] += plug * (D * D / 16 + plugLen * plugLen / 12);
  // M = R R^T (R lower), C = R^-1 K R^-T, into K.
  for (int j = 0; j < n; ++j) {
    double d = M[j][j];
    for (int k = 0; k < j; ++k) d -= M[j][k] * M[j][k];
    M[j][j] = std::sqrt(d);
    for (int i = j + 1; i < n; ++i) {
      double s = M[i][j];
      for (int k = 0; k < j; ++k) s -= M[i][k] * M[j][k];
      M[i][j] = s / M[j][j];
    }
  }
  for (int col = 0; col < n; ++col)  // K <- R^-1 K (columns)
    for (int i = 0; i < n; ++i) {
      double s = K[i][col];
      for (int k = 0; k < i; ++k) s -= M[i][k] * K[k][col];
      K[i][col] = s / M[i][i];
    }
  for (int row = 0; row < n; ++row)  // K <- K R^-T (rows)
    for (int i = 0; i < n; ++i) {
      double s = K[row][i];
      for (int k = 0; k < i; ++k) s -= M[i][k] * K[row][k];
      K[row][i] = s / M[i][i];
    }
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) V[i][j] = i == j;
  for (int sweep = 0; sweep < 30; ++sweep) {
    double off = 0, diag = 0;
    for (int i = 0; i < n; ++i) {
      diag += K[i][i] * K[i][i];
      for (int j = i + 1; j < n; ++j) off += K[i][j] * K[i][j];
    }
    if (off < 1e-26 * diag) break;
    for (int p = 0; p < n; ++p)
      for (int q = p + 1; q < n; ++q) {
        if (std::fabs(K[p][q]) < 1e-300) continue;
        const double th = (K[q][q] - K[p][p]) / (2 * K[p][q]);
        const double t = (th >= 0 ? 1 : -1) / (std::fabs(th) + std::sqrt(th * th + 1));
        const double cs = 1 / std::sqrt(t * t + 1), sn = t * cs;
        for (int k = 0; k < n; ++k) {
          const double kp = K[k][p], kq = K[k][q];
          K[k][p] = cs * kp - sn * kq;
          K[k][q] = sn * kp + cs * kq;
        }
        for (int k = 0; k < n; ++k) {
          const double pk = K[p][k], qk = K[q][k];
          K[p][k] = cs * pk - sn * qk;
          K[q][k] = sn * pk + cs * qk;
        }
        for (int k = 0; k < n; ++k) {
          const double vp = V[k][p], vq = V[k][q];
          V[k][p] = cs * vp - sn * vq;
          V[k][q] = sn * vp + cs * vq;
        }
      }
  }
  int idx[n];
  for (int i = 0; i < n; ++i) idx[i] = i;
  std::sort(idx, idx + n, [&](int a, int b) { return K[a][a] < K[b][b]; });
  // (The two rigid motions first.) The mode's shape x = R^-T y: its top displacement x[0].
  for (int m = 0; m < want; ++m) {
    const int e = idx[m + 2];
    f[m] = std::sqrt(std::max(K[e][e], 0.0)) / (2 * kPi);
    double x[n];
    for (int i = n - 1; i >= 0; --i) {
      double s = V[i][e];
      for (int k = i + 1; k < n; ++k) s -= M[k][i] * x[k];
      x[i] = s / M[i][i];
    }
    top[m] = std::fabs(x[0]);
  }
}

// A tube: brass, hung by a cord near its top, struck on the cap at its top. Its bending modes
// (tubeModes), the 4th tuned to twice the note; each a doublet (the tube not quite round); its
// longitudinal mode, which only the stroke's share along the tube drives. Each mode's sound is set
// by its level under a hard impulse (`lv`, dB; fitted to the recorded sets, section 3.1 / 10), the
// hammer's contact then shaping it by face and speed.
void tubeKey(BarKey& k, double fs, double D, uint32_t& rng) {
  static const double lossA = knob("PJ_BELLS_LOSS", 0.11), lossC = knob("PJ_BELLS_LOSSCUBE", 0.025);
  static const double split = knob("PJ_BELLS_SPLIT", 1.0), axial = knob("PJ_BELLS_AXIAL", 0.1);
  static const double rawK = knob("PJ_BELLS_RAWHIDE", 2e8), woodK = knob("PJ_BELLS_WOOD", 1.2e9);
  static const double level = knob("PJ_BELLS_TUBELEVEL", 1.6), plugK = knob("PJ_BELLS_PLUG", 1.0);
  const double E = 105e9, rho = 8500, nu = 0.34, G = E / (2 * (1 + nu)), kap = 0.53;
  const double wall = 0.05 * D, Di = D - 2 * wall, r2 = (D * D + Di * Di) / 16;
  const double A = kPi / 4 * (D * D - Di * Di), I = kPi / 64 * (D * D * D * D - Di * Di * Di * Di);
  const double cb = std::sqrt(E * I / (rho * A)), shear = r2 * (1 + E / (kap * G));
  // (20 g for a 3/4" tube, as the area of its end.)
  const double plug = plugK * 0.02 * (D / 0.01905) * (D / 0.01905), plugLen = 0.015 * D / 0.01905;
  // The length: Timoshenko's 4th mode without the plug (f_EB / sqrt(1 + (beta r / L)^2 (1 +
  // E / (kappa G)))), then the elements' with it.
  auto f4 = [&](double L) {
    const double b = kBeta[3] / L;
    return b * b * cb / (2 * kPi) / std::sqrt(1 + b * b * shear);
  };
  double L = std::sqrt(kBeta[3] * kBeta[3] * cb / (2 * kPi * 2 * fs));
  for (int it = 0; it < 8; ++it) L *= std::sqrt(f4(L) / (2 * fs));
  double f[12], top[12];
  for (int it = 0; it < 2; ++it) {
    tubeModes(L, D, wall, plug, plugLen, 12, f, top);
    L *= std::sqrt(f[3] / (2 * fs));
  }
  tubeModes(L, D, wall, plug, plugLen, 12, f, top);
  const double M = rho * A * L + plug, mh = 0.15, tune = 2 * fs / f[3];
  // (Evened over the compass, about as the recorded sets are: the short tubes would be ~4 dB an
  // octave quieter.)
  const double even = level * 0.75 * std::pow(fs / 261.6, 0.63);
  k.f1 = fs;
  // Under a hard impulse (dB re the 5th): the low modes hardly radiate (a thin tube's two sides
  // against each other), the strike note's group the most.
  static const double lv[12] = {-40, -26, -18, -3, 0, -2, -4, -12, -18, -24, -30, -36};
  for (int n = 0; n < 12; ++n) {
    const double fn = f[n] * tune;
    if (fn * 1.01 > 20000) break;
    // (The split: 0.15 % at the 4th, a slow wah of ~1.5 Hz at C5, widening upwards, Hibbert's
    // "very wide" in the high partials: ~1 % at the 8th.)
    const double d = split * 0.0015 * std::exp2(0.65 * (n - 3)) * (0.5 + urand(rng));
    for (int h = 0; h < 2; ++h) {
      const int m = k.modes++;
      const double fm = fn * (h ? 1 + d / 2 : 1 - d / 2), w = 2 * kPi * fm;
      k.ratio[m] = fm / fs;
      k.hit[m] = top[n];
      // P4 ~35 s, P5-P6 15-25 s, P7 and up 3-8 s (the user's listening, section 3.5); the
      // weaker direction of a doublet dies faster (the recordings: 2-4 x).
      k.loss[m] = (lossA + lossC * std::pow(fm / 1000, 3)) * (h ? 1.5 : 1.0);
      k.rad[m] = even * std::pow(10.0, lv[n] / 20) / (w * top[n] * mh);
    }
    k.pair[k.modes - 2] = k.modes - 1;
  }
  // The longitudinal mode, c / 2L: driven by the stroke's share along the tube.
  {
    const double fl = std::sqrt(E / rho) / (2 * L), nrm = 1 / std::sqrt(M);
    const int m = k.modes++;
    k.ratio[m] = fl / fs;
    k.hit[m] = axial * 2 * nrm;
    k.hitVary[m] = 0.5 * axial * 2 * nrm;
    k.loss[m] = 0.45;
    k.rad[m] = even * std::pow(10.0, -22 / 20.0) / (2 * kPi * fl * 2 * nrm * mh);
  }
  k.theta0 = 2 * kPi * urand(rng);
  k.thetaVary = 0.17;  // (+-10 degrees: a careful player's horizontal blow)
  // The hammer: rolled rawhide, and its wood face (research/tubular_bells.md 4).
  k.mallet = mh; k.K = rawK; k.p = 1.5; k.hyst = 0.3;
  k.mallet2 = mh; k.K2 = woodK; k.p2 = 1.5; k.hyst2 = 0.05;
  k.knock[0] = 0.2; k.knock[1] = 1.0;
}

// A cast bronze bell, true-harmonic (Simpson) tuned, hanging still and struck on its sound bow:
// hum, prime, tierce, quint, nominal, superquint, octave nominal and two above (Lehr 1986's F4
// bell; the carillon bells' tierce a little sharp of the minor third). Each a doublet (a cast
// bell is less round than a tube). The hum and the prime ring longest; a big bell longer than a
// small one. Its mass ~ 2 t at C4, x 2 every four semitones down (Chime Master), at least 12 kg
// (the small bells are thick). The hammer: a padded face and an iron one, heavier for a bigger
// bell. Old-style tuning (PJ_BELLS_OLDSTYLE): the hum a little sharp, the prime flat.
void castBellKey(BarKey& k, double fs, bool carillon, uint32_t& rng) {
  static const double oldStyle = knob("PJ_BELLS_OLDSTYLE", 0), level = knob("PJ_BELLS_BELLLEVEL", 0.45);
  static const double ring = knob("PJ_BELLS_BELLRING", 1.0), split = knob("PJ_BELLS_SPLIT", 1.0);
  static const double padK = knob("PJ_BELLS_PADDED", 4e8), ironK = knob("PJ_BELLS_IRON", 1e10);
  double r[9] = {0.5, 1, 1.205, 1.5, 2, 3.008, 4.174, 5.472, 6.832};
  if (oldStyle > 0) { r[0] *= std::exp2(oldStyle * 150 / 1200.0); r[1] *= std::exp2(-oldStyle * 120 / 1200.0); }
  static const double lv[9] = {-10, -6, -6, -22, 0, -5, -8, -12, -16};
  static const double t60[9] = {90, 50, 45, 15, 22, 7, 4, 2.5, 1.8};  // (s, a C4 bell) [est]
  static const double dsp[9] = {0.0025, 0.004, 0.003, 0.002, 0.0015, 0.0012, 0.001, 0.001, 0.001};
  const double M = std::max(12.0, 2000 * std::pow(fs / 261.6, -3.0)), nrm = 1.5 / std::sqrt(0.25 * M);
  const double mh = std::clamp(1.5 * std::cbrt(M / 2000), 0.25, 6.0);
  const double size = std::pow(261.6 / fs, 0.8);
  k.f1 = fs;
  for (int n = 0; n < 9; ++n) {
    const double f = fs * r[n];
    if (f * 1.01 > 20000) break;
    const double d = split * dsp[n] * (0.5 + urand(rng));
    for (int h = 0; h < 2; ++h) {
      const int m = k.modes++;
      const double fm = f * (h ? 1 + d / 2 : 1 - d / 2), w = 2 * kPi * fm;
      k.ratio[m] = fm / fs;
      k.hit[m] = nrm;
      k.loss[m] = 6.91 / std::min(150.0, ring * t60[n] * size) * (h ? 1.3 : 1.0);
      k.rad[m] = level * (carillon ? 0.88 * std::pow(fs / 830.6, 0.7) : 0.85) * std::pow(10.0, lv[n] / 20) / (w * nrm * mh);
    }
    k.pair[k.modes - 2] = k.modes - 1;
  }
  k.theta0 = 2 * kPi * urand(rng);
  k.thetaVary = 0.2;
  // (A carillon's clappers are bare iron: the soft face there a lighter touch of it.)
  static const double carK = knob("PJ_BELLS_CARILLON", 3e9);
  k.mallet = mh; k.K = carillon ? carK : padK; k.p = 1.5; k.hyst = carillon ? 0.05 : 0.3;
  k.mallet2 = mh; k.K2 = ironK; k.p2 = 1.5; k.hyst2 = 0.02;
  k.knock[0] = k.knock[1] = 0;
}
}  // namespace

BarDesign tubularBellsDesign() {
  BarDesign d;
  d.name = "tubular-bells";
  d.lowMidi = 36;
  d.keys = 73;  // C2 .. C8
  d.speedLo = 0.5; d.speedHi = 3.0;
  d.micDist = 1.5;
  // Dampers: a key's note-off brings the felt down unless the pedal (CC 64) holds them all off
  // (BarsPlayer::noteOff, setHold).
  d.dampers = true;
  d.damperTime = knob("PJ_BELLS_DAMPER_TIME", 0.04);
  const double damper = knob("PJ_BELLS_DAMPER", 20);
  // The tubes' diameter (mm): 19.05 (3/4", as the recorded sets and the user's video), 31.8 or 38.1.
  const double D = knob("PJ_BELLS_DIAMETER", 19.05) * 1e-3;
  uint32_t rng = 1515;
  for (int i = 0; i < d.keys; ++i) {
    BarKey& k = d.key[i];
    const int midi = d.lowMidi + i;
    const double fs = pitch(midi);  // the strike note
    const bool tube = midi >= kTubeLow && midi <= kTubeHigh;
    if (tube) tubeKey(k, fs, D, rng);
    else castBellKey(k, fs, midi > kTubeHigh, rng);
    for (int m = 0; m < k.modes; ++m) {
      if (fs * k.ratio[m] > 20000) { k.hit[m] = k.hitVary[m] = 0; k.rad[m] = 0; }
      // The damper (research/tubular_bells.md 2, 6): the felt bar pressed against the tube's side,
      // the higher modes (shorter waves, more motion in the felt) taken sooner. PJ_BELLS_DAMPER
      // (1/s at 500 Hz: 20, ~0.35 s to -60 dB). The bells beyond the tubes have none on a real set;
      // a file's note-off asks for the stop all the same: a hand on the heavier bell, 0.6 of it.
      const double f = fs * k.ratio[m];
      k.damp[m] = damper * (tube ? 1.0 : 0.6) * std::clamp(std::sqrt(f / 500), 0.7, 2.5);
    }
    k.x = 1.6 * ((i + 0.5) / d.keys - 0.5);
  }
  // The cap's rim under the blow: a short metallic tick (the wood face's more than the rawhide's).
  const double kn = knob("PJ_BELLS_CLICK", 1.0);
  const double f[3] = {3200, 5100, 7400}, t60[3] = {0.02, 0.015, 0.01}, g[3] = {0.5, 0.4, 0.3};
  for (int i = 0; i < 3; ++i) { d.knockF[i] = f[i]; d.knockT[i] = t60[i]; d.knockG[i] = kn * g[i]; }
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
    k.damperWant = d_.dampers ? 1 : 0;
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
    k.engage = k.damperWant;
    coefs(k, d_.key[i]);
  }
  nActive_ = 0;
  prevL_ = prevR_ = side_ = 0;
  knockIn_ = 0;
  for (Case& c : case_) c.x = c.v = 0;
}

void Bars::retune(int key, double cents) {
  if (key < 0 || key >= n_) return;
  Key& k = key_[key];
  const BarKey& b = d_.key[key];
  k.cents = cents;
  const double f1 = b.f1 * std::exp2(cents / 1200);
  for (int m = 0; m < k.nm; ++m) {
    const double wr = 2 * kPi * f1 * b.ratio[m];
    k.w2[m] = float(wr * wr);
  }
  coefs(k, b);
}

void Bars::strike(int key, double speed, int face) {
  if (key < 0 || key >= n_) return;
  Key& k = key_[key];
  const BarKey& b = d_.key[key];
  // The face that meets it.
  const bool second = face == 1 && b.mallet2 > 0;
  k.mMass = float(second ? b.mallet2 : b.mallet);
  k.mK = float(second ? b.K2 : b.K);
  k.mP = float(second ? b.p2 : b.p);
  k.mHyst = float(second ? b.hyst2 : b.hyst);
  k.knockGain = float(b.knock[second ? 1 : 0]);
  // Where the mallet lands this time, and from which side (a doublet's share).
  const double stray = 2 * urand(rng_) - 1;
  const double th = b.theta0 + b.thetaVary * (2 * urand(rng_) - 1), ct = std::cos(th), st = std::sin(th);
  double g = 0;
  for (int m = 0; m < k.nm; ++m) k.hit[m] = float(b.hit[m] + stray * b.hitVary[m]);
  for (int m = 0; m < k.nm; ++m)
    if (b.pair[m] > m && b.pair[m] < k.nm) {
      const float h = k.hit[m];
      k.hit[m] = float(h * ct);
      k.hit[b.pair[m]] = float(h * st);
    }
  for (int m = 0; m < k.nm; ++m) g += double(k.hit[m]) * k.hit[m];
  if (!k.active) { k.active = true; k.quiet = 0; active_[nActive_++] = key; }
  if (b.pluck) {
    // Bent by the pin and let go: each mode starts from where the bending
    // force held it (on top of whatever it was doing).
    const double force = b.K * speed;
    for (int m = 0; m < k.nm; ++m) k.q[m] += float(k.hit[m] * force / k.w2[m]);
    knockIn_ += float(force * fs_ * 0.02);
    return;
  }
  k.G = float(dt_ * dt_ * (g + 1 / k.mMass));
  double h = 0;
  for (int m = 0; m < k.nm; ++m) h += double(k.hit[m]) * k.q[m];
  k.hz = h + 1e-5;  // from above, a hair over the piece as it is now
  k.hv = -speed;
  k.hammerOn = true;
  k.dPrev = 0;
}

void Bars::damper(int key, float depth) {
  if (key < 0 || key >= n_ || !d_.dampers) return;
  Key& k = key_[key];
  k.damperWant = std::clamp(depth, 0.0f, 1.0f);
  if (!k.active && k.engage != k.damperWant) { k.engage = k.damperWant; coefs(k, d_.key[key]); }
}

float Bars::sample() {
  float l = 0, r = 0;
  const float dt = float(dt_);
  for (int a = 0; a < nActive_; ++a) {
    const int ki = active_[a];
    Key& k = key_[ki];
    const BarKey& b = d_.key[ki];
    // The damper coming down or going up.
    const float want = k.damperWant;
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
        const double hyst = 1 + k.mHyst * std::clamp((free - k.dPrev) / dt_, -2.0, 6.0);
        const double K = k.mK, P = k.mP;
        for (int it = 0; it < 34; ++it) {
          d = 0.5 * (lo + hi);
          if (d + k.G * K * fastPow(d, P) * hyst > free) hi = d; else lo = d;
        }
        force = std::max(0.0, K * fastPow(d, P) * hyst);
      }
      k.dPrev = float(d);
      knockIn_ += float(force) * k.knockGain;
      k.hv += dt_ * force / k.mMass;
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
