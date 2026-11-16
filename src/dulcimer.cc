// The hammered dulcimer (General MIDI 16), on the piano's strings and
// hammers (piano.h; research/chromatic_percussion.md 7): courses of two or
// three steel strings over a small trapezoid of spruce, struck near the
// bridge with light wooden hammers faced with leather, and no dampers: every
// note rings on into the next. A large instrument's compass, G2 .. A6 (a
// concert cimbalom's); the piano's player folds other notes in by octaves.
//
// (A first form: the scale, the hammers and the unisons are the dulcimer's;
// the soundboard is still the grand's.)
#include <algorithm>
#include <cmath>

#include "knob.h"
#include "piano.h"

namespace pajupilli {

PianoDesign dulcimerDesign() {
  PianoDesign des = grandDesign();
  des.name = "dulcimer";
  static const double kE = 2.02e11, kRho = 7850;
  const double hard = knob("PJ_DULCIMER_HARD", 6.0), near = knob("PJ_DULCIMER_STRIKE", 0.14);
  for (int key = 1; key <= 88; ++key) {
    PianoKey& s = des.key[key - 1];
    const int midi = key + 20;
    const double f = 440 * std::exp2((key - 49) / 12.0);
    s.strings = midi < 55 ? 2 : 3;
    // The scale: 166 mm at D6, 1.7 times longer for each octave down (not
    // twice: the wire thickens instead), no longer than 1.2 m.
    s.len = std::clamp(0.166 * std::pow(1.7, (86 - midi) / 12.0), 0.10, 1.2);
    const double d = midi >= 62 ? 0.5e-3 : std::min(0.9e-3, 0.5e-3 + 0.4e-3 * (62 - midi) / 19.0);
    s.mu = kRho * M_PI * d * d / 4;
    s.tension = s.mu * (2 * s.len * f) * (2 * s.len * f);
    if (s.tension < 80) {  // (the lowest courses are overspun: mass without stiffness)
      s.tension = 80;
      s.mu = s.tension / ((2 * s.len * f) * (2 * s.len * f));
    }
    s.B = M_PI * M_PI * M_PI * kE * d * d * d * d / (64 * s.tension * s.len * s.len);
    s.strike = near;
    // The hammers: a few grams of wood, their striking face leather: far
    // harder than a piano's felt, and no soft outer layer.
    s.mh = 1e-3 * (3.5 - 1.5 * (key - 1) / 87.0);
    s.K *= hard;
    s.p = 2.2;
    s.colour = 0;
    s.nap = 0;
    s.napK = 0;
    s.hasDamper = false;
  }
  tuneByPartials(&des);
  // The courses are never quite in tune with themselves: the shimmer.
  const double mistune = knob("PJ_DULCIMER_MISTUNE", 1.0);
  des.unisonCents[0] = mistune * 1.2;
  des.unisonCents[1] = mistune * 0.8;
  return des;
}

}  // namespace pajupilli
