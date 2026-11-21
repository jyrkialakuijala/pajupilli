#include "accordion.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <mutex>

#include "knob.h"

namespace pajupilli {

bool accordionInstrument(const Instrument& inst) {
  return std::strcmp(inst.name, "accordion") == 0 || std::strcmp(inst.name, "bandoneon") == 0;
}

namespace {

constexpr double kRho = 1.2;

// A reed of the instrument at hz: the accordion's steel on aluminium; the
// bandoneón's steel on zinc, the plates thicker and nailed (less of the
// wax's damping: a quicker start). The bandoneón's tongues are also voiced
// stiffer (a closing pressure of 2.5 kPa against the accordion's 1.5): the
// tango's fortissimo pushes the bellows to ~2.2 kPa, and a tongue held at
// 1.5 kPa is then pressed into its thicker plate's slot and stops (the keys
// above ~G#6 fell silent at velocity 127, the ones below swung about a new
// centre, their pitch ~10-20 c off); at 2.5 kPa every reed speaks to the top
// of the arm's range and its pitch falls only ~8 c (C2) to ~1 c (C7) from
// pianissimo to fortissimo, half the accordion reed's fall.
FreeReedBuild buildFor(double hz, bool bandoneon) {
  FreeReedBuild b = accordionReed(hz);
  if (bandoneon) {
    b.plate *= 1.3;
    b.q *= 1.25;
    b.closeP = 2500;
  }
  return b;
}

// The reed's speaking pitch at a mezzo-forte supply: the zero crossings of
// the tip's swing after it has settled.
double speakingHz(FreeReed r, double fs, double supply) {
  r.reset();
  const double hz = r.hz();
  const int warm = int(std::max(0.25, 30 / hz) * fs), len = int(std::max(0.15, 20 / hz) * fs);
  double mean = 0;
  for (int n = 0; n < warm; ++n) {
    r.sample(supply * std::min(1.0, n / (0.01 * fs)), 1e5);
    if (n >= warm - int(fs / hz) * 4) mean += r.tip();
  }
  mean /= int(fs / hz) * 4;
  double prev = r.tip() - mean, first = -1, last = 0;
  int count = 0;
  for (int n = 1; n < len; ++n) {
    r.sample(supply, 1e5);
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

// The tuner's scraping, as a table by pitch (MIDI 12 .. 120): the ratio that
// brings each reed to its pitch at mezzo-forte. The reed's own pitch is a
// little above its sounding one (the duct's air and the pressure pull it
// down, ~25 c); measured once per instrument, interpolated between semitones.
// The bandoneón is tuned at ~1 kPa, the middle of tango's playing (velocity
// ~80): its 8' and 4' are a pure octave there, and drift apart by under
// ~1.3 c from pianissimo to fortissimo (beats of under ~0.5 Hz); tuned at
// 600 Pa they were ~1 c apart at velocity 96, the highest keys beating at
// ~1 Hz.
struct TuneTable {
  double r[121] = {};
  void build(double fs, bool bandoneon) {
    for (int m = 12; m <= 120; ++m) {
      const double hz = 440 * std::exp2((m - 69) / 12.0);
      FreeReed reed;
      reed.setup(fs, buildFor(hz, bandoneon));
      double ratio = 1;
      for (int it = 0; it < 2; ++it) {
        const double got = speakingHz(reed, fs, bandoneon ? 1000 : 600);
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

const TuneTable& tuneTable(double fs, bool bandoneon) {
  static TuneTable t[2];
  static std::once_flag once[2];
  std::call_once(once[bandoneon], [&] { t[bandoneon].build(fs, bandoneon); });
  return t[bandoneon];
}

}  // namespace

AccordionModel::AccordionModel(const Instrument& inst)
    : Tube(inst), bandoneon_(std::strcmp(inst.name, "bandoneon") == 0), fs2_(2 * inst.fs) {
  const TuneTable& tt = tuneTable(fs2_, bandoneon_);
  // The musette: the two outer 8' sets this many beats a second apart from
  // the middle at A4, rising slowly with the pitch (tuned in beats, not
  // cents: the low notes wider in cents, ~25 c at most).
  const double beat = knob("PJ_ACC_BEAT", 3.5);
  uint32_t rng = 0x5eed1234u;
  auto urand = [&rng] { rng = rng * 1664525u + 1013904223u; return (rng >> 8) * (2.0 / 16777216.0) - 1; };
  keys_.resize(kHi - kLo + 1);
  for (int m = kLo; m <= kHi; ++m) {
    Key& k = keys_[m - kLo];
    const double hz = 440 * std::exp2((m - 69) / 12.0);
    struct Set { double hz; bool cass; };
    std::vector<Set> sets;
    if (bandoneon_) {
      sets.push_back({hz, false});
      if (m + 12 <= 105) sets.push_back({2 * hz, false});
    } else if (m < 48) {  // the bass: 16' + 8', both in the cassotto
      sets.push_back({hz, true});
      sets.push_back(m - 12 >= 28 ? Set{hz / 2, true} : Set{2 * hz, true});
    } else {  // the musette: 8' in tune (the cassotto), one flat, one sharp
      const double b = beat * std::sqrt(hz / 440);
      const double c = std::min(25.0, 1200 * std::log2(1 + b / hz));
      sets.push_back({hz, true});
      if (c > 0.05) {
        sets.push_back({hz * std::exp2(-c / 1200), false});
        sets.push_back({hz * std::exp2(c / 1200), false});
      }
    }
    double eight[2] = {};  // (the bandoneón: each direction's 8' reed as tuned)
    for (size_t si = 0; si < sets.size(); ++si)
      for (int dir = 0; dir < 2; ++dir) {
        const Set& s = sets[si];
        // Each reed its own: the tuner's last scrape within ~1 c, the set
        // and the plate a little different. The bandoneón's 4' is tuned to
        // its 8' by ear, a pure octave (research/free_reeds.md §4: "no
        // beating when well tuned"), within ~0.2 c: scraped to within 1 c
        // of the fork each on its own, the 4' beat with the 8' reed's
        // second harmonic at ~0.5-3 Hz, up to 8 dB deep over the held note.
        const double hzR = bandoneon_ && si == 1 ? 2 * eight[dir] * std::exp2(0.2 * urand() / 1200)
                                                 : s.hz * std::exp2(1.0 * urand() / 1200);
        if (si == 0) eight[dir] = hzR;
        FreeReedBuild b = buildFor(hzR, bandoneon_);
        b.set *= 1 + 0.05 * urand();
        b.clear *= 1 + 0.1 * urand();
        FreeReed r;
        r.setup(fs2_, b);
        r.tune(tt.at(hzR));
        k.reeds.push_back(r);
        k.group.push_back(s.cass ? 1 : 0);
      }
    k.pan = m < 48 && !bandoneon_ ? -0.5f : bandoneon_ ? float(std::clamp((m - 66) / 30.0, -0.5, 0.5)) : 0.3f;
  }
  // The radiation (research/free_reeds.md §3.2): the cassotto lowers the
  // highs (two poles at ~1.4 kHz), the grille and the box the very top; on
  // the pull the reeds sound into the bellows, a little duller. The
  // bandoneón has no cassotto, but every reed of it sounds into a chamber of
  // its wooden reed block, out through a leather-valved hole and the
  // fretwork of the wooden end-plate: one pole at ~500 Hz for that (the
  // highs falling 6 dB an octave above it; PJ_BN_BLOCK) [design: fitted to
  // the SoundFonts' bandoneóns, research/gm_compare.md "24 Tango Accordion",
  // and research/free_reeds.md §4's "a large, deep sound", "sweet and
  // somewhat dull"]. With the grille alone (3.5 kHz) its spectral centroid
  // at velocity 96 was ~2 kHz (19.7 on the ERB scale) against the fonts'
  // 0.9-1.3 kHz (14.3-15.6), 6-17 dB short below 400 Hz; with the blocks
  // 1.08 kHz (15.3), 2-9 dB short below 150 Hz, 0-8 dB over from 150 Hz to
  // 6 kHz, 5-10 dB under above it (level removed). Two poles at ~1.6 kHz (a
  // cassotto) reached the same centroid but left a hump at 0.4-2.5 kHz,
  // 5-11 dB over the fonts, and 11-17 dB less above 6 kHz.
  auto pole = [&](double hz) { return 1 - std::exp(-2 * M_PI * hz / fs2_); };
  aCass_ = pole(knob("PJ_ACC_CASS", 1400));
  if (bandoneon_) aBlock_ = pole(knob("PJ_BN_BLOCK", 500));
  aGrille_ = pole(knob("PJ_ACC_GRILLE", bandoneon_ ? 3500 : 5000));
  aPull_ = pole(knob("PJ_ACC_PULL", 3500));
  // The half-band filter (2x down): a Kaiser-windowed sinc, cut at 21 kHz.
  {
    const double fc = 21000 / fs2_, beta = 8, c = 0.5 * (kHalf - 1);
    auto i0 = [](double x) { double s = 1, t = 1; for (int k = 1; k < 30; ++k) { t *= (x / (2 * k)) * (x / (2 * k)); s += t; } return s; };
    double sum = 0;
    for (int i = 0; i < kHalf; ++i) {
      const double t = i - c, r = t / c;
      const double sinc = t == 0 ? 2 * fc : std::sin(2 * M_PI * fc * t) / (M_PI * t);
      hb_[i] = float(sinc * i0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / i0(beta));
      sum += hb_[i];
    }
    for (float& h : hb_) h = float(h / sum);
  }
}

void AccordionModel::press(int midi, int velocity) {
  while (midi < kLo) midi += 12;
  while (midi > kHi) midi -= 12;
  Key& k = keys_[midi - kLo];
  k.down = true;
  // The key's stroke: ~6 ms soft, ~2 ms struck.
  k.speed = 1 / (0.006 - 0.004 * std::clamp(velocity / 127.0, 0.0, 1.0));
  if (std::find(active_.begin(), active_.end(), midi - kLo) == active_.end()) active_.push_back(midi - kLo);
}

void AccordionModel::release(int midi) {
  while (midi < kLo) midi += 12;
  while (midi > kHi) midi -= 12;
  keys_[midi - kLo].down = false;
}

bool AccordionModel::anyKey() const {
  for (int i : active_)
    if (keys_[i].down) return true;
  return false;
}

bool AccordionModel::asleep() const { return active_.empty(); }

void AccordionModel::resetAir() {
  Tube::resetAir();
  for (Key& k : keys_) {
    for (FreeReed& r : k.reeds) r.reset();
    k.open = 0;
    k.down = false;
  }
  active_.clear();
  pb_ = 0;
  cass1_ = cass2_ = grille_ = pull_ = pullS_ = block_ = 0;
}

float AccordionModel::half(float* hist, int& pos, float x0, float x1) {
  for (float x : {x0, x1}) {
    pos = (pos + 1) % kHalf;
    hist[pos] = hist[pos + kHalf] = x;
  }
  const float* h = hist + pos + 1;  // the last kHalf inputs (the filter is symmetric)
  float s = 0;
  for (int i = 0; i < kHalf; ++i) s += hb_[i] * h[i];
  return s;
}

float AccordionModel::sample() {
  static const double kSag = knob("PJ_ACC_SAG", 2e4);    // the bellows' give under the reeds' air (Pa per m^3/s)
  static const double kOut = knob("PJ_ACC_OUT", 1.0);
  // The bend: the pallet this much shut, the duct's air this much heavier, at a full bend.
  static const double kChoke = knob("PJ_ACC_CHOKE", 0.7), kBendDuct = knob("PJ_ACC_BENDDUCT", 1.0);
  // (The bandoneón's stiffer tongues, buildFor(), are pulled down less by the same air: x1 bent
  // its A4 -12 c and 1.8 dB louder at half a bend, x2 -23 c and 3.5 dB quieter, as before them.)
  static const double kBendDuctBn = knob("PJ_BN_BENDDUCT", 2.0);
  const double bendDuct = bandoneon_ ? kBendDuctBn : kBendDuct;
  static const double kPallet = 1e5;                       // the open pallet's resistance (Pa s/m^3)
  const double dt = 1 / fs2_;
  const double bend = std::clamp(-bend_, 0.0, 1.0);
  const double target = dir_ * (armP_ * (1 + 0.6 * bend) - kSag * flow_);
  const double aArm = 1 - std::exp(-dt / std::max(armTau_, 1e-3));
  // The radiated pressure at 1 m from the flow's rate of change: rho / (4 pi r).
  // (The bandoneón x2.97: its reed blocks' pole took 9.5 dB of its loudness
  // (tools/levels.py: -28.2 to -37.7 LUFS), given back here so that
  // src/levels_data.h's trim, measured before it, still holds.)
  double g = kRho / (4 * M_PI) * kOut;
  if (bandoneon_) g *= 2.97;
  float mid[2], side[2];
  for (int sub = 0; sub < 2; ++sub) {
    pb_ += aArm * (target - pb_);
    const double push = std::max(pb_, 0.0), pull = std::max(-pb_, 0.0);
    double sum[2][2] = {};  // [cassotto][pull]
    double sideSum = 0, flow = 0;
    for (size_t a = 0; a < active_.size(); ++a) {
      Key& k = keys_[active_[a]];
      k.open = std::clamp(k.open + (k.down ? 1 : -0.7) * k.speed * dt, 0.0, 1.0);
      const double open = k.open * (1 - kChoke * bend);
      const double r = kPallet / std::max(open * open, 1e-6);
      double keySum = 0;
      bool sounding = k.open > 0;
      for (size_t i = 0; i < k.reeds.size(); ++i) {
        FreeReed& reed = k.reeds[i];
        const double s = (i & 1) ? pull : push;
        if (s == 0 && reed.quiet()) continue;
        sounding = true;
        reed.setDuct(1 + bendDuct * bend);
        const double y = reed.sample(s, r);
        sum[k.group[i]][i & 1] += y;
        keySum += y;
        flow += std::fabs(reed.flow());
      }
      sideSum += k.pan * keySum;
      if (!sounding && !k.down) {
        active_[a] = active_.back();
        active_.pop_back();
        --a;
      }
    }
    flow_ = flow;
    // The pull side duller, the cassotto's highs lowered, then the grille.
    pull_ += aPull_ * (sum[1][1] - pull_);
    pullS_ += aPull_ * (sum[0][1] - pullS_);
    const double cIn = sum[1][0] + pull_;
    cass1_ += aCass_ * (cIn - cass1_);
    cass2_ += aCass_ * (cass1_ - cass2_);
    double y = 1.4 * cass2_ + sum[0][0] + pullS_;
    if (bandoneon_) {
      block_ += aBlock_ * (y - block_);
      y = block_;
    }
    grille_ += aGrille_ * (y - grille_);
    mid[sub] = float(g * grille_);
    side[sub] = float(g * sideSum * 0.6);
  }
  last_ = half(histM_, posM_, mid[0], mid[1]);
  const float s = half(histS_, posS_, side[0], side[1]);
  side_.push_back(float(outScale()) * s);
  return finish(last_);
}

const float* AccordionModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

AccordionPlayer::AccordionPlayer(AccordionModel& m)
    : Performer(m), m_(m), bandoneon_(std::strcmp(m.instrument().name, "bandoneon") == 0) {}

// The arm's push for a key's velocity (Pa): ~220 Pa pianissimo (the reeds
// just speak) to ~2.2 kPa fortissimo [design; research/free_reeds.md §2.3].
double AccordionPlayer::pressure(int velocity) const {
  const double v = std::clamp(velocity / 127.0 + breathCtl_, 0.0, 1.2);
  return 220 * std::pow(10.0, v);
}

void AccordionPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, held_ > 0);
  // The bellows run out: turned between notes once they have travelled a
  // few seconds one way.
  if (held_ == 0 && travel_ > travelMax_) {
    m_.reverse();
    travel_ = 0;
    travelMax_ = 2.5 + 2.5 * urand();
  }
  note_ = midi;
  vel_ = velocity;
  blowing_ = true;
  if (!down_[midi & 127]) ++held_;
  down_[midi & 127] = true;
  // The arm: a harder key, a quicker push (the bandoneón's marcato quicker still).
  const double v = std::clamp(velocity / 127.0, 0.0, 1.0);
  m_.setArm(pressure(velocity), (bandoneon_ ? 0.03 : 0.045) * (1 - 0.8 * v) + 0.003);
  m_.press(midi, velocity);
}

void AccordionPlayer::noteOff(int midi) {
  if (down_[midi & 127]) --held_;
  down_[midi & 127] = false;
  m_.release(midi);
  if (held_ == 0) {
    lastOff_ = now_;
    m_.setArm(0, 0.06);  // the arm rests
  }
}

void AccordionPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double dt = m / fs_;
    tick(dt);
    if (held_ > 0) {
      travel_ += dt;
      // Out of bellows in the middle of a phrase: turned under the notes.
      if (travel_ > 1.7 * travelMax_) {
        m_.reverse();
        travel_ = 0;
        travelMax_ = 2.5 + 2.5 * urand();
      }
      m_.setArmPressure(pressure(vel_));  // (the breath controller, on a held note)
    }
    m_.setBend(std::min(roll_, 0.0));
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
