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
  setRegistration(hammondPreset(inst.name));
}

void HammondModel::setRegistration(const HammondRegistration& r) {
  reg_ = r;
  auto gain = [](int bar) { return bar <= 0 ? 0.0 : std::pow(10.0, -3.0 * (8 - std::min(bar, 8)) / 20); };
  for (int d = 0; d < 9; ++d) {
    gainU_[d] = gain(r.upper[d]);
    gainL_[d] = gain(r.lower[d]);
  }
  if (r.perc) gainU_[8] = 0;  // (the percussion takes the 1' drawbar's busbar)
  gainP_[0] = gain(r.pedal[0]);
  gainP_[1] = gain(r.pedal[1]);
  leslie_.setFast(r.leslieFast);
  leslie_.setDrive(r.drive);
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
  static const double kOut = knob("PJ_HAMMOND_OUT", 1.0), kConsole = 0.06;
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
  if (reg_.leslie) leslie_.process(float(y), l, r);
  last_ = float(kOut * 0.5 * (l + r));
  side_.push_back(float(outScale() * kOut * 0.5 * (l - r)));
  return finish(last_);
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
    m_.process(out + i, m);
  }
  blowing_ = held_ > 0;
}

}  // namespace pajupilli
