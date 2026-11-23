#include "organ.h"
#include "heard.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <sys/stat.h>
#include <unistd.h>

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <thread>
#include <utility>

#include "knob.h"
#include "organplan.h"
#include "rng.h"

namespace pajupilli {

bool organRank(const Instrument& inst) {
  for (const char* n : {"gedackt-8", "rohrgedackt-8", "gedackt-rohrgedackt", "principal-8", "organ", "organ-crescendo",
                        "organ-88", "organ-plan"})
    if (std::strcmp(inst.name, n) == 0) return true;
  return false;
}

namespace {

// Gedackt 8': a stopped wooden pipe, its voicing by pitch (scanned:
// research/church_organ.md §8, step 2). The bass pipes lose more (a lower
// Q) and their jets are voiced bolder, so 8' C speaks in ~150 ms, middle C
// in ~90, the top in ~20; the twelfth (the 3rd harmonic) ~-18 dB, the even
// ones ~-45.
PipeVoicing gedackt(double hz) {
  PipeVoicing v;
  v.stopped = true;
  v.q1 = std::clamp(20 * std::pow(hz / 65.4, 0.3), 15.0, 35.0);
  v.gain = std::clamp(16 * std::pow(262 / hz, 0.3), 16.0, 24.0);
  v.modeTilt = knob("PJ_GEDACKT_TILT", 1.75);
  // The upper harmonics fall steeply above the twelfth, as the GrandOrgue
  // demo's Bourdon 8' (recorded pipes F#/G of each octave) shows: the 5th
  // ~-35..-60 dB, the 7th ~-50..-75, the 9th ~-60..-85 against the
  // fundamental (without it ours were 10-50 dB too strong there: buzzy).
  v.rollK = knob("PJ_GEDACKT_ROLL", 4.0);
  v.jetRad = knob("PJ_GEDACKT_JETRAD", 0.3);
  v.jetLp = knob("PJ_GEDACKT_JETLP", 2.0);
  v.breath = knob("PJ_GEDACKT_BREATH", 0.04);
  // The puff at each note's start (the recorded Bourdon: the noise between
  // its harmonics 20-30 dB above its steady breath for ~100-200 ms in the
  // bass, less and shorter up high).
  v.puff = knob("PJ_GEDACKT_PUFF", 0.25) * std::max(1.0, std::sqrt(262 / hz));
  v.puffTime = std::clamp(0.1 * std::pow(262 / hz, 0.4), 0.04, 0.2);
  // A big pipe's foot and its bore fill slowly: the bass speaks gently, its
  // wind rising over ~20 ms at 8' C (the recorded Bourdon's bass ~100-260 ms
  // to speak), the treble promptly.
  v.footRise = 0.006 * std::clamp(std::pow(262 / hz, knob("PJ_GEDACKT_FOOTEXP", 1.0)), 1.0, 6.0);
  // Its chiff: the twelfth leading at the attack (the recorded Bourdon's ~2-6
  // dB over its steady level, less than an open pipe's octave).
  v.chiff = knob("PJ_GEDACKT_CHIFF", 4.0) * std::clamp(std::sqrt(hz / 262), 1.0, 3.0);  // (the small pipes' chiff the most)
  v.chiffTime = std::clamp(0.03 * std::pow(262 / hz, 0.3), 0.025, 0.06);  // (at least as long as a small pipe takes to speak)
  // Below C5 more wind (a wider mouth, more flow: the voicer's bass): the
  // jet's drive ~ (523 Hz / f)^PJ_GEDACKT_AIR, so the bass is as full as the
  // treble (its mouth radiates its flow's rate of change: ~6 dB an octave
  // weaker for the same flow) and, the jet swinging harder, richer; its upper
  // modes driven a little more (the tilt eased towards 1.3 at 8' C). C5 and
  // above as they were.
  static const double kAir = knob("PJ_GEDACKT_AIR", 0.95), kBassTilt = knob("PJ_GEDACKT_BASSTILT", 1.3);
  if (hz < 523.25) {
    const double o = std::log2(523.25 / hz);  // octaves below C5
    v.drive = std::pow(523.25 / hz, kAir);
    v.modeTilt += (kBassTilt - v.modeTilt) * std::min(1.0, o / 3);
  } else {
    // Above C5 narrower mouths, less flow: the treble no louder than C5
    // (it was ~6 dB an octave louder: shrill, "stressed"), as the recorded
    // Bourdon (level, if anything, falling a little towards the top).
    static const double kTreble = knob("PJ_GEDACKT_TREBLE", 1.0);
    v.drive = std::pow(523.25 / hz, kTreble);
  }
  return v;
}

// The pipe's speaking pitch after a short run (its steady part), sounding
// alone in its place: its enclosure's field acting on its jet as it will
// when played (a tuner tunes a pipe where it stands). The zero crossings'
// rate of its sound low-passed to the fundamental (two one-poles at 1.5 x its
// pitch: a low pipe's waveform, with its noise, can wander across zero twice
// a period; the fundamental alone crosses once).
template <typename P>
double speakingHz(P p, Enclosure e, int src, int srcTop, float hear, double fs, int os = 1, float gain = 1) {
  p.reset();
  e.reset();
  p.open();
  // (at least 25 periods to settle, 12 to count: a 32' pipe speaks slowly)
  const double per = fs / (p.hz() * p.ratio());
  // (counted over 0.8 s: the unisons of a tutti beat by their residual
  // errors, so a tenth of a cent matters)
  const int warm = int(std::max(0.5 * fs, 25 * per)), len = int(std::max(0.8 * fs, 12 * per));
  // (the fundamental alone: two band-passes at the pitch, Q 4, the octave
  // ~35 dB down: a reed's or a string's harmonics up to 20 dB over its
  // fundamental would cross zero too)
  const double w0 = 2 * M_PI * p.hz() * p.ratio() / fs, al = std::sin(w0) / 8;
  // (in double: at 30 Hz the poles stand within 1e-3 of the unit circle)
  const double bb0 = al / (1 + al), ba1 = 2 * std::cos(w0) / (1 + al), ba2 = -(1 - al) / (1 + al);
  double x1 = 0, x2 = 0, m1 = 0, m2 = 0, p1 = 0, p2 = 0, q1 = 0, q2 = 0, prev = 0;
  double first = -1, last = 0;
  int count = 0;
  for (int n = 0; n < warm + len; ++n) {
    const float heard = hear * e.heard(src);
    float y = 0, top = 0;
    for (int k = 0; k < os; ++k) {
      y += p.sample(1.0f, heard);
      top += p.top();
    }
    y /= float(os);
    top /= float(os);
    e.add(src, gain * y);  // (into its enclosure as loud as when played: its regulation)
    if (srcTop >= 0) e.add(srcTop, gain * top);
    e.step(0);
    const double l1 = bb0 * (y - x2) + ba1 * m1 + ba2 * m2;
    x2 = x1;
    x1 = y;
    m2 = m1;
    m1 = l1;
    const double l2 = bb0 * (l1 - p2) + ba1 * q1 + ba2 * q2;
    p2 = p1;
    p1 = l1;
    q2 = q1;
    q1 = l2;
    if (n > warm && prev < 0 && l2 >= 0) {
      const double t = n - 1 + prev / (prev - l2);
      if (first < 0) first = t;
      last = t;
      ++count;
    }
    prev = l2;
  }
  return count > 2 ? (count - 1) * fs / (last - first) : 0;
}

// Rohrgedackt 8': the Positiv's chimney flute, metal, a smaller scale than
// the Gedackt: the chimney half-opens it (the even harmonics ~-50 dB in the
// bass .. -30 at the top) and lifts the 4th-6th (the "liquid" tone),
// brighter, a little less breath.
PipeVoicing rohrgedackt(double hz) {
  PipeVoicing v = gedackt(hz);
  v.evenLevel = knob("PJ_ROHR_EVEN", 2.5);
  v.chimneyK = knob("PJ_ROHR_CHIMNEYK", 5.0);
  v.chimneyBoost = knob("PJ_ROHR_BOOST", 2.5);
  v.chimneyWidth = 1.3;
  v.rollK = knob("PJ_ROHR_ROLL", 5.0);
  v.breath *= 0.7;
  v.puff *= 0.8;
  return v;
}

// Principal 8': the Hauptwerk's open metal pipe at normal scale, the
// organ's own voice. Voiced against a recorded Montre 8' (GrandOrgue's demo
// set): the octave nearly as strong as the fundamental in the bass (-8 ..
// -19 dB above), the 3rd and 4th ~-10 .. -30, harmonics to the 12th
// (~-35 .. -60); speaking ~250 ms at 47 Hz, ~120 at 93, ~40 at 370, a few
// ms at the top, the octave first from the middle up (overshooting ~3-9 dB:
// the jet, slow while the foot fills, fits the octave's period first).
PipeVoicing principal(double hz) {
  PipeVoicing v;
  v.stopped = false;
  v.maxModes = 16;
  // (the treble's mouths relatively larger, losing more a cycle: a lower Q,
  // quick speech; the bass's higher, slower)
  v.q1 = std::clamp(knob("PJ_PRIN_Q", 30.0) * std::pow(262 / hz, knob("PJ_PRIN_QEXP", 0.3)), 12.0, 45.0);
  v.qRise = 0.5;
  v.gain = knob("PJ_PRIN_GAIN", 16.0) * std::clamp(std::pow(hz / 262, knob("PJ_PRIN_GEXP", 0.3)), 0.6, 1.6);
  v.bias = knob("PJ_PRIN_BIAS", 0.3);
  v.harmFeed = knob("PJ_PRIN_HFEED", 0.2);
  v.harmLead = knob("PJ_PRIN_LEAD", 0.0);
  v.chiff = knob("PJ_PRIN_CHIFF", 8.0);
  v.chiffTime = std::clamp(0.03 * std::pow(262 / hz, 0.3), 0.025, 0.06);  // (at least as long as a small pipe takes to speak)  // (tried: the foot fills too fast for it to tell; an open question)
  v.driveNorm = knob("PJ_PRIN_DNORM", 0.0);
  v.biasFrac = knob("PJ_PRIN_BFRAC", 0.4);
  v.evenTilt = knob("PJ_PRIN_EVENTILT", 1.5);
  // The octave: strong in the bass and middle (~0 dB), less at the top
  // (~-12): the even modes driven ~ 8x, falling above C5.
  v.openEven = knob("PJ_PRIN_OCT", 1.0) * std::min(1.0, std::pow(523.25 / hz, knob("PJ_PRIN_OCTFALL", 0.8)));  // (the languid's offset: the even harmonics)
  v.modeTilt = knob("PJ_PRIN_TILT", 0.8);
  v.rollK = knob("PJ_PRIN_ROLL", 9.0);
  v.jetRad = knob("PJ_PRIN_JETRAD", 0.3);
  v.jetLp = 3.0;
  v.breath = knob("PJ_PRIN_BREATH", 0.03);
  v.puff = knob("PJ_PRIN_PUFF", 0.2) * std::max(1.0, std::sqrt(262 / hz));
  v.puffTime = std::clamp(0.08 * std::pow(262 / hz, 0.5), 0.02, 0.2);
  v.footRise = knob("PJ_PRIN_FOOT", 0.012) * std::clamp(std::pow(262 / hz, 1.0), 0.2, 4.0);
  static const double kAir = knob("PJ_PRIN_AIR", 0.95);
  v.drive = hz < 523.25 ? std::pow(523.25 / hz, kAir) : std::pow(523.25 / hz, knob("PJ_PRIN_TREBLE", 1.0));
  return v;
}

// An open flute (Waldflöte, Nasard, Terz, Flûte octaviante; the Sesquialtera's
// ranks): a wide scale, a low cut-up: the Principal's pipe with a rounder tone,
// the octave ~-20 dB, the upper harmonics falling fast, more breath.
PipeVoicing openFlute(double hz) {
  PipeVoicing v = principal(hz);
  v.openEven *= knob("PJ_FLUTE_OCT", 0.35);
  v.modeTilt = knob("PJ_FLUTE_TILT", 1.4);
  v.rollK = knob("PJ_FLUTE_ROLL", 4.0);
  v.biasFrac = 0.3;
  if (hz > 1000) v.openEven *= 1000 / hz;  // (a small flute's octave weaker still: the recorded Octavin's ~-30 dB)
  v.chiff *= std::clamp(std::sqrt(hz / 262), 1.0, 3.0);  // (its chiff stronger in the small pipes: the Octavin's 5-11 dB)
  v.breath *= 1.5;
  v.puff *= 1.2;
  return v;
}

// The Flûte harmonique: from G3 up its pipes twice as long, a small hole at
// the middle making them overblow to the octave (Cavaillé-Coll's): its
// resonances then at the speaking pitch's harmonics, a longer pipe's higher
// Q, a full, round, singing tone (the octave weak, the fundamental strong);
// below G3 an ordinary open flute.
PipeVoicing harmonicFlute(double hz) {
  PipeVoicing v = openFlute(hz);
  if (hz >= 196) {
    v.q1 *= knob("PJ_HARM_Q", 1.6);
    v.openEven *= 0.6;
    v.modeTilt += 0.3;
    v.breath *= 1.2;
  }
  return v;
}

// A string (Viola di Gamba, Voix céleste): a narrow scale, its small mouth
// and diameter radiating little, so its upper modes keep their Q and the jet
// drives them nearly as hard as the fundamental. Voiced against a recorded
// Gamba 8' (GrandOrgue's demo set): in the bass harmonics 2-5 as strong as
// the fundamental or stronger, ~-20 dB to the 12th, -35 .. -55 to the 18th;
// thinner up the compass (at 370 Hz the 2nd and 3rd ~-7, at 1.5 kHz ~-12).
PipeVoicing gamba(double hz) {
  PipeVoicing v = principal(hz);
  v.maxModes = 24;
  const double o = std::log2(std::max(hz, 65.4) / 65.4);  // octaves over 8' C
  v.modeTilt = knob("PJ_GAMBA_TILT", -0.8) + knob("PJ_GAMBA_TILTRISE", 0.35) * o;
  v.rollK = std::max(4.0, knob("PJ_GAMBA_ROLL", 16.0) - 2.5 * o);
  v.q1 *= knob("PJ_GAMBA_Q", 1.0) * std::clamp(std::pow(hz / 92, 0.25), 1.0, 2.0);  // (a narrow pipe's Q, most in the treble: its slower speech there, ~30-60 ms)
  v.qRise = 0.3;
  v.qCap = 200;
  v.openEven = knob("PJ_GAMBA_OCT", 1.0);
  v.evenTilt = 0;
  v.biasFrac = knob("PJ_GAMBA_BFRAC", 0.25);  // (a centred jet: the duty near 1/2, the 3rd kept)
  // The bass octave bridged (a roller across the mouth, as on real string
  // basses: by itself the narrow bass pipe hops between regimes): its
  // voicing towards the Principal's at 8' C, the full string's from F#3.
  const double sb = std::clamp(std::log2(hz / 65.4) / knob("PJ_GAMBA_BRIDGE", 1.5), 0.0, 1.0);
  v.modeTilt = 0.4 * (1 - sb) + v.modeTilt * sb;
  v.biasFrac = 0.4 * (1 - sb) + v.biasFrac * sb;
  v.evenTilt = 1.5 * (1 - sb);
  v.breath *= 0.8;
  // The frein harmonique (a cross-bar before the mouth, from F#2 in, full
  // from C3; research/church_organ.md §8a). Without it the narrow middle
  // spoke late and unsure (the pipe alone: C3 to 90 % in ~200 ms, middle C
  // ~120): the octave led ~40 ms with the fundamental stalled ~40 dB down,
  // ~+100 c sharp, then it crept up at its narrow pipe's Q (~40: a ring-up
  // q1 / (pi f) ~100 ms at C3). The bar's loss (the mouth's flow squeezed
  // past it) adds ~5 Hz to every mode's bandwidth: the fundamental's Q ~40
  // -> ~16 at C3, ~23 at middle C, the treble's little changed; it rings up
  // in ~1 / (pi (f / q1 + 5)), ~40 ms at C3. It brakes the octave's
  // precursor (the chiff a quarter). With it the string takes its low
  // cut-up (the jet's travel 0.22 of the period, not 0.25: less phase lost
  // while the foot fills, a shallower scoop into the pitch), its toe open
  // (the wind regulated at the flue: the foot fills in half the time, the
  // level the lower cut-up loses back, ~+0.8 dB).
  const double fw = std::clamp(2 * std::log2(hz / 92.5), 0.0, 1.0);
  v.freinHz = knob("PJ_GAMBA_FREIN", 5.0) * fw;
  v.chiff *= 1 - (1 - knob("PJ_GAMBA_FREINCHIFF", 0.25)) * fw;
  v.tauFrac -= knob("PJ_GAMBA_CUTUP", 0.03) * fw;
  v.drive *= 1 + knob("PJ_GAMBA_FREINTOE", 0.10) * std::clamp(std::pow(262 / hz, 0.45), 0.53, 1.0) * fw;
  v.footRise *= 1 - (1 - knob("PJ_GAMBA_FOOT", 0.5)) * fw;
  return v;
}

}  // namespace

// The divisions' enclosures, each a C-side half (left) and a C#-side half
// (right): the swell boxes, the Rückpositiv's case at the rail, the main
// case (the Hauptwerk's chest), its façade, the Pedal's towers.
enum { kSwellC = 0, kSwellCis = 1, kPosC = 2, kPosCis = 3, kHwC = 4, kHwCis = 5, kFacC = 6, kFacCis = 7,
       kPedC = 8, kPedCis = 9, kEnclosures = 10 };
enum Division { kHW, kSW, kPos, kPed };
enum Family { kStoppedWood, kChimney, kOpenPrincipal, kOpenFlute, kString, kHarmonicFlute, kTrumpet, kOboe, kCromorne, kFagott };
namespace {

bool isReed(Family f) { return f >= kTrumpet; }

// The reeds (src/organreed.h): a conical resonator's every harmonic, the
// trumpet's bright and strong (Trompete, Trompette, Clairon, Posaune), the
// oboe's narrower and nasal, the bassoon's (Fagott) thin and dark; the
// Cromorne's half-length cylinder its odd harmonics, a clarinet's hollow.
ReedVoicing reedVoicing(Family f, double hz) {
  ReedVoicing v;
  // (the bass near its tongue's closing: a quick, blaring speech, ~50 ms at 185 Hz; the treble lower: near
  // closing, its wind falling at the key's release passed through a louder swing, a blat)
  // (the bass nearer still, 0.95 below 100 Hz: at 0.9 a 16' reed's heavy tongue grew from rest over 150 ..
  // 300 ms, a short pedal note over before it spoke; at 0.95 within ~60 .. 100 ms, as a Posaune's does.
  // Its smaller gap passes less wind, made up in its level; 0.97 and more choked it on a long note)
  const double p0 = knob("PJ_REED_P", 0.9) - 0.2 * std::clamp(std::log2(hz / 300), 0.0, 1.0);
  v.pressure = p0 + knob("PJ_REED_BASSP", 0.05) * std::clamp(std::log2(200 / hz), 0.0, 1.0);
  v.zeta = knob("PJ_REED_Z", 1.0);
  v.tongueQ = knob("PJ_REED_TQ", 20);
  v.tongueRatio = knob("PJ_REED_TR", 1.03);
  v.footRise = knob("PJ_REED_FOOT", 0.004);
  // (the recorded Trompette's and Hautbois's bass: harmonics 2-7 louder than
  // the fundamental, a blare; their treble: falling fast above the 5th)
  const double below = std::log2(262 / hz);  // octaves below middle C
  switch (f) {
    case kTrumpet:
      v.bright = std::clamp(knob("PJ_TRUMPET_BRIGHT", 1.0) + knob("PJ_REED_BRIGHTOCT", 0.5) * below, 1.0, 2.2);
      v.modeTilt = 0.3;
      v.rollHz = knob("PJ_TRUMPET_ROLL", 6000);
      v.formantHz = knob("PJ_TRUMPET_FORMANT", 1500);
      v.formantGain = knob("PJ_TRUMPET_FGAIN", 2.5);
      v.formantOct = 1.5;
      break;
    case kOboe:
      v.zeta *= 0.8;
      v.bright = std::clamp(1.2 + 0.5 * below, 1.2, 2.4);
      v.modeTilt = 0.0;
      v.q1 = 30;
      v.rollHz = 5000;
      v.formantHz = knob("PJ_OBOE_FORMANT", 1300);
      v.formantGain = knob("PJ_OBOE_FGAIN", 4.0);
      v.formantOct = 0.8;
      break;
    case kCromorne:
      v.cylinder = true;
      v.bright = 0.8;
      v.modeTilt = 0.2;
      break;
    case kFagott:
      v.zeta *= 0.9;
      v.bright = 0.9;
      v.modeTilt = 0.1;
      v.rollHz = 5000;
      break;
    default:
      break;
  }
  // The treble (C5 up at 8' pitch: research/church_organ.md §8a). The formant
  // is the band the resonator's top radiates best (ka ~ 1 at its flare's
  // mouth); a treble pipe's top is a few mm to 2 cm across, its band higher:
  // once the pitch reaches half the stop's band (F#5 at 8' for the trumpets,
  // E5 the oboe) it rides on the 2nd harmonic, its top's roll above the 6th,
  // so the treble keeps the middle's spectrum (fixed, the top octave's
  // harmonics fell 15-45 dB below its fundamental: a sine). And the voicer's
  // line (the model's flow is per unit of a pipe's scale, its sound the
  // flow's rate of change: from middle C to C7 the reeds rose ~27 dB, the
  // formant reached as well): each pipe regulated from middle C up, 12 dB
  // down at C5 and 6 dB per octave more, the treble ~+1 dB per octave over
  // middle C (kept a little forward, as voicers leave a reed's treble); the
  // Oboe's 4 and 5 (its middle C a dip, its line its bass's).
  if (v.formantHz > 0) v.formantHz = std::max(v.formantHz, knob("PJ_REED_TREBLE_FORMANT", 2.0) * hz);
  v.rollHz = std::max(v.rollHz, knob("PJ_REED_TREBLE_ROLL", 6.0) * hz);
  const double up = std::log2(hz / 262), line1 = f == kOboe ? 4 : 12, line2 = f == kOboe ? 5 : 6;
  v.level = std::pow(10.0, -knob("PJ_REED_LINE", 1.0) * (line1 * std::clamp(up, 0.0, 1.0) + line2 * std::max(0.0, up - 1)) / 20);
  if (v.pressure > p0 && p0 < 1) v.level *= std::pow((1 - p0) / (1 - v.pressure), 0.91);
  return v;
}

}  // namespace

// The stops built so far (research/church_organ.md §6): each its division,
// its pipes' family, its pitch against the unison (8' = 1, 16' = 1/2, 4' =
// 2, 2 2/3' = 3 ...: a mutation tuned pure to its harmonic), its row on the
// chest (0 the front), its level against the division's unison (the voicer's
// regulation: mutations and high ranks softer).
// A mixture's rank has a composition instead: its ratio from C2, C3, C4,
// C5 and C6 on (breaking back as the keys rise, its pitches kept in the
// range small pipes can speak and the ear can blend).
struct Stop {
  const char* token;
  Division div;
  Family fam;
  double ratio;
  int row;
  double levelDb;
  double comp[5] = {0, 0, 0, 0, 0};
  double cents = 0;  // tuned off (a céleste: sharp, beating against its unison)
  int fromKey = 0;   // its compass's start (a céleste: tenor C)
  int breakKey = 0;  // a reprise: from this key up an octave lower (the Clairon's top octave)
  int toKey = 0;     // its compass's end, if short (a rank shared below another: Janakkala's Querflöt)
};
const Stop kStops[] = {
    // Hauptwerk: the principal chorus.
    {"principal16", kHW, kOpenPrincipal, 0.5, 10, -2},
    {"principal8", kHW, kOpenPrincipal, 1, 0, 0},
    {"fluteharm8", kHW, kHarmonicFlute, 1, 11, -1},
    {"octave4", kHW, kOpenPrincipal, 2, 1, -2},
    {"quinte223", kHW, kOpenPrincipal, 3, 2, -6},
    {"superoctave2", kHW, kOpenPrincipal, 4, 3, -4},
    {"rohrflote8", kHW, kChimney, 1, 4, -1},
    // Mixtur V (2'): 2' 1 1/3' 1' 2/3' 1/2' at C, breaking back an octave at
    // each c (the fifth and octave ranks rising through the compass).
    {"mixtur5", kHW, kOpenPrincipal, 0, 5, -9, {4, 3, 2, 1.5, 1}},
    {"mixtur5", kHW, kOpenPrincipal, 0, 5, -9, {6, 4, 3, 2, 1.5}},
    {"mixtur5", kHW, kOpenPrincipal, 0, 6, -9, {8, 6, 4, 3, 2}},
    {"mixtur5", kHW, kOpenPrincipal, 0, 6, -9, {12, 8, 6, 4, 3}},
    {"mixtur5", kHW, kOpenPrincipal, 0, 7, -9, {16, 12, 8, 6, 4}},
    // Cymbel III (1/2'): high, bright, breaking back each octave.
    {"cymbel3", kHW, kOpenPrincipal, 0, 8, -11, {16, 8, 4, 3, 2}},
    {"cymbel3", kHW, kOpenPrincipal, 0, 8, -11, {24, 12, 6, 4, 3}},
    {"cymbel3", kHW, kOpenPrincipal, 0, 8, -11, {32, 16, 8, 6, 4}},
    {"trompete16", kHW, kTrumpet, 0.5, 12, 0},
    // (beyond the 37 of the selection: the rest of the user's disposition)
    {"salicional8", kHW, kString, 1, 13, -4},
    {"blockflote4", kHW, kOpenFlute, 2, 14, -3},
    {"trompete8", kHW, kTrumpet, 1, 9, 1},
    // Schwellwerk: the flutes and the cornet's ranks.
    {"gedackt8", kSW, kStoppedWood, 1, 0, 0},
    {"bourdon16", kSW, kStoppedWood, 0.5, 1, -2},
    {"flute4", kSW, kOpenFlute, 2, 2, -2},
    {"nasard223", kSW, kOpenFlute, 3, 3, -6},
    {"waldflote2", kSW, kOpenFlute, 4, 4, -5},
    {"terz135", kSW, kOpenFlute, 5, 5, -8},
    {"gamba8", kSW, kString, 1, 6, -2},
    {"celeste8", kSW, kString, 1, 7, -2, {0, 0, 0, 0, 0}, 6, 48},
    {"fagott16", kSW, kFagott, 0.5, 8, -2},
    {"trompette8", kSW, kTrumpet, 1, 9, 0},
    {"oboe8", kSW, kOboe, 1, 10, -3},
    // (the Clairon breaks back at c''' (key 84), as most do: its top octave's pipes would sound
    // 2-4 kHz, resonators a few cm long; repeated at 8' pitch, no reed above C7)
    {"clairon4", kSW, kTrumpet, 2, 11, -1, {0, 0, 0, 0, 0}, 0, 0, 84},
    {"swprincipal8", kSW, kOpenPrincipal, 1, 12, -1},
    {"swoctave4", kSW, kOpenPrincipal, 2, 13, -3},
    // Plein jeu IV (2'): 2' 1 1/3' 1' 2/3' at C, breaking back an octave at each c.
    {"pleinjeu4", kSW, kOpenPrincipal, 0, 14, -9, {4, 3, 2, 1.5, 1}},
    {"pleinjeu4", kSW, kOpenPrincipal, 0, 14, -9, {6, 4, 3, 2, 1.5}},
    {"pleinjeu4", kSW, kOpenPrincipal, 0, 15, -9, {8, 6, 4, 3, 2}},
    {"pleinjeu4", kSW, kOpenPrincipal, 0, 15, -9, {12, 8, 6, 4, 3}},
    // Positiv.
    {"rohrgedackt8", kPos, kChimney, 1, 0, 0},
    {"posoctave4", kPos, kOpenPrincipal, 2, 1, -2},
    {"gedacktflote4", kPos, kStoppedWood, 2, 2, -3},
    {"sesquialtera", kPos, kOpenFlute, 3, 3, -6},
    {"sesquialtera", kPos, kOpenFlute, 5, 3, -8},
    {"cromorne8", kPos, kCromorne, 1, 4, -2},
    {"posprincipal8", kPos, kOpenPrincipal, 1, 5, -1},
    {"gemshorn8", kPos, kOpenFlute, 1, 6, -4},
    {"posoctave2", kPos, kOpenPrincipal, 4, 7, -4},
    {"quinte113", kPos, kOpenPrincipal, 0, 8, -7, {6, 6, 6, 6, 3}},  // (breaking back in the top octave)
    {"sifflote1", kPos, kOpenFlute, 0, 9, -7, {8, 8, 8, 8, 4}},      // (likewise)
    // Scharf IV (1'): 1' 2/3' 1/2' 1/3' at C, breaking back an octave at each c.
    {"scharf4", kPos, kOpenPrincipal, 0, 10, -10, {8, 6, 4, 3, 2}},
    {"scharf4", kPos, kOpenPrincipal, 0, 10, -10, {12, 8, 6, 4, 3}},
    {"scharf4", kPos, kOpenPrincipal, 0, 11, -10, {16, 12, 8, 6, 4}},
    {"scharf4", kPos, kOpenPrincipal, 0, 11, -10, {24, 16, 12, 8, 6}},
    {"dulcian16", kPos, kFagott, 0.5, 12, -3},
    {"chamade8", kPos, kTrumpet, 1, 13, 3},  // (the Trompette en chamade: horizontal, at the rail, into the nave)
    // Pedal.
    {"subbass16", kPed, kStoppedWood, 0.5, 1, 0},
    {"octavbass8", kPed, kOpenPrincipal, 1, 0, -2},
    {"choralbass4", kPed, kOpenPrincipal, 2, 2, -4},
    {"pedmixtur4", kPed, kOpenPrincipal, 0, 3, -9, {3, 3, 3, 3, 3}},
    {"pedmixtur4", kPed, kOpenPrincipal, 0, 3, -9, {4, 4, 4, 4, 4}},
    {"pedmixtur4", kPed, kOpenPrincipal, 0, 3, -9, {6, 6, 6, 6, 6}},
    {"pedmixtur4", kPed, kOpenPrincipal, 0, 3, -9, {8, 8, 8, 8, 8}},
    {"posaune16", kPed, kTrumpet, 0.5, 4, 1},
    {"untersatz32", kPed, kStoppedWood, 0.25, 5, -2},
    {"principalbass16", kPed, kOpenPrincipal, 0.5, 6, 0},
    {"violonbass16", kPed, kString, 0.5, 7, -2},
    {"pedtrompete8", kPed, kTrumpet, 1, 8, 0},
    {"gedacktbass8", kPed, kStoppedWood, 1, 9, -3},
    {"contraposaune32", kPed, kTrumpet, 0.25, 10, 1},
    {"klarine4", kPed, kTrumpet, 2, 11, -2},
    // ---- After the organ of Janakkala church (Martti Porthan, opus 11, 1993; 40/III, North German
    // baroque after Stade, St. Cosmae; Organ.md). The tokens start j_; the page gives the stops and
    // footages, not the mixtures' compositions: those are typical ones, as above.
    // I Rückpositiv (the Positiv's case on the gallery's rail).
    {"j_rp_principal8", kPos, kOpenPrincipal, 1, 0, -1},
    {"j_rp_gedackt8", kPos, kStoppedWood, 1, 1, -1},
    {"j_rp_quintadena8", kPos, kStoppedWood, 1, 2, -3},
    {"j_rp_octav4", kPos, kOpenPrincipal, 2, 3, -2},
    {"j_rp_rohrflot4", kPos, kChimney, 2, 4, -3},
    {"j_rp_waltflot2", kPos, kOpenFlute, 4, 5, -5},
    {"j_rp_sieflot113", kPos, kOpenFlute, 6, 6, -7},
    {"j_rp_sesquialter", kPos, kOpenFlute, 3, 7, -6},
    {"j_rp_sesquialter", kPos, kOpenFlute, 5, 7, -8},
    {"j_rp_scharff4", kPos, kOpenPrincipal, 0, 8, -10, {8, 6, 4, 3, 2}},
    {"j_rp_scharff4", kPos, kOpenPrincipal, 0, 8, -10, {12, 8, 6, 4, 3}},
    {"j_rp_scharff4", kPos, kOpenPrincipal, 0, 9, -10, {16, 12, 8, 6, 4}},
    {"j_rp_scharff4", kPos, kOpenPrincipal, 0, 9, -10, {24, 16, 12, 8, 6}},
    {"j_rp_dulcian16", kPos, kFagott, 0.5, 10, -3},
    {"j_rp_trechterregal8", kPos, kOboe, 1, 11, -4},
    // II Hauptwerk.
    {"j_hw_quintadena16", kHW, kStoppedWood, 0.5, 8, -3},
    {"j_hw_principal8", kHW, kOpenPrincipal, 1, 0, 0},
    {"j_hw_rohrflot8", kHW, kChimney, 1, 4, -1},
    {"j_hw_octav4", kHW, kOpenPrincipal, 2, 1, -2},
    {"j_hw_flot4", kHW, kOpenFlute, 2, 9, -3},
    {"j_hw_nassat3", kHW, kOpenFlute, 3, 10, -6},
    {"j_hw_octav2", kHW, kOpenPrincipal, 4, 2, -4},
    {"j_hw_mixtur5", kHW, kOpenPrincipal, 0, 5, -9, {4, 3, 2, 1.5, 1}},
    {"j_hw_mixtur5", kHW, kOpenPrincipal, 0, 5, -9, {6, 4, 3, 2, 1.5}},
    {"j_hw_mixtur5", kHW, kOpenPrincipal, 0, 6, -9, {8, 6, 4, 3, 2}},
    {"j_hw_mixtur5", kHW, kOpenPrincipal, 0, 6, -9, {12, 8, 6, 4, 3}},
    {"j_hw_mixtur5", kHW, kOpenPrincipal, 0, 7, -9, {16, 12, 8, 6, 4}},
    {"j_hw_cimbel3", kHW, kOpenPrincipal, 0, 3, -11, {16, 8, 4, 3, 2}},
    {"j_hw_cimbel3", kHW, kOpenPrincipal, 0, 3, -11, {24, 12, 6, 4, 3}},
    {"j_hw_cimbel3", kHW, kOpenPrincipal, 0, 3, -11, {32, 16, 8, 6, 4}},
    {"j_hw_trommet16", kHW, kTrumpet, 0.5, 11, 0},
    {"j_hw_trommet8", kHW, kTrumpet, 1, 12, 1},
    // III Brustwerk (printed "BW<": here in the swell boxes, with their shutters).
    {"j_bw_gedackt8", kSW, kStoppedWood, 1, 0, 0},
    // (the Querflöt: its own pipes from c1; "C-h0 komb. Ged.": below them a stopped rank)
    {"j_bw_querflot8", kSW, kOpenFlute, 1, 1, -2, {0, 0, 0, 0, 0}, 0, 60},
    {"j_bw_querflot8", kSW, kStoppedWood, 1, 1, -2, {0, 0, 0, 0, 0}, 0, 0, 0, 59},
    {"j_bw_gedacktflot4", kSW, kStoppedWood, 2, 2, -3},
    {"j_bw_octav2", kSW, kOpenPrincipal, 4, 3, -4},
    {"j_bw_sedetz1", kSW, kOpenPrincipal, 8, 4, -7},
    {"j_bw_tertia", kSW, kOpenFlute, 5, 5, -8},
    {"j_bw_tertia", kSW, kOpenFlute, 6, 5, -8},
    {"j_bw_scharff3", kSW, kOpenPrincipal, 0, 6, -10, {8, 6, 4, 3, 2}},
    {"j_bw_scharff3", kSW, kOpenPrincipal, 0, 6, -10, {12, 8, 6, 4, 3}},
    {"j_bw_scharff3", kSW, kOpenPrincipal, 0, 7, -10, {16, 12, 8, 6, 4}},
    {"j_bw_krumphorn8", kSW, kCromorne, 1, 8, -2},
    {"j_bw_schalmey4", kSW, kOboe, 2, 9, -4},
    // Pedal.
    {"j_p_principal16", kPed, kOpenPrincipal, 0.5, 6, 0},
    {"j_p_subbass16", kPed, kStoppedWood, 0.5, 1, 0},
    {"j_p_octav8", kPed, kOpenPrincipal, 1, 0, -2},
    {"j_p_octav4", kPed, kOpenPrincipal, 2, 2, -4},
    {"j_p_nachthorn2", kPed, kOpenFlute, 4, 7, -6},
    {"j_p_mixtur4", kPed, kOpenPrincipal, 0, 3, -9, {3, 3, 3, 3, 3}},
    {"j_p_mixtur4", kPed, kOpenPrincipal, 0, 3, -9, {4, 4, 4, 4, 4}},
    {"j_p_mixtur4", kPed, kOpenPrincipal, 0, 3, -9, {6, 6, 6, 6, 6}},
    {"j_p_mixtur4", kPed, kOpenPrincipal, 0, 3, -9, {8, 8, 8, 8, 8}},
    {"j_p_posaun16", kPed, kTrumpet, 0.5, 4, 1},
    {"j_p_dulcian16", kPed, kFagott, 0.5, 5, -2},
    {"j_p_trommet8", kPed, kTrumpet, 1, 8, 0},
};

// The regulation (the voicer's on site): each stop's output against the
// Principal 8', measured with every stop alone at three keys (K-weighted)
// and set to a balanced organ's: the Trompete and the Posaune +2 dB, the
// Octave 4' -2, the Mixtur -1, the swell's Gedackt -6 (its box 3/4 open),
// the mutations -7 .. -11, the strings -7, the Oboe -6; the Principal 16'
// -3, the Flûte harmonique -2, the Trompete 16' 0; in the Pedal the
// Principalbass -1, the Violonbass -6, the Untersatz 32' -10 (felt more than
// heard), the Trompete 0; the rest of the disposition: the Salicional -9,
// the Swell's Principal -2, the Plein jeu and the Scharf -3/-4, the Gemshorn
// -7, the chamade +5 (the loudest stop), the Contra Posaune 0. Its output only
// (the pipes' flow, the wind they draw, unchanged).
// The reeds re-regulated (2026-11-17) after their treble line (reedVoicing:
// 12 dB down at C5, 6 more per octave): their three keys' mean had counted a
// treble ~20 dB too loud, so they had come out 6-13 dB under these targets
// (the Trompete 8' -7, the Trompette -12, the chamade -4 against the
// Principal), and the crescendo's top steps added nothing. Now measured at
// C3, C4, C5 (the 16's at C4, C5; the Pedal's at C2, G2, D3) to the targets
// above, and the Swell's Trompette 0, the Fagott -2, the Clairon -1 (~1 dB
// under the Trompette, as it was), the Cromorne -6 (a solo reed, as the Oboe),
// the Klarine -2; the Dulcian (-6) as it was.
struct Regulation {
  const char* token;
  float db;
};
const Regulation kRegulation[] = {
    {"octave4", 1.3f},
    {"quinte223", 1.4f},
    {"superoctave2", 2.8f},
    {"rohrflote8", 7.4f},
    {"mixtur5", 6.2f},
    {"cymbel3", 10.0f},
    {"trompete8", 30.3f},
    {"gedackt8", 2.4f},
    {"bourdon16", 6.8f},
    {"flute4", 3.0f},
    {"nasard223", 3.4f},
    {"waldflote2", 0.7f},
    {"terz135", 8.0f},
    {"gamba8", 3.5f},
    {"celeste8", 2.9f},
    {"fagott16", 44.8f},
    {"trompette8", 32.9f},
    {"oboe8", 24.2f},
    {"clairon4", 32.5f},  // (its treble evened: re-regulated ~1 dB under the Trompette, its middle as before)
    {"rohrgedackt8", -0.7f},
    {"posoctave4", 0.0f},
    {"gedacktflote4", 1.3f},
    {"sesquialtera", 2.1f},
    {"cromorne8", 31.7f},
    {"subbass16", 8.2f},
    {"octavbass8", 2.0f},
    {"choralbass4", 3.4f},
    {"pedmixtur4", 7.1f},
    {"posaune16", 39.0f},
    {"principal8", 0.0f},
    {"principal16", 1.8f},
    {"fluteharm8", 9.8f},
    {"trompete16", 33.5f},
    {"untersatz32", 16.7f},
    {"principalbass16", 14.1f},
    {"violonbass16", 10.7f},
    {"pedtrompete8", 37.9f},
    {"salicional8", 4.3f},
    {"blockflote4", 8.0f},
    {"swprincipal8", 11.1f},
    {"swoctave4", 10.1f},
    {"pleinjeu4", 12.0f},
    {"posprincipal8", 4.2f},
    {"gemshorn8", 2.7f},
    {"posoctave2", 4.5f},
    {"quinte113", 3.7f},
    {"sifflote1", 3.7f},
    {"scharf4", 6.2f},
    {"dulcian16", 38.4f},
    {"chamade8", 31.2f},
    {"gedacktbass8", 13.2f},
    {"contraposaune32", 49.7f},
    {"klarine4", 39.7f},
    // (Janakkala's: each as its nearest stop above, to be regulated by ear)
    {"j_rp_principal8", 4.2f},
    {"j_rp_gedackt8", 2.4f},
    {"j_rp_quintadena8", 2.4f},
    {"j_rp_octav4", 0.0f},
    {"j_rp_rohrflot4", 1.3f},
    {"j_rp_waltflot2", 0.7f},
    {"j_rp_sieflot113", 3.7f},
    {"j_rp_sesquialter", 2.1f},
    {"j_rp_scharff4", 6.2f},
    {"j_rp_dulcian16", 38.4f},
    {"j_rp_trechterregal8", 24.2f},
    {"j_hw_quintadena16", 6.8f},
    {"j_hw_principal8", 0.0f},
    {"j_hw_rohrflot8", 7.4f},
    {"j_hw_octav4", 1.3f},
    {"j_hw_flot4", 8.0f},
    {"j_hw_nassat3", 3.4f},
    {"j_hw_octav2", 2.8f},
    {"j_hw_mixtur5", 6.2f},
    {"j_hw_cimbel3", 10.0f},
    {"j_hw_trommet16", 33.5f},
    {"j_hw_trommet8", 30.3f},
    {"j_bw_gedackt8", 2.4f},
    {"j_bw_querflot8", 3.0f},
    {"j_bw_gedacktflot4", 1.3f},
    {"j_bw_octav2", 4.5f},
    {"j_bw_sedetz1", 3.7f},
    {"j_bw_tertia", 2.1f},
    {"j_bw_scharff3", 6.2f},
    {"j_bw_krumphorn8", 31.7f},
    {"j_bw_schalmey4", 24.2f},
    {"j_p_principal16", 14.1f},
    {"j_p_subbass16", 8.2f},
    {"j_p_octav8", 2.0f},
    {"j_p_octav4", 3.4f},
    {"j_p_nachthorn2", 3.0f},
    {"j_p_mixtur4", 7.1f},
    {"j_p_posaun16", 39.0f},
    {"j_p_dulcian16", 38.4f},
    {"j_p_trommet8", 37.9f},
};
int gDisposition = 0;
OrganTemperament gTemperament = kJyrkiSpectral;
// A temperament against equal temperament, in cents, A kept at its pitch. Quarter-comma meantone: a
// key's place on the chain of fifths from Eb to G# (PJ_ORGAN_MEANTONE_LOW, -3; -4: Ab for G#). The
// well temperaments: each fifth of the circle from C by how far it is tempered from pure.
double temperamentOf(OrganTemperament temperament, int key) {
  if (temperament == kJyrkiSpectral || temperament == kJyrkiHeuristic || temperament == kTet12) return 0;
  const int pc = key % 12;
  if (temperament == kMeantone) {
    static const int kLow = int(knob("PJ_ORGAN_MEANTONE_LOW", -3));
    const double kFifth = 1200 * std::log2(5.0) / 4;
    const int f = kLow + ((pc * 7 - kLow) % 12 + 12) % 12;
    return (kFifth - 700) * (f - 3);
  }
  const double kPure = 1200 * std::log2(1.5), P = 12 * kPure - 8400, S = 1200 * std::log2(81.0 / 80);  // (the Pythagorean and syntonic commas)
  // (C-G G-D D-A A-E E-B B-F# F#-C# C#-G# G#-Eb Eb-Bb Bb-F F-C)
  const double kirnberger[12] = {-S / 4, -S / 4, -S / 4, -S / 4, 0, 0, -(P - S), 0, 0, 0, 0, 0};
  const double neidhardt[12] = {-P / 6, -P / 6, -P / 6, -P / 12, 0, -P / 12, -P / 12, -P / 12, 0, -P / 12, -P / 12, 0};
  const double lehman[12] = {-P / 6, -P / 6, -P / 6, -P / 6, 0, 0, 0, -P / 12, -P / 12, -P / 12, P / 12, -P / 6};
  const double* d = temperament == kKirnberger3 ? kirnberger : temperament == kNeidhardt ? neidhardt : lehman;
  auto at = [&](int target) {  // (up the circle from C to this pitch class: against equal)
    double c = 0;
    for (int i = 0, q = 0; q != target; ++i, q = (q + 7) % 12) c += kPure + d[i] - 700;
    return c;
  };
  return at(pc) - at(9);
}
double temperamentCents(int key) { return temperamentOf(gTemperament, key); }
bool ofThisOrgan(const Stop& st) { return (std::strncmp(st.token, "j_", 2) == 0) == (gDisposition == 1); }
float regulationDb(const char* token) {
  for (const Regulation& r : kRegulation)
    if (std::strcmp(r.token, token) == 0) return r.db;
  return 0;
}

// Registrations by name (in PJ_ORGAN_STOPS, alone or among stops): an
// organist's usual combinations (research/church_organ.md §6); the tutti
// without the Voix céleste (it beats against the chorus: as a real tutti
// and crescendo pedal leave it out).
struct Preset {
  const char* name;
  const char* stops;
};
const Preset kPresets[] = {
    {"foundations", "principal8,fluteharm8,rohrflote8,gedackt8,gamba8,rohrgedackt8,subbass16,octavbass8"},
    {"plenum", "principal8,octave4,quinte223,superoctave2,mixtur5,principalbass16,subbass16,octavbass8,choralbass4,pedmixtur4"},
    {"plenum16", "principal16,principal8,octave4,quinte223,superoctave2,mixtur5,cymbel3,principalbass16,subbass16,"
                 "octavbass8,choralbass4,pedmixtur4,untersatz32"},
    {"grandjeu", "principal8,octave4,trompete8,gedackt8,flute4,trompette8,clairon4,rohrgedackt8,posoctave4,cromorne8,"
                 "subbass16,octavbass8,posaune16,pedtrompete8"},
    {"cornet", "gedackt8,flute4,nasard223,waldflote2,terz135,subbass16"},
    {"sesquialtera", "rohrgedackt8,gedacktflote4,sesquialtera,subbass16"},
    {"strings", "gamba8,celeste8,violonbass16"},
    {"soft", "gedackt8,subbass16"},
    {"tutti", "principal16,principal8,fluteharm8,rohrflote8,octave4,quinte223,superoctave2,mixtur5,cymbel3,trompete16,"
              "trompete8,bourdon16,gedackt8,gamba8,flute4,nasard223,waldflote2,terz135,fagott16,trompette8,oboe8,"
              "clairon4,rohrgedackt8,posoctave4,gedacktflote4,sesquialtera,cromorne8,untersatz32,principalbass16,subbass16,"
              "violonbass16,octavbass8,choralbass4,pedmixtur4,posaune16,pedtrompete8,salicional8,blockflote4,swprincipal8,"
              "swoctave4,pleinjeu4,posprincipal8,gemshorn8,posoctave2,quinte113,sifflote1,scharf4,dulcian16,chamade8,"
              "gedacktbass8,contraposaune32,klarine4"},
    {"tutti37", "principal16,principal8,fluteharm8,rohrflote8,octave4,quinte223,superoctave2,mixtur5,cymbel3,trompete16,"
                "trompete8,bourdon16,gedackt8,gamba8,flute4,nasard223,waldflote2,terz135,fagott16,trompette8,oboe8,"
                "clairon4,rohrgedackt8,posoctave4,gedacktflote4,sesquialtera,cromorne8,untersatz32,principalbass16,subbass16,"
                "violonbass16,octavbass8,choralbass4,pedmixtur4,posaune16,pedtrompete8"},
};

// How strongly a pipe's jet hears the field at its mouth (its enclosure's,
// its neighbours').
// (0.3: at full strength, in a big registration, the field and the
// neighbours pulled detuned unisons 10 c and more and set up slow warbles on
// long-held keys; a real pipe pulls only those a few cents from it)
const float kHear = float(knob("PJ_ORGAN_COUPLE", 0.3));

OrganModel::OrganModel(const Instrument& inst) : Tube(inst) {
  const double fs = inst.fs;
  enc_.resize(kEnclosures);
  Enclosure::Params sw;
  sw.dims = {knob("PJ_SWELL_W", 2.6), 2.4, 1.6};
  sw.t60Low = knob("PJ_SWELL_T60", 0.6);
  sw.t60High = 0.12;
  sw.openT60 = 0.3;
  sw.field = knob("PJ_SWELL_FIELD", 0.5);
  sw.outDelay = 0.006;  // (behind and above the Positiv: ~2 m farther)
  const double spread = knob("PJ_SWELL_PAN", 0.7);
  sw.pan = -spread;
  enc_[kSwellC].setup(fs, sw);
  sw.pan = spread;
  enc_[kSwellCis].setup(fs, sw);
  Enclosure::Params pos;
  pos.dims = {1.1, 1.4, 0.7};
  pos.t60Low = 0.15;
  pos.t60High = 0.05;
  pos.shutters = false;
  pos.leak = 0;
  pos.field = 0.3;
  pos.pan = -0.3;
  enc_[kPosC].setup(fs, pos);
  pos.pan = 0.3;
  enc_[kPosCis].setup(fs, pos);
  // The main case: open in front, its roof and back reflecting; the
  // Hauptwerk's chest ~1.5 m behind the Positiv (and the façade in front of
  // it, the pipes standing in the church: almost no case around them).
  Enclosure::Params hw;
  hw.dims = {2.0, 3.0, 1.4};
  hw.t60Low = 0.25;
  hw.t60High = 0.08;
  hw.shutters = false;
  hw.leak = 0;
  hw.field = 0.4;
  hw.outDelay = 0.0045;
  hw.pan = -0.35;
  enc_[kHwC].setup(fs, hw);
  hw.pan = 0.35;
  enc_[kHwCis].setup(fs, hw);
  Enclosure::Params fac;
  fac.dims = {2.2, 4.0, 0.4};
  fac.t60Low = 0.08;
  fac.t60High = 0.03;
  fac.shutters = false;
  fac.leak = 0;
  fac.field = 0.1;
  fac.outDelay = 0.0035;
  fac.pan = -0.55;
  enc_[kFacC].setup(fs, fac);
  fac.pan = 0.55;
  enc_[kFacCis].setup(fs, fac);
  // The Pedal's towers at the case's sides, ~6 m apart: tall, open towards
  // the church, the big pipes standing in them.
  Enclosure::Params ped;
  ped.dims = {1.8, 6.0, 1.8};
  ped.t60Low = 0.3;
  ped.t60High = 0.08;
  ped.shutters = false;
  ped.leak = 0;
  ped.field = 0.35;
  ped.outDelay = 0.005;
  ped.directLp = knob("PJ_PEDAL_LP", 2500);  // (the towers behind the case and the façade: their direct sound's fizz softened)
  ped.pan = -0.85;
  enc_[kPedC].setup(fs, ped);
  ped.pan = 0.85;
  enc_[kPedCis].setup(fs, ped);
  // The stops drawn: by the instrument, or (-inst organ) PJ_ORGAN_STOPS, a
  // list of the tokens in kStops (default: Principal 8' and the swell's
  // Gedackt 8', coupled, and the Subbass 16').
  const std::string name = inst.name;
  std::string stops = name == "gedackt-8" ? "gedackt8" : name == "rohrgedackt-8" ? "rohrgedackt8"
                    : name == "gedackt-rohrgedackt" ? "gedackt8,rohrgedackt8" : name == "principal-8" ? "principal8"
                    : name == "organ-crescendo" ? "crescendo" : name == "organ-88" || name == "organ-plan" ? "all"
                    : gDisposition == 1 ? "j_hw_principal8,j_bw_gedackt8,j_p_subbass16" : "principal8,gedackt8,subbass16";
  zones_ = name == "organ-88";
  if (name == "organ")
    if (const char* e = std::getenv("PJ_ORGAN_STOPS")) stops = e;
  // (a preset's name expands to its stops; a stop named twice is drawn once)
  std::string expanded;
  for (size_t at = 0; at <= stops.size();) {
    const size_t e = std::min(stops.find(',', at), stops.size());
    std::string tok = stops.substr(at, e - at);
    for (const Preset& pr : kPresets)
      if (tok == pr.name) tok = pr.stops;
    if (!tok.empty()) expanded += (expanded.empty() ? "" : ",") + tok;
    at = e + 1;
  }
  std::vector<std::string> seen;
  stops.clear();
  for (size_t at = 0; at <= expanded.size();) {
    const size_t e = std::min(expanded.find(',', at), expanded.size());
    const std::string tok = expanded.substr(at, e - at);
    if (!tok.empty() && std::find(seen.begin(), seen.end(), tok) == seen.end()) {
      seen.push_back(tok);
      stops += (stops.empty() ? "" : ",") + tok;
    }
    at = e + 1;
  }
  for (size_t at = 0; at <= stops.size();) {
    const size_t e = std::min(stops.find(',', at), stops.size());
    const std::string tok = stops.substr(at, e - at);
    bool found = false;
    if (tok == "all") {  // (every stop built, none drawn: played by manuals, registered as it goes)
      byManual_ = true;
      for (const Stop& st : kStops)
        if (ofThisOrgan(st)) addRank(int(&st - kStops));
      at = e + 1;
      continue;
    }
    if (tok == "crescendo") {  // (the roller: every stop built, drawn by the roller's position)
      crescendo_ = true;
      for (const Stop& st : kStops)  // (the roller's steps name the large organ's stops)
        if (std::strncmp(st.token, "j_", 2) != 0) addRank(int(&st - kStops));
      at = e + 1;
      continue;
    }
    if (tok == "tremsw" || tok == "trempos") {  // (the tremulants: draw stops on the wind)
      (tok == "tremsw" ? tremSw_ : tremPos_) = true;
      found = true;
    }
    for (const Stop& st : kStops)
      if (tok == st.token) {
        addRank(int(&st - kStops));
        found = true;
      }
    if (!found && !tok.empty()) std::fprintf(stderr, "organ: no stop %s\n", tok.c_str());
    at = e + 1;
  }
  drawn_.assign(std::size(kStops), !crescendo_ && !byManual_);  // (else every stop built is drawn)
  if (name == "organ-plan")  // (until a plan registers it: -inst organ's default, every key on every manual)
    for (size_t i = 0; i < std::size(kStops); ++i)
      drawn_[i] = gDisposition == 1
                      ? !std::strcmp(kStops[i].token, "j_hw_principal8") || !std::strcmp(kStops[i].token, "j_bw_gedackt8") ||
                            !std::strcmp(kStops[i].token, "j_p_subbass16")
                      : !std::strcmp(kStops[i].token, "principal8") || !std::strcmp(kStops[i].token, "gedackt8") ||
                            !std::strcmp(kStops[i].token, "subbass16");
  if (crescendo_ || byManual_) manualRanks_ = true;
  calibrate();
  linkNeighbours();
  paths_ = knob("PJ_ORGAN_PATHS", 1) != 0;
  if (paths_) buildPaths();
  if (paths_) setupNave();
  // The wind (src/organwind.h; PJ_WIND=0: steady, as before).
  windOn_ = knob("PJ_WIND", 1) != 0;
  OrganWind::Params wp;
  wp.qMax = knob("PJ_WIND_QMAX", wp.qMax);
  wp.stiffness = knob("PJ_WIND_STIFF", wp.stiffness);
  wp.valveHz = knob("PJ_WIND_VALVE", wp.valveHz);
  wp.damping = knob("PJ_WIND_DAMP", wp.damping);
  wind_.setup(fs, wp);
  const double tRate = knob("PJ_TREM_RATE", 5.5), tDepth = knob("PJ_TREM_DEPTH", 8);  // (Hz; litres: ~13 % p-p, the flues ~+-3 c)
  wind_.setTremulant(kSW, tremSw_, tRate, tDepth);
  wind_.setTremulant(kPos, tremPos_, tRate * 1.05, tDepth * 0.7);  // (the Positiv's a little quicker, lighter)
  split_ = int(knob("PJ_ORGAN_SPLIT", 55));
  setSwell(0.75);
  // The microphones: the enclosures' places across the gallery as their
  // highs' level difference (at most ~6 dB: PJ_ORGAN_WIDTH), optionally time.
  static const double kWidth = knob("PJ_ORGAN_WIDTH", 0.5), kItd = knob("PJ_ORGAN_ITD", 0.0);  // (s; spaced microphones' time difference, ~0.0006: off by default, as it combs the mono sum)
  xoverA_ = float(1 - std::exp(-2 * M_PI * knob("PJ_ORGAN_XOVER", 300.0) / fs));
  mic_.resize(enc_.size());
  for (size_t i = 0; i < enc_.size(); ++i) {
    const double p = std::clamp(enc_[i].pan() * kWidth, -1.0, 1.0);
    mic_[i].gL = float(1 - p);
    mic_[i].gR = float(1 + p);
    const int d = std::min(63, int(std::lround(std::fabs(enc_[i].pan()) * kItd * fs)));
    (p < 0 ? mic_[i].delayR : mic_[i].delayL) = d;
  }
  for (Pipe& pp : pipes_) pp.baseRatio = pp.p.ratio();
  // The playing data back into the tuning (PJ_ORGAN_TUNEFILE: pitch logs,
  // concatenated): each pipe measured long enough and steady enough (100
  // periods, a spread under 2 c) retuned by half its mean deviation in play
  // (averaged over the logs): a tuner's correction in the finished organ,
  // damped so that one registration's pull does not mistune the others.
  if (const char* tf = std::getenv("PJ_ORGAN_TUNEFILE"))
    if (FILE* f = std::fopen(tf, "r")) {
      std::map<std::pair<int, int>, std::pair<double, double>> acc;  // (rank, key) -> (sum of means x periods, periods)
      char line[512];
      while (std::fgets(line, sizeof line, f)) {
        if (line[0] == '#') continue;
        char tok[64];
        int key, rank;
        double hz, mean, sd, lo, hi, secs;
        long periods;
        if (std::sscanf(line, "%63s %d %lf %lf %lf %lf %lf %lf %ld %d", tok, &key, &hz, &mean, &sd, &lo, &hi, &secs, &periods,
                        &rank) != 10 || periods < 100 || sd > 2)
          continue;
        auto& a = acc[{rank, key}];
        a.first += mean * periods;
        a.second += periods;
      }
      std::fclose(f);
      int n = 0;
      static const double kDamp = knob("PJ_ORGAN_TUNEDAMP", 0.5);
      for (Pipe& pp : pipes_) {
        auto it = acc.find({pp.stop, pp.key});
        if (it == acc.end()) continue;
        const double mean = it->second.first / it->second.second;
        pp.baseRatio *= std::exp2(-kDamp * mean / 1200);
        pp.p.tune(pp.baseRatio);
        ++n;
      }
      std::fprintf(stderr, "organ: %d pipes retuned from %s\n", n, tf);
    }
  ji_ = knob("PJ_ORGAN_JI", 1) != 0 && (gTemperament == kJyrkiSpectral || gTemperament == kJyrkiHeuristic);  // (the temperament is the tuning: nothing retunes it)
  std::fill(std::begin(jiSince_), std::end(jiSince_), -1L);
  std::fill(std::begin(jiPlanAt_), std::end(jiPlanAt_), -1000000L);
  pitchLog_ = std::getenv("PJ_ORGAN_PITCHLOG") != nullptr;
  if (pitchLog_)
    for (Pipe& pp : pipes_) {  // (a band-pass at its pitch, Q 8)
      const double w0 = 2 * M_PI * pp.target / fs_, al = std::sin(w0) / 16;
      pp.log.b0 = al / (1 + al);
      pp.log.a1 = 2 * std::cos(w0) / (1 + al);
      pp.log.a2 = -(1 - al) / (1 + al);
    }
  encPipes_.assign(enc_.size(), {});
  for (int i = 0; i < int(pipes_.size()); ++i) encPipes_[pipes_[i].enc].push_back(i);
  encOut_.assign(enc_.size(), {});
  encDemand_.assign(enc_.size(), {});
  // (Its swell boxes side by side: on the shared pool by default, where -play's parts and other
  // voices are too; PJ_ORGAN_RENDER_THREADS > 1: a pool of its own of that many; 1: one thread.)
  const int threads = int(knob("PJ_ORGAN_RENDER_THREADS", 0));
  if (threads > 1 && pipes_.size() > 100) pool_ = std::make_unique<WorkPool>(threads);
  sharedPool_ = threads == 0 && pipes_.size() > 100;
}

namespace {

PipeVoicing voicing(Family f, double hz) {
  PipeVoicing v;
  switch (f) {
    case kStoppedWood: v = gedackt(hz); break;
    case kChimney: v = rohrgedackt(hz); break;
    case kOpenPrincipal: v = principal(hz); break;
    case kOpenFlute: v = openFlute(hz); break;
    case kString: v = gamba(hz); break;
    case kHarmonicFlute: v = harmonicFlute(hz); break;
    default: v = principal(hz); break;
  }
  // The smallest pipes (above ~2 kHz: the 4' and higher ranks' top octaves):
  // their narrow mouths pass less flow, not a weaker acoustic velocity; the
  // jet deflected by the flow / the drive, so they still lock (at 4-8 kHz
  // the drive ~0.1: by the flow alone the loop's gain fell short).
  if (hz > 2000) v.driveNorm = std::max(v.driveNorm, 1.0);
  // The small pipes' chiff (from C6, ~1 kHz, fully from C7, ~2 kHz: the 2'
  // and smaller ranks, the mutations' and mixtures' upper ranks, the 4' top
  // octave; the 8' and 4' middle, and the strings, as they were;
  // research/church_organ.md §8a). A small pipe's chiff is the jet's first
  // few unsteady breaths: short (it forms in a few periods: ~11 ms at 2 kHz,
  // ~8 at 4, ~5 at 8, fading from when the wind reaches the foot, not from the
  // key's fall: else it was spent before the pipe spoke), on its first
  // overtone whatever the voicer's steady octave (an open flute's languid
  // sets its octave ~-30 dB; its chiff was then inaudible whatever its
  // strength), a stopped pipe's twelfth no stronger than the Gedackt's (it
  // was x3, the 4' top's twelfth leading by ~+16 dB, up to +50: overblown at
  // the attack). Its strength in the octave's own weight (1: driven at a
  // full principal's octave at the attack's peak): the principals, lightly
  // nicked, 2 (more tips them into the octave: at 4 their leads jumped from
  // ~+6 to +20..+30 dB); the open flutes' low cut-up as much (their leads a
  // few dB over the principals' all the same); the stopped 5. Each pipe its
  // own (the cut-up and the nicking never quite alike: x exp(+-0.25)), each
  // attack its own (+-15 %, a quicker pallet up to 15 % more); a key struck
  // again before its pallet closed gets less (FluePipe::open).
  if (const double w = std::clamp((std::log2(hz / 1046.5) - 0.02) / 0.98, 0.0, 1.0); w > 0 && f != kString) {  // (C6 itself, with its few cents of life, as it was)
    const double tSmall = std::clamp(0.03 * std::sqrt(262 / hz), 0.005, 0.03) * knob("PJ_SMALL_CHIFFTIME", 1.0);
    v.chiffTime = (1 - w) * v.chiffTime + w * tSmall;
    v.chiffFoot = w;
    const double cSmall = v.stopped ? knob("PJ_SMALL_CHIFF_S", 5.0)
                          : f == kOpenFlute || f == kHarmonicFlute ? knob("PJ_SMALL_CHIFF_F", 2.0)
                          : knob("PJ_SMALL_CHIFF_P", 2.0);
    const double unit = v.stopped ? 1.0 : v.openEven;  // (an open pipe's chiff in its octave's own weight)
    v.chiffEven = v.stopped ? 0.0 : 1.0;
    v.chiff = (1 - w) * v.chiff * unit + w * cSmall;
    v.chiffVar = w * knob("PJ_SMALL_CHIFFVAR", 0.25);
    v.chiffPress = w * knob("PJ_SMALL_CHIFFPRESS", 0.15);
  }
  // The wind's pitch: a flue ~0.5 c flat per 1 % of sag (research §4a.3: the
  // full organ's ~8 % sag, ~4 c, as the user chose).
  v.windTau = knob("PJ_WIND_PITCH", 0.6);
  // The field at the mouth driving the modes as the pipe starts (a partial
  // already in the air: a head start; PJ_ORGAN_SYMP 0: none).
  v.sympDrive = knob("PJ_ORGAN_SYMP", 0.0) * knob("PJ_ORGAN_SYMPGAIN", 8.0);  // (off: the field includes the pipe's own sound, which this fought - slower speech; to be reworked)
  return v;
}

}  // namespace

OrganModel::~OrganModel() {
  if (std::getenv("PJ_WIND_LOG")) std::fprintf(stderr, "organ wind: lowest chest %.4f\n", wind_.lowest());
  if (pitchLog_) writePitchLog();
}

// Every pipe that sounded: its stop, key, the pitch it was tuned to, and as
// it played: its mean against it (cents), the spread, the lowest and highest,
// how long (s): the wind's sag, the neighbours' and the field's pull, the
// head starts, as a tuner would hear them.
void OrganModel::writePitchLog() const {
  FILE* f = std::fopen(std::getenv("PJ_ORGAN_PITCHLOG"), "w");
  if (!f) return;
  std::fprintf(f, "# stop key target_hz mean_cents sd_cents lo_cents hi_cents seconds periods rank\n");
  double worst = 0;
  const Pipe* worstPipe = nullptr;
  int count = 0;
  for (const Pipe& pp : pipes_) {
    const Pipe::PitchLog& L = pp.log;
    if (L.n < 20) continue;
    const double m = L.sum / L.n, sd = std::sqrt(std::max(0.0, L.sum2 / L.n - m * m));
    std::fprintf(f, "%s %d %.3f %+.2f %.2f %+.2f %+.2f %.2f %ld %d\n", kStops[pp.stop].token, pp.key, pp.target, m, sd, L.lo,
                 L.hi, L.sounding / fs_, L.n, pp.stop);
    ++count;
    if (std::fabs(m) > std::fabs(worst)) {
      worst = m;
      worstPipe = &pp;
    }
  }
  std::fclose(f);
  std::fprintf(stderr, "organ pitch log: %d pipes to %s", count, std::getenv("PJ_ORGAN_PITCHLOG"));
  if (worstPipe) std::fprintf(stderr, " (the farthest on average: %s key %d, %+.1f c)", kStops[worstPipe->stop].token, worstPipe->key, worst);
  std::fprintf(stderr, "\n");
}

// One rank: its pipes voiced, tuned (each a little off, as a tuner leaves
// them: +-0.5 c; the swell box's air a little warmer than the church's, its
// pipes sharper), placed on its division's divided chest (the C side in the
// left enclosure, the C# side in the right, the biggest at the outer ends;
// the big basses offset or in the façade). Calibrated afterwards.
void OrganModel::addRank(int stop) {
  const Stop& st = kStops[stop];
  const double fs = fs_;
  Rng rng(0x5EED0000u + stop);
  // (its offset against the other ranks, -1 .. 1 of PJ_ORGAN_RANK_HZ; the
  // two stopped flutes set: the Rohrgedackt at 0 Hz, the Gedackt +0.15 Hz,
  // PJ_ORGAN_ROHR_HZ, PJ_ORGAN_GEDACKT_HZ: their pair's slow beat)
  double rankHz = rng.uniform();
  {
    static const double kRohr = knob("PJ_ORGAN_ROHR_HZ", 0.0), kGed = knob("PJ_ORGAN_GEDACKT_HZ", 0.15),
                        kRank = knob("PJ_ORGAN_RANK_HZ", 1.0 / 4.5);
    const std::string tok = kStops[stop].token;
    if (kRank > 0 && (tok == "gedackt8" || tok == "rohrgedackt8")) rankHz = (tok == "gedackt8" ? kGed : kRohr) / kRank;
  }
  const double warm = st.div == kSW ? knob("PJ_SWELL_WARM", 0.3) : 0.0;  // degrees C over the church
  const double warmCents = 1200 * std::log2(std::sqrt((293.15 + warm) / 293.15));
  const bool pedal = st.div == kPed;
  const int lo = std::max(kLo, st.fromKey), hi = std::min(pedal ? kLo + 31 : kHi, st.toKey ? st.toKey : 127);  // (the Pedal: 32 keys, C2..G4 as played)
  if (!pedal) manualRanks_ = true;
  // Which pipes stand apart (the façade, the offset basses): the big ones.
  auto offset = [&](double hz) {
    if (st.div == kHW) return st.fam == kOpenPrincipal && st.ratio > 0 && st.ratio <= 1 && hz < 180;  // the façade
    if (st.div == kSW) return hz < 131 && st.ratio <= 1;                               // the back wall
    return false;
  };
  int nChest[2] = {0, 0}, nOff[2] = {0, 0};
  auto ratio = [&](int m) {
    const double r = st.ratio > 0 ? st.ratio : st.comp[std::clamp((m - 36) / 12, 0, 4)];
    return st.breakKey && m >= st.breakKey ? r / 2 : r;
  };
  for (int m = lo; m <= hi; ++m) {
    const double hz = 440 * std::exp2((m - 69) / 12.0) * ratio(m);
    (offset(hz) ? nOff : nChest)[(m - lo) % 2]++;
  }
  int iChest[2] = {0, 0}, iOff[2] = {0, 0};
  const int first = int(pipes_.size());
  for (int m = lo; m <= hi; ++m) {
    static const double kDetune = knob("PJ_ORGAN_DETUNE", 0.0);  // (c: an offset in cents too, if wanted)
    // (a little life: each pipe +- 1/4.5 Hz off its stop, its stop +- 1/4.5 Hz off
    // the others: two pipes beat at most ~0.44 Hz whatever their pitch, a slow
    // swell; pure intervals alone sound clinical)
    static const double kPipeHz = knob("PJ_ORGAN_DETUNE_HZ", 1.0 / 4.5), kRankHz = knob("PJ_ORGAN_RANK_HZ", 1.0 / 4.5);
    const double cents = warmCents + st.cents + kDetune * rng.uniform() + temperamentCents(m);
    const double hz = 440 * std::exp2((m - 69) / 12.0 + cents / 1200) * ratio(m) + rankHz * kRankHz + kPipeHz * rng.uniform();
    Pipe pp;
    // (the smallest pipes run oversampled: their jet's travel a few samples
    // at 48 kHz, too coarse for them to lock)
    pp.os = hz > 6000 ? 4 : hz > 2500 ? 2 : 1;
    if (isReed(st.fam)) {
      pp.os = 1;  // (a reed runs 2x oversampled inside)
      pp.p.reed = true;
      pp.p.r.setup(fs, hz, reedVoicing(st.fam, hz), randomSeed(stop, m));
    } else {
      {
      PipeVoicing v = voicing(st.fam, hz);
      if (pedal) {  // (the pedal's pipes in their towers, behind walls: their breath and puff dark and softer)
        v.noiseLp = knob("PJ_PEDAL_NOISELP", 800);
        v.breath *= 0.5;
        // (their attack's puff: PJ_PEDAL_PUFF_DB further down)
        static const double kPedalPuff = std::pow(10.0, knob("PJ_PEDAL_PUFF_DB", -3) / 20);
        v.puff *= 0.5 * kPedalPuff;
      }
      pp.p.f.setup(fs * pp.os, hz, v, randomSeed(stop, m));
    }
    }
    pp.hz = hz;
    pp.key = m;
    pp.div = st.div;
    // Its flow (research/church_organ.md §4a.2): ~3 L/s at 8' C, ~0.5 at
    // middle C, a few hundredths for a mixture's smallest; a stopped wood's
    // wide mouth more, a string's narrow one less; a softer rank's mouth
    // smaller.
    pp.flow = float(3.0 * std::pow(hz / 65.4, -1.2) * (st.fam == kStoppedWood ? 1.3 : st.fam == kString ? 0.5 : isReed(st.fam) ? 1.4 : 1.0) *
                    std::pow(10.0, st.levelDb / 20));
    pp.stop = stop;
    pp.pedal = pedal;
    pp.gain = float(std::pow(10.0, (st.levelDb + regulationDb(st.token)) / 20));
    const float regGain = float(std::pow(10.0, regulationDb(st.token) / 20));
    const int side = (m - lo) % 2;
    const double outer = side == 0 ? -1 : 1;
    const bool off = offset(hz);
    const int j = off ? iOff[side]++ : iChest[side]++;  // 0: the biggest of its side (here)
    const int n = std::max(1, (off ? nOff : nChest)[side] - 1);
    Vec3 at;
    switch (st.div) {
      case kSW: {
        const double w = 2.6, h = 2.4, d = 1.6;
        if (off)  // along the back wall, lower, ~0.33 m apart
          at = {outer * (w / 2 - 0.25 - j * std::min(0.33, (w / 2 - 0.4) / n)), -h / 2 + 0.3 + 0.3 * st.row, -d / 2 + 0.2};
        else
          at = {outer * (w / 2 - 0.3 - j * (w / 2 - 0.4) * 2 / (2 * n)), -h / 2 + 0.55, 0.1 - 0.22 * st.row};
        pp.enc = side == 0 ? kSwellC : kSwellCis;
        break;
      }
      case kPos:
        at = {outer * (0.45 - j * 0.9 / std::max(n, 30)), -0.35, -0.12 * st.row};
        pp.enc = side == 0 ? kPosC : kPosCis;
        break;
      case kHW:
        if (off) {  // the façade, mouths at the impost
          at = {outer * (1.0 - j * 0.11), -1.4, 0.0};
          pp.enc = side == 0 ? kFacC : kFacCis;
        } else {
          at = {outer * (0.85 - j * 1.7 / std::max(n, 21)), -1.0, 0.4 - 0.18 * st.row};
          pp.enc = side == 0 ? kHwC : kHwCis;
        }
        break;
      case kPed:
        at = {outer * (0.7 - j * 1.4 / std::max(n, 15)), -2.6, 0.5 - 0.3 * st.row};
        pp.enc = side == 0 ? kPedC : kPedCis;
        break;
    }
    if (isReed(st.fam))  // (a reed sounds from its resonator's top: a cone ~ half a wavelength long, the
                         // Cromorne's cylinder a quarter)
      at.y += 343 / ((st.fam == kCromorne ? 4 : 2) * hz) * 0.9;
    pp.at = at;
    pp.src = enc_[pp.enc].addSource(at);
    if (pp.p.openPipe()) {  // its top, ~ half a wavelength above the mouth
      Vec3 top = at;
      top.y += 0.95 * 343 / (2 * hz);
      pp.srcTop = enc_[pp.enc].addSource(top);
    }
    // The field moves its jet by the acoustic velocity across the mouth, ~ the
    // pressure / its frequency: the treble's jets hear the box less.
    // (PJ_ORGAN_HEARCAP caps it, e.g. at 1: middle C's)
    pp.hear = float(kHear * std::min(knob("PJ_ORGAN_HEARCAP", 100.0), std::pow(262 / hz, knob("PJ_ORGAN_HEAREXP", 1.0))));
    // (a reed is driven by its boot's pressure, hundreds of times the field at
    // its resonator's top: it does not hear the room; even a twentieth of a
    // flue's hearing, the low reeds' loud sound through their enclosure back
    // into their resonators howled on after the keys' release)
    if (isReed(st.fam)) pp.hear *= float(knob("PJ_REED_HEAR", 0.0));
    // (the regulation evens out the families' outputs, not how loud a pipe is
    // to itself: its own field through its enclosure heard as before it)
    pp.hear /= regGain;
    if (pedal && isReed(st.fam)) {  // (inside the case: PJ_PEDREED_CASE_DB down above PJ_PEDREED_CASE_HZ)
      pp.caseHz = float(knob("PJ_PEDREED_CASE_HZ", 300));
      pp.caseA = float(1 - std::exp(-2 * M_PI * pp.caseHz / fs));
      pp.caseG = float(std::pow(10.0, -knob("PJ_PEDREED_CASE_DB", 5) / 20));
    }
    if (pedal && (std::string(kStops[stop].token) == "posaune16" || std::string(kStops[stop].token) == "j_p_posaun16")) {
      // Each Posaune pipe's speech by its own place: its height over the floor, the boxes and
      // intermediate walls near it give a broad peak or dip, its way out more or less highs.
      static const double kDb = knob("PJ_POSAUNE_ATTACK_DB", -2), kColor = knob("PJ_POSAUNE_COLOR_DB", 2.5);
      static const double kHoldMs = knob("PJ_POSAUNE_ATTACK_MS", 170), kFadeMs = knob("PJ_POSAUNE_ATTACK_FADE", 60);
      uint64_t h = (uint64_t(m) + 1) * 0x9E3779B97F4A7C15ull;
      auto draw = [&h] { h ^= h >> 29; h *= 0xBF58476D1CE4E5B9ull; h ^= h >> 32; h += 0x9E3779B97F4A7C15ull; return double(h >> 40) / double(1 << 24); };
      const double fc = 400 * std::exp2(2 * draw()), peakDb = kColor * (2 * draw() - 1), tiltDb = 0.6 * kColor * (2 * draw() - 1);
      const double A = std::pow(10.0, peakDb / 40), w = 2 * M_PI * fc / fs, al = std::sin(w) / (2 * 0.8), a0 = 1 + al / A;
      // (the groove and the foot behind the pallet, ReedPipe::setGroove: PJ_POSAUNE_TOE, the toe's
      // resistance, smaller for a more open toe and more overshoot; < 0, by default: off)
      static const double kToe = knob("PJ_POSAUNE_TOE", -1), kGrooveHz = knob("PJ_POSAUNE_GROOVE_HZ", 45);
      static const double kTurb = knob("PJ_POSAUNE_TURB", 0.3), kBleed = knob("PJ_POSAUNE_BLEED", 0.25);
      if (kToe >= 0) pp.p.r.setGroove(kGrooveHz, kToe, kTurb, kBleed);
      if (kToe >= 0) pp.p.r.setValveScale(knob("PJ_POSAUNE_PALLET", 1));  // (its pallet's time x this)
      // (the release's blat: its tongue damped as the wind falls; Janakkala's Posaun its own)
      pp.p.r.setReleaseDamp(std::strncmp(kStops[stop].token, "j_", 2) == 0 ? knob("PJ_JPOSAUN_RELDAMP", 200) : knob("PJ_POSAUNE_RELDAMP", 200));
      pp.p.r.setReleaseNoise(knob("PJ_POSAUNE_RELNOISE", 4));  // (the release's hiss: its turbulence gone with the wind, not back)
      pp.p.r.setCloseScale(knob("PJ_POSAUNE_CLOSE", 0.5));       // (the airflow's closing time x this)
      Pipe::Speech& sp = pp.speech;
      sp.on = kDb != 0 || kColor != 0 || knob("PJ_POSAUNE_DARK_DB", -8) != 0;
      sp.b0 = float((1 + al * A) / a0), sp.b1 = float(-2 * std::cos(w) / a0), sp.b2 = float((1 - al * A) / a0);
      sp.a1 = sp.b1, sp.a2 = float((1 - al / A) / a0);
      sp.tiltA = float(1 - std::exp(-2 * M_PI * 1500 / fs)), sp.tiltG = float(std::pow(10.0, tiltDb / 20));
      sp.gain = float(std::pow(10.0, kDb / 20));
      static const double kDarkDb = knob("PJ_POSAUNE_DARK_DB", -8), kDarkHz = knob("PJ_POSAUNE_DARK_HZ", 500);
      sp.darkA = float(1 - std::exp(-2 * M_PI * kDarkHz / fs)), sp.darkG = float(std::pow(10.0, kDarkDb / 20));
      sp.holdN = int(kHoldMs * 1e-3 * fs), sp.decay = float(std::exp(-1 / (kFadeMs * 1e-3 * fs)));
    }
    pp.target = hz;
    pipes_.push_back(pp);
    (pedal ? byPedal_ : byKey_)[m].push_back(int(pipes_.size()) - 1);
  }
  (void)first;
}

// Every pipe voiced in tune where it stands: the jet and its enclosure pull
// the pitch a little; turn the modes back (twice: the second pass measures
// the corrected pipe). The pipes apart, on a few threads.
// (cached in ~/.cache/pajupilli, keyed on this binary and the PJ_* knobs,
// the stops drawn among them, as src/calibrate.cc keys its own; PJ_NOCALCACHE
// bypasses it)
static std::string organCachePath(const char* inst) {
  const char* home = std::getenv("HOME");
  struct stat st;
  if (std::getenv("PJ_NOCALCACHE") || !home || stat("/proc/self/exe", &st) != 0) return "";
  uint64_t h = 1469598103934665603ull;  // FNV-1a
  auto mix = [&h](const void* d, size_t n) {
    for (size_t i = 0; i < n; ++i) h = (h ^ static_cast<const unsigned char*>(d)[i]) * 1099511628211ull;
  };
  mix(&st.st_size, sizeof st.st_size);
  mix(&st.st_mtime, sizeof st.st_mtime);
  mix(&gDisposition, sizeof gDisposition);
  mix(&gTemperament, sizeof gTemperament);
  std::vector<std::string> env;
  for (char** e = environ; *e; ++e)
    if (std::strncmp(*e, "PJ_", 3) == 0) {
      // (not the settings that do not touch the pipes: the threads, the logs)
      bool skip = false;
      for (const char* k : {"PJ_ORGAN_THREADS=", "PJ_ORGAN_RENDER_THREADS=", "PJ_WIND_LOG=", "PJ_CALDBG=",
                            "PJ_PLAY_THREADS=", "PJ_PLAY_LEAD=", "PJ_ROLLER_OMIT=", "PJ_THREADS=",
                            "PJ_VOICE_THREADS=", "PJ_PLAY_UNTIL=", "PJ_PLAY_TICK="})
        skip |= std::strncmp(*e, k, std::strlen(k)) == 0;
      if (!skip) env.emplace_back(*e);
    }
  std::sort(env.begin(), env.end());
  for (const std::string& e : env) mix(e.data(), e.size() + 1);
  mix(inst, std::strlen(inst) + 1);
  const std::string cache = std::string(home) + "/.cache", dir = cache + "/pajupilli";
  mkdir(cache.c_str(), 0755);
  mkdir(dir.c_str(), 0755);
  char name[48];
  std::snprintf(name, sizeof name, "/organ-%016llx.bin", static_cast<unsigned long long>(h));
  return dir + name;
}

// A pipe's spectrum from the model: alone, settled (25 periods or 0.3 s),
// its output's level at each harmonic up to 40 x or 16 kHz (a Hann window
// over 12 periods or more, each harmonic read on its own), normalised to the
// strongest; spectrum[k - 1] the k-th harmonic's.
static std::vector<float> modelSpectrum(AnyPipe p, double target, double fs, int os) {
  p.reset();
  p.open();
  const int warm = int(std::max(0.3 * fs, 25 * fs / target)), N = std::max(4096, int(12 * fs / target));
  std::vector<double> y(N);
  for (int n = 0; n < warm + N; ++n) {
    double v = 0;
    for (int k = 0; k < os; ++k) v += p.sample(1.0f, 0.0f);
    if (n >= warm) y[n - warm] = v / os * (0.5 - 0.5 * std::cos(2 * M_PI * (n - warm + 0.5) / N));
  }
  const int K = std::max(1, int(std::min(40.0, std::min(16000.0, 0.45 * fs) / target)));
  std::vector<float> out(K);
  double top = 0;
  std::vector<double> lv(K);
  for (int k = 1; k <= K; ++k) {  // (Goertzel at k x its pitch)
    const double w = 2 * M_PI * k * target / fs, c = 2 * std::cos(w);
    double s1 = 0, s2 = 0;
    for (int n = 0; n < N; ++n) {
      const double s0 = y[n] + c * s1 - s2;
      s2 = s1;
      s1 = s0;
    }
    lv[k - 1] = std::sqrt(std::max(0.0, s1 * s1 + s2 * s2 - c * s1 * s2));
    top = std::max(top, lv[k - 1]);
  }
  for (int k = 0; k < K; ++k) out[k] = float(top > 0 ? lv[k] / top : 0);
  return out;
}

void OrganModel::calibrate() {
  const int n = int(pipes_.size());
  const std::string path = organCachePath(inst_.name);
  if (!path.empty())
    if (FILE* f = std::fopen(path.c_str(), "rb")) {
      std::vector<double> r(n);
      bool ok = int(std::fread(r.data(), sizeof(double), n, f)) == n;
      std::vector<std::vector<float>> sp(n);
      for (int i = 0; i < n && ok; ++i) {  // (each pipe's spectrum: its modes' count, their levels)
        uint32_t c = 0;
        ok = std::fread(&c, sizeof c, 1, f) == 1 && c < 1024;
        if (!ok) break;
        sp[i].resize(c);
        ok = std::fread(sp[i].data(), sizeof(float), c, f) == c;
      }
      ok = ok && std::fgetc(f) == EOF;
      std::fclose(f);
      if (ok) {
        for (int i = 0; i < n; ++i) {
          pipes_[i].p.tune(r[i]);
          pipes_[i].spectrum = sp[i];
        }
        return;
      }
    }
  const int threads = std::clamp(int(knob("PJ_ORGAN_THREADS", 4)), 1, 32);
  std::atomic<int> next{0};
  auto work = [&] {
    for (int i; (i = next++) < n;) {
      Pipe& pp = pipes_[i];
      // (a flue: two proportional steps; a reed's pitch, its tongue's and its
      // resonator's together, follows a retuning unevenly - a plateau, then
      // steep: secant steps on the last two tries, until within half a cent)
      double r0 = 0, g0 = 0, bestR = pp.p.ratio(), bestE = 1e9;
      for (int it = 0; it < (pp.p.reed ? 6 : 3); ++it) {
        const double r1 = pp.p.ratio(), got = speakingHz(pp.p, enc_[pp.enc], pp.src, pp.srcTop, pp.hear, fs_, pp.os, pp.gain);
        if (got > 0 && std::fabs(std::log2(got / pp.target)) < bestE) {
          bestE = std::fabs(std::log2(got / pp.target));
          bestR = r1;
        }
        if (std::getenv("PJ_CALDBG") && pp.key == std::atoi(std::getenv("PJ_CALDBG")))
          std::fprintf(stderr, "cal %s key %d pass %d: got %.4f want %.4f ratio %.5f\n", kStops[pp.stop].token, pp.key, it, got, pp.target, r1);
        if (got <= 0) break;
        if (pp.p.reed && std::fabs(1200 * std::log2(got / pp.target)) < 0.5) break;
        double next = r1 * pp.target / got;
        if (pp.p.reed && it > 0 && std::fabs(got - g0) > 1e-4 * got) {
          const double sec = r1 + (pp.target - got) * (r1 - r0) / (got - g0);
          if (sec > 0.9 * r1 && sec < 1.1 * r1) next = sec;
        }
        r0 = r1;
        g0 = got;
        pp.p.tune(next);
      }
      // (a reed still off by more than 2 c: its pitch jumps between two
      // regimes there; the closest of a fine scan of tunings, 0.1 % apart,
      // around the best of the tries so far)
      if (pp.p.reed) {
        const double got = speakingHz(pp.p, enc_[pp.enc], pp.src, pp.srcTop, pp.hear, fs_, pp.os, pp.gain);
        if (got > 0 && std::fabs(std::log2(got / pp.target)) < bestE) {
          bestE = std::fabs(std::log2(got / pp.target));
          bestR = pp.p.ratio();
        }
        pp.p.tune(bestR);
        if (1200 * bestE > 2) {
          const double r = bestR;
          double best = r, bestErr = bestE;
          for (int i = -15; i <= 15; ++i) {
            pp.p.tune(r * (1 + 0.001 * i));
            const double g = speakingHz(pp.p, enc_[pp.enc], pp.src, pp.srcTop, pp.hear, fs_, pp.os, pp.gain);
            if (g > 0 && std::fabs(std::log2(g / pp.target)) < bestErr) {
              bestErr = std::fabs(std::log2(g / pp.target));
              best = r * (1 + 0.001 * i);
            }
          }
          pp.p.tune(best);
          if (std::getenv("PJ_CALDBG") && pp.key == std::atoi(std::getenv("PJ_CALDBG")))
            std::fprintf(stderr, "cal %s key %d scan: best ratio %.5f, %.2f c off\n", kStops[pp.stop].token, pp.key, best, 1200 * bestErr);
        }
      }
      pp.spectrum = modelSpectrum(pp.p, pp.target, fs_, pp.os);
      pp.p.reset();
    }
  };
  std::vector<std::thread> pool;
  for (int t = 1; t < threads; ++t) pool.emplace_back(work);
  work();
  for (std::thread& t : pool) t.join();
  if (!path.empty())
    if (FILE* f = std::fopen(path.c_str(), "wb")) {
      for (const Pipe& pp : pipes_) {
        const double r = pp.p.ratio();
        std::fwrite(&r, sizeof r, 1, f);
      }
      for (const Pipe& pp : pipes_) {
        const uint32_t c = uint32_t(pp.spectrum.size());
        std::fwrite(&c, sizeof c, 1, f);
        std::fwrite(pp.spectrum.data(), sizeof(float), c, f);
      }
      std::fclose(f);
    }
}

// The near field: each pipe hears the pipes nearest it directly, ~1 /
// distance: its rank's neighbours on its side (a whole tone and two away), and
// the same key's pipes in the neighbouring rows of its chest (a unison or
// an octave a hand's breadth away: they pull each other, "Mitnahme").
void OrganModel::linkNeighbours() {
  static const double kNear = knob("PJ_ORGAN_NEAR", 0.3);
  const int n = int(pipes_.size());
  for (int a = 0; a < n; ++a) {
    Pipe& pa = pipes_[a];
    std::vector<std::pair<double, int>> cand;
    for (int b = 0; b < n; ++b) {
      if (b == a || pipes_[b].enc != pa.enc) continue;
      const Pipe& pb = pipes_[b];
      const bool sameRank = pb.stop == pa.stop && std::abs(pb.key - pa.key) <= 4;
      const bool sameKey = pb.key == pa.key && pb.stop != pa.stop;
      if (!sameRank && !sameKey) continue;
      const double dx = pb.at.x - pa.at.x, dy = pb.at.y - pa.at.y, dz = pb.at.z - pa.at.z;
      cand.push_back({std::sqrt(dx * dx + dy * dy + dz * dz) + 0.03, b});
    }
    std::sort(cand.begin(), cand.end());
    for (int c = 0; c < 4 && c < int(cand.size()); ++c) {
      pa.near[c] = cand[c].second;
      pa.nearG[c] = float(kNear * 0.03 / cand[c].first);
    }
  }
}

// The keys: the manuals' pipes for every key held; the Pedal's for the
// lowest key held below PJ_ORGAN_SPLIT (the bass line) when manual stops are
// drawn too, else for every key in its compass.
// A pallet's time: 24 ms to open, 30 to close (a reed's own voicing aside),
// +- the variance (each press, each pallet: a finger's, a tracker's, a
// spring's unevenness), the bass a little slower (its pallets bigger, up to
// +4 ms at 8' C), the pedal's more (+6 ms).
void OrganModel::drawPallet(int div, int key, bool opening) {
  static const double kVar = knob("PJ_ORGAN_PALLETVAR", 0.006);
  const double bass = 0.004 * std::clamp((72 - key) / 36.0, 0.0, 1.0) + (div == 3 ? 0.006 : 0.0);
  // (the touch: a softer finger's slower pallet. A release's at least as soft
  // as the key held long and high asks: from C5, held from 0.5 s to 2 s:
  // played live, with no plan to say the note's length ahead)
  double touch = touch_[key];
  if (!opening) {
    const double held = double(now_ - pressAt_[key]) / fs_;
    touch = std::max(touch, std::clamp((key - 72) / 12.0 + 0.5, 0.0, 1.0) * std::clamp((held - 0.5) / 1.5, 0.0, 1.0));
  }
  static const double kSoft = knob("PJ_ORGAN_SOFTTOUCH", 1.0);
  const double slow = 1 + kSoft * touch * (opening ? 1.5 : 1.0);
  const double t = ((opening ? 0.024 : 0.030) + bass) * slow + kVar * palletRng_.uniform();
  (opening ? palletOpen_ : palletClose_)[div][key] = float(std::max(0.004, t));
}

double OrganModel::liveTouch(int midi) {
  static const double kOn = knob("PJ_ORGAN_LIVETOUCH", 1.0);
  const long win = long(7 * fs_);
  recent_.erase(std::remove_if(recent_.begin(), recent_.end(), [&](auto& r) { return now_ - r.first > win; }), recent_.end());
  // (a long melody note: one released in the last 7 s, or one still held 1 s)
  bool longNote = longAt_ >= 0 && now_ - longAt_ <= win;
  for (int m = 60; m < 128 && !longNote; ++m) {
    bool down = held_[m];
    for (int d = 0; d < 3; ++d) down |= heldOn_[d][m];
    longNote = down && m != midi && now_ - pressAt_[m] >= long(fs_);
  }
  if (midi >= 60 && (groupAt_ < 0 || now_ - groupAt_ > long(0.02 * fs_))) {
    if (groupTop_ >= 0) prevHigh_ = groupTop_;
    groupAt_ = now_;
    groupTop_ = -1;
  }
  double t = 0;
  if (kOn > 0 && midi >= 60 && longNote && prevHigh_ >= 0 && midi > prevHigh_) {
    double sum = 0, sum2 = 0, n = 0, act = 0;
    for (auto& r : recent_) {
      if (now_ - r.first <= long(2 * fs_)) ++act;
      if (r.second < 60) continue;
      sum += r.second; sum2 += double(r.second) * r.second; ++n;
    }
    const double mean = n > 0 ? sum / n : midi, sd = n > 1 ? std::sqrt(std::max(0.0, sum2 / n - mean * mean)) : 0;
    const double leap = std::clamp((midi - prevHigh_) / 12.0, 0.0, 1.0);          // (an octave: all)
    const double exposed = std::clamp((midi - mean) / std::max(sd, 2.0) / 2, 0.0, 1.0);  // (two spreads above: all)
    const double quiet = std::clamp(1 - act / 2 / 6.0, 0.0, 1.0);                   // (6 notes a second: busy)
    t = kOn * std::clamp(0.2 + 0.4 * leap + 0.4 * exposed, 0.0, 1.0) * (0.4 + 0.6 * quiet);
  }
  recent_.push_back({now_, midi});
  if (midi >= 60) groupTop_ = std::max(groupTop_, midi);
  if (std::getenv("PJ_ORGAN_TOUCHDBG") && t > 0)
    say(stderr, "touch %.2f s key %d (previous %d): %.2f\n", now_ / fs_, midi, prevHigh_, t);
  return t;
}

void OrganModel::noteReleased(int midi) {
  if (midi >= 60 && now_ - pressAt_[midi] >= long(fs_)) longAt_ = now_;
}

void OrganModel::open(int midi) {
  if (midi < 0 || midi > 127) return;
  held_[midi] = true;
  touch_[midi] = float(liveTouch(midi));
  pressAt_[midi] = now_;
  for (int d = 0; d < 4; ++d) drawPallet(d, midi, true);
  refreshKeys();
}

void OrganModel::close(int midi) {
  if (midi < 0 || midi > 127) return;
  held_[midi] = false;
  noteReleased(midi);
  for (int d = 0; d < 4; ++d) drawPallet(d, midi, false);
  refreshKeys();
}

void OrganModel::pedalUpdate() { refreshKeys(); }

void OrganModel::openOn(int division, int midi, double touch) {
  if (division < 0 || division > 3 || midi < 0 || midi > 127) return;
  heldOn_[division][midi] = true;
  touch_[midi] = float(std::clamp(std::max(touch, division == 3 ? 0.0 : liveTouch(midi)), 0.0, 1.0));
  pressAt_[midi] = now_;
  for (int d = 0; d < 4; ++d) drawPallet(d, midi, true);  // (its own and the coupled divisions' pallets)
  refreshKeys();
}

void OrganModel::closeOn(int division, int midi) {
  if (division < 0 || division > 3 || midi < 0 || midi > 127) return;
  heldOn_[division][midi] = false;
  if (division != 3) noteReleased(midi);
  for (int d = 0; d < 4; ++d) drawPallet(d, midi, false);
  refreshKeys();
}

void setOrganDisposition(int d) { gDisposition = d; }
int organDisposition() { return gDisposition; }
void setOrganTemperament(OrganTemperament t) { gTemperament = t; }
double temperamentCentsOf(OrganTemperament t, int midi) { return temperamentOf(t, midi); }
double organTuningLookAhead() { return gTemperament == kJyrkiHeuristic ? knob("PJ_JI_AHEAD_MS", 150) * 1e-3 : 0.0; }
const char* organDispositionName() { return gDisposition == 1 ? "after Janakkala, Porthan 1993" : "the large organ"; }
namespace {
const struct { const char* name; OrganTemperament t; const char* shown; } kTunings[] = {
    {"jyrki-spectral", kJyrkiSpectral, "jyrki-spectral (adaptive, by the pipes' partials)"},
    {"jyrki-heuristic", kJyrkiHeuristic, "jyrki-heuristic (adaptive, by an interval table)"},
    {"tet12", kTet12, "12-TET"},
    {"meantone", kMeantone, "quarter-comma meantone"},
    {"kirnberger3", kKirnberger3, "Kirnberger III"},
    {"neidhardt", kNeidhardt, "Neidhardt 1724, for a large town"},
    {"lehman", kLehman, "Lehman's Bach tuning"},
};
}  // namespace
bool organTuningByName(const std::string& name, OrganTemperament& t) {
  for (const auto& k : kTunings)
    if (name == k.name) {
      t = k.t;
      return true;
    }
  return false;
}
const char* organTuningName() {
  for (const auto& k : kTunings)
    if (k.t == gTemperament) return k.shown;
  return "";
}

void organSize(int& stops, int& ranks, int& pipes) {
  std::vector<std::string> seen;
  stops = ranks = pipes = 0;
  for (const Stop& st : kStops) {
    if (!ofThisOrgan(st)) continue;
    ++ranks;
    const std::string name = std::to_string(int(st.div)) + st.token;
    if (std::find(seen.begin(), seen.end(), name) == seen.end()) seen.push_back(name), ++stops;
    const int lo = std::max(OrganModel::kLo, st.fromKey), hi = std::min(st.div == kPed ? OrganModel::kLo + 31 : OrganModel::kHi, st.toKey ? st.toKey : 127);
    pipes += hi - lo + 1;
  }
}

void OrganModel::setRegistration(const std::string& list) {
  // (PJ_ORGAN_ONLY: these stops whatever a plan draws, to hear a stop alone through the church)
  if (const char* only = std::getenv("PJ_ORGAN_ONLY"))
    if (list != only) {
      setRegistration(only);
      return;
    }
  std::string stops = list;
  for (const Preset& pr : kPresets)  // (presets expand, as in PJ_ORGAN_STOPS)
    for (size_t at; (at = (',' + stops + ',').find(',' + std::string(pr.name) + ',')) != std::string::npos;)
      stops.replace(at, std::strlen(pr.name), pr.stops);
  std::vector<std::string> toks;
  for (size_t at = 0; at <= stops.size();) {
    const size_t e = std::min(stops.find(',', at), stops.size());
    if (e > at) toks.push_back(stops.substr(at, e - at));
    at = e + 1;
  }
  auto has = [&](const char* t) { return std::find(toks.begin(), toks.end(), t) != toks.end(); };
  for (size_t i = 0; i < std::size(kStops); ++i) drawn_[i] = has(kStops[i].token);
  const char* names[4] = {"hw", "sw", "pos", "ped"};
  for (int f = 0; f < 4; ++f)
    for (int t = 0; t < 4; ++t) coupler_[f][t] = f != t && has((std::string(names[f]) + "-" + names[t]).c_str());
  wind_.setTremulant(kSW, has("tremsw"), knob("PJ_TREM_RATE", 5.5), knob("PJ_TREM_DEPTH", 8));
  wind_.setTremulant(kPos, has("trempos"), knob("PJ_TREM_RATE", 5.5) * 1.05, knob("PJ_TREM_DEPTH", 8) * 0.7);
  for (const std::string& t : toks) {
    bool known = t == "tremsw" || t == "trempos";
    for (const Stop& st : kStops) known |= t == st.token;
    for (int f = 0; f < 4; ++f)
      for (int d = 0; d < 4; ++d) known |= t == std::string(names[f]) + "-" + names[d];
    if (!known) std::fprintf(stderr, "organ: no stop or coupler %s\n", t.c_str());
  }
  moveSecs_ = knob("PJ_ORGAN_SLIDER", 0.08);  // (stops drawn under held keys: the sliders move, not the pallets)
  refreshKeys();
  moveSecs_ = 0;
}

// A flue pipe about to speak, silent until now: its modes set vibrating as
// the field at its mouth (its enclosure's: the other pipes sounding there)
// already drives them: the partials a sounding octave or fifth has put in the
// air give it a head start, in step with them (PJ_ORGAN_SYMP: 0 none).
void OrganModel::headStart(Pipe& pp) {
  // (set the modes at the key: kinked the sound as the jet came in, ~25 ms
  // on; replaced by the field driving the modes at the start, FluePipe's
  // sympDrive. PJ_ORGAN_PRESET=1 brings the old way back, for comparison.)
  static const double kSymp = knob("PJ_ORGAN_PRESET", 0.0);
  if (kSymp <= 0 || pp.p.reed || pp.p.awake()) return;
  const int n = pp.p.f.modes();
  double hz[32];
  std::complex<double> field[32];
  for (int i = 0; i < n && i < 32; ++i) hz[i] = pp.p.f.modeHz(i);
  enc_[pp.enc].fieldAt(pp.src, hz, std::min(n, 32), field);
  // (and the whole organ's sound as it fills the gallery: through the case,
  // off the walls and the vault; the enclosures each hear the others there)
  static const double kGallery = knob("PJ_ORGAN_SYMP_GALLERY", 0.5);
  {
    const int N = int(organHist_.size());
    double wsum = 0;
    for (int k = 0; k < n && k < 32; ++k) {
      std::complex<double> acc = 0, ph = std::polar(1.0, -2 * M_PI * hz[k] / fs_ * (N - 1));
      const std::complex<double> rot = std::polar(1.0, 2 * M_PI * hz[k] / fs_);
      double ws = 0;
      for (int t = 0; t < N; ++t) {
        const double w = 0.5 - 0.5 * std::cos(2 * M_PI * (t + 0.5) / N);
        acc += w * double(organHist_[(organPos_ + t) % N]) * std::conj(ph);
        ph *= rot;
        ws += w;
      }
      wsum = ws;
      field[k] += kGallery * acc * (2 / wsum);
    }
  }
  pp.p.f.preExcite(field, kSymp * pp.hear * kSympScale);

}

// Every pipe sounds when its key is down (a pedal pipe: its key the bass
// line's) and its stop is drawn: a stop drawn while keys are held speaks at
// once on them, one retired falls silent (the slider moving under open
// pallets).
void OrganModel::refreshKeys() {
  int lowest = -1;
  for (int m = 0; m < 128 && lowest < 0; ++m)
    if (held_[m] || (autoPedal_ && (heldOn_[0][m] || heldOn_[1][m] || heldOn_[2][m]))) lowest = m;
  for (int m = 0; m < 128; ++m) {
    for (int i : byKey_[m]) {
      Pipe& pp = pipes_[i];
      // (a key held for every manual; or on its own division's keys, or on a
      // division it is coupled to)
      bool keyDown = held_[m] || heldOn_[pp.div][m];
      for (int d = 0; d < 4 && !keyDown; ++d) keyDown = coupler_[pp.div][d] && heldOn_[d][m];
      const bool want = keyDown && drawn_[pp.stop];
      if (want != pp.keyed) {
        pp.keyed = want;
        if (want) headStart(pp);
        want ? pp.p.open(moveSecs_ > 0 ? moveSecs_ : palletOpen_[pp.div][m]) : pp.p.close(moveSecs_ > 0 ? moveSecs_ : palletClose_[pp.div][m]);
      }
    }
    const bool pedalKey = (held_[m] && (!manualRanks_ || (m == lowest && m < split_))) ||
                          (autoPedal_ && m == lowest && m < split_);
    for (int i : byPedal_[m]) {
      Pipe& pp = pipes_[i];
      const bool want = (pedalKey || heldOn_[3][m]) && drawn_[pp.stop];
      if (want != pp.keyed) {
        pp.keyed = want;
        if (want) headStart(pp);
        want ? pp.p.open(moveSecs_ > 0 ? moveSecs_ : palletOpen_[pp.div][m]) : pp.p.close(moveSecs_ > 0 ? moveSecs_ : palletClose_[pp.div][m]);
      }
    }
  }
}

// The crescendo roller (the Walze): from silence through ppp .. fff, each
// stop drawn at its point of the roller, softest first, the swell boxes
// opening on the way (closed at ppp, fully open from mf); a stop drawn while
// keys are held speaks on them at once.
namespace {
struct RollerStep {
  const char* token;
  double at;  // (0..1: the roller's position where it is drawn)
};
std::vector<RollerStep> rollerSteps() {
  // (each dynamic its band of the roller, its stops spread evenly over it;
  // ff the Hauptwerk's and the Pedal's reed chorus and the high mixtures, fff
  // the swell's reed battery 16-8-4 whole, as the French crescendo ends, with
  // the 16', 32' and the chamade: a reed added under a full chorus is what
  // still adds; another flute or mutation there adds nothing)
  const std::vector<std::vector<const char*>> levels = {
      {"gedackt8", "subbass16"},                                                         // ppp
      {"gamba8", "flute4", "salicional8"},                                               // pp (no céleste: it beats)
      {"rohrgedackt8", "gedacktflote4", "gemshorn8", "gedacktbass8"},                    // p
      {"rohrflote8", "fluteharm8", "bourdon16", "swprincipal8", "octavbass8"},           // mp
      {"principal8", "posprincipal8", "octave4", "posoctave4", "swoctave4", "blockflote4", "principalbass16",
       "violonbass16"},                                                                  // mf
      {"principal16", "superoctave2", "quinte223", "posoctave2", "waldflote2", "nasard223", "mixtur5", "pleinjeu4",
       "choralbass4", "pedmixtur4"},                                                     // f
      {"oboe8", "trompete8", "cymbel3", "scharf4", "terz135", "sesquialtera", "quinte113", "sifflote1", "cromorne8",
       "posaune16", "pedtrompete8", "untersatz32"},                                      // ff
      {"fagott16", "trompette8", "clairon4", "trompete16", "dulcian16", "klarine4", "contraposaune32",
       "chamade8"},                                                                      // fff
  };
  // (PJ_ROLLER_OMIT: stops left off the roller, to hear what each adds)
  const char* omit = std::getenv("PJ_ROLLER_OMIT");
  auto omitted = [omit](const char* t) {
    if (!omit) return false;
    const std::string s = std::string(",") + omit + ",";
    return s.find(std::string(",") + t + ",") != std::string::npos;
  };
  std::vector<RollerStep> out;
  for (size_t l = 0; l < levels.size(); ++l)
    for (size_t k = 0; k < levels[l].size(); ++k)
      if (!omitted(levels[l][k])) out.push_back({levels[l][k], 0.04 + 0.12 * (l + double(k) / levels[l].size())});
  return out;
}
}  // namespace

const char* OrganModel::dynamicName(double x) {
  static const char* const kNames[] = {"ppp", "pp", "p", "mp", "mf", "f", "ff", "fff"};
  return x < 0.04 ? "silence" : kNames[std::clamp(int((x - 0.04) / 0.12), 0, 7)];
}

void OrganModel::setRoller(double x) {
  x = std::clamp(x, 0.0, 1.0);
  if (std::fabs(x - roller_) < 1e-4) return;
  roller_ = x;
  static const std::vector<RollerStep> steps = rollerSteps();
  bool changed = false;
  for (const RollerStep& r : steps) {
    const bool on = x >= r.at;
    for (size_t i = 0; i < std::size(kStops); ++i)
      if (std::strcmp(kStops[i].token, r.token) == 0 && drawn_[i] != on) {
        drawn_[i] = on;
        changed = true;
      }
  }
  if (changed) {
    moveSecs_ = knob("PJ_ORGAN_SLIDER", 0.08);
    refreshKeys();
    moveSecs_ = 0;
  }
  // The swell boxes: closed at the start of ppp, opening evenly to mf's end.
  setSwell(0.03 + 0.97 * std::clamp((x - 0.04) / 0.56, 0.0, 1.0));
}

void OrganModel::setSwell(double s) {
  for (int e : {kSwellC, kSwellCis}) enc_[e].setOpening(s);
}

bool OrganModel::asleep() const {
  for (const Pipe& p : pipes_)
    if (p.p.awake()) return false;
  for (const Enclosure& e : enc_)
    if (!e.quiet()) return false;
  return true;
}

void OrganModel::resetAir() {
  Tube::resetAir();
  for (Pipe& p : pipes_) {
    p.p.reset();
    p.y = 0;
  }
  for (Enclosure& e : enc_) e.reset();
  if (paths_) nave_.reset();
  if (naveOn_) late_.reset();
  blockPos_ = kBlock;
}

// The enclosures' places in the church (src/organroom.h's coordinates: the
// west gallery's floor at 7 m, the main case's front 3 m from the west wall),
// their sizes, their cases.
namespace {
struct EncPlace {
  OrganRoom::V3 centre, dims;
  int caseId;
};
const EncPlace kPlace[kEnclosures] = {
    {{-2.3, 14.6, 2.1}, {2.6, 2.4, 1.6}, OrganRoom::kSwellCaseC},   {{2.3, 14.6, 2.1}, {2.6, 2.4, 1.6}, OrganRoom::kSwellCaseCis},
    {{-0.6, 8.6, 5.0}, {1.1, 1.4, 0.7}, OrganRoom::kPosCase},       {{0.6, 8.6, 5.0}, {1.1, 1.4, 0.7}, OrganRoom::kPosCase},
    {{-1.0, 11.8, 2.3}, {2.0, 3.0, 1.4}, OrganRoom::kMainCase},     {{1.0, 11.8, 2.3}, {2.0, 3.0, 1.4}, OrganRoom::kMainCase},
    {{-1.15, 10.9, 3.25}, {2.2, 4.0, 0.4}, OrganRoom::kNoCase},     {{1.15, 10.9, 3.25}, {2.2, 4.0, 0.4}, OrganRoom::kNoCase},
    {{-3.6, 10.2, 2.0}, {1.8, 6.0, 1.8}, OrganRoom::kPedCaseC},     {{3.6, 10.2, 2.0}, {1.8, 6.0, 1.8}, OrganRoom::kPedCaseCis},
};
}  // namespace

// Each pipe's mouth and top (a reed's resonator's top) a source of its own
// in the church; each enclosure's field one at its opening.
void OrganModel::buildPaths() {
  // (the listener: a main pair on a tall stand, as an organ is recorded: 4 m
  // high, 10 m into the nave: ~10 m from the main case, the direct sound ~4 dB
  // over a listener's in the pews at 15 m)
  nave_.setup(fs_, knob("PJ_ORGAN_LISTENER", 10), knob("PJ_ORGAN_LISTENER_X", 0), knob("PJ_ORGAN_LISTENER_Y", 4));
  static const double kPedOccl = knob("PJ_ORGAN_PEDAL_OCCL", 0.5);  // (the towers behind the case and the façade: darker)
  auto setOf = [](int e) { return e == kSwellC ? 1 : e == kSwellCis ? 2 : 0; };
  auto world = [](int e, const Vec3& at) {
    const OrganRoom::V3& c = kPlace[e].centre;
    return OrganRoom::V3{c.x + at.x, c.y + at.y, c.z + at.z};
  };
  auto occl = [&](const Pipe& pp) {
    const bool facade = pp.enc == kFacC || pp.enc == kFacCis;
    double o = facade ? 0.0 : std::min(0.6, 0.12 * kStops[pp.stop].row);
    if (pp.enc == kPedC || pp.enc == kPedCis) o += kPedOccl;
    return o;
  };
  for (Pipe& pp : pipes_) {
    const bool facade = pp.enc == kFacC || pp.enc == kFacCis;
    pp.rsrc = nave_.addSource(world(pp.enc, pp.at), kPlace[pp.enc].caseId, facade, occl(pp), pp.p.reed ? 0.0 : 0.8, setOf(pp.enc), pp.hz);
  }
  const int mouths = nave_.sources();
  for (Pipe& pp : pipes_)
    if (pp.p.openPipe()) {
      Vec3 top = pp.at;
      top.y += 0.95 * 343 / (2 * pp.hz);
      const bool facade = pp.enc == kFacC || pp.enc == kFacCis;
      pp.rtop = nave_.addSource(world(pp.enc, top), kPlace[pp.enc].caseId, facade, 0.5 * occl(pp), 0.0, setOf(pp.enc), pp.hz);
    }
  // The attack's puff, out of each flue's own mouth: at that pipe's place, with its own aim. The
  // mouths of a rank look mostly into the church, each a little its own way (+-55 degrees
  // across, +-20 up and down), so one puff finds the side wall and another the vault.
  puffPaths_ = knob("PJ_ORGAN_PUFFPATH", 1) != 0;
  if (puffPaths_) {
    static const double kLobe = knob("PJ_ORGAN_PUFFLOBE", 0.8);
    for (Pipe& pp : pipes_) {
      // (a reed: only the Posaune's speech has a place of its own: at its resonator's top, out at
      // the tower's front, in the open, where the side walls and the floor can see it)
      int from = pp.rsrc;
      if (pp.p.reed) {
        if (!pp.speech.on) continue;
        Vec3 top = pp.at;
        top.y += 0.95 * 343 / (2 * pp.hz);
        OrganRoom::V3 w = world(pp.enc, top);
        w.z = nave_.caseBox(kPlace[pp.enc].caseId).hi.z + 0.05;
        from = nave_.addSource(w, OrganRoom::kNoCase, false, 0.25, 0.0, setOf(pp.enc), pp.hz);
      }
      uint32_t h = uint32_t(pp.stop + 1) * 2654435761u ^ uint32_t(pp.key + 1) * 40503u ^ uint32_t(pp.enc + 1) * 769u;
      auto draw = [&] { h = h * 1664525u + 1013904223u; return 2 * ((h >> 8) / 16777216.0) - 1; };
      const double yaw = draw() * 55 * M_PI / 180, pitch = draw() * 20 * M_PI / 180;
      pp.rpuff = nave_.addAimed(from, {std::sin(yaw) * std::cos(pitch), std::sin(pitch), std::cos(yaw) * std::cos(pitch)}, kLobe);
    }
  }
  zoneNow_.assign(enc_.size(), {});
  zoneIn_.assign(enc_.size(), {});
  zonePtr_.assign(enc_.size(), nullptr);
  repointZones();
  regionSrc_.assign(enc_.size(), -1);
  for (int e = 0; e < int(enc_.size()); ++e) {
    const EncPlace& P = kPlace[e];
    double y = P.centre.y;
    if (e == kFacC || e == kFacCis) y -= 1.4;
    if (e == kHwC || e == kHwCis) y -= 0.6;
    if (e == kPedC || e == kPedCis) y -= 1.8;
    const double zf = P.caseId >= 0 ? nave_.caseBox(P.caseId).hi.z : P.centre.z + P.dims.z / 2;
    regionSrc_[e] = nave_.addSource({P.centre.x, y, zf + 0.05}, OrganRoom::kNoCase, false, 0, 0, setOf(e));
  }
  nave_.finish(mouths);
  if (std::getenv("PJ_ORGAN_TAPLOG"))
    for (const Pipe& pp : pipes_)
      if (pp.enc == kPedC || pp.enc == kPedCis) {
        const auto m = nave_.tapSum(pp.rsrc);
        const auto t = pp.rtop >= 0 ? nave_.tapSum(pp.rtop) : std::array<double, 2>{};
        std::fprintf(stderr, "tap %s %d mouth %.3f %.3f top %.3f %.3f y %.2f\n", kStops[pp.stop].token, pp.key, m[0], m[1], t[0], t[1], pp.at.y);
        if (pp.rpuff >= 0)
          std::fprintf(stderr, "puffaim %s %d mouth L/R %.2f puff L/R %.2f dB, level %+.2f dB\n", kStops[pp.stop].token, pp.key,
                       20 * std::log10(nave_.tapGain(pp.rsrc, 0) / nave_.tapGain(pp.rsrc, 1)), 20 * std::log10(nave_.tapGain(pp.rpuff, 0) / nave_.tapGain(pp.rpuff, 1)),
                       20 * std::log10((nave_.tapGain(pp.rpuff, 0) + nave_.tapGain(pp.rpuff, 1)) / (nave_.tapGain(pp.rsrc, 0) + nave_.tapGain(pp.rsrc, 1))));
      }
  encField_.assign(enc_.size(), {});
}

// One microphone's block: the pipes sounding in the last block and the
// enclosures' fields scattered into its rings, read out through the classes'
// filters (the swell's rings through its shutters).
void OrganModel::scatterPaths(int mic) {
  for (const Pipe& pp : pipes_) {
    if (!pp.live) continue;
    nave_.scatter(pp.rsrc, mic, pp.blk);
    if (pp.rtop >= 0) nave_.scatter(pp.rtop, mic, pp.blkTop);
    if (pp.puffLive) nave_.scatter(pp.rpuff, mic, pp.blkPuff);
  }
  for (size_t e = 0; e < enc_.size(); ++e) nave_.scatter(regionSrc_[e], mic, encField_[e].data());
  nave_.endBlock(mic, roomOut_[mic]);
}

void OrganModel::takeEarly(int n, float* l, float* r, float g) {
  const int have = int(earlyL_.size()), m = std::min(n, have);
  const float ge = g;
  for (int i = 0; i < m; ++i) {
    l[n - m + i] += ge * earlyL_[have - m + i];
    r[n - m + i] += ge * earlyR_[have - m + i];
  }
  earlyL_.clear();
  earlyR_.clear();
  if (naveOn_) {
    // (the late field as the shared room's was: its microphones' gain (in g)
    // and the player's velocity, not the instrument's gain and level trim)
    const int hl = int(lateL_.size()), ml = std::min(n, hl);
    const float gl = g * lateTrim_ * naveGain_;
    for (int i = 0; i < ml; ++i) {
      l[n - ml + i] += gl * lateL_[hl - ml + i];
      r[n - ml + i] += gl * lateR_[hl - ml + i];
    }
    lateL_.clear();
    lateR_.clear();
  }
}

// The organ's own nave (src/organnave.h): a 50 x 18.3 x 21.5 m stone church
// (PJ_NAVE_VOLUME, the same proportions), its T60 per octave (PJ_NAVE_T60_63
// .. PJ_NAVE_T60_16K, s; all scaled by PJ_NAVE_T60); the late field from
// PJ_NAVE_ONSET_MS after the direct sound. Its level is physical against the
// pipes' early paths (both re the organ at 1 m: 1/r and 16 pi / A), and both
// reach the output through the room's microphone gain (src/ensemble.cc), so
// their balance stays the physical one (PJ_NAVE_GAIN: dB on the late field
// only; PJ_ORGAN_EARLY_DB the early's).
void OrganModel::setupNave() {
  naveOn_ = knob("PJ_NAVE", 1) != 0;
  if (!naveOn_) return;
  OrganNave::Params np;
  const double v = std::max(100.0, knob("PJ_NAVE_VOLUME", np.volume)), sc = std::cbrt(v / np.volume);
  np.surface *= sc * sc;
  np.volume = v;
  static const char* const kT60[OrganNave::kBands] = {"PJ_NAVE_T60_63", "PJ_NAVE_T60_125", "PJ_NAVE_T60_250",
                                                      "PJ_NAVE_T60_500", "PJ_NAVE_T60_1K", "PJ_NAVE_T60_2K",
                                                      "PJ_NAVE_T60_4K", "PJ_NAVE_T60_8K", "PJ_NAVE_T60_16K"};
  const double t60 = knob("PJ_NAVE_T60", 1.0);
  for (int b = 0; b < OrganNave::kBands; ++b) np.t60[b] = std::max(0.1, t60 * knob(kT60[b], np.t60[b]));
  np.onsetMs = knob("PJ_NAVE_ONSET_MS", np.onsetMs);
  np.latency = kBlock;  // (fed a block late)
  np.lensDepth = knob("PJ_NAVE_LENS", 0.0);
  np.lensSecs = knob("PJ_NAVE_LENS_SECS", np.lensSecs);
  late_.setup(fs_, np);
  if (puffPaths_) late_.setZones(int(enc_.size()));
  naveFb_ = float(knob("PJ_NAVE_FB", 1.0));
  lateTrim_ = float(1 / (inst_.gain * levelTrim(inst_)));
  naveGain_ = float(std::pow(10.0, knob("PJ_NAVE_GAIN", 0.0) / 20));
}

// One block of the nave: the organ's last block in; the microphones' late
// field and the pressure at the case for the next.
void OrganModel::repointZones() {
  for (size_t e = 0; e < zoneIn_.size(); ++e) zonePtr_[e] = zoneIn_[e].data();
}

void OrganModel::runNave() {
  std::fill(naveL_, naveL_ + kBlock, 0.0f);
  std::fill(naveR_, naveR_ + kBlock, 0.0f);
  std::fill(naveP_, naveP_ + kBlock, 0.0f);
  late_.process(naveIn_, kBlock, naveL_, naveR_, naveP_, puffPaths_ ? zonePtr_.data() : nullptr);
}

// One enclosure's block: its sounding pipes (each the field at its mouth,
// its neighbours' last samples, its chest's wind as the block began), into
// it; its sound at the opening.
void OrganModel::renderEnclosure(int e) {
  Enclosure& enc = enc_[e];
  std::array<double, 4>& demand = encDemand_[e];
  demand.fill(0);
  bool any = false;
  for (int i : encPipes_[e]) {
    any |= pipes_[i].p.awake();
    pipes_[i].live = false;
    pipes_[i].puffLive = false;
  }
  if (puffPaths_) zoneNow_[e].fill(0.0f);
  if (!any && enc.quiet()) {
    encOut_[e].fill(0.0f);
    if (paths_) encField_[e].fill(0.0f);
    return;
  }
  for (int s = 0; s < kBlock; ++s) {
    for (int i : encPipes_[e]) {
      Pipe& pp = pipes_[i];
      if (!pp.p.awake()) {
        pp.y = 0;
        pp.blk[s] = pp.blkTop[s] = 0;
        continue;
      }
      pp.live = true;
      float heard = enc.heard(pp.src);
      for (int c = 0; c < 4 && pp.near[c] >= 0; ++c) heard += pp.nearG[c] * pipes_[pp.near[c]].y;
      float y = 0, top = 0, puff = 0;
      const float wind = windOn_ ? windNow_[pp.div] : 1.0f;
      for (int k = 0; k < pp.os; ++k) {
        y += pp.p.sample(wind, pp.hear * heard);
        top += pp.p.top();
        puff += pp.p.puff();
      }
      pp.y = y / float(pp.os);
      if (pitchLog_) {
        Pipe::PitchLog& L = pp.log;
        // (two band-passes: a reed's harmonics, up to 20 dB over its
        // fundamental in the bass, through one moved its zero crossings)
        const double v1 = L.b0 * (pp.y - L.x2) + L.a1 * L.y1 + L.a2 * L.y2;
        const double v = L.b0 * (v1 - L.y2) + L.a1 * L.z1 + L.a2 * L.z2;
        L.x2 = L.x1;
        L.x1 = pp.y;
        L.y2 = L.y1;
        L.y1 = v1;
        L.z2 = L.z1;
        L.z1 = v;
        if (pp.keyed) {
          ++L.since;
          ++L.sounding;
          if (L.prev < 0 && v >= 0 && L.since > long(std::max(0.15, 20 / pp.target) * fs_)) {  // (once spoken)
            const double t = double(L.sounding) - v / (v - L.prev);
            if (L.lastCross >= 0) {
              // (against where it is meant to be now: its just offset too)
              const double c = 1200 * std::log2(fs_ / (t - L.lastCross) / pp.target) - (ji_ ? jiNow_[pp.key] : 0.0) - climateOf(pp);
              if (std::fabs(c) < 200) {
                L.sum += c;
                L.sum2 += c * c;
                L.lo = std::min(L.lo, c);
                L.hi = std::max(L.hi, c);
                ++L.n;
              }
            }
            L.lastCross = t;
          }
        } else {
          L.since = 0;
          L.lastCross = -1;
        }
        L.prev = v;
      }
      demand[pp.div] += pp.flow * std::sqrt(std::max(pp.p.foot(), 0.0f));
      // (a reed's resonator radiates from its top, above the chest: its sound
      // into the field the flue jets hear a part only; all of it, a loud
      // reed's pushed the flues near it off their oscillation, quieter)
      static const float kReedField = float(knob("PJ_REED_FIELD", 0.3));
      float out = pp.y;
      if (pp.caseG < 1) {  // (the low shelf passes, the rest pp.caseG of itself)
        pp.caseLp += pp.caseA * (out - pp.caseLp);
        pp.caseLp2 += pp.caseA * (pp.caseLp - pp.caseLp2);
        out = pp.caseLp2 + pp.caseG * (out - pp.caseLp2);
      }
      if (pp.speech.on) out = pp.speech.process(out, pp.keyed);
      enc.add(pp.src, pp.gain * out, pp.p.reed ? kReedField : 1.0f);
      if (pp.srcTop >= 0) enc.add(pp.srcTop, pp.gain * top / float(pp.os));
      pp.blk[s] = pp.gain * out;
      pp.blkTop[s] = pp.gain * top / float(pp.os);
      if (pp.rpuff >= 0) {  // (the puff apart from the tone, on the paths its mouth's aim gives it)
        static const float kSpeechOpen = float(knob("PJ_POSAUNE_OPEN", 0.6));  // (how much of its speech leaves from the open)
        const float pf = pp.p.reed ? pp.gain * out * kSpeechOpen * pp.speech.e : pp.gain * puff / float(pp.os);
        pp.blkPuff[s] = pf;
        pp.blk[s] -= pf;
        if (pf != 0) pp.puffLive = true;
        if (!pp.p.reed && e != kSwellC && e != kSwellCis) {  // (a swell's goes through its shutters, in the common feed)
          static const float kOut = float(knob("PJ_ORGAN_OUT", 0.05));  // (the nave's feed is the output's scale)
          zoneNow_[e][s] += kOut * pf;
        }
      }
    }
    encOut_[e][s] = enc.step(church_[s]);
    if (paths_) encField_[e][s] = enc.lastField();
  }
}

namespace {
// The just intervals by interval class (semitones mod 12): the offset from
// equal temperament (cents) and how much it matters (src/ensemble.cc's).
struct JustIv { double cents, weight; };
constexpr JustIv kJustIv[12] = {{0.0, 1.0},   {11.7, 0.1},  {3.9, 0.15},  {15.6, 0.4}, {-13.7, 0.5}, {-2.0, 0.6},
                                {-9.8, 0.05}, {2.0, 0.8},   {13.7, 0.35}, {-15.6, 0.45}, {-31.2, 0.3}, {-11.7, 0.1}};
}  // namespace

// The held keys' offsets: each pair pulled to its just interval (weaker
// across octaves), a new key pulled a little to equal temperament, a
// sounding one held near where it is (at most 3 c a solve), the new ones'
// mean to 0 c; healing to 0 c while the chord stays (every 0.1 s, the
// anchors x exp(-0.1 / 3 s)); +-35 c.
void OrganModel::justSolve() {
  int note[64], n = 0;
  long long sig = 0;
  for (int m = 0; m < 128 && n < 64; ++m) {
    bool down = held_[m];
    for (int d = 0; d < 4; ++d) down |= heldOn_[d][m];
    if (down) {
      if (jiSince_[m] < 0) jiSince_[m] = now_;
      note[n++] = m;
      sig = sig * 1000003 + m + 1;
    } else {
      jiSince_[m] = -1;
      jiTarget_[m] = 0;
    }
  }
  // (jyrki-heuristic in -play: a key landing now is tuned against what will sound just after it: its
  // own and the other new keys, and the keys the score has on then. Keys about to be let go do not
  // pull it; keys about to come do, as partners only, each tuned when it lands.)
  bool virt[128] = {};
  if (gTemperament == kJyrkiHeuristic && jiAheadN_ > 0 && now_ - jiAheadAt_ < long(0.04 * fs_)) {
    bool ahead[128] = {};
    for (int q = 0; q < jiAheadN_; ++q) ahead[jiAhead_[q] & 127] = true;
    int k = 0, list[64];
    for (int m = 0; m < 128 && k < 64; ++m) {
      bool down = held_[m];
      for (int d = 0; d < 4; ++d) down |= heldOn_[d][m];
      const bool landing = down && now_ - jiSince_[m] < long(0.04 * fs_);
      if (!landing && !ahead[m]) continue;
      list[k++] = m;
      virt[m] = !down;
      sig = sig * 1000003 + m + 500;
    }
    if (k > 0) {
      std::copy(list, list + k, note);
      n = k;
    }
  }
  const bool same = sig == jiSig_;
  if (same && now_ - jiSolved_ < long(0.1 * fs_)) return;
  const double decay = same ? std::exp(-double(now_ - jiSolved_) / fs_ / 3.0) : 1.0;
  jiSig_ = sig;
  jiSolved_ = now_;
  if (n == 0) return;
  double A[64][64] = {}, b[64] = {}, c[64], lim[64];
  // Each key's range: +-30 c; a pedal key's +-15 (the bass's beats slow, its
  // pitch the harmony's floor); the highest key's +-10 (the melody: heard
  // alone on top, a drift in it heard as out of tune).
  static const double kLim = knob("PJ_JI_RANGE", 30), kLimPed = knob("PJ_JI_RANGE_PEDAL", 15), kLimTop = knob("PJ_JI_RANGE_TOP", 10);
  for (int i = 0; i < n; ++i) {
    const int m = note[i];
    lim[i] = kLim;
    if (heldOn_[3][m] || pedalOn_[m]) lim[i] = std::min(lim[i], kLimPed);
    if (i == n - 1 && n > 1) lim[i] = std::min(lim[i], kLimTop);
  }
  bool young[64];
  int ny = 0;
  for (int i = 0; i < n; ++i) {
    // (free to take its place only before it has spoken: a chord's keys landing together)
    young[i] = virt[note[i]] || now_ - jiSince_[note[i]] < long(0.04 * fs_);
    ny += young[i];
  }
  for (int i = 0; i < n; ++i) {
    const double l = young[i] ? 0.05 : 1.0, a = young[i] ? 0.0 : jiTarget_[note[i]] * decay;
    A[i][i] += l;
    b[i] += l * a;
  }
  if (ny > 0)
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < n; ++j)
        if (young[i] && young[j]) A[i][j] += 4.0 / (double(ny) * ny);
  // Each pair's pull: by the spectra (the default; PJ_JI_SPECTRAL=0: by the
  // interval table alone): every two partials of theirs within 36 c of each other beat, and pull the interval to where
  // they meet, by the weaker one's level (the beating's depth); the keys' spectra their sounding
  // pipes' modelled spectra (each harmonic's level in the pipe's own output,
  // read from the model once at calibration and cached: a Gedackt's odd partials only, a Nasard's at 3x, a Terz's at 5x,
  // a mixture's ranks). Else (or with no partials met) the interval table's.
  const bool kSpectral = knob("PJ_JI_SPECTRAL", 1) != 0 && gTemperament == kJyrkiSpectral;  // (by the spectra; else the interval table alone)
  static const double kSpecW = knob("PJ_JI_SPECTRAL_W", 1.0);
  std::vector<std::vector<std::pair<double, double>>> spec(n);  // per key: (log2 of the partial over the key's 8' pitch, strength)
  if (kSpectral)
    for (int i = 0; i < n; ++i) {
      const int m = note[i];
      const double keyHz = 440 * std::exp2((m - 69) / 12.0);
      auto addPipe = [&](const Pipe& pp) {
        if (!pp.keyed) return;
        for (int q = 0; q < int(pp.spectrum.size()); ++q) {
          const int k = q + 1;
          double w = pp.spectrum[q] * pp.gain;
          if (pp.caseG < 1) {  // (as the nave hears it: through the case's shelf)
            const double r = pp.target * k / pp.caseHz;
            w *= pp.caseG + (1 - pp.caseG) / (1 + r * r);
          }
          const double hz = pp.target * k;
          if (w <= 0 || hz > 8000) continue;
          spec[i].push_back({std::log2(hz / keyHz), w});
        }
      };
      for (int q : byKey_[m]) addPipe(pipes_[q]);
      for (int q : byPedal_[m]) addPipe(pipes_[q]);
      // (merged within 5 c; the strongest 32 kept; normalised to its power)
      std::sort(spec[i].begin(), spec[i].end());
      std::vector<std::pair<double, double>> mer;
      for (auto& p : spec[i])
        if (!mer.empty() && (p.first - mer.back().first) * 1200 < 5) mer.back().second += p.second;
        else mer.push_back(p);
      std::sort(mer.begin(), mer.end(), [](auto& a, auto& b) { return a.second > b.second; });
      if (mer.size() > 32) mer.resize(32);
      double pw = 0;
      for (auto& p : mer) pw += p.second * p.second;
      for (auto& p : mer) p.second /= std::sqrt(std::max(pw, 1e-30));
      spec[i] = mer;
      if (std::getenv("PJ_JI_SPECDBG")) {  // (its 32 strongest partials: frequency / the key's 8' pitch, level)
        std::fprintf(stderr, "spec key %d:", m);
        for (auto& p : spec[i]) std::fprintf(stderr, " %.3f:%.4f", std::exp2(p.first), p.second);
        std::fprintf(stderr, "\n");
      }
    }
  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j) {
      const int semis = note[j] - note[i];
      double w = 0, d = 0;
      if (kSpectral && !spec[i].empty() && !spec[j].empty()) {
        // (partials' offsets in cents, with the keys in equal temperament:
        // where j's partial lies against i's)
        double sw = 0, sd = 0;
        for (auto& a : spec[i])
          for (auto& bq : spec[j]) {
            // (near coincidences only, within 36 c, PJ_JI_SPECTRAL_WINDOW: the
            // beats a retuning of a few cents removes; partials farther apart are
            // rough, not beating, and pulled the interval the wrong way; the
            // keys' corrections themselves at most +-30 c)
            static const double kWin = knob("PJ_JI_SPECTRAL_WINDOW", 36);
            const double delta = 1200 * (bq.first + semis / 12.0 - a.first);
            if (std::fabs(delta) > kWin) continue;
            // (the beating's depth: the weaker partial's level)
            const double ww = std::min(a.second, bq.second) * std::exp(-(delta / 20) * (delta / 20));
            sw += ww;
            sd += ww * -delta;  // (c_j - c_i that makes them meet)
          }
        if (sw > 1e-6) {
          w = kSpecW * sw;
          d = sd / sw;
        }
      }
      if (w <= 0) {
        const JustIv& J = kJustIv[semis % 12];
        w = (kSpectral ? 0.1 : 1.0) * J.weight / (1 + 0.5 * (semis / 12));
        d = J.cents;
      }
      A[i][i] += w; A[j][j] += w; A[i][j] -= w; A[j][i] -= w;
      b[j] += w * d; b[i] -= w * d;
    }
  for (int k = 0; k < n; ++k)
    for (int r = k + 1; r < n; ++r) {
      const double f = A[r][k] / A[k][k];
      for (int cc = k; cc < n; ++cc) A[r][cc] -= f * A[k][cc];
      b[r] -= f * b[k];
    }
  for (int k = n - 1; k >= 0; --k) {
    double v = b[k];
    for (int jj = k + 1; jj < n; ++jj) v -= A[k][jj] * c[jj];
    c[k] = std::clamp(v / A[k][k], -lim[k], lim[k]);
  }
  for (int i = 0; i < n; ++i) {
    if (virt[note[i]]) {  // (not down yet: only a partner; the place worked out for it, for when it lands)
      jiPlan_[note[i]] = c[i];
      jiPlanAt_[note[i]] = now_;
      continue;
    }
    double t = c[i];
    if (young[i] && now_ - jiPlanAt_[note[i]] < long(0.25 * fs_)) t = jiPlan_[note[i]];
    // (a short note keeps the tuning it started with: its pitch is hard to place, a bend in it is not.
    // Only a key held past PJ_JI_SHORT_MS, 600, moves, and a little at a time)
    static const double kShortMs = knob("PJ_JI_SHORT_MS", 600);
    if (!young[i]) {
      const bool isShort = now_ - jiSince_[note[i]] < long(kShortMs * 1e-3 * fs_);
      t = isShort ? jiTarget_[note[i]] : std::clamp(t, jiTarget_[note[i]] - 3, jiTarget_[note[i]] + 3);
    }
    jiTarget_[note[i]] = std::clamp(t, -lim[i], lim[i]);
  }
}

