#include "guitaramp.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "knob.h"
#include "oversample_data.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

// A Fender Twin Reverb (AB763), clean: research/electric_guitars.md 11.3 (the blackface stack:
// 250 pF, 100 nF, 47 nF; treble 250 k, bass 250 k, mid 10 k, slope 100 k [YS06]) after the first
// stage. Its knobs as a jazz or clean player leaves them; the volume at about 3 of 10 (a log
// pot: 0.2 of the signal): clean for a single coil, a hard strum just touching the power stage.
GuitarAmp::Preset GuitarAmp::twin() {
  Preset p{};
  p.name = "twin";
  p.stage[0] = {40, 0, 18, 0};
  p.stage[1] = {40, 0, 22, 0};
  p.stages = 2;
  p.stackAfter = 1;
  p.c1 = 250e-12; p.c2 = 100e-9; p.c3 = 47e-9; p.r1 = 250e3; p.r2 = 250e3; p.r3 = 10e3; p.r4 = 100e3;
  p.treble = knob("PJ_AMP_TREBLE", 0.6);
  p.mid = knob("PJ_AMP_MID", 0.5);
  p.bass = knob("PJ_AMP_BASS", 0.4);
  p.volume = knob("PJ_AMP_VOLUME", 0.2);
  p.bright = 0;
  p.master = 1;
  p.clip = knob("PJ_AMP_CLIP", 80.0);
  p.cabinet = 0;
  p.oversample = 1;
  return p;
}

// The same with a compressor pedal in front (GM 29: research/electric_guitars.md 13): ratio 4
// above 50 mV, its make-up putting a 0.2 V peak where it was.
GuitarAmp::Preset GuitarAmp::twinComp() {
  Preset p = twin();
  p.name = "twin-comp";
  p.compRatio = knob("PJ_AMP_COMP", 4.0);
  p.compAt = knob("PJ_AMP_COMPAT", 0.05);
  return p;
}

// A Marshall JCM800 2203 at crunch (GM 30; 11.2-11.5 [E] from the circuit): V1A (gain ~50, its
// cathode capacitor small: the lowest bass not amplified), the preamp volume with its 470 pF
// bright capacitor, V1B biased cold (10 k unbypassed: less gain, clipping its lower half first;
// the Miller roll-off from 7 kHz), the cathode follower into the Marshall stack (470 pF, 22 nF,
// 22 nF; 220 k, 1 M, 22 k, 33 k), the master, the EL34s. The knobs as a rock player sets them for
// crunch: the gain low (0.08 of the pot: about 3 of 10, a log pot) so that soft picking comes
// through nearly clean, a hard stroke drives the cold stage well past its knee; the master at
// 0.6: the power stage at the edge on chords. PJ_AMP_OS=1: without oversampling (to compare).
GuitarAmp::Preset GuitarAmp::jcm800() {
  Preset p{};
  p.name = "jcm800";
  p.stage[0] = {50, 0, 35, 0};
  p.stage[1] = {30, -1.2, 15, 7000};
  p.stages = 2;
  p.stackAfter = 2;
  p.c1 = 470e-12; p.c2 = 22e-9; p.c3 = 22e-9; p.r1 = 220e3; p.r2 = 1e6; p.r3 = 22e3; p.r4 = 33e3;
  p.treble = knob("PJ_AMP_TREBLE", 0.7);
  p.mid = knob("PJ_AMP_MID", 0.6);
  p.bass = knob("PJ_AMP_BASS", 0.5);
  p.volume = knob("PJ_AMP_GAIN", 0.08);
  p.bright = 470e-12;
  p.master = knob("PJ_AMP_MASTER", 0.6);
  p.clip = knob("PJ_AMP_CLIP", 80.0);
  p.cabinet = 1;
  p.oversample = knob("PJ_AMP_OS", 4) > 1.5 ? 4 : 1;
  // A compressor pedal in front (the rock player's: it keeps a decaying note driving the amp, so
  // it sustains and sings instead of falling back to clean): ratio 2.5 above 0.2 V, a humbucker's
  // 0.5 V peak left where it was. A held note keeps its level through its first second and is
  // 5 dB stronger at 2 s; the cost, touch: soft against hard picking 6.5 dB apart, not 12.
  // PJ_AMP_COMP=1: none.
  p.compRatio = knob("PJ_AMP_COMP", 2.5);
  p.compAt = knob("PJ_AMP_COMPAT", 0.2);
  p.compRef = 0.5;
  return p;
}

// A Soldano SLO-100's overdrive channel (GM 31: research/distortion_guitar.md 2.1, 10.2; Kuehnel's
// reading of the circuit): V1a (36 dB; its cathode partly bypassed: the bass 3 dB under the
// treble below ~170 Hz), the gain pot (470 k with 2000 pF across it, then the 500 k pot: the bass
// 5.6 dB down, the treble whole; the pot at about 4 of 10), V1b (32 dB, its bass 4.5 dB under),
// a 0.68 divider, V2a the cold clipper (39 k unbypassed, bias near cut-off: ~2.4x, its plate
// rolled off from 1.6 kHz), 0.77, V2b (38 dB: where it distorts most; its coupling capacitor
// small, 72 Hz: the bass kept out), the DC-coupled cathode follower, then the stack (Marshall
// values: the Soldano's are not given) after all of it, the master low (the 6L6s nearly clean:
// sag and a soft knee on chord attacks), resonance and presence, a closed 4x12 of Vintage 30s
// and an SM57. A humbucker's 0.5 V is some 35 dB over V2b's knee; a soft note still 20. The
// noise of the pickup and the hum, and a gate on the guitar's own signal (2 mV) to close on them.
// The stages at 2x with their curves antialiased (PJ_AMP_OS, PJ_AMP_ADAA). PJ_AMP_TS: a Tube
// Screamer in front (its level; 0: none), drive PJ_AMP_TSDRIVE.
GuitarAmp::Preset GuitarAmp::slo() {
  Preset p{};
  p.name = "slo";
  // (Its grid stoppers, 68 k and 220 k, and its high plate resistors: grid current charges the
  // coupling capacitors some five times slower than in the Marshall. Faster, a hard attack
  // would push each stage cold for tens of milliseconds: thin pulses, a dark, blocked attack.)
  const double grid = knob("PJ_AMP_GRID", 0.2);
  p.stage[0] = {60, 0, 10, 0, 170, -3, 0, 0, 1, 0, grid};
  p.stage[1] = {40, 0, 15, 0, 130, -4.5, 235, -5.6, 0.68, 0, grid};
  p.stage[2] = {2.4, -2.2, 30, 1600, 0, 0, 0, 0, 0.77, 0, grid};
  p.stage[3] = {75, 0, 72, 9000, 0, 0, 0, 0, 1, knob("PJ_AMP_FOLLOWER", 1.8), grid};
  p.stages = 4;
  p.stackAfter = 4;
  p.c1 = 470e-12; p.c2 = 22e-9; p.c3 = 22e-9; p.r1 = 220e3; p.r2 = 1e6; p.r3 = 22e3; p.r4 = 33e3;
  p.treble = knob("PJ_AMP_TREBLE", 0.6);
  p.mid = knob("PJ_AMP_MID", 0.5);
  p.bass = knob("PJ_AMP_BASS", 0.6);
  p.volume = knob("PJ_AMP_GAIN", 0.1);  // (about 4 of 10: a hard stroke's attack still drives it harder than a soft one's)
  p.bright = 0;
  p.master = knob("PJ_AMP_MASTER", 0.4);
  p.clip = knob("PJ_AMP_CLIP", 80.0);
  p.knee = 2;
  p.resonance = knob("PJ_AMP_RESONANCE", 4.0);
  p.presence = knob("PJ_AMP_PRESENCE", 3.0);
  p.cabinet = 2;
  const int os = int(knob("PJ_AMP_OS", 2));  // (1, 2 or 4)
  p.oversample = os <= 1 ? 1 : os >= 3 ? 4 : 2;
  p.adaa = knob("PJ_AMP_ADAA", 1) > 0.5;
  // (The noise: the pickup's winding, the 68 k grid stopper and the first tube, ~8 uV rms; the
  // hum the coils pick up and the amp's own (heaters, the supply's ripple), as one at the input:
  // an ungated rest some 45 dB under a held note.)
  p.hiss = knob("PJ_AMP_HISS", 8e-6);
  p.hum = knob("PJ_AMP_HUM", 250e-6);
  p.humHz = knob("PJ_AMP_HUMHZ", 50);
  p.gateAt = knob("PJ_AMP_GATE", 2e-3);
  p.gateRelease = knob("PJ_AMP_GATEREL", 0.08);
  p.gateHold = knob("PJ_AMP_GATEHOLD", 0.015);
  p.gateDrop = knob("PJ_AMP_GATEDROP", 20);
  p.tsLevel = knob("PJ_AMP_TS", 0.0);
  p.tsDrive = knob("PJ_AMP_TSDRIVE", 0.0);
  p.tsTone = knob("PJ_AMP_TSTONE", 0.55);
  return p;
}

