#include "echoes.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {

bool fxEchoes(const Instrument& inst) { return std::strcmp(inst.name, "fx-echoes") == 0; }

namespace {

constexpr double kTwoPi = 6.283185307179586;
// The fork: a pair of prongs, each a clamped-free bar (Euler-Bernoulli: the bending modes at
// 1, 6.267, 17.55 x the first), struck near the tip.
constexpr double kRatio[EchoesModel::kModes] = {1, 6.267, 17.55};
// The resonance box: its Q, and its detuning from the fork (each box cut a little off, +-kBoxCut).
constexpr double kBoxQ = 25, kBoxCut = 0.008;
// What the microphone hears besides the box: the prongs' own radiation (weak for the note, a
// quadrupole; rising with frequency), and the octave a loud fork's stem gives.
constexpr double kDirect = 0.06, kOctave = 0.35;
// The tape: the playback heads' distances from the record head (m), the speed range (m/s;
// kSpeedLo x kSpeedSpan^K1), the motor's time constant (s), the head-to-tape spacing (m; worn
// tape and heads more), the tape's saturation (the record level where the anhysteretic curve
// bends), the playback's coupling high-pass (Hz).
constexpr double kHead[2] = {0.06, 0.12};
constexpr double kSpeedLo = 0.08, kSpeedSpan = 5, kMotor = 0.35;
constexpr double kSpacing0 = 3e-6, kSpacingWear = 8e-6, kGammaMin = 0.5;
constexpr double kSat = 3, kCoupling = 60;
// Wow and flutter (fractions of the speed, at the default wear): the capstan's eccentricity (its
// diameter kCapstan), the pinch roller's, the loop's splice and tension (once per kLoop of tape),
// and scrape flutter (noise below ~30 Hz).
constexpr double kCapstan = 0.012, kPinch = 0.03, kLoop = 1.6;
constexpr double kWowCap = 0.0012, kWowPinch = 0.0006, kWowLoop = 0.002, kScrape = 0.0003;
constexpr int kTape = 1 << 17, kTaps = 24;  // (the tape's memory: 2.7 s; the head's kernel +-24 samples:
// the spacing loss's cusp at low frequencies lives in the kernel's 1/t^2 tails)
constexpr double kLevel = 0.5, kWidth = 0.5;
constexpr int kMaxNotes = 32;

// The anhysteretic magnetisation (AC bias): 3 L(u), the Langevin function scaled to slope 1.
inline double langevin3(double u) {
  if (std::fabs(u) < 1e-3) return u - u * u * u / 15;
  return 3 * (1 / std::tanh(u) - 1 / u);
}

}  // namespace

EchoesModel::EchoesModel(const Instrument& inst) : Tube(inst) {
  // (the knobs' starting places; live's K1-K4 and -seq's "knob K X" move them)
  rate_ = std::clamp(knob("PJ_ECHO_RATE", 0.5), 0.0, 1.0);
  intensity_ = std::clamp(knob("PJ_ECHO_INTENSITY", 0.5), 0.0, 1.0);
  volume_ = std::clamp(knob("PJ_ECHO_VOLUME", 0.5), 0.0, 1.0);
  wear_ = std::clamp(knob("PJ_ECHO_WEAR", 0.5), 0.0, 1.0);
  resetAir();
}

void EchoesModel::resetAir() {
  Tube::resetAir();
  notes_.clear();
  side_.clear();
  // A blank tape, already running at the knob's speed.
  const double h = 1 / instrument().fs;
  speed_ = kSpeedLo * std::pow(kSpeedSpan, rate_);
  tapeM_.assign(kTape, 0.0f);
  tapePos_.assign(kTape, 0.0);
  n_ = kTape;
  pos_ = 0;
  for (int i = 0; i < kTape; ++i) tapePos_[i] = (i - kTape) * speed_ * h;
  for (int k = 0; k < 2; ++k) {
    rd_[k] = kTape - kHead[k] / (speed_ * h) - 1;
    hp_[k][0] = hp_[k][1] = head_[k] = 0;
  }
  idleLimit_ = kTape;
  idle_ = idleLimit_ + 1;
}