// Each key's pipes glide to its offset: no glide quicker than 25 ms, none
// faster than 1 c a millisecond (a 12 c correction ~0.5 c a ms; a jump would
// click, a fast glide sounds like one); the rate set when the goal changes,
// from where the key is then.
void OrganModel::justGlide() {
  static const double kMinMs = knob("PJ_JI_GLIDEMS", 25), kMaxRate = knob("PJ_JI_GLIDERATE", 1.0);
  const double blockMs = kBlock / fs_ * 1000;
  for (int m = 0; m < 128; ++m) {
    const double want = jiSince_[m] >= 0 ? jiTarget_[m] : jiNow_[m];  // (a key let go keeps its tuning while it rings)
    if (std::fabs(want - jiNow_[m]) < 0.02) continue;
    if (want != jiGoal_[m]) {
      jiGoal_[m] = want;
      jiRate_[m] = std::min(kMaxRate, std::fabs(want - jiNow_[m]) / kMinMs);
    }
    const double step = std::max(jiRate_[m], 0.001) * blockMs;
    jiNow_[m] += std::clamp(want - jiNow_[m], -step, step);
    for (int i : byKey_[m]) retune(i);
    for (int i : byPedal_[m]) retune(i);
  }
}

// The air's speed of sound ~ sqrt(T) (+1 C at 20: +0.17 %, 2.95 c) and a
// little with the humidity (+0.0035 % a % RH at 20 C); applied a few times a
// second when it has moved 0.02 c. A reed's tongue, at the wind's temperature
// (its own motion warms it by nothing), softens and lengthens as it warms
// (~ -0.35 c a degree), its resonator's air pulling it up a little: a little
// flat on the whole, -0.1 of the flues' rise.
void OrganModel::climateStep() {
  static const double kDT = knob("PJ_ORGAN_WARMUP_C", 1.0), kDRH = knob("PJ_ORGAN_WARMUP_RH", 5.0),
                      kSecs = knob("PJ_ORGAN_WARMUP_SECS", 600), kReed = knob("PJ_ORGAN_WARMUP_REED", -0.1);
  if (now_ % (kBlock * 512) != 0) return;
  const double u = std::clamp(double(now_) / fs_ / std::max(kSecs, 1.0), 0.0, 1.0);
  const double flue = 1200 * std::log2(std::sqrt((293.15 + kDT * u) / 293.15) * (1 + 0.000035 * kDRH * u));
  double before[16];
  std::copy(encAir_, encAir_ + 16, before);
  airStep(double(now_) / fs_);
  bool moved = std::fabs(flue - climate_[0]) >= 0.02;
  for (size_t e = 0; e < enc_.size(); ++e) moved |= std::fabs(encAir_[e] - before[e]) * 2.95 >= 0.02;
  if (!moved) return;
  climate_[0] = flue;
  climate_[1] = kReed * flue;
  for (int i = 0; i < int(pipes_.size()); ++i) retune(i);
}