// GM 34: a Jazz Bass on a splitter into an active DI (10 MOhm) and an Ampeg B-15N's 1966 channel
// (1 MOhm: the pickup sees 0.9 MOhm), the two blended half and half, then an LA-2A (research/
// bass_amps.md 10.1; the B-15N drawn by Piazza, 3.1). The amp: V1a and V1b (6SL7s, 470 k and 220
// k plates, their cathodes unbypassed: ~30x each and a swing four times a 12AX7's), the volume
// between them, the James stack after V1b (bass 1 M with 220 k / 22 k and 1 nF / 10 nF, treble 1 M
// with 470 pF / 4.7 nF, 120 k to the output, the 270 k from the bass wiper; fed from V1b's ~37 k,
// into ~500 k), the phase splitter (V3, ~10x), two 6L6s (a soft knee) on a 5AR4 rectifier (the
// supply sags more), a 1x15 with an RE20 at ~15 cm. The coupling corners of a few Hz, the output
// at 8 Hz. The DI's top rolled off a little (the engineers' "roll off the top"). The LA-2A at 3:1:
// a mf line 3 dB under, a ff one 6. Knobs PJ_BAMP_*.
GuitarAmp::Preset GuitarAmp::b15Di() {
  Preset p{};
  p.name = "b15-di";
  const double sw = knob("PJ_BAMP_SWING", 4.0);
  p.stage[0] = {30, 0, 3, 0};
  p.stage[1] = {30, 0, 16, 0};
  p.stage[2] = {10, 0, 3, 0};
  for (int i = 0; i < 2; ++i) p.stage[i].swing = sw;
  // (The phase splitter into the push-pull pair: its even harmonic cancels in the output
  // transformer; what is left of its curve is its clipping, far off.)
  p.stage[2].swing = 2.5 * sw;
  p.stages = 3;
  p.stackAfter = 2;
  p.stackType = 1;
  p.james = {37e3, 220e3, 1e6, 1e-9, 10e-9, 22e3, 470e-12, 1e6, 4.7e-9, 120e3, 270e3, 500e3};
  p.treble = knob("PJ_BAMP_TREBLE", 0.45);
  p.bass = knob("PJ_BAMP_BASS", 0.6);
  p.mid = 0.5;
  p.volume = knob("PJ_BAMP_VOLUME", 0.15);
  p.bright = 0;
  p.master = 1;
  p.clip = knob("PJ_BAMP_CLIP", 150.0);
  p.knee = 2;
  p.sagDepth = 0.25;
  p.hpOut = 8;
  p.cabinet = 4;
  p.oversample = 1;
  p.inputR = knob("PJ_BAMP_INPUTR", 0.9e6);
  p.blend = knob("PJ_BAMP_BLEND", 0.5);
  p.di[0] = {3, 5000, 0.707, -2};
  p.dis = 1;
  p.diPolarity = int(knob("PJ_BAMP_POLARITY", 0));
  p.diAlign = knob("PJ_BAMP_ALIGN", 1) > 0.5;
  p.micDelay = knob("PJ_BAMP_MICDELAY", 0);
  p.studio = int(knob("PJ_BAMP_STUDIO", 1));
  p.stRatio = knob("PJ_BAMP_RATIO", 3);
  p.stAt = knob("PJ_BAMP_THRESHOLD", 0.023);
  p.stAttack = 0.01;
  p.stRelease = 0.06;
  p.stTail = 1.0;
  p.stMakeup = knob("PJ_BAMP_MAKEUP", 4);
  return p;
}

// GM 35 (the Precision with its pick, src/ebass.cc PickBassPlayer): an Ampeg SVT's Normal input
// (research/bass_amps.md 10.2; Piazza's 1972 drawing, 3.2): V1 a 12AU7-like stage (~13x, a swing
// three times a 12AX7's), the volume, V3a (~18x), the James stack (treble 47 pF / 470 pF: a higher
// shelf), the mid at 800 Hz +3 dB, the driver; the master half up, six 6550s on silicon (little
// sag, a harder knee), at 4x (the growl on hard attacks); an 8x10 with a D112 close. The DI 35 %;
// an 1176 at 4:1, its slowest attack, a fast release, its threshold set on the P played (0.04 V:
// a mf line hardly touched, a ff one ~2.5 dB down, E1 v40 -> v120 11 dB after it: research/
// bass_amps.md 10.6 item 9). PJ_BAMP_MID (Hz), PJ_BAMP_MIDDB, Ultra Lo
// and Hi (PJ_BAMP_ULTRALO, PJ_BAMP_ULTRAHI: 1 on).
GuitarAmp::Preset GuitarAmp::svt() {
  Preset p{};
  p.name = "svt";
  p.stage[0] = {13, 0, 2, 0};
  p.stage[0].swing = 3;
  p.stage[1] = {18, 0, 16, 0};
  p.stage[1].swing = 2;
  p.stage[2] = {8, 0, 3, 0};
  p.stage[2].swing = 3;
  p.stages = 3;
  p.stackAfter = 2;
  p.stackType = 1;
  p.james = {6e3, 220e3, 1e6, 1e-9, 10e-9, 22e3, 47e-12, 1e6, 470e-12, 120e3, 0, 1e6};
  p.treble = knob("PJ_BAMP_TREBLE", 0.55);
  p.bass = knob("PJ_BAMP_BASS", 0.6);
  p.mid = 0.5;
  p.voice[p.voices++] = {2, knob("PJ_BAMP_MID", 800), 1.0, knob("PJ_BAMP_MIDDB", 3)};
  if (knob("PJ_BAMP_ULTRALO", 0) > 0.5) {
    p.voice[p.voices++] = {4, 40, 0.7, 2.5};
    p.voice[p.voices++] = {2, 500, 0.7, -11};
  }
  if (knob("PJ_BAMP_ULTRAHI", 0) > 0.5) p.voice[p.voices++] = {3, 8000, 0.7, 8.5};
  p.volume = knob("PJ_BAMP_VOLUME", 0.35);
  p.bright = 0;
  p.master = knob("PJ_BAMP_MASTER", 0.5);
  p.clip = knob("PJ_BAMP_CLIP", 60.0);
  p.knee = 3;
  p.sagDepth = 0.08;
  p.hpOut = 8;
  p.cabinet = 3;
  p.oversample = 4;
  p.inputR = 0.9e6;
  p.blend = knob("PJ_BAMP_BLEND", 0.35);
  p.diAlign = true;
  p.studio = 2;
  p.stRatio = 4;
  p.stAt = knob("PJ_BAMP_THRESHOLD", 0.04);
  p.stAttack = 0.0008;
  p.stRelease = 0.15;
  p.stMakeup = knob("PJ_BAMP_MAKEUP", 4);
  return p;
}

// GM 37 (src/ebass.cc SlapBassPlayer): the DI first (75 %; the Markbass-like VPF at noon:
// +4 dB at 35 Hz, -6 at 380, +4 at 10 kHz), a clean solid-state head (GK-like: op-amp stages
// straight, its EQ with the Mid Contour in) into a 4x10 with a horn; an 1176 at 4:1 (research/
// bass_amps.md 10.4).
GuitarAmp::Preset GuitarAmp::slapDi() {
  Preset p{};
  p.name = "slap-di";
  p.stage[0] = {4, 0, 3, 0};
  p.stage[0].swing = 20;
  p.stages = 1;
  p.stackAfter = -1;
  p.voice[0] = {4, 60, 0.7, 3};
  p.voice[1] = {2, 250, 1.0, -3};
  p.voice[2] = {2, 500, 1.4, -8};
  p.voice[3] = {2, 1000, 1.0, 2};
  p.voice[4] = {3, 4000, 0.7, 3};
  p.voices = 5;
  p.volume = 1;
  p.master = 1;
  p.clip = 200;
  p.knee = 3;
  p.sagDepth = 0;
  p.hpOut = 8;
  p.cabinet = 5;
  p.oversample = 1;
  p.inputR = 0.9e6;
  p.blend = knob("PJ_BAMP_BLEND", 0.75);
  p.di[0] = {4, 35, 0.7, 4};
  p.di[1] = {2, 380, 0.7, -6};
  p.di[2] = {3, 10000, 0.7, 4};
  p.dis = 3;
  p.diAlign = true;
  p.studio = int(knob("PJ_BAMP_STUDIO", 2));
  p.stRatio = knob("PJ_BAMP_RATIO", 4);
  p.stAt = knob("PJ_BAMP_THRESHOLD", 0.08);
  p.stAttack = 0.0008;
  p.stRelease = 0.15;
  p.stMakeup = knob("PJ_BAMP_MAKEUP", 4);
  return p;
}

// GM 38 (research/slap_bass_2.md 4, 5.3, revising bass_amps.md 10.5): every SoundFont's 38 is
// brighter than its 37 and rings longer, but no more squashed (its crest factor as high or
// higher): the brightness is the active bass's (src/ebass.cc), not the chain's. So 37's chain,
// the DI a little more of the blend (80 %), the 1176 at 4:1 with a slower attack (1.5 ms: the
// pop's snap kept) and a quicker release (120 ms). The stomp compressor (PJ_BAMP_STOMP: its
// ratio, 0 off) and the SansAmp-like drive on the DI (PJ_BAMP_DRIVEMIX: its share, 0 off) as
// bass_amps.md 10.5 had them, off unless asked.
GuitarAmp::Preset GuitarAmp::slapSquash() {
  Preset p = slapDi();
  p.name = "slap-squash";
  p.blend = knob("PJ_BAMP_BLEND", 0.8);
  p.compRatio = knob("PJ_BAMP_STOMP", 0);
  p.compAt = knob("PJ_BAMP_COMPAT", 0.05);
  p.compRef = 0.2;
  p.driveMix = knob("PJ_BAMP_DRIVEMIX", 0);
  p.driveAt = p.driveMix > 0 ? knob("PJ_BAMP_DRIVE", 0.03) : 0;
  p.stRatio = knob("PJ_BAMP_RATIO", 4);
  p.stAttack = knob("PJ_BAMP_ATTACK", 0.0015);
  p.stRelease = 0.12;
  return p;
}

