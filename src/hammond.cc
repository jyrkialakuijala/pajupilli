#include "hammond.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"

namespace pajupilli {

bool hammondInstrument(const Instrument& inst) { return std::strcmp(inst.family, "hammond") == 0; }

namespace {

// The 12 gear pairs (research/hammond_b3.md §3.1): a wheel turns at 20 x ratio a second.
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

double HammondModel::wheelHz(int n) { return 20 * kRatio[gearOf(n)] * teethOf(n); }

// The lower manual for an upper one (research/hammond_b3.md: the drawbar knobs): the melody's
// accompaniment, from the same family but softer and darker, as organists derive it. Each drawbar
// keeps a share of the upper's (the foundation all of it, the 5 1/3' half: less buzz under the
// tune; the top little), then a step less (~3 dB under).
std::array<double, 9> hammondLowerFor(const std::array<double, 9>& upper) {
  static const double kKeep[9] = {0.85, 0.45, 1.0, 0.55, 0.45, 0.3, 0.25, 0.2, 0.15};
  static const double kSofter = knob("PJ_HAMMOND_LOWER_SOFTER", 1.0);  // (steps)
  std::array<double, 9> lower{};
  for (int d = 0; d < 9; ++d) lower[d] = std::clamp(upper[d] * kKeep[d] - (upper[d] > 0 ? kSofter : 0), 0.0, 8.0);
  return lower;
}

// The pedals for an upper manual (its two drawbars, 16' and 8'): how loud the upper is sets the
// 16' (a whisper over a booming pedal would be wrong), how bright it is the 8' (round registrations
// want the simple 16', bright ones a brighter pedal).
std::array<double, 2> hammondPedalFor(const std::array<double, 9>& upper) {
  auto gain = [](double x) { return x >= 1 ? std::pow(10.0, -3.0 * (8 - std::min(x, 8.0)) / 20) : std::max(x, 0.0) * std::pow(10.0, -21.0 / 20); };
  double all = 0, high = 0;
  for (int d = 0; d < 9; ++d) {
    all += gain(upper[d]);
    if (d >= 3) high += gain(upper[d]);
  }
  const double loud = std::clamp(all / 3, 0.0, 1.0);          // (three drawbars out full: as loud as a pedal wants)
  const double bright = all > 0 ? high / all : 0;             // (the share of the 4' and above)
  return {std::clamp(4.5 + 3.5 * loud, 0.0, 8.0), std::clamp(1.5 + 8 * bright, 0.0, 8.0)};
}

std::string HammondRegistration::text() const {
  auto bars = [](const int* b, int n) { std::string s; for (int i = 0; i < n; ++i) s += char('0' + b[i]); return s; };
  static const char* const kVib[7] = {"off", "V1", "V2", "V3", "C1", "C2", "C3"};
  char buf[256];
  std::snprintf(buf, sizeof buf, "upper %s, lower %s, pedal %s, percussion %s, vibrato %s, Leslie %s, drive %.1f",
                bars(upper.data(), 9).c_str(), bars(lower.data(), 9).c_str(), bars(pedal.data(), 2).c_str(),
                perc ? (std::string(percThird ? "3rd" : "2nd") + (percFast ? " fast" : " slow") + (percSoft ? " soft" : "")).c_str() : "off",
                kVib[std::clamp(vibrato, 0, 6)], leslieFast ? "fast" : "slow", drive);
  return buf;
}

// research/hammond_b3.md §13.3: 17 the jazz organ's 888 with C3 and the Leslie slow; 18 with the
// percussion (3rd, fast); 19 the rock organ: four drawbars out, the Leslie fast, the amp hot.
HammondRegistration hammondPreset(const std::string& name) {
  HammondRegistration r;
  if (name == "percussive-organ") {
    r.perc = true;
    r.vibrato = 0;
  } else if (name == "rock-organ") {
    r.upper = {8, 8, 8, 8, 0, 0, 0, 0, 0};
    r.vibrato = 0;
    r.leslieFast = true;
    r.drive = 3.5;
  }
  hammondOverrides(r);
  return r;
}

void hammondOverrides(HammondRegistration& r) {
  // Overrides: PJ_HAMMOND_UPPER / _LOWER (nine digits), _PEDAL (two), _PERC ("off", "3f", "2s",
  // "3fs": the harmonic, fast or slow, soft), _VIB ("off", "V1".."C3"), _LESLIE ("slow", "fast",
  // "off": dry).
  auto bars = [](const char* e, int* b, int n) {
    if (!e) return;
    for (int i = 0; i < n && e[i]; ++i) b[i] = std::clamp(e[i] - '0', 0, 8);
  };
  bars(std::getenv("PJ_HAMMOND_UPPER"), r.upper.data(), 9);
  bars(std::getenv("PJ_HAMMOND_LOWER"), r.lower.data(), 9);
  bars(std::getenv("PJ_HAMMOND_PEDAL"), r.pedal.data(), 2);
  if (const char* e = std::getenv("PJ_HAMMOND_PERC")) {
    const std::string p = e;
    r.perc = p != "off";
    if (r.perc) {
      r.percThird = p[0] == '3';
      r.percFast = p.size() < 2 || p[1] != 's';
      r.percSoft = p.find('s', 2) != std::string::npos;
    }
  }
  if (const char* e = std::getenv("PJ_HAMMOND_VIB")) {
    const std::string v = e;
    r.vibrato = v == "off" ? 0 : (v[0] == 'C' ? 3 : 0) + std::clamp(v.size() > 1 ? v[1] - '0' : 3, 1, 3);
  }
  if (const char* e = std::getenv("PJ_HAMMOND_LESLIE")) {
    const std::string l = e;
    r.leslie = l != "off";
    r.leslieFast = l == "fast";
  }
}

HammondModel::HammondModel(const Instrument& inst) : Tube(inst) {
  const double fs = inst.fs;
  const uint32_t serial = uint32_t(knob("PJ_HAMMOND_SERIAL", 1957));
  const double age = knob("PJ_HAMMOND_AGE", 1.0);
  rng_ = serial * 2654435761u + 5u;
  for (int g = 0; g < 12; ++g) {
    turnsPerSec_[g] = 20 * kRatio[g];
    inc_[g] = uint32_t(std::llround(turnsPerSec_[g] / fs * 4294967296.0));
    Rng r{serial * 7919u + uint32_t(g) * 104729u};
    phase_[g] = uint32_t(r.u() * 4294967296.0);  // (where each shaft stood when the motor was started)
  }
  // Each wheel: one revolution of its pickup's voltage (research/hammond_b3.md §3.3, §4).
  table_.resize(kWheels + 1);
  for (int n = 1; n <= kWheels; ++n) {
    gearOf_[n] = gearOf(n);
    partner_[n] = n <= 36 ? n + 48 : n <= 41 ? 0 : n <= 48 ? n + 43 : n <= 84 ? n - 48 : n - 43;
    const int T = teethOf(n);
    int N = 1024;
    while (N < 32 * T) N <<= 1;
    Rng r{serial * 31337u + uint32_t(n) * 2246822519u};
    // The wheel's eccentricity and ovality: its tone swells and fades once a revolution.
    const double ma = age * 0.05 * (0.3 + std::fabs(r.n())), pa = 2 * M_PI * r.u();
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
    double h[6] = {0, 1, 0, 0, 0, 0}, ph[6] = {};
    if (n <= 12) {
      h[2] = 0.02; h[3] = 0.25; h[5] = 0.08;
    } else {
      h[2] = std::pow(10.0, -35 / 20.0) * (1 + 0.3 * r.n());
      h[3] = std::pow(10.0, -38 / 20.0) * (1 + 0.3 * r.n());
    }
    for (int k = 2; k < 6; ++k) ph[k] = 2 * M_PI * r.u();
    // The tuned filter of wheels 49-91 (a capacitor and transformer per wheel), aged capacitors
    // tuning it lower.
    if (n >= 49) {
      const double q = 4, detune = 1 / std::sqrt(knob("PJ_HAMMOND_CAPS", 1.0) * (1 + 0.1 * age * std::fabs(r.n())));
      for (int k = 1; k < 6; ++k) {
        const double x = k / detune;
        h[k] /= std::sqrt(1 + q * q * (x - 1 / x) * (x - 1 / x));
      }
    }
    const double level = std::pow(10.0, 0.8 * age * r.n() / 20) / std::max(h[1], 1e-6);
    std::vector<float>& t = table_[n];
    t.resize(N);
    for (int i = 0; i < N; ++i) {
      const double phi = 2 * M_PI * i / N, u = double(i) * T / N;
      const int k = int(u) % T, k1 = (k + 1) % T;
      const double w = 0.5 * (1 - std::cos(M_PI * (u - std::floor(u))));
      const double th = 2 * M_PI * u + pos[k] + (pos[k1] - pos[k]) * w;
      const double am = 1 + ma * std::cos(phi - pa) + mo * std::cos(2 * phi - po) + dep[k] + (dep[k1] - dep[k]) * w;
      double v = 0;
      // (where the off-centre wheel runs closer, the flux bends more: its harmonics swell more
      // than its fundamental, once a revolution: the tone's colour breathes, not only its level)
      const double near = ma * std::cos(phi - pa);
      for (int j = 1; j < 6; ++j) v += h[j] * (1 + 0.8 * (j - 1) * near) * std::sin(j * th + ph[j]);
      t[i] = float(level * (am * v + dr * std::sin(phi - pd)));
    }
    // The whole generator's whisper (the pickups hear their neighbours: ~-70 dB each).
    background_[n] = std::pow(10.0, (-70 + 6 * r.n()) / 20) * (r.u() < 0.5 ? -1 : 1);
    // Its spring coupling: a torsional resonance of 3-8 Hz, Q ~3.
    flW_[n] = 2 * M_PI * (3 + 5 * r.u());
    flQ_[n] = 2 + 2 * r.u();
  }
  // The flutter's scale: each wheel's resonator run a while to find its spread, then set so that
  // a wheel turns ~1.5e-4 revolution either way (x age): a few tenths of a cent on the high
  // wheels, less low down (research/hammond_b3.md §3.1: the couplings absorb, not remove).
  {
    const double ctlDt = 32 / fs, sigma = knob("PJ_HAMMOND_FLUTTER", 1.0) * age * 1.5e-4;
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
  // The scanner's line (~0.8 ms).
  line_.assign(256, 0.0f);
  leslie_.setup(fs, serial);
  roomMics_ = std::getenv("PJ_LESLIE_MICS") == nullptr || std::string(std::getenv("PJ_LESLIE_MICS")) != "close";
  for (auto& ring : rayRing_) ring.assign(2048, 0.0f);
  setRegistration(hammondPreset(inst.name));
}

void HammondModel::setRegistration(const HammondRegistration& r) {
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
  leslie_.setFast(r.leslieFast);
  leslie_.setDrive(r.drive);
}

void HammondModel::setLowerGoal(const std::array<double, 9>& bars) {
  for (int d = 0; d < 9; ++d) goal_[kLower][d] = std::clamp(bars[d], 0.0, 8.0);
}

void HammondModel::setPedalGoal(const std::array<double, 2>& bars) {
  for (int d = 0; d < 2; ++d) goal_[kPedal][d] = std::clamp(bars[d], 0.0, 8.0);
}

void HammondModel::placeDrawbars() {
  for (int v = 0; v < 3; ++v)
    for (int d = 0; d < 9; ++d) {
      pos_[v][d] = goal_[v][d];
      integ_[v][d] = 0;
    }
  drawbarGains();
}

void HammondModel::setUpperGoal(const std::array<double, 9>& bars) {
  for (int d = 0; d < 9; ++d) goal_[kUpper][d] = std::clamp(bars[d], 0.0, 8.0);
}

// The gains of the sliders where they stand: 3 dB a step, below the first step fading linearly
// to silence (the drawbar's contact leaving its first tap).
void HammondModel::drawbarGains() {
  auto gain = [](double x) { return x >= 1 ? std::pow(10.0, -3.0 * (8 - std::min(x, 8.0)) / 20) : std::max(x, 0.0) * std::pow(10.0, -21.0 / 20); };
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
void HammondModel::moveDrawbars(double dt) {
  static const double kP = knob("PJ_HAMMOND_DRAWBAR_KP", 8), kI = knob("PJ_HAMMOND_DRAWBAR_KI", 20);
  static const double kMax = knob("PJ_HAMMOND_DRAWBAR_SPEED", 16);  // (steps a second)
  bool moving = false;
  for (int v = 0; v < 3; ++v)
    for (int d = 0; d < (v == kPedal ? 2 : 9); ++d) {
      const double e = goal_[v][d] - pos_[v][d];
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
// the key's speed; each bounces a few times (research/hammond_b3.md §6.2;
// research/hammond_character.md §3).
void HammondModel::schedule(Key& k, int velocity, bool closing) {
  Rng order{uint32_t(knob("PJ_HAMMOND_SERIAL", 1957)) * 97u + uint32_t(k.div * 128 + k.midi) * 2654435761u};
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
    const int bounces = int(r.u() * (closing ? 4 : 2));
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
}

void HammondModel::press(int div, int midi, int velocity) {
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
}

void HammondModel::release(int div, int midi) {
  const int lo = div == kPedal ? 24 : 36, hi = div == kPedal ? 48 : 96;
  while (midi < lo) midi += 12;
  while (midi > hi) midi -= 12;
  for (Key& k : keys_)
    if (k.div == div && k.midi == midi && k.down) {
      k.down = false;
      schedule(k, 64, false);
    }
}

void HammondModel::resetAir() {
  Tube::resetAir();
  keys_.clear();
  std::fill(line_.begin(), line_.end(), 0.0f);
  percEnv_ = 0;
  percCharge_ = 1;
}

float HammondModel::sample() {
  static const double kOut = knob("PJ_HAMMOND_OUT", 5.0), kConsole = 0.06;
  static const double kPartner = std::pow(10.0, -45 / 20.0);
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
    static const double kMains = knob("PJ_HAMMOND_MAINS", 1.0);
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
      const double e = (0.6 * common + 0.8 * noise()) / std::sqrt(cdt);
      const double a = e - flW_[n] / flQ_[n] * flV_[n] - flW_[n] * flW_[n] * flX_[n];
      flV_[n] += a * cdt;
      flX_[n] += flV_[n] * cdt;
      flOff_[n] = int32_t(flG_[n] * flX_[n] * 4294967296.0);
    }
  }
  int32_t off[12];
  for (int g = 0; g < 12; ++g) {
    phase_[g] += inc_[g] + int32_t(double(inc_[g]) * mains_);
    off[g] = int32_t(turnsPerSec_[g] * wob * 4294967296.0);
  }
  double bg = 0;
  for (int n = 1; n <= kWheels; ++n) {
    const std::vector<float>& t = table_[n];
    const uint32_t p = phase_[gearOf_[n]] + uint32_t(off[gearOf_[n]]) + uint32_t(flOff_[n]);
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
      // A dirty contact now and then breaks for a moment while held (PJ_HAMMOND_DIRT: 1 an
      // old, unserviced organ, ~1 crackle a second under a chord; 0 clean).
      static const double kDirt = knob("PJ_HAMMOND_DIRT", 0.3) * knob("PJ_HAMMOND_AGE", 1.0);
      if (c.brk > 0) c.brk -= dt;
      else if (c.closed && kDirt > 0) {
        dirtRng_ = dirtRng_ * 1664525u + 1013904223u;  // (its own draws: the wheels' flutter is the generator's alone)
        if ((dirtRng_ >> 8) * (1.0 / 16777216.0) < kDirt * 0.03 * dt) c.brk = 1e-4 + 1.9e-3 * ((dirtRng_ >> 4) & 0xfff) / 4096.0;
      }
      // The contact's resistance falls over ~0.15 ms as it closes, and breaks at once.
      const double target = c.closed && c.brk <= 0 ? 1 : 0;
      c.gain += (target - c.gain) * std::min(1.0, dt / (c.closed ? 1.5e-4 : 3e-5));
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
  }
  upperHeld_ = upperDown;
  if (!upperDown) percCharge_ += (1 - percCharge_) * dt / 0.034;
  percEnv_ *= 1 - dt / (reg_.percFast ? 0.375 : 1.55);
  const double percGain = reg_.perc ? std::clamp((percEnv_ - 0.6) / 0.4, 0.0, 1.0) * (reg_.percSoft ? 0.5 : 1.0) : 0.0;
  double upper = 0, lower = 0, pedal = 0;
  for (int d = 0; d < 9; ++d) {
    upper += gainU_[d] * bus[kUpper][d];
    lower += gainL_[d] * bus[kLower][d];
  }
  if (reg_.perc && !reg_.percSoft) upper *= 0.7;  // (the normal percussion drops the manual a little)
  upper += 1.4 * percGain * percBus;
  pedal = gainP_[0] * bus[kPedal][0] + gainP_[1] * bus[kPedal][1];
  // The scanner: a delay line read back and forth (6.87 Hz) by the rotating capacitor.
  const double vin = (reg_.vibUpper ? upper : 0) + (reg_.vibLower ? lower : 0);
  const double dry = upper + lower - vin;
  double vout = vin;
  if (reg_.vibrato > 0) {
    line_[linePos_] = float(vin);
    scanPh_ += 6.867 * (1 + mains_) * dt;  // (its synchronous motor follows the grid too)
    if (scanPh_ > 1) scanPh_ -= 1;
    const double tri = scanPh_ < 0.5 ? 2 * scanPh_ : 2 - 2 * scanPh_;
    static const double kDepth[4] = {0, 1.0 / 3, 0.5, 1};
    const double depth = kDepth[(reg_.vibrato - 1) % 3 + 1] * 0.55e-3 * fs;
    const double rp = linePos_ - (1 + depth * tri);
    const int i = int(std::floor(rp));
    const double f = rp - i;
    const int m = int(line_.size()) - 1;
    const double wet = line_[i & m] + (line_[(i + 1) & m] - line_[i & m]) * f;
    lineLp_ += (1 - std::exp(-2 * M_PI * 7000 / fs)) * (wet - lineLp_);
    linePos_ = (linePos_ + 1) & m;
    vout = reg_.vibrato >= 4 ? 0.7 * (vin + lineLp_) : lineLp_;
  }
  // The power supply's hum (the mains and its harmonics through the preamp's grounds) under it all.
  static const double kHum = knob("PJ_HAMMOND_HUM", 1.0) * 3e-4;
  const double hum = kHum * (std::sin(2 * M_PI * 60 * t_) + 0.5 * std::sin(2 * M_PI * 120 * t_ + 1) + 0.3 * std::sin(2 * M_PI * 180 * t_ + 2));
  double x = dry + vout + pedal + bg + hum;
  // The preamp: the expression pedal (~35 dB, the bass held up as it closes), the tube's soft knee.
  expr_ += (exprTarget_ - expr_) * dt / 0.03;
  shelf_ += (1 - std::exp(-2 * M_PI * 250 / fs)) * (x - shelf_);
  const double g = std::pow(10.0, -35 * std::pow(1 - std::clamp(expr_, 0.0, 1.0), 1.3) / 20);
  // (the B+ ripple: 120 Hz, a fifth of a percent, on the gain)
  double y = g * (x + 1.2 * (1 - expr_) * shelf_) * kConsole * (1 + 0.002 * std::sin(2 * M_PI * 120 * t_));
  y = std::tanh(1.5 * y) / 1.5;
  rng_ = rng_ * 1664525u + 1013904223u;
  y += 2e-6 * ((rng_ >> 8) * (2.0 / 16777216.0) - 1);  // (the preamp's hiss)
  float l = float(y * 8), r = l;  // (dry: the console's line, as a tone cabinet would play it)
  float rays[Leslie::kRays];
  for (float& v : rays) v = l;
  if (reg_.leslie) leslie_.process(float(y), l, r, inRoom_ ? rays : nullptr);
  if (inRoom_) {
    // The rays into the room: the listener's microphones, a pair 2 m apart aimed out to the
    // sides, D m in front of the cabinet (PJ_LESLIE_LISTENER, 4: past a hall's critical distance,
    // more room than cabinet), each hearing every ray by its own path (image sources): the front
    // ray directly and off the floor, the side rays off the side walls (4 m off), the rear ones
    // off the corners and the back wall (1.5 m behind the cabinet). Each path its own delay and
    // 1/r, each wall taking a quarter: the two microphones hear the swirl differently, as ears in
    // a room do. The room's late field is fed the rays' mean (all round: its level steady).
    static const double kD = knob("PJ_LESLIE_LISTENER", 4.0), kC = 343, kW = 4.0, kB = 1.5, kH = 1.0;
    static double path[Leslie::kRays + 1][2], gain[Leslie::kRays + 1][2];
    static bool init = false;
    if (!init) {
      for (int m = 0; m < 2; ++m) {
        const double my = m == 0 ? 1 : -1;  // (the left microphone on the cabinet's left: +y)
        auto set = [&](int k, double ix, double iy, double refl, double aim) {
          const double dist = std::hypot(kD - ix, my - iy);
          // (a microphone aimed out to its side, 55 degrees: between omni and cardioid)
          const double ang = std::atan2(iy - my, ix - kD) - (m == 0 ? 1 : -1) * 55 * M_PI / 180 - M_PI;
          path[k][m] = dist;
          gain[k][m] = refl * aim * (0.6 + 0.4 * std::cos(ang)) / dist;
        };
        set(0, 0, 0, 1.0, 1);                      // the front ray: direct
        set(1, 0, 2 * kW, 0.75, 1);                // +60: off the left wall
        set(2, 0, -2 * kW, 0.75, 1);               // -60: off the right wall
        set(3, -2 * kB, 2 * kW, 0.56, 1);          // +120: off the back wall and the left
        set(4, -2 * kB, -2 * kW, 0.56, 1);         // -120: off the back wall and the right
        set(5, -2 * kB, 0, 0.75, 1);               // 180: off the back wall
        path[6][m] = std::hypot(kD, 2 * kH);       // the front ray off the floor
        gain[6][m] = 0.6 * 0.8 / path[6][m];
      }
      init = true;
    }
    double el = 0, er = 0, mean = 0;
    const int len = int(rayRing_[0].size()), mask = len - 1;
    for (int k = 0; k < Leslie::kRays; ++k) {
      rayRing_[k][rayPos_] = rays[k];
      mean += rays[k];
    }
    for (int k = 0; k <= Leslie::kRays; ++k) {
      const int ray = k == Leslie::kRays ? 0 : k;
      for (int m = 0; m < 2; ++m) {
        const int dly = std::min(int(path[k][m] / kC * fs_), len - 1);
        (m == 0 ? el : er) += gain[k][m] * rayRing_[ray][(rayPos_ - dly) & mask];
      }
    }
    rayPos_ = (rayPos_ + 1) & mask;
    earlyL_.push_back(float(kOut * el * outScale()));
    earlyR_.push_back(float(kOut * er * outScale()));
    last_ = float(kOut * mean / Leslie::kRays);
    side_.push_back(0);
  } else {
    last_ = float(kOut * 0.5 * (l + r));
    side_.push_back(float(outScale() * kOut * 0.5 * (l - r)));
  }
  return finish(last_);
}

// (called by the room each block: from then on the Leslie radiates into it)
void HammondModel::takeEarly(int n, float* l, float* r, float g) {
  inRoom_ = true;
  const int have = int(earlyL_.size()), m = std::min(n, have);
  for (int i = 0; i < m; ++i) {
    l[n - m + i] += g * earlyL_[have - m + i];
    r[n - m + i] += g * earlyR_[have - m + i];
  }
  earlyL_.clear();
  earlyR_.clear();
}

const float* HammondModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

// --- The player.

HammondPlayer::HammondPlayer(HammondModel& m) : Performer(m), m_(m) {
  std::fill(heldDiv_, heldDiv_ + 128, -1);
  leslieFast_ = m.registration().leslieFast;
  // PJ_HAMMOND_KNOBS="k1,k2,k3,k4" (0..1): the drawbar knobs set at the start (tests; offline).
  if (const char* e = std::getenv("PJ_HAMMOND_KNOBS")) {
    double k[4];
    if (std::sscanf(e, "%lf,%lf,%lf,%lf", &k[0], &k[1], &k[2], &k[3]) == 4) {
      for (int i = 0; i < 4; ++i) setKnob(i, k[i]);
      m.placeDrawbars();  // (set before playing: the sliders already there)
    }
  }
}

void HammondPlayer::noteOn(int midi, int velocity) {
  // Played live (no plan): the pedals below C2, the upper manual above.
  noteOnManual(midi < 36 && m_.registration().pedal[0] + m_.registration().pedal[1] > 0 ? HammondModel::kPedal
                                                                                          : HammondModel::kUpper,
               midi, velocity);
}

void HammondPlayer::noteOff(int midi) {
  noteOffManual(heldDiv_[midi & 127] >= 0 ? heldDiv_[midi & 127] : HammondModel::kUpper, midi);
}

void HammondPlayer::noteOnManual(int div, int midi, int velocity) {
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

void HammondPlayer::noteOffManual(int div, int midi) {
  if (heldDiv_[midi & 127] >= 0) --held_;
  heldDiv_[midi & 127] = -1;
  m_.release(div, midi);
}

void HammondPlayer::setLeslie(int state) {
  leslieSeen_ = true;
  leslieFast_ = state == 2;
  if (state == 0) m_.setLeslieBrake();
  else m_.setLeslieFast(leslieFast_);
}

void HammondPlayer::setHold(bool h) {
  leslieSeen_ = true;
  leslieFast_ = h;
  m_.setLeslieFast(h);
}

// The riders (research/hammond_character.md §5.4): with no expression pedal in the file the
// swell follows the velocities (slowly: a foot, not a key); with no Leslie switch the player
// turns it fast as a phrase builds (more, louder notes than the piece's usual) and slow again when
// it eases or rests, never sooner than a few seconds after the last change.
void HammondPlayer::ride(double dt) {
  if (breathCtl_ != lastBreath_) exprSeen_ = true;
  lastBreath_ = breathCtl_;
  if (flutter_ != lastFlutter_) {
    leslieSeen_ = true;
    leslieFast_ = flutter_ >= 0.5;
    m_.setLeslieFast(leslieFast_);
  }
  lastFlutter_ = flutter_;
  if (exprSeen_) m_.setExpression(std::clamp(0.5 + breathCtl_, 0.0, 1.0));
  else m_.setExpression(std::clamp(0.45 + 0.55 * (velSmooth_ - 20) / 107, 0.35, 1.0));
  // The intensity: notes a second, weighted by loudness, over ~2 s; the piece's usual over ~20 s.
  sinceSwitch_ += dt;
  sinceNote_ += dt;
  const double now = rate_ / dt;
  rate_ = 0;
  intensity_ += (now * velSmooth_ / 100 - intensity_) * dt / 2.0;
  usual_ += (intensity_ - usual_) * dt / 20.0;
  if (leslieSeen_) return;
  // (A registration that sets the Leslie fast, the rock organ's, is at home there: it slows only
  // in a real rest.)
  const bool home = m_.registration().leslieFast;
  if (!leslieFast_ && sinceSwitch_ > 4 && held_ > 0 &&
      (home ? sinceNote_ < 0.1 : intensity_ > std::max(3.0, 1.5 * usual_))) {
    leslieFast_ = true;
    sinceSwitch_ = 0;
    m_.setLeslieFast(true);
  } else if (leslieFast_ && sinceSwitch_ > 3 &&
             (home ? sinceNote_ > 2.0
                   : sinceNote_ > 0.8 || intensity_ < 0.8 * usual_ || (sinceSwitch_ > 8 && urand() < dt / 4))) {
    leslieFast_ = false;
    sinceSwitch_ = 0;
    m_.setLeslieFast(false);
  }
}

void HammondPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double dt = m / fs_;
    tick(dt);
    ride(dt);
    // PJ_HAMMOND_KNOBS2="t,k1,k2,k3,k4": the knobs turned at t s (tests; demos).
    static const char* later = std::getenv("PJ_HAMMOND_KNOBS2");
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
// a tune grows, piecewise linear between them (research/hammond_b3.md: the drawbar knobs).
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
std::array<double, 9> HammondPlayer::drawbarsAt(const double k[4], const char** classic) {
  // K1: between two stations, the drawbars that differ move one step at a time.
  const double pos = std::clamp(k[0], 0.0, 1.0) * (kStations - 1);
  const int s0 = std::min(int(pos), kStations - 2);
  double f = pos - s0;
  static const double kDetent = knob("PJ_HAMMOND_DETENT", 0.0);  // (a flat stretch at each station, of its span: 0 piecewise linear)
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

bool HammondPlayer::setKnob(int k, double x) {
  if (k == 4) {  // K5: the Leslie's half-moon switch: stop (left), slow (middle), fast (right)
    const int state = x < 1.0 / 3 ? 0 : x < 2.0 / 3 ? 1 : 2;
    if (state != lastLeslieKnob_) {
      static const char* const kNames[3] = {"stop (brake)", "slow (chorale)", "fast (tremolo)"};
      std::fprintf(stderr, "Leslie: %s\n", kNames[state]);
      lastLeslieKnob_ = state;
      setLeslie(state);
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
  HammondRegistration r = m_.registration();
  const std::array<double, 9> goal = drawbarsAt(knobs_, &name);
  for (int d = 0; d < 9; ++d) r.upper[d] = int(std::lround(goal[d]));  // (the registration's text)
  // (K1's hard end: the 122 pushed harder too, the grit of a shout)
  r.drive = baseDrive_ * (1 + 1.5 * smooth01((knobs_[0] - 0.7) / 0.3));
  r.leslieFast = leslieFast_;
  m_.setRegistration(r);
  m_.setUpperGoal(goal);  // (the sliders glide there, not in steps)
  const std::array<double, 9> lower = hammondLowerFor(goal);  // (the accompaniment's manual follows)
  m_.setLowerGoal(lower);
  const std::array<double, 2> pedal = hammondPedalFor(goal);  // (and the pedals, by its loudness and brightness)
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
    std::fprintf(stderr, "drawbars %s%s%s (drive %.1f); lower %s; pedal %.1f %.1f\n", bars.c_str(), name ? ": " : "", name ? name : "",
                 r.drive, low.c_str(), pedal[0], pedal[1]);
    for (int d = 0; d < 9; ++d) printed_[d] = goal[d];
    lastName_ = name;
  }
  return true;
}

}  // namespace pajupilli