// The air events: a new one now and then (on average PJ_ORGAN_AIR_EVERY
// seconds apart, 180), each a region, a change (0.3 .. 1.5 degrees, warmer or
// colder), coming on over 15 .. 45 s, held 1 .. 4 min, gone over 2 .. 5 min;
// summed per enclosure and per nave line.
void OrganModel::airStep(double t) {
  static const double kOn = knob("PJ_ORGAN_AIR", 1.0), kEvery = knob("PJ_ORGAN_AIR_EVERY", 180);
  if (kOn <= 0) return;
  if (airNext_ < 0) airNext_ = t + kEvery * (0.3 + 0.7 * (0.5 + 0.5 * airRng_.uniform()));
  if (t >= airNext_) {
    AirEvent ev;
    ev.region = int((0.5 + 0.5 * airRng_.uniform()) * 4.999);
    const double mag = 0.3 + 1.2 * (0.5 + 0.5 * airRng_.uniform());
    ev.dT = kOn * (airRng_.uniform() < 0 ? -mag : mag);
    ev.t0 = t;
    ev.rise = 15 + 30 * (0.5 + 0.5 * airRng_.uniform());
    ev.hold = 60 + 180 * (0.5 + 0.5 * airRng_.uniform());
    ev.fall = 120 + 180 * (0.5 + 0.5 * airRng_.uniform());
    for (int j = 0; j < 8; ++j) {
      ev.line[j] = int((0.5 + 0.5 * airRng_.uniform()) * (OrganNave::lines() - 0.001));
      ev.share[j] = 0.3 + 0.7 * (0.5 + 0.5 * airRng_.uniform());
    }
    air_.push_back(ev);
    static const char* kWhere[5] = {"the main case and the Positiv", "the swell boxes", "the left side", "the right side", "the nave"};
    if (std::getenv("PJ_ORGAN_SAY"))
      say(stderr, "organ: at %.0f s %s air (%+.1f C) reaches %s\n", t, ev.dT > 0 ? "warm" : "cold", ev.dT, kWhere[ev.region]);
    // (the next, exponentially apart)
    airNext_ = t - kEvery * std::log(std::max(1e-3, 0.5 + 0.5 * airRng_.uniform()));
  }
  std::fill(encAir_, encAir_ + 16, 0.0);
  std::fill(lineAir_, lineAir_ + 64, 0.0);
  for (size_t i = 0; i < air_.size();) {
    const AirEvent& ev = air_[i];
    const double a = t - ev.t0;
    double w;
    if (a < ev.rise) w = 0.5 - 0.5 * std::cos(M_PI * a / ev.rise);
    else if (a < ev.rise + ev.hold) w = 1;
    else if (a < ev.rise + ev.hold + ev.fall) w = 0.5 + 0.5 * std::cos(M_PI * (a - ev.rise - ev.hold) / ev.fall);
    else {
      air_.erase(air_.begin() + i);
      continue;
    }
    const double d = w * ev.dT;
    auto add = [&](std::initializer_list<int> es) {
      for (int e : es) encAir_[e] += d;
    };
    switch (ev.region) {
      case 0: add({kHwC, kHwCis, kFacC, kFacCis, kPosC, kPosCis}); break;
      case 1: add({kSwellC, kSwellCis}); break;
      case 2: add({kSwellC, kPosC, kHwC, kFacC, kPedC}); break;
      case 3: add({kSwellCis, kPosCis, kHwCis, kFacCis, kPedCis}); break;
      default: break;
    }
    // (its part of the nave: a path through warmer air shorter, ~2.5 samples a degree over its share)
    for (int j = 0; j < 8; ++j) lineAir_[ev.line[j]] -= 2.5 * d * ev.share[j];
    ++i;
  }
  if (naveOn_)
    for (int k = 0; k < OrganNave::lines(); ++k) late_.setStatic(k, std::clamp(lineAir_[k], -3.0, 3.0));
}

