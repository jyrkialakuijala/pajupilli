#include "tonewheel.h"
#include "heard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

bool tonewheelInstrument(const Instrument& inst) { return std::strcmp(inst.family, "tonewheel") == 0; }

namespace {

// The 12 gear pairs (research/tonewheel_organ.md §3.1): a wheel turns at 20 x ratio a second.
constexpr double kRatio[12] = {85.0 / 104, 71.0 / 82, 67.0 / 73, 105.0 / 108, 103.0 / 100, 84.0 / 77,
                               74.0 / 64, 98.0 / 80, 96.0 / 74, 88.0 / 64, 67.0 / 46, 108.0 / 70};
// The drawbars' pitches against the 8' (semitones): 16' 5 1/3' 8' 4' 2 2/3' 2' 1 3/5' 1 1/3' 1'.
constexpr int kFoot[9] = {-12, 7, 0, 12, 19, 24, 28, 31, 36};

int teethOf(int n) { return n >= 85 ? 192 : 2 << ((n - 1) / 12); }
int gearOf(int n) { return n >= 85 ? 5 + (n - 85) : (n - 1) % 12; }

struct Rng {
  uint32_t s;
  double u() { s = s * 1664525u + 1013904223u; return (s >> 8) * (1.0 / 16777216.0); }
  double n() {  // (a normal draw)
    const double a = std::max(u(), 1e-12), b = u();
    return std::sqrt(-2 * std::log(a)) * std::cos(2 * M_PI * b);
  }
};

}  // namespace

double TonewheelModel::wheelHz(int n) { return 20 * kRatio[gearOf(n)] * teethOf(n); }

// The lower manual for an upper one (research/tonewheel_organ.md: the drawbar knobs): the melody's
// accompaniment, from the same family but softer and darker, as organists derive it. Each drawbar
// keeps a share of the upper's (the foundation all of it, the 5 1/3' half: less buzz under the
// tune; the top little), then a step less (~3 dB under).
std::array<double, 9> tonewheelLowerFor(const std::array<double, 9>& upper, double extraSofter, double clarity) {
  // (the 5 1/3' half: the jazz organist's lower for bass and comping under 888 is 838 or 848; under
  // a pedal bass the clarity below eases it toward another's favourite, 808000000)
  static const double kKeep[9] = {0.85, 0.45, 1.0, 0.55, 0.45, 0.3, 0.25, 0.2, 0.15};
  static const double kSofter = knob("PJ_TONEWHEEL_LOWER_SOFTER", 1.0);  // (steps)
  std::array<double, 9> lower{};
  for (int d = 0; d < 9; ++d)
    lower[d] = std::clamp(upper[d] * kKeep[d] - (upper[d] > 0 ? kSofter + extraSofter : 0), 0.0, 8.0);
  // (never silenced: the 8' stays at least a whisper where the upper has one)
  if (upper[2] > 0) lower[2] = std::max(lower[2], std::min(2.0, upper[2]));
  // (clarity: where the pedals carry the bass, under counterpoint or rich harmony, the lower's own
  // sub-octave and fifth are eased back: no doubled bass an octave down, no mud)
  lower[0] *= 1 - 0.6 * clarity;
  lower[1] *= 1 - 0.5 * clarity;
  return lower;
}

// The lower manual on a shape of its own, at the level of the lower registration it replaces (low:
// its whole loudness, 3 dB a step). kind 1, the percussion's stand-in (008880080: the 8', 4',
// 2 2/3' and 1 1/3'; no sub-octave, no fifth below): a sharp, bright attack, an octave of
// percussion-like sound where the left hand does not play the bass (the tonewheel organ has the percussion on
// the upper manual alone). kind 2, the hollow 8' (008060400: the 8' with its odd harmonics, the
// 2 2/3' and 1 3/5', the clarinet): a line of another colour from the upper's.
std::array<double, 9> tonewheelLowerShaped(const std::array<double, 9>& low, int kind) {
  auto gain = [](double x) { return x >= 1 ? fastPow(10.0, -3.0 * (8 - std::min(x, 8.0)) / 20) : std::max(x, 0.0) * fastPow(10.0, -21.0 / 20); };
  static const double kOff[3][9] = {{-99, -99, 0, -99, -99, -99, -99, -99, -99},
                                    {-99, -99, 0, 0, 0, -99, -99, 0, -99},
                                    {-99, -99, 0, -99, -2, -99, -4, -99, -99}};
  const double(&off)[9] = kOff[std::clamp(kind, 0, 2)];
  double g = 0;
  for (double x : low) g += gain(x);
  auto at = [&](double b) {
    std::array<double, 9> r{};
    for (int d = 0; d < 9; ++d) r[d] = off[d] > -50 ? std::clamp(b + off[d], 0.0, 8.0) : 0;
    return r;
  };
  auto sum = [&](const std::array<double, 9>& r) { double t = 0; for (double x : r) t += gain(x); return t; };
  double lo = 0, hi = 8;  // (the top drawbar's setting: bisected to the level)
  for (int it = 0; it < 40; ++it) {
    const double mid = (lo + hi) / 2;
    (sum(at(mid)) < g ? lo : hi) = mid;
  }
  return at(lo);
}

// The pedals for an upper manual (its two drawbars, 16' and 8'): how loud the upper is sets the
// 16' (a whisper over a booming pedal would be wrong), how bright it is the 8' (round registrations
// want the simple 16', bright ones a brighter pedal).
std::array<double, 2> tonewheelPedalFor(const std::array<double, 9>& upper, double clarity) {
  auto gain = [](double x) { return x >= 1 ? fastPow(10.0, -3.0 * (8 - std::min(x, 8.0)) / 20) : std::max(x, 0.0) * fastPow(10.0, -21.0 / 20); };
  double all = 0, high = 0;
  for (int d = 0; d < 9; ++d) {
    all += gain(upper[d]);
    if (d >= 3) high += gain(upper[d]);
  }
  const double loud = std::clamp(all / 3, 0.0, 1.0);          // (three drawbars out full: as loud as a pedal wants)
  const double bright = all > 0 ? high / all : 0;             // (the share of the 4' and above)
  // (clarity: a moving pedal line under counterpoint or rich harmony on less 16', more 8': its
  // notes defined, not a boom)
  return {std::clamp(4.5 + 3.5 * loud - 3 * clarity, 0.0, 8.0), std::clamp(1.5 + 8 * bright + 3 * clarity, 0.0, 8.0)};
}

std::string TonewheelRegistration::text() const {
  auto bars = [](const int* b, int n) { std::string s; for (int i = 0; i < n; ++i) s += char('0' + b[i]); return s; };
  static const char* const kVib[7] = {"off", "V1", "V2", "V3", "C1", "C2", "C3"};
  char buf[256];
  std::snprintf(buf, sizeof buf, "upper %s, lower %s, pedal %s, percussion %s, vibrato %s, Rotary %s, drive %.1f",
                bars(upper.data(), 9).c_str(), bars(lower.data(), 9).c_str(), bars(pedal.data(), 2).c_str(),
                perc ? (std::string(percThird ? "3rd" : "2nd") + (percFast ? " fast" : " slow") + (percSoft ? " soft" : "")).c_str() : "off",
                (std::string(kVib[std::clamp(vibrato, 0, 6)]) +
                 (vibrato > 0 && vibUpper != vibLower ? (vibUpper ? " upper only" : " lower only") : vibrato > 0 && !vibUpper ? " (tabs off)" : ""))
                    .c_str(),
                rotaryFast ? "fast" : "slow", drive);
  return buf;
}

// research/tonewheel_organ.md §13.3: 17 the jazz organ's 888000000 with C3 and the Rotary slow; 18 with
// the percussion (3rd, fast); 19 the rock organ: 688600000 (Jon Lord's), C3, the lower 808000000,
// the amp hotter, the Rotary slow. Live
// (-inst) plays these as they are; -play's planner takes them as its starting point, a hint, and
// adapts from there (the mud, the Rotary's gestures, the ending).
TonewheelRegistration tonewheelPreset(const std::string& name) {
  TonewheelRegistration r;
  if (name == "percussive-organ") {
    r.perc = true;
    r.vibrato = 0;
  } else if (name == "drawbar-organ") {  // (888000000 on the slow Rotary)
    r.upper = {8, 8, 8, 0, 0, 0, 0, 0, 0};
    r.rotaryFast = false;
  } else if (name == "rock-organ") {  // (688600000, C3, the amp hotter, the Rotary resting on slow;
                                       // the left hand's comping 808000000, clear of the bass)
    r.upper = {6, 8, 8, 6, 0, 0, 0, 0, 0};
    r.lower = {8, 0, 8, 0, 0, 0, 0, 0, 0};
    r.vibrato = 6;
    r.rotaryFast = false;
    r.drive = 5.0;
  }
  tonewheelOverrides(r);
  return r;
}

void tonewheelOverrides(TonewheelRegistration& r) {
  // Overrides: PJ_TONEWHEEL_UPPER / _LOWER (nine digits), _PEDAL (two), _PERC ("off", "3f", "2s",
  // "3fs": the harmonic, fast or slow, soft), _VIB ("off", "V1".."C3"), _ROTARY ("slow", "fast",
  // "off": dry).
  auto bars = [](const char* e, int* b, int n) {
    if (!e) return;
    for (int i = 0; i < n && e[i]; ++i) b[i] = std::clamp(e[i] - '0', 0, 8);
  };
  bars(std::getenv("PJ_TONEWHEEL_UPPER"), r.upper.data(), 9);
  bars(std::getenv("PJ_TONEWHEEL_LOWER"), r.lower.data(), 9);
  bars(std::getenv("PJ_TONEWHEEL_PEDAL"), r.pedal.data(), 2);
  if (const char* e = std::getenv("PJ_TONEWHEEL_PERC")) {
    const std::string p = e;
    r.perc = p != "off";
    if (r.perc) {
      r.percThird = p[0] == '3';
      r.percFast = p.size() < 2 || p[1] != 's';
      r.percSoft = p.find('s', 2) != std::string::npos;
    }
  }
  if (const char* e = std::getenv("PJ_TONEWHEEL_VIB")) {
    const std::string v = e;
    r.vibrato = v == "off" ? 0 : (v[0] == 'C' ? 3 : 0) + std::clamp(v.size() > 1 ? v[1] - '0' : 3, 1, 3);
  }
  if (const char* e = std::getenv("PJ_TONEWHEEL_ROTARY")) {
    const std::string l = e;
    r.rotary = l != "off";
    r.rotaryFast = l == "fast";
  }
}

