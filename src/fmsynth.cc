#include "fmsynth.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {

bool fmPiano(const Instrument& inst) { return std::strcmp(inst.name, "electric-piano-2") == 0; }

namespace {

// The factory bank ROM1A (yamahablackboxes.com, rom1a.syx, its checksum good), voice 11
// "E.PIANO 1", in the bank's packed form (128 bytes: operator 6 first, 17 bytes each; then the
// pitch envelope, algorithm, feedback, LFO, transpose, name).
const uint8_t kEPiano1[128] = {
    95, 29, 20, 50, 99, 95, 0, 0, 41, 0, 19, 0, 115, 24, 79, 2,
    0, 95, 20, 20, 50, 99, 95, 0, 0, 0, 0, 0, 0, 3, 0, 99,
    2, 0, 95, 29, 20, 50, 99, 95, 0, 0, 0, 0, 0, 0, 59, 24,
    89, 2, 0, 95, 20, 20, 50, 99, 95, 0, 0, 0, 0, 0, 0, 59,
    8, 99, 2, 0, 95, 50, 35, 78, 99, 75, 0, 0, 0, 0, 0, 0,
    59, 28, 58, 28, 0, 96, 25, 25, 67, 99, 75, 0, 0, 0, 0, 0,
    0, 83, 8, 99, 2, 0, 94, 67, 95, 60, 50, 50, 50, 50, 4, 6,
    34, 33, 0, 0, 56, 24, 69, 46, 80, 73, 65, 78, 79, 32, 49, 32,
};

struct OpParams {
  int rates[4], levels[4];
  int bp, ld, rd, lc, rc, rs, det, ams, kvs, ol, mode, fc, ff;
};
struct Voice {
  OpParams op[6];  // op[0] = operator 6
  int alg, fb, oks;
};

Voice unpack(const uint8_t* b) {
  Voice v{};
  for (int i = 0; i < 6; ++i) {
    const uint8_t* o = b + 17 * i;
    OpParams& p = v.op[i];
    for (int k = 0; k < 4; ++k) p.rates[k] = o[k], p.levels[k] = o[4 + k];
    p.bp = o[8], p.ld = o[9], p.rd = o[10];
    p.lc = o[11] & 3, p.rc = (o[11] >> 2) & 3;
    p.rs = o[12] & 7, p.det = o[12] >> 3;
    p.ams = o[13] & 3, p.kvs = (o[13] >> 2) & 7;
    p.ol = o[14];
    p.mode = o[15] & 1, p.fc = (o[15] >> 1) & 31;
    p.ff = o[16];
  }
  v.alg = b[110] & 31;  // (0-based: 4 is algorithm 5)
  v.fb = b[111] & 7;
  v.oks = (b[111] >> 3) & 1;
  return v;
}

const Voice& voice() {
  static const Voice v = unpack(kEPiano1);
  return v;
}

// --- The FM synth's tables and scalings (MSFA: env.cc, fmsynthnote.cc).

int scaleOutLevel(int l) {
  static const int kLut[20] = {0, 5, 9, 13, 17, 20, 23, 25, 27, 29, 31, 33, 35, 37, 39, 41, 42, 43, 45, 46};
  return l >= 20 ? 28 + l : kLut[l];
}

int scaleVelocity(int velocity, int sensitivity) {
  static const uint8_t kVel[64] = {0,   70,  86,  97,  106, 114, 121, 126, 132, 138, 142, 148, 152, 156, 160, 163,
                                   166, 170, 173, 174, 178, 181, 184, 186, 189, 190, 194, 196, 198, 200, 202, 205,
                                   206, 209, 211, 214, 216, 218, 220, 222, 224, 225, 227, 229, 230, 232, 233, 235,
                                   237, 238, 240, 241, 242, 243, 244, 246, 246, 248, 249, 250, 251, 252, 253, 254};
  const int v = kVel[std::clamp(velocity, 0, 127) >> 1] - 239;
  return ((sensitivity * v + 7) >> 3) << 4;
}

int scaleRate(int midi, int sensitivity) {
  const int x = std::min(31, std::max(0, midi / 3 - 7));
  return (sensitivity * x) >> 3;
}

int scaleCurve(int group, int depth, int curve) {
  static const uint8_t kExp[33] = {0,  1,  2,  3,  4,  5,   6,   7,   8,   9,   11,  14,  16,  19,  23,  27, 33,
                                   39, 47, 56, 66, 80, 94, 110, 126, 142, 158, 174, 190, 206, 222, 238, 250};
  int scale;
  if (curve == 0 || curve == 3) scale = (group * depth * 329) >> 12;  // linear
  else scale = (kExp[std::min(group, 32)] * depth * 329) >> 15;       // exponential
  return curve < 2 ? -scale : scale;
}

int scaleLevel(int midi, int bp, int ld, int rd, int lc, int rc) {
  const int offset = midi - bp - 17;
  return offset >= 0 ? scaleCurve((offset + 1) / 3, rd, rc) : scaleCurve(-(offset - 1) / 3, ld, lc);
}

// log2 of a frequency, 2^24 an octave.
int32_t logOf(double hz) { return int32_t(std::lround(std::log2(hz) * (1 << 24))); }

// The operator's frequency (mode 0: a ratio). The detune as Dexed measured it on an FM synth.
int32_t oscFreq(double noteHz, const OpParams& p) {
  int32_t lf = logOf(noteHz);
  if (p.mode == 0) {
    const double ratio = 0.0209 * std::exp(-0.396 * (double(lf) / (1 << 24))) / 7;
    lf += int32_t(ratio * lf * (p.det - 7));
    lf += p.fc == 0 ? -(1 << 24) : logOf(p.fc);  // (coarse 0: x 0.5)
    if (p.ff) lf += int32_t(std::floor(24204406.323123 * std::log(1 + 0.01 * p.ff) + 0.5));
  } else {
    // fixed: 10^(coarse & 3) Hz, then the fine steps (not used by this voice)
    lf = int32_t((4458616LL * ((p.fc & 3) * 100 + p.ff)) >> 3);
    lf += p.det > 7 ? 13457 * (p.det - 7) : 0;
  }
  return lf;
}

int32_t exp2q24(int32_t x) { return int32_t(std::lround(std::exp2(double(x) / (1 << 24)) * (1 << 24))); }

constexpr int kSinBits = 12;
float sinTable[(1 << kSinBits) + 1];
const bool kSinInit = [] {
  for (int i = 0; i <= (1 << kSinBits); ++i) sinTable[i] = float(std::sin(2 * M_PI * i / (1 << kSinBits)));
  return true;
}();

// sin of a phase (2^24 a cycle), 2^24 = 1.
inline int32_t sinQ24(uint32_t phase) {
  const uint32_t p = phase & 0xffffff;
  const uint32_t i = p >> (24 - kSinBits);
  const float f = float(p & ((1 << (24 - kSinBits)) - 1)) * (1.0f / (1 << (24 - kSinBits)));
  return int32_t((sinTable[i] + f * (sinTable[i + 1] - sinTable[i])) * float(1 << 24));
}

constexpr int kLevelThresh = 1120;  // (MSFA: an operator below this gain, both ends of the block, is silent)

}  // namespace