void OrganModel::renderBlock() {
  if (ji_) {
    justSolve();
    justGlide();
  }
  climateStep();
  now_ += kBlock;
  for (int s = 0; s < kBlock; ++s) {
    if (roomPos_ < roomN_) pRoom_ = room_[roomPos_++];
    church_[s] = pRoom_ / float(outScale());
    if (naveOn_ && room_) church_[s] += naveFb_ * naveP_[s] / float(outScale());  // (its own nave's field)
  }
  for (int d = 0; d < 4; ++d) windNow_[d] = wind_.pressure(d);
  const int n = int(enc_.size());
  if (pool_) {
    pool_->run(n, [this](int e) { renderEnclosure(e); });
  } else if (sharedPool_) {
    TaskPool::shared().run(n, [this](int e) { renderEnclosure(e); });
  } else {
    for (int e = 0; e < n; ++e) renderEnclosure(e);
  }
  if (paths_) {  // (each pipe's own paths: the two microphones on two threads)
    for (int k : {1, 2}) {
      const Enclosure& sw = enc_[k == 1 ? kSwellC : kSwellCis];
      nave_.setShutter(k, sw.shutterA(), sw.shutterGain(), sw.leak());
    }
    const bool late = naveOn_ && room_;  // (its own nave: a third job)
    if (pool_) {
      pool_->run(late ? 3 : 2, [this](int m) { m < 2 ? scatterPaths(m) : runNave(); });
    } else if (sharedPool_) {
      TaskPool::shared().run(late ? 3 : 2, [this](int m) { m < 2 ? scatterPaths(m) : runNave(); });
    } else {
      scatterPaths(0);
      scatterPaths(1);
      if (late) runNave();
    }
    nave_.advance();
    if (puffPaths_) zoneIn_.swap(zoneNow_), repointZones();
  }
  // The wind: the block's flows, averaged, drawn sample by sample.
  double demand[4] = {0, 0, 0, 0};
  for (int e = 0; e < n; ++e)
    for (int d = 0; d < 4; ++d) demand[d] += encDemand_[e][d] / kBlock;
  for (int s = 0; s < kBlock; ++s) {
    for (int d = 0; d < 4; ++d) wind_.draw(d, demand[d]);
    wind_.step();
  }
}