TonewheelModel::TonewheelModel(const Instrument& inst) : Tube(inst) {
  const double fs = inst.fs;
  const uint32_t serial = uint32_t(knob("PJ_TONEWHEEL_SERIAL", 1957));
  const double age = knob("PJ_TONEWHEEL_AGE", 1.0);
  rng_ = serial * 2654435761u + 5u;
  for (int g = 0; g < 12; ++g) {
    turnsPerSec_[g] = 20 * kRatio[g];
    inc_[g] = uint32_t(std::llround(turnsPerSec_[g] / fs * 4294967296.0));
    Rng r{serial * 7919u + uint32_t(g) * 104729u};
    phase_[g] = uint32_t(r.u() * 4294967296.0);  // (where each shaft stood when the motor was started)
  }
  // Each wheel: one revolution of its pickup's voltage (research/tonewheel_organ.md §3.3, §4).
  table_.resize(kWheels + 1);
  for (int n = 1; n <= kWheels; ++n) {
    gearOf_[n] = gearOf(n);
    partner_[n] = n <= 36 ? n + 48 : n <= 41 ? 0 : n <= 48 ? n + 43 : n <= 84 ? n - 48 : n - 43;
    // (its assembly: the two wheels on one shaft, four octaves apart (1-36 with 49-84, 42-48 with
    // 85-91; 37-41 alone), named by the lower; they and no others share its errors)
    shaftOf_[n] = partner_[n] && partner_[n] < n ? partner_[n] : n;
    const int T = teethOf(n);
    int N = 1024;
    while (N < 32 * T) N <<= 1;
    Rng r{serial * 31337u + uint32_t(n) * 2246822519u};
    // The wheel's eccentricity and ovality: its tone swells and fades once a revolution.
    const double ma = age * 0.035 * (0.3 + std::fabs(r.n())), pa = 2 * M_PI * r.u();
    // The shaft's own (both its wheels alike, no other shaft): bent, its wheels' gap to their
    // pickups swelling and closing once a turn; its driven gear's runout and ovality, the shaft
    // turning a little fast and slow once and twice a turn (its angle off by up to ~1e-3 rad:
    // ~1 c of vibrato at the rotation rate); where it stood when the motor started.
    Rng rs{serial * 15485863u + uint32_t(shaftOf_[n]) * 2654435761u};
    const double msh = age * 0.03 * (0.3 + std::fabs(rs.n())), psh = 2 * M_PI * rs.u();
    const double de1 = knob("PJ_TONEWHEEL_SHAFT", 1.0) * age * 6e-4 * std::fabs(rs.n()), pe1 = 2 * M_PI * rs.u();
    const double de2 = 0.4 * de1 * rs.u(), pe2 = 2 * M_PI * rs.u();
    shaftOff_[shaftOf_[n]] = uint32_t(rs.u() * 4294967296.0);
    const double mo = 0.3 * ma * r.u(), po = 2 * M_PI * r.u();
    // The slow flux of the off-centre wheel itself: a line at the rotation rate (the 2- and
    // 4-tooth wheels' sub-octaves).
    const double dr = age * 0.03 * (2.0 / T) * std::fabs(r.n()), pd = 2 * M_PI * r.u();
    // Each tooth a little off its place (the cutter's spacing) and in depth.
    std::vector<double> pos(T), dep(T);
    for (int k = 0; k < T; ++k) {
      pos[k] = age * T * 2e-4 * r.n();
      dep[k] = age * 0.006 * r.n();
    }
    // Its harmonics: the 2-tooth "complex" wheels shaped for odd ones; the others near sines.
    // (the machined teeth: never a perfect sine, each wheel's profile its own (cutter wear, the
    // tooth's flanks, the pickup's tip): overtones to the 7th, ~-30 dB for the 2nd and 3rd, the 5th
    // ~-34 (a "tonewheel geometry" 5th of ~-32 dB, Reid), +-4 dB each wheel. Exact multiples of the
    // wheel, they beat against the tempered wheels of the drawbars above (the 2 2/3' ~2 c narrow of
    // the 3rd, the 1 3/5' ~14 c sharp of the 5th): the full registrations' dissonant brilliance.
    // PJ_TONEWHEEL_WHEELHARM: dB on all of them)
    double h[8] = {0, 1, 0, 0, 0, 0, 0, 0}, ph[8] = {};
    static const double kHarm = knob("PJ_TONEWHEEL_WHEELHARM", 0.0);
    static const double kLevel[8] = {0, 0, -30, -30, -42, -34, -48, -45};
    if (n <= 12) {
      h[2] = 0.02; h[3] = 0.25; h[5] = 0.08; h[7] = 0.03;
    } else {
      for (int k = 2; k < 8; ++k) h[k] = fastPow(10.0, (kLevel[k] + kHarm + 4 * r.n()) / 20);
    }
    for (int k = 2; k < 8; ++k) ph[k] = 2 * M_PI * r.u();
    // The tuned filter of wheels 49-91 (a capacitor and transformer per wheel), aged capacitors
    // tuning it lower.
    double detune = 1;
    const double q = 4;
    if (n >= 49) {
      detune = 1 / std::sqrt(knob("PJ_TONEWHEEL_CAPS", 1.0) * (1 + 0.1 * age * std::fabs(r.n())));
      for (int k = 1; k < 8; ++k) {
        const double x = k / detune;
        h[k] /= std::sqrt(1 + q * q * (x - 1 / x) * (x - 1 / x));
      }
    }
    // Its subharmonics: the wheel's own set, from its rotation rate up through that rate's
    // harmonics (the magnet's and the wheel's irregularities over one turn: a periodic signal of one
    // revolution), up to 3 x its teeth (8 .. 48); its own levels (~1/k, +-3 dB) and phases, together
    // ~-40 dB under the tone (x age; PJ_TONEWHEEL_SUBS, dB); the tone's own harmonics (multiples of
    // the teeth) left out; through the wheel's filter (49-91) as the rest.
    const int kSubs = std::clamp(3 * T, 8, 48);
    std::vector<double> sa(kSubs + 1, 0.0), sp(kSubs + 1, 0.0);
    {
      static const double kSubDb = knob("PJ_TONEWHEEL_SUBS", -40.0);
      Rng rsub{serial * 92821u + uint32_t(n) * 3266489917u};
      double e2 = 0;
      for (int k = 1; k <= kSubs; ++k) {
        if (k % T == 0) continue;
        sa[k] = fastPow(10.0, 3 * rsub.n() / 20) / k;
        sp[k] = 2 * M_PI * rsub.u();
        e2 += sa[k] * sa[k];
      }
      const double total = age * fastPow(10.0, kSubDb / 20) * h[1] / std::sqrt(std::max(e2, 1e-12));
      for (int k = 1; k <= kSubs; ++k) {
        sa[k] *= total;
        if (n >= 49) {
          const double x = double(k) / T / detune;
          sa[k] /= std::sqrt(1 + q * q * (x - 1 / x) * (x - 1 / x));
        }
      }
    }
    const double level = fastPow(10.0, 0.8 * age * r.n() / 20) / std::max(h[1], 1e-6);
    std::vector<float>& t = table_[n];
    t.resize(N);
    for (int i = 0; i < N; ++i) {
      const double phi = 2 * M_PI * i / N;
      const double u = (phi + de1 * std::sin(phi + pe1) + de2 * std::sin(2 * phi + pe2)) * T / (2 * M_PI);
      const int k = int(u) % T, k1 = (k + 1) % T;
      const double w = 0.5 * (1 - std::cos(M_PI * (u - std::floor(u))));
      const double th = 2 * M_PI * u + pos[k] + (pos[k1] - pos[k]) * w;
      // The wobble works through the gap: the wheel's own eccentricity and ovality and its shaft's
      // bend carry the teeth nearer the pickup's pole piece and away from it once (and twice) a
      // turn. The coupling falls with the gap, never to nothing: the fundamental's as (1 + dg)^-2
      // (dg the gap's change over its rest gap), steeper closing than opening; the overtones'
      // finer field patterns fall off faster with distance, (1 + dg)^-(2 + 1.6 (j - 1)), so the
      // tone's colour breathes with its level. The tooth's own depth error on top.
      const double dg = -0.5 * (ma * std::cos(phi - pa) + msh * std::cos(phi - psh) + mo * std::cos(2 * phi - po));
      const double g = std::max(1 + dg, 0.3);
      const double depth = 1 + dep[k] + (dep[k1] - dep[k]) * w;
      double v = 0;
      for (int j = 1; j < 8; ++j) v += h[j] * fastPow(g, -(2 + 1.6 * (j - 1))) * std::sin(j * th + ph[j]);
      v *= depth;
      double sub = 0;
      for (int k = 1; k <= kSubs; ++k)
        if (sa[k] != 0) sub += sa[k] * std::sin(k * phi + sp[k]);
      t[i] = float(level * (v + dr * std::sin(phi - pd) + sub));
    }
    // The whole generator's whisper (the pickups hear their neighbours: ~-70 dB each).
    background_[n] = fastPow(10.0, (-70 + 6 * r.n()) / 20) * (r.u() < 0.5 ? -1 : 1);
    // Its spring coupling: a torsional resonance of 3-8 Hz, Q ~3.
    flW_[n] = 2 * M_PI * (3 + 5 * r.u());
    flQ_[n] = 2 + 2 * r.u();
  }
  // The flutter's scale: each wheel's resonator run a while to find its spread, then set so that
  // a wheel turns ~1.5e-4 revolution either way (x age): a few tenths of a cent on the high
  // wheels, less low down (research/tonewheel_organ.md §3.1: the couplings absorb, not remove).
  {
    const double ctlDt = 32 / fs, sigma = knob("PJ_TONEWHEEL_FLUTTER", 1.0) * age * 1.5e-4;
    Rng r{serial * 4099u + 17u};
    for (int n = 1; n <= kWheels; ++n) {
      double x = 0, v = 0, sum = 0;
      const int steps = int(5 / ctlDt);
      for (int i = 0; i < steps; ++i) {
        const double a = r.n() / std::sqrt(ctlDt) - flW_[n] / flQ_[n] * v - flW_[n] * flW_[n] * x;
        v += a * ctlDt;
        x += v * ctlDt;
        sum += x * x;
      }
      flG_[n] = sigma / std::max(std::sqrt(sum / steps), 1e-12);
    }
    flRng_ = serial * 6007u + 3u;
  }
  microRing_.assign(size_t(0.02 * fs) + 8, 0.0f);
  // The scanner's line (~0.8 ms).
  line_.assign(256, 0.0f);
  lineL_.assign(256, 0.0f);
  {
    // The cabinets (PJ_ROTARY_COUNT, 1-4; 4): each its parts off nominal (+-7 %, PJ_ROTARY_SPREAD),
    // its own speed and horn-to-drum ratio, its own place and facing in the room. Their speeds
    // (tools/rotary_ratios.py --four --q 11: four factors whose every cross ratio stays furthest
    // from the fractions up to N/11, the simple ones the most: none within 1.2 % of one over 1 .. 5,
    // the closest misses 0.6-0.9 % from 8/7, 10/7, 11/9, 13/10, 12/11; scaled to centre on the
    // usual speeds, which keeps every cross ratio):
    //   1: x 1.081, the model's own horn/drum (1.6), a little left, turned 15 degrees left;
    //   2: x 0.953, the published 122's ratios (1.20 on slow, 1.17 on fast), right and back,
    //      turned 30 degrees right;
    //   3: x 0.827, published ratios, by the left wall, further back, turned 40 left;
    //   4: x 1.172, published ratios, toward the right wall, turned 50 degrees right.
    // PJ_ROTARY_SPEEDS="a,b,c,d" sets them. One cabinet: as built (no spread, x 1), where it
    // always stood.
    static const int kCount = std::clamp(int(knob("PJ_ROTARY_COUNT", 4)), 1, 4);
    static const double kSpread = knob("PJ_ROTARY_SPREAD", 0.07);
    double speed[4] = {1.1724, 1.0811, 0.9534, 0.8271};
    if (const char* e = std::getenv("PJ_ROTARY_SPEEDS")) std::sscanf(e, "%lf,%lf,%lf,%lf", &speed[0], &speed[1], &speed[2], &speed[3]);
    if (kCount == 1) speed[0] = 1;
    // (x toward the listener, y to the left, m; the facing, degrees)
    const double place[4][3] = {{0, 0, 0}, {0.3, 2.0, 25}, {0.3, -2.0, -25}, {-5.0, 0.3, 10}};
    // Which manuals each plays (upper, lower, pedals): the centre's the upper (75 %, the others
    // 12.5 % each), the left and right the lower, the one 5 m behind the pedals
    // (PJ_ROTARY_MIX="u,l,p;u,l,p;u,l,p;u,l,p"). Each manual's shares scaled so its whole power over
    // the cabinets is as it would be in all of them alike: the balance kept.
    double mix[4][3] = {{0.75, 0.125, 0.125}, {0.125, 0.75, 0.125}, {0.125, 0.75, 0.125}, {0.125, 0.125, 0.75}};
    if (const char* e = std::getenv("PJ_ROTARY_MIX"))
      std::sscanf(e, "%lf,%lf,%lf;%lf,%lf,%lf;%lf,%lf,%lf;%lf,%lf,%lf", &mix[0][0], &mix[0][1], &mix[0][2], &mix[1][0], &mix[1][1],
                  &mix[1][2], &mix[2][0], &mix[2][1], &mix[2][2], &mix[3][0], &mix[3][1], &mix[3][2]);
    cab_.resize(kCount);
    for (int c = 0; c < kCount; ++c) {
      cab_[c].les.setup(fs, serial + 7919u * c, kCount > 1 ? kSpread : 0.0, c == 0, speed[c], c ? 1.20 : 0.0, c ? 1.17 : 0.0);
      for (auto& ring : cab_[c].ring) ring.assign(2048, 0.0f);
      if (kCount > 1) {
        cab_[c].x = place[c][0];
        cab_[c].y = place[c][1];
        cab_[c].rot = place[c][2] * M_PI / 180;
      }
    }
    for (int m = 0; m < 3; ++m) {
      double e2 = 0;
      for (int c = 0; c < kCount; ++c) e2 += mix[c][m] * mix[c][m];
      for (int c = 0; c < kCount; ++c) cab_[c].mix[m] = e2 > 0 ? mix[c][m] * std::sqrt(kCount / e2) : 1;
    }
    placeCabinets();
  }
  roomMics_ = std::getenv("PJ_ROTARY_MICS") == nullptr || std::string(std::getenv("PJ_ROTARY_MICS")) != "close";
  setRegistration(tonewheelPreset(inst.name));
}