// GM 27 (research/jazz_guitar.md 5, 9.2, 12.3): a Polytone Mini-Brute-like solid-state amp.
// Its preamp an op amp (one stage, straight: a swing far beyond what a humbucker gives); no Fender
// stack and its 400 Hz scoop but an active EQ (a design: the circuit was not found): +3 dB under
// 150 Hz, +2 dB at 500 Hz, -2 dB above 2.5 kHz (PJ_JAZZ_BASS, _MID, _TREBLE in dB; the research's
// -4 made a soft note dull once the tone pot and the 15" had taken theirs); a solid-state
// power stage with its headroom (clean); a closed 1x15 (cabinet 6). Then the speaking strip
// (guitaramp.h Preset::presDb ...): the presence +2 dB at 3.4 kHz (PJ_JAZZ_PRESENCE, _PRESHZ),
// opened and closed by the band's own level, 0.8 dB a dB (PJ_JAZZ_SPEAK) about its level on a firm
// mf attack (PJ_JAZZ_PRESREF: -64 dB of the output; a soft phrase lies 10-20 dB under it and is
// left as dark as the amp has it), within -4 .. +8 dB (PJ_JAZZ_PRESMAX); the compressor 2:1
// (PJ_JAZZ_COMP; 1: none) above -56 dB rms (PJ_JAZZ_COMPAT: a hard stroke's peak), its ear on the
// body: soft playing passes untouched.
// PJ_JAZZ_STRIP=0: the strip left out (the amp alone).
GuitarAmp::Preset GuitarAmp::jazz() {
  Preset p{};
  p.name = "jazz";
  p.stage[0] = {4, 0, 3, 0};
  p.stage[0].swing = 20;
  p.stages = 1;
  p.stackAfter = -1;
  p.voice[0] = {4, 150, 0.7, knob("PJ_JAZZ_BASS", 3)};
  p.voice[1] = {2, 500, 0.7, knob("PJ_JAZZ_MID", 2)};
  p.voice[2] = {3, 2500, 0.7, knob("PJ_JAZZ_TREBLE", -2)};
  p.voices = 3;
  p.volume = 1;
  p.master = 1;
  p.clip = 200;
  p.knee = 3;
  p.sagDepth = 0;
  p.cabinet = 6;
  p.oversample = 1;
  if (knob("PJ_JAZZ_STRIP", 1) > 0.5) {
    p.presDb = knob("PJ_JAZZ_PRESENCE", 2);
    p.presHz = knob("PJ_JAZZ_PRESHZ", 3400);
    p.presQ = 0.8;
    p.speak = knob("PJ_JAZZ_SPEAK", 0.8);
    p.presRef = knob("PJ_JAZZ_PRESREF", -64);
    p.presLo = -4;
    p.presHi = knob("PJ_JAZZ_PRESMAX", 8);
    p.scRatio = knob("PJ_JAZZ_COMP", 2);
    p.scAt = knob("PJ_JAZZ_COMPAT", -56);
  }
  return p;
}

// The triode curve's integral, for antialiasing: tabulated from -32 to +16 V (the curve taken as
// straight between the table's points, 1/64 V apart, and its integral exact for that), beyond
// in closed form (the cut-off side exactly; the grid side from its expansion: within 1e-9).
namespace {
constexpr double kTLo = -32, kTHi = 16, kTStep = 1.0 / 64;
constexpr int kTN = int((kTHi - kTLo) / kTStep) + 1;
struct TriodeTable {
  double f[kTN], F[kTN];
};
double cutoffSide(double x) {  // the integral of y + 0.04 y^2, y = x / sqrt(1 + (x / a)^2), for x <= -32
  const double a = 2.8, z = x / a, iz = 1 / z;
  const double at = -kPi / 2 - iz + iz * iz * iz / 3 - iz * iz * iz * iz * iz / 5;  // atan(z), z < -11
  return a * a * std::sqrt(1 + z * z) + 0.04 * (a * a * x - a * a * a * at);
}
double gridSide(double x) {  // the same for y = x / (1 + (x / b)^4)^(1/4), x >= 16
  const double b = 1.4, b2 = b * b, b5 = b2 * b2 * b, x3 = x * x * x;
  return b * x + b5 / (12 * x3) + 0.04 * (b2 * x + b5 * b / (6 * x3));
}
const TriodeTable* gTriode = nullptr;
}  // namespace

double GuitarAmp::triodeF(double x) {
  const TriodeTable& t = *gTriode;
  if (x <= kTLo) return cutoffSide(x) - cutoffSide(kTLo);
  if (x >= kTHi) return t.F[kTN - 1] + gridSide(x) - gridSide(kTHi);
  const double p = (x - kTLo) * (1 / kTStep);
  int i = int(p);
  double u = p - i;
  if (i >= kTN - 1) { i = kTN - 2; u = 1; }
  return t.F[i] + kTStep * u * (t.f[i] + 0.5 * u * (t.f[i + 1] - t.f[i]));
}

double GuitarAmp::triodeLin(double x) {
  if (x <= kTLo || x >= kTHi) return triode(x);
  const TriodeTable& t = *gTriode;
  const double p = (x - kTLo) * (1 / kTStep);
  int i = int(p);
  double u = p - i;
  if (i >= kTN - 1) { i = kTN - 2; u = 1; }
  return t.f[i] + u * (t.f[i + 1] - t.f[i]);
}

GuitarAmp::Biquad GuitarAmp::rbj(int type, double fs, double f, double q, double db) {
  const double w = 2 * kPi * std::min(f, 0.45 * fs) / fs, cw = std::cos(w), al = std::sin(w) / (2 * q);
  double b0, b1, b2, a0, a1, a2;
  if (type == 0) {
    b0 = (1 - cw) / 2; b1 = 1 - cw; b2 = b0; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al;
  } else if (type == 1) {
    b0 = (1 + cw) / 2; b1 = -(1 + cw); b2 = b0; a0 = 1 + al; a1 = -2 * cw; a2 = 1 - al;
  } else if (type == 2) {
    const double A = std::pow(10, db / 40);
    b0 = 1 + al * A; b1 = -2 * cw; b2 = 1 - al * A; a0 = 1 + al / A; a1 = -2 * cw; a2 = 1 - al / A;
  } else {
    const double A = std::pow(10, db / 40), r = 2 * std::sqrt(A) * al;
    b0 = A * ((A + 1) + (A - 1) * cw + r); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - r);
    a0 = (A + 1) - (A - 1) * cw + r; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - r;
  }
  Biquad b;
  b.b0 = b0 / a0; b.b1 = b1 / a0; b.b2 = b2 / a0; b.a1 = a1 / a0; b.a2 = a2 / a0;
  return b;
}

// rbj's types and a low shelf (4). (Kept apart: rbj's own code is what the guitar presets ran.)
GuitarAmp::Biquad GuitarAmp::eq(const Eq& e, double fs) {
  if (e.type != 4) return rbj(e.type, fs, e.f, e.q, e.db);
  const double w = 2 * kPi * std::min(e.f, 0.45 * fs) / fs, cw = std::cos(w), al = std::sin(w) / (2 * e.q);
  const double A = std::pow(10, e.db / 40), r = 2 * std::sqrt(A) * al;
  const double b0 = A * ((A + 1) - (A - 1) * cw + r), b1 = 2 * A * ((A - 1) - (A + 1) * cw), b2 = A * ((A + 1) - (A - 1) * cw - r);
  const double a0 = (A + 1) + (A - 1) * cw + r, a1 = -2 * ((A - 1) + (A + 1) * cw), a2 = (A + 1) + (A - 1) * cw - r;
  Biquad b;
  b.b0 = b0 / a0; b.b1 = b1 / a0; b.b2 = b2 / a0; b.a1 = a1 / a0; b.a2 = a2 / a0;
  return b;
}

std::complex<double> GuitarAmp::at(const Biquad& b, double f, double fs) {
  const std::complex<double> z1 = std::polar(1.0, -2 * kPi * f / fs), z2 = z1 * z1;
  return (b.b0 + b.b1 * z1 + b.b2 * z2) / (1.0 + b.a1 * z1 + b.a2 * z2);
}

std::complex<double> GuitarAmp::at(const OnePole& o, double f, double fs) {
  const std::complex<double> z1 = std::polar(1.0, -2 * kPi * f / fs);
  return (o.b0 + o.b1 * z1) / (1.0 + o.a1 * z1);
}

GuitarAmp::OnePole GuitarAmp::onePole(int type, double fs, double f) {
  const double k = std::tan(kPi * std::min(f, 0.45 * fs) / fs);
  OnePole o;
  o.a1 = (k - 1) / (k + 1);
  if (type == 0) { o.b0 = k / (k + 1); o.b1 = o.b0; }
  else { o.b0 = 1 / (k + 1); o.b1 = -o.b0; }
  return o;
}

// A first-order low shelf: the bass db (negative: under) the treble, about f its middle: (s +
// g wp) / (s + wp), g the bass's share, its corners f sqrt(g) and f / sqrt(g).
GuitarAmp::OnePole GuitarAmp::lowShelf(double fs, double f, double db) {
  const double g = std::pow(10, db / 20), wp = 2 * kPi * f / std::sqrt(g), wz = g * wp, c = 2 * fs;
  OnePole o;
  o.b0 = (c + wz) / (c + wp);
  o.b1 = (wz - c) / (c + wp);
  o.a1 = (wp - c) / (c + wp);
  return o;
}