float OrganModel::sample() {
  static const float g = float(knob("PJ_ORGAN_OUT", 0.05));
  if (blockPos_ == kBlock) {
    renderBlock();
    blockPos_ = 0;
  }
  const int bp = blockPos_++;
  // To the microphones: below ~300 Hz both hear each enclosure alike (the
  // bass's wavelengths over the gallery's width, the church's field
  // everywhere); its highs a few dB stronger on its side; the far one ~0.5 ms
  // later (the paths' difference).
  float l = 0, r = 0;
  const int w = micPos_;
  for (size_t i = 0; i < enc_.size(); ++i) {
    Mic& mc = mic_[i];
    const float y = encOut_[i][bp];
    mc.lo1 += xoverA_ * (y - mc.lo1);
    mc.lo2 += xoverA_ * (mc.lo1 - mc.lo2);
    const float hi = y - mc.lo2;
    mc.ringL[(w + mc.delayL) & 63] += mc.lo2 + mc.gL * hi;
    mc.ringR[(w + mc.delayR) & 63] += mc.lo2 + mc.gR * hi;
    l += mc.ringL[w];
    r += mc.ringR[w];
    mc.ringL[w] = mc.ringR[w] = 0;
  }
  micPos_ = (w + 1) & 63;
  const float mid = 0.5f * (l + r), sd = 0.5f * (l - r);
  organHist_[organPos_] = mid;  // (the whole organ's sound, for the pipes' head start)
  organPos_ = (organPos_ + 1) % int(organHist_.size());
  last_ = g * mid;
  side_.push_back(g * float(outScale()) * sd);
  if (paths_) {
    // (below the crossover both microphones hear the paths alike, as the mic
    // stage's lows: the bass's wavelengths over the pair's spacing)
    float e[2];
    for (int m = 0; m < 2; ++m) {
      const float y = roomOut_[m][bp];
      eLo1_[m] += xoverA_ * (y - eLo1_[m]);
      eLo2_[m] += xoverA_ * (eLo1_[m] - eLo2_[m]);
      e[m] = y - eLo2_[m];
    }
    const float lo = 0.5f * (eLo2_[0] + eLo2_[1]);
    // (the early sound's balance against the church's late field, as a
    // recording's main pair against its room: PJ_ORGAN_EARLY_DB)
    static const float kEarly = float(std::pow(10.0, knob("PJ_ORGAN_EARLY_DB", 0.0) / 20));
    if (earlyL_.size() >= 65536) {  // (nobody taking it: -seq, no room; keep the recent)
      earlyL_.erase(earlyL_.begin(), earlyL_.begin() + 32768);
      earlyR_.erase(earlyR_.begin(), earlyR_.begin() + 32768);
    }
    earlyL_.push_back(kEarly * g * float(outScale()) * (lo + e[0]));
    earlyR_.push_back(kEarly * g * float(outScale()) * (lo + e[1]));
    if (naveOn_ && room_) {  // (its own nave: the sound at 1 m in, Pa, as the room's feed was)
      naveIn_[bp] = last_;
      lateL_.push_back(float(outScale()) * naveL_[bp]);
      lateR_.push_back(float(outScale()) * naveR_[bp]);
    }
  }
  return finish(last_);
}

