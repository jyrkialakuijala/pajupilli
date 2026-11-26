#include "piano.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "pianoboard_data.h"
#include "vmath.h"

namespace pajupilli {

namespace {

constexpr int kRing = 512;
static_assert(pianoboard::kPatches <= 64, "more patches than Piano::kMaxPatches");

// The wound bass strings' speaking lengths (m), keys 1 .. 20 (the plain
// strings' lengths follow from their stiffness, below).
double bassLength(int key) { return 2.01 * fastPow(1.33 / 2.01, (key - 1) / 19.0); }

// The strings' stiffness B (partial n at n f0 sqrt(1 + B n^2)) as measured
// on two recorded concert grands (research/piano.md, 11): doubling every
// 8.2 keys from the lowest plain string up; the wound bass least near key 13.
double stiffness(int key) {
  if (key > 20) return 7.5e-5 * std::exp2((key - 24) / 8.2);
  const double a = key <= 13 ? (key - 1) / 12.0 : 1 + (key - 13) / 7.0;
  return key <= 13 ? 8.0e-5 * fastPow(4.5e-5 / 8.0e-5, a) : 4.5e-5 * fastPow(5.5e-5 / 4.5e-5, a - 1);
}

double lerp(double a, double b, double t) { return a + (b - a) * std::clamp(t, 0.0, 1.0); }

}  // namespace

PianoDesign grandDesign() {
  PianoDesign des;
  PianoKey* sc = des.key;
  static const double kE = 2.02e11, kRho = 7850;
  const double dampRate = knob("PJ_PIANO_DAMPER", 1.0);
  for (int key = 1; key <= 88; ++key) {
    PianoKey& s = sc[key - 1];
    s.strings = key <= 8 ? 1 : key <= 13 ? 2 : 3;
    const double f = 440 * std::exp2((key - 49) / 12.0);
    s.B = stiffness(key);
    if (key <= 20) {  // copper-wound: the tension chosen, the winding makes up the mass, the core the stiffness
      s.len = bassLength(key);
      s.tension = 1300 - 15.0 * (key - 1);
      s.mu = s.tension / ((2 * s.len * f) * (2 * s.len * f));
    } else {  // plain steel, 1.2 mm thinning to 0.775 mm: as long as its stiffness says (1.90 m .. 53 mm)
      const double d = lerp(1.2e-3, 0.775e-3, (key - 21) / 67.0);
      s.mu = kRho * M_PI * d * d / 4;
      s.len = std::pow(M_PI * M_PI * M_PI * kE * d * d * d * d / (256 * s.mu * f * f * s.B), 0.25);
      s.tension = s.mu * (2 * s.len * f) * (2 * s.len * f);
    }
    s.strike = key <= 52 ? 0.122 : lerp(0.122, 0.07, (key - 52) / 36.0);
    // The hammers: 10.5 g in the bass to 4.5 g at the top. The felt's
    // hardening (the exponent) steeper towards the treble (Chaigne &
    // Askenfelt's C2, C4, C7); its stiffness theirs in the bass, and from
    // the middle up as voiced on two recorded grands: three to ten times
    // softer than their C4 and C7, by how little of a note's sound those
    // pianos have above its second partial (research/piano.md, 11).
    // The hammers' strike weights (the mass the string meets, the shank's share included):
    // a published medium strike-weight set (PTG Journal, April 2000, Table I: 11.9 g at A0, 11.6 g at key 9,
    // 6.4 g at key 80, 5.5 g at C8), smooth between (log-linear), as a voiced action's are.
    // Heavier than the 10.5 .. 4.5 g used before: the wound strings' hammers by ~1.5-2 g, the top's
    // by ~1 g. PJ_PIANO_SW scales them (that study's light 0.94, heavy 1.06); PJ_PIANO_WOUNDMH
    // adds to the wound strings' alone (0).
    {
      static const double kk[] = {1, 9, 80, 88}, kw[] = {11.9, 11.6, 6.4, 5.5};
      double w = kw[3];
      for (int i = 0; i < 3; ++i)
        if (key <= kk[i + 1]) {
          w = std::exp(lerp(std::log(kw[i]), std::log(kw[i + 1]), (key - kk[i]) / (kk[i + 1] - kk[i])));
          break;
        }
      static const double kSw = knob("PJ_PIANO_SW", 1.0), kWoundMh = knob("PJ_PIANO_WOUNDMH", 0.0);
      s.mh = 1e-3 * w * kSw + (key <= 20 ? kWoundMh : 0.0);
    }
    static const double pk[] = {1, 16, 40, 76, 88}, hp[] = {2.2, 2.3, 2.5, 3.0, 3.1};
    static const double hk[] = {1, 16, 30, 49, 64, 76, 88};
    static const double hK[] = {1.5e8, 4.0e8, 1.3e9, 1.7e9, 2.5e10, 1.0e12, 3.0e12};
    for (int i = 0; i < 4; ++i)
      if (key <= pk[i + 1]) {
        s.p = lerp(hp[i], hp[i + 1], (key - pk[i]) / (pk[i + 1] - pk[i]));
        break;
      }
    for (int i = 0; i < 6; ++i)
      if (key <= hk[i + 1]) {
        s.K = std::exp(lerp(std::log(hK[i]), std::log(hK[i + 1]), (key - hk[i]) / (hk[i + 1] - hk[i])));
        break;
      }
    // The voicer's work across the bass break (research/piano.md, 12): the plain strings from F2
    // up are lighter than the wound ones below and give under the same hammer, which then
    // presses its felt less and meets it softer: with one law of felt across the break the
    // wound notes had ~4 dB more and the plain ones 7-15 dB less in partials 5-16 than a
    // recorded grand, whose balance runs through the break without a step. The felt x
    // PJ_PIANO_KV21 at key 21, x KV30, x KV40 (C4), x KV52, 1 from key 64; the wound strings' x
    // PJ_PIANO_KVBASS at key 20, 1 at key 8 and below (log-linear between).
    {
      static const double kv[] = {knob("PJ_PIANO_KV21", 1.0), knob("PJ_PIANO_KV30", 1.0), knob("PJ_PIANO_KV40", 1.0),
                                  knob("PJ_PIANO_KV52", 1.0), 1.0};
      static const double kvBass = knob("PJ_PIANO_KVBASS", 1.0);
      static const double vk[] = {21, 30, 40, 52, 64};
      if (key <= 20) {
        s.K *= fastPow(kvBass, std::clamp((key - 8) / 12.0, 0.0, 1.0));
      } else {
        for (int i = 0; i < 4; ++i)
          if (key <= vk[i + 1]) {
            s.K *= std::exp(lerp(std::log(kv[i]), std::log(kv[i + 1]), (key - vk[i]) / (vk[i + 1] - vk[i])));
            break;
          }
      }
    }
    // A hammer is soft at its surface and hard underneath: the felt
    // hardens far more steeply than the measured power laws say when it is
    // pressed deep. As soft as fitted at a gentle blow's force (kPivot newtons
    // a string), the exponent raised from there: a middling blow meets felt
    // twice as stiff, a hard one five times, and a loud note has its upper
    // partials (with the fitted softness at every force it kept only the
    // first two: hollow, like a clarinet). (Applied when the piano is built.)
    // (Less of it in the upper register: there a hard blow on felt that hard
    // turned glassy, three partials level with the fundamental, where the
    // recorded grands keep the fundamental well over the rest at any blow.)
    s.colour = lerp(knob("PJ_PIANO_COLOUR", 2.0), knob("PJ_PIANO_COLOURTOP", 0.8), (key - 40) / 24.0);
    // The top octave and a half: soft again at a gentle blow (hard felt on a light hammer
    // gave even the softest notes there a clear second partial).
    s.colour += knob("PJ_PIANO_COLOURHIGH", 2.0) * std::clamp((key - 68) / 8.0, 0.0, 1.0);
    s.K *= fastPow(knob("PJ_PIANO_KHIGH", 0.3), std::clamp((key - 68) / 8.0, 0.0, 1.0));
    // The top two octaves (from C6, fully from C7): rounder, softer heads, voiced for pearls,
    // not glass: the felt softer still (x PJ_PIANO_KTOP2, 0.5) and hardening less under a hard
    // blow (its colour - PJ_PIANO_COLOURTOP2, 1.0); the face rounder (Piano: the contact wider and
    // tapering, PJ_PIANO_HAMROUND).
    {
      const double soft2 = std::clamp((key - 64) / 12.0, 0.0, 1.0);
      s.K *= fastPow(knob("PJ_PIANO_KTOP2", 0.5), soft2);
      s.colour = std::max(0.0, s.colour - knob("PJ_PIANO_COLOURTOP2", 1.0) * soft2);
    }
    s.pivot = knob("PJ_PIANO_PIVOT", 2.0);
    // The keys evened across the bass break: the board gives the lowest plain strings ~3 dB more
    // than the top wound ones (their partials 2-4, which the long bridge's end radiates better).
    // PJ_PIANO_BREAKEVEN (dB): the wound keys up by it at key 20, none at key 12; the plain ones
    // down by it at key 21, none at key 30.
    {
      static const double kEven = knob("PJ_PIANO_BREAKEVEN", 0.0);
      s.levelDb += key <= 20 ? kEven * std::clamp((key - 12) / 8.0, 0.0, 1.0) : -kEven * std::clamp((30 - key) / 9.0, 0.0, 1.0);
    }
    {
      // The nap: thinner and firmer up the keyboard, as the hammers are lighter and the notes' periods shorter.
      const double oct = (key - 40) / 12.0;
      s.nap = knob("PJ_PIANO_NAP", 1.4e-3) * std::exp2(-knob("PJ_PIANO_NAPTHIN", 1.2) * std::max(oct, 0.0));
      s.napK = knob("PJ_PIANO_NAPK", 1200) * std::exp2(oct * (oct > 0 ? knob("PJ_PIANO_NAPSLOPE", 2.5) : knob("PJ_PIANO_NAPSLOPEBASS", 0.8)));
    }
    s.loss = knob("PJ_PIANO_LOSS", 0.05) * std::exp2(3.5 * (key - 1) / 87.0);
    // The loss's rise with frequency (1/s per Hz): the wire's own Q; the wound bass strings lose more.
    s.b2 = lerp(knob("PJ_PIANO_LOSS2BASS", 3.5e-4), knob("PJ_PIANO_LOSS2", 2e-4), (key - 16) / 24.0);
    s.b3 = knob("PJ_PIANO_LOSS3", 3e-8);    // and with frequency^2 (1/s per Hz^2)
    s.damper = s.strike + 0.035;
    // (the felt's length along the strings: ~75 mm in the bass, still ~55 mm at the last damper, as
    // a 274 cm concert grand's heads are seen from above)
    s.damperLen = lerp(0.075, 0.055, (key - 1) / 69.0);
    // The damper: a length of felt; a mode loses by its mean square motion under it.
    // (A released note is gone in about 0.15 s in the middle, 0.3 s in the bass: from memory, not
    // measured. A key struck again sooner meets what is left of its own sound.) A 274 cm concert grand has 71:
    // A0 .. G6; the top 17 keys (G#6 .. C8) have none and ring on, with the pedal or without.
    s.hasDamper = key <= 71;
    s.damperRate = dampRate * lerp(16, 38, (key - 1) / 69.0) / 0.2;
    // What lands: the head (felt on a wooden block, double wedges in the bass), its wire and the
    // underlever's share at the head, ~20 g in the bass to ~12 g at the last damper (the heads
    // grow lighter up the keyboard; the task's and the technicians' 10-20 g).
    s.damperMass = lerp(0.020, 0.012, (key - 1) / 69.0);
  }
  des.pedalNoise = knob("PJ_PIANO_PEDALNOISE", 1.0);
  des.pedalUp = knob("PJ_PIANO_PEDALUP", des.pedalUp);
  des.keyKnock = knob("PJ_PIANO_KEYKNOCK", 0);  // (through the board: weak; the knock is heard through the air, PianoModel)
  des.longShare = knob("PJ_PIANO_LONG", 0.15);  // (the stretching's push on the bridge: its share moving the board at 1 kHz)
  des.damperContact = knob("PJ_PIANO_DAMPERCONTACT", des.damperContact);  // (the felt's cushion, s)
  des.rimShare = knob("PJ_PIANO_RIMSHARE", des.rimShare);                // (a knock in the case reaching the board)
  tuneByPartials(&des);
  const double mistune = knob("PJ_PIANO_MISTUNE", 1.0);  // the unisons' spread (x the usual)
  des.unisonCents[0] = mistune * 0.08;
  des.unisonCents[1] = mistune * 0.03;
  // (How far off the vertical the blow sets the string swinging: the
  // hammer's face, the string's seat on the bridge's pins.)
  des.tilt = knob("PJ_PIANO_TILT", 0.4);
  des.sideTuneCents = knob("PJ_PIANO_SIDETUNE", 1.0);
  des.sideLoad = knob("PJ_PIANO_SIDE", 0.4);  // the bridge's share of the sideways swing
  des.top = knob("PJ_PIANO_TOP", 10000);
  des.topTreble = knob("PJ_PIANO_TOPTREBLE", 20000);
  des.hammerWidth = knob("PJ_PIANO_HAMW", 0.010);
  des.board.n = pianoboard::kModes;
  des.board.patches = pianoboard::kPatches;
  des.board.freq = pianoboard::kFreq;
  des.board.loss = pianoboard::kLoss;
  des.board.phi = pianoboard::kPhi;
  des.board.patchVol = pianoboard::kPatchVol;
  des.board.patchPos = pianoboard::kPatchPos;
  des.boardLoss = knob("PJ_PIANO_ETA", 1.0);  // the board's own loss (x the wood's and the air's)
  des.stringLoad = knob("PJ_PIANO_STRINGLOAD", 1.0);
  // The two microphones: pressure-gradient ("differential", figure-of-eight)
  // ones, close together over the strings 50 cm from the keyboard's end of
  // the board (nearer, the short treble strings' end of the board is heard as
  // evenly as the rest) and 65 cm above it, one turned 45 degrees towards the bass
  // side, the other towards the treble side (a crossed pair). Each hears the
  // difference of the pressure across itself: a patch of the board by the
  // cosine of its angle off the microphone's axis, behind 90 degrees with
  // its sign turned. A partial sits in the picture where the board moves
  // with it; the two together are one such microphone looking straight down
  // (no gaps from two distances).
  const double mx = knob("PJ_PIANO_MICX", 0.5), mz = knob("PJ_PIANO_MICZ", 0.65);
  const double myc = knob("PJ_PIANO_MICY", 0.70), space = knob("PJ_PIANO_MICSPACE", 0.17);
  const double ang = knob("PJ_PIANO_MICANGLE", 60), pat = knob("PJ_PIANO_MICPATTERN", 1.0);
  for (int c = 0; c < 2; ++c) {
    const double sgn = c ? 1 : -1;
    des.mic[c] = {mx, myc + sgn * space / 2, mz, sgn * ang, pat};
  }
  return des;
}

void concertScale(PianoDesign* d) {
  // The string from the bridge to the plate, a share of the speaking length, key by key (A0 .. C8).
  static constexpr double S = 1.0 / 6, H = 1.0 / 3, Q = 1.0 / 2;
  static constexpr double kRear[88] = {
      S, S, S, S, S, S, S, S, S, S, S, S,  // A0 .. G#1
      S, S, S, S, S, S, S, S, S, S, S, S,  // A1 .. G#2
      S, S, S, S, S, S, S, S, S, S, S, S,  // A2 .. G#3
      S, S, S, S, S, S, S, S, S, S, S, S,  // A3 .. G#4
      S, S, S, S, S, S, S, S, S, S, S, S,  // A4 .. G#5
      S, S, S, S, H, H, H, H, H, H, H, H,  // A5 .. C6 at 1/6, C#6 .. G#6 at 1/3
      H, H, H, H, Q, Q, Q, Q, Q, Q, Q, Q,  // A6 .. C7 at 1/3, C#7 .. G#7 at 1/2
      Q, Q, Q, Q};                         // A7 .. C8
  for (int key = 0; key < 88; ++key) d->key[key].rear = kRear[key];
  static constexpr double kLen[88] = {
      2.010, 2.009, 2.008, 2.007, 1.997, 1.981, 1.965, 1.938, 1.911, 1.879, 1.842, 1.805,
      1.762, 1.709, 1.655, 1.602, 1.548, 1.495, 1.442, 1.378, 1.837, 1.757,
      1.660, 1.591, 1.482, 1.403, 1.329, 1.259, 1.192, 1.129, 1.070, 1.013, 0.960, 0.909,
      0.861, 0.816, 0.773, 0.732, 0.694, 0.657, 0.622, 0.590, 0.559, 0.529,
      0.501, 0.475, 0.450, 0.426, 0.404, 0.383, 0.363, 0.344, 0.326, 0.308, 0.292, 0.277,
      0.262, 0.249, 0.236, 0.223, 0.211, 0.200, 0.190, 0.180, 0.171, 0.162,
      0.153, 0.145, 0.138, 0.130, 0.124, 0.117, 0.111, 0.105, 0.100, 0.095, 0.090, 0.085,
      0.081, 0.076, 0.072, 0.069, 0.065, 0.062, 0.058, 0.055, 0.052, 0.049};
  // Each string's tension (kgf), A0 to C8: about 20.4 tonnes over the 243 strings.
  static constexpr double kKgf[88] = {
      116.37, 115.15, 113.93, 112.70, 111.48, 110.26, 109.03, 107.81, 106.59, 105.37, 104.14, 102.92,
      101.70, 100.47, 99.25, 98.03, 96.81, 95.58, 94.36, 93.14, 87.17, 87.44, 87.72, 87.99,
      84.56, 84.83, 85.10, 85.37, 81.96, 82.23, 82.50, 82.77, 79.39, 79.66, 79.92, 80.19,
      73.31, 73.56, 73.80, 74.05, 70.81, 71.05, 71.30, 71.54, 68.33, 68.57, 68.81, 69.05,
      73.75, 74.76, 75.79, 76.83, 77.88, 78.96, 75.99, 77.04, 78.10, 79.18, 80.27, 81.38,
      78.22, 79.31, 80.41, 81.52, 82.65, 83.80, 80.44, 81.56, 82.69, 83.85, 85.02, 86.20,
      82.62, 83.78, 84.95, 86.14, 87.35, 88.57, 84.76, 85.95, 87.16, 88.38, 89.63, 90.89,
      81.65, 82.80, 83.97, 85.15};
  for (int key = 1; key <= 88; ++key) {
    PianoKey& s = d->key[key - 1];
    const double f = 440 * std::exp2((key - 49) / 12.0), c = 2 * kLen[key - 1] * f;
    s.len = kLen[key - 1];
    // (PJ_PIANO_TENSIONS: 1, the table's tension on the wound notes; 2, on every note, the mass
    // following; 0, none. The plain wire's table comes from another gauge layout than kWire below:
    // length, pitch, gauge and tension cannot all hold, so by default its gauge and length do.)
    static const int kTensions = int(knob("PJ_PIANO_TENSIONS", 1));
    const double tableT = kKgf[key - 1] * 9.80665;
    if (key <= 20) {  // (the 20 wound notes: 8 single, 5 double, 7 triple strings)
      if (kTensions > 0) s.tension = tableT;
      s.mu = s.tension / (c * c);
      continue;
    }
    // The plain wire by its gauge, F2 up: the last key of each size, and the size (#13 is 0.775 mm,
    // each half size 0.025 mm more). PJ_PIANO_WIREB=1: the stiffness from this wire too, instead of
    // the two recorded grands' (research/piano.md, 11).
    static constexpr struct { int last; double gauge; } kWire[] = {
        {24, 21}, {30, 20}, {37, 19}, {43, 18}, {51, 17}, {57, 16.5},
        {63, 16}, {71, 15.5}, {76, 15}, {80, 14.5}, {84, 14}, {88, 13.5}};
    double gauge = 13.5;
    for (const auto& w : kWire)
      if (key <= w.last) { gauge = w.gauge; break; }
    const double wire = (0.775 + (gauge - 13) * 0.05) * 1e-3;
    s.mu = 7850 * M_PI * wire * wire / 4;
    s.tension = s.mu * c * c;
    if (kTensions > 1) {
      s.tension = tableT;
      s.mu = s.tension / (c * c);
    }
    static const bool kWireB = knob("PJ_PIANO_WIREB", 0) != 0;
    if (kWireB) s.B = M_PI * M_PI * M_PI * 2.02e11 * wire * wire * wire * wire / (64 * s.tension * s.len * s.len);
  }
  // A graded break (PJ_PIANO_BREAKBLEND, keys each side of E2 / F2; 0: none): the copper thins out
  // over the top wound keys and a light winding goes on over the lowest plain ones, so length,
  // tension and mass per length pass from the bass scale's line to the plain wire's in even steps
  // instead of at one key (the mass halved between E2 and F2).
  static const double kBlend = knob("PJ_PIANO_BREAKBLEND", 0);
  if (kBlend > 0) {
    const double rw = kLen[19] / kLen[18], rp = kLen[20] / kLen[21];
    for (int key = 1; key <= 88; ++key) {
      const double w = (key - (20.5 - kBlend)) / (2 * kBlend);
      if (w <= 0 || w >= 1) continue;
      PianoKey& s = d->key[key - 1];
      const double f = 440 * std::exp2((key - 49) / 12.0);
      const double lw = key <= 20 ? kLen[key - 1] : kLen[19] * fastPow(rw, key - 20);
      const double lp = key >= 21 ? kLen[key - 1] : kLen[20] * fastPow(rp, 21 - key);
      const double tw = (key <= 20 ? kKgf[key - 1] : kKgf[19] + (kKgf[19] - kKgf[18]) * (key - 20)) * 9.80665;
      const double tp = key >= 21 ? s.tension : d->key[20].tension;
      s.len = std::exp(std::log(lw) * (1 - w) + std::log(lp) * w);
      s.tension = tw * (1 - w) + tp * w;
      s.mu = s.tension / ((2 * s.len * f) * (2 * s.len * f));
    }
  }
}

void tuneByPartials(PianoDesign* d, int lo, int hi, bool byRegister) {
  PianoKey* sc = d->key;
  const int keys = d->keys;
  // Tuned as a tuner does: an octave in the middle by the fundamental, then
  // octave by octave, a partial of the lower note against one of the upper
  // (2 with 1 going up, 6 with 3 going down, as the recorded grands sit):
  // the stiff wires' sharp partials stretch the octaves.
  auto part = [&](int key, int n) { return n * std::sqrt((1 + sc[key - 1].B * n * n) / (1 + sc[key - 1].B)); };
  for (int key = lo; key <= hi; ++key) sc[key - 1].f1 = 440 * std::exp2((key - 49) / 12.0);
  // (byRegister: each octave by the partials a tuner takes there, the key being tuned deciding:
  // 10:5 to G#1, 8:4 to E2, 6:3 to C3, 4:2 to C4, 2:1 to D7, 4:2 above.)
  const auto upper = [](int key) { return key <= 12 ? 5 : key <= 20 ? 4 : key <= 28 ? 3 : key <= 40 ? 2 : key <= 78 ? 1 : 2; };
  for (int key = hi + 1; key <= keys; ++key) {
    const int m = byRegister ? upper(key) : 1;
    sc[key - 1].f1 = sc[key - 13].f1 * part(key - 12, 2 * m) / part(key, m);
  }
  for (int key = lo - 1; key >= 1; --key) {
    const int m = byRegister ? upper(key) : 3;
    sc[key - 1].f1 = sc[key + 11].f1 * part(key + 12, m) / part(key, 2 * m);
  }
}

Piano::Piano(double fs, const PianoDesign& design) : d_(design), fs_(fs), dt_(float(1 / fs)) { build(); }

Piano::~Piano() { std::free(mem_); }

void Piano::build() {
  nKeys_ = std::min(d_.keys, int(kKeys));
  const bool board = d_.bridge == PianoDesign::Bridge::kBoard && d_.board.n > 0;
  nB_ = board ? d_.board.n : 0;
  nP_ = board ? std::min(d_.board.patches, int(kMaxPatches)) : 0;
  pickup_ = d_.output == PianoDesign::Output::kPickup || !board;
  kSide_ = float(d_.sideLoad);
  {  // (PJ_PIANO_FORCEDUMP=file: the strings' force on the bridge, raw float32 per sample, of the key PJ_PIANO_FORCEKEY, 0 .. 87)
    static FILE* dump = std::getenv("PJ_PIANO_FORCEDUMP") ? std::fopen(std::getenv("PJ_PIANO_FORCEDUMP"), "wb") : nullptr;
    dump_ = dump;
    dumpKey_ = dump ? int(knob("PJ_PIANO_FORCEKEY", -1)) : -1;
  }
  pickSide_ = float(d_.pickupSide);
  rng_ = d_.seed;
  const int kB = nB_, kP = nP_, kKeys = nKeys_;
  // The felt, hardening more steeply from a gentle blow's force up.
  PianoKey sc[PianoDesign::kMaxKeys];
  for (int k = 0; k < kKeys; ++k) {
    sc[k] = d_.key[k];
    if (sc[k].colour != 0) {
      const double dRef = fastPow(sc[k].pivot / sc[k].K, 1 / sc[k].p);  // the felt's compression at that force (m)
      sc[k].K *= fastPow(dRef, -sc[k].colour);
      sc[k].p += sc[k].colour;
    }
  }
  // How many modes each string has: in whole registers of 16.
  const double top = d_.top, topTreble = std::min(0.45 * fs_, d_.topTreble);
  int count[PianoDesign::kMaxKeys];
  total_ = 0;
  str_.clear();
  for (int k = 0; k < kKeys; ++k) {
    const PianoKey& s = sc[k];
    const double fmax = s.top > 0 ? std::min(0.45 * fs_, s.top) : lerp(top, topTreble, (k + 1 - 40) / 12.0);
    const double f0 = s.f1 / std::sqrt(1 + s.B);
    int n = 0;
    while (n < 176 && (n + 1) * f0 * std::sqrt(1 + s.B * (n + 1) * (n + 1)) < fmax) ++n;
    count[k] = (std::max(n, 1) + 15) / 16 * 16;
    key_[k].s0 = int(str_.size());
    key_[k].ns = s.strings;
    key_[k].f1 = s.f1;
    key_[k].B = s.B;
    key_[k].len = s.len;
    key_[k].mh = float(s.mh);
    key_[k].K = float(s.K);
    key_[k].p = float(s.p);
    key_[k].nap = float(s.nap);
    key_[k].napK = float(s.napK);
    key_[k].napPack = float(d_.napPack);
    if (d_.rearGain > 0 && d_.key[k].rear > 0) {
      // (The same wire at the same tension, rear x as long: rear partials of 1 / rear x the pitch,
      // each string's a few cents its own: the plate end is not tuned.)
      const PianoKey& q = d_.key[k];
      const double Ld = q.rear * q.len, fd = q.f1 / q.rear, Bd = q.B / (q.rear * q.rear);
      Key& K = key_[k];
      K.rearN = 0;
      for (int j = 0; j < q.strings; ++j) {
        const double cents = (j - 0.5 * (q.strings - 1)) * 5 + 3 * std::sin(k * 2.399 + j);
        for (int m = 1; m <= Key::kRear; ++m) {
          const double f = m * fd * std::sqrt(1 + Bd * m * m) * std::exp2(cents / 1200);
          if (f > std::min(12000.0, 0.45 * fs_)) break;
          const int i = K.rearN++;
          const double w = 2 * M_PI * f, g = std::exp(-w / (2 * d_.rearQ) / fs_), sign = m % 2 ? 1 : -1;
          K.rc[i] = float(g * std::cos(w / fs_));
          K.rs[i] = float(g * std::sin(w / fs_));
          K.rin[i] = float(-2 * sign / (m * M_PI) * w * d_.rearBridge);
          K.rout[i] = float(q.mu * Ld * sign / (m * M_PI) * d_.rearGain);
          K.rdrive[i] = float(d_.rearOwn * 2 / (q.mu * Ld) * sign / m / q.strings);
        }
      }
    }
    key_[k].inBack = float(1 / (d_.bridgeIn * fastPow(d_.bridgeBass, std::clamp((72 - (k + 21)) / 24.0, 0.0, 1.0))));
    if (d_.bassLength > 0 && d_.keys == 88) {
      // (over seven keys in decibels: no step at the bass break, where the strings grow longer again)
      double db = 0;
      int n = 0;
      for (int q = std::max(0, k - 3); q <= std::min(87, k + 3); ++q, ++n)
        db += 20 * d_.bassLength * std::log10(std::max(d_.key[q].len / d_.key[39].len, 1.0));
      key_[k].lenGain = float(fastPow(10.0, db / n / 20));
    }
    if (s.levelDb != 0) key_[k].lenGain *= float(fastPow(10.0, s.levelDb / 20));
    key_[k].invLenGain = 1 / key_[k].lenGain;
    // (PJ_PIANO_DGRIP: the felt's grip on the up-and-down swing where the strings' coupling was lowered)
    static const float kGrip = float(knob("PJ_PIANO_DGRIP", 0.5));
    key_[k].grip = kGrip * (1 - 1 / key_[k].inBack);
    key_[k].glideN = k % 32;  // (the keys' looks at their tension spread over the 32 samples)
    key_[k].pluckK = float(s.pluckK);
    key_[k].pluckLimit = float(s.pluckLimit);
    key_[k].pluckSide = float(s.pluckSide);
    key_[k].pluckSoft = float(s.pluckSoft);
    key_[k].pluckSnap = float(s.pluckSnap);
    key_[k].pluckSnapLo = float(s.pluckSnapLo);
    key_[k].pluckSnapHi = float(s.pluckSnapHi);
    key_[k].pluckSnapTilt = float(s.pluckSnapTilt);
    dContact_ = knob("PJ_PIANO_DCONTACT", 1) != 0;
    dLin_ = float(knob("PJ_PIANO_DLIN", 0.2));
    dLinN_ = int(0.02 * fs_);
    dFeltK_ = float(knob("PJ_PIANO_DFELTK", 300));
    dHold_ = float(knob("PJ_PIANO_DHOLD", 0));
    key_[k].hasDamper = s.hasDamper;
    key_[k].damperDown = key_[k].hasDamper;
    key_[k].engage = key_[k].hasDamper ? 1 : 0;
    // (the damper as a contact: its head's mass, and its weight on the strings, x PJ_PIANO_DFORCE
    // for the underlever's share and any spring)
    {
      static const double kDForce = knob("PJ_PIANO_DFORCE", 2.0);
      key_[k].dM = float(std::max(1e-3, s.damperMass));
      key_[k].dW = float(kDForce * 9.81 * std::max(1e-3, s.damperMass));
    }
    for (int j = 0; j < s.strings; ++j) {
      Str st;
      st.key = k;
      st.off = total_;
      st.n = count[k];
      rng_ = rng_ * 1664525u + 1013904223u;
      const double r = (rng_ >> 8) / 16777216.0;
      st.tanH = float((j == 1 ? -1 : 1) * d_.tilt * (0.55 + 0.9 * r));
      total_ += st.n;
      str_.push_back(st);
    }
  }
  // (every array is rounded up to 16 floats: counted so)
  const auto r16 = [](size_t n) { return (n + 15) / 16 * 16; };
  const size_t words = r16(total_) * (17 + 3 * kDPts) + r16(kB) * 12 + r16(size_t(kB) * kKeys) + r16(size_t(kB) * kP) + r16(size_t(kP) * kRing) + 64;
  mem_ = static_cast<float*>(std::aligned_alloc(64, (words * sizeof(float) + 63) / 64 * 64));
  std::memset(mem_, 0, words * sizeof(float));
  float* p = mem_;
  auto take = [&](size_t n) { float* q = p; p += (n + 15) / 16 * 16; return q; };
  x_[0] = take(total_); x_[1] = take(total_); v_[0] = take(total_); v_[1] = take(total_);
  cs_ = take(total_); sn_ = take(total_); cs1_ = take(total_); sn1_ = take(total_); in_ = take(total_); out_ = take(total_);
  damp_ = take(total_); hk_ = take(total_); hc_ = take(total_); hs_ = take(total_); vw_ = take(total_); dk_ = take(total_); nw_ = take(total_);
  for (int q = 0; q < kDPts; ++q) { dsh_[q] = take(total_); dshw_[q] = take(total_); dkp_[q] = take(total_); }
  bx_ = take(kB); bv_ = take(kB); bcs_ = take(kB); bsn_ = take(kB); bf_ = take(kB); ba_ = take(kB); binv_ = take(kB);
  bacc_ = take(kB);
  phi_ = take(size_t(kB) * kKeys);
  pvol_ = take(size_t(kB) * kP);
  ring_ = take(size_t(kP) * kRing);
  outFull_.assign(total_, 0.0f);
  restrikeLn_.assign(total_, 0.0f);
  wdt_.assign(total_, 0.0f); cs0_.assign(total_, 0.0f); sn0_.assign(total_, 0.0f); cs10_.assign(total_, 0.0f); sn10_.assign(total_, 0.0f);
  bloomTau_.assign(total_, 0.0f);
  bloomFloor_.assign(total_, 1.0f);

  const double hamW = d_.hammerWidth;
  uint32_t rng2 = d_.seed * 2654435761u + 1;  // (the out-of-tune piano's own draws: the others stay as they are)
  auto draw2 = [&]() { rng2 = rng2 * 1664525u + 1013904223u; return (rng2 >> 8) / 16777216.0; };
  if (d_.tackHard > 0) {
    // (the tacks: each key's hammer meets one or two of its strings with a metal head, the rest
    // with softer felt around it; its face uneven, each string met at its own moment; its own draws)
    uint32_t rng3 = d_.seed * 40503u + 7;
    auto draw3 = [&]() { rng3 = rng3 * 1664525u + 1013904223u; return (rng3 >> 8) / 16777216.0; };
    for (int k = 0; k < keys(); ++k) {
      const int ns = key_[k].ns;
      const int hit = int(draw3() * ns), two = ns > 2 && draw3() < 0.4 ? (hit + 1 + int(draw3() * (ns - 1))) % ns : -1;
      const double key = 0.5 + draw3();  // (how well this key's tacks sit)
      for (int j = 0; j < ns; ++j) {
        Str& st = str_[key_[k].s0 + j];
        const bool tacked = j == hit || j == two;
        st.tack = float(tacked ? d_.tackHard * key * (0.5 + draw3()) : 0);
        st.hard = float(0.75 + 0.5 * draw3());  // (the felt around the tacks, uneven)
        st.gap = float(d_.tackGap * draw3());
      }
    }
  }
  for (Str& st : str_) {
    const PianoKey& s = sc[st.key];
    const double b2 = s.b2, b3 = s.b3;
    const int j = int(&st - &str_[key_[st.key].s0]);
    // The unison: the strings of a key a little apart (cents): the middle
    // one true, the outer ones either side, never quite evenly.
    rng_ = rng_ * 1664525u + 1013904223u;
    const double r = (rng_ >> 8) / 16777216.0;
    // (PJ_PIANO_UNIDET >= 0: every unison exactly that far apart (cents), for the coupling study)
    static const double kUniDet = knob("PJ_PIANO_UNIDET", -1);
    const double spread = kUniDet >= 0 ? kUniDet : lerp(d_.unisonCents[0], d_.unisonCents[1], st.key / 87.0) * (0.7 + 0.6 * r);
    double cents = s.strings == 1 ? 0 : s.strings == 2 ? (j == 0 ? -0.5 : 0.5) * spread : (j - 1) * spread;
    // (an out-of-tune piano: a trichord's outer strings either side of its true middle one, by a
    // spread growing from the bass (x 0.67) to the treble (x 1.33), each key x 0.6 .. 1.4; the
    // wound bichords clean)
    if (d_.honkyCents > 0 && s.strings == 3 && j != 1) {
      const double reg = 0.67 + 0.66 * std::clamp((st.key - 20) / 60.0, 0.0, 1.0);
      cents += (j == 0 ? -1 : 1) * d_.honkyCents * reg * (0.6 + 0.8 * draw2());
    }
    // (or by hertz, every string of a key its own: the outer ones about honkyHz either side, the
    // middle one of three a little; the beats about as fast all over the keyboard, capped at 20 c
    // (PJ_HONKY_CAP: 1.5 Hz is 40 c at C2, sour))
    if (d_.honkyHz > 0 && s.strings > 1) {
      const double side = s.strings == 2 ? (j == 0 ? -1 : 1) : j - 1.0;
      const double hz = (side != 0 ? side * (0.6 + 0.8 * draw2()) : 0.4 * (draw2() - 0.5) * 2 * 0.5) * d_.honkyHz;
      static const double kCap = knob("PJ_HONKY_CAP", 20.0);
      cents += std::clamp(1200 * std::log2(1 + hz / s.f1), -kCap, kCap);
    }
    // (the treble's strings drift flat too, each its own: uniform 0 .. -1 Hz above middle C,
    // 0 .. -2 Hz above C5; PJ_HONKY_TREBLEFLAT scales it)
    if (d_.honkyHz > 0 && st.key + 21 > 60) {
      static const double kTf = knob("PJ_HONKY_TREBLEFLAT", 1.0);
      const double hz = (st.key + 21 > 72 ? 2.0 : 1.0) * kTf * draw2();
      cents += 1200 * std::log2(1 - hz / s.f1);
    }
    if (d_.flatShare > 0 && draw2() < d_.flatShare) {
      // (a string gone flat: by up to flatMax cents, or by about flatHz hertz, x 0.7 .. 1.3,
      // capped at 40 c)
      if (d_.flatHz > 0) cents -= std::min(40.0, -1200 * std::log2(1 - std::min(0.5, d_.flatHz * (0.7 + 0.6 * draw2()) / s.f1)));
      else cents -= d_.flatMax * draw2();
    }
    // (A support that does not move takes from every mode alike.)
    const double support = nB_ ? 0.0 : s.tension / (s.len * std::max(d_.supportR, 1.0));
    const double supportFc = d_.supportFc;
    const double f0 = s.f1 / std::sqrt(1 + s.B) * std::exp2(cents / 1200);
    const double modalMass = s.mu * s.len / 2;
    // Sideways the string is tuned a little apart from up-and-down (the
    // bridge gives differently the two ways): a cent or so, either way.
    // Without that the two swings, pushing one bridge at one pitch, would
    // settle at once into the one mix that moves it and the one that does
    // not (silent): no aftersound, a tone as steady as a wind instrument's.
    // Apart, the sideways swing is heard by itself: weaker, far longer, and
    // drifting against the other.
    const double kSideTune = d_.sideTuneCents;
    rng_ = rng_ * 1664525u + 1013904223u;
    const double rs = (rng_ >> 8) / 16777216.0;
    const double sideTune = std::exp2((rs < 0.5 ? -1 : 1) * kSideTune * (0.5 + std::fmod(rs * 2, 1.0)) / 1200);
    const double rate0 = s.damperRate;
    double comply = 0;
    for (int i = 0; i < st.n; ++i) {
      const int n = i + 1, m = st.off + i;
      // The top two octaves (from C6, fully from C7) less stiff than measured: their partials
      // nearer whole multiples (B x (1 - PJ_PIANO_TOPHARM), 0.75), the fundamental where it was:
      // a round, pearly tone instead of a bell's clang.
      static const double kTopHarm = knob("PJ_PIANO_TOPHARM", 0.75);
      const double top2 = std::clamp((st.key + 21 - 84) / 12.0, 0.0, 1.0);
      const double Bf = s.B * (1 - kTopHarm * top2);
      const double f = n * f0 * std::sqrt((1 + Bf * n * n) * (1 + s.B) / (1 + Bf));
      if (f > 0.47 * fs_) continue;  // (left at rest: all its numbers 0)
      st.live = i + 1;
      if (d_.symTop <= 0 || f < d_.symTop) st.sym = i + 1;
      const double w = 2 * M_PI * f, wdt = w / fs_;
      double sigma = s.loss + b2 * f + b3 * f * f + (supportFc > 0 ? support / (1 + (supportFc / f) * (supportFc / f)) : support);
      // The top two and a half octaves (from F#5, fully from C6): the second partial dies away
      // faster (its own loss x PJ_PIANO_TOPH2, 1.6: with the board's share ~35 % faster overall),
      // so the tone settles onto the fundamental; the partials above it
      // optionally too (x PJ_PIANO_TOPHI).
      {
        static const double kTopH2 = knob("PJ_PIANO_TOPH2", 1.6), kTopHi = knob("PJ_PIANO_TOPHI", 1.0);
        const double top = std::clamp((st.key + 21 - 78) / 6.0, 0.0, 1.0);  // (key 0 is A0, MIDI 21)
        if (n == 2) sigma *= 1 + (kTopH2 - 1) * top;
        else if (n > 2) sigma *= 1 + (kTopHi - 1) * top;
        // The top octaves (from C6, fully from E7): the partials above the fundamental dying a
        // little faster in the wire itself (x PJ_PIANO_TOPUP, 1.5), the fundamental not (x
        // PJ_PIANO_TOPF0, 1.0): their short life is the bridge's (TOPBRIDGE below), as sound.
        const double kTopUp = d_.topUp, kTopF0 = d_.topF0;
        const double top8 = std::clamp((st.key + 21 - 84) / 16.0, 0.0, 1.0);
        sigma *= 1 + ((n >= 2 ? kTopUp : kTopF0) - 1) * top8;
      }
      if (d_.hiDecay > 0)
        sigma *= 1 + d_.hiDecay * std::clamp((st.key + 21 - 36) / 12.0, 0.0, 1.0) * std::clamp(std::log2(f / 600) / 1.5, 0.0, 1.0);
      const double g = std::exp(-sigma / fs_);
      cs_[m] = float(g * std::cos(wdt));
      sn_[m] = float(g * std::sin(wdt));
      cs1_[m] = float(g * std::cos(wdt * sideTune));
      sn1_[m] = float(g * std::sin(wdt * sideTune));
      wdt_[m] = float(wdt); cs0_[m] = cs_[m]; sn0_[m] = sn_[m]; cs10_[m] = cs1_[m]; sn10_[m] = sn1_[m];
      const double sign = n % 2 ? 1 : -1;  // (-1)^(n+1)
      in_[m] = float(-2 * sign / (n * M_PI) * w * d_.bridgeIn * fastPow(d_.bridgeBass, std::clamp((72 - (st.key + 21)) / 24.0, 0.0, 1.0)));
      nw_[m] = float(n / w);
      out_[m] = float(s.mu * s.len * sign / (n * M_PI));
      outFull_[m] = out_[m];
      if (d_.bloom > 0) {
        // (From C2 down in full, gone by E3; from ~350 Hz up. Each partial its own time, drawn from
        // its key and number so that a note always opens alike: most 15-60 ms, one in three late,
        // 80-220 ms, and those starting further under.)
        const double place = std::clamp((52 - (st.key + 21)) / 16.0, 0.0, 1.0);
        const double up = std::clamp(std::log2(f / 350) / 1.5, 0.0, 1.0);
        if (place * up > 0) {
          uint32_t h = uint32_t(st.key + 1) * 2654435761u ^ uint32_t(n) * 40503u;
          h = h * 1664525u + 1013904223u;
          const double r = (h >> 8 & 0xffff) / 65536.0;
          const bool late = (h >> 24) % 3 == 0;
          const double ms = late ? 80 * fastPow(220.0 / 80, r) : 15 * fastPow(4.0, r);
          bloomTau_[m] = float(ms * 1e-3 * fs_);
          const double db = -(late ? 16.0 : 7.0) * std::min(1.5, d_.bloom) * place * up;
          bloomFloor_[m] = float(fastPow(10.0, db / 20));
        }
      }
      // (The felt's footprint on the string, as the head's crown makes it: the bass and middle
      // hammers big, their crowns broad and round, pressing a wide patch most at its middle (a
      // raised cosine 24 mm across at A0, 18 at C4); up the keyboard smaller and more pointed,
      // the patch narrower (12 mm at C6, 7 at C8; PJ_PIANO_HAMW scales them, 0.010 = 1).
      // A round crown holds back the high partials smoothly, a point lets them through:
      // the sinc of a flat patch with its side lobes never was either.)
      const double kWscale = hamW / 0.010;  // (this design's: a static here kept the first-built instrument's for all)
      static const double kKw[] = {1, 40, 64, 88}, kW[] = {0.024, 0.018, 0.012, 0.007};
      double crown = kW[3];
      for (int j = 0; j < 3; ++j)
        if (st.key + 1 <= kKw[j + 1]) {
          crown = kW[j] + (kW[j + 1] - kW[j]) * (st.key + 1 - kKw[j]) / (kKw[j + 1] - kKw[j]);
          break;
        }
      const double a = n * M_PI * s.strike, hw = n * M_PI * crown * kWscale / (2 * s.len);
      const double q = hw / M_PI, flat = hw > 1e-6 ? std::sin(hw) / hw : 1.0;
      const double shape = std::sin(a) * (std::fabs(1 - q * q) > 1e-6 ? flat / (1 - q * q) : 0.5);
      hk_[m] = float(shape / modalMass / fs_);
      if (d_.restrike > 0) {
        // (A hammer stops less of a long string: the energy left = 0.4 ^ (0.1 m / length x how far the
        // partial moves at the hammer's place), so 0.4 at a full swing on a 10 cm string, 0.95 on a
        // 2 m one, and all of it at a node. PJ_PIANO_RESTRIKE scales the exponent, 0.6 = 1. The log is
        // for the amplitude; a partial that turns within the event is caught only on its half-turns,
        // so it is taken twice as fast then.)
        static const double kRef = knob("PJ_PIANO_RESTRIKE_LEN", 0.1);
        const double left = std::log(0.4) * d_.restrike / 0.6 * kRef / s.len * std::fabs(std::sin(a));
        restrikeLn_[m] = float(0.5 * left * (1 + std::min(1.0, f * 1.5e-3)));
      } else if (d_.repluck > 0 && s.pluckK > 0) {
        // (A plectrum on a ringing string: the energy left = (1 - repluck) ^ how far the partial
        // moves at the pluck point, whatever the string's length: the tip is held by the jack and
        // does not give as a felt does. The log is for the amplitude, and a partial that turns
        // within the touch is taken twice as fast on its half-turns, as under the hammer.)
        const double left = std::log(1 - std::min(d_.repluck, 0.99)) * std::fabs(std::sin(a));
        restrikeLn_[m] = float(0.5 * left * (1 + std::min(1.0, f * d_.repluckTime)));
      }
      vw_[m] = 1;
      hc_[m] = float(shape * cs_[m] / w);
      hs_[m] = float(shape * sn_[m] / w);
      comply += hk_[m] * hs_[m];
      // (The mean of sin^2 over the felt's length.)
      const double c0 = n * M_PI * s.damper, hl = n * M_PI * s.damperLen / (2 * s.len);
      const double mean = 0.5 * (1 - std::cos(2 * c0) * (hl > 1e-6 ? std::sin(2 * hl) / (2 * hl) : 1.0));
      damp_[m] = float(std::exp(-rate0 * mean / fs_));
      // (The felt's push, spread over its length: the mean of sin there.)
      dk_[m] = float(std::sin(c0) * (hl > 1e-6 ? std::sin(hl) / hl : 1.0) / modalMass / fs_);
      for (int q = 0; q < kDPts; ++q) {  // (its quarters: their middles at -3/8 .. +3/8 of the felt)
        const double cq = n * M_PI * (s.damper + s.damperLen / s.len * (q - 1.5) / kDPts), hq = hl / kDPts;
        const double sh = std::sin(cq) * (hq > 1e-6 ? std::sin(hq) / hq : 1.0);
        dsh_[q][m] = float(sh);
        dshw_[q][m] = float(sh / w);
        dkp_[q][m] = float(sh / modalMass / fs_);
      }
    }
    // (what a push at one quarter of the damper's felt does to the string's speed at another)
    for (int q = 0; q < kDPts; ++q)
      for (int r = 0; r < kDPts; ++r) {
        double g = 0;
        for (int i = 0; i < st.live; ++i) g += double(dsh_[q][st.off + i]) * dkp_[r][st.off + i];
        st.dG[q][r] = float(g);
      }
    // (the felt pressed in, its friction: as much as the loss per mode above at a small swing,
    // the felt always on the string: rate0 mean = R shape^2 / 2 m)
    st.dampR = float(2 * rate0 * modalMass);
    st.comply = float(comply);
    // Its lengthwise modes. The wire's core carries the stretch (a wound string's copper winding
    // only weighs): its diameter from the stiffness, B = pi^3 E d^4 / 64 T L^2, the string's
    // whole mass moving with it: the first at c / 2 L, c = sqrt(E A / mu) (A0: a 1.44 mm core,
    // 440 Hz, 16 x its fundamental; the lowest plain string, 1.9 m: 1.3 kHz, 15 x; Conklin's
    // and Bank's ~16-20 x). Driven by the strain's square, E A / 2 (dy/dx)^2: mode k by the
    // partials' pairs k apart (i, i + k; their sum, f_i + f_(i+k), is what it hears: at a
    // resonance, the "phantom" partials ring out) and those adding up to k; its push on the
    // bridge E A du/dx there (Bank & Sujbert, JASA 117, 2005). It grows with the square of the
    // blow and dies with the product of two partials' decays: the loud bass note's short
    // metallic ring over the first half second.
    // (The pitch's rise: the tension's mean gain E A pi^2 / 4 L^2 x the sum of (n U)^2 / 2 over the
    // modes and both swings, n U = x n / w; the frequency up by half of its share of the tension.)
    st.glideK = 0;
    if (d_.glide > 0 && s.B > 0) {
      static constexpr double kE = 2.02e11;
      const double d4 = 64 * s.B * s.tension * s.len * s.len / (M_PI * M_PI * M_PI * kE);
      const double EA = kE * M_PI * std::sqrt(d4) / 4;
      // (PJ_PIANO_GLIDEBASS: the wound bass's rise x this, up to E2, back to 1 by C4: the core's
      // stretch alone gives E1 under 3 cents at ff, a concert grand's is put at +6 to +12.)
      static const double kBass = knob("PJ_PIANO_GLIDEBASS", 3);
      const double bass = fastPow(kBass, std::clamp((60 - (st.key + 21)) / 20.0, 0.0, 1.0));
      st.glideK = float(d_.glide * bass * EA * M_PI * M_PI / (4 * s.len * s.len) / 2 / (2 * s.tension));
    }
    st.nl = 0;
    if (d_.longShare > 0 && s.B > 0) {
      static constexpr double kE = 2.02e11;
      const double d4 = 64 * s.B * s.tension * s.len * s.len / (M_PI * M_PI * M_PI * kE);
      const double EA = kE * M_PI * std::sqrt(d4) / 4, cL2 = EA / s.mu, fL = std::sqrt(cL2) / (2 * s.len);
      // (The push along the string, at the bridge's top some 3 cm over the board, is a moment on
      // it: it moves the board as a force of (height x the board's bending wavenumber) of it
      // would, ~0.7 at 1 kHz for the ~7 mm spruce under the bass bridge (k = (w^2 rho h /
      // D)^(1/4), 24 /m; 21 /m at 9 mm), as sqrt(f). An estimate, not a measurement: Conklin
      // found the bridge as mobile along the strings as across them.)
      auto share = [&](double f) { return d_.longShare * std::sqrt(f / 1000); };
      if (fL < d_.longTop) {
        // (The transverse modes under 5 kHz and the lengthwise ones under it: above, the phantoms
        // are far under the partials, and the cost is in these products.)
        st.nlx = 0;
        while (st.nlx < st.n && (st.nlx + 1) * f0 * std::sqrt(1 + s.B * (st.nlx + 1) * (st.nlx + 1)) < 5000) ++st.nlx;
        for (int k = 1; k <= Str::kLong && k * fL < std::min(5000.0, 0.45 * fs_); ++k) {
          const double w = 2 * M_PI * k * fL, g = std::exp(-w / (2 * d_.longQ) / fs_);
          const int i = st.nl++;
          st.lcs[i] = float(g * std::cos(w / fs_));
          st.lsn[i] = float(g * std::sin(w / fs_));
          st.ldrive[i] = float(-cL2 / 2 * k * std::pow(M_PI / s.len, 3) / fs_);
          st.lout[i] = float(share(k * fL) * EA * (k % 2 ? -1 : 1) * k * M_PI / s.len / w / fs_);
          st.lrest[i] = float(share(k * fL) * EA * (k % 2 ? -1 : 1) * (M_PI / s.len) * (M_PI / s.len) / 2 / fs_);
        }
        // (the even tension: mostly at twice the fundamental)
        st.ltension = float(share(2 * f0) * EA * M_PI * M_PI / (4 * s.len * s.len) / fs_);
      }
    }
  }
  // The soundboard, and what the damped strings standing on its bridges do
  // to it. A string under its damper is no resonator but a load: from the
  // bridge it looks like a line that swallows most of what is sent down it
  // (impedance -i Zc cot(k L), the damper's loss in k): a resistance near and
  // above its own pitch, a spring (its tension over its length) below. Each
  // resonance of the board is damped and raised by the 200 of them by what
  // it moves at their bridge points; without them the lowest resonances are
  // so sharp that a key whose fundamental meets one loses it at once (a wolf
  // no grand has). A lifted damper takes its key's load away: the strings
  // are then themselves, computed.
  if (kB) std::memcpy(phi_, d_.board.phi, sizeof(float) * kB * kKeys);
  // The treble's bridge: thin and on the board's thinner, freer part, it gives far more at high
  // frequencies than the board's broad resonances alone do there (the top strings' mobility,
  // Re Z0 Y, some 3e-3 .. 8e-3: Weinreich; Giordano). Most of a top string's life goes into it:
  // its unison's strings, drawn by one bridge, pull together within its bandwidth (a few cents
  // at C8: one tone, no beats), and what they lose is sound. Its admittance above ~1 kHz x
  // PJ_PIANO_TOPBRIDGE (4.8) at the top key, rising from C5 (the board's resonances there moving
  // sqrt of it more at those bridge points).
  {
    const double kTopBridge = d_.topBridge;
    for (int k = 0; k < kKeys && kTopBridge != 1; ++k) {
      const double kr = std::clamp((k + 21 - 72) / 36.0, 0.0, 1.0);
      if (kr <= 0) continue;
      for (int m = 0; m < kB; ++m) {
        const double fr = std::clamp((d_.board.freq[m] - 800) / 1200.0, 0.0, 1.0);
        phi_[size_t(k) * kB + m] *= float(std::sqrt(1 + (kTopBridge - 1) * kr * fr));
      }
    }
  }
  loadR_.assign(size_t(kKeys) * kB, 0.0f);
  loadX_.assign(size_t(kKeys) * kB, 0.0f);
  const double kSideLoad = d_.sideLoad;
  const double loadScale = d_.stringLoad;
  for (int k = 0; k < kKeys; ++k) {
    if (!key_[k].hasDamper) continue;
    const PianoKey& s = sc[k];
    const double zc = std::sqrt(s.tension * s.mu), f0 = s.f1;
    const double rate0 = s.damperRate, b2 = s.b2;
    for (int m = 0; m < kB; ++m) {
      const double f = d_.board.freq[m], n = f / f0;
      const double c0 = n * M_PI * s.damper, hl = n * M_PI * s.damperLen / (2 * s.len);
      const double sigma = s.loss + b2 * f + rate0 * 0.5 * (1 - std::cos(2 * c0) * (hl > 1e-6 ? std::sin(2 * hl) / (2 * hl) : 1.0));
      const std::complex<double> arg(M_PI * n, -sigma / (2 * f0));
      const std::complex<double> z = std::complex<double>(0, -1) * zc * std::cos(arg) / std::sin(arg);
      const double ph = phi_[k * kB + m], c = loadScale * s.strings * (1 + kSideLoad * kSideLoad) * ph * ph;
      loadR_[size_t(k) * kB + m] = float(c * z.real());
      loadX_[size_t(k) * kB + m] = float(c * z.imag());
    }
  }
  boardNow();
  // The trapwork's knock enters the board through the rim, along its edge: in this board's
  // resonances, at the two ends of the bridges, which lie nearest it (the bass bridge's end by
  // the bentside, the long bridge's treble end by the belly rail and the cheek, where the lift
  // rail's stop is).
  noise_ = float(std::max(0.0, d_.pedalNoise));
  trapPhi_.assign(kB, 0.0f);
  for (int m = 0; m < kB; ++m) trapPhi_[m] = 0.5f * (phi_[m] + phi_[size_t(kKeys - 1) * kB + m]);
  {
    // Each damper's regulation: how much of the lift it still has to fall when the others touch
    // (0 .. damperSpread m), fixed for the instrument; its own draws.
    uint32_t r = d_.seed * 2246822519u + 3;
    for (int k = 0; k < kKeys; ++k) {
      r = r * 1664525u + 1013904223u;
      key_[k].landDelay = float(d_.damperSpread * ((r >> 8) / 16777216.0));
    }
    noiseRng_ = d_.seed * 3266489917u + 11;
  }
  // (The board's data: [patch][resonance], this board's count of each.)
  for (int p = 0; p < kP; ++p)
    std::memcpy(pvol_ + size_t(p) * kB, d_.board.patchVol + size_t(p) * d_.board.n, sizeof(float) * kB);
  // The microphones. Each patch is a small source in a wall: its sound
  // arrives after its distance, weaker by it (rho / 2 pi r times the air's
  // acceleration), and by the microphone's pattern at its angle.
  for (int c = 0; c < 2; ++c) {
    const PianoDesign::Mic& mic = d_.mic[c];
    const double mx = mic.x, my = mic.y, mz = mic.z, ang = mic.angleDeg * M_PI / 180, pat = mic.pattern;
    const double ay = std::sin(ang), az = -std::cos(ang);  // the axis: down and to its side
    for (int p = 0; p < kP; ++p) {
      const double dx = d_.board.patchPos[2 * p] - mx, dy = d_.board.patchPos[2 * p + 1] - my;
      const double r = std::sqrt(dx * dx + dy * dy + mz * mz);
      const double d = std::min(r / 345.0 * fs_, double(kRing - 2));
      const double cosine = (dy * ay - mz * az) / r;
      tapI_[c][p] = int(d);
      tapF_[c][p] = float(d - int(d));
      tapG_[c][p] = float(1.18 / (2 * M_PI * r) * ((1 - pat) + pat * cosine));
    }
  }
  // The array microphone in place of the pair (PJ_PIANO_ARRAY=1): PJ_PIANO_ARRAY_N mono omnis (32) in a
  // line across the board, bass to treble, at the pair's distance from the keyboard's end and
  // centred over the board, each a random 2 .. 11 cm from the next (no two gaps alike: their
  // sum's combs fall apart). Each stands as high as makes its sum of 1 / d to the board's patches
  // the same as the middle one's, PJ_PIANO_ARRAY_LIFT (0.15 m) over the pair's height: over the
  // board's edges, where less of it lies around, they come down. Each goes to both channels, a
  // crossfade along the line: the bass end's 80 % left and 20 % right, the treble end's 20 % left
  // and 80 % right (PJ_PIANO_ARRAY_PAN, 0.6: the difference between the two at an end); a
  // channel is its mics' weighted mean, then PJ_PIANO_ARRAY_DB (-5: omnis hear the board's bass
  // that the crossed figure-of-eight pair turns away; at -5 as loud as the pair overall).
  if (knob("PJ_PIANO_ARRAY", 0) != 0 && kP > 0) {
    const int n = std::clamp(int(knob("PJ_PIANO_ARRAY_N", 32)), 2, 128);
    const double pan = std::clamp(knob("PJ_PIANO_ARRAY_PAN", 0.6), 0.0, 1.0);
    const double lift = knob("PJ_PIANO_ARRAY_LIFT", 0.15), base = d_.mic[0].z, mx = d_.mic[0].x;
    double ylo = 1e9, yhi = -1e9;
    for (int p = 0; p < kP; ++p) {
      ylo = std::min(ylo, double(d_.board.patchPos[2 * p + 1]));
      yhi = std::max(yhi, double(d_.board.patchPos[2 * p + 1]));
    }
    std::vector<double> ys(n, 0.0), zs(n);
    uint32_t r = d_.seed * 2654435761u + 77;
    for (int m = 1; m < n; ++m) {
      r = r * 1664525u + 1013904223u;
      ys[m] = ys[m - 1] + 0.02 + 0.09 * ((r >> 8) / 16777216.0);
    }
    const double shift = 0.5 * (ylo + yhi) - 0.5 * (ys[0] + ys[n - 1]);
    for (double& y : ys) y += shift;
    auto near = [&](double y, double z) {  // (its sum of 1 / d over the patches)
      double s = 0;
      for (int p = 0; p < kP; ++p) {
        const double dx = d_.board.patchPos[2 * p] - mx, dy = d_.board.patchPos[2 * p + 1] - y;
        s += 1 / std::sqrt(dx * dx + dy * dy + z * z);
      }
      return s;
    };
    const double yc = 0.5 * (ys[0] + ys[n - 1]), target = near(yc, base + lift);
    for (int m = 0; m < n; ++m) {
      double lo = 0.05, hi = 3.0;  // (the sum falls as it rises: halve the height's range)
      if (near(ys[m], lo) <= target) { zs[m] = lo; continue; }
      for (int it = 0; it < 50; ++it) {
        const double mid = 0.5 * (lo + hi);
        (near(ys[m], mid) > target ? lo : hi) = mid;
      }
      zs[m] = 0.5 * (lo + hi);
    }
    double sl = 0, sr = 0;
    std::vector<double> wl(n), wr(n);
    for (int m = 0; m < n; ++m) {
      const double u = std::clamp((ys[m] - yc) / std::max(0.5 * (ys[n - 1] - ys[0]), 1e-6), -1.0, 1.0);
      wl[m] = 0.5 * (1 - pan * u);
      wr[m] = 0.5 * (1 + pan * u);
      sl += wl[m];
      sr += wr[m];
    }
    const double trim = fastPow(10.0, knob("PJ_PIANO_ARRAY_DB", -5.0) / 20);
    arrN_ = n;
    arrI_.assign(size_t(kP) * n, 0);
    arrF_.assign(size_t(kP) * n, 0.0f);
    arrGL_.assign(size_t(kP) * n, 0.0f);
    arrGR_.assign(size_t(kP) * n, 0.0f);
    for (int p = 0; p < kP; ++p)
      for (int m = 0; m < n; ++m) {
        const double dx = d_.board.patchPos[2 * p] - mx, dy = d_.board.patchPos[2 * p + 1] - ys[m];
        const double rr = std::sqrt(dx * dx + dy * dy + zs[m] * zs[m]);
        const double d = std::min(rr / 345.0 * fs_, double(kRing - 2));
        const size_t at = size_t(p) * n + m;
        arrI_[at] = int(d);
        arrF_[at] = float(d - int(d));
        const double g = trim * 1.18 / (2 * M_PI * rr);
        arrGL_[at] = float(g * wl[m] / sl);
        arrGR_[at] = float(g * wr[m] / sr);
      }
    static bool told = false;
    if (!told) {
      told = true;
      double zlo = 1e9, zhi = 0;
      for (double z : zs) { zlo = std::min(zlo, z); zhi = std::max(zhi, z); }
      std::fprintf(stderr, "  piano array mic: %d omnis over %.2f m across (the board %.2f m), %.2f .. %.2f m high (middle %.2f)\n",
                   n, ys[n - 1] - ys[0], yhi - ylo, zlo, zhi, zs[n / 2]);
    }
  }
  // (the regions as sources: as a patch straight under the pair is heard by it, its delay and
  // gain, the same for every patch)
  {
    const double mz = d_.mic[0].z, ang = d_.mic[0].angleDeg * M_PI / 180, pat = d_.mic[0].pattern;
    regionSrc_ = knob("PJ_PIANO_PARTSRC", 0) != 0;
    srcD_ = std::min(int(std::lround(mz / 345.0 * fs_)), kRing - 2);
    srcG_ = float(1.18 / (2 * M_PI * mz) * ((1 - pat) + pat * std::cos(ang)));
  }
  reset();
}

// The board's resonances under the strings that now rest damped on it.
void Piano::boardNow() {
  const double etaScale = d_.boardLoss;
  const int kB = nB_, kKeys = nKeys_;
  std::vector<double> r(kB, 0.0), x(kB, 0.0);
  for (int k = 0; k < kKeys; ++k) {
    if (!key_[k].hasDamper || !key_[k].damperDown) continue;
    const float *lr = &loadR_[size_t(k) * kB], *lx = &loadX_[size_t(k) * kB];
    for (int m = 0; m < kB; ++m) { r[m] += lr[m]; x[m] += lx[m]; }
  }
  for (int m = 0; m < kB; ++m) {
    const double w0 = 2 * M_PI * d_.board.freq[m];
    // (velocity * (R + iX) opposes the motion: R damps, -w X adds to the stiffness.)
    const double w = std::sqrt(std::clamp(w0 * w0 - w0 * x[m], 0.5 * w0 * w0, 2.0 * w0 * w0)), wdt = w / fs_;
    const double rate = 0.5 * (etaScale * d_.board.loss[m] * w + std::max(r[m], 0.0));
    const double g = std::exp(-std::min(rate, 0.35 * w) / fs_);
    bcs_[m] = float(g * std::cos(wdt));
    bsn_[m] = float(g * std::sin(wdt));
    binv_[m] = float(1 / w);
  }
  boardStale_ = false;
}

void Piano::reset() {
  const int kB = nB_, kP = nP_;
  for (int d = 0; d < 2; ++d) {
    std::memset(x_[d], 0, sizeof(float) * total_);
    std::memset(v_[d], 0, sizeof(float) * total_);
  }
  std::memset(bx_, 0, sizeof(float) * kB);
  std::memset(bv_, 0, sizeof(float) * kB);
  std::memset(ba_, 0, sizeof(float) * kB);
  if (!outFull_.empty()) std::memcpy(out_, outFull_.data(), sizeof(float) * total_);
  if (!cs0_.empty()) {
    std::memcpy(cs_, cs0_.data(), sizeof(float) * total_); std::memcpy(sn_, sn0_.data(), sizeof(float) * total_);
    std::memcpy(cs1_, cs10_.data(), sizeof(float) * total_); std::memcpy(sn1_, sn10_.data(), sizeof(float) * total_);
    for (Str& s : str_) s.glide = s.glideSet = 0;
  }
  for (Key& k : key_) {
    k.bloomT = -1;
    k.restrikeT = -1;
    k.softStep = 0;
    std::fill(std::begin(k.rx), std::end(k.rx), 0.0f);
    std::fill(std::begin(k.rv), std::end(k.rv), 0.0f);
    k.hammerOn = false;
    k.acc = k.force = 0;
    k.landWait = k.landLeft = 0;
    k.hy = k.damperDown ? 0.0f : 1.0f;  // (resting on its strings, or lifted)
    k.hv = 0;
    k.seated = k.damperDown;
    k.lin = false;
    k.linQuiet = 0;
  }
  landing_ = knocks_ = 0;
  for (Knock& q : knock_) q.left = 0;
  for (Str& s : str_) {
    s.active = !key_[s.key].damperDown;
    s.use = (s.sym + 15) & ~15;
    s.hiPeak = 0;
    s.dPrev = 0;
    s.longLeft = 0;
    for (int i = 0; i < Str::kLong; ++i) s.lx[i] = s.lv[i] = 0;
  }
  std::memset(ring_, 0, sizeof(float) * kP * kRing);
  side_ = 0;
  asleep_ = true;
  census();
}

void Piano::census() {
  const int kKeys = nKeys_;
  nActive_ = 0;
  for (int k = 0; k < kKeys; ++k) {
    bool any = false;
    for (int j = 0; j < key_[k].ns; ++j) any = any || str_[key_[k].s0 + j].active;
    key_[k].active = any;
    if (any) activeKeys_[nActive_++] = k;
    else key_[k].acc = 0;
  }
}

int Piano::activeStrings() const {
  int n = 0;
  for (const Str& s : str_) n += s.active;
  return asleep_ ? 0 : n;
}

// Awake, every string is computed, the damped ones under their dampers too, until all of it has
// died away: a key answers the others alike whether it has been played before or not. (Before, a
// string was computed from its first note on and left out again only at rest: in running music
// never, the others stirring it; a key not yet played did not answer at all.) PJ_PIANO_ALLSTRINGS=0:
// as before.
void Piano::wake() {
  static const bool kAll = knob("PJ_PIANO_ALLSTRINGS", 1) != 0;
  allStrings_ = kAll;
  for (Str& s : str_)
    if (s.use == 0) s.use = (s.sym + 15) & ~15;
  if (asleep_ && allStrings_) {
    for (Str& s : str_) s.active = !key_[s.key].noSym;
    census();
  }
  asleep_ = false;
}

// The blow's vowel. The hammer's speed walks a path of six vowels, y u o a e i (0.45 to 5.35 m/s,
// even in the logarithm: about velocity 10, 30, 50, 70, 95, 127), their two formants between them
// gliding: what of the push reaches a partial is 0 dB at a formant (less its level), falling to
// -vowelDb an octave and a bit away (less deep toward i). The fundamental and what lies under the first formant keep
// all of it. Only ever takes away, so the contact stays as stable as it was.
static void blowVowel(double speed, double depth, double f1, double B, int n, int midi, float* w) {
  struct V { double F1, F2, g2; };
  static constexpr V kV[6] = {{250, 1900, -24}, {300, 700, -14}, {480, 850, -8},
                              {720, 1250, -3},  {480, 1900, -3}, {280, 2400, 0}};
  const double pos = std::clamp(5 * std::log(speed / 0.45) / std::log(5.35 / 0.45), 0.0, 5.0);
  const int a = std::min(int(pos), 4);
  const double t = pos - a;
  const double F1 = std::exp((1 - t) * std::log(kV[a].F1) + t * std::log(kV[a + 1].F1));
  const double F2 = std::exp((1 - t) * std::log(kV[a].F2) + t * std::log(kV[a + 1].F2));
  const double g2 = (1 - t) * kV[a].g2 + t * kV[a + 1].g2;
  // (from e to i the valleys fill: the hardest blow has everything, its stretch fed by all the partials)
  depth *= 1 - 0.6 * std::max(pos - 4, 0.0);
  // (By velocity 50 the low and middle keys are a full piano, their valleys nearly gone; from C4
  // to C6 they stay, the upper keys opening later.)
  const double reg = std::clamp((midi - 60) / 24.0, 0.0, 1.0);
  const double open = std::clamp(std::log(speed / 0.6) / std::log(1.3 / 0.6), 0.0, 1.0);
  depth *= 1 - 0.85 * open * (1 - reg);
  // (around C4 the valleys are shallow at any blow: emptied there it sounded of plastic, not of wire and wood)
  depth *= 1 - 0.6 * std::max(std::exp(-(midi - 60) * (midi - 60) / 49.0), std::exp(-(midi - 72) * (midi - 72) / 49.0));
  const double count = 3.0 * fastPow(std::max(speed, 0.395) / 0.395, 2.2);
  for (int i = 0; i < n; ++i) {
    const double m = i + 1, f = m * f1 * std::sqrt(1 + B * m * m);
    // (The gentlest blows count their overtones: one or two up to velocity 8, two to four at 16,
    // the rest opening quickly above: partials past `count` hardly get the push.)
    const double over = std::pow(m / count, 8), few = 1 / (1 + over);
    if (i == 0 || f <= F1) { w[i] = float(i == 0 ? 1 : few); continue; }
    const auto fall = [&](double c) { const double o = std::log2(f / c) / 0.6; return depth * std::min(1.0, o * o); };
    const double db = std::max({-depth, -fall(F1), g2 - fall(F2)});
    w[i] = float(few * fastPow(10.0, db / 20));
  }
}

void Piano::strike(int key, double speed) {
  if (key < 0 || key >= nKeys_) return;
  Key& k = key_[key];
  k.lin = false;
  k.linQuiet = 0;
  bool rang = false;
  for (int j = 0; j < k.ns; ++j) {
    const Str& st = str_[k.s0 + j];
    if (!st.active) continue;
    float en = 0;
    for (int i = 0; i < st.n; ++i) en += x_[0][st.off + i] * x_[0][st.off + i] + v_[0][st.off + i] * v_[0][st.off + i];
    rang = rang || en > 1e-9f;
  }
  // The hammer starts just under the lowest of its strings.
  float lowest = 0;
  for (int j = 0; j < k.ns; ++j) {
    Str& s = str_[k.s0 + j];
    float y = 0;
    for (int i = 0; i < s.n; ++i) y += hc_[s.off + i] * x_[0][s.off + i] + hs_[s.off + i] * v_[0][s.off + i];
    lowest = std::min(lowest, y);
    s.active = true;
    s.use = (s.live + 15) & ~15;  // (played: all its modes)
    s.hiPeak = 0;
    s.dPrev = 0;
    if (s.nl > 0) {
      s.longLeft = int(d_.longTime * fs_);
      s.longPeak = 0;
    }
  }
  k.eta = k.pluckK > 0 ? lowest : lowest - 1e-4f;  // (a plectrum starts touching its string)
  // (the voicing above and below goes by the blow as played; only the energy by the mass)
  k.vh = float(speed * (d_.massPower != 1 && nKeys_ > 48 ? fastPow(std::max(k.mh / key_[48].mh, 1.0f), (d_.massPower - 1) / 2) : 1.0));
  k.hammerOn = true;
  k.restrikeT = (d_.restrike > 0 || (d_.repluck > 0 && k.pluckK > 0)) && rang ? 0 : -1;
  if (d_.bloom > 0 && !rang) k.bloomT = 0;  // (a string still ringing is not made to duck)
  // The nap by the blow: a gentle hammer lands on all of its depth and stays round (x
  // PJ_PIANO_NAPPACKP, 3.0, at 0.5 m/s and under), by a moderate blow it is the design's own
  // (the low and middle keys from 0.9 m/s, the upper ones from 1.7), and only a forte, from
  // 2.6 m/s, packs it down and reaches the felt (x PJ_PIANO_NAPPACKF, 0.5, at 5.35 m/s and over).
  if (d_.napPack > 0 && speed > 0) {
    static const double kSoft = knob("PJ_PIANO_NAPPACKP", 3.0), kHard = knob("PJ_PIANO_NAPPACKF", 0.5);
    const double reg = std::clamp((key + 21 - 60) / 24.0, 0.0, 1.0);
    const double settled = std::exp((1 - reg) * std::log(0.9) + reg * std::log(1.7));
    const double a1 = std::clamp(std::log(speed / 0.5) / std::log(settled / 0.5), 0.0, 1.0);
    const double a2 = std::clamp(std::log(speed / 2.6) / std::log(5.35 / 2.6), 0.0, 1.0);
    // (Around C5 the felt is voiced firmer from a moderate blow on: at velocity 50 the note already
    // shows several overtones. PJ_PIANO_NAPMID: its nap's depth there, x.)
    static const double kMid = knob("PJ_PIANO_NAPMID", 0.3);
    const double place = std::exp(-(key + 21 - 72) * (key + 21 - 72) / 64.0);
    const double from = std::clamp(std::log(speed / 0.8) / std::log(1.3 / 0.8), 0.0, 1.0);
    const double mid = place * from * (1 - a2) * std::log(kMid);
    // From F3 up, from a low-medium blow on, the nap gives out a little sooner (its depth x
    // PJ_PIANO_BRILL, 0.75): a bit more of the overtones, more brilliance. In from D3 to G#3 and
    // from 0.9 to 1.8 m/s (velocity ~35 to 60); the softest blows and the bass as they were. It is
    // the nap, not the felt under it, that holds the overtones back there: the felt 4 x stiffer
    // gave ~1 dB, the nap a third as deep ~10 dB at velocity 60. (The user's ear, 2026-11-24.)
    static const double kBrill = knob("PJ_PIANO_BRILL", 0.75);
    static const double kBrillFrom = knob("PJ_PIANO_BRILLFROM", 50);  // (MIDI: in over six keys from here)
    const double upK = std::clamp((key + 21 - kBrillFrom) * (1 / 6.0), 0.0, 1.0);
    const double hardK = std::clamp(std::log(speed / 0.9) / std::log(1.8 / 0.9), 0.0, 1.0);
    const double brill = upK * hardK * std::log(kBrill);
    k.napPack = float(d_.napPack * std::exp((1 - a1) * std::log(kSoft) + a2 * std::log(kHard) + mid + brill));
  }
  if (d_.vowelDb > 0 && speed > 0)
    for (int j = 0; j < k.ns; ++j) blowVowel(speed, d_.vowelDb, k.f1, k.B, str_[k.s0 + j].n, key + 21, vw_ + str_[k.s0 + j].off);
  if (d_.keyKnock > 0 && speed > 0) knock(d_.keyKnock * speed, d_.keyKnockTime, key);
  wake();
  census();
}

void Piano::softLand(int key, double seconds) {
  if (key < 0 || key >= nKeys_ || seconds <= 0) return;
  Key& k = key_[key];
  if (!k.hasDamper || !k.damperDown) return;
  if (k.landWait > 0 || k.landLeft > 0) --landing_;
  k.landWait = k.landLeft = 0;
  k.engage = 0;
  k.hy = 1.0f;
  k.hv = 0;
  k.seated = false;
  k.softT = 0;
  k.softStep = float(1 / (seconds * fs_));
  wake();
}

void Piano::damper(int key, bool down, double speed, double loud) {
  if (key < 0 || key >= nKeys_) return;
  Key& k = key_[key];
  if (!k.hasDamper || k.damperDown == down) return;
  k.damperDown = down;
  k.lin = false;
  k.linQuiet = 0;
  boardStale_ = true;
  k.softStep = 0;
  if (!down) {
    k.engage = 0;
    k.hy = 1.0f;  // (lifted off: out of reach)
    k.hv = 0;
    k.seated = false;
    if (k.landWait > 0 || k.landLeft > 0) --landing_;  // (lifted again before it landed)
    k.landWait = k.landLeft = 0;
    for (int j = 0; j < k.ns; ++j) str_[k.s0 + j].active = true;
    census();
  } else if (dContact_) {
    // (the contact: the head let go from where it was lifted to, falling; the felt meets the
    // strings when it gets there, and takes them as the contact does)
    if (speed < 0) {
      noiseRng_ = noiseRng_ * 1664525u + 1013904223u;
      speed = d_.damperFall * (0.75 + 0.5 * ((noiseRng_ >> 8) / 16777216.0));
    }
    if (noise_ > 0) {
      k.hy = 0.3e-3f + k.landDelay + k.dW / (dFeltK_ * std::max(1, k.ns));  // (from its rest)
      // (loud: the pedal let go, PianoDesign::pedalUp; it was capped at 1 here, so that setting
      // reached only the rail's knock)
      k.hv = -float(speed * noise_ * std::max(0.3, loud));
      k.engage = 0;
    } else {  // (the landings unheard (PJ_PIANO_PEDALNOISE=0): set down on its strings at rest)
      k.hy = k.hv = 0;
      k.seated = true;
      k.engage = 1;
    }
    for (int j = 0; j < k.ns; ++j) str_[k.s0 + j].active = true;
    census();
    wake();
  } else if (noise_ > 0 && d_.key[key].damperMass > 0) {
    // The damper falls onto its strings (research/piano.md 6). Its momentum m v is taken up by
    // the felt (a cushion: the head does not bounce, it stays on the strings) in a half sine of
    // the contact time, 8 ms (the soft damper felt a spring of ~2.5 kN/m under a 16 g head:
    // pi sqrt(m / k)): a peak force of pi m v / 2 T, shared by the key's strings; under ~150 Hz
    // mostly, the strings' low partials and the board's lowest resonances: a thump, no click. A key let go
    // lets its damper fall at about 0.12 m/s (the key rising under the hammer's and the
    // damper's weight: 4-6 mm of fall in some 40 ms), each time a little its own (x 0.75 .. 1.25).
    // It touches after its regulation: the share of the lift it still has to fall when the first
    // ones touch, over its speed (0 .. ~4 ms at 0.12 m/s: the landings of a pedal's release
    // smeared into a "chuff", not one click).
    if (speed < 0) {
      noiseRng_ = noiseRng_ * 1664525u + 1013904223u;
      speed = d_.damperFall * (0.75 + 0.5 * ((noiseRng_ >> 8) / 16777216.0));
    }
    const double T = d_.damperContact;
    k.landN = std::max(2, int(T * fs_ + 0.5));
    k.landLeft = k.landN;
    k.landWait = int(k.landDelay / std::max(speed, 0.02) * fs_);
    k.landAmp = float(loud * noise_ * d_.damperSide * M_PI * d_.key[key].damperMass * speed / (2 * T) / k.ns);
    ++landing_;
    for (int j = 0; j < k.ns; ++j) str_[k.s0 + j].active = true;
    census();
    wake();
  }
}

// The lift rail and the pedal's lever: released, they fall with the dampers and land on their
// felt stop (the rail's ~0.6 kg with the rod and the lever's share: a half sine of 8 ms, felt on
// wood, its force's energy under ~200 Hz), the case takes the knock and half of it reaches the
// board through the rim (rimShare, an estimate: the thud then sits ~15 dB under the dampers'
// own landing, the strings' "chuff" first, a dull thud under it). Pressed, the
// rail meets the underlevers and lifts them off the strings: a softer knock (railPress of it);
// the dampers leaving the strings give up only their weight's rest (some 0.15 N each, its step
// under 1/10 of a landing's push at 100 Hz), left out.
void Piano::pedal(bool down, double speed) {
  if (noise_ <= 0 || speed <= 0) return;
  knock(noise_ * (down ? d_.railPress : d_.pedalUp) * d_.railMass * speed, d_.railContact);
}

// A knock in the case (impulse N s, a half sine of T s), the rim's share of it into the board.
void Piano::knock(double impulse, double T, int at) {
  if (!nB_ || impulse <= 0) return;
  Knock* q = &knock_[0];
  for (Knock& k : knock_)
    if (k.left < q->left) q = &k;  // (a free one, or the nearest done)
  if (q->left == 0) ++knocks_;
  q->n = std::max(2, int(T * fs_ + 0.5));
  q->left = q->n;
  q->amp = float((at >= 0 ? 1.0 : d_.rimShare) * M_PI * impulse / (2 * T));
  q->at = at;
  wake();
}

// The string's stretch this sample: the strain's square in its lengthwise modes' terms (both
// directions of swing stretch it), their step, and their push on the bridge (x dt). Slowly
// (far under the lengthwise modes) the tension is even along the string, E A / 2L times the
// integral of the slope's square: the partials' squares only, at twice their frequencies (and
// the pitch's rise at a loud blow, not followed here); what the modes do beyond that, near
// their own frequencies, is their motion less what they would hold at rest: the phantoms.
float Piano::stretch(Str& s) {
  const int n = s.nlx;
  float b[2][256];
  const float* nw = nw_ + s.off;
  for (int d = 0; d < 2; ++d) {
    const float* x = x_[d] + s.off;
    for (int i = 0; i < n; ++i) b[d][i] = x[i] * nw[i];
  }
  // (Over its last 0.3 s the drive fades, the modes ringing out: no step when it stops.)
  const float fade = std::min(1.0f, s.longLeft * dt_ / 0.3f);
  float push = 0;
  for (int d = 0; d < 2; ++d)
    for (int i = 0; i < n; ++i) push += s.ltension * b[d][i] * b[d][i];
  for (int j = 0; j < s.nl; ++j) {
    const int k = j + 1;
    float S = 0;
    for (int d = 0; d < 2; ++d) {
      const float* q = b[d];
      for (int i = 0; i + k < n; ++i) S += q[i] * q[i + k];      // (pairs k apart: mode i + 1 with i + 1 + k)
      float h = 0;
      for (int i = 0; i < k - 1; ++i) h += q[i] * q[k - 2 - i];  // (pairs adding up to k)
      S += 0.5f * h;
    }
    const float vv = s.lv[j] + fade * s.ldrive[j] * S;
    const float xn = s.lx[j] * s.lcs[j] + vv * s.lsn[j];
    s.lv[j] = vv * s.lcs[j] - s.lx[j] * s.lsn[j];
    s.lx[j] = xn;
    push += s.lout[j] * xn + s.lrest[j] * S;
  }
  if (--s.longLeft == 0)
    for (int j = 0; j < s.nl; ++j) s.lx[j] = s.lv[j] = 0;
  return push;
}

// The damper as a contact (PJ_PIANO_DCONTACT): its head, a mass on the felt, lies on its strings
// by its weight (and the underlever's). The felt only pushes: while a string, swinging up, presses
// into it, a spring (dFeltK_, N/m) and its friction (the string's dampR, N s/m, as the
// per-mode loss was) push it back down and the head up; the head flies off a swing bigger than
// its weight can hold, and falls back. So a loud bass note is caught only by and by, the felt
// patting it (a buzz), the strings tuned a little higher while it presses (its stiffness on
// them), and its push, at one place, hands energy from the low partials to the higher ones.
// The sideways swing: the felt's friction while it touches (the swing's own loss as before,
// by how much of the time it touches).
void Piano::damperContact(Key& k) {
  // (Counted from rest: the head's place hy from where it sits on resting strings, each pressed
  // F0 / kDPts at each quarter of the felt (d0 deep); the strings' motion from their rest under
  // that weight once it has sat on them, only what differs from it moving them. At rest every
  // term is exactly 0.)
  const float F0 = k.dW / std::max(1, k.ns), d0 = F0 / dFeltK_;
  bool any = false;
  for (int j = 0; j < k.ns; ++j) any = any || str_[k.s0 + j].active;
  if (!any) {  // (all at rest: the head too, on them)
    k.hy = k.hv = 0;
    k.engage = 1;
    k.seated = true;
    return;
  }
  float head = 0;  // (the felt's push on the head, less its weight)
  bool touch = false;
  const float Fq0 = F0 / kDPts, Kq = dFeltK_ / kDPts;
  const float invFq0 = 1 / Fq0, invD0 = 1 / d0;
  bool allOn = true;
  float peak = 0;  // (the largest swing at the felt, as a share of what would lift it off)
  for (int j = 0; j < k.ns; ++j) {
    const Str& s = str_[k.s0 + j];
    if (!s.active) continue;
    const float* x = x_[0] + s.off;
    float* v = v_[0] + s.off;
    const int n = s.use;
    const float Rq = s.dampR * (1.0f / kDPts);
    // (Each quarter of the felt: its own contact, lifting by itself. The quarters are taken one
    // after another, each seeing what the ones before it did to the string: their sums over the
    // modes all in one pass, the earlier quarters' part put in from the string's table dG.)
    float uq[kDPts], udq[kDPts], Fq[kDPts];
    feltSums(s, uq, udq);
    bool push = false;
    for (int q = 0; q < kDPts; ++q) {
      const float u = uq[q], ud = udq[q];
      // (its push less the resting one; touching while pressed in and pushing)
      const float dev = Kq * (u - k.hy) + Rq * (ud - k.hv);
      const bool on = u - k.hy + d0 > 0 && dev > -Fq0;
      touch = touch || on;
      allOn = allOn && on;
      peak = std::max({peak, std::fabs(dev) * invFq0, std::fabs(u - k.hy) * invD0});
      const float Fs = k.seated ? (on ? dev : -Fq0) : (on ? Fq0 + dev : 0.0f);
      Fq[q] = Fs;
      if (Fs != 0) {
        push = true;
        for (int r = q + 1; r < kDPts; ++r) udq[r] -= s.dG[r][q] * Fs;
      }
      head += on ? dev : -Fq0;
    }
    if (push) {
      const float *k0 = dkp_[0] + s.off, *k1 = dkp_[1] + s.off, *k2 = dkp_[2] + s.off, *k3 = dkp_[3] + s.off;
      const float f0 = Fq[0], f1 = Fq[1], f2 = Fq[2], f3 = Fq[3];
      for (int i = 0; i < n; ++i) v[i] -= k0[i] * f0 + k1[i] * f1 + k2[i] * f2 + k3[i] * f3;
    }
  }
  if (touch) k.seated = true;
  // (dHold_: a drag on the head, its wire in the guide rail's cloth bushings; 0 by default)
  k.hv += dt_ * (head - dHold_ * k.hv) / k.dM;
  k.hy += dt_ * k.hv;
  // (how much of the time the felt touches, over ~4 ms)
  k.engage += (float(touch) - k.engage) * std::min(1.0f, dt_ / 0.004f);
  // (All the way there: in single precision the approach stops ~6e-6 short of 1, and a string is
  // only left out of the computing once its damper's grip is whole: without this no string that
  // had sounded ever came to rest again.)
  if (touch && k.engage > 0.9999f) k.engage = 1;
  // Seated, every quarter touching and the strings barely moving for a while: the felt is then a
  // linear thing on them, taken as each mode's own loss (linear(): PJ_PIANO_DLIN).
  if (dLin_ > 0) {
    if (k.seated && allOn && k.engage >= 1 && peak < 0.5f * dLin_) {
      if (++k.linQuiet > dLinN_) {
        k.lin = true;
        k.hy = k.hv = 0;
      }
    } else {
      k.linQuiet = 0;
    }
  }
}

// A string's place and speed at the four quarters of its damper's felt: eight sums over its modes
// in one pass (the modes past its top are padding at rest: whole 16s).
void Piano::feltSums(const Str& s, float* uq, float* udq) const {
  const float* x = x_[0] + s.off;
  const float* v = v_[0] + s.off;
  const float *sw0 = dshw_[0] + s.off, *sw1 = dshw_[1] + s.off, *sw2 = dshw_[2] + s.off, *sw3 = dshw_[3] + s.off;
  const float *sh0 = dsh_[0] + s.off, *sh1 = dsh_[1] + s.off, *sh2 = dsh_[2] + s.off, *sh3 = dsh_[3] + s.off;
  const int n = s.use;
  float u0 = 0, u1 = 0, u2 = 0, u3 = 0, d0 = 0, d1 = 0, d2 = 0, d3 = 0;
  for (int i = 0; i < n; ++i) {
    const float xi = x[i], vi = v[i];
    u0 += sw0[i] * xi; u1 += sw1[i] * xi; u2 += sw2[i] * xi; u3 += sw3[i] * xi;
    d0 += sh0[i] * vi; d1 += sh1[i] * vi; d2 += sh2[i] * vi; d3 += sh3[i] * vi;
  }
  uq[0] = u0, uq[1] = u1, uq[2] = u2, uq[3] = u3;
  udq[0] = d0, udq[1] = d1, udq[2] = d2, udq[3] = d3;
}

// A damper seated on barely moving strings (Key::lin): the felt neither lifts nor slips, a linear
// thing: each mode loses by its motion under the felt (damp_, in swing), the contact not computed.
// Looked at every 32 samples: once the swing at the felt passes PJ_PIANO_DLIN (0.2) of what would
// lift it off, the contact is computed again (it leaves at half of that, held for 20 ms). 0: never.
void Piano::linearWatch(Key& k) {
  const float F0 = k.dW / std::max(1, k.ns), invD0 = dFeltK_ / F0, invFq0 = kDPts / F0;
  const float Kq = dFeltK_ / kDPts;
  float peak = 0;
  for (int j = 0; j < k.ns; ++j) {
    const Str& s = str_[k.s0 + j];
    if (!s.active) continue;
    float uq[kDPts], udq[kDPts];
    feltSums(s, uq, udq);
    const float Rq = s.dampR * (1.0f / kDPts);
    for (int q = 0; q < kDPts; ++q)
      peak = std::max({peak, std::fabs(Kq * uq[q] + Rq * udq[q]) * invFq0, std::fabs(uq[q]) * invD0});
  }
  if (peak > dLin_) {
    k.lin = false;
    k.linQuiet = 0;
  }
}

// A damper's felt pushing its strings this sample, while it lands; its grip comes on with it.
void Piano::land(Key& k) {
  if (k.landWait > 0) { --k.landWait; return; }
  if (k.landLeft <= 0) return;
  const float f = k.landAmp * std::sin(float(M_PI) * (k.landN - k.landLeft + 0.5f) / k.landN);
  for (int j = 0; j < k.ns; ++j) {
    const Str& s = str_[k.s0 + j];
    float* v0 = v_[0] + s.off;
    const float* dk = dk_ + s.off;
    for (int i = 0; i < s.n; ++i) v0[i] += dk[i] * f;
  }
  if (--k.landLeft == 0) --landing_;
}

// The hammer against its strings, one sample: the felt's compression on each
// string at the end of the sample depends on the force during it (the string
// gives way, the hammer is slowed): solved for, not stepped (a hard blow on a
// stiff felt would otherwise overshoot).
// The plectrum: a spring between the rising jack and the string. It bends
// (the string hardly moving: this takes some milliseconds and is not heard)
// until its force passes the limit; then the string is off its tip.
void Piano::pluck(Key& k) {
  const float reach = k.eta + dt_ * k.vh;  // the jack a sample on
  float total = 0;
  bool gone = false;
  for (int j = 0; j < k.ns; ++j) {
    const Str& s = str_[k.s0 + j];
    float* x = x_[0] + s.off;
    float* v0 = v_[0] + s.off;
    float* v1 = v_[1] + s.off;
    const float *hc = hc_ + s.off, *hs = hs_ + s.off, *hk = hk_ + s.off;
    float y = 0;
    for (int i = 0; i < s.n; ++i) y += hc[i] * x[i] + hs[i] * v0[i];
    const float d = reach - y;
    const float f = d > 0 ? k.pluckK * d * (d < k.pluckSoft ? d / k.pluckSoft : 1.0f) / (1 + k.pluckK * s.comply) : 0;
    if (f >= k.pluckLimit * voicing_) {
      gone = true;
      if (k.pluckSnap > 0) {  // (the quill's snap: the high modes kicked at once)
        const float* w = wdt_.data() + s.off;
        const float lo = float(2 * M_PI * k.pluckSnapLo / fs_), hi = float(2 * M_PI * k.pluckSnapHi / fs_);
        const float kick = k.pluckSnap * f;
        for (int i = 0; i < s.n; ++i) {
          float g = std::clamp((w[i] - lo) / (hi - lo), 0.0f, 1.0f);
          if (k.pluckSnapTilt > 0) g *= 1 + k.pluckSnapTilt * std::clamp((w[i] - 2 * hi) / (3 * hi), 0.0f, 1.0f);
          if (g > 0) v0[i] += hk[i] * kick * g;
        }
      }
      continue;
    }
    if (f <= 0) continue;
    const float f1 = f * k.pluckSide;
    for (int i = 0; i < s.n; ++i) {
      v0[i] += hk[i] * f;
      v1[i] += hk[i] * f1;
    }
    total += f;
  }
  k.eta = reach;
  k.force = total;
  if (gone) {
    k.hammerOn = false;
    k.force = 0;
  }
}

// The bass's partials coming out: every 32 samples each one's output nearer its full value. The
// string keeps what the blow gave it (only less of it leaves at first), as a bell's late partials.
void Piano::bloomStep(Key& k) {
  if (k.bloomT++ % 32) return;
  const bool done = k.bloomT > int(1.2 * fs_);
  for (int j = 0; j < k.ns; ++j) {
    const Str& st = str_[k.s0 + j];
    for (int i = 0; i < st.n; ++i) {
      const size_t m = st.off + i;
      if (bloomFloor_[m] >= 1) continue;
      const float g = done ? 1.0f : 1 - (1 - bloomFloor_[m]) * std::exp(-k.bloomT / bloomTau_[m]);
      out_[m] = outFull_[m] * g;
    }
  }
  if (done) k.bloomT = -1;
}

// The pitch's rise, looked at every 32 samples: each string's partials turned a little faster by
// what its swing now adds to its tension, gliding back as it dies.
void Piano::glideStep(Key& k) {
  if (++k.glideN < 32) return;
  k.glideN = 0;
  for (int j = 0; j < k.ns; ++j) {
    Str& s = str_[k.s0 + j];
    if (s.glideK <= 0) continue;
    const size_t o = s.off;
    float en = 0;
    for (int i = 0; i < s.live; ++i) {
      const float q = nw_[o + i] * nw_[o + i];
      en += q * (x_[0][o + i] * x_[0][o + i] + v_[0][o + i] * v_[0][o + i] + x_[1][o + i] * x_[1][o + i] + v_[1][o + i] * v_[1][o + i]);
    }
    s.glide += 0.5f * (std::min(0.03f, s.glideK * en) - s.glide);
    if (std::fabs(s.glide - s.glideSet) < 3e-6f) continue;
    s.glideSet = s.glide;
    for (int i = 0; i < s.live; ++i) {
      const size_t m = o + i;
      const float th = wdt_[m] * s.glide, c = 1 - 0.5f * th * th;
      cs_[m] = cs0_[m] * c - sn0_[m] * th;
      sn_[m] = sn0_[m] * c + cs0_[m] * th;
      cs1_[m] = cs10_[m] * c - sn10_[m] * th;
      sn1_[m] = sn10_[m] * c + cs10_[m] * th;
    }
  }
}

// A hammer landing on a string still ringing stops part of what swings into it: over 1.5 ms (a
// smooth window), each partial's up-and-down swing, by the string's length (restrikeLn_), only
// while it is on the hammer's side of the string. The sideways swing goes on. (PJ_PIANO_RESTRIKE, 0.6; PJ_PIANO_RESTRIKE_DIR=-1: the
// other half of the swing, for comparing.)
// A plectrum touching a ringing string does the same from under it, over the design's repluckTime
// (PianoDesign::repluck): the swing toward the tip at the pluck point is what it stops.
void Piano::restrikeStep(Key& k) {
  const int n = std::max(1, int((k.pluckK > 0 ? d_.repluckTime : 1.5e-3) * fs_));
  if (k.restrikeT >= n) { k.restrikeT = -1; return; }
  const float sn = std::sin(float(M_PI) * (k.restrikeT + 0.5f) / n), w = 2 * sn * sn / n;
  static const float kDir = knob("PJ_PIANO_RESTRIKE_DIR", 1) < 0 ? -1.0f : 1.0f;
  for (int j = 0; j < k.ns; ++j) {
    const Str& s = str_[k.s0 + j];
    float* x = x_[0] + s.off;
    float* v = v_[0] + s.off;
    const float* hs = hk_ + s.off;
    const float* ln = restrikeLn_.data() + s.off;
    for (int i = 0; i < s.live; ++i)
      if (-kDir * x[i] * hs[i] > 0) {
        const float g = std::max(0.0f, 1 + ln[i] * w);
        x[i] *= g;
        v[i] *= g;
      }
  }
  ++k.restrikeT;
}

// The strings behind the bridge, one sample: moved mostly by their own key's strings across the
// bridge (own: their push on it this sample), a little by the bridge's vibration, ringing a
// little, and pushing back on it (no hammer or damper reaches them).
float Piano::rearStep(Key& k, float own) {
  float sum = 0;
  const float dy = k.acc;
  for (int i = 0; i < k.rearN; ++i) {
    const float xa = k.rx[i] + k.rin[i] * dy;
    const float v0 = k.rv[i] + k.rdrive[i] * own;
    const float va = v0 * k.rc[i] - xa * k.rs[i];
    k.rx[i] = xa * k.rc[i] + v0 * k.rs[i];
    sum += k.rout[i] * (va - v0);
    k.rv[i] = va;
  }
  return -sum;
}

void Piano::hammer(Key& k) {
  if (k.pluckK > 0) { pluck(k); return; }
  static const float kHyst = float(knob("PJ_PIANO_HYST", 0.25));  // s/m: more force pressing than releasing
  const float K0 = k.K * voicing_, p = k.p;
  const float hm = dt_ * dt_ / k.mh;
  float yFree[kMaxStr], F[kMaxStr] = {0, 0, 0};
  for (int j = 0; j < k.ns; ++j) {
    const Str& s = str_[k.s0 + j];
    const float* x = x_[0] + s.off;
    const float* v = v_[0] + s.off;
    const float* hc = hc_ + s.off;
    const float* hs = hs_ + s.off;
    float y = 0;
    for (int i = 0; i < s.n; ++i) y += hc[i] * x[i] + hs[i] * v[i];
    yFree[j] = y;
  }
  bool touching[kMaxStr];
  for (int j = 0; j < k.ns; ++j) touching[j] = str_[k.s0 + j].dPrev > 0;
  const float reach = k.eta + dt_ * k.vh;  // the hammer's place a sample on, unopposed
  float total = 0;
  for (int sweep = 0; sweep < 3; ++sweep) {
    for (int j = 0; j < k.ns; ++j) {
      Str& s = str_[k.s0 + j];
      const float others = total - F[j];
      const float A = s.comply + hm, b = reach - s.gap - yFree[j] - hm * others;
      const float K = K0 * s.hard;
      // d + A f(d) = b, f(d) = K d^p (1 + hyst (d - dPrev) / dt) for d > 0.
      float d = std::max(b, 0.0f), f = 0;
      if (b > 0 && k.nap > 0) {
        // The nap on the felt, two springs in a row: at the force g the nap
        // is pressed nap g / (g + c) (c = napK nap) and the felt under it
        // (g / K)^(1/p). g + A g h(d(g)) ... = b, solved for g (Newton, from
        // below: the left side rises with g and is concave).
        // (With napPack the nap gives out: as soft at first, nap g / c, but flat at napPack of
        // its depth, n (1 - exp(-g / (n c / nap))): a hard blow goes through it to the felt.)
        const float c = k.napK * k.nap, ip = 1 / p, iK = 1 / K;
        const float nD = k.nap * k.napPack, nF = c * k.napPack;
        const auto napD = [&](float x) { return k.napPack > 0 ? nD * (1 - std::exp(-x / nF)) : k.nap * x / (x + c); };
        const auto napS = [&](float x) { return k.napPack > 0 ? k.nap / c * std::exp(-x / nF) : k.nap * c / ((x + c) * (x + c)); };
        float g = 0;
        d = 0;
        for (int it = 0; it < 12; ++it) {
          const float dn = napD(g), dm = fastPow(g * iK, ip);
          d = dn + dm;
          const float dd = napS(g) + (g > 1e-12f ? ip * dm / g : 1e6f);
          const float h = std::max(0.0f, 1 + kHyst * (d - s.dPrev) / dt_);
          const float r = d + A * g * h - b;
          const float dr = dd + A * (h + g * kHyst / dt_ * dd);
          g = std::max(0.0f, g - r / dr);
          if (it == 0 && g <= 0) g = 1e-6f;
        }
        d = std::min(b, napD(g) + fastPow(g * iK, ip));
        f = g * std::max(0.0f, 1 + kHyst * (d - s.dPrev) / dt_);
      } else if (b > 0) {
        d = std::min(b, std::max(s.dPrev, 1e-7f));
        for (int it = 0; it < 6; ++it) {
          const float pw = K * fastPow(d, p - 1);
          const float h = std::max(0.0f, 1 + kHyst * (d - s.dPrev) / dt_);
          f = pw * d * h;
          const float df = pw * p * h + pw * d * kHyst / dt_;
          const float step = (d + A * f - b) / (1 + A * df);
          d = std::clamp(d - step, 0.0f, b);
        }
        f = K * fastPow(d, p) * std::max(0.0f, 1 + kHyst * (d - s.dPrev) / dt_);
      }
      total += f - F[j];
      F[j] = f;
      if (sweep == 2) s.dPrev = b > 0 ? d : 0;
    }
  }
  // (a tack's metal head meets its string first: a sharp knock, a half sine of tackTime)
  for (int j = 0; j < k.ns; ++j) {
    Str& s = str_[k.s0 + j];
    if (s.tack <= 0) continue;
    if (!touching[j] && s.dPrev > 0 && k.vh > 0) {
      s.tickLeft = std::max(2, int(d_.tackTime / dt_ + 0.5));
      s.tickAmp = std::min(s.tack * k.vh, 4 * s.tack);  // (capped: the head gives at a hard blow)
      s.tickN = s.tickLeft;
    }
    if (s.tickLeft > 0) {
      F[j] += s.tickAmp * std::sin(float(M_PI) * (s.tickN - s.tickLeft + 0.5f) / s.tickN);
      --s.tickLeft;
    }
  }
  for (int j = 0; j < k.ns; ++j) {
    if (F[j] <= 0) continue;
    const Str& s = str_[k.s0 + j];
    float* v0 = v_[0] + s.off;
    float* v1 = v_[1] + s.off;
    const float* hk = hk_ + s.off;
    const float* vw = vw_ + s.off;
    const float f0 = F[j], f1 = F[j] * s.tanH;
    for (int i = 0; i < s.n; ++i) {
      v0[i] += hk[i] * vw[i] * f0;
      v1[i] += hk[i] * vw[i] * f1;
    }
  }
  k.force = total;
  // (The hammer falls back: its weight and the action's spring.)
  k.vh -= dt_ * (total / k.mh + 30.0f);
  k.eta += dt_ * k.vh;
  // Falling away from the strings: caught by the action.
  if (total <= 0 && k.vh < 0) {
    float lowest = 1e9f;
    for (int j = 0; j < k.ns; ++j) lowest = std::min(lowest, yFree[j]);
    if (k.eta < lowest - 5e-4f) {
      k.hammerOn = false;
      k.force = 0;
    }
  }
}

// One sample of a string's modes, both directions of swing: the bridge's motion dy0 / dy1 into
// them, the damper's grip e (0: free), and what they passed back to the bridge (sum0 / sum1).
static inline void swing(int n, float e, float e1, float dy0, float dy1, const float* __restrict in,
                         const float* __restrict out, const float* __restrict dm, float* __restrict x0,
                         float* __restrict v0, float* __restrict x1, float* __restrict v1,
                         const float* __restrict c0, const float* __restrict s0, const float* __restrict c1,
                         const float* __restrict s1, float* sum0p, float* sum1p) {
  // (n: a whole number of 16s: the modes past a string's top are padding at rest, all their
  // numbers 0, so every pass is a whole vector and nothing is left over at the end)
  float sum0 = 0, sum1 = 0;
  if (e > 0 || e1 > 0) {
    // (e: the damper's grip on the up-and-down swing, e1 on the sideways one)
    for (int i = 0; i < n; ++i) {
      const float g = 1 - e * (1 - dm[i]), g1 = 1 - e1 * (1 - dm[i]);
      const float xa = x0[i] + in[i] * dy0, xb = x1[i] + in[i] * dy1;
      const float va = g * (v0[i] * c0[i] - xa * s0[i]), vb = g1 * (v1[i] * c1[i] - xb * s1[i]);
      x0[i] = g * (xa * c0[i] + v0[i] * s0[i]);
      x1[i] = g1 * (xb * c1[i] + v1[i] * s1[i]);
      sum0 += out[i] * (va - v0[i]);
      sum1 += out[i] * (vb - v1[i]);
      v0[i] = va;
      v1[i] = vb;
    }
  } else {
    for (int i = 0; i < n; ++i) {
      const float xa = x0[i] + in[i] * dy0, xb = x1[i] + in[i] * dy1;
      const float va = v0[i] * c0[i] - xa * s0[i], vb = v1[i] * c1[i] - xb * s1[i];
      x0[i] = xa * c0[i] + v0[i] * s0[i];
      x1[i] = xb * c1[i] + v1[i] * s1[i];
      sum0 += out[i] * (va - v0[i]);
      sum1 += out[i] * (vb - v1[i]);
      v0[i] = va;
      v1[i] = vb;
    }
  }
  *sum0p = sum0;
  *sum1p = sum1;
}

// Four sums over the board's modes at once (independent chains: no wait on each other).
static inline void dot4(const float* const* r, const float* __restrict x, int n, float* out) {
  const float *__restrict a = r[0], *__restrict b = r[1], *__restrict c = r[2], *__restrict d = r[3];
  float sa = 0, sb = 0, sc = 0, sd = 0;
  for (int m = 0; m < n; ++m) {
    sa += a[m] * x[m];
    sb += b[m] * x[m];
    sc += c[m] * x[m];
    sd += d[m] * x[m];
  }
  out[0] = sa, out[1] = sb, out[2] = sc, out[3] = sd;
}

// y += the four rows times their weights.
static inline void axpy4(float* __restrict y, const float* const* r, const float* f, int n) {
  const float *__restrict a = r[0], *__restrict b = r[1], *__restrict c = r[2], *__restrict d = r[3];
  const float fa = f[0], fb = f[1], fc = f[2], fd = f[3];
  for (int m = 0; m < n; ++m) y[m] += a[m] * fa + b[m] * fb + c[m] * fc + d[m] * fd;
}

float Piano::sample(float testForce) {
  if (boardStale_) boardNow();
  if (asleep_) {
    if (testForce == 0) {
      if (keepRegions_) std::fill(region_, region_ + kRegions, 0.0f);
      return 0;
    }
    wake();
  }
  const int kB = nB_, kP = nP_;
  const float kSide = kSide_;
  for (int a = 0; a < nActive_; ++a) {
    Key& k = key_[activeKeys_[a]];
    if (k.hammerOn) hammer(k);
    if (k.restrikeT >= 0) restrikeStep(k);
    if (k.bloomT >= 0) bloomStep(k);
    if (d_.glide > 0) glideStep(k);
  }
  std::memset(bf_, 0, sizeof(float) * kB);
  float pick = 0;
  // The keys four at a time: how far the bridge moved under each in the last sample (four
  // independent sums over the board's modes), their strings, their push on the board (its force
  // read and written once for the four).
  for (int a0 = 0; a0 < nActive_; a0 += 4) {
    const int g = std::min(4, nActive_ - a0);
    const float* ph[4];
    for (int u = 0; u < 4; ++u) ph[u] = phi_ + size_t(activeKeys_[a0 + std::min(u, g - 1)]) * kB;
    float acc[4], F4[4] = {0, 0, 0, 0};
    dot4(ph, ba_, kB, acc);
    for (int u = 0; u < g; ++u) {
      const int ki = activeKeys_[a0 + u];
      Key& k = key_[ki];
      k.acc = acc[u];
      float e1 = -1;
      if (k.softStep > 0 && k.damperDown) {  // (laid on slowly: the grip grows from almost nothing)
        k.softT = std::min(1.0f, k.softT + k.softStep);
        k.engage = k.softT * k.softT * k.softT;
        if (k.softT >= 1) {
          k.softStep = 0;
          k.hy = k.hv = 0;
          k.seated = true;
        }
      } else if (dContact_ && k.hasDamper) {
        if (!k.damperDown) k.lin = false;
        else if (!k.lin) damperContact(k);
        else if (((clock_ + ki) & 31) == 0) linearWatch(k);
        e1 = k.damperDown ? k.engage : 0.0f;  // (the felt's friction sideways, while it touches)
      } else if (k.landWait > 0 || k.landLeft > 0) {
        // (Landing: no grip until the felt touches; then half of it within the push (the felt
        // closing on the swinging strings: the first contact is quick), the rest as it beds in.)
        const bool touching = k.landWait == 0;
        land(k);
        if (touching) k.engage = std::min(1.0f, k.engage + 0.5f / k.landN);
      } else if (k.damperDown && k.engage < 1) k.engage = std::min(1.0f, k.engage + dt_ / 0.05f);
      // (The strings that sang longer reach the felt with more in them and throw its head off: where
      // their coupling was lowered, the felt also grips the up-and-down swing, so a note stops as fast.)
      // (seated on barely moving strings, linear: the felt's friction is each mode's own loss)
      const float e = e1 >= 0 ? (k.damperDown ? (k.lin ? 1 + k.grip : std::min(1.0f, k.grip * k.engage)) : 0.0f) : k.engage;
      if (e1 < 0) e1 = e;
      // (The longer singing is the free strings': under the damper they lose to the bridge as before.)
      const float dy = k.acc * k.invLenGain * (k.damperDown ? (k.softStep > 0 ? 1 + k.engage * (k.inBack - 1) : k.inBack) : 1.0f);
      float F = 0, Fp = 0;
      for (int j = 0; j < k.ns; ++j) {
        const Str& s = str_[k.s0 + j];
        if (!s.active) continue;
        // Both directions of swing in one pass (the bridge's coupling and output weights shared).
        float sum0, sum1;
        const size_t o = s.off;
        swing(s.use, e, e1, dy, kSide * dy, in_ + o, out_ + o, damp_ + o, x_[0] + o, v_[0] + o, x_[1] + o,
              v_[1] + o, cs_ + o, sn_ + o, cs1_ + o, sn1_ + o, &sum0, &sum1);
        F -= sum0 + kSide * sum1;
        if (pickup_) Fp -= sum0 + pickSide_ * sum1;
        if (s.longLeft > 0) {
          const float fl = stretch(str_[k.s0 + j]);
          F += fl;
          if (pickup_) Fp += fl;
        }
      }
      pick += d_.pickupGain[ki] * Fp;
      F4[u] = F * k.lenGain + (k.rearN ? rearStep(k, F) : 0.0f);
      if (ki == dumpKey_) std::fwrite(&F4[u], sizeof(float), 1, dump_);
    }
    axpy4(bf_, ph, F4, kB);
  }
  if (knocks_ > 0) {  // (the knocks in the case, at the rim)
    float f = 0;
    for (Knock& q : knock_) {
      if (q.left <= 0) continue;
      const float fq = q.amp * std::sin(float(M_PI) * (q.n - q.left + 0.5f) / q.n) * dt_;
      if (q.at >= 0) {  // (a key's: at its own bridge point)
        const float* ph = phi_ + size_t(q.at) * kB;
        for (int m = 0; m < kB; ++m) bf_[m] += ph[m] * fq;
      } else {
        f += fq;
      }
      if (--q.left == 0) --knocks_;
    }
    const float* tp = trapPhi_.data();
    if (f != 0)
      for (int m = 0; m < kB; ++m) bf_[m] += tp[m] * f;
  }
  if (testForce != 0 && kB) {  // (tests: a force at middle C's bridge point)
    const float* ph = phi_ + size_t(39) * kB;
    for (int m = 0; m < kB; ++m) bf_[m] += ph[m] * testForce * dt_;
  }
  // The soundboard's resonances.
  const float fs = float(fs_);
  for (int m = 0; m < kB; ++m) {
    const float vv = bv_[m] + bf_[m];
    const float xn = bx_[m] * bcs_[m] + vv * bsn_[m];
    const float vn = vv * bcs_[m] - bx_[m] * bsn_[m];
    const float a = (vn - bv_[m]) * fs;
    ba_[m] = (xn - bx_[m]) * binv_[m];  // how far it moved
    bx_[m] = xn;
    bv_[m] = vn;
    bacc_[m] = a;
  }
  // (How far the bridge moved under each sounding key: taken at the next sample's start.)
  // The air each patch of the board moves, and what of it reaches the two
  // microphones now.
  ringPos_ = (ringPos_ + 1) & (kRing - 1);
  float l = 0, r = 0, hs = 0;  // (hs: the board's own sound, for a blend with the hall)
  const bool blend = keepRegions_ && !regionSrc_ && hallG_ > 0;
  if (keepRegions_) std::fill(region_, region_ + kRegions, 0.0f);
  for (int p0 = 0; p0 < kP; p0 += 4) {
    const int g = std::min(4, kP - p0);
    const float* pv[4];
    for (int u = 0; u < 4; ++u) pv[u] = pvol_ + size_t(p0 + std::min(u, g - 1)) * kB;
    float q4[4];
    dot4(pv, bacc_, kB, q4);
    for (int u = 0; u < g; ++u) {
      const int p = p0 + u;
      float* ring = ring_ + size_t(p) * kRing;
      ring[ringPos_] = q4[u];
      float cl = 0, cr = 0;
      if (arrN_) {  // (the array: each of its mics, mixed to the two sides)
        const size_t at = size_t(p) * arrN_;
        const int* ai = &arrI_[at];
        const float *af = &arrF_[at], *gl = &arrGL_[at], *gr = &arrGR_[at];
        for (int m = 0; m < arrN_; ++m) {
          const int i = (ringPos_ - ai[m]) & (kRing - 1);
          const float v = ring[i] + af[m] * (ring[(i - 1) & (kRing - 1)] - ring[i]);
          cl += gl[m] * v;
          cr += gr[m] * v;
        }
      } else {
        const int i0 = (ringPos_ - tapI_[0][p]) & (kRing - 1), i1 = (ringPos_ - tapI_[1][p]) & (kRing - 1);
        cl = tapG_[0][p] * (ring[i0] + tapF_[0][p] * (ring[(i0 - 1) & (kRing - 1)] - ring[i0]));
        cr = tapG_[1][p] * (ring[i1] + tapF_[1][p] * (ring[(i1 - 1) & (kRing - 1)] - ring[i1]));
      }
      l += cl;
      r += cr;
      if (keepRegions_)
        {
        if (regionSrc_ || blend) {
          const int ih = (ringPos_ - srcI_[p]) & (kRing - 1);
          const float h = srcW_[p] * (ring[ih] + srcF_[p] * (ring[(ih - 1) & (kRing - 1)] - ring[ih]));
          if (regionSrc_) {
            region_[regionOf_[p]] += h;
          } else {
            region_[regionOf_[p]] += closeG_ * 0.5f * (cl + cr) + hallG_ * h;
            hs += h;
          }
        } else {
          region_[regionOf_[p]] += 0.5f * (cl + cr);
        }
      }
    }
  }
  side_ = pickup_ ? 0 : (blend ? closeG_ : 1.0f) * 0.5f * (l - r);
  pick_ = pick * fs;  // (what the strings gave the bridge in this sample, as a force)
  // Now and then: strings that have come to rest under their dampers stop
  // being computed; with every string and the board at rest, the piano sleeps.
  if ((++clock_ & 1023) == 0) {
    bool changed = false, hammers = false;
    const int wasQuiet = quiet_;
    quiet_ = 0;
    float loud = 0;
    for (Str& s : str_) {
      if (!s.active) continue;
      const Key& k = key_[s.key];
      hammers = hammers || k.hammerOn;
      float en = 0;
      for (int d = 0; d < 2; ++d) {
        const float* x = x_[d] + s.off;
        const float* v = v_[d] + s.off;
        for (int i = 0; i < s.n; ++i) en += x[i] * x[i] + v[i] * v[i];
      }
      loud = std::max(loud, en);
      // (played, now under its damper: once the modes above the sympathetic ones have lost 40 dB
      // they are left out again, a string ringing in sympathy as the others)
      const int symN = (s.sym + 15) & ~15;
      if (s.use > symN) {
        float hi = 0;
        for (int d = 0; d < 2; ++d) {
          const float* x = x_[d] + s.off;
          const float* v = v_[d] + s.off;
          for (int i = symN; i < s.use; ++i) hi += x[i] * x[i] + v[i] * v[i];
        }
        s.hiPeak = std::max(s.hiPeak, hi);
        if (k.hasDamper && k.damperDown && k.engage >= 1 && !k.hammerOn && k.landLeft == 0 && hi < 1e-4f * s.hiPeak) {
          for (int d = 0; d < 2; ++d) {
            std::memset(x_[d] + s.off + symN, 0, sizeof(float) * (s.use - symN));
            std::memset(v_[d] + s.off + symN, 0, sizeof(float) * (s.use - symN));
          }
          s.use = symN;
        }
      }
      // (its stretching: computed until the swing has lost 20 dB; its square then 40 dB under
      // what it was, the phantoms 20 dB further under the partials than at their loudest)
      if (s.longLeft > int(0.3 * fs_)) {
        s.longPeak = std::max(s.longPeak, en);
        if (en < 0.01f * s.longPeak) s.longLeft = int(0.3 * fs_);
      }
      // (no damper reaches the strings behind the bridge: the key stays awake until they have rung out)
      float rearEn = 0;
      for (int q = 0; q < k.rearN; ++q) rearEn += k.rx[q] * k.rx[q] + k.rv[q] * k.rv[q];
      if (!allStrings_ && k.damperDown && k.engage >= 1 && !k.hammerOn && k.landLeft == 0 && en < 1e-13f && rearEn < 1e-13f) {
        s.active = false;
        for (int d = 0; d < 2; ++d) {
          std::memset(x_[d] + s.off, 0, sizeof(float) * s.n);
          std::memset(v_[d] + s.off, 0, sizeof(float) * s.n);
        }
        changed = true;
      }
    }
    if (changed) census();
    float board = 0;
    for (int m = 0; m < kB; ++m) board += bx_[m] * bx_[m] + bv_[m] * bv_[m];
    if (!hammers && !landing_ && !knocks_ && loud < 1e-13f && board < 1e-16f && (quiet_ = wasQuiet + 1) > 2) {  // (and the air has carried the last of it)
      for (int d = 0; d < 2; ++d) {
        std::memset(x_[d], 0, sizeof(float) * total_);
        std::memset(v_[d], 0, sizeof(float) * total_);
      }
      std::memset(bx_, 0, sizeof(float) * kB);
      std::memset(bv_, 0, sizeof(float) * kB);
      std::memset(ba_, 0, sizeof(float) * kB);
      for (Key& k : key_) k.acc = 0;
      std::memset(ring_, 0, sizeof(float) * kP * kRing);
      asleep_ = true;
    }
  }
  if (keepRegions_ && regionSrc_ && !pickup_) {  // (the whole: the regions' sum, nothing left at the centre)
    float sum = 0;
    for (int j = 0; j < kRegions; ++j) sum += region_[j];
    return sum;
  }
  if (blend && !pickup_) return closeG_ * 0.5f * (l + r) + hallG_ * hs;
  return pickup_ ? pick_ : 0.5f * (l + r);
}

// The patches in regions: four bands across the board by where they lie (bass to treble, as many
// patches in each), each band halved along the strings (the keyboard's half, the tail's).
void Piano::keepRegions(bool on) {
  keepRegions_ = on && !pickup_ && nP_ > 0;
  if (!keepRegions_) return;
  const int kP = nP_;
  std::vector<int> byY(kP);
  for (int p = 0; p < kP; ++p) byY[p] = p;
  const float* pos = d_.board.patchPos;
  std::sort(byY.begin(), byY.end(), [&](int a, int b) { return pos[2 * a + 1] < pos[2 * b + 1]; });
  double sx[kRegions] = {}, sy[kRegions] = {};
  int n[kRegions] = {};
  for (int band = 0; band < 4; ++band) {
    std::vector<int> in(byY.begin() + band * kP / 4, byY.begin() + (band + 1) * kP / 4);
    std::sort(in.begin(), in.end(), [&](int a, int b) { return pos[2 * a] < pos[2 * b]; });
    for (size_t i = 0; i < in.size(); ++i) {
      const int j = 2 * band + (2 * i >= in.size() ? 1 : 0);
      regionOf_[in[i]] = j;
      sx[j] += pos[2 * in[i]];
      sy[j] += pos[2 * in[i] + 1];
      ++n[j];
    }
  }
  for (int j = 0; j < kRegions; ++j) {
    regionX_[j] = n[j] ? sx[j] / n[j] : 0;
    regionY_[j] = n[j] ? sy[j] / n[j] : 0;
  }
  // (Toward the audience: off the bentside, past the raised lid, ~30 degrees over the board. The
  // room places each region; within one, a patch nearer the listener speaks a little sooner, so
  // that the highs of a broad board add by power at a seat, not all in phase.)
  static const double kUp = knob("PJ_PIANO_HALLUP", 30) * M_PI / 180;
  const double dy = std::cos(kUp);
  for (int p = 0; p < kP; ++p) {
    const int j = regionOf_[p];
    const double lead = dy * (pos[2 * p + 1] - regionY_[j]);
    const double d = std::clamp(srcD_ - lead / 345.0 * fs_, 1.0, double(kRing - 2));
    srcI_[p] = int(d);
    srcF_[p] = float(d - int(d));
    // (The bass bands' sound adds up in the open as it did not between the crossed figure-of-eight
    // pair, which turns much of it away: each band set back to the balance the keys were voiced at.)
    static const double kTrim[4] = {knob("PJ_PIANO_HALLB0", -15), knob("PJ_PIANO_HALLB1", -13.5),
                                    knob("PJ_PIANO_HALLB2", -0.5), knob("PJ_PIANO_HALLB3", -3.5)};
    srcW_[p] = srcG_ * float(fastPow(10.0, kTrim[j / 2] / 20));
  }
}

}  // namespace pajupilli
