#include "organ.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {

bool organRank(const Instrument& inst) { return std::strcmp(inst.name, "gedackt-8") == 0; }

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

// The pipe's speaking pitch after a short run (its steady part): the zero
// crossings' rate of its sound low-passed to the fundamental (two one-poles
// at 1.5 x its pitch: a low pipe's waveform, with its noise, can wander
// across zero twice a period; the fundamental alone crosses once).
double speakingHz(FluePipe p, double fs) {
  p.reset();
  p.open();
  const int warm = int(0.5 * fs), len = int(0.4 * fs);
  const float a = float(1 - std::exp(-2 * M_PI * 1.5 * p.hz() * p.ratio() / fs));
  float l1 = 0, l2 = 0;
  for (int n = 0; n < warm; ++n) {
    const float y = p.sample(1.0f);
    l1 += a * (y - l1);
    l2 += a * (l1 - l2);
  }
  float prev = l2;
  double first = -1, last = 0;
  int count = 0;
  for (int n = 1; n < len; ++n) {
    const float y = p.sample(1.0f);
    l1 += a * (y - l1);
    l2 += a * (l1 - l2);
    if (prev < 0 && l2 >= 0) {
      const double t = n - 1 + prev / (prev - l2);
      if (first < 0) first = t;
      last = t;
      ++count;
    }
    prev = l2;
  }
  return count > 2 ? (count - 1) * fs / (last - first) : 0;
}

}  // namespace

OrganModel::OrganModel(const Instrument& inst) : Tube(inst) {
  const double fs = inst.fs;
  for (int m = kLo; m <= kHi; ++m) {
    const double hz = 440 * std::exp2((m - 69) / 12.0);
    FluePipe p;
    p.setup(fs, hz, gedackt(hz), unsigned(m * 7919));
    // Voiced in tune: the jet pulls the pitch a little; turn the modes back
    // (twice: the second pass measures the corrected pipe).
    for (int it = 0; it < 2; ++it) {
      const double got = speakingHz(p, fs);
      if (got > 0) p.tune(p.ratio() * hz / got);
    }
    p.reset();
    pipes_.push_back(p);
    // C side (C, D, E, F#, G#, A#) left, C# side right; the biggest at the
    // ends, the smallest in the middle of each side.
    const int i = m - kLo;
    const double out = 1 - i / double(kHi - kLo);
    pan_.push_back(float((i % 2 == 0 ? -1 : 1) * (0.25 + 0.65 * out)));
  }
}

void OrganModel::open(int midi) {
  if (midi < kLo || midi > kHi) return;
  pipes_[midi - kLo].open();
}

void OrganModel::close(int midi) {
  if (midi < kLo || midi > kHi) return;
  pipes_[midi - kLo].close();
}

bool OrganModel::asleep() const {
  for (const FluePipe& p : pipes_)
    if (p.awake()) return false;
  return true;
}

void OrganModel::resetAir() {
  Tube::resetAir();
  for (FluePipe& p : pipes_) p.reset();
}

float OrganModel::sample() {
  static const float g = float(knob("PJ_ORGAN_OUT", 0.05));
  float y = 0, s = 0;
  for (size_t i = 0; i < pipes_.size(); ++i) {
    if (!pipes_[i].awake()) continue;
    const float v = pipes_[i].sample(1.0f);
    y += v;
    s += pan_[i] * v;
  }
  last_ = g * y;
  side_.push_back(g * float(outScale()) * 0.5f * s);
  return finish(last_);
}

const float* OrganModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

OrganPlayer::OrganPlayer(OrganModel& m) : Performer(m), m_(m) {}

void OrganPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  down_[midi & 127] = true;
  m_.open(midi);
}

void OrganPlayer::noteOff(int midi) {
  down_[midi & 127] = false;
  m_.close(midi);
}

void OrganPlayer::setHold(bool) {}  // (an organ has no sustain pedal)

void OrganPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