void TonewheelModel::setRegistration(const TonewheelRegistration& r) {
  reg_ = r;
  // The drawbars' goals: each slider then moves there by its own control (dynamics(); the first
  // registration is where they already stand).
  for (int d = 0; d < 9; ++d) {
    goal_[kUpper][d] = r.upper[d];
    goal_[kLower][d] = r.lower[d];
  }
  goal_[kPedal][0] = r.pedal[0];
  goal_[kPedal][1] = r.pedal[1];
  if (!placed_) {
    for (int v = 0; v < 3; ++v)
      for (int d = 0; d < 9; ++d) pos_[v][d] = goal_[v][d];
    placed_ = true;
    drawbarGains();
  }
  for (Cab& c : cab_) {
    c.les.setFast(r.rotaryFast);
    c.les.setDrive(r.drive);
  }
}

void TonewheelModel::setLowerGoal(const std::array<double, 9>& bars) {
  for (int d = 0; d < 9; ++d) goal_[kLower][d] = std::clamp(bars[d], 0.0, 8.0);
}

void TonewheelModel::setPedalGoal(const std::array<double, 2>& bars) {
  for (int d = 0; d < 2; ++d) goal_[kPedal][d] = std::clamp(bars[d], 0.0, 8.0);
}

void TonewheelModel::placeDrawbars() {
  for (int v = 0; v < 3; ++v)
    for (int d = 0; d < 9; ++d) {
      pos_[v][d] = goal_[v][d];
      integ_[v][d] = 0;
    }
  drawbarGains();
}

void TonewheelModel::snap(int div) {
  for (int d = 0; d < 9; ++d) {
    pos_[div][d] = goal_[div][d];
    integ_[div][d] = 0;
  }
  drawbarGains();
}

void TonewheelModel::setUpperGoal(const std::array<double, 9>& bars) {
  for (int d = 0; d < 9; ++d) goal_[kUpper][d] = std::clamp(bars[d], 0.0, 8.0);
}

// The gains of the sliders where they stand: 3 dB a step, below the first step fading linearly
// to silence (the drawbar's contact leaving its first tap).
void TonewheelModel::drawbarGains() {
  auto gain = [](double x) { return x >= 1 ? fastPow(10.0, -3.0 * (8 - std::min(x, 8.0)) / 20) : std::max(x, 0.0) * fastPow(10.0, -21.0 / 20); };
  for (int d = 0; d < 9; ++d) {
    gainU_[d] = gain(pos_[kUpper][d]);
    gainL_[d] = gain(pos_[kLower][d]);
  }
  if (reg_.perc) gainU_[8] = 0;  // (the percussion takes the 1' drawbar's busbar)
  gainP_[0] = gain(pos_[kPedal][0]);
  gainP_[1] = gain(pos_[kPedal][1]);
}

// Each drawbar a slider pulled by a hand: a PI control of its speed (the error's proportion and
// its integral), limited to a hand's top speed, stopped by the slider's ends (no winding up
// against them): it glides to its goal, a touch past it now and then, never in steps.
// The mud in what is held now: close intervals low on the manuals (a second under C4, worse the
// lower; a third under C3), where the 16' and 5 1/3' pile their low partials together. 0..1.
double TonewheelModel::mudLevel() const {
  int m[32], n = 0;
  double mud = 0;
  int ped = 1000;
  for (const Key& k : keys_) {
    if (k.down && k.div != kPedal && n < 32) m[n++] = k.midi;
    if (k.down && k.div == kPedal) ped = std::min(ped, k.midi);
  }
  // (a pedal note under low manual notes close above it, within a fifth under C3: the pedal's 16'
  // and the manuals' subs crowd one octave)
  if (ped < 1000)
    for (int i = 0; i < n; ++i)
      if (m[i] < 48 && m[i] - ped >= 0 && m[i] - ped <= 7) mud += 0.5 * std::clamp((48 - m[i]) / 12.0, 0.2, 1.0);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      const int lo = std::min(m[i], m[j]), iv = m[j] - m[i];
      if (iv <= 0) continue;
      if (iv <= 2) mud += std::clamp((60 - lo) / 18.0, 0.0, 1.0) * (iv == 1 ? 1.0 : 0.7);
      else if (iv <= 4) mud += std::clamp((48 - lo) / 12.0, 0.0, 1.0) * 0.4;
    }
  return std::clamp(mud / 1.5, 0.0, 1.0);
}