// --- The envelope (MSFA's Env, its steps at 44.1 kHz rescaled to our rate).

void FmSynthModel::Env::init(const int r[4], const int l[4], int ol, int rs, double scale) {
  for (int i = 0; i < 4; ++i) rates[i] = r[i], levels[i] = l[i];
  outlevel = ol;
  rateScaling = rs;
  incScale = scale;
  level = 0;
  down = true;
  advance(0);
}

void FmSynthModel::Env::advance(int i) {
  ix = i;
  if (ix < 4) {
    int actual = scaleOutLevel(levels[ix]) >> 1;
    actual = (actual << 6) + outlevel - 4256;
    actual = std::max(actual, 16);
    target = actual << 16;
    rising = target > level;
    int qrate = (rates[ix] * 41) >> 6;
    qrate = std::min(qrate + rateScaling, 63);
    const int32_t raw = (4 + (qrate & 3)) << (2 + 6 + (qrate >> 2));
    inc = int32_t(raw * incScale);
  }
}

int32_t FmSynthModel::Env::step() {
  if (ix < 3 || (ix < 4 && !down)) {
    if (rising) {
      constexpr int32_t kJump = 1716;
      if (level < (kJump << 16)) level = kJump << 16;
      level += ((((17 << 24) - level) >> 24) * inc);
      if (level >= target) {
        level = target;
        advance(ix + 1);
      }
    } else {
      level -= inc;
      if (level <= target) {
        level = target;
        advance(ix + 1);
      }
    }
  }
  return level;
}