const float* OrganModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

OrganPlayer::OrganPlayer(OrganModel& m) : Performer(m), m_(m) {
  zones_ = m.zones();
  if (const char* e = std::getenv("PJ_ORGAN_SPLITS")) std::sscanf(e, "%d,%d", &split1_, &split2_);
  pedalUp_ = int(knob("PJ_ORGAN_PEDAL_UP", 12));
  std::fill(std::begin(keyDiv_), std::end(keyDiv_), -1);
}

void OrganPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  if (!zones_) {
    m_.open(midi);
    return;
  }
  // The zones: the pedal's keys (an octave up: the key's pitch the 16''s),
  // the left hand's (the Schwellwerk), the right hand's (the Hauptwerk or, soft,
  // the Positiv; the top octave folded into the manuals' compass).
  midi &= 127;
  zoneLevel();
  int div, key = midi;
  if (midi < split1_) {
    div = 3;
    key = midi + pedalUp_;
    while (key < OrganModel::kLo) key += 12;
    while (key > OrganModel::kLo + 31) key -= 12;
  } else if (midi < split2_) {
    div = 1;
  } else {
    div = organZoneRegistration(std::max(level_, 0)).rightHand;
    while (key > OrganModel::kHi) key -= 12;
  }
  if (keyDiv_[midi] >= 0) m_.closeOn(keyDiv_[midi], keyTo_[midi]);
  keyDiv_[midi] = div;
  keyTo_[midi] = key;
  m_.openOn(div, key);
}

