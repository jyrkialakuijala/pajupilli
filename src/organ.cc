#include "organ.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"
#include "rng.h"

namespace pajupilli {

bool organRank(const Instrument& inst) {
  return std::strcmp(inst.name, "gedackt-8") == 0 || std::strcmp(inst.name, "rohrgedackt-8") == 0 ||
         std::strcmp(inst.name, "gedackt-rohrgedackt") == 0;
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
double speakingHz(FluePipe p, Enclosure e, int src, float hear, double fs) {
  p.reset();
  e.reset();
  p.open();
  const int warm = int(0.5 * fs), len = int(0.4 * fs);
  const float a = float(1 - std::exp(-2 * M_PI * 1.5 * p.hz() * p.ratio() / fs));
  float l1 = 0, l2 = 0, prev = 0;
  double first = -1, last = 0;
  int count = 0;
  for (int n = 0; n < warm + len; ++n) {
    const float y = p.sample(1.0f, hear * e.heard(src));
    e.add(src, y);
    e.step(0);
    l1 += a * (y - l1);
    l2 += a * (l1 - l2);
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

}  // namespace

// The divisions' enclosures: 0, 1 the swell boxes (C side left, C# right),
// 2, 3 the Rückpositiv case's halves (C left, C# right).
enum { kSwellC = 0, kSwellCis = 1, kPosC = 2, kPosCis = 3 };

// How strongly a pipe's jet hears the field at its mouth (its enclosure's,
// its neighbours').
const float kHear = float(knob("PJ_ORGAN_COUPLE", 1.0));

OrganModel::OrganModel(const Instrument& inst) : Tube(inst) {
  const double fs = inst.fs;
  enc_.resize(4);
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
  const bool sw8 = std::strcmp(inst.name, "rohrgedackt-8") != 0;
  const bool pos8 = std::strcmp(inst.name, "gedackt-8") != 0;
  if (sw8) addRank(0);
  if (pos8) addRank(1);
  setSwell(0.75);
}

// One rank: its pipes voiced, tuned (each a little off, as a tuner leaves
// them: +-0.5 c; the swell box's air a little warmer than the church's, its
// pipes sharper), calibrated, placed on its division's divided chest.
void OrganModel::addRank(int rank) {
  const double fs = fs_;
  Rng rng(0x5EED0000u + rank);
  const double warm = rank == 0 ? knob("PJ_SWELL_WARM", 0.6) : 0.0;  // degrees C over the church
  const double warmCents = 1200 * std::log2(std::sqrt((293.15 + warm) / 293.15));
  const int first = int(pipes_.size());
  for (int m = kLo; m <= kHi; ++m) {
    const double cents = warmCents + 0.5 * rng.uniform();
    const double hz = 440 * std::exp2((m - 69) / 12.0 + cents / 1200);
    Pipe pp;
    pp.p.setup(fs, hz, rank == 0 ? gedackt(hz) : rohrgedackt(hz), unsigned(m * 7919 + rank * 104729));
    // Its place: the C side (C, D, E, F#, G#, A#) in the left enclosure, the
    // C# side in the right; the biggest at the outer ends.
    const int i = m - kLo, side = i % 2, j = i / 2;  // j: 0 the biggest of its side
    const double outer = side == 0 ? -1 : 1;
    Vec3 at;
    if (rank == 0) {
      const double w = 2.6, h = 2.4, d = 1.6;
      if (m < 48) {  // the offset bass octave: along the back wall, lower
        at = {outer * (w / 2 - 0.25 - j * 0.33), -h / 2 + 0.3, -d / 2 + 0.2};
      } else {
        const int jj = j - 6;
        at = {outer * (w / 2 - 0.3 - jj * (w - 0.6) / 24), -h / 2 + 0.55, 0.1};
      }
      pp.enc = side == 0 ? kSwellC : kSwellCis;
    } else {
      at = {outer * (0.45 - j * 0.9 / 30), -0.35, 0.0};
      pp.enc = side == 0 ? kPosC : kPosCis;
    }
    pp.src = enc_[pp.enc].addSource(at);
    // The field moves its jet by the acoustic velocity across the mouth, ~ the
    // pressure / its frequency: the treble's jets hear the box less.
    pp.hear = float(kHear * std::pow(262 / hz, knob("PJ_ORGAN_HEAREXP", 1.0)));
    // Voiced in tune where it stands: the jet and its box pull the pitch a
    // little; turn the modes back (twice: the second pass measures the
    // corrected pipe).
    for (int it = 0; it < 2; ++it) {
      const double got = speakingHz(pp.p, enc_[pp.enc], pp.src, pp.hear, fs);
      if (got > 0) pp.p.tune(pp.p.ratio() * hz / got);
    }
    pp.p.reset();
    pipes_.push_back(pp);
    byKey_[m].push_back(int(pipes_.size()) - 1);
  }
  // The near field: each pipe hears its chest neighbours (a whole tone and
  // two away, on its side) directly, ~1 / distance (a few cm to ~0.3 m).
  static const double kNear = knob("PJ_ORGAN_NEAR", 1.0);
  const int n = int(pipes_.size()) - first;
  for (int a = 0; a < n; ++a) {
    int c = 0;
    for (int dj : {-4, -2, 2, 4}) {
      const int b = a + dj;
      if (b < 0 || b >= n) continue;
      const double dist = 0.08 * std::abs(dj) / 2 + 0.03;
      pipes_[first + a].near[c] = first + b;
      pipes_[first + a].nearG[c] = float(kNear * 0.03 / dist);
      ++c;
    }
  }
}

void OrganModel::open(int midi) {
  if (midi < 0 || midi > 127) return;
  for (int i : byKey_[midi]) pipes_[i].p.open();
}

void OrganModel::close(int midi) {
  if (midi < 0 || midi > 127) return;
  for (int i : byKey_[midi]) pipes_[i].p.close();
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
}

float OrganModel::sample() {
  static const float g = float(knob("PJ_ORGAN_OUT", 0.05));
  if (roomPos_ < roomN_) pRoom_ = room_[roomPos_++];
  // Every sounding pipe: the field at its mouth (its enclosure's, its
  // neighbours' last samples), its sample, into its enclosure.
  for (Pipe& pp : pipes_) {
    if (!pp.p.awake()) {
      pp.y = 0;
      continue;
    }
    float heard = enc_[pp.enc].heard(pp.src);
    for (int c = 0; c < 4 && pp.near[c] >= 0; ++c) heard += pp.nearG[c] * pipes_[pp.near[c]].y;
    pp.y = pp.p.sample(1.0f, pp.hear * heard);
    enc_[pp.enc].add(pp.src, pp.y);
  }
  float l = 0, r = 0;
  for (Enclosure& e : enc_) {
    const float y = e.step(pRoom_ / float(outScale()));
    l += y * float(1 - e.pan());
    r += y * float(1 + e.pan());
  }
  const float mid = 0.5f * (l + r), sd = 0.5f * (l - r);
  last_ = g * mid;
  side_.push_back(g * float(outScale()) * sd);
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
  m_.open(midi);
}

void OrganPlayer::noteOff(int midi) { m_.close(midi); }

void OrganPlayer::setHold(bool) {}  // (an organ has no sustain pedal)

void OrganPlayer::render(float* out, int n) {
  // The swell pedal: the dynamics (a knob) and the breath (a controller) open
  // the boxes' shutters (0.5, the default, ~3/4 open; 2/3 and up fully).
  m_.setSwell(std::clamp(1.5 * (dyn_ + breathCtl_), 0.03, 1.0));
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
