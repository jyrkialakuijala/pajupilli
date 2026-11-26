#include "scifi.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

bool sciFi(const Instrument& inst) { return std::strcmp(inst.name, "fx-sci-fi") == 0; }

SciFiModel::SciFiModel(const Instrument& inst) : Tube(inst) {
  // (the knobs' starting places; live's K1-K4 and -seq's "knob K X" move them)
  fold_ = std::clamp(knob("PJ_SCIFI_FOLD", 0.5), 0.0, 1.0);
  ratio_ = std::clamp(knob("PJ_SCIFI_RATIO", 0.684), 0.0, 1.0);
  balance_ = std::min(1.0, knob("PJ_SCIFI_BALANCE", -1));
  ring_ = std::clamp(knob("PJ_SCIFI_RING", 0.4), 0.0, 1.0);
}

namespace {

constexpr double kTwoPi = 6.283185307179586;

// The 1 ms Hann impulse's n-th sample (of len).
double pulseAt(int n, int len) {
  const double s = std::sin(M_PI * (n + 0.5) / len);
  return s * s;
}

}  // namespace

void SciFiModel::keyOn(int midi, int velocity) {
  static const int kMax = int(knob("PJ_SCIFI_POLY", 24));
  const double fs = instrument().fs;
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  // A key struck again: its sounding note goes on in its release, a new one starts.
  for (Note& n : notes_)
    if (n.midi == midi && n.down) n.down = false;
  if (int(notes_.size()) >= kMax) {  // the oldest released note, else the oldest, gives way
    auto it = std::find_if(notes_.begin(), notes_.end(), [](const Note& n) { return !n.down; });
    notes_.erase(it != notes_.end() ? it : notes_.begin());
  }
  Note n;
  n.midi = midi;
  n.f0 = 440 * std::exp2((midi - 69) / 12.0);
  const double oct = (midi - 60) / 12.0;  // octaves from middle C

  // The discharge. The drive: 1 (a sine, barely folded) .. the fold knob's 1.5 .. 30, by the
  // touch. The drop: 0.7 .. 3 octaves by the touch, starting no higher than 5 kHz; quicker high
  // up (12 .. 120 ms). The collapse: the drive falls e-fold in 60 ms (top) .. 1.2 s (bottom).
  const double gMax = 1.5 * fastPow(20.0, fold_);
  const double z0 = 1 + (gMax - 1) * fastPow(v, 1.3);
  n.drop = std::min(0.7 + 2.3 * v, std::max(0.0, std::log2(5000 / n.f0)));
  const double tauP = std::clamp(0.045 * std::exp2(-oct / 3), 0.012, 0.12);
  const double tauZ = std::clamp(0.35 * std::exp2(-oct / 2.5), 0.06, 1.2);
  n.pitchDecay = std::exp(-1 / (tauP * fs));
  n.z = z0;
  n.zDecay = std::exp(-1 / (tauZ * fs));
  n.bias = 0.15 + 0.35 * urand();
  n.dc = std::sin(n.bias) * std::cyl_bessel_j(0.0, std::min(z0, 20.0));
  n.foldAmp = 0.35 + 0.65 * v;
  n.relDecay = std::exp(-1 / (0.15 * fs));
  n.phi = 0;  // (sine phase from zero: the shot starts at once, no click)

  // The ping. The modulator's ratio (0.05 .. 4, the knob; drifting 6 % per three octaves off
  // middle C), the FM index by the touch (0.25 .. 1.75), the cutoff's deviation that makes,
  // held under 0.9 of the note and under what keeps the sidebands below 0.45 fs.
  const double r = 0.05 * fastPow(80.0, ratio_) * fastPow(1.06, oct / 3);
  n.ratio = r;
  // (below a ratio of 1 the deviation shrinks as the ratio squared: a low ratio is a pure ping
  // with a slight shimmer, not a siren)
  const double beta = 0.25 + 1.5 * v;
  n.dev = std::clamp(std::min(0.9, beta * r * std::min(1.0, r)), 0.0, std::max(0.0, 0.45 * fs / n.f0 - 1 - 2 * r));
  // Damping from the ring time: T60 = Q ln(1000) / (pi f) held (0.25 .. 64 s at middle C,
  // halving every three octaves up); released at most 0.35 s (quicker high up).
  const double t60 = std::clamp(0.25 * fastPow(256.0, ring_) * std::exp2(-oct / 3), 0.05, 120.0);
  const double t60r = std::min(t60, 0.35 * std::exp2(-oct / 4));
  n.kHeld = 6.908 / (M_PI * n.f0 * t60);
  n.kRel = 6.908 / (M_PI * n.f0 * t60r);
  n.k = n.kHeld;
  // The impulse: 1 ms, shorter above 450 Hz (0.45 of a period); scaled by its spectrum at the
  // note so that the ring's amplitude is the touch's (0.15 .. 1). (A resonator's ring after a
  // short input x: about 2 g |X(w)|, g = tan(w / 2).)
  n.pulseLen = std::max(3, int(std::lround(std::min(0.001, 0.45 / n.f0) * fs)));
  const double w = kTwoPi * n.f0 / fs;
  std::complex<double> X = 0;
  for (int i = 0; i < n.pulseLen; ++i) X += pulseAt(i, n.pulseLen) * std::polar(1.0, -w * i);
  const double g = std::tan(0.5 * w);
  n.pingAmp = (0.15 + 0.85 * fastPow(v, 1.2)) / (2 * g * std::max(1e-9, std::abs(X)));
  n.psi = 0.25;  // (the modulator at its crest's zero crossing: the ping starts on the note)
  // The balance: by the key (the bass mostly the discharge, the treble mostly the ping), or K3.
  n.mix = balance_ >= 0 ? balance_ : std::clamp(0.5 + 0.25 * oct / 3.5, 0.2, 0.8);
  notes_.push_back(n);
}