GuitarAmp::GuitarAmp(double fs, const Preset& p) : fs_(fs), fsOs_(fs * std::max(1, p.oversample)), p_(p) {
  connect(Source{});
  setTone(p.treble, p.mid, p.bass);
  for (int i = 0; i < 4; ++i) {
    const double* c = p.oversample == 2 ? oversample::kSos2[i] : oversample::kSos[i];
    up_[i].b0 = c[0]; up_[i].b1 = c[1]; up_[i].b2 = c[2]; up_[i].a1 = c[3]; up_[i].a2 = c[4];
    down_[i] = up_[i];
  }
  p_.stages = std::clamp(p_.stages, 1, kMaxStages);
  if (p_.adaa) {
    // (The curve's integral, once: the curve straight between the points, its integral exact.)
    static const TriodeTable* table = [] {
      TriodeTable* r = new TriodeTable;
      for (int i = 0; i < kTN; ++i) r->f[i] = triode(kTLo + i * kTStep);
      r->F[0] = 0;
      for (int i = 1; i < kTN; ++i) r->F[i] = r->F[i - 1] + 0.5 * kTStep * (r->f[i - 1] + r->f[i]);
      return r;
    }();
    gTriode = table;
  }
  // The stages' coupling capacitors and Miller roll-offs, their cathodes' and dividers' shelves.
  for (int i = 0; i < p_.stages; ++i) {
    const Stage& s = p_.stage[i];
    Tube& t = t_[i];
    t.hp = onePole(1, fsOs_, s.hp);
    if (s.lp > 0) { t.lp = onePole(0, fsOs_, s.lp); t.lpOn = true; }
    if (s.shelfDb != 0 && s.shelfHz > 0) { t.shelf = lowShelf(fsOs_, s.shelfHz, s.shelfDb); t.shelfOn = true; }
    if (s.inDb != 0 && s.inHz > 0) { t.in = lowShelf(fsOs_, s.inHz, s.inDb); t.inOn = true; }
    const double b = s.swing == 1 ? s.bias : s.bias / s.swing;
    t.rest = triode(b);
    t.u1 = b;
    t.F1 = p_.adaa ? triodeF(b) : 0;
    slope_[i] = (triode(b + 1e-4) - triode(b - 1e-4)) / 2e-4;
  }
  linear_ = int(knob("PJ_AMP_LINEAR", 0));
  peaks_ = knob("PJ_AMP_PEAKS", 0) > 0;
  hpOut_ = onePole(1, fsOs_, p.hpOut);
  // The pot between the stages: its share g of 1 MOhm, a capacitor C across the part above the
  // wiper: H(s) = g (1 + s Rt C) / (1 + s g Rt C), Rt = (1 - g) 1 MOhm (the treble passes whole).
  {
    const double g = std::clamp(p.volume, 1e-4, 1.0), Rt = (1 - g) * 1e6, C = p.bright;
    if (C > 0 && Rt > 0) {
      const double c = 2 * fsOs_, a = Rt * C, b = g * Rt * C;
      const double d0 = 1 + b * c, d1 = 1 - b * c;
      pot_.b0 = g * (1 + a * c) / d0;
      pot_.b1 = g * (1 - a * c) / d0;
      pot_.a1 = d1 / d0;
    } else {
      pot_.b0 = g; pot_.b1 = 0; pot_.a1 = 0;
    }
  }
  // The speaker in its cabinet ([E] from the published curves of 12" guitar speakers, 11.5).
  const double cab = knob("PJ_AMP_CAB", 1.0);
  if (p.cabinet == 0) {
    // 2x12 open back: its resonance (a small bump, the open back losing what is below), the
    // cone's lift at 2-4 kHz, a breakup peak and dip, the fall above 5 kHz.
    cab_[0] = rbj(1, fs, 80, 1.1);
    cab_[1] = rbj(2, fs, 2400, 1.0, 3.5 * cab);
    cab_[2] = rbj(2, fs, 3700, 3.0, 3.0 * cab);
    cab_[3] = rbj(2, fs, 4600, 4.0, -6.0 * cab);
    cab_[4] = rbj(0, fs, 5200 / std::max(0.2, cab), 0.85);
    cab_[5] = rbj(0, fs, 6800 / std::max(0.2, cab), 0.6);
    nCab_ = 6;
  } else if (p.cabinet == 1) {
    // A closed 4x12 of Celestion Greenbacks (75 Hz, usable to 5 kHz): the closed box's thump
    // (a sharper resonance, a little low bump), the forward upper mids, breakup, a steeper fall.
    cab_[0] = rbj(1, fs, 72, 1.4);
    cab_[1] = rbj(2, fs, 120, 1.0, 2.0 * cab);
    cab_[2] = rbj(2, fs, 1800, 1.0, 4.0 * cab);
    cab_[3] = rbj(2, fs, 3300, 3.0, 3.0 * cab);
    cab_[4] = rbj(2, fs, 4200, 4.0, -7.0 * cab);
    cab_[5] = rbj(0, fs, 4800 / std::max(0.2, cab), 0.9);
    cab_[6] = rbj(0, fs, 6200 / std::max(0.2, cab), 0.6);
    nCab_ = 7;
  }
  if (p.cabinet == 2) {
    // A closed 4x12 of Celestion Vintage 30s, an SM57 close, just off the dust cap
    // (distortion_guitar.md 5.4's table as the start, then fitted, the amp before it as built,
    // to FluidR3's distortion-guitar samples: their pooled harmonic envelope within 4 dB from
    // 200 Hz to 8 kHz, 800 Hz 5 dB under). The closed box below its resonance (12 dB an octave)
    // and its broad bump; the low mids of the close microphone's proximity, strong (+9 dB at
    // 350 Hz, where Celestion's plot of the bare speaker is flat: the recording's, not the
    // speaker's); a wide hollow at 900 Hz; the presence pair at 2.6 and 3.25 kHz; the fall
    // above 6 kHz, steep; the cone's break-up floor near 9 kHz.
    cab_[0] = rbj(1, fs, 80, 0.6);
    cab_[1] = rbj(2, fs, 150, 0.5, 2.5 * cab);
    cab_[2] = rbj(2, fs, 350, 2.0, 9.0 * cab);
    cab_[3] = rbj(2, fs, 900, 1.0, -10.0 * cab);
    cab_[4] = rbj(2, fs, 2600, 2.0, 4.0 * cab);
    cab_[5] = rbj(2, fs, 3250, 2.9, 10.0 * cab);
    cab_[6] = rbj(0, fs, 6500 / std::max(0.2, cab), 1.5);
    cab_[7] = rbj(0, fs, 7000 / std::max(0.2, cab), 1.0);
    cab_[8] = rbj(2, fs, 9300, 2.0, knob("PJ_AMP_BREAKUP", 5.5) * cab);
    nCab_ = 9;
  }
  // The bass cabinets with their microphones (research/bass_amps.md 4.3: fitted to the 810E's
  // published range and the Eminence drivers' measured curves; the mics' presence and proximity
  // [M]). Relative to 200 Hz: the 8x10 -11 dB at 40 Hz, -3 at 58, -20 at 5 kHz; the 1x15 -4 at 40,
  // -23 at 5 kHz; the 4x10 with its horn -5.5 at 40, -1.6 at 5 kHz. (Their cones' break-up floor
  // at 8-12 kHz, -35 dB and under, is left out. The research's numbers are the analog filters':
  // here the 8x10's and the 1x15's low-passes 5-6 % higher, the bilinear's squeeze made good.)
  if (p.cabinet == 3) {
    // An Ampeg SVT-810E (8x10, sealed), a D112 ~5 cm off the centre of one cone.
    const Eq e[8] = {{1, 60, 0.8, 0}, {1, 40, 0.6, 0}, {2, 95, 1.0, 3}, {2, 470, 1.4, -3}, {2, 2000, 2.5, 5}, {0, 2440, 1.0, 0}, {0, 3070, 0.7, 0}, {2, 4000, 2.0, 3}};
    for (const Eq& q : e) cab_[nCab_++] = eq(q, fs);
  } else if (p.cabinet == 4) {
    // The B-15's 1x15 (ported), an RE20 at ~15 cm (no proximity: its Variable-D).
    const Eq e[6] = {{1, 44, 1.0, 0}, {1, 38, 0.7, 0}, {2, 600, 1.5, -2}, {2, 1900, 3.0, 7}, {0, 2310, 1.1, 0}, {0, 2940, 0.7, 0}};
    for (const Eq& q : e) cab_[nCab_++] = eq(q, fs);
  } else if (p.cabinet == 5) {
    // A 4x10 with a horn (ported), the mic between a cone and the horn; the horn's L-pad at noon.
    const Eq e[7] = {{1, 48, 1.0, 0}, {1, 35, 0.6, 0}, {2, 500, 1.4, -2}, {2, 1800, 2.0, 2.5}, {0, 2600, 0.8, 0}, {3, 3800, 0.9, 12}, {0, 14000, 0.7, 0}};
    for (const Eq& q : e) cab_[nCab_++] = eq(q, fs);
  }
  if (p.cabinet == 6) {
    // A closed 1x15 jazz speaker (research/jazz_guitar.md 5 [E]): the closed box's resonance at
    // 85 Hz (12 dB an octave under), flat to 1.5 kHz, -3 dB at 2.5 kHz, 24 dB an octave above
    // 3.5 kHz; a stiff, clean cone: no break-up peaks.
    const Eq e[4] = {{1, 85, 1.0, 0}, {2, 2500, 0.9, -2}, {0, 3700, 0.54, 0}, {0, 3700, 1.31, 0}};
    for (const Eq& q : e) cab_[nCab_++] = eq(q, fs);
  }
  // A guitar preset's fixed filters after its stages (the bass presets set theirs below).
  if (!(p.blend > 0 || p.studio > 0) && p.voices > 0) {
    p_.voices = std::clamp(p_.voices, 0, 6);
    for (int i = 0; i < p_.voices; ++i) voice_[i] = eq(p_.voice[i], fsOs_);
  }
  // The speaking strip: the presence band (a constant-peak band-pass: x + (g - 1) bp is a peak
  // of g), its detector, the compressor's.
  if (p.presDb != 0 || p.speak != 0 || p.scRatio > 1) {
    strip_ = true;
    const double w = 2 * kPi * std::min(p.presHz, 0.45 * fs) / fs, al = std::sin(w) / (2 * p.presQ), a0 = 1 + al;
    pres_.b0 = al / a0; pres_.b1 = 0; pres_.b2 = -al / a0; pres_.a1 = -2 * std::cos(w) / a0; pres_.a2 = (1 - al) / a0;
    presAtk_ = 1 - std::exp(-1 / (std::max(1e-4, p.presAttack) * fs));
    presRel_ = 1 - std::exp(-1 / (std::max(1e-3, p.presRelease) * fs));
    presSmooth_ = 1 - std::exp(-1 / (0.002 * fs));
    scAtk_ = 1 - std::exp(-1 / (std::max(1e-4, p.scAttack) * fs));
    scRel_ = 1 - std::exp(-1 / (std::max(1e-3, p.scRelease) * fs));
    presGain_ = p.presDb;
    msA_ = 1 - std::exp(-1 / (0.01 * fs));
  }
  // The power stage's resonance and presence (the feedback taken off around 100 Hz, above 3 kHz).
  if (p.resonance != 0) post_[nPost_++] = rbj(2, fs, 100, 1.5, p.resonance);
  if (p.presence != 0) post_[nPost_++] = rbj(3, fs, 3000, 0.707, p.presence);
  // The noise: the hum's one period (its odd harmonics, falling, up to 1 kHz; peak 1).
  if (p.hum > 0) {
    const int n = std::max(8, int(std::lround(fs / std::max(10.0, p.humHz))));
    humWave_.assign(n, 0.0f);
    double peak = 1e-9;
    std::vector<double> w(n, 0.0);
    for (int i = 0; i < n; ++i) {
      for (int k = 1; k * p.humHz < 1000; k += 2) w[i] += std::sin(2 * kPi * k * i / n + 0.7 * k) / std::sqrt(double(k));
      peak = std::max(peak, std::fabs(w[i]));
    }
    for (int i = 0; i < n; ++i) humWave_[i] = float(w[i] / peak);
  }
  // The gate: its detector quick to rise (0.5 ms), slower to fall (10 ms); open in 0.5 ms, closed
  // to -80 dB over its release.
  gateAtk_ = 1 - std::exp(-1 / (0.0005 * fs));
  gateRel_ = 1 - std::exp(-1 / (0.01 * fs));
  gateRise_ = 80 / (0.0005 * fs);
  gateFall_ = 80 / (std::max(0.005, p.gateRelease) * fs);
  gateGain_ = 1;
  if (p.gateAt > 0) { gateDb_ = -80; gateGain_ = 1e-4; }
  gateDebug_ = knob("PJ_AMP_GATEDEBUG", 0) > 0;
  // (The quick close: the last peak falling at 30 dB a second, so a string's own decay never
  // reaches it; the bar to open again back to the threshold in 50 ms.)
  gatePeakFall_ = std::exp(-1 / (0.3 * fs));
  gateReopenFall_ = std::exp(-1 / (0.05 * fs));
  gateDropTo_ = p.gateDrop > 0 ? std::pow(10.0, -p.gateDrop / 20) : 0;
  // The Tube Screamer (EG 11.8 [YehThesis]): 15.9 Hz in; its gain's leg 4.7 k + 0.047 uF
  // (720 Hz); R2 = 51 k + drive 500 k with 51 pF across; the tone's low-pass 723 Hz .. 10 kHz.
  if (p.tsLevel > 0) {
    const double r2 = 51e3 + std::clamp(p.tsDrive, 0.0, 1.0) * 500e3;
    tsIn_ = onePole(1, fsOs_, 15.9);
    tsLeg_ = onePole(1, fsOs_, 720);
    tsCap_ = onePole(0, fsOs_, 1 / (2 * kPi * r2 * 51e-12));
    tsTone_ = onePole(0, fsOs_, 723 * std::pow(10000 / 723.0, std::clamp(p.tsTone, 0.0, 1.0)));
    tsGain_ = r2 / 4.7e3;
  }
  sagA_ = 1 - std::exp(-1 / (0.1 * fsOs_));
  // Grid current: it charges the coupling capacitor through the stage before (~0.9 ms), which
  // lets go through the grid resistor (~22 ms).
  gridA_ = 1 - std::exp(-1 / (0.0009 * fsOs_));
  gridRel_ = 1 - std::exp(-1 / (0.022 * fsOs_));
  compAtk_ = 1 - std::exp(-1 / (0.003 * fs));
  compRel_ = 1 - std::exp(-1 / (0.2 * fs));
  if (p.compRatio > 1) compMakeup_ = std::pow(p.compRef / p.compAt, 1 - 1 / p.compRatio);
  // Bass: the fixed filters after the stages; the DI's; its drive; the mic's delay; the studio
  // compressor; the DI and the mic lined up.
  if (p.blend > 0 || p.studio > 0) {
    p_.voices = std::clamp(p_.voices, 0, 6);
    p_.dis = std::clamp(p_.dis, 0, 4);
    for (int i = 0; i < p_.voices; ++i) voice_[i] = eq(p_.voice[i], fsOs_);
    for (int i = 0; i < p_.dis; ++i) di_[i] = eq(p_.di[i], fs);
    if (p.driveAt > 0) {
      drHp_ = rbj(1, fs, 150, 0.707);
      drLp_ = rbj(0, fs, 5000, 0.707);
    }
    if (p.micDelay >= 1) delay_.assign(size_t(p.micDelay) + 1, 0.0);
    msA_ = 1 - std::exp(-1 / (0.01 * fs));
    stAtk_ = 1 - std::exp(-1 / (std::max(1e-5, p.stAttack) * fs));
    // (The opto: the quick half lets go to half in stRelease; the slow half charges over 0.1 s
    // and lets go over stTail: after a long hold the release has a tail of seconds, after a
    // single note it is quick.)
    stRel_ = 1 - std::exp(-std::log(2.0) / (std::max(1e-3, p.stRelease) * fs));
    if (p.studio == 2) stRel_ = 1 - std::exp(-1 / (std::max(1e-3, p.stRelease) * fs));
    stCharge_ = 1 - std::exp(-1 / (0.1 * fs));
    stTailA_ = 1 - std::exp(-1 / (std::max(0.01, p.stTail) * fs));
    stThrDb_ = 20 * std::log10(std::max(1e-9, p.stAt));
    bass_ = true;
    align();
  }
}