// The lower manual crowding the pedals: its held notes low (under C4, the lower the more) and
// close above the pedal note under them (within an octave and a half), where its 16' doubles its
// line an octave down into the pedals' register. 0..1.
double TonewheelModel::crowdLevel() const {
  int ped = 1000;
  for (const Key& k : keys_)
    if (k.down && k.div == kPedal) ped = std::min(ped, k.midi);
  double c = 0;
  int n = 0;
  for (const Key& k : keys_)
    if (k.down && k.div == kLower) {
      double w = std::clamp((60 - k.midi) / 14.0, 0.0, 1.0);
      if (ped < 1000) w *= 0.5 + 0.5 * std::clamp((18 - (k.midi - ped)) / 12.0, 0.0, 1.0);
      else w *= 0.5;
      c = std::max(c, w);
      ++n;
    }
  return std::clamp(c * (n > 1 ? 1.2 : 1.0), 0.0, 1.0);
}

// The upper manual crowding the voices under it: its held notes low (under G4, the lower the more)
// and close above the lower manual's or the pedals' notes (within an octave), where its 16' doubles
// its line an octave down onto theirs. 0..1.
double TonewheelModel::upperCrowdLevel() const {
  int below = -1000;
  for (const Key& k : keys_)
    if (k.down && k.div != kUpper) below = std::max(below, k.midi);
  double c = 0;
  for (const Key& k : keys_)
    if (k.down && k.div == kUpper) {
      double w = std::clamp((67 - k.midi) / 12.0, 0.0, 1.0);
      w *= below > -1000 ? 0.4 + 0.6 * std::clamp((12 - (k.midi - below)) / 8.0, 0.0, 1.0) : 0.3;
      c = std::max(c, w);
    }
  return c;
}

void TonewheelModel::moveDrawbars(double dt) {
  static const double kP = knob("PJ_TONEWHEEL_DRAWBAR_KP", 8), kI = knob("PJ_TONEWHEEL_DRAWBAR_KI", 20);
  static const double kMax = knob("PJ_TONEWHEEL_DRAWBAR_SPEED", 16);  // (steps a second)
  // (online: where low clusters are held, the player eases the manuals' 16' and 5 1/3' back (up to
  // 70 % and 80 %) and gives the 4' a little more for clarity; quick to react (~0.3 s), slow to
  // restore (~1.5 s); PJ_TONEWHEEL_MUD scales it, 0 off)
  static const double kMud = knob("PJ_TONEWHEEL_MUD", 1.0);
  if (kMud > 0) {
    const double want = kMud * mudLevel();
    mud_ += (want - mud_) * std::min(1.0, dt / (want > mud_ ? 0.3 : 1.5));
  }
  // (the lower manual crowding the pedals: its 16' and 5 1/3' eased back a little, gradually: the
  // crowding's measure followed over ~1.5 s rising, ~3 s falling (counterpoint changes its held
  // notes every beat), its effect over ~1 s more by the sliders; up to 75 % and 70 %;
  // PJ_TONEWHEEL_CROWD scales it, 0 off)
  thinNow_ += (thin_ - thinNow_) * std::min(1.0, dt / 0.3);
  // (the lower manual carrying a moving bass line, the pedals resting over a second: its 4',
  // 2 2/3' and 2' drawn out (up to 3, 2 and 2 steps) for the line's definition, over ~0.8 s, back
  // over ~2 s once the pedals return or it stops moving; PJ_TONEWHEEL_BASSLINE scales it, 0 off)
  {
    static const double kBass = knob("PJ_TONEWHEEL_BASSLINE", 1.0);
    lowRate_ *= std::exp(-dt / 1.0);
    for (const Key& k : keys_)
      if (k.down && k.div == kPedal) lastPedal_ = t_;
    const double want = t_ - lastPedal_ > 1.0 ? kBass * std::clamp((lowRate_ - 1.5) / 3.0, 0.0, 1.0) : 0.0;
    bass_ += (want - bass_) * std::min(1.0, dt / (want > bass_ ? 0.8 : 2.0));
  }
  static const double kCrowd = knob("PJ_TONEWHEEL_CROWD", 1.0);
  if (kCrowd > 0) {
    const double want = kCrowd * crowdLevel();
    crowdT_ += (want - crowdT_) * std::min(1.0, dt / (want > crowdT_ ? 0.8 : 3.0));
    crowd_ += (crowdT_ - crowd_) * std::min(1.0, dt / 1.0);
    // (the upper manual's own, the same way)
    const double wantU = kCrowd * upperCrowdLevel();
    upCrowdT_ += (wantU - upCrowdT_) * std::min(1.0, dt / (wantU > upCrowdT_ ? 0.8 : 3.0));
    upCrowd_ += (upCrowdT_ - upCrowd_) * std::min(1.0, dt / 1.0);
    static const bool kLog = std::getenv("PJ_TONEWHEEL_MUDLOG") != nullptr;
    static double logT = 0;
    if (kLog && (logT += dt) >= 1) {
      logT -= 1;
      say(stderr, "mud t %.0f crowd %.2f upper crowd %.2f clusters %.2f trill %.2f bassline %.2f lower 16' %.1f upper %.1f%.1f%.1f\n", t_, crowd_, upCrowd_, mud_, thinNow_, bass_, pos_[kLower][0], pos_[kUpper][0], pos_[kUpper][1], pos_[kUpper][2]);
    }
  }
  bool moving = false;
  for (int v = 0; v < 3; ++v)
    for (int d = 0; d < (v == kPedal ? 2 : 9); ++d) {
      double goal = goal_[v][d];
      if (v != kPedal && thinNow_ > 1e-3 && d != 2 && d != 3) goal *= 1 - 0.8 * thinNow_;  // (a trill: the 8' and 4' kept)
      if (v == kUpper && upCrowd_ > 1e-3) {  // (the upper low over the others: its 16' and 5 1/3' eased, a little more 4')
        if (d == 0) goal *= 1 - 0.75 * upCrowd_;
        if (d == 1) goal *= 1 - 0.7 * upCrowd_;
        if (d == 3) goal = std::min(8.0, goal + 1.5 * upCrowd_);
      }
      if (v == kLower && bass_ > 1e-3) {
        if (d == 3) goal = std::min(8.0, goal + 3 * bass_);
        if (d == 4 || d == 5) goal = std::min(8.0, goal + 2 * bass_);
      }
      if (v == kLower && crowd_ > 1e-3) {
        if (d == 0) goal *= 1 - 0.75 * crowd_;
        if (d == 1) goal *= 1 - 0.7 * crowd_;
      }
      if (v == kPedal && d == 0 && mud_ > 1e-3) goal *= 1 - 0.5 * std::min(mud_, 1.0);  // (the pedal's 16' too)
      if (v != kPedal && mud_ > 1e-3) {
        if (d == 0) goal *= 1 - 0.7 * std::min(mud_, 1.0);
        if (d == 1) goal *= 1 - 0.8 * std::min(mud_, 1.0);
        if (d == 3 && goal_[v][2] > 0) goal = std::min(8.0, goal + 1.5 * std::min(mud_, 1.0));
      }
      const double e = goal - pos_[v][d];
      if (std::fabs(e) < 1e-4 && std::fabs(integ_[v][d]) < 1e-4) continue;
      moving = true;
      double u = kP * e + kI * (integ_[v][d] + e * dt);
      const bool limited = std::fabs(u) > kMax;
      u = std::clamp(u, -kMax, kMax);
      double p = pos_[v][d] + u * dt;
      const bool stopped = p < 0 || p > 8;
      p = std::clamp(p, 0.0, 8.0);
      if (!limited && !stopped) integ_[v][d] += e * dt;
      pos_[v][d] = p;
    }
  if (moving) drawbarGains();
}

// A key's nine contacts: their closing order is the key's own (the springs' heights), the spread
// the key's speed; each bounces a few times (research/tonewheel_organ.md §6.2;
// research/tonewheel_character.md §3).
void TonewheelModel::schedule(Key& k, int velocity, bool closing) {
  Rng order{uint32_t(knob("PJ_TONEWHEEL_SERIAL", 1957)) * 97u + uint32_t(k.div * 128 + k.midi) * 2654435761u};
  int slot[9];
  for (int j = 0; j < 9; ++j) slot[j] = j;
  for (int j = 8; j > 0; --j) std::swap(slot[j], slot[int(order.u() * (j + 1))]);
  rng_ = rng_ * 1664525u + 1013904223u;
  Rng r{rng_};
  const double v = std::clamp(velocity / 127.0, 0.0, 1.0);
  const double spread = closing ? (velocity < 20 ? (10 + (20 - velocity) * 4.5) * 1e-3 : (0.1 + 2.9 * (1 - v) * (1 - v)) * 1e-3)
                                : (0.3 + 1.2 * r.u()) * 1e-3;
  const int nc = k.div == kPedal ? 2 : 9;
  for (int j = 0; j < nc; ++j) {
    Contact& c = k.c[j];
    double t = spread * (slot[j] + 0.3 * r.n()) / 8;
    t = std::max(t, 0.0);
    c.n = 0;
    c.next = 0;
    c.t[c.n++] = t;  // closes (or opens)
    // (a hard key slams the contacts: more bounces; a soft one lays them on)
    const int bounces = int(r.u() * (closing ? 0.5 + 3 * v * v : 2));
    double gap = (0.03 + 0.25 * r.u()) * 1e-3, back = (0.02 + 0.08 * r.u()) * 1e-3;
    for (int b = 0; b < bounces && c.n + 2 <= 8; ++b) {
      t += gap;
      c.t[c.n++] = t;  // bounces off
      t += back;
      c.t[c.n++] = t;  // and back
      gap *= 0.6;
      back *= 0.6;
    }
  }
  k.since = 0;
  // The contact's resistance falling as it closes: at once under a hard key (a sharp click), over
  // ~1 ms under a soft one (the step on the busbar rounded: a soft click).
  if (closing) k.rampClose = (0.12 + 0.88 * (1 - v) * (1 - v)) * 1e-3;
  k.velocity = velocity;
}