void SciFiModel::keyOff(int midi) {
  for (Note& n : notes_)
    if (n.midi == midi && n.down) n.down = false;
}

float SciFiModel::sample() {
  const double fs = instrument().fs, ifs = 1 / fs;
  const double bk = std::exp2(bend_ / 12);
  double out = 0;
  for (Note& n : notes_) {
    // The discharge: the falling sine into the folder.
    double fold = 0;
    const double amp = n.foldAmp * n.rel;
    if (amp * std::min(1.0, n.z) > 1e-5) {
      const double fi = n.f0 * bk * std::exp2(n.drop * n.pitchEnv);
      // (the folder's spectrum: J_m(z) at m fi, negligible from m ~ z + 3: held under 20 kHz)
      const double z = std::min(n.z, std::max(0.0, 20000 / fi - 3));
      if ((n.age & 31) == 0) n.dc = std::sin(n.bias) * std::cyl_bessel_j(0.0, z);
      fold = amp * (std::sin(z * std::sin(kTwoPi * n.phi) + n.bias) - n.dc);
      n.phi += fi * ifs;
      n.phi -= std::floor(n.phi);
      n.pitchEnv *= n.pitchDecay;
      n.z *= n.zDecay;
      if (!n.down) n.rel *= n.relDecay;
    }
    // The ping: the impulse into the band-pass, its cutoff swung by the modulator.
    double x = 0;
    if (n.pulsePos < n.pulseLen) x = n.pingAmp * pulseAt(n.pulsePos++, n.pulseLen);
    const double fc = n.f0 * bk * (1 + n.dev * std::sin(kTwoPi * n.psi));
    n.psi += n.ratio * n.f0 * bk * ifs;
    n.psi -= std::floor(n.psi);
    const double g = std::tan(M_PI * std::min(fc, 0.47 * fs) * ifs);
    n.k += ((n.down ? n.kHeld : n.kRel) - n.k) * 0.002;
    const double a1 = 1 / (1 + g * (g + n.k)), a2 = g * a1, a3 = g * a2;
    const double v3 = x - n.ic2;
    const double v1 = a1 * n.ic1 + a2 * v3;
    const double v2 = n.ic2 + a2 * n.ic1 + a3 * v3;
    n.ic1 = 2 * v1 - n.ic1;
    n.ic2 = 2 * v2 - n.ic2;
    // (the swung cutoff can pump the ring, as a swing pumped at twice its rate: held at the
    // self-oscillating filter's limit)
    const double e = n.ic1 * n.ic1 + n.ic2 * n.ic2;
    if (e > 4) {
      const double s = 2 / std::sqrt(e);
      n.ic1 *= s;
      n.ic2 *= s;
    }
    const double ping = std::tanh(1.2 * v1) * (1 / 1.2);
    out += (1 - n.mix) * fold + n.mix * ping;
    ++n.age;
    n.quiet = (n.pulsePos >= n.pulseLen && amp * std::min(1.0, n.z) <= 1e-5 && e < 1e-10) ? n.quiet + 1 : 0;
  }
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(), [](const Note& n) { return n.quiet > 256; }), notes_.end());
  // (a NaN: Tube::process clears the notes, resetAir)
  last_ = float(0.5 * out);
  return finish(last_);
}

// --- The player.

SciFiPlayer::SciFiPlayer(SciFiModel& m) : Performer(m), m_(m) {}

void SciFiPlayer::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  articulate(midi, velocity, false);
  keyDown_[midi] = sounding_[midi] = true;
  note_ = midi;
  blowing_ = true;
  // (the director's accent: a harder touch)
  m_.keyOn(midi, std::clamp(int(std::lround(velocity * hardness_)), 1, 127));
}

void SciFiPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  keyDown_[midi] = false;
  if (!hold_) {
    m_.keyOff(midi);
    sounding_[midi] = false;
  }
}

void SciFiPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int k = 0; k < 128; ++k)
      if (sounding_[k] && !keyDown_[k]) {
        m_.keyOff(k);
        sounding_[k] = false;
      }
}

bool SciFiPlayer::setKnob(int k, double x) {
  x = std::clamp(x, 0.0, 1.0);
  switch (k) {
    case 0: m_.setFold(x); return true;     // K1: the fold
    case 1: m_.setRatio(x); return true;    // K2: the modulator's ratio
    case 2: m_.setBalance(x); return true;  // K3: discharge .. ping
    case 3: m_.setRing(x); return true;     // K4: the ping's ring
    default: return false;
  }
}

void SciFiPlayer::render(float* out, int n) {
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