// The James stack as two biquads: its admittance matrix (seven nodes: the source's, R1's top, the
// bass wiper, the bass pot's foot, the treble pot's top, its wiper (the output), its foot) solved
// at eight points on a circle in s; the determinant and the output's numerator are polynomials of
// degree <= 4 (one per capacitor), their coefficients a DFT of the eight values. Their roots
// (Durand-Kerner) to z one by one (bilinear), paired into biquads, the gain matched at 1 kHz.
// The pots audio taper (9 % of the track below the wiper at noon: the stack flat there).
void GuitarAmp::james(double treble, double bass) {
  using C = std::complex<double>;
  const James& j = p_.james;
  const auto taper = [](double x) { return (std::pow(102.0, std::clamp(x, 0.0, 1.0)) - 1) / 101; };
  const double lo = std::clamp(taper(bass), 1e-3, 1 - 1e-3), tr = std::clamp(taper(treble), 1e-3, 1 - 1e-3);
  constexpr int N = 7, M = 8;
  const double w0 = 2 * kPi * 500;
  C den[M], num[M];
  for (int k = 0; k < M; ++k) {
    const C s = w0 * std::polar(1.0, 2 * kPi * k / M);
    C Y[N][N + 1] = {};
    const auto g = [&](int a, int b, C y) {  // an element between nodes a and b (1..7; 0 ground)
      if (a) Y[a - 1][a - 1] += y;
      if (b) Y[b - 1][b - 1] += y;
      if (a && b) { Y[a - 1][b - 1] -= y; Y[b - 1][a - 1] -= y; }
    };
    g(1, 0, 1 / j.rs);
    g(1, 2, 1 / j.r1);
    g(2, 3, 1 / ((1 - lo) * j.rb) + s * j.c1);
    g(3, 4, 1 / (lo * j.rb) + s * j.c2);
    g(4, 0, 1 / j.r2);
    g(1, 5, s * j.c3);
    g(5, 6, 1 / ((1 - tr) * j.rt));
    g(6, 7, 1 / (tr * j.rt));
    g(7, 0, s * j.c4);
    g(3, 6, 1 / j.r3);
    if (j.rw > 0) g(3, 0, 1 / j.rw);
    g(6, 0, 1 / j.rl);
    Y[0][N] = 1 / j.rs;  // (the source: 1 V behind rs)
    C det = 1;
    for (int c = 0; c < N; ++c) {
      int piv = c;
      for (int r = c + 1; r < N; ++r) if (std::abs(Y[r][c]) > std::abs(Y[piv][c])) piv = r;
      if (piv != c) { for (int q = 0; q <= N; ++q) std::swap(Y[c][q], Y[piv][q]); det = -det; }
      det *= Y[c][c];
      for (int r = c + 1; r < N; ++r) {
        const C f = Y[r][c] / Y[c][c];
        for (int q = c; q <= N; ++q) Y[r][q] -= f * Y[c][q];
      }
    }
    C v[N];
    for (int r = N - 1; r >= 0; --r) {
      C a = Y[r][N];
      for (int q = r + 1; q < N; ++q) a -= Y[r][q] * v[q];
      v[r] = a / Y[r][r];
    }
    den[k] = det;
    num[k] = v[5] * det;  // (node 6: the output)
  }
  // The coefficients (of s / w0) and the roots.
  const auto roots = [&](const C* val, std::vector<C>& r) {
    double c[5], big = 0;
    for (int i = 0; i < 5; ++i) {
      C a = 0;
      for (int k = 0; k < M; ++k) a += val[k] * std::polar(1.0, -2 * kPi * i * k / M);
      c[i] = a.real() / M;
      big = std::max(big, std::fabs(c[i]));
    }
    int n = 4;
    while (n > 0 && std::fabs(c[n]) < 1e-10 * big) --n;
    r.assign(n, C(0));
    for (int i = 0; i < n; ++i) r[i] = std::pow(C(0.4, 0.9), i + 1);
    for (int it = 0; it < 500; ++it) {
      double moved = 0;
      for (int i = 0; i < n; ++i) {
        C p = c[n], q = 1;
        for (int d = n - 1; d >= 0; --d) p = p * r[i] + c[d];
        for (int m = 0; m < n; ++m) if (m != i) q *= r[i] - r[m];
        const C step = p / (c[n] * q);
        r[i] -= step;
        moved = std::max(moved, std::abs(step) / (1 + std::abs(r[i])));
      }
      if (moved < 1e-15) break;
    }
    for (C& x : r) x *= w0;
    return std::vector<double>(c, c + 5);
  };
  std::vector<C> zr, pr;
  const std::vector<double> cn = roots(num, zr), cd = roots(den, pr);
  // To z; a missing zero at z = -1 (s = infinity).
  const double c2 = 2 * fsOs_;
  const auto toZ = [&](std::vector<C>& r) {
    for (C& x : r) x = (c2 + x) / (c2 - x);
    while (r.size() < 4) r.push_back(-1.0);
  };
  toZ(zr);
  toZ(pr);
  // Pairs: a complex root with its conjugate, the real ones in order of their distance from 1.
  const auto pairs = [](std::vector<C> r) {
    std::vector<C> out;
    std::sort(r.begin(), r.end(), [](C a, C b) { return std::abs(1.0 - a) < std::abs(1.0 - b); });
    std::vector<bool> used(r.size(), false);
    for (size_t i = 0; i < r.size(); ++i) {
      if (used[i] || std::fabs(r[i].imag()) < 1e-9) continue;
      used[i] = true;
      size_t best = i;
      double d = 1e9;
      for (size_t m = 0; m < r.size(); ++m)
        if (!used[m] && std::abs(r[m] - std::conj(r[i])) < d) { d = std::abs(r[m] - std::conj(r[i])); best = m; }
      used[best] = true;
      out.push_back(r[i]);
      out.push_back(std::conj(r[i]));
    }
    for (size_t i = 0; i < r.size(); ++i) if (!used[i]) out.push_back(C(r[i].real(), 0));
    return out;
  };
  const std::vector<C> zp = pairs(zr), pp = pairs(pr);
  for (int b = 0; b < 2; ++b) {
    const C z0 = zp[2 * b], z1 = zp[2 * b + 1], p0 = pp[2 * b], p1 = pp[2 * b + 1];
    jb_[b] = Biquad{};
    jb_[b].b0 = 1;
    jb_[b].b1 = -(z0 + z1).real();
    jb_[b].b2 = (z0 * z1).real();
    jb_[b].a1 = -(p0 + p1).real();
    jb_[b].a2 = (p0 * p1).real();
  }
  // The gain: the analog response at 1 kHz against the digital one where the bilinear puts it.
  const double wa = 2 * kPi * 1000, fd = std::atan(wa / c2) * fsOs_ / kPi;
  C ns = 0, ds = 0, sp = 1;
  for (int i = 0; i < 5; ++i) { ns += cn[i] * sp; ds += cd[i] * sp; sp *= C(0, wa / w0); }
  const C hd = at(jb_[0], fd, fsOs_) * at(jb_[1], fd, fsOs_);
  const double k = (ns / ds / hd).real();
  jb_[0].b0 *= k; jb_[0].b1 *= k; jb_[0].b2 *= k;
}

