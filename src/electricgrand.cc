#include "electricgrand.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"
#include "pianoinst.h"

namespace pajupilli {

namespace {
constexpr int kLowest = 21;  // A0
double lerp(double a, double b, double t) { return a + (b - a) * std::clamp(t, 0.0, 1.0); }
}  // namespace

bool electricGrand(const Instrument& inst) { return std::strcmp(inst.name, "electric-grand") == 0; }

// research/electricgrand.md 2.3, 3.1, 4.3, 7.2. What is known: 88 keys, single wound
// strings A0..F#2 (keys 1..22), two strings a key from G2 up, no
// soundboard; the smaller electric grand's bass string 0.679 m (no other length, and no
// gauge, is published). The rest is the grand's, fitted to what is known:
//   - lengths: the grand's from the middle up; below, about 0.68 m held
//     nearly flat down to A0 (0.73 m): the case is a third of a grand's;
//   - the bass wound (keys 1..30) at 700 N, its stiffness B from 2.0e-3 at
//     A0 (the open electricgrand-model's fit, 2.07e-3) to the plain wire's at key 31;
//     plain wire the grand's gauge on the shorter length (B ~ 1 / L^4: the
//     tenor's partials sharp, the "clank");
//   - the hammers: urethane and leather (the manual), or felt: the grand's
//     felt 1.5x stiffer; far softer in the low bass (fitted to recordings);
//   - the wound strings' wire loss x 3 (their upper partials' decay, fitted);
//   - unisons wider than a grand's (0.4 .. 2 c: the stiff bridge does not
//     pull them together, they beat);
//   - no dampers on the top 8 keys (GSi's model);
//   - the bridge on urethane and a heavy plate: a lossy, near-rigid support
//     (supportR: to be fitted to recordings), read by a piezo per key.
PianoDesign electricGrandDesign() {
  PianoDesign d = grandDesign();
  d.name = "electric-grand";
  static const double kE = 2.02e11, kRho = 7850;
  static const double kHard = knob("PJ_CP_HAMMER", 1.5);  // the hammers' stiffness x the grand's felt
  // The bass hammers, softer: x 0.03 up to D1, back to the rest by D#2.
  static const double kBassHam = knob("PJ_CP_BASSHAM", 0.03), kWoundLoss = knob("PJ_CP_WOUNDLOSS", 3);
  double plainB31 = 0;
  for (int key = 31; key >= 1; --key) {
    PianoKey& s = d.key[key - 1];
    const double f = 440 * std::exp2((key - 49) / 12.0);
    s.len = std::min(s.len, 0.68 + 0.0016 * std::max(0, 31 - key));
    if (key == 31) {  // plain: the grand's gauge, the shorter length
      const double dia = lerp(1.2e-3, 0.775e-3, (key - 21) / 67.0);
      s.mu = kRho * M_PI * dia * dia / 4;
      s.tension = s.mu * (2 * s.len * f) * (2 * s.len * f);
      s.B = M_PI * M_PI * M_PI * kE * dia * dia * dia * dia / (256 * s.mu * f * f * s.len * s.len * s.len * s.len);
      plainB31 = s.B;
    } else {  // wound
      s.tension = 700;
      s.mu = s.tension / ((2 * s.len * f) * (2 * s.len * f));
      s.B = std::exp(lerp(std::log(2.0e-3), std::log(plainB31), (key - 1) / 30.0));
    }
  }
  for (int key = 32; key <= 88; ++key) {  // plain: the grand's gauge on the CP's length
    PianoKey& s = d.key[key - 1];
    const double f = 440 * std::exp2((key - 49) / 12.0);
    const double len = std::min(s.len, 0.68);
    if (len >= s.len) continue;
    const double dia = lerp(1.2e-3, 0.775e-3, (key - 21) / 67.0);
    s.len = len;
    s.tension = s.mu * (2 * s.len * f) * (2 * s.len * f);
    s.B = M_PI * M_PI * M_PI * kE * dia * dia * dia * dia / (256 * s.mu * f * f * s.len * s.len * s.len * s.len);
  }
  for (int key = 1; key <= 88; ++key) {
    PianoKey& s = d.key[key - 1];
    s.strings = key <= 22 ? 1 : 2;
    s.K *= kHard;
    // The lowest octave and a half far darker than a grand's: electric grand
    // recordings (four, ~100 notes measured by tools/pianofit.py) have
    // the energy of A0..D2 nearly all below 1 kHz (1..2 kHz -16 .. -22 dB
    // of it, 2..4 kHz -27 dB and less); the grand's felt here gave -8, -15.
    // A softer bass hammer matches all three bands.
    const double w = std::clamp((19 - key) / 13.0, 0.0, 1.0);  // 1 up to key 6 (D1), 0 from key 19 (D#2)
    s.K *= std::pow(kBassHam, w);
    // The wound strings lose their upper partials twice as fast as the
    // grand's: in the recordings A1..G#2's partials at 0.5 .. 4 kHz die at
    // 10 .. 18 dB/s (2 .. 4 kHz faster still), their lowest ones as the
    // model's; the wire's loss rising with frequency (b2, x 3: 0.5 .. 2 kHz
    // -11 .. -18 dB/s, 2 .. 4 kHz -28 .. -31) fits, where the support's,
    // flat above 150 Hz and the treble's too, would not.
    // (The wire's own losses as fitted here: the grand's of 2026-11-15
    // morning, held, not followed: its later, longer ring is the grand's
    // voicing, not this instrument's, whose decays are fitted to recordings.)
    s.loss = 0.12 * std::exp2(3.5 * (key - 1) / 87.0);
    s.b2 = 3.5e-4;
    s.b3 = 3e-8;
    s.b2 *= std::pow(kWoundLoss, std::clamp((32 - key) / 8.0, 0.0, 1.0));  // x 3 up to key 24 (G#2), x 1 from key 32 (E3)
    s.hasDamper = key <= 80;
    // (No felt nap: the hammers as fitted to the recordings, the grand's of
    // 2026-11-15 midday; urethane and leather have none.)
    s.nap = 0;
    s.napK = 0;
  }
  // Tuned as on a small piano: the middle octave equal, up by 2 : 1, down
  // halfway between 2 : 1 and 4 : 2 (the grand's 6 : 3 on these stiff short
  // strings stretched the bass half a semitone flat, 4 : 2 C3 18 c).
  auto part = [&](int key, int n) { const double B = d.key[key - 1].B; return n * std::sqrt((1 + B * n * n) / (1 + B)); };
  for (int key = 43; key <= 54; ++key) d.key[key - 1].f1 = 440 * std::exp2((key - 49) / 12.0);
  for (int key = 55; key <= 88; ++key) d.key[key - 1].f1 = d.key[key - 13].f1 * part(key - 12, 2);
  for (int key = 42; key >= 1; --key)
    d.key[key - 1].f1 = d.key[key + 11].f1 * std::sqrt(part(key + 12, 1) / part(key, 2) * part(key + 12, 2) / part(key, 4));
  d.unisonCents[0] = 2.0;
  d.unisonCents[1] = 0.4;
  d.bridge = PianoDesign::Bridge::kRigid;
  d.output = PianoDesign::Output::kPickup;
  // The bridge on urethane strips over a duralumin plate (one maker's 1977
  // patent: "a kind of spring and dash pot", to make the decay piano-like):
  // a string's end on a spring and a dashpot loses (T / L) / R a second
  // above the corner (k / R), less below as (f / fc)^2: the fundamentals
  // keep their long ring, the upper partials go about as on a grand.
  d.supportR = knob("PJ_CP_SUPPORT", 4000);
  d.supportFc = knob("PJ_CP_FC", 150);
  // The top six keys reach the bus through the 0.022 uF mixing capacitor:
  // a capacitive divider, flat in frequency (~ -3.5 dB with ~2 nF elements).
  for (int k = 82; k < 88; ++k) d.pickupGain[k] = 0.67f;
  return d;
}

ElectricGrandCircuit::ElectricGrandCircuit(double fs) : fs_(fs) {
  hpA_ = std::exp(-2 * M_PI * 3.4 / fs);
  setBass(0);
  setMiddle(0);
  setTreble(0);
  setBrilliance(2);
}

// The tone section. The diagram's parts are legible, their wiring through the
// brilliance switch is not (research/electricgrand.md 3.3), so the controls are built
// as the manual describes them (flat at the centre detent, each "affects the
// entire keyboard"), first order as a passive network is, at the corners its
// parts give:
//   - bass (VR2, A50k): a low shelf at 1 / (2 pi 3.3 k 0.47 uF) ~ 100 Hz
//     (the 0.47 uF coupling caps against the 1 k + 2.2 k of the middle pot
//     and the FET's source; assumed), +-12 dB;
//   - middle (VR3, A1k with 0.47 uF): a broad peak or dip at
//     1 / (2 pi 1 k 0.47 uF) = 340 Hz, an octave and a half wide, +-10 dB;
//   - treble (VR4, B10k, fed by 2200 pF + 3.3 k): a high shelf at
//     1 / (2 pi (3.3 k + 10 k / 2) 2200 pF) = 8.7 kHz (the pot at its
//     centre), +-12 dB;
//   - brilliance: high flat; medium the treble branch through the 47 k of
//     its divider, 1 / (2 pi 47 k 2200 pF) = 1.5 kHz, a gentle 4 dB cut
//     above it; low through the 4.7 k leg as well, -9 dB above 1.5 kHz
//     (the cut sizes assumed: the divider's 47 k : 4.7 k only bounds them).
// (The gains are assumed: the network's response cannot be computed
// without its wiring.)
void ElectricGrandCircuit::shelf(Biquad* q, double fc, double db, bool high) {
  // A first-order shelf, bilinear with the corner prewarped: unity at the
  // far end, 10^(db/20) at the shelf.
  const double g = std::pow(10.0, db / 20), k = std::tan(M_PI * std::min(fc, 0.45 * fs_) / fs_);
  const double a0 = 1 + k;
  if (high) {  // H(s) = (g s + w) / (s + w)
    q->b0 = (g + k) / a0;
    q->b1 = (k - g) / a0;
  } else {  // H(s) = (s + g w) / (s + w)
    q->b0 = (1 + g * k) / a0;
    q->b1 = (g * k - 1) / a0;
  }
  q->b2 = 0;
  q->a1 = (k - 1) / a0;
  q->a2 = 0;
}

void ElectricGrandCircuit::setBass(double b) {
  b = std::clamp(b, -1.0, 1.0);
  if (b == bassSet_) return;
  bassSet_ = b;
  shelf(&bass_, 100, 12 * b, false);
}

void ElectricGrandCircuit::setMiddle(double m) {
  m = std::clamp(m, -1.0, 1.0);
  if (m == middleSet_) return;
  middleSet_ = m;
  // A peak (RBJ's peaking biquad), 1.5 octaves wide.
  const double A = std::pow(10.0, 10 * m / 40), w = 2 * M_PI * 340 / fs_;
  const double bw = 1.5, alpha = std::sin(w) * std::sinh(std::log(2.0) / 2 * bw * w / std::sin(w));
  const double a0 = 1 + alpha / A;
  middle_.b0 = (1 + alpha * A) / a0;
  middle_.b1 = -2 * std::cos(w) / a0;
  middle_.b2 = (1 - alpha * A) / a0;
  middle_.a1 = -2 * std::cos(w) / a0;
  middle_.a2 = (1 - alpha / A) / a0;
}

void ElectricGrandCircuit::setTreble(double t) {
  t = std::clamp(t, -1.0, 1.0);
  if (t == trebleSet_) return;
  trebleSet_ = t;
  shelf(&treble_, 8700, 12 * t, true);
}

void ElectricGrandCircuit::setBrilliance(int b) {
  b = std::clamp(b, 0, 2);
  if (b == brillSet_) return;
  brillSet_ = b;
  static const double kCut[3] = {-9, -4, 0};
  shelf(&brill_, 1500, kCut[b], true);
}

void ElectricGrandCircuit::reset() {
  hpX_ = hpY_ = 0;
  for (Biquad* q : {&bass_, &middle_, &treble_, &brill_}) q->x1 = q->x2 = q->y1 = q->y2 = 0;
  ldrG_[0] = ldrG_[1] = 0;
}

void ElectricGrandCircuit::process(float in, float* left, float* right) {
  // The input's high-pass (C101 into R102).
  const double hp = hpA_ * (hpY_ + in - hpX_);
  hpX_ = in;
  hpY_ = hp;
  // The tone section.
  const double y = brill_.run(treble_.run(middle_.run(bass_.run(hp))));
  // The tremolo. The Wien bridge's frequency 1 / (2 pi R C), R = 5.6 k + the
  // speed pot (linear, 100 k), C = 2.2 uF; its sine held a little at the
  // peaks by the Zeners.
  double g[2] = {1, 1};
  const bool on = depth_ > 0;
  if (on) {
    const double hz = 1 / (2 * M_PI * (5.6e3 + 100e3 * (1 - speed_)) * 2.2e-6);
    ph_ += 2 * M_PI * hz / fs_;
    if (ph_ > 2 * M_PI) ph_ -= 2 * M_PI;
  }
  const double s = std::tanh(1.5 * std::sin(ph_)) / std::tanh(1.5);
  // The LDR at full light ~11.8 k (each channel dips 8 dB with the depth
  // pot at 0), dark ~2 M; the depth pot in series (0 .. 67 k). Fitted to a
  // electric grand's tremolo at 0.92 Hz (two recordings, the L/R balance folded over
  // the cycle: a near-square swing of +-8 dB, 1 dB rms from this model).
  static const double kGLit = 1 / 11.8e3, kGDark = 1 / 2e6;
  const double rDepth = 67.5e3 * (1 - std::clamp(depth_, 0.0, 1.0));
  for (int c = 0; c < 2; ++c) {
    // The driver switches its LED fully on over a short stretch of the swing
    // (0.20 .. 0.64 of it); the LDR follows at once as it is lit, slowly
    // back in the dark (~43 ms).
    const double drive = on ? std::clamp(((c ? -s : s) - 0.20) / 0.44, 0.0, 1.0) : 0;
    const double target = kGDark + kGLit * drive;
    const double tau = target > ldrG_[c] ? 0.001 : 0.043;
    ldrG_[c] += (1 - std::exp(-1 / (tau * fs_))) * (target - ldrG_[c]);
    // The shunt: LDR + depth, in parallel with the 100 k volume pot, after
    // the 22 k series resistor; 1 when dark.
    const double rsh = 1 / std::max(ldrG_[c], kGDark) + rDepth;
    const double p = rsh * 100e3 / (rsh + 100e3);
    g[c] = (p / (22e3 + p)) / (100e3 / 122e3);
  }
  *left = float(y * g[0]);
  *right = float(y * g[1]);
}

ElectricGrandModel::ElectricGrandModel(const Instrument& inst) : Tube(inst), piano_(inst.fs, electricGrandDesign()), circuit_(inst.fs) {}

void ElectricGrandModel::resetAir() {
  Tube::resetAir();
  piano_.reset();
  circuit_.reset();
}

const float* ElectricGrandModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

float ElectricGrandModel::sample() {
  // The bus: the strings' force on the bridge (N) through the piezos (a
  // mezzo-forte middle C at the program's common level: PJ_CP_GAIN).
  static const float kGain = float(knob("PJ_CP_GAIN", 0.023));
  // (Tests: testDrive's value on the bus itself: there is no board to push.)
  const float bus = kGain * piano_.sample() + float(testDp_);
  testDp_ = 0;
  float l, r;
  circuit_.process(bus, &l, &r);
  last_ = 0.5f * (l + r);
  side_.push_back(float(outScale()) * 0.5f * (l - r));
  return finish(last_);
}

ElectricGrandPlayer::ElectricGrandPlayer(ElectricGrandModel& m) : Performer(m), m_(m) {
  // (PJ_PIANO_PEDAL: the sustain pedal held from the start, as the grand's.)
  if (knob("PJ_PIANO_PEDAL", 0) > 0) setHold(true);
}

void ElectricGrandPlayer::noteOn(int midi, int velocity) {
  const int key = midi - kLowest;
  if (key < 0 || key >= m_.piano().keys()) return;
  articulate(midi, velocity, false);
  down_[key] = true;
  note_ = midi;
  blowing_ = true;
  m_.piano().damper(key, false);
  m_.piano().strike(key, PianoPlayer::hammerSpeed(velocity, dyn_));
}

void ElectricGrandPlayer::noteOff(int midi) {
  const int key = midi - kLowest;
  if (key < 0 || key >= m_.piano().keys()) return;
  down_[key] = false;
  if (!hold_) m_.piano().damper(key, true);
}

void ElectricGrandPlayer::setHold(bool h) {
  hold_ = h;
  for (int k = 0; k < m_.piano().keys(); ++k)
    if (h) m_.piano().damper(k, false);
    else if (!down_[k]) m_.piano().damper(k, true);
}

void ElectricGrandPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  // (Tests: PJ_CP_TREM the tremolo's depth, PJ_CP_SPEED its speed, in place of the knobs.)
  static const double kTrem = knob("PJ_CP_TREM", -1), kSpeed = knob("PJ_CP_SPEED", -1);
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.piano().setVoicing(hardness_);
    m_.circuit().setTreble(std::clamp((brightness_ - 1) / 1.5, -1.0, 1.0));
    const double depth = kTrem >= 0 ? kTrem : std::clamp(vibDepth_ / 0.15, 0.0, 1.0);
    const double speed = kSpeed >= 0 ? kSpeed : std::clamp((vibRate_ - 4) / 3, 0.0, 1.0);
    m_.circuit().setTremolo(depth, speed);
    m_.process(out + i, m);
  }
  blowing_ = !m_.piano().asleep();
}

}  // namespace pajupilli