// The zones' level from the dynamics (K1): silence, then ppp .. fff on every
// division; the right hand's keys held moved when its division changes.
void OrganPlayer::zoneLevel() {
  const double x = std::clamp(dyn_ + breathCtl_, 0.0, 1.0);
  const int level = x < 0.04 ? -1 : std::clamp(int((x - 0.04) / 0.12), 0, 7);
  if (level == level_) return;
  const int oldHand = organZoneRegistration(std::max(level_, 0)).rightHand;
  level_ = level;
  if (level < 0) {
    m_.setRegistration("");
  } else {
    const ZoneRegistration z = organZoneRegistration(level);
    m_.setRegistration(z.stops);
    m_.setSwell(z.swell);
    if (z.rightHand != oldHand)
      for (int k = 0; k < 128; ++k)
        if (keyDiv_[k] == oldHand && k >= split2_) {
          m_.closeOn(oldHand, keyTo_[k]);
          keyDiv_[k] = z.rightHand;
          m_.openOn(z.rightHand, keyTo_[k]);
        }
  }
  const char* name = OrganModel::dynamicName(x);
  if (name != lastLevel_ && std::getenv("PJ_ORGAN_SAY")) say(stderr, "organ: %s\n", name);
  lastLevel_ = name;
}

void OrganPlayer::noteOff(int midi) {
  if (!zones_) {
    m_.close(midi);
    return;
  }
  midi &= 127;
  if (keyDiv_[midi] < 0) return;
  m_.closeOn(keyDiv_[midi], keyTo_[midi]);
  keyDiv_[midi] = -1;
}

void OrganPlayer::setHold(bool) {}  // (an organ has no sustain pedal)

void OrganPlayer::render(float* out, int n) {
  // The swell pedal: the dynamics (a knob) and the breath (a controller) open
  // the boxes' shutters (0.5, the default, ~3/4 open; 2/3 and up fully).
  if (zones_) {
    zoneLevel();
  } else if (swellFixed_ >= 0) {
    m_.setSwell(swellFixed_);
  } else if (m_.crescendo()) {  // (the roller: the dynamics, K1, from silence to fff)
    const double x = std::clamp(dyn_ + breathCtl_, 0.0, 1.0);
    m_.setRoller(x);
    const char* name = OrganModel::dynamicName(x);
    if (name != lastDyn_ && std::getenv("PJ_ORGAN_SAY")) say(stderr, "organ: %s\n", name);
    lastDyn_ = name;
  } else {
    m_.setSwell(std::clamp(1.5 * (dyn_ + breathCtl_), 0.03, 1.0));
  }
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