// The amp's path in small signal: the stages at their bias's slope, the pot, the stack, the
// fixed filters, the master, the output's high-pass, the oversampling filters, the power stage's
// feedback filters, the cabinet, the mic's delay.
std::complex<double> GuitarAmp::ampPath(double f) const {
  std::complex<double> h = 1;
  const auto stackAt = [&]() {
    if (p_.stackType == 1) return at(jb_[0], f, fsOs_) * at(jb_[1], f, fsOs_);
    const std::complex<double> z1 = std::polar(1.0, -2 * kPi * f / fsOs_);
    std::complex<double> b = 0, a = 0, zk = 1;
    for (int i = 0; i < 4; ++i) { b += tb_[i] * zk; a += ta_[i] * zk; zk *= z1; }
    return b / a;
  };
  if (p_.stackAfter == 0) h *= stackAt();
  for (int i = 0; i < p_.stages; ++i) {
    const Stage& s = p_.stage[i];
    const Tube& t = t_[i];
    if (t.inOn) h *= at(t.in, f, fsOs_);
    if (t.shelfOn) h *= at(t.shelf, f, fsOs_);
    h *= s.gain * slope_[i] * s.divider * at(t.hp, f, fsOs_);
    if (t.lpOn) h *= at(t.lp, f, fsOs_);
    if (p_.stackAfter == i + 1) h *= stackAt();
    if (i == 0) h *= at(pot_, f, fsOs_);
  }
  for (int i = 0; i < p_.voices; ++i) h *= at(voice_[i], f, fsOs_);
  h *= p_.master * at(hpOut_, f, fsOs_);
  if (p_.oversample > 1)
    for (int i = 0; i < 4; ++i) h *= at(up_[i], f, fsOs_) * at(down_[i], f, fsOs_);
  for (int i = 0; i < nPost_; ++i) h *= at(post_[i], f, fs_);
  for (int i = 0; i < nCab_; ++i) h *= at(cab_[i], f, fs_);
  if (!delay_.empty()) h *= std::polar(1.0, -2 * kPi * f * (delay_.size() - 1) / fs_);
  return h;
}

std::complex<double> GuitarAmp::diPath(double f) const {
  std::complex<double> h = diSign_;
  for (int i = 0; i < p_.dis; ++i) h *= at(di_[i], f, fs_);
  if (diApOn_) h *= at(diAp_, f, fs_);
  return h;
}

std::complex<double> GuitarAmp::response(int what, double f) const {
  switch (what) {
    case 0:
      if (tone_) {
        const std::complex<double> z1 = std::polar(1.0, -2 * kPi * f / fs_);
        std::complex<double> b = 0, a = 0, zk = 1;
        for (int i = 0; i < 4; ++i) { b += ib_[i] * zk; a += ia_[i] * zk; zk *= z1; }
        return b / a;
      }
      return at(input_, f, fs_);
    case 1:
      if (p_.stackType == 1) return at(jb_[0], f, fsOs_) * at(jb_[1], f, fsOs_);
      else {
        const std::complex<double> z1 = std::polar(1.0, -2 * kPi * f / fsOs_);
        std::complex<double> b = 0, a = 0, zk = 1;
        for (int i = 0; i < 4; ++i) { b += tb_[i] * zk; a += ta_[i] * zk; zk *= z1; }
        return b / a;
      }
    case 2: {
      std::complex<double> h = 1;
      for (int i = 0; i < nCab_; ++i) h *= at(cab_[i], f, fs_);
      return h;
    }
    case 3: return ampPath(f) * (bass_ ? ampNorm_ : 1 / p_.clip);
    case 4: return diPath(f);
    default: return p_.blend * diPath(f) + (1 - p_.blend) * ampPath(f) * ampNorm_;
  }
}

// The DI and the mic lined up, as an engineer does it: the amp's path level-matched to the DI at
// 200 Hz; then the polarity (if the preset leaves it open) and a second-order allpass on the DI
// (if it asks for one) that keep the blend from 30 Hz to 2 kHz nearest to the louder of the two
// alone (the cabinet's high-pass turns the mic's phase by up to half a turn around 40 Hz).
void GuitarAmp::align() {
  if (!bass_) return;
  ampNorm_ = 1 / std::max(1e-12, std::abs(ampPath(200)));
  if (p_.blend <= 0 || p_.blend >= 1) {
    diSign_ = p_.diPolarity < 0 ? -1 : 1;
    diApOn_ = false;
    return;
  }
  std::vector<double> fq;
  for (double f = 30; f < 2000; f *= 1.12) fq.push_back(f);
  std::vector<std::complex<double>> A(fq.size()), D0(fq.size());
  for (size_t i = 0; i < fq.size(); ++i) {
    A[i] = (1 - p_.blend) * ampPath(fq[i]) * ampNorm_;
    std::complex<double> h = p_.blend;
    for (int k = 0; k < p_.dis; ++k) h *= at(di_[k], fq[i], fs_);
    D0[i] = h;
  }
  const auto cost = [&](double sign, const Biquad* ap) {
    double c = 0;
    for (size_t i = 0; i < fq.size(); ++i) {
      const std::complex<double> d = sign * D0[i] * (ap ? at(*ap, fq[i], fs_) : 1.0);
      const double loss = 20 * std::log10(std::max(std::abs(d), std::abs(A[i])) / std::max(1e-12, std::abs(d + A[i])));
      c += loss > 0 ? loss * loss : 0;
    }
    return c;
  };
  double best = 1e30;
  for (int sg = -1; sg <= 1; sg += 2) {
    if (p_.diPolarity && sg != p_.diPolarity) continue;
    if (cost(sg, nullptr) < best) { best = cost(sg, nullptr); diSign_ = sg; diApOn_ = false; }
    if (!p_.diAlign) continue;
    for (double f0 = 15; f0 < 400; f0 *= 1.06)
      for (double q = 0.3; q < 3; q *= 1.15) {
        // (An allpass: the low-pass's denominator over its mirror.)
        Biquad ap = rbj(0, fs_, f0, q);
        const double a1 = ap.a1, a2 = ap.a2;
        ap.b0 = a2; ap.b1 = a1; ap.b2 = 1;
        const double c = cost(sg, &ap);
        if (c < best) { best = c; diSign_ = sg; diApOn_ = true; diAp_ = ap; }
      }
  }
  diAp_.z1 = diAp_.z2 = 0;
}

// The DI: its filters, its allpass, its drive (above 150 Hz, clipped, rolled off above 5 kHz,
// added), its polarity.
double GuitarAmp::di(double x) {
  for (int i = 0; i < p_.dis; ++i) x = di_[i].run(x);
  if (diApOn_) x = diAp_.run(x);
  if (p_.driveAt > 0) {
    const double v = drHp_.run(x) / p_.driveAt;
    x += p_.driveMix * drLp_.run(p_.driveAt * v / std::sqrt(1 + v * v));
  }
  return diSign_ * x;
}

// The studio compressor: the level (rms over two 10 ms poles), a 6 dB soft knee, the gain
// reduction's attack and release (the opto's in two halves), the make-up.
double GuitarAmp::compress(double y) {
  if (p_.studio <= 0) return y;
  ms1_ += msA_ * (y * y - ms1_);
  ms2_ += msA_ * (ms1_ - ms2_);
  const double over = 10 * std::log10(ms2_ + 1e-30) - stThrDb_, r = 1 - 1 / p_.stRatio;
  const double target = over <= -3 ? 0 : over >= 3 ? r * over : r * (over + 3) * (over + 3) / 12;
  double gr;
  if (p_.studio == 1) {
    grFast_ += (target > grFast_ ? stAtk_ : stRel_) * (target - grFast_);
    grSlow_ += (target > grSlow_ ? stCharge_ : stTailA_) * (target - grSlow_);
    gr = 0.5 * (grFast_ + grSlow_);
  } else {
    grFast_ += (target > grFast_ ? stAtk_ : stRel_) * (target - grFast_);
    gr = grFast_;
  }
  return y * std::exp((p_.stMakeup - gr) * (std::log(10.0) / 20));
}

GuitarAmp::~GuitarAmp() {
  if (!peaks_) return;
  std::fprintf(stderr, "amp %s peaks:", p_.name);
  for (int i = 0; i < p_.stages; ++i) std::fprintf(stderr, " stage %d %.2f", i + 1, peak_[i]);
  std::fprintf(stderr, " power %.2f (of the knees)\n", peak_[kMaxStages]);
}

