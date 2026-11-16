#include "harmonica.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"

namespace pajupilli {

bool harmonicaInstrument(const Instrument& inst) { return std::strcmp(inst.name, "harmonica") == 0; }

namespace {

constexpr double kRho = 1.2, kC = 343;
constexpr double kHole = 2e5;  // an open hole and its chamber's way in (Pa s/m^3; ~60 Pa at 0.3 L/s)

// A harmonica reed (research/harmonica.md §2.3): the accordion's scaling (22 mm at C3, 6.5 at
// D7), on a 1.05 mm brass plate, no wider than 2.8 mm, in a 0.3-0.4 cm^3 chamber.
FreeReedBuild harmonicaReed(double hz) {
  FreeReedBuild b = accordionReed(hz);
  b.plate = 1.05e-3;
  b.width = std::min(b.width, 2.8e-3);
  b.cell = 0.35e-6 * b.len / 13e-3;
  return b;
}

// The tract at rest: open, its resonance low and broad (it couples to the reeds only when the
// player tunes it: a bend).
double restHz() { static const double v = knob("PJ_HARP_TRACTHZ", 500); return v; }
double restQ() { static const double v = knob("PJ_HARP_TRACTQ", 1.2); return v; }

double smooth01(double x) {
  x = std::clamp(x, 0.0, 1.0);
  return x * x * (3 - 2 * x);
}

}  // namespace

// Solo tuning, the middle octave of a Richter harp in every octave: per four holes blow C E G C,
// draw D F A B; the slide a semitone up; the top hole's draw with the slide D7 (§2.1).
int HarmonicaModel::noteOf(int h, int row, int dir) {
  static const int kBlow[4] = {0, 4, 7, 12}, kDraw[4] = {2, 5, 9, 11};
  if (h == 15 && row == 1 && dir == 1) return 98;
  return 48 + 12 * (h / 4) + (dir == 0 ? kBlow[h % 4] : kDraw[h % 4]) + row;
}

HarmonicaModel::HarmonicaModel(const Instrument& inst) : Tube(inst), fs2_(2 * inst.fs), dt_(1 / (2 * inst.fs)) {
  ct_ = 40e-6 / (kRho * kC * kC);  // the mouth, ~40 cm^3
  ch_ = 75e-6 / (kRho * kC * kC);  // the cupped hands, ~75 cm^3 of air
  check_ = std::getenv("PJ_HARP_CHECK") != nullptr;
  uint32_t rng = 0xa5a5f00du;
  auto urand = [&rng] { rng = rng * 1664525u + 1013904223u; return (rng >> 8) * (2.0 / 16777216.0) - 1; };
  for (int h = 0; h < kHoles; ++h)
    for (int row = 0; row < 2; ++row)
      for (int dir = 0; dir < 2; ++dir) {
        const double hz = 440 * std::exp2((noteOf(h, row, dir) - 69) / 12.0);
        FreeReedBuild b = harmonicaReed(hz);
        b.set *= 1 + 0.05 * urand();  // each reed gapped by hand
        Reed r;
        r.r.setup(fs2_, b);
        r.hole = h;
        r.row = row;
        r.dir = dir;
        reeds_.push_back(r);
      }
  // Tuned in place (the reed in its chamber, the mouth and the cupped hands round it), at
  // mezzo-forte, twice; within the tuner's ~1 c.
  for (size_t i = 0; i < reeds_.size(); ++i) {
    const double want = reeds_[i].r.hz() * std::exp2(1.0 * urand() / 1200);
    for (int it = 0; it < 2; ++it) {
      const double got = calibrate(int(i));
      if (got > 0) reeds_[i].r.tune(reeds_[i].r.ratio() * want / got);
    }
  }
  resetAir();
}

HarmonicaModel::~HarmonicaModel() {
  if (check_) std::fprintf(stderr, "harmonica: %ld samples with blow and draw reeds driven at once\n", violations_);
}

double HarmonicaModel::calibrate(int i) {
  resetAir();
  Reed& e = reeds_[i];
  for (int h = 0; h < kHoles; ++h) lip_[h] = lipTarget_[h] = h == e.hole ? 1 : 0;
  slideIn_ = e.row == 1;
  slide_ = e.row;
  dir_ = wantDir_ = e.dir;
  breathP_ = e.dir == 0 ? 800 : -800;
  breathTau_ = 0.01;
  tractHz_ = restHz();
  tractQ_ = restQ();
  handA_ = handTarget_ = knob("PJ_HARP_CUPPED", 0.012);
  active_ = {i};
  const double hz = e.r.hz();
  const int warm = int(std::max(0.15, 30 / hz) * fs2_), len = int(std::max(0.1, 20 / hz) * fs2_);
  const int per = int(fs2_ / hz) * 4;
  double mean = 0;
  for (int n = 0; n < warm; ++n) {
    step();
    if (n >= warm - per) mean += e.r.tip();
  }
  mean /= per;
  double prev = e.r.tip() - mean, first = -1, last = 0;
  int count = 0;
  for (int n = 1; n < len; ++n) {
    step();
    const double y = e.r.tip() - mean;
    if (prev < 0 && y >= 0) {
      const double t = n - 1 + prev / (prev - y);
      if (first < 0) first = t;
      last = t;
      ++count;
    }
    prev = y;
  }
  return count > 2 ? (count - 1) * fs2_ / (last - first) : 0;
}

void HarmonicaModel::setLips(int h, double open, double tau) {
  if (h < 0 || h >= kHoles) return;
  lipTarget_[h] = open;
  lipTau_[h] = tau;
  refreshActive();
}

void HarmonicaModel::refreshActive() {
  active_.clear();
  for (size_t i = 0; i < reeds_.size(); ++i) {
    const int h = reeds_[i].hole;
    if (lip_[h] > 1e-4 || lipTarget_[h] > 0 || !reeds_[i].r.quiet()) active_.push_back(int(i));
  }
}

bool HarmonicaModel::asleep() const {
  if (std::fabs(pL_) > 1 || std::fabs(breathP_) > 0) return false;
  for (int i : active_)
    if (!reeds_[i].r.quiet()) return false;
  return true;
}

void HarmonicaModel::resetAir() {
  Tube::resetAir();
  for (Reed& e : reeds_) e.r.reset();
  pL_ = ut_ = pm_ = ph_ = uo_ = prevUo_ = prevUm_ = holdGap_ = 0;
  breathP_ = 0;
  for (int h = 0; h < kHoles; ++h) lip_[h] = lipTarget_[h] = 0;
  active_.clear();
  down_.reset();
}

float HarmonicaModel::step() {
  const double dt = dt_;
  // The breath: towards its target; turned round, it first dies away, the chest pauses
  // (~35-60 ms), then the other way (the recordings' 60-120 ms dip, §7).
  const int want = breathP_ > 0 ? 0 : breathP_ < 0 ? 1 : dir_;
  if (want != dir_) {
    pL_ += (0 - pL_) * std::min(1.0, dt / revDecay_);
    if (std::fabs(pL_) < 5) {
      holdGap_ += dt;
      if (holdGap_ > revPause_) {
        dir_ = want;
        holdGap_ = 0;
      }
    }
  } else {
    pL_ += (breathP_ - pL_) * std::min(1.0, dt / std::max(breathTau_, 1e-3));
  }
  // The tract (one Helmholtz mode: the throat's inertance, the mouth's volume).
  const double wt = 2 * M_PI * tractHz_;
  const double mt = 1 / (wt * wt * ct_), rt = std::sqrt(mt / ct_) / tractQ_;
  ut_ = (ut_ + dt / mt * (pL_ - pm_)) / (1 + dt * rt / mt);
  // The slide (in ~20 ms, back on its spring in ~20) and the lips.
  slide_ = std::clamp(slide_ + (slideIn_ ? dt / 0.020 : -dt / 0.020), 0.0, 1.0);
  const double xs = smooth01(slide_);
  // Each row's opening along the travel; both partly open over the middle (the recordings' 1.5-6
  // dB dip, both pitches sounding at once).
  static const double kOverlap = knob("PJ_HARP_SLIDEOVERLAP", 1.0);
  const double row[2] = {std::clamp((0.5 + 0.5 * kOverlap - xs) / kOverlap, 0.0, 1.0),
                         std::clamp((xs - 0.5 + 0.5 * kOverlap) / kOverlap, 0.0, 1.0)};
  for (int h = 0; h < kHoles; ++h) lip_[h] += (lipTarget_[h] - lip_[h]) * std::min(1.0, dt / std::max(lipTau_[h], 1e-3));
  // The reeds of the breath's direction, driven by the mouth against the hands.
  double g = 0, k = 0;
  bool blow = false, draw = false;
  for (int i : active_) {
    Reed& e = reeds_[i];
    const double open = lip_[e.hole] * row[e.row];
    const double r = kHole / std::max(open * open, 1e-6);
    const double s = e.dir == 0 ? 1 : -1;
    const double sup = e.dir == dir_ ? std::max(s * (pm_ - ph_), 0.0) : 0.0;
    if (sup == 0 && e.r.quiet()) continue;
    e.r.sample(sup, r);
    if (sup > 0) {
      g += 1 / r;
      (e.dir == 0 ? blow : draw) = true;
    }
    k += s * e.r.cellPressure() / r;
  }
  if (blow && draw) ++violations_;
  // The ear: the lead reed's swing, its period from upward crossings.
  if (earReed_ >= 0) {
    const double z = reeds_[earReed_].r.tip();
    earMean_ += (z - earMean_) * 0.002;
    const double y = z - earMean_;
    earAge_ += 1;
    if (earPrev_ < 0 && y >= 0) {
      const double frac = earPrev_ / (earPrev_ - y);
      const double since = earAge_ - 1 + frac;
      if (since > 8 && since < 2000) earPeriod_ = since;
      earAge_ = 1 - frac;
    }
    earPrev_ = y;
  }
  // The mouth and the hands (implicit in the holes' conductance: a chord's many open holes).
  pm_ = (pm_ + dt / ct_ * (ut_ + g * ph_ + k)) / (1 + dt * g / ct_);
  const double a = std::clamp(handA_ * (1 + tremDepth_ * std::sin(tremPh_)), 0.003, 0.045);
  const double mo = kRho * 1.7 / (M_PI * a);                 // the opening's air (a flanged hole)
  static const double kHandQ = knob("PJ_HARP_HANDQ", 3);
  const double ro = std::sqrt(mo / ch_) / kHandQ;  // radiation and the soft hands' losses
  uo_ = (uo_ + dt / mo * ph_) / (1 + dt * ro / mo);
  ph_ = (ph_ + dt / ch_ * (g * pm_ - k - uo_)) / (1 + dt * g / ch_);
  const double um = g * (pm_ - ph_) - k;
  lung_ = std::clamp(lung_ - um * dt / 3e-3, 0.0, 1.0);  // ~3 L to spend
  // The hand tremolo's flutter: ~6.5 Hz, its period wandering ~12 %.
  rng_ = rng_ * 1664525u + 1013904223u;
  tremWander_ += (((rng_ >> 8) * (2.0 / 16777216.0) - 1) - tremWander_) * 2e-4;
  tremPh_ += 2 * M_PI * tremHz_ * (1 + 3 * tremWander_) * dt;
  if (tremPh_ > 2 * M_PI) tremPh_ -= 2 * M_PI;
  handA_ += (handTarget_ - handA_) * std::min(1.0, dt / std::max(handTau_, 1e-3));
  // Radiated at 1 m: the hands' opening, and a little straight through the fingers. Open, the
  // harmonica faces the listener (its covers' backs forward): projected, ~+3.5 dB.
  static const double kCup = knob("PJ_HARP_CUPPED", 0.012), kOpenA = knob("PJ_HARP_OPEN", 0.035);
  const double proj = 1 + 0.5 * std::clamp((a - kCup) / std::max(kOpenA - kCup, 1e-3), 0.0, 1.0);
  const double out = proj * kRho / (4 * M_PI) * ((uo_ - prevUo_) + 0.1 * (um - prevUm_)) * fs2_;
  prevUo_ = uo_;
  prevUm_ = um;
  return float(out);
}

float HarmonicaModel::sample() {
  static const double kOut = knob("PJ_HARP_OUT", 1.0);
  static int count = 0;
  if (++count >= 256) {
    count = 0;
    refreshActive();
  }
  const float a = step(), b = step();
  last_ = float(kOut) * down_.process(a, b);
  return finish(last_);
}

// --- The player.

HarmonicaPlayer::HarmonicaPlayer(HarmonicaModel& m) : Performer(m), m_(m) {}

// The breath for a velocity (Pa): ~250 at the softest to ~3 kPa (research/harmonica.md §10.3).
double HarmonicaPlayer::pressure(int vel) const {
  const double v = std::clamp(vel / 127.0 + breathCtl_, 0.0, 1.2);
  return 250 * std::pow(12.0, v);
}

void HarmonicaPlayer::decide(bool attack) {
  sounding_.clear();
  if (held_.empty()) return;
  const double fill = m_.lungFill();
  // The lungs: full, blowing is easier; empty, drawing (+: favours blow).
  const double lungs = 2 * (fill - 0.5);
  auto fold = [](int m) {
    while (m < HarmonicaModel::kLo) m += 12;
    while (m > HarmonicaModel::kHi) m -= 12;
    return m;
  };
  if (held_.size() == 1) {
    // One line: the place that keeps the breath, then the slide, then the mouth where it is.
    const int m = fold(held_[0].midi);
    double best = 1e9;
    Place p{0, 0, 0};
    for (int h = 0; h < HarmonicaModel::kHoles; ++h)
      for (int row = 0; row < 2; ++row)
        for (int d = 0; d < 2; ++d) {
          if (HarmonicaModel::noteOf(h, row, d) != m) continue;
          double c = (d != dir_ ? (attack ? 1.5 : 10) : 0) - 8 * lungs * (d == 0 ? 1 : -1) +
                     (row != row_ ? 3 : 0) + 0.3 * std::abs(h - hole_);
          if (c < best) { best = c; p = {h, row, d}; }
        }
    sounding_.push_back({held_[0].midi, p});
  } else {
    // A chord: one breath. The direction whose notes are louder on average (a bonus for more
    // notes, the lungs' leaning, a little for staying), then the slide most of them want, then
    // the holes: the narrowest span, at most four under the lips.
    struct Opt { int midi, vel; std::vector<Place> at; };
    double score[2];
    for (int d = 0; d < 2; ++d) {
      int n = 0;
      double sum = 0;
      for (const Held& hn : held_) {
        const int m = fold(hn.midi);
        bool ok = false;
        for (int h = 0; h < HarmonicaModel::kHoles && !ok; ++h)
          for (int row = 0; row < 2; ++row) ok |= HarmonicaModel::noteOf(h, row, d) == m;
        if (ok) { ++n; sum += hn.vel; }
      }
      score[d] = n ? sum / n + 6 * n + 15 * lungs * (d == 0 ? 1 : -1) + (d == dir_ && !attack ? 5 : 0) : -1e9;
    }
    const int d = score[1] > score[0] ? 1 : 0;
    int bestRow = row_, bestN = -1, bestV = -1;
    for (int row = 0; row < 2; ++row) {
      int n = 0, v = 0;
      for (const Held& hn : held_)
        for (int h = 0; h < HarmonicaModel::kHoles; ++h)
          if (HarmonicaModel::noteOf(h, row, d) == fold(hn.midi)) { ++n; v += hn.vel; break; }
      if (n > bestN || (n == bestN && (v > bestV || (v == bestV && row == row_)))) { bestN = n; bestV = v; bestRow = row; }
    }
    std::vector<Opt> opts;
    for (const Held& hn : held_) {
      Opt o{hn.midi, hn.vel, {}};
      for (int h = 0; h < HarmonicaModel::kHoles; ++h)
        if (HarmonicaModel::noteOf(h, bestRow, d) == fold(hn.midi)) o.at.push_back({h, bestRow, d});
      if (!o.at.empty()) opts.push_back(o);
    }
    // The lips' window: four holes; the one that holds the most (by velocity), nearest the mouth.
    double best = -1e9;
    int bestW = 0;
    for (int w = 0; w + 4 <= HarmonicaModel::kHoles; ++w) {
      double v = 0;
      for (const Opt& o : opts)
        for (const Place& p : o.at)
          if (p.hole >= w && p.hole < w + 4) { v += o.vel; break; }
      v -= 0.5 * std::abs(w + 1.5 - hole_);
      if (v > best) { best = v; bestW = w; }
    }
    std::vector<bool> used(HarmonicaModel::kHoles, false);
    for (const Opt& o : opts)
      for (const Place& p : o.at)
        if (p.hole >= bestW && p.hole < bestW + 4 && !used[p.hole]) {
          used[p.hole] = true;
          sounding_.push_back({o.midi, p});
          break;
        }
  }
  if (sounding_.empty()) return;
  dir_ = sounding_[0].second.dir;
  row_ = sounding_[0].second.row;
  hole_ = sounding_[0].second.hole;
  m_.setEar(sounding_.back().second.hole, sounding_.back().second.row, sounding_.back().second.dir);
  // The mouth: the notes' holes open, the tongue on the rest.
  bool open[HarmonicaModel::kHoles] = {};
  for (auto& s : sounding_) open[s.second.hole] = true;
  const bool tongued = attack && tongued_;
  for (int h = 0; h < HarmonicaModel::kHoles; ++h) m_.setLips(h, open[h] ? 1 : 0, tongued ? 0.002 : attack ? 0.004 : 0.006);
  m_.setSlide(row_ == 1);
  applyBreath();
}

void HarmonicaPlayer::applyBreath() {
  if (sounding_.empty()) return;
  double v = 0;
  for (auto& s : sounding_)
    for (const Held& h : held_)
      if (h.midi == s.first) { v += h.vel; break; }
  v /= sounding_.size();
  const double bendSt = 2 * roll_;
  const double down = std::min(bendSt, 0.0) + scoop_;
  // Bending, the breath is pushed harder; an upward bend is only a softer breath (a few cents).
  const double p = pressure(int(v)) * (1 + 0.3 * std::min(1.0, -down)) * (1 - 0.4 * std::clamp(bendSt / 2, 0.0, 1.0));
  m_.setBreath(dir_ == 0 ? p : -p);
}

void HarmonicaPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, !held_.empty());
  const bool attack = !breathing_;
  // How fast the notes come: a quick passage turns the breath quickly (the recordings' 60-120 ms
  // gaps are a slow line's) and has no time to scoop.
  const double ioi = now_ - noteOnAt_;
  m_.setReversal(std::clamp(0.08 * ioi, 0.003, 0.012), std::clamp(0.25 * ioi, 0.005, 0.03));
  held_.erase(std::remove_if(held_.begin(), held_.end(), [&](const Held& h) { return h.midi == midi; }), held_.end());
  held_.push_back({midi, velocity, now_});
  note_ = midi;
  vel_ = velocity;
  blowing_ = true;
  noteOnAt_ = now_;
  endBreath_ = false;
  // The tongue: a hard key builds the breath behind it and lets go; a soft one is a breath attack.
  const double v = velocity / 127.0;
  tongued_ = velocity > 70 || ioi < 0.4;
  m_.setBreathTau(attack ? (tongued_ ? 0.008 : 0.07 - 0.05 * v) : std::clamp(0.15 * ioi, 0.004, 0.02));
  decide(attack);
  breathing_ = true;
  // A scoop into the note (the tract starting low): the recordings' most notes, 50-185 c.
  scoop_ = urand() < (attack ? 0.6 : 0.35) * smooth01((ioi - 0.15) / 0.3) ? -(0.5 + 1.3 * urand()) : 0;
  // The hands close round it; on a long note they may open part-way through. More drama when
  // the others leave the register free.
  static const double kAuto = knob("PJ_HARP_HANDS", 1);  // the player's own hand moves (0: none)
  const double drama = kAuto / (1 + 2 * busy_);
  m_.setHands(knob("PJ_HARP_CUPPED", 0.012), 0.05);
  handsOpen_ = false;
  handOpenAt_ = urand() < 0.45 * drama ? 0.5 + 0.4 * urand() : -1;
  habitTrem_ = urand() < 0.6 * drama ? (0.25 + 0.25 * urand()) * (0.5 + 0.5 * drama) : 0;
}