const float* EchoesModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

// The fork's modes at its pitch (bent), and the box.
void EchoesModel::tune(Note& n) {
  const double fs = instrument().fs;
  const double f1 = n.f0 * bk_;
  n.modes = 0;
  for (int k = 0; k < kModes; ++k) {
    if (f1 * kRatio[k] > 0.45 * fs) break;
    n.w[k] = kTwoPi * f1 * kRatio[k];
    n.modes = k + 1;
  }
  decay(n);
  // The box: a band-pass (RBJ, 0 dB at its peak) on the stem's velocity.
  const double w0 = kTwoPi * f1 * (1 + n.boxDetune) / fs, al = std::sin(w0) / (2 * kBoxQ);
  n.b0 = al / (1 + al);
  n.a1 = -2 * std::cos(w0) / (1 + al);
  n.a2 = (1 - al) / (1 + al);
}

// Each mode's rotation and decay a sample (the damper's share as it lands).
void EchoesModel::decay(Note& n) {
  const double h = 1 / instrument().fs;
  for (int k = 0; k < n.modes; ++k) {
    const double s = n.sFree[k] + n.damper * n.sDamp[k], r = std::exp(-s * h);
    n.cr[k] = float(r * std::cos(n.w[k] * h));
    n.ci[k] = float(r * std::sin(n.w[k] * h));
  }
}

void EchoesModel::setBend(double semitones) {
  if (semitones == bend_) return;
  bend_ = semitones;
  bk_ = std::exp2(semitones / 12);
  for (Note& n : notes_) tune(n);
}

void EchoesModel::keyOn(int midi, int velocity) {
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  for (Note& n : notes_)
    if (n.midi == midi && n.down) keyOff(midi);
  if (int(notes_.size()) >= kMaxNotes) {  // the oldest released note, else the oldest, gives way
    auto it = std::find_if(notes_.begin(), notes_.end(), [](const Note& n) { return !n.down; });
    notes_.erase(it != notes_.end() ? it : notes_.begin());
  }
  notes_.emplace_back();
  Note& n = notes_.back();
  n.midi = midi;
  n.f0 = 440 * std::exp2((midi - 69) / 12.0);
  // The fork's own losses (its mounting and the box take the note's energy over seconds; the
  // clang's modes die in a fraction of one) and the damper's felt.
  const double f = n.f0;
  const double t60[kModes] = {std::clamp(20 * std::pow(f / 261.63, -0.5), 3.0, 40.0),
                              std::clamp(0.8 * std::pow(f * kRatio[1] / 1640, -0.5), 0.08, 3.0),
                              std::clamp(0.35 * std::pow(f * kRatio[2] / 4590, -0.5), 0.04, 1.5)};
  const double tDamp = std::clamp(0.2 * std::pow(f / 261.63, -0.2), 0.1, 0.4);
  for (int k = 0; k < kModes; ++k) {
    n.sFree[k] = 6.91 / t60[k];
    n.sDamp[k] = 6.91 / (k ? 0.08 : tDamp);
  }
  // Each box cut a little off its fork (fixed per key).
  uint32_t hsh = uint32_t(midi) * 2654435761u;
  hsh ^= hsh >> 13;
  n.boxDetune = kBoxCut * (2 * ((hsh & 1023) / 1023.0) - 1);
  n.pan = std::clamp((midi - 64) / 100.0, -0.2, 0.2);
  std::memset(n.re, 0, sizeof(n.re));
  std::memset(n.im, 0, sizeof(n.im));
  tune(n);
  // The hammer: its speed from the key; the felt's contact time at 1 m/s (shorter for the small
  // treble forks), the felt stiffening as it is squeezed (Hertz: F ~ compression^2.5).
  n.vh = 0.02 + 3 * std::pow(v, 1.4);
  n.yh = 0;
  n.mh = 0.25;
  const double tc = std::clamp(1.2e-3 * std::pow(261.63 / f, 0.7), 0.1e-3, 5e-3);
  n.kh = n.mh * (M_PI / tc) * (M_PI / tc);
  n.dref = std::sqrt(n.mh / n.kh);  // (the compression at 1 m/s on a linear felt)
  n.hammer = true;
  idle_ = 0;
}

