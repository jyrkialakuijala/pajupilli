#include "sk7.h"

#include <algorithm>
#include <cmath>

#include "knob.h"
#include "sk7board_data.h"

namespace pajupilli {

namespace {
double lerp(double a, double b, double t) { return a + (b - a) * std::clamp(t, 0.0, 1.0); }
double lerpLog(double a, double b, double t) { return std::exp(lerp(std::log(a), std::log(b), t)); }
}  // namespace

PianoDesign sk7Design() {
  PianoDesign d = grandDesign();
  d.name = "bright-piano";
  d.seed = 7007;
  static const double kE = 2.02e11, kRho = 7850;
  // (research/shigeru_kawai_sk7.md 3, 5, 7: the scale, the felt.)
  static const double kA0 = knob("PJ_SK7_A0", 1.66), kE2 = knob("PJ_SK7_E2", 1.16);
  static const double kF2 = knob("PJ_SK7_F2", 0.76);       // the lowest plain string, x the grand's length
  static const double kGauge = knob("PJ_SK7_GAUGE", 0.96);  // its wire, x the grand's diameter (to 1 at C5)
  static const double kHard = knob("PJ_SK7_HARD", 2.0);    // the felt's stiffness at C4, x the grand's
  static const double kHardTop = knob("PJ_SK7_HARDTOP", 2.5);  // ... from C6 up
  // (the felt's loose outer layer half the grand's: pressed dense, only its shoulders needled; it
  // is what keeps a middling blow dark, research/piano.md 11: the brightness's main lever here)
  static const double kNap = knob("PJ_SK7_NAP", 0.5);      // the nap, x the grand's thickness
  static const double kNapK = knob("PJ_SK7_NAPK", 1.0);    // ... and its stiffness, x the grand's
  static const double kColour = knob("PJ_SK7_COLOUR", 0.0);  // the felt's hardening under a hard blow, + the grand's
  static const double kMass = knob("PJ_SK7_MASS", 1.0);     // the hammers' mass, x the grand's
  for (int key = 1; key <= 88; ++key) {
    PianoKey& s = d.key[key - 1];
    const double f = 440 * std::exp2((key - 49) / 12.0);
    if (key <= 20) {  // copper-wound: the grand's core and tension on a shorter string: B ~ 1 / L^2
      const double len = kA0 * std::pow(kE2 / kA0, (key - 1) / 19.0);
      s.B *= (s.len / len) * (s.len / len);
      s.len = len;
      s.mu = s.tension / ((2 * s.len * f) * (2 * s.len * f));
    } else if (key < 52) {  // plain: the grand's wire (a little thinner low down), shorter: B ~ d^2 / L^4
      const double t = (key - 21) / 31.0;
      const double dia = lerp(1.2e-3, 0.775e-3, (key - 21) / 67.0) * lerp(kGauge, 1.0, t);
      s.len *= kF2 * std::pow(1 / kF2, t);
      s.mu = kRho * M_PI * dia * dia / 4;
      s.tension = s.mu * (2 * s.len * f) * (2 * s.len * f);
      s.B = M_PI * M_PI * M_PI * kE * dia * dia * dia * dia / (256 * s.mu * f * f * s.len * s.len * s.len * s.len);
    }  // from C5 up: the grand's strings
    // The hammers: Kawai's high-compression felt, firmer than the grand's from the tenor up.
    const double hard = key <= 40 ? lerpLog(1.0, kHard, (key - 1) / 39.0) : lerpLog(kHard, kHardTop, (key - 40) / 24.0);
    s.K *= hard;
    s.nap *= kNap;
    s.napK *= kNapK;
    s.colour += kColour * std::clamp((key - 20) / 20.0, 0.0, 1.0);
    s.mh *= kMass;
  }
  tuneByPartials(&d);
  d.board.n = sk7board::kModes;
  d.board.patches = sk7board::kPatches;
  d.board.freq = sk7board::kFreq;
  d.board.loss = sk7board::kLoss;
  d.board.phi = sk7board::kPhi;
  d.board.patchVol = sk7board::kPatchVol;
  d.board.patchPos = sk7board::kPatchPos;
  // (the board's own loss x 0.7: the long-sustaining aged spruce, the hard rim; a dealer's claim, a
  // small step: middle C's fall to -40 dB 9.5 s, the grand's 5.7)
  d.boardLoss *= knob("PJ_SK7_ETA", 0.7);
  // The microphones: the grand's crossed pair, nearer: 30 cm over the strings, 40 cm from the belly
  // rail (over the hammers' half of the board), as a pop or studio grand is miked. (Placed, with
  // the board's outline, for evenness from key to key: in the top 22 keys 2.5 dB rms off the
  // neighbours' mean, the grand's 2.3; at 35 cm over the strings 3-5 dB.)
  const double mx = knob("PJ_SK7_MICX", 0.40), mz = knob("PJ_SK7_MICZ", 0.30);
  for (int c = 0; c < 2; ++c) {
    d.mic[c].x = mx;
    d.mic[c].z = mz;
  }
  return d;
}

}  // namespace pajupilli
