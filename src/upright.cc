#include "upright.h"

#include <algorithm>
#include <cmath>

#include "knob.h"
#include "uprightboard_data.h"
#include "vmath.h"

namespace pajupilli {

namespace {
double lerp(double a, double b, double t) { return a + (b - a) * std::clamp(t, 0.0, 1.0); }
}  // namespace

PianoDesign uprightDesign() {
  PianoDesign d = grandDesign();
  d.name = "upright-piano";
  static const double kE = 2.02e11, kRho = 7850;
  static const int kWound = 27;  // A0 .. B2 on the bass bridge
  // The wound strings' stiffness: the grand's shape (least near key 13), at
  // A0 2.5e-4 (research/piano_variants.md 3.1: 2.3e-4 for the grand's A0
  // core and tension on 1.19 m), 1.5e-4 at key 13, 2e-4 at B2, under the
  // lowest plain string's 2.5e-4: estimates, to be fitted to recordings.
  static const double kB0 = knob("PJ_UP_B0", 2.5e-4), kBmid = 1.5e-4, kB1 = knob("PJ_UP_B1", 2e-4);
  const double gLen52 = d.key[51].len;
  for (int key = 1; key <= 88; ++key) {
    PianoKey& s = d.key[key - 1];
    const double f = 440 * std::exp2((key - 49) / 12.0);
    s.strings = key <= 8 ? 1 : key <= kWound ? 2 : 3;
    if (key <= kWound) {  // copper-wound: the tension chosen, the winding the mass
      const double t = (key - 1) / double(kWound - 1);
      s.len = 1.19 * fastPow(0.98 / 1.19, t);
      s.tension = lerp(1100, 850, t);
      s.mu = s.tension / ((2 * s.len * f) * (2 * s.len * f));
      s.B = key <= 13 ? std::exp(lerp(std::log(kB0), std::log(kBmid), (key - 1) / 12.0))
                      : std::exp(lerp(std::log(kBmid), std::log(kB1), (key - 13) / double(kWound - 13)));
    } else if (key < 52) {  // plain, 1.10 mm at C3 thinning as the grand's; shorter than the grand's
      const double dia = lerp(1.10e-3, 0.775e-3, (key - 28) / 60.0);
      s.len = 1.03 * fastPow(gLen52 / 1.03, (key - 28) / 24.0);
      s.mu = kRho * M_PI * dia * dia / 4;
      s.tension = s.mu * (2 * s.len * f) * (2 * s.len * f);
      s.B = M_PI * M_PI * M_PI * kE * dia * dia * dia * dia / (256 * s.mu * f * f * s.len * s.len * s.len * s.len);
    }  // from C5 up: the grand's
    // The dampers: the upright's lever dampers stop where the grand's do. They stand on the
    // hammers' side of the strings and are pressed onto them by springs, not their weight: they
    // push the strings the way the hammers do, land quicker (0.18 m/s from a key, the spring
    // behind it), and are lighter (a small head on a wooden lever: x 0.6 of the grand's).
    s.damperMass *= 0.6;
  }
  d.damperSide = 1;
  d.damperFall = 0.18;
  tuneByPartials(&d);
  if (knob("PJ_UP_GRANDBOARD", 0)) return d;  // (tests: the grand's board and microphones)
  d.board.n = uprightboard::kModes;
  d.board.patches = uprightboard::kPatches;
  d.board.freq = uprightboard::kFreq;
  d.board.loss = uprightboard::kLoss;
  d.board.phi = uprightboard::kPhi;
  d.board.patchVol = uprightboard::kPatchVol;
  d.board.patchPos = uprightboard::kPatchPos;
  // Behind the board (the back open): a crossed pair (the grand's pattern and angle) at
  // the middle of its height, 60 cm off it, between the bass and the tenor.
  const double mx = knob("PJ_UP_MICX", 0.45), my = knob("PJ_UP_MICY", 0.70), mz = knob("PJ_UP_MICZ", 0.60);
  for (int c = 0; c < 2; ++c) {
    const double sgn = c ? 1 : -1;
    d.mic[c] = {mx, my + sgn * 0.085, mz, sgn * std::fabs(d.mic[c].angleDeg),
                knob("PJ_UP_MICPAT", d.mic[c].pattern)};  // (the grand's pattern and angle)
  }
  return d;
}

}  // namespace pajupilli