void EchoesModel::keyOff(int midi) {
  for (Note& n : notes_)
    if (n.midi == midi && n.down) n.down = false;
}

// A playback head over the tape at `target` (m along the tape): the recorded magnetisation where
// the tape now under it was written, seen through the head's spacing (a Poisson kernel `gamma`
// samples wide; it also interpolates between the samples).
double EchoesModel::readHead(int k, double target, double gamma) {
  constexpr int64_t mask = kTape - 1;
  int64_t i = int64_t(rd_[k]);
  if (i < n_ - kTape + kTaps + 1) i = n_ - kTape + kTaps + 1;
  while (i + 1 < n_ - kTaps - 1 && tapePos_[(i + 1) & mask] <= target) ++i;
  const double p0 = tapePos_[i & mask], p1 = tapePos_[(i + 1) & mask];
  const double fr = std::clamp(p1 > p0 ? (target - p0) / (p1 - p0) : 0.0, 0.0, 1.0);
  rd_[k] = i + fr;
  const double g2 = gamma * gamma;
  double acc = 0, ws = 0;
  for (int j = -kTaps + 1; j <= kTaps; ++j) {
    const double d = j - fr, w = 1 / (g2 + d * d);
    acc += w * tapeM_[(i + j) & mask];
    ws += w;
  }
  return acc / ws;
}