void TonewheelModel::press(int div, int midi, int velocity) {
  // Into the division's compass by octaves.
  const int lo = div == kPedal ? 24 : 36, hi = div == kPedal ? 48 : 96;
  while (midi < lo) midi += 12;
  while (midi > hi) midi -= 12;
  for (Key& k : keys_)
    if (k.div == div && k.midi == midi) {
      if (!k.down) {
        k.down = true;
        for (Contact& c : k.c) c.closed = c.gain > 0.5;
        schedule(k, velocity, true);
      }
      return;
    }
  Key k;
  k.div = div;
  k.midi = midi;
  k.down = true;
  if (div == kPedal) {
    const int p = midi - 23;
    k.wheel[0] = p;
    k.wheel[1] = p + 12;
  } else {
    const int key = midi - 35;
    for (int d = 0; d < 9; ++d) {
      int w = key + 12 + kFoot[d];
      if (w < 13) w += 12;
      while (w > kWheels) w -= 12;
      k.wheel[d] = w;
    }
  }
  schedule(k, velocity, true);
  keys_.push_back(k);
  if (div == kLower && midi < 57) lowRate_ += 1;
  if (div == kPedal) lastPedal_ = t_;
}

void TonewheelModel::release(int div, int midi) {
  const int lo = div == kPedal ? 24 : 36, hi = div == kPedal ? 48 : 96;
  while (midi < lo) midi += 12;
  while (midi > hi) midi -= 12;
  for (Key& k : keys_)
    if (k.div == div && k.midi == midi && k.down) {
      k.down = false;
      schedule(k, 64, false);
    }
}

void TonewheelModel::resetAir() {
  Tube::resetAir();
  keys_.clear();
  std::fill(line_.begin(), line_.end(), 0.0f);
  percEnv_ = 0;
  percCharge_ = 1;
}

