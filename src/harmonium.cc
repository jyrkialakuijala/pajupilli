#include "harmonium.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

bool harmoniumInstrument(const Instrument& inst) { return std::strcmp(inst.name, "reed-organ") == 0; }

namespace {

constexpr double kRho = 1.2;

// A row's voicing: broad (the front rows' dark, round tone), plain, narrow (the back rows' thin,
// string-like reeds). The reeds' scale is the accordion's (src/freereed.h) at its width x this.
enum Voicing { kBroad, kPlain, kNarrow, kVoicings };
constexpr double kWidth[kVoicings] = {1.25, 1.0, 0.8};

FreeReedBuild harmoniumReed(double hz, Voicing v) {
  FreeReedBuild b = accordionReed(hz);
  b.width = std::clamp(b.width * kWidth[v], 1.2e-3, 5e-3);
  // The bass and the top tongues voiced softer (thinner, the bass ones weighted): they still speak
  // at a pianissimo's wind.
  // Voiced close and stiff (the tip's set half the accordion's, the tongue twice as stiff): the
  // pitch moves only a few cents from a soft to a hard wind (reed-organ reeds: Cottingham), where
  // the accordion's voicing fell 30-60 c.
  b.set *= knob("PJ_HARM_SET", 0.5);
  b.closeP = knob("PJ_HARM_STIFF", 3000);
  static const double kSoft = knob("PJ_HARM_SOFTEND", 0.5);
  b.closeP *= 1 - kSoft * std::clamp((std::fabs(std::log2(hz / 440)) - 1) / 1.5, 0.0, 1.0);
  // The reed's cell (research/reed_organ.md §4.4): its Helmholtz resonance with the pallet's
  // opening ~1.2-3 kHz.
  b.cell = 1.5e-6 * std::pow(b.len / 13e-3, 1.5);
  return b;
}

double palletArea(const FreeReedBuild& b) { return 30e-6 * std::sqrt(b.len / 13e-3); }

// The pallet's felt: the cell's resonance at Q ~2 (no peak standing over the fundamental); the
// top reeds, their harmonics above it, less damped (they would not speak).
double palletQ(double hz) {
  static const double q = knob("PJ_HARM_PALLETQ", 2), top = knob("PJ_HARM_PALLETQTOP", 6);
  return q + (top - q) * std::clamp(std::log2(hz / 1000) / 1.1, 0.0, 1.0);
}

// The reed's speaking pitch in its cell, the pallet open, at a mezzo-forte chest (800 Pa).
double speakingHz(FreeReed r, double fs, double area) {
  r.reset();
  const double hz = r.hz(), q = palletQ(hz);
  const int warm = int(std::max(0.25, 30 / hz) * fs), len = int(std::max(0.15, 20 / hz) * fs);
  const int per = int(fs / hz) * 4;
  double mean = 0;
  for (int n = 0; n < warm; ++n) {
    r.sampleDown(800, area, 5e-3, q);
    if (n >= warm - per) mean += r.tip();
  }
  mean /= per;
  double prev = r.tip() - mean, first = -1, last = 0;
  int count = 0;
  for (int n = 1; n < len; ++n) {
    r.sampleDown(800, area, 5e-3, q);
    const double y = r.tip() - mean;
    if (prev < 0 && y >= 0) {
      const double t = n - 1 + prev / (prev - y);
      if (first < 0) first = t;
      last = t;
      ++count;
    }
    prev = y;
  }
  return count > 2 ? (count - 1) * fs / (last - first) : 0;
}

// The tuner's table by pitch (MIDI 12 .. 120) for one voicing, once per process.
struct TuneTable {
  double r[121] = {};
  void build(double fs, Voicing v) {
    for (int m = 12; m <= 120; ++m) {
      const double hz = 440 * std::exp2((m - 69) / 12.0);
      const FreeReedBuild b = harmoniumReed(hz, v);
      FreeReed reed;
      reed.setup(fs, b);
      double ratio = 1;
      for (int it = 0; it < 2; ++it) {
        const double got = speakingHz(reed, fs, palletArea(b));
        if (got > 0) ratio *= hz / got;
        reed.tune(ratio);
      }
      r[m] = ratio;
    }
  }
  double at(double hz) const {
    const double m = std::clamp(69 + 12 * std::log2(hz / 440), 12.0, 119.999);
    const int i = int(m);
    return r[i] + (r[i + 1] - r[i]) * (m - i);
  }
};

const TuneTable& tuneTable(double fs, Voicing v) {
  static TuneTable t[kVoicings];
  static std::once_flag once[kVoicings];
  std::call_once(once[v], [&] { t[v].build(fs, v); });
  return t[v];
}

// The stops (research/reed_organ.md §3.1): its pitch (x the key's), front or back, voicing, the
// céleste's beats a second at A4 (0: in tune), treble only, the Percussion hammer.
struct Row { double foot; bool back; Voicing voicing; double beats; bool trebleOnly; bool percussion; };
constexpr Row kCorAnglais = {1, false, kBroad, 0, false, true};   // 1: Cor anglais / Flûte 8'
constexpr Row kBourdon = {0.5, false, kBroad, 0, false, false};   // 2: Bourdon / Clarinette 16'
constexpr Row kClairon = {2, true, kPlain, 0, false, false};      // 3: Clairon / Fifre 4'
constexpr Row kBasson = {1, true, kNarrow, 0, false, false};      // 4: Basson / Hautbois 8'
constexpr Row kCeleste = {1, true, kNarrow, 1.5, true, false};    // Voix céleste (8')

std::vector<Row> registration() {
  const char* e = std::getenv("PJ_HARM_REG");
  const std::string reg = e ? e : "default";
  if (reg == "flute") return {kCorAnglais};
  if (reg == "celeste") return {kCorAnglais, kCeleste};
  if (reg == "grandjeu") return {kCorAnglais, kBourdon, kClairon, kBasson};
  return {kCorAnglais, kBasson};
}

}  // namespace