float EchoesModel::sample() {
  const double fs = instrument().fs, h = 1 / fs;
  // --- The forks.
  double dry = 0, side = 0;
  for (Note& n : notes_) {
    // The damper lands over ~15 ms after the key-up.
    if (!n.down && n.damper < 1) {
      n.damper = std::min(1.0, n.damper + h / 0.015);
      decay(n);
    }
    double x = 0;
    for (int k = 0; k < n.modes; ++k) {
      const float r = n.re[k], i = n.im[k];
      n.re[k] = n.cr[k] * r - n.ci[k] * i;
      n.im[k] = n.cr[k] * i + n.ci[k] * r;
      x += n.im[k];
    }
    if (n.hammer) {  // the felt on the prong's tip
      const double d = n.yh - x;
      if (d > 0) {
        const double F = n.kh * d * std::pow(d / n.dref, 1.5);
        n.vh -= F / n.mh * h;
        for (int k = 0; k < n.modes; ++k) n.re[k] += float(F * h / n.w[k]);
      } else if (n.vh < 0 || n.t > 0.02) {
        n.hammer = false;  // (it has bounced off)
      }
      n.yh += n.vh * h;
    }
    // The stem's velocity drives the box; the prongs radiate a little themselves (more for the
    // higher modes); a loud fork's stem gives its octave.
    const double u1 = n.w[0] * n.re[0];
    double direct = 0;
    for (int k = 0; k < n.modes; ++k) direct += (n.w[k] / n.w[0]) * n.w[k] * n.re[k];
    const double by = n.b0 * (u1 - n.bx2) - n.a1 * n.by1 - n.a2 * n.by2;
    n.bx2 = n.bx1;
    n.bx1 = u1;
    n.by2 = n.by1;
    n.by1 = by;
    const double o = by + kDirect * direct + kOctave * (n.w[0] * n.im[0]) * u1;
    dry += o;
    side += n.pan * o;
    n.t += h;
    n.quiet = (!n.down && n.damper >= 1 && std::fabs(o) < 1e-7 && u1 * u1 < 1e-14) ? n.quiet + 1 : 0;
  }
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(), [](const Note& n) { return n.quiet > 256; }), notes_.end());

  // --- The tape.
  const double wear = 0.2 + 3.2 * wear_ * wear_;
  speed_ += (kSpeedLo * std::pow(kSpeedSpan, rate_) - speed_) * h / kMotor;
  {
    const double kn = 1 - std::exp(-kTwoPi * 30 * h), nsig = std::sqrt(3 * (2 - kn) / kn);
    scrape_ += kn * (nsig * (2 * urand() - 1) - scrape_);
    scrape2_ += kn * (scrape_ - scrape2_);
  }
  const double wow = wear * (kWowCap * std::sin(phCap_) + kWowPinch * std::sin(phPinch_) + kWowLoop * std::sin(phLoop_) +
                             kScrape * 1.4 * scrape2_);
  const double v = speed_ * (1 + wow);
  phCap_ = std::fmod(phCap_ + kTwoPi * v / (M_PI * kCapstan) * h, kTwoPi);
  phPinch_ = std::fmod(phPinch_ + kTwoPi * v / (M_PI * kPinch) * h, kTwoPi);
  phLoop_ = std::fmod(phLoop_ + kTwoPi * v / kLoop * h, kTwoPi);
  pos_ += v * h;
  // The record head: the microphone's signal and the second head's, through the bias's
  // anhysteretic curve.
  const double g = 1.2 * std::pow(intensity_, 1.5);
  const double in = dry + g * head_[1];
  const float m = float(kSat * langevin3(in / kSat));
  tapeM_[n_ & (kTape - 1)] = m;
  tapePos_[n_ & (kTape - 1)] = pos_;
  ++n_;
  // The playback heads, each through its spacing (Wallace: exp(-2 pi d f / v); a slower tape
  // or a worn one is darker) and the playback's coupling.
  const double gamma = (kSpacing0 + kSpacingWear * wear_) / v * fs + kGammaMin;
  const double ahp = 1 / (1 + kTwoPi * kCoupling * h);
  for (int k = 0; k < 2; ++k) {
    const double y = readHead(k, pos_ - kHead[k], gamma);
    hp_[k][1] = ahp * (hp_[k][1] + y - hp_[k][0]);
    hp_[k][0] = y;
    head_[k] = hp_[k][1];
  }
  // Asleep once the tape has nothing on it worth hearing.
  idle_ = (std::fabs(in) > 1e-6 || std::fabs(head_[0]) > 1e-6 || !notes_.empty()) ? 0 : idle_ + 1;
  // Out: the forks (panned by key), head 1 to the left, head 2 to the right (each kWidth of the
  // way: fully apart, a held chord's echoes beating against each other would leave the two
  // sides anticorrelated in a room).
  const double e = 1.2 * volume_;
  const double out = dry + e * 0.5 * (head_[0] + head_[1]);
  side += e * 0.5 * kWidth * (head_[0] - head_[1]);
  side_.push_back(float(outScale() * kLevel * side));
  last_ = float(kLevel * out);
  return finish(last_);
}

// --- The player.

EchoesPlayer::EchoesPlayer(EchoesModel& m) : Performer(m), m_(m) {}

void EchoesPlayer::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  articulate(midi, velocity, false);
  keyDown_[midi] = sounding_[midi] = true;
  note_ = midi;
  blowing_ = true;
  // (the director's accent: a harder touch)
  m_.keyOn(midi, std::clamp(int(std::lround(velocity * hardness_)), 1, 127));
}

void EchoesPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  keyDown_[midi] = false;
  if (!hold_) {
    m_.keyOff(midi);
    sounding_[midi] = false;
  }
}

void EchoesPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int k = 0; k < 128; ++k)
      if (sounding_[k] && !keyDown_[k]) {
        m_.keyOff(k);
        sounding_[k] = false;
      }
}

bool EchoesPlayer::setKnob(int k, double x) {
  x = std::clamp(x, 0.0, 1.0);
  switch (k) {
    case 0: m_.setRate(x); return true;       // K1: the tape's speed (the echo interval)
    case 1: m_.setIntensity(x); return true;  // K2: the intensity (the repeats)
    case 2: m_.setVolume(x); return true;     // K3: the echo's volume
    case 3: m_.setWear(x); return true;       // K4: the tape's wear
    default: return false;
  }
}

void EchoesPlayer::render(float* out, int n) {
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
