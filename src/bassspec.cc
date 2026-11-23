// The electric basses' instruments (src/ebass.h): the strings and the neck they share, the Jazz
// Bass's two pickups (GM 34), the split-coil bass's split coil (GM 35). Only guitar.h and the knobs: the
// unit checks (tools/pickcontact.cc) build them without the rest.
#include <algorithm>
#include <cmath>

#include "ebass.h"
#include "knob.h"

namespace pajupilli {

namespace {

// What the two basses share (research/bass_section.md 2, 4, 5): 34 in., 20 frets, the strings,
// the neck, the steel in each string. flats: flatwounds instead of roundwounds (research/
// pick_bass.md 2.4, 9.2a [E]: the wrap's turns rub on each other: the overtones lose more; a
// little stiffer; a heavier set, ~10 % more tension).
void bassStrings(GuitarSpec& g, bool flats) {
  g.strings = 4;
  g.scale = 0.864;
  g.frets = 20;
  // E1 A1 D2 G2 (and two strings the bass has not: never plucked, never woken).
  const double f[6] = {41.20, 55.00, 73.42, 98.00, 1, 1};
  const double T[6] = {154.8, 178.6, 215.6, 189.6, 1, 1};
  const double B[6] = {2.0e-4, 1.0e-4, 6.0e-5, 3.5e-5, 0, 0};
  // Losses: the internal damping's pi eta f for the fundamental (eta 0.001 .. 0.00046) and the
  // neck's support at 1 ms/kg (~0.17 /s); above it the partials' loss growing with their
  // frequency, so that at 8 kHz they last 0.4-1 s (Kemp: the form loss0 + loss1 k^1.2).
  const double eta[6] = {0.001, 0.00055, 0.00038, 0.00046, 0, 0};
  const double lossT = knob("PJ_EBASS_LOSS", 1.0), lossHigh = knob("PJ_EBASS_LOSSHIGH", 1.0);
  for (int i = 0; i < 6; ++i)
    g.str[i] = {f[i], T[i], B[i], lossT * (M_PI * eta[i] * f[i] + 0.17), lossT * lossHigh * 0.02 * std::pow(f[i] / 41.2, 1.2)};
  if (flats) {
    const double fl = knob("PJ_PBASS_FLATLOSS", 2.5);
    for (int i = 0; i < 4; ++i) {
      g.str[i].loss1 *= fl;
      g.str[i].B *= 1.2;
      g.str[i].tension *= 1.1;
    }
  }
  g.ring = g.ringOvertones = g.ringOvertones45 = 1;
  g.modes = Guitar::kMaxModes;
  g.topHz = knob("PJ_EBASS_TOP", 5000);
  g.alongDetune = 0.0006;
  g.alongJitter = 0.5;
  g.tilt = 0.3;
  g.tiltJitter = 0.3;
  g.towardsLoss = 1.15;
  // The neck: its second and third bending modes (Fleischer), heavier than a guitar's: the
  // bass strings are four times as heavy. The second makes the G string's dead spot.
  const double neck = knob("PJ_EBASS_NECK", 1.0);
  g.body = {{knob("PJ_EBASS_DEADHZ", 147), 25, 0.5 / neck, 0, 0, 0.15}, {290, 30, 0.8 / neck, 0, 0, -0.1}};
  g.radiate = 0;
  // The steel in each string's section, against the D's (all wound: ~60 % of the section).
  const double d[6] = {0.100, 0.080, 0.065, 0.045, 0.065, 0.065};
  for (int i = 0; i < 6; ++i) g.steel[i] = std::pow(d[i] * d[i] / (0.065 * 0.065), 0.8);
  g.magnetR = 2.4e-3;
}

}  // namespace

// The two-pickup bass (research/bass_section.md 2, 3, 5): numbers marked there.
GuitarSpec twoPickupBass() {
  GuitarSpec g;
  g.name = "electric-bass";
  bassStrings(g, false);
  // The J pickups: neck 156 mm, bridge 55 mm from the saddle; two poles a string, the field
  // ~15 mm wide; 3.3 H, 7.4 kOhm, 90 pF; gaps [E] (2.5-3 mm fretted out: more at rest).
  const double volts = 0.15 * knob("PJ_EBASS_VOLTS", 1.0), width = knob("PJ_EBASS_WIDTH", 7e-3);
  g.pickups = {
      {"neck", {0.156, 0.156, 0.156, 0.156, 0.156, 0.156}, {4.0e-3, 3.9e-3, 3.8e-3, 3.7e-3, 3.7e-3, 3.7e-3}, width, 0, 3.2, 7.25e3, 90e-12, volts},
      {"bridge", {0.055, 0.055, 0.055, 0.055, 0.055, 0.055}, {3.5e-3, 3.4e-3, 3.3e-3, 3.2e-3, 3.2e-3, 3.2e-3}, width, 0, 3.35, 7.5e3, 90e-12, volts}};
  // Both full up (GM 34; PJ_EBASS_PICKUP 1 bridge, 2 both, 3 neck).
  static const int kSel[3][2] = {{1, -1}, {1, 0}, {0, -1}};
  const int pos = std::clamp(int(knob("PJ_EBASS_PICKUP", 2)), 1, 3) - 1;
  g.select[0] = kSel[pos][0];
  g.select[1] = kSel[pos][1];
  g.pots = 83e3;  // (two 250 k volumes and a 250 k tone)
  return g;
}

// The split-coil bass (research/pick_bass.md 2.1, 9.2a; bass_section.md 3): the two-pickup bass's strings
// and neck (roundwounds; PJ_PBASS_FLATS=1 flatwounds), one split coil: the E/A half 149 mm from
// the saddle, the D/G half 124 mm, each string under one half; 6.4 H, 10.5 kOhm, 30 pF, its
// field as wide as the J's. Wound hotter than a J coil (twice the inductance: ~1.4 times the
// turns, the EMF with them [E]). One 250 k volume, one 250 k tone with 0.047 uF (the model's
// ElectricBassModel: PJ_PBASS_TONE).
GuitarSpec splitCoilBass() {
  GuitarSpec g;
  g.name = "pick-bass";
  bassStrings(g, knob("PJ_PBASS_FLATS", 0) > 0.5);
  // (The field along the string: a 4.8 mm pole 3.4 mm under it, ~12 mm at half its height:
  // a Gaussian's sigma ~5 mm [E]; the J's 7 is as uncertain. What a pick's top keeps above 3 kHz.)
  const double volts = 0.15 * std::sqrt(6.4 / 3.3) * knob("PJ_PBASS_VOLTS", 1.0), width = knob("PJ_PBASS_WIDTH", 5e-3);
  g.pickups = {{"split", {0.149, 0.149, 0.124, 0.124, 0.124, 0.124}, {3.5e-3, 3.4e-3, 3.3e-3, 3.2e-3, 3.2e-3, 3.2e-3}, width, 0, 6.4, 10.5e3, 30e-12, volts}};
  g.select[0] = 0;
  g.select[1] = -1;
  g.pots = 125e3;  // (the volume and the tone full up)
  return g;
}

}  // namespace pajupilli