float TonewheelModel::sample() {
  static const double kOut = knob("PJ_TONEWHEEL_OUT", 5.0), kConsole = 0.06;
  static const double kPartner = fastPow(10.0, -45 / 20.0);
  const double fs = fs_, dt = 1 / fs;
  t_ += dt;
  // The shaft's wobble (20 Hz) and the motor's pulses (120 Hz): every wheel's phase together.
  const double wob = 1.6e-6 * std::sin(2 * M_PI * 20 * t_) + 2.6e-8 * std::sin(2 * M_PI * 120 * t_);
  // At a control rate (1.5 kHz): the grid's slow drift (the synchronous motor follows it: under
  // a cent, wandering over tens of seconds) and each wheel's flutter on its coupling, excited
  // partly by the motor's pulses (all wheels alike) and partly by its own bearing.
  if (++ctl_ >= 32) {
    ctl_ = 0;
    moveDrawbars(32 * dt);
    static const double kMains = knob("PJ_TONEWHEEL_MAINS", 1.0);
    const double cdt = 32 * dt;
    auto noise = [this] {
      flRng_ = flRng_ * 1664525u + 1013904223u;
      const double a = std::max((flRng_ >> 8) * (1.0 / 16777216.0), 1e-12);
      flRng_ = flRng_ * 1664525u + 1013904223u;
      return std::sqrt(-2 * std::log(a)) * std::cos(2 * M_PI * (flRng_ >> 8) * (1.0 / 16777216.0));
    };
    mains_ += -mains_ * cdt / 20 + kMains * 3e-4 * std::sqrt(2 * cdt / 20) * noise();
    const double common = noise();
    for (int n = 1; n <= kWheels; ++n) {
      if (shaftOf_[n] != n) continue;  // (the shaft's flutter: its second wheel turns with it)
      const double e = (0.6 * common + 0.8 * noise()) / std::sqrt(cdt);
      const double a = e - flW_[n] / flQ_[n] * flV_[n] - flW_[n] * flW_[n] * flX_[n];
      flV_[n] += a * cdt;
      flX_[n] += flV_[n] * cdt;
      flOff_[n] = int32_t(flG_[n] * flX_[n] * 4294967296.0);
    }
    for (int n = 1; n <= kWheels; ++n) flOff_[n] = flOff_[shaftOf_[n]];
  }
  int32_t off[12];
  for (int g = 0; g < 12; ++g) {
    phase_[g] += speed_ == 1 ? inc_[g] + int32_t(double(inc_[g]) * mains_) : uint32_t(double(inc_[g]) * (1 + mains_) * speed_);
    off[g] = int32_t(turnsPerSec_[g] * wob * 4294967296.0);
  }
  double bg = 0;
  for (int n = 1; n <= kWheels; ++n) {
    const std::vector<float>& t = table_[n];
    const uint32_t p = phase_[gearOf_[n]] + uint32_t(off[gearOf_[n]]) + shaftOff_[shaftOf_[n]] + uint32_t(flOff_[n]);
    const int bits = 31 - __builtin_clz(unsigned(t.size()));
    const uint32_t i = p >> (32 - bits), mask = uint32_t(t.size()) - 1;
    const float f = float(p << bits) * (1.0f / 4294967296.0f);
    // (loaded by the contacts it feeds: ~3 % less voltage for each beyond the first)
    wheel_[n] = float((t[i] + (t[(i + 1) & mask] - t[i]) * f) / (1 + 0.03 * std::max(load_[n] - 1, 0.0)));
    load_[n] = loadNext_[n];
    loadNext_[n] = 0;
    bg += background_[n] * wheel_[n];
  }
  // The keys' contacts onto the busbars.
  double bus[3][9] = {};
  double percBus = 0;
  const int percBar = reg_.percThird ? 4 : 3;
  bool upperDown = false;
  for (size_t a = 0; a < keys_.size(); ++a) {
    Key& k = keys_[a];
    k.since += dt;
    const int nc = k.div == kPedal ? 2 : 9;
    bool live = k.down;
    for (int j = 0; j < nc; ++j) {
      Contact& c = k.c[j];
      while (c.next < c.n && k.since >= c.t[c.next]) {
        c.closed = (c.next % 2 == 0) == k.down;
        ++c.next;
      }
      // A dirty contact now and then breaks for a moment while held (PJ_TONEWHEEL_DIRT: 1 an
      // old, unserviced organ, ~1 crackle a second under a chord; 0 clean).
      static const double kDirt = knob("PJ_TONEWHEEL_DIRT", 0.3) * knob("PJ_TONEWHEEL_AGE", 1.0);
      if (c.brk > 0) c.brk -= dt;
      else if (c.closed && kDirt > 0) {
        dirtRng_ = dirtRng_ * 1664525u + 1013904223u;  // (its own draws: the wheels' flutter is the generator's alone)
        if ((dirtRng_ >> 8) * (1.0 / 16777216.0) < kDirt * 0.03 * dt) c.brk = 1e-4 + 1.9e-3 * ((dirtRng_ >> 4) & 0xfff) / 4096.0;
      }
      // The contact's resistance falls over ~0.15 ms as it closes, and breaks at once.
      const double target = c.closed && c.brk <= 0 ? 1 : 0;
      c.gain += (target - c.gain) * std::min(1.0, dt / (c.closed ? k.rampClose : 3e-5));
      if (c.gain > 1e-4 || c.next < c.n) live = true;
      if (c.gain > 1e-6) {
        const int w = k.wheel[j];
        loadNext_[w] += c.gain;
        const double x = wheel_[w] + (partner_[w] ? kPartner * wheel_[partner_[w]] : 0.0);
        bus[k.div][j] += c.gain * x;
        if (k.div == kUpper && j == percBar) percBus += c.gain * wheel_[w];
      }
    }
    if (k.div == kUpper && k.down) upperDown = true;
    if (!live) {
      keys_[a] = keys_.back();
      keys_.pop_back();
      --a;
    }
  }
  // Percussion: single-trigger from the first upper key after all were up; its capacitor
  // charges again only while they are up (a short gap: a part charge, a softer, shorter ping).
  if (upperDown && !upperHeld_ && reg_.perc) {
    percEnv_ = 0.6 + 0.4 * percCharge_;
    percCharge_ = 0;
    // (touch-sensitive percussion, as the modern Tonewheels offer: the striking key's speed sets the
    // ping, from ~-5 dB under a soft key to full under a hard one)
    int vel = 100;
    for (const Key& k : keys_)
      if (k.div == kUpper && k.down) vel = std::max(vel == 100 ? 0 : vel, k.velocity);
    static const double kTouch = knob("PJ_TONEWHEEL_PERCTOUCH", 1.0);
    percTouch_ = 1 - kTouch * 0.45 * (1 - std::clamp(vel / 127.0, 0.0, 1.0));
  }
  upperHeld_ = upperDown;
  if (!upperDown) percCharge_ += (1 - percCharge_) * dt / 0.034;
  percEnv_ *= 1 - dt / (reg_.percFast ? 0.375 : 1.55);
  const double percGain = reg_.perc ? std::clamp((percEnv_ - 0.6) / 0.4, 0.0, 1.0) * (reg_.percSoft ? 0.5 : 1.0) * percTouch_ : 0.0;
  double upper = 0, lower = 0, pedal = 0;
  for (int d = 0; d < 9; ++d) {
    upper += gainU_[d] * bus[kUpper][d];
    lower += gainL_[d] * bus[kLower][d];
  }
  if (reg_.perc && !reg_.percSoft) upper *= 0.7;  // (the normal percussion drops the manual a little)
  upper += 1.4 * percGain * percBus;
  pedal = gainP_[0] * bus[kPedal][0] + gainP_[1] * bus[kPedal][1];
  // The scanner: a delay line read back and forth (6.87 Hz) by the rotating capacitor; the upper's
  // and the lower's each their own line under the one scanner (so each manual's signal stays its
  // own for the Rotaries it feeds).
  double U = upper, L = lower;
  if (reg_.vibrato > 0) {
    scanPh_ += 6.867 * (1 + mains_) * speed_ * dt;  // (its synchronous motor follows the grid too)
    if (scanPh_ > 1) scanPh_ -= 1;
    const double tri = scanPh_ < 0.5 ? 2 * scanPh_ : 2 - 2 * scanPh_;
    static const double kDepth[4] = {0, 1.0 / 3, 0.5, 1};
    const double depth = kDepth[(reg_.vibrato - 1) % 3 + 1] * 0.55e-3 * fs;
    const double rp = linePos_ - (1 + depth * tri);
    const int i = int(std::floor(rp));
    const double f = rp - i;
    const int m = int(line_.size()) - 1;
    auto scan = [&](std::vector<float>& line, double& lp, double in) {
      line[linePos_] = float(in);
      const double wet = line[i & m] + (line[(i + 1) & m] - line[i & m]) * f;
      lp += (1 - std::exp(-2 * M_PI * 7000 / fs)) * (wet - lp);
      return reg_.vibrato >= 4 ? 0.7 * (in + lp) : lp;
    };
    if (reg_.vibUpper) U = scan(line_, lineLp_, upper);
    if (reg_.vibLower) L = scan(lineL_, lineLpL_, lower);
    linePos_ = (linePos_ + 1) & m;
  }
  // The power supply's hum (the mains and its harmonics through the preamp's grounds) under it all.
  static const double kHum = knob("PJ_TONEWHEEL_HUM", 1.0) * 3e-4;
  const double hum = kHum * (std::sin(2 * M_PI * 60 * t_) + 0.5 * std::sin(2 * M_PI * 120 * t_ + 1) + 0.3 * std::sin(2 * M_PI * 180 * t_ + 2));
  // The run switch: off, the generator coasts down on its own inertia, its pitch sinking and its
  // voltage falling with its speed (the wheels' pickups); on again, the synchronous motor pulls it
  // back up within a few tenths of a second (PJ_TONEWHEEL_COAST: the coasting's time constant, s).
  if (!run_ || speed_ != 1) {
    static const double kCoast = knob("PJ_TONEWHEEL_COAST", 4.0);
    if (run_) speed_ += (1 - speed_) * dt / 0.3;
    else speed_ *= 1 - dt / kCoast;
    if (run_ && speed_ > 0.9999) speed_ = 1;
  }
  // The preamp: the expression pedal (~35 dB, the bass held up as it closes), the tube's soft knee.
  expr_ += (exprTarget_ - expr_) * dt / 0.03;
  const double g = fastPow(10.0, -35 * fastPow(1 - std::clamp(expr_, 0.0, 1.0), 1.3) / 20);
  const double ripple = 1 + 0.002 * std::sin(2 * M_PI * 120 * t_);  // (the B+ ripple: 120 Hz, a fifth of a percent)
  // Microphonics: the console's valves hear the room. The Rotaries' sound reaching the console
  // (~2 m: ~6 ms) shakes the valves' electrodes (a mass on a spring: flat below its resonance of a
  // few kHz, Q ~8, peaking there). Moving the grid against the cathode, it modulates the stage's
  // gain (~1 % at playing levels, PJ_TONEWHEEL_MICROMOD): the organ's signal multiplied by the
  // turning speakers' sound, sidebands moving with the rotors; and it adds a little of it, ~40 dB
  // down (PJ_TONEWHEEL_MICRO, dB). A faint loop through the Rotaries (its gain far under 1).
  double micro = 0, vib = 0;
  {
    static const double kMicro = fastPow(10.0, knob("PJ_TONEWHEEL_MICRO", -22) / 20);  // (of what they hear: ~40 dB under the signal)
    static const double w0 = 2 * M_PI * knob("PJ_TONEWHEEL_MICROHZ", 3300) / fs, q = 8;
    static const double al = std::sin(w0) / (2 * q), a0 = 1 + al, cw = std::cos(w0);
    static const double lb0 = (1 - cw) / 2 / a0, lb1 = (1 - cw) / a0, a1 = -2 * cw / a0, a2 = (1 - al) / a0;
    const int len = int(microRing_.size());
    const double heard = microRing_[(microPos_ - int(0.006 * fs) + len) % len];
    vib = lb0 * heard + lb1 * microZ_[0] + lb0 * microZ_[1] - a1 * microZ_[2] - a2 * microZ_[3];  // (the electrode's displacement)
    microZ_[1] = microZ_[0];
    microZ_[0] = heard;
    microZ_[3] = microZ_[2];
    microZ_[2] = vib;
    micro = kMicro * vib;
  }
  // Each Rotary its own mix of the manuals (mix_: placeCabinets), through its own channel of the
  // console's amplification (the expression's bass, the tone, the valve stage, its overload): each
  // cabinet's amplifier distorts its own mix. One Rotary (or none): all of each.
  float rays[4][Rotary::kRays];
  const int nc = reg_.rotary ? int(cab_.size()) : 1;
  // (several cabinets: each fed the organ down, together as loud as one: their low ends add in
  // phase in the room, more than unrelated sounds would; measured)
  static const float kShare[5] = {1, 1, float(knob("PJ_ROTARY_TWOGAIN", 0.54)), 0.37f, float(knob("PJ_ROTARY_FOURGAIN", 0.38))};
  const float share = kShare[nc];
  static const double kGain = fastPow(10.0, knob("PJ_TONEWHEEL_BRIGHT", 3.0) / 20) - 1;  // (the tone: the mids and highs up)
  static const double kA = 1 - std::exp(-2 * M_PI * 1000 / fs), kShelfA = 1 - std::exp(-2 * M_PI * 250 / fs);
  static const double kValve = knob("PJ_TONEWHEEL_VALVE", 1.0) * 0.45;
  static const double kMod = knob("PJ_TONEWHEEL_MICROMOD", 0.6);  // (the valves hear ~1/8 of the signal: ~1 % of gain)
  rng_ = rng_ * 1664525u + 1013904223u;
  const double hiss = 2e-6 * ((rng_ >> 8) * (2.0 / 16777216.0) - 1);  // (the preamp's hiss)
  double y = 0, yc[4];
  float l = 0, r = 0;
  for (int c = 0; c < nc; ++c) {
    const double* w = nc > 1 ? cab_[c].mix : nullptr;
    const double x = ((w ? w[0] * U + w[1] * L + w[2] * pedal : U + L + pedal) + bg) * speed_ + hum;
    Chan& ch = chan_[c];
    ch.shelf += kShelfA * (x - ch.shelf);
    double v = g * (x + 1.2 * (1 - expr_) * ch.shelf) * kConsole * ripple + micro;
    // The preamp's tone: the mids and highs 3 dB up (above ~1 kHz, a first-order shelf:
    // PJ_TONEWHEEL_BRIGHT dB; by ear, the user's: it was a little dark).
    ch.tone += kA * (v - ch.tone);
    v += kGain * (v - ch.tone);
    // The valve stage: a single-ended triode bends one way more than the other, its 2nd harmonic
    // growing with the level (PJ_TONEWHEEL_VALVE); the grid shaken against the cathode by the room's
    // sound (the gain moving with it); then its overload.
    v *= 1 + kMod * vib;
    v += kValve * v * v;
    ch.dc += (1 - std::exp(-2 * M_PI * 8 / fs)) * (v - ch.dc);
    v -= ch.dc;
    v = std::tanh(1.5 * v) / 1.5 + hiss;
    yc[c] = v;
    y += v / nc;
    static const bool kMixLog = std::getenv("PJ_ROTARY_MIXLOG") != nullptr;  // (each cabinet's input level, every second)
    if (kMixLog) {
      static double e[4] = {};
      e[c] += v * v;
      if (c == nc - 1 && std::fmod(t_, 1.0) < dt) {
        std::string d = strf("rotary inputs t %.0f:", t_);
        for (int k = 0; k < nc; ++k) d += strf(" %.1f dB", 10 * std::log10(e[k] * dt + 1e-20)), e[k] = 0;
        say(stderr, "%s\n", d.c_str());
      }
    }
  }
  const float console = float(y * 8);  // (dry: the console's line, as a tone cabinet would play it)
  if (reg_.rotary) {
    for (int c = 0; c < nc; ++c) {
      float cl = console, cr = console;
      cab_[c].les.process(float(yc[c]) * share, cl, cr, inRoom_ ? rays[c] : nullptr);
      l += cl;
      r += cr;
    }
  } else {
    l = r = console;
    for (float& v : rays[0]) v = console;
  }
  {  // (what the console's valves hear, in the console's units: in the room the Rotaries' rays)
    double h = 0.5 * (l + r) / 8;
    if (inRoom_ && reg_.rotary) {
      h = 0;
      for (int c = 0; c < nc; ++c)
        for (int k = 0; k < Rotary::kRays; ++k) h += rays[c][k];
      h /= 8.0 * Rotary::kRays * share * nc;
    }
    microRing_[microPos_] = float(h);
  }
  microPos_ = (microPos_ + 1) % int(microRing_.size());
  if (inRoom_) {
    // The rays into the room: the listener's microphones, a pair 2 m apart aimed out to the
    // sides, D m in front of the cabinets (PJ_ROTARY_LISTENER, 4: past a hall's critical
    // distance), each hearing every cabinet's rays by their own paths (image sources: directly and
    // off the floor, the side walls 4 m off, the back wall 1.5 m behind, its corners), each path
    // fed by the rays leaving the cabinet toward it (placeCabinets). Each microphone hears each
    // cabinet's swirl its own way, as ears in a room do. The room's late field is fed the rays'
    // mean (all round: its level steady).
    double el = 0, er = 0, mean = 0;
    const int len = int(cab_[0].ring[0].size()), mask = len - 1;
    for (int c = 0; c < nc; ++c) {
      Cab& cb = cab_[c];
      for (int k = 0; k < Rotary::kRays; ++k) {
        cb.ring[k][rayPos_] = rays[c][k];
        mean += rays[c][k];
      }
      for (int p = 0; p < 7; ++p)
        for (int m = 0; m < 2; ++m) {
          const int at = (rayPos_ - cb.dly[p][m]) & mask;
          (m == 0 ? el : er) += cb.g[p][m][0] * cb.ring[cb.ray[p][m][0]][at] + cb.g[p][m][1] * cb.ring[cb.ray[p][m][1]][at];
        }
    }
    rayPos_ = (rayPos_ + 1) & mask;
    earlyL_.push_back(float(kOut * el * outScale()));
    earlyR_.push_back(float(kOut * er * outScale()));
    last_ = float(kOut * mean / Rotary::kRays);
    side_.push_back(0);
  } else {
    last_ = float(kOut * 0.5 * (l + r));
    side_.push_back(float(outScale() * kOut * 0.5 * (l - r)));
  }
  return finish(last_);
}