void HarmonicaPlayer::noteOff(int midi) {
  held_.erase(std::remove_if(held_.begin(), held_.end(), [&](const Held& h) { return h.midi == midi; }), held_.end());
  // Its hole closes (the others keep theirs: a release never turns the breath).
  for (size_t i = 0; i < sounding_.size(); ++i)
    if (sounding_[i].first == midi) {
      const int h = sounding_[i].second.hole;
      sounding_.erase(sounding_.begin() + i);
      bool still = false;
      for (auto& s : sounding_) still |= s.second.hole == h;
      if (!still && !held_.empty()) m_.setLips(h, 0, 0.006);
      break;
    }
  if (!held_.empty()) {
    if (sounding_.empty()) decide(false);  // (only silent notes were left: play them now)
    else applyBreath();
    return;
  }
  offAt_ = now_;
  endBreath_ = true;
  // A short note ends with the tongue (the breath held for the next); a long one with the breath.
  if (now_ - noteOnAt_ < 0.12) {
    for (int h = 0; h < HarmonicaModel::kHoles; ++h) m_.setLips(h, 0, 0.003);
  } else {
    m_.setBreathTau(0.05);
    m_.setBreath(0);
  }
  // The phrase's end, or a rest: the hands open ("doo-wah"), more with less around.
  static const double kAuto = knob("PJ_HARP_HANDS", 1);
  const double drama = kAuto / (1 + 2 * busy_);
  handOpenAt_ = kAuto > 0 && urand() < 0.35 + 0.65 * drama ? (now_ - noteOnAt_) + 0.15 * urand() : -1;
}

void HarmonicaPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  static const double kOpen = knob("PJ_HARP_OPEN", 0.035), kCupped = knob("PJ_HARP_CUPPED", 0.012);
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double dt = m / fs_;
    tick(dt);
    scoop_ *= std::exp(-dt / 0.025);
    const double drama = 1 / (1 + 2 * busy_);
    if (!held_.empty()) {
      applyBreath();
      // The tract: neutral (~500 Hz, Q 3), or tuned ~4 % above the bent pitch of the lead note.
      int lead = held_.back().midi;
      for (auto& s : sounding_) lead = s.first;
      const double down = std::min(2 * roll_, 0.0) + scoop_;
      const double fN = 440 * std::exp2((std::clamp(lead, 48, 98) - 69) / 12.0);
      // Bending, the player tunes the tract above the wanted pitch and listens: too low, the
      // tongue moves the resonance up (the reed pulled less), too high, down (~30 ms).
      const double want = fN * std::exp2(down / 12);
      if (down < -0.02) {
        const double heard = m_.heardHz();
        if (heard > 0) tractCorr_ = std::clamp(tractCorr_ - std::log(heard / want) * dt / 0.03, -0.1, 0.6);
      } else {
        tractCorr_ = 0.04;
      }
      const double w = smooth01(-down / 0.1);
      const double fT = std::exp(std::log(restHz()) * (1 - w) + (std::log(want) + tractCorr_) * w);
      m_.setTract(fT, restQ() + (10 - restQ()) * std::min(1.0, -down));
    } else if (endBreath_ && now_ - offAt_ > 0.08) {
      m_.setBreathTau(0.05);
      m_.setBreath(0);
      endBreath_ = false;
      breathing_ = false;
      blowing_ = !m_.asleep();
    }
    if (held_.empty() && !endBreath_) breathing_ = false;
    // The hands: open on cue (a long note's middle, a phrase's end), over 0.1-0.25 s.
    if (!handsOpen_ && handOpenAt_ >= 0 && now_ - noteOnAt_ > handOpenAt_) {
      handsOpen_ = true;
      m_.setHands(kCupped + (kOpen - kCupped) * (0.4 + 0.6 * drama), 0.1 + 0.15 * urand());
    }
    // The tremolo: CC1, or the player's habit on a long note (after ~0.35 s).
    const double held = now_ - noteOnAt_;
    const double habit = !held_.empty() && held > 0.35 ? habitTrem_ * smooth01((held - 0.35) / 0.4) : 0;
    m_.setTremolo(std::max(flutter_, habit), 6.5);
    static const bool kDebug = std::getenv("PJ_HARP_DEBUG") != nullptr;
    if (kDebug && int(now_ * 1000) % 10 == 0)
      std::fprintf(stderr, "%.3f hole %d row %d held %zu snd %zu dir %d pL %.0f bdir %d scoop %.2f corr %.2f heard %.0f lung %.2f\n", now_, hole_, row_, held_.size(),
                   sounding_.size(), dir_, m_.breath(), m_.breathDir(), scoop_, tractCorr_, m_.heardHz(), m_.lungFill());
    m_.process(out + i, m);
  }
  if (held_.empty() && !endBreath_) blowing_ = !m_.asleep();
}

}  // namespace pajupilli