void FmSynthModel::Env::keyDown(bool d) {
  if (down != d) {
    down = d;
    advance(d ? 0 : 3);
  }
}

// --- The model.

FmSynthModel::FmSynthModel(const Instrument& inst) : Tube(inst) { (void)kSinInit; }

void FmSynthModel::resetAir() {
  notes_.clear();
  pos_ = 64;
}

void FmSynthModel::retune(Note& n) {
  const double k = std::exp2(bend_ / 12.0);
  for (Gen& g : n.gens)
    for (Op& o : g.op) o.freq = int32_t(std::lround(std::exp2(double(o.logfreq) / (1 << 24)) * k / instrument().fs * (1 << 24)));
}

void FmSynthModel::keyOn(int midi, int velocity) {
  // How many generators (the unison of a piano's strings): one in the bass, two low, three from
  // the middle up (PJ_DX_ONE, PJ_DX_TWO: the lowest key with two, with three); 1/7 Hz apart
  // (PJ_DX_SPREAD x); their output levels (+-half a step: 0.4 dB) and envelope speeds (+-3 %)
  // their own (PJ_DX_VARY x; the bass's one generator is the FM synth's own).
  static const int kTwo = int(knob("PJ_DX_ONE", 36)), kThree = int(knob("PJ_DX_TWO", 48));
  static const double kSpread = knob("PJ_DX_SPREAD", 1.0), kVary = knob("PJ_DX_VARY", 1.0);
  static const int kMax = int(knob("PJ_DX_POLY", 32));  // notes at once (the FM synth's 16 voices; here more)
  const Voice& v = voice();
  const int ng = midi < kTwo ? 1 : midi < kThree ? 2 : 3;
  // A key struck again: its sounding note goes on in its release, a new one starts (as on an FM synth).
  for (Note& n : notes_)
    if (n.midi == midi && n.down) {
      n.down = false;
      for (Gen& g : n.gens)
        for (Op& o : g.op) o.env.keyDown(false);
    }
  if (int(notes_.size()) >= kMax) {  // the oldest released note, else the oldest, gives way
    auto it = std::find_if(notes_.begin(), notes_.end(), [](const Note& n) { return !n.down; });
    notes_.erase(it != notes_.end() ? it : notes_.begin());
  }
  Note n;
  n.midi = midi;
  n.gens.resize(ng);
  n.norm = 1 / std::sqrt(double(ng));
  const double f0 = 440 * std::exp2((midi - 69) / 12.0);
  const double kRateScale = 44100.0 / instrument().fs;
  for (int gi = 0; gi < ng; ++gi) {
    Gen& g = n.gens[gi];
    const double dHz = ng > 1 ? kSpread * (gi - (ng - 1) / 2.0) / 7.0 : 0.0;
    // the generator's own ways: fixed per key and generator
    unsigned h = 2166136261u ^ unsigned(midi * 131 + gi * 7919);
    auto u = [&h] {  // -1 .. 1
      h ^= h << 13, h ^= h >> 17, h ^= h << 5;
      return (h & 0xffffff) / double(0x800000) - 1;
    };
    const double vary = ng > 1 ? kVary : 0.0;
    for (int i = 0; i < 6; ++i) {
      const OpParams& p = v.op[i];
      Op& o = g.op[i];
      int ol = scaleOutLevel(p.ol);
      ol += scaleLevel(midi, p.bp, p.ld, p.rd, p.lc, p.rc);
      ol = std::min(127, ol);
      ol = ol << 5;
      ol += scaleVelocity(velocity, p.kvs);
      ol += int(std::lround(vary * 16 * u()));
      ol = std::max(0, ol);
      o.env.init(p.rates, p.levels, ol, scaleRate(midi, p.rs), kRateScale * std::exp(vary * 0.03 * u()));
      o.logfreq = oscFreq(f0 + dHz, p);
      // Key sync off: each operator's phase wherever its free-running oscillator is.
      rng_ = rng_ * 1664525u + 1013904223u;
      o.phase = v.oks ? 0 : (rng_ >> 8);
      o.gain = 0;
    }
  }
  retune(n);
  notes_.push_back(std::move(n));
}

void FmSynthModel::keyOff(int midi) {
  for (Note& n : notes_)
    if (n.midi == midi && n.down) {
      n.down = false;
      for (Gen& g : n.gens)
        for (Op& o : g.op) o.env.keyDown(false);
    }
}