void GuitarAmp::connect(const Source& s, double cable) {
  const double C = s.C + cable, Rl = 1 / (1 / s.pots + 1 / p_.inputR);
  const double A = s.L * C, B = s.L / Rl + s.R * C, D = 1 + s.R / Rl;
  const double w0 = 1 / std::sqrt(A), c = w0 / std::tan(std::min(w0 / (2 * fs_), 1.5));
  // (1 + z)^2 -> 1, 2, 1; (1 - z)(1 + z) -> 1, 0, -1; (1 - z)^2 -> 1, -2, 1.
  const double a0 = A * c * c + B * c + D, a1 = -2 * A * c * c + 2 * D, a2 = A * c * c - B * c + D;
  input_.b0 = 1 / a0;
  input_.b1 = 2 / a0;
  input_.b2 = 1 / a0;
  input_.a1 = a1 / a0;
  input_.a2 = a2 / a0;
  tone_ = s.Ct > 0;
  if (!tone_) return;
  // With the tone control (research/jazz_guitar.md 4.3): the EMF through R + sL into the node,
  // and at the node C (coil and cable), G (the pots and the amp) and the tone branch Rt + 1/(s Ct):
  //   V / E = (1 + s tau) / (D0 + D1 s + D2 s^2 + D3 s^3), tau = Rt Ct,
  //   D0 = 1 + R G, D1 = tau + R (C + G tau + Ct) + L G, D2 = R C tau + L (C + G tau + Ct), D3 = L C tau.
  // The bilinear transform, warped to agree at the coil's resonance (as the biquad above).
  const double G = 1 / Rl, tau = s.Rt * s.Ct;
  const double Dn[4] = {1 + s.R * G, tau + s.R * (C + G * tau + s.Ct) + s.L * G, s.R * C * tau + s.L * (C + G * tau + s.Ct), s.L * C * tau};
  const double N[4] = {1, tau, 0, 0};
  // s^k -> c^k (1 - z)^k (1 + z)^(3 - k).
  static const double kE[4][4] = {{1, 3, 3, 1}, {1, 1, -1, -1}, {1, -1, -1, 1}, {1, -3, 3, -1}};
  double Bz[4] = {}, Az[4] = {}, ck = 1;
  for (int k = 0; k < 4; ++k, ck *= c)
    for (int i = 0; i < 4; ++i) { Bz[i] += N[k] * ck * kE[k][i]; Az[i] += Dn[k] * ck * kE[k][i]; }
  for (int i = 0; i < 4; ++i) { ib_[i] = Bz[i] / Az[0]; ia_[i] = Az[i] / Az[0]; }
}

double GuitarAmp::input3(double x) {
  const double y = ib_[0] * x + iz_[0];
  iz_[0] = ib_[1] * x - ia_[1] * y + iz_[1];
  iz_[1] = ib_[2] * x - ia_[2] * y + iz_[2];
  iz_[2] = ib_[3] * x - ia_[3] * y;
  return y;
}

// The speaking strip (guitaramp.h; research/jazz_guitar.md 12.3). The presence band's level
// (quick up, 80 ms down) sets its lift: presDb, and speak dB more for each dB the band stands over
// presRef (less under it), within presLo .. presHi; smoothed over 2 ms. The compressor hears the
// signal before the lift (rms over two 10 ms poles), a soft knee, and takes the level down over
// scAttack (10 ms: the pick's first transient passes) and gives it back over scRelease.
double GuitarAmp::strip(double x) {
  const double b = pres_.run(x);
  const double a = std::fabs(b);
  presEnv_ += (a > presEnv_ ? presAtk_ : presRel_) * (a - presEnv_);
  const double want = std::clamp(p_.presDb + p_.speak * (20 * std::log10(presEnv_ + 1e-12) - p_.presRef), p_.presLo, p_.presHi);
  presGain_ += presSmooth_ * (want - presGain_);
  double y = x + (std::pow(10.0, presGain_ / 20) - 1) * b;
  if (p_.scRatio > 1) {
    scMs1_ += msA_ * (x * x - scMs1_);
    scMs2_ += msA_ * (scMs1_ - scMs2_);
    const double over = 10 * std::log10(scMs2_ + 1e-30) - p_.scAt, r = 1 - 1 / p_.scRatio, h = 0.5 * p_.scKnee;
    const double target = over <= -h ? 0 : over >= h ? r * over : r * (over + h) * (over + h) / (4 * h);
    scGr_ += (target > scGr_ ? scAtk_ : scRel_) * (target - scGr_);
    y *= std::pow(10.0, -scGr_ / 20);
  }
  return y;
}
void GuitarAmp::setTone(double treble, double mid, double bass) {
  if (p_.stackType == 1) {
    james(treble, bass);
    align();
    return;
  }
  const double t = std::clamp(treble, 0.0, 1.0), m = std::clamp(mid, 0.0, 1.0);
  const double l = std::exp((std::clamp(bass, 0.0, 1.0) - 1) * 3.4);  // (log taper)
  const double C1 = p_.c1, C2 = p_.c2, C3 = p_.c3, R1 = p_.r1, R2 = p_.r2, R3 = p_.r3, R4 = p_.r4;
  const double b1 = t * C1 * R1 + m * C3 * R3 + l * (C1 * R2 + C2 * R2) + (C1 * R3 + C2 * R3);
  const double b2 = t * (C1 * C2 * R1 * R4 + C1 * C3 * R1 * R4) - m * m * (C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3) +
                    m * (C1 * C3 * R1 * R3 + C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3) +
                    l * (C1 * C2 * R1 * R2 + C1 * C2 * R2 * R4 + C1 * C3 * R2 * R4) + l * m * (C1 * C3 * R2 * R3 + C2 * C3 * R2 * R3) +
                    (C1 * C2 * R1 * R3 + C1 * C2 * R3 * R4 + C1 * C3 * R3 * R4);
  const double C123 = C1 * C2 * C3;
  const double b3 = l * m * (C123 * R1 * R2 * R3 + C123 * R2 * R3 * R4) - m * m * (C123 * R1 * R3 * R3 + C123 * R3 * R3 * R4) +
                    m * (C123 * R1 * R3 * R3 + C123 * R3 * R3 * R4) + t * C123 * R1 * R3 * R4 - t * m * C123 * R1 * R3 * R4 +
                    t * l * C123 * R1 * R2 * R4;
  const double a1 = (C1 * R1 + C1 * R3 + C2 * R3 + C2 * R4 + C3 * R4) + m * C3 * R3 + l * (C1 * R2 + C2 * R2);
  const double a2 = m * (C1 * C3 * R1 * R3 - C2 * C3 * R3 * R4 + C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3) +
                    l * m * (C1 * C3 * R2 * R3 + C2 * C3 * R2 * R3) - m * m * (C1 * C3 * R3 * R3 + C2 * C3 * R3 * R3) +
                    l * (C1 * C2 * R2 * R4 + C1 * C2 * R1 * R2 + C1 * C3 * R2 * R4 + C2 * C3 * R2 * R4) +
                    (C1 * C2 * R1 * R4 + C1 * C3 * R1 * R4 + C1 * C2 * R3 * R4 + C1 * C2 * R1 * R3 + C1 * C3 * R3 * R4 + C2 * C3 * R3 * R4);
  const double a3 = l * m * (C123 * R1 * R2 * R3 + C123 * R2 * R3 * R4) - m * m * (C123 * R1 * R3 * R3 + C123 * R3 * R3 * R4) +
                    m * (C123 * R3 * R3 * R4 + C123 * R1 * R3 * R3 - C123 * R1 * R3 * R4) + l * C123 * R1 * R2 * R4 + C123 * R1 * R3 * R4;
  const double c = 2 * fsOs_;  // (it runs with the stages)
  // s^k -> c^k (1 - z)^k (1 + z)^(3 - k).
  static const double kE[4][4] = {{1, 3, 3, 1}, {1, 1, -1, -1}, {1, -1, -1, 1}, {1, -3, 3, -1}};
  const double num[4] = {0, b1 * c, b2 * c * c, b3 * c * c * c}, den[4] = {1, a1 * c, a2 * c * c, a3 * c * c * c};
  double B[4] = {}, A[4] = {};
  for (int k = 0; k < 4; ++k)
    for (int i = 0; i < 4; ++i) { B[i] += num[k] * kE[k][i]; A[i] += den[k] * kE[k][i]; }
  for (int i = 0; i < 4; ++i) { tb_[i] = B[i] / A[0]; ta_[i] = A[i] / A[0]; }
}

// A triode stage's swing at its grid (V, from its bias): nearly straight until the grid
// conducts at about +1.4 V (a firm knee), the tube cutting off below about -2.8 V (a softer
// one); its curve a little steeper on the way up (an even harmonic, ~-40 dB at a 0.4 V swing).
double GuitarAmp::triode(double x) {
  double y;
  if (x >= 0) {
    const double q = x / 1.4, q2 = q * q;
    y = x / std::sqrt(std::sqrt(1 + q2 * q2));
  } else {
    const double q = x / 2.8;
    y = x / std::sqrt(1 + q * q);
  }
  return y + 0.04 * y * y;
}

template <bool kPeaks>
double GuitarAmp::stage(const Stage& s, Tube& t, double x) {
  // In front of the grid: a divider's shelf; the cathode's (its partial bypass: the stage
  // amplifies the bass less, as if it came in weaker).
  if (t.inOn) x = t.in.run(x);
  if (t.shelfOn) x = t.shelf.run(x);
  // The grid: where the signal puts it against the stage's bias, less what grid current has
  // charged the coupling capacitor with.
  const double u = x + s.bias - t.shift;
  if (linear_ == 1) {
    // (Straight: its slope at its bias.)
    double y = t.hp.run(s.gain * slope_[&t - t_] * (u - s.bias));
    if (t.lpOn) y = t.lp.run(y);
    return s.divider != 1 ? y * s.divider : y;
  }
  const double sw = s.swing;
  if constexpr (kPeaks) {
    double& m = peak_[&t - t_];
    m = std::max(m, u > s.bias ? (u - s.bias) / (1.4 * sw) : (s.bias - u) / (2.8 * sw));
  }
  if (u > sw) t.shift += (s.grid == 1 ? gridA_ : gridA_ * s.grid) * (u - sw);
  t.shift -= gridRel_ * t.shift;
  double c;
  if (sw != 1) {
    // (A wider swing: the same curve, stretched.)
    const double v = u / sw;
    if (p_.adaa) {
      const double F = triodeF(v), d = v - t.u1;
      c = std::fabs(d) > 1e-6 ? (F - t.F1) / d : triodeLin(0.5 * (v + t.u1));
      t.u1 = v;
      t.F1 = F;
    } else {
      c = triode(v);
    }
    c = sw * (c - t.rest);
  } else {
    if (p_.adaa) {
      // The curve averaged over the sample (its integral's difference: PZL16), the midpoint's
      // value where the grid hardly moved.
      const double F = triodeF(u), d = u - t.u1;
      c = std::fabs(d) > 1e-6 ? (F - t.F1) / d : triodeLin(0.5 * (u + t.u1));
      t.u1 = u;
      t.F1 = F;
    } else {
      c = triode(u);
    }
    c -= t.rest;
  }
  // A cathode follower after it: when its grid conducts, the stage's cut-off swing is squashed.
  if (s.follower > 0 && c < 0) c = c / std::sqrt(1 + (c * c) / (s.follower * s.follower));
  double y = t.hp.run(s.gain * c);
  if (t.lpOn) y = t.lp.run(y);
  return s.divider != 1 ? y * s.divider : y;
}