HarmoniumModel::HarmoniumModel(const Instrument& inst)
    : Tube(inst), fs2_(2 * inst.fs), expression_(knob("PJ_HARM_EXPR", 1) > 0) {
  const std::vector<Row> rows = registration();
  static const bool percussion = knob("PJ_HARM_PERC", 1) > 0;
  uint32_t rng = 0x4a3b2c1du;
  auto urand = [&rng] { rng = rng * 1664525u + 1013904223u; return (rng >> 8) * (2.0 / 16777216.0) - 1; };
  keys_.resize(kHi - kLo + 1);
  for (int m = kLo; m <= kHi; ++m) {
    Key& k = keys_[m - kLo];
    const double hz = 440 * std::exp2((m - 69) / 12.0);
    const bool treble = m >= 65;  // the split: E4 | F4
    for (const Row& row : rows) {
      if (row.trebleOnly && !treble) continue;
      double f = hz * row.foot;
      if (row.beats > 0) f += row.beats * std::sqrt(f / 440);  // the céleste: its beats rise slowly with pitch
      f *= std::exp2(1.0 * urand() / 1200);                   // the tuner's last scrape, ~1 c
      FreeReedBuild b = harmoniumReed(f, row.voicing);
      b.set *= 1 + 0.05 * urand();
      FreeReed r;
      r.setup(fs2_, b);
      r.tune(tuneTable(fs2_, row.voicing).at(f));
      k.reeds.push_back(r);
      k.back.push_back(row.back);
      k.q.push_back(palletQ(f));
      // The hammer starts the tongue at ~20 % of its full swing (§4.3).
      k.kick.push_back(percussion && row.percussion ? 0.2 * 2 * M_PI * f * 0.12 * b.len : 0);
      if (row.foot == 1 && !row.back) k.area = palletArea(b);
    }
    k.pan = treble ? 0.3f : -0.3f;
  }
  auto pole = [&](double hz) { return 1 - std::exp(-2 * M_PI * hz / fs2_); };
  aFront_ = pole(knob("PJ_HARM_FRONT", 2500));
  aBack_ = pole(knob("PJ_HARM_LID", 1400));  // the closed forte lids over the back rows
}

void HarmoniumModel::press(int midi, int velocity) {
  while (midi < kLo) midi += 12;
  while (midi > kHi) midi -= 12;
  Key& k = keys_[midi - kLo];
  if (!k.down) {
    const double v = std::clamp(velocity / 127.0, 0.0, 1.0);
    for (size_t i = 0; i < k.reeds.size(); ++i)
      if (k.kick[i] > 0) k.reeds[i].kick(k.kick[i] * (0.4 + 0.6 * v));
  }
  k.down = true;
  k.speed = 1 / (0.006 - 0.003 * std::clamp(velocity / 127.0, 0.0, 1.0));
  if (std::find(active_.begin(), active_.end(), midi - kLo) == active_.end()) active_.push_back(midi - kLo);
}

void HarmoniumModel::release(int midi) {
  while (midi < kLo) midi += 12;
  while (midi > kHi) midi -= 12;
  keys_[midi - kLo].down = false;
}

void HarmoniumModel::resetAir() {
  Tube::resetAir();
  for (Key& k : keys_) {
    for (FreeReed& r : k.reeds) r.reset();
    k.open = 0;
    k.down = false;
  }
  active_.clear();
  pL_ = use_ = front_ = front2_ = back1_ = back2_ = 0;
  downM_.reset();
  downS_.reset();
}