void FmSynthModel::setBend(double semitones) {
  if (semitones == bend_) return;
  bend_ = semitones;
  for (Note& n : notes_) retune(n);
}

// One block of 64 samples (the envelopes step once a block, as the FM synth's do in MSFA's engine).
void FmSynthModel::block() {
  constexpr int N = 64;
  std::fill(buf_, buf_ + N, 0.0f);
  const Voice& v = voice();
  const int fbShift = v.fb ? 8 - v.fb : 16;
  for (Note& n : notes_) {
    bool sounding = false;
    for (Gen& g : n.gens) {
      int32_t g1[6], dg[6];
      bool on[6];
      for (int i = 0; i < 6; ++i) {
        Op& o = g.op[i];
        const int32_t level = o.env.step();
        const int32_t g2 = exp2q24(level - (14 << 24));
        g1[i] = o.gain;
        dg[i] = (g2 - o.gain + (N >> 1)) >> 6;
        on[i] = o.gain >= kLevelThresh || g2 >= kLevelThresh;
        o.gain = g2;
      }
      if (on[1] || on[3] || on[5]) sounding = true;
      // Algorithm 5: 6 (feedback) -> 5, 4 -> 3, 2 -> 1; carriers 1, 3, 5.
      Op* op = g.op;
      int32_t gg[6] = {g1[0], g1[1], g1[2], g1[3], g1[4], g1[5]};
      int32_t y0 = g.fb[0], y = g.fb[1];
      for (int s = 0; s < N; ++s) {
        for (int i = 0; i < 6; ++i) gg[i] += dg[i];
        // operator 6, on itself
        int32_t m6 = 0;
        if (on[0]) {
          const int32_t sfb = (y0 + y) >> (fbShift + 1);
          y0 = y;
          y = int32_t((int64_t(sinQ24(op[0].phase + sfb)) * gg[0]) >> 24);
          m6 = y;
        } else {
          y0 = y = 0;
        }
        int64_t out = 0;
        if (on[1]) out += (int64_t(sinQ24(op[1].phase + m6)) * gg[1]) >> 24;
        const int32_t m4 = on[2] ? int32_t((int64_t(sinQ24(op[2].phase)) * gg[2]) >> 24) : 0;
        if (on[3]) out += (int64_t(sinQ24(op[3].phase + m4)) * gg[3]) >> 24;
        const int32_t m2 = on[4] ? int32_t((int64_t(sinQ24(op[4].phase)) * gg[4]) >> 24) : 0;
        if (on[5]) out += (int64_t(sinQ24(op[5].phase + m2)) * gg[5]) >> 24;
        for (int i = 0; i < 6; ++i) op[i].phase += op[i].freq;
        buf_[s] += float(double(out) * n.norm * (1.0 / (1 << 24)));
      }
      g.fb[0] = y0;
      g.fb[1] = y;
    }
    n.quiet = sounding ? 0 : n.quiet + 1;
  }
  // A note whose carriers are all silent (released, or decayed while held: E.PIANO 1's sustain
  // levels are 0) ends.
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(), [](const Note& n) { return n.quiet > 4; }), notes_.end());
}

float FmSynthModel::sample() {
  if (pos_ >= 64) {
    block();
    pos_ = 0;
  }
  // (0.25: three carriers at full gain, x 2 each, sum to 6; levels.py sets the instrument's level)
  last_ = 0.25f * buf_[pos_++];
  return finish(last_);
}

// --- The player.

FmSynthPlayer::FmSynthPlayer(FmSynthModel& m) : Performer(m), m_(m) {}

void FmSynthPlayer::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  articulate(midi, velocity, false);
  keyDown_[midi] = sounding_[midi] = true;
  note_ = midi;
  blowing_ = true;
  // (the director's accent: a harder touch)
  m_.keyOn(midi, std::clamp(int(std::lround(velocity * hardness_)), 1, 127));
}

void FmSynthPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  keyDown_[midi] = false;
  if (!hold_) {
    m_.keyOff(midi);
    sounding_[midi] = false;
  }
}

void FmSynthPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int k = 0; k < 128; ++k)
      if (sounding_[k] && !keyDown_[k]) {
        m_.keyOff(k);
        sounding_[k] = false;
      }
}

void FmSynthPlayer::render(float* out, int n) {
  constexpr int kBlock = 64;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.setBend(2.0 * roll_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