// Each cabinet's paths to the listener's microphones (2 m apart, aimed 55 degrees out to their
// sides, D m in front): for each image of the cabinet (the walls 4 m either side, the back wall 1.5
// m behind, its corners, the floor 1 m down), the direction leaving the cabinet toward it (the
// path to the microphone, mirrored back by its walls), in the cabinet's own frame (its facing):
// the two rays either side of it blended (they are 60 degrees apart). (One cabinet at the origin
// facing the listener: the paths as before, but each now fed by the rays toward it, the direct one
// mostly the front ray with a little of the +-60 ones, not the front ray alone.)
void TonewheelModel::placeCabinets() {
  static const double kD = knob("PJ_ROTARY_LISTENER", 4.0), kC = 343, kW = 4.0, kH = 1.0;
  // (the back wall 1.5 m behind the rearmost cabinet)
  double kB = 1.5;
  for (const Cab& cb : cab_) kB = std::max(kB, 1.5 - cb.x);
  static const double kRefl[7] = {1.0, 0.75, 0.75, 0.75, 0.56, 0.56, 0.6 * 0.8};
  // (the rays' angles in order round the cabinet, -180 .. 180, and which ray each is)
  static const double kAng[7] = {-180, -120, -60, 0, 60, 120, 180};
  static const int kRay[7] = {5, 4, 2, 0, 1, 3, 5};
  const int len = int(cab_[0].ring[0].size());
  for (Cab& cb : cab_)
    for (int m = 0; m < 2; ++m) {
      const double mx = kD, my = m == 0 ? 1 : -1;
      for (int p = 0; p < 7; ++p) {
        // the image: mirrored in y (side walls), in x (the back wall), both (corners)
        const bool fy = p == 1 || p == 2 || p == 4 || p == 5, fx = p == 3 || p == 4 || p == 5;
        const double wy = (p == 1 || p == 4) ? kW : -kW;
        const double ix = fx ? -2 * kB - cb.x : cb.x, iy = fy ? 2 * wy - cb.y : cb.y;
        double vx = mx - ix, vy = my - iy;
        const double flat = std::hypot(vx, vy);
        const double dist = p == 6 ? std::hypot(flat, 2 * kH) : flat;
        // (the microphone aimed out to its side: between omni and cardioid)
        const double arrive = std::atan2(iy - my, ix - mx) - (m == 0 ? 1 : -1) * 55 * M_PI / 180 - M_PI;
        const double aim = 0.6 + 0.4 * std::cos(arrive);
        // (the direction the sound leaves the cabinet: the path mirrored back by its walls)
        if (fy) vy = -vy;
        if (fx) vx = -vx;
        double a = (std::atan2(vy, vx) - cb.rot) * 180 / M_PI;
        a = std::remainder(a, 360.0);
        int seg = 0;
        while (seg < 5 && a > kAng[seg + 1]) ++seg;
        const double w = std::clamp((a - kAng[seg]) / 60, 0.0, 1.0);
        const double gain = kRefl[p] * aim / dist;
        cb.dly[p][m] = std::min(int(dist / kC * fs_), len - 1);
        cb.ray[p][m][0] = kRay[seg];
        cb.ray[p][m][1] = kRay[seg + 1];
        cb.g[p][m][0] = gain * (1 - w);
        cb.g[p][m][1] = gain * w;
      }
    }
}

// (called by the room each block: from then on the Rotary radiates into it)
void TonewheelModel::takeEarly(int n, float* l, float* r, float g) {
  inRoom_ = true;
  const int have = int(earlyL_.size()), m = std::min(n, have);
  for (int i = 0; i < m; ++i) {
    l[n - m + i] += g * earlyL_[have - m + i];
    r[n - m + i] += g * earlyR_[have - m + i];
  }
  earlyL_.clear();
  earlyR_.clear();
}

const float* TonewheelModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

// --- The player.

TonewheelPlayer::TonewheelPlayer(TonewheelModel& m) : Performer(m), m_(m) {
  std::fill(heldDiv_, heldDiv_ + 128, -1);
  rotaryFast_ = m.registration().rotaryFast;
  // PJ_TONEWHEEL_KNOBS="k1,k2,k3,k4" (0..1): the drawbar knobs set at the start (tests; offline).
  if (const char* e = std::getenv("PJ_TONEWHEEL_KNOBS")) {
    double k[4];
    if (std::sscanf(e, "%lf,%lf,%lf,%lf", &k[0], &k[1], &k[2], &k[3]) == 4) {
      for (int i = 0; i < 4; ++i) setKnob(i, k[i]);
      m.placeDrawbars();  // (set before playing: the sliders already there)
    }
  }
}

void TonewheelPlayer::noteOn(int midi, int velocity) {
  // Played live (no plan): the pedals below C2, the upper manual above.
  noteOnManual(midi < 36 && m_.registration().pedal[0] + m_.registration().pedal[1] > 0 ? TonewheelModel::kPedal
                                                                                          : TonewheelModel::kUpper,
               midi, velocity);
}

void TonewheelPlayer::noteOff(int midi) {
  noteOffManual(heldDiv_[midi & 127] >= 0 ? heldDiv_[midi & 127] : TonewheelModel::kUpper, midi);
}

void TonewheelPlayer::noteOnManual(int div, int midi, int velocity) {
  articulate(midi, velocity, held_ > 0);
  note_ = midi;
  blowing_ = true;
  if (heldDiv_[midi & 127] < 0) ++held_;
  heldDiv_[midi & 127] = div;
  m_.press(div, midi, velocity);
  velSmooth_ += 0.15 * (velocity - velSmooth_);
  rate_ += 1;
  sinceNote_ = 0;
}

void TonewheelPlayer::noteOffManual(int div, int midi) {
  if (heldDiv_[midi & 127] >= 0) --held_;
  heldDiv_[midi & 127] = -1;
  m_.release(div, midi);
}

void TonewheelPlayer::setRotary(int state) {
  rotarySeen_ = true;
  rotaryFast_ = state == 2;
  if (state == 0) m_.setRotaryBrake();
  else m_.setRotaryFast(rotaryFast_);
}

void TonewheelPlayer::setHold(bool h) {
  rotarySeen_ = true;
  rotaryFast_ = h;
  m_.setRotaryFast(h);
}

// The riders (research/tonewheel_character.md §5.4): with no expression pedal in the file the
// swell follows the velocities (slowly: a foot, not a key); with no Rotary switch the player
// turns it fast as a phrase builds (more, louder notes than the piece's usual) and slow again when
// it eases or rests, never sooner than a few seconds after the last change.
void TonewheelPlayer::ride(double dt) {
  if (breathCtl_ != lastBreath_) exprSeen_ = true;
  lastBreath_ = breathCtl_;
  if (flutter_ != lastFlutter_) {
    rotarySeen_ = true;
    rotaryFast_ = flutter_ >= 0.5;
    m_.setRotaryFast(rotaryFast_);
  }
  lastFlutter_ = flutter_;
  if (exprSeen_) m_.setExpression(std::clamp(exprShape_ * (0.5 + breathCtl_), 0.0, 1.0));
  else m_.setExpression(std::clamp(exprShape_ * std::clamp(0.45 + 0.55 * (velSmooth_ - 20) / 107, 0.35, 1.0), 0.0, 1.0));
  // The intensity: notes a second, weighted by loudness, over ~2 s; the piece's usual over ~20 s.
  sinceSwitch_ += dt;
  sinceNote_ += dt;
  const double now = rate_ / dt;
  rate_ = 0;
  intensity_ += (now * velSmooth_ / 100 - intensity_) * dt / 2.0;
  usual_ += (intensity_ - usual_) * dt / 20.0;
  if (rotarySeen_) return;
  // (A registration that sets the Rotary fast, the rock organ's, is at home there: it slows only
  // in a real rest.)
  const bool home = m_.registration().rotaryFast;
  if (!rotaryFast_ && sinceSwitch_ > 4 && held_ > 0 &&
      (home ? sinceNote_ < 0.1 : intensity_ > std::max(3.0, 1.5 * usual_))) {
    rotaryFast_ = true;
    sinceSwitch_ = 0;
    m_.setRotaryFast(true);
  } else if (rotaryFast_ && sinceSwitch_ > 3 &&
             (home ? sinceNote_ > 2.0
                   : sinceNote_ > 0.8 || intensity_ < 0.8 * usual_ || (sinceSwitch_ > 8 && urand() < dt / 4))) {
    rotaryFast_ = false;
    sinceSwitch_ = 0;
    m_.setRotaryFast(false);
  }
}

void TonewheelPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double dt = m / fs_;
    tick(dt);
    ride(dt);
    // PJ_TONEWHEEL_KNOBS2="t,k1,k2,k3,k4": the knobs turned at t s (tests; demos).
    static const char* later = std::getenv("PJ_TONEWHEEL_KNOBS2");
    if (later && !laterDone_) {
      double t, k[4];
      if (std::sscanf(later, "%lf,%lf,%lf,%lf,%lf", &t, &k[0], &k[1], &k[2], &k[3]) == 5 && now_ >= t) {
        for (int j = 0; j < 4; ++j) setKnob(j, k[j]);
        laterDone_ = true;
      }
    }
    m_.process(out + i, m);
  }
  blowing_ = held_ > 0;
}

// --- The drawbar knobs (live K1-K4): the tradition's own paths, not a linear map.

namespace {

// K1, soft to hard: sixteen of the most used registrations, in the order an organist pulls them as
// a tune grows, piecewise linear between them (research/tonewheel_organ.md: the drawbar knobs).
struct Station { const char* bars; const char* name; };
constexpr Station kPath[] = {
    {"008000000", "a whisper: the 8' alone"},
    {"008400000", "a small flute"},
    {"808000000", "soft foundation"},
    {"838000000", "the ballad"},
    {"868000000", "mellow"},
    {"688600000", "the gospel lead"},
    {"888000000", "the jazz organ"},
    {"888000008", "jazz with the 1' sparkle"},
    {"888400000", "jazz, opening up"},
    {"888600000", "a lead"},
    {"888800000", "gospel, rock"},
    {"888808000", "the gospel shout"},
    {"888880000", "rock"},
    {"888888000", "rock, full"},
    {"888888880", "nearly all out"},
    {"888888888", "full organ"},
};
constexpr int kStations = sizeof(kPath) / sizeof(kPath[0]);

double smooth01(double x) {
  x = std::clamp(x, 0.0, 1.0);
  return x * x * (3 - 2 * x);
}

}  // namespace

// The drawbars at the knobs (each 0..1; the middle of K2-K4 changes nothing). K1 walks the
// stations (detents); K4 then sets the depth (16', 5 1/3'), K2 hollow or full, K3 dark or bright;
// whole steps.
std::array<double, 9> TonewheelPlayer::drawbarsAt(const double k[4], const char** classic) {
  // K1: between two stations, the drawbars that differ move one step at a time.
  const double pos = std::clamp(k[0], 0.0, 1.0) * (kStations - 1);
  const int s0 = std::min(int(pos), kStations - 2);
  double f = pos - s0;
  static const double kDetent = knob("PJ_TONEWHEEL_DETENT", 0.0);  // (a flat stretch at each station, of its span: 0 piecewise linear)
  if (kDetent > 0) f = f < kDetent ? 0 : f > 1 - kDetent ? 1 : (f - kDetent) / (1 - 2 * kDetent);
  double b[9];
  for (int d = 0; d < 9; ++d) b[d] = (kPath[s0].bars[d] - '0') * (1 - f) + (kPath[s0 + 1].bars[d] - '0') * f;
  const double top = std::max({b[0], b[2], 1.0});
  // The other knobs' detented middles.
  auto bend = [](double x) { const double v = 2 * std::clamp(x, 0.0, 1.0) - 1; return std::fabs(v) < 0.1 ? 0.0 : (v - std::copysign(0.1, v)) / 0.9; };
  const double depth = bend(k[3]), body = bend(k[1]), light = bend(k[2]);
  // K4: light (no 16', 5 1/3') .. deep (the 16' growl).
  if (depth < 0) { b[0] *= 1 + depth; b[1] *= 1 + depth; }
  else { b[0] = std::max(b[0], 8 * depth); b[1] = std::max(b[1], 6 * depth * (b[1] > 0 || b[0] > 4 ? 1 : 0.5)); }
  // K2: hollow (the odd harmonics of the 8': 8', 2 2/3', 1 3/5', the clarinet's sound) .. full
  // (the octaves and mutations filled in: a chorus).
  if (body < 0) {
    const double h = -body;
    b[0] *= 1 - 0.6 * h; b[1] *= 1 - h; b[3] *= 1 - h; b[5] *= 1 - h; b[7] *= 1 - h; b[8] *= 1 - h;
    b[4] = std::max(b[4], 0.9 * h * top);
    b[6] = std::max(b[6], 0.7 * h * top);
  } else if (body > 0) {
    static const double kFull[9] = {1, 1, 1, 1, 0.9, 0.85, 0.6, 0.6, 0.7};
    for (int d = 0; d < 9; ++d) b[d] += body * std::max(0.0, kFull[d] * top - b[d]);
  }
  // K3: dark (the top taken away) .. bright (the whistle: 1', 1 1/3', 2').
  if (light < 0) {
    for (int d = 5; d < 9; ++d) b[d] *= 1 + light;
    b[3] *= 1 + 0.5 * light;
    b[4] *= 1 + 0.5 * light;
  } else if (light > 0) {
    b[8] = std::max(b[8], 8 * light);
    b[7] = std::max(b[7], 6 * light * light);
    b[5] = std::max(b[5], 5 * light * light);
  }
  std::array<double, 9> out{};
  for (int d = 0; d < 9; ++d) out[d] = std::clamp(b[d], 0.0, 8.0);
  if (classic) {  // (on a station: all its drawbars within a twentieth of a step)
    *classic = nullptr;
    for (const Station& st : kPath) {
      bool on = true;
      for (int d = 0; d < 9 && on; ++d) on = std::fabs(out[d] - (st.bars[d] - '0')) < 0.05;
      if (on) *classic = st.name;
    }
  }
  return out;
}

bool TonewheelPlayer::setKnob(int k, double x) {
  if (k == 4) {  // K5: the overdrive: the 122's amp driven harder, 1 x (clean, as registered) to 8 x (grit)
    overdrive_ = std::exp(std::log(8.0) * std::clamp(x, 0.0, 1.0));
    const int tenths = int(std::lround(10 * overdrive_));
    if (tenths != lastOverdrive_) {
      say(stderr, "overdrive x %.1f\n", overdrive_);
      lastOverdrive_ = tenths;
    }
    if (knobs_[0] < 0) {
      knobs_[0] = 6.0 / (kStations - 1);
      knobs_[1] = knobs_[2] = knobs_[3] = 0.5;
      baseDrive_ = m_.registration().drive;
    }
    TonewheelRegistration r = m_.registration();
    r.drive = baseDrive_ * (1 + 1.5 * smooth01((knobs_[0] - 0.7) / 0.3)) * overdrive_;
    r.rotaryFast = rotaryFast_;
    m_.setRegistration(r);
    return true;
  }
  if (k == 5) {  // K6: the Rotary's half-moon switch: stop (left), slow (middle), fast (right)
    const int state = x < 1.0 / 3 ? 0 : x < 2.0 / 3 ? 1 : 2;
    if (state != lastRotaryKnob_) {
      static const char* const kNames[3] = {"stop (brake)", "slow (chorale)", "fast (tremolo)"};
      say(stderr, "Rotary: %s\n", kNames[state]);
      lastRotaryKnob_ = state;
      setRotary(state);
    }
    return true;
  }
  if (k < 0 || k > 3) return false;
  if (knobs_[0] < 0) {  // (the knobs not yet moved: K1 at the jazz organ, the others at their middles)
    knobs_[0] = 6.0 / (kStations - 1);  // (the jazz organ)
    knobs_[1] = knobs_[2] = knobs_[3] = 0.5;
    baseDrive_ = m_.registration().drive;
  }
  knobs_[k] = std::clamp(x, 0.0, 1.0);
  const char* name = nullptr;
  TonewheelRegistration r = m_.registration();
  const std::array<double, 9> goal = drawbarsAt(knobs_, &name);
  for (int d = 0; d < 9; ++d) r.upper[d] = int(std::lround(goal[d]));  // (the registration's text)
  // (K1's hard end: the 122 pushed harder too, the grit of a shout)
  r.drive = baseDrive_ * (1 + 1.5 * smooth01((knobs_[0] - 0.7) / 0.3)) * overdrive_;
  r.rotaryFast = rotaryFast_;
  m_.setRegistration(r);
  m_.setUpperGoal(goal);  // (the sliders glide there, not in steps)
  const std::array<double, 9> lower = tonewheelLowerFor(goal);  // (the accompaniment's manual follows)
  m_.setLowerGoal(lower);
  const std::array<double, 2> pedal = tonewheelPedalFor(goal);  // (and the pedals, by its loudness and brightness)
  m_.setPedalGoal(pedal);
  std::string bars;
  for (int d = 0; d < 9; ++d) {
    char b[8];
    std::snprintf(b, sizeof b, "%s%.1f", d ? " " : "", goal[d]);
    bars += b;
  }
  // (printed when a drawbar has moved half a step since, or a station is reached)
  bool moved = lastName_ != name;
  for (int d = 0; d < 9; ++d) moved |= std::fabs(goal[d] - printed_[d]) >= 0.5;
  if (moved) {
    std::string low;
    for (int d = 0; d < 9; ++d) {
      char b[8];
      std::snprintf(b, sizeof b, "%s%.1f", d ? " " : "", lower[d]);
      low += b;
    }
    say(stderr, "drawbars %s%s%s (drive %.1f); lower %s; pedal %.1f %.1f\n", bars.c_str(), name ? ": " : "", name ? name : "",
                 r.drive, low.c_str(), pedal[0], pedal[1]);
    for (int d = 0; d < 9; ++d) printed_[d] = goal[d];
    lastName_ = name;
  }
  return true;
}

}  // namespace pajupilli