float HarmoniumModel::sample() {
  static const double kOut = knob("PJ_HARM_OUT", 1.0);
  static const double kSag = knob("PJ_HARM_SAG", 3e4);  // the feeders' limit: Pa lost per m^3/s drawn
  const double dt = 1 / fs2_;
  // The feet: with Expression their pressure, each stroke's changeover (~0.3 s, the two feet in
  // turn) a few % dip; else the reservoir's spring, steady.
  tread_ += dt / 0.6;
  if (tread_ > 1) tread_ -= 1;
  const double ripple = expression_ ? 0.03 * std::cos(4 * M_PI * tread_) : 0.003 * std::cos(4 * M_PI * tread_);
  const double want = (expression_ ? feetP_ : (feetP_ > 0 || !active_.empty() ? 800.0 : 0.0)) * (1 + ripple) - kSag * use_;
  const double aFeet = std::min(1.0, dt / std::max(expression_ ? feetTau_ : 0.05, 1e-3));
  const double g = kRho / (4 * M_PI) * kOut;
  float mid[2], side[2];
  for (int sub = 0; sub < 2; ++sub) {
    pL_ += aFeet * (std::max(want, 0.0) - pL_);
    double front = 0, back = 0, sideSum = 0, use = 0;
    for (size_t a = 0; a < active_.size(); ++a) {
      Key& k = keys_[active_[a]];
      k.open = std::clamp(k.open + (k.down ? 1 : -0.7) * k.speed * dt, 0.0, 1.0);
      const double area = k.area * k.open;
      double keySum = 0;
      bool sounding = k.open > 0;
      for (size_t i = 0; i < k.reeds.size(); ++i) {
        FreeReed& r = k.reeds[i];
        if (area == 0 && r.quiet()) continue;
        sounding = true;
        const double y = r.sampleDown(pL_, area, 5e-3, k.q[i]);
        (k.back[i] ? back : front) += y;
        keySum += y;
        use += std::fabs(r.palletFlow());
      }
      sideSum += k.pan * keySum;
      if (!sounding && !k.down) {
        active_[a] = active_.back();
        active_.pop_back();
        --a;
      }
    }
    use_ = use;
    // The front rows through the case's cloth and grille; the back rows under the closed lids.
    front_ += aFront_ * (front - front_);
    front2_ += aFront_ * (front_ - front2_);
    back1_ += aBack_ * (back - back1_);
    back2_ += aBack_ * (back1_ - back2_);
    mid[sub] = float(g * (front2_ + 0.5 * back2_));
    side[sub] = float(g * sideSum * 0.5);
  }
  last_ = downM_.process(mid[0], mid[1]);
  side_.push_back(float(outScale()) * downS_.process(side[0], side[1]));
  return finish(last_);
}

const float* HarmoniumModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

HarmoniumPlayer::HarmoniumPlayer(HarmoniumModel& m) : Performer(m), m_(m) {}

// The feet's pressure for a velocity (Pa): ~300 at the softest (the reeds, felt-damped in their
// cells, just speak) to ~2.2 kPa fortissimo (§8.3).
double HarmoniumPlayer::pressure(int velocity) const {
  const double v = std::clamp(velocity / 127.0 + breathCtl_, 0.0, 1.2);
  return 300 * fastPow(7.3, v);
}

void HarmoniumPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, held_ > 0);
  note_ = midi;
  vel_ = velocity;
  blowing_ = true;
  if (!down_[midi & 127]) ++held_;
  down_[midi & 127] = true;
  sustained_[midi & 127] = false;
  // The feet: a harder chord, a quicker, harder push.
  const double v = std::clamp(velocity / 127.0, 0.0, 1.0);
  m_.setFeet(pressure(velocity), 0.06 - 0.045 * v);
  m_.press(midi, velocity);
}

void HarmoniumPlayer::noteOff(int midi) {
  if (!down_[midi & 127]) return;
  down_[midi & 127] = false;
  --held_;
  if (hold_) {
    sustained_[midi & 127] = true;
    return;
  }
  m_.release(midi);
}

void HarmoniumPlayer::setHold(bool h) {
  hold_ = h;
  if (h) return;
  for (int m = 0; m < 128; ++m)
    if (sustained_[m]) {
      sustained_[m] = false;
      m_.release(m);
    }
}

void HarmoniumPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    bool any = held_ > 0;
    for (int k = 0; k < 128 && !any; ++k) any = sustained_[k];
    // The feet keep treading while keys sound (CC2 / channel pressure moves them); at rest they ease off.
    if (any) m_.setFeetPressure(pressure(vel_));
    else m_.setFeet(0, 0.2);
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