double GuitarAmp::stack(double x) {
  if (p_.stackType == 1) return jb_[1].run(jb_[0].run(x));
  const double y = tb_[0] * x + tz_[0];
  tz_[0] = tb_[1] * x - ta_[1] * y + tz_[1];
  tz_[1] = tb_[2] * x - ta_[2] * y + tz_[2];
  tz_[2] = tb_[3] * x - ta_[3] * y;
  return y;
}

// The Tube Screamer: the clean signal, and added to it its part above 720 Hz, amplified and
// clipped by the two diodes in the op amp's feedback (a firm knee at ~0.6 V); its tone; its level.
double GuitarAmp::screamer(double x) {
  x = tsIn_.run(x);
  double v = tsCap_.run(tsGain_ * tsLeg_.run(x));
  const double q = v * (1 / 0.6), q2 = q * q;
  v /= std::sqrt(std::sqrt(1 + q2 * q2));
  return tsTone_.run(x + v) * p_.tsLevel;
}

template <bool kPeaks>
double GuitarAmp::chain(double x) {
  if (p_.tsLevel > 0) x = screamer(x);
  if (p_.stackAfter == 0) x = stack(x);
  for (int i = 0; i < p_.stages; ++i) {
    x = stage<kPeaks>(p_.stage[i], t_[i], x);
    if (p_.stackAfter == i + 1) x = stack(x);
    if (i == 0) x = pot_.run(x);
  }
  for (int i = 0; i < p_.voices; ++i) x = voice_[i].run(x);
  if (p_.gateAt > 0) x *= gateGain_;
  x *= p_.master;
  if (linear_) return hpOut_.run(x);
  // The power stage: push-pull, the supply giving way under a loud passage (the guitars' to 85 %
  // of its swing).
  sag_ += sagA_ * (std::fabs(x) - sag_);
  const double lim = p_.clip * (1 - p_.sagDepth * std::min(1.0, sag_ / p_.clip));
  const double a = std::fabs(x) / lim;
  if constexpr (kPeaks) peak_[kMaxStages] = std::max(peak_[kMaxStages], a);
  return hpOut_.run(p_.knee == 2 ? x / std::sqrt(1 + a * a) : x / std::cbrt(1 + a * a * a));
}

// The gate's detector, on the guitar's own signal (before the amp: the noise the preamp makes
// does not hold it open): open above the threshold; closing once it has been 6 dB under for
// its hold (between the two it stays as it is: no chatter on a decaying note); fading.
void GuitarAmp::gate(double x) {
  const double a = std::fabs(x);
  gateEnv_ += (a > gateEnv_ ? gateAtk_ : gateRel_) * (a - gateEnv_);
  gatePeak_ = std::max(gateEnv_, gatePeak_ * gatePeakFall_);
  const bool was = gateOpen_;
  if (!gateOpen_) {
    // (After a quick close only a new attack opens it: 6 dB over where it closed, that bar
    // falling back to the threshold in 50 ms.)
    if (gateEnv_ > std::max(p_.gateAt, gateReopen_)) {
      gateOpen_ = true;
      gateHold_ = p_.gateHold;
    }
    gateReopen_ *= gateReopenFall_;
  } else if (gateEnv_ < 0.5 * p_.gateAt || gateEnv_ < gatePeak_ * gateDropTo_) {
    // (A quick fall: a third of the hold, and the fade four times as fast.)
    gateQuick_ = gateEnv_ >= 0.5 * p_.gateAt;
    if ((gateHold_ -= (gateQuick_ ? 3 : 1) / fs_) <= 0) {
      gateOpen_ = false;
      gateReopen_ = 2 * gateEnv_;
    }
  } else {
    gateHold_ = p_.gateHold;
  }
  ++gateT_;
  if (gateDebug_ && was != gateOpen_) std::fprintf(stderr, "gate %s %.3f s\n", gateOpen_ ? "open" : "close", gateT_ / fs_);
  if (gateOpen_ ? gateDb_ < 0 : gateDb_ > -80) {
    gateDb_ = gateOpen_ ? std::min(0.0, gateDb_ + gateRise_) : std::max(-80.0, gateDb_ - (gateQuick_ ? 4 : 1) * gateFall_);
    gateGain_ = std::pow(10.0, gateDb_ / 20);
  }
}

void GuitarAmp::reset() {
  input_.z1 = input_.z2 = 0;
  for (Biquad& b : cab_) b.z1 = b.z2 = 0;
  for (Biquad& b : post_) b.z1 = b.z2 = 0;
  for (int i = 0; i < 4; ++i) up_[i].z1 = up_[i].z2 = down_[i].z1 = down_[i].z2 = 0;
  for (int i = 0; i < kMaxStages; ++i) {
    Tube& t = t_[i];
    t.hp.z = t.lp.z = t.shelf.z = t.in.z = 0;
    t.shift = 0;
    t.u1 = p_.stage[i].swing == 1 ? p_.stage[i].bias : p_.stage[i].bias / p_.stage[i].swing;
    t.F1 = p_.adaa ? triodeF(t.u1) : 0;
  }
  pot_.z = hpOut_.z = 0;
  for (Biquad& b : jb_) b.z1 = b.z2 = 0;
  for (Biquad& b : voice_) b.z1 = b.z2 = 0;
  for (Biquad& b : di_) b.z1 = b.z2 = 0;
  diAp_.z1 = diAp_.z2 = drHp_.z1 = drHp_.z2 = drLp_.z1 = drLp_.z2 = 0;
  std::fill(delay_.begin(), delay_.end(), 0.0);
  ms1_ = ms2_ = grFast_ = grSlow_ = 0;
  tsIn_.z = tsLeg_.z = tsCap_.z = tsTone_.z = 0;
  std::fill(tz_, tz_ + 3, 0.0);
  sag_ = compEnv_ = gateEnv_ = gatePeak_ = gateReopen_ = 0;
  gateT_ = 0;
  gateOpen_ = gateQuick_ = false;
  gateDb_ = p_.gateAt > 0 ? -80 : 0;
  gateGain_ = p_.gateAt > 0 ? 1e-4 : 1;
  std::fill(iz_, iz_ + 3, 0.0);
  pres_.z1 = pres_.z2 = 0;
  presEnv_ = scMs1_ = scMs2_ = scGr_ = 0;
  presGain_ = p_.presDb;
}

float GuitarAmp::process(float emf) {
  double e = emf;
  if (p_.hiss > 0) {
    // The pickup's winding and the grid stopper: white, p_.hiss V rms (a uniform's rms is 1/sqrt 3).
    noise_ ^= noise_ << 13; noise_ ^= noise_ >> 17; noise_ ^= noise_ << 5;
    e += p_.hiss * 1.7320508 * (noise_ * (2.0 / 4294967296.0) - 1);
  }
  if (!humWave_.empty()) {
    e += p_.hum * humWave_[humAt_];
    if (++humAt_ == humWave_.size()) humAt_ = 0;
  }
  double x = tone_ ? input3(e) : input_.run(e);
  if (p_.gateAt > 0) gate(x);
  if (p_.compRatio > 1) {
    // The compressor: its gain from the level it has followed (quick up, slow down).
    const double a = std::fabs(x);
    compEnv_ += (a > compEnv_ ? compAtk_ : compRel_) * (a - compEnv_);
    const double over = compEnv_ / p_.compAt;
    x *= (over > 1 ? std::pow(over, 1 / p_.compRatio - 1) : 1.0) * compMakeup_;
  }
  // Bass: the DI taken here (after a stomp box, if any); the amp's path skipped if the DI is all.
  const double d = x;
  if (bass_ && p_.blend >= 1) return float(compress(di(d)));
  if (p_.oversample <= 1) {
    x = peaks_ ? chain<true>(x) : chain<false>(x);
  } else {
    // Up (a sample and three zeros, through the low-pass), the stages, and down again.
    const int n = p_.oversample;
    double out = 0;
    for (int i = 0; i < n; ++i) {
      double v = i == 0 ? x * n : 0.0;
      for (Biquad& b : up_) v = b.run(v);
      v = peaks_ ? chain<true>(v) : chain<false>(v);
      for (Biquad& b : down_) v = b.run(v);
      if (i == n - 1) out = v;
    }
    x = out;
  }
  // The power stage's resonance and presence; the speaker.
  for (int i = 0; i < nPost_; ++i) x = post_[i].run(x);
  for (int i = 0; i < nCab_; ++i) x = cab_[i].run(x);
  if (strip_ && !bass_) return float(strip(x / p_.clip));
  if (!bass_) return float(x / p_.clip);
  // The mic's path level-matched to the DI (and delayed by its distance), the blend, the
  // compressor.
  x *= ampNorm_;
  if (!delay_.empty()) {
    delay_[delayAt_] = x;
    if (++delayAt_ == delay_.size()) delayAt_ = 0;
    x = delay_[delayAt_];
  }
  return float(compress(p_.blend * di(d) + (1 - p_.blend) * x));
}

}  // namespace pajupilli
