#include "metalperc.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "instrument.h"
#include "knob.h"
#include "vmath.h"

namespace pajupilli {

namespace {

constexpr double kPi = 3.14159265358979323846;

// The free-free beam's roots (cos b cosh b = 1), then (n + 1/2) pi.
double freeRoot(int n) {
  static const double r[] = {4.7300407449, 7.8532046241, 10.9956078380, 14.1371654913, 17.2787596574};
  return n <= 5 ? r[n - 1] : (n + 0.5) * kPi;
}

// Its shape at u = x / L (0 .. 1), without cancelling for large b:
// cosh + cos - s (sinh + sin), s = (cosh b - cos b) / (sinh b - sin b); with
// 1 - s = (cos b - sin b - e^-b) / (sinh b - sin b).
double freeShape(double b, double u) {
  const double den = 1 - std::exp(-2 * b) - 2 * std::sin(b) * std::exp(-b);  // (2 (sinh b - sin b)) e^-b
  const double oneMinusS = (std::cos(b) - std::sin(b) - std::exp(-b)) * 2 * std::exp(-b) / den;
  const double s = 1 - oneMinusS;
  // cosh(bu) - s sinh(bu) = e^bu (1 - s) / 2 + e^-bu (1 + s) / 2; e^bu (1 - s) / 2 =
  // (cos b - sin b - e^-b) e^(b (u - 1)) / den.
  const double grow = (std::cos(b) - std::sin(b) - std::exp(-b)) * std::exp(b * (u - 1)) / den;
  return grow + std::exp(-b * u) * (1 + s) / 2 + std::cos(b * u) - s * std::sin(b * u);
}

// A cowbell, measured on a recorded one (FluidR3's General MIDI cowbell, key
// 56: research/triangle_cowbell.md 2.2): the partials against the lowest
// (472 Hz), their levels (dB) and T60s (s). The lowest four: the mouth's
// breathing and its neighbours (a rough cluster); 4-8 x: the box's walls.
const double kBellRatio[] = {1, 1.072, 1.142, 1.229, 4.20, 4.74, 5.28, 5.35, 5.42, 5.52, 5.61, 6.60, 7.14, 7.79};
const double kBellDb[] = {0, -13, -24, -30, -27, -30, -20, -31, -28, -32, -29, -29, -28, -32};
const double kBellT60[] = {0.75, 0.7, 0.74, 0.66, 0.67, 0.4, 0.73, 0.6, 0.4, 0.75, 0.67, 0.65, 0.35, 0.68};
constexpr int kBellN = int(sizeof kBellRatio / sizeof kBellRatio[0]);

}  // namespace

bool metalPerc(const Instrument& inst) {
  return std::strcmp(inst.name, "triangle") == 0 || std::strcmp(inst.name, "cowbell") == 0;
}

MetalPercModel::MetalPercModel(const Instrument& inst)
    : Tube(inst), kind_(std::strcmp(inst.name, "cowbell") == 0 ? kCowbell : kTriangle), set_(inst.fs) {
  if (kind_ == kTriangle) {
    // Steel: E 200 GPa, 7850 kg/m^3. A 10 mm rod, 0.57 m (three 19 cm sides).
    const double d = knob("PJ_TRI_D", 0.010);
    L_ = knob("PJ_TRI_L", 0.57);
    const double c = std::sqrt(2e11 / 7850), kappa = d / 4 * c;  // sqrt(EI / rho A) = r_g c
    const double mass = 7850 * kPi * d * d / 4 * L_;
    const double top = 17000;
    nVib_ = 2;
    for (int fam = 0; fam < 2; ++fam) {
      ModalSet::Def def;
      def.one = true;
      def.modalMass = float(mass);  // (shapes of mean square 1)
      def.zShare = 0;
      int n = 0;
      uint32_t h = 0x9e3779b9u * (fam + 1);
      for (int m = 1; n < 40; ++m) {
        const double b = freeRoot(m);
        // Across the plane the corners stiffen the bending: 4 % at the first
        // mode (156 / 162 Hz measured), about 12 % from the fourth up (the
        // recorded triangle's second family), each a little apart.
        h = h * 1664525u + 1013904223u;
        const double up = fam == 0 ? 1.0 : 1 + std::min(0.12, 0.04 + 0.027 * (m - 1)) + 0.03 * (((h >> 8) & 0xffff) / 65536.0 - 0.5);
        const double f = b * b / (2 * kPi * L_ * L_) * kappa * up;
        if (f > top) break;
        def.freq[n] = float(f);
        beta_[fam][n] = b;
        ++n;
      }
      nBend_[fam] = n;
      // In the plane, the rod's first longitudinal modes (c / 2L, the corners
      // turning a stroke across a side into a push along the next).
      if (fam == 0)
        for (int k = 1; k <= 3 && n < ModalSet::kModes; ++k) def.freq[n++] = float(k * c / (2 * L_));
      def.n = n;
      for (int k = 0; k < n; ++k) {
        const double f = def.freq[k];
        // T60: about 3 s through the 1 - 6 kHz (the recorded open triangle's
        // 2 - 3 s, its sample cut short), shorter above (the air, the rod's own
        // loss): 1.2 s at 10 kHz, 0.7 s at 15 kHz.
        const double t = f < 6000 ? std::min(5.0, 3.0 * std::pow(2000 / f, 0.25)) : 2.28 * fastPow(6000 / f, 1.3);
        def.t60[k] = float(t * knob("PJ_TRI_T60", 1.0));
        // A thin rod radiates its low modes poorly (a line dipole, ka << 1).
        def.rad[k] = float(std::min(2.0, std::pow(f / 2000, 0.5)));
        h = h * 1664525u + 1013904223u;
        def.side[k] = float(0.5 * def.rad[k] * (((h >> 9) & 1) ? 1 : -1));
      }
      set_.setVib(fam, def);
    }
    gain_ = knob("PJ_TRI_GAIN", 0.025);  // (to the program's scale: a peak ~0.3 at velocity 100)
  } else {
    nVib_ = 1;
    ModalSet::Def def;
    def.one = true;
    def.modalMass = float(knob("PJ_COWBELL_MASS", 0.08));
    def.zShare = 0;
    const double f0 = knob("PJ_COWBELL_HZ", 472);
    def.n = kBellN;
    uint32_t h = 777;
    for (int k = 0; k < kBellN; ++k) {
      def.freq[k] = float(f0 * kBellRatio[k]);
      def.t60[k] = float(kBellT60[k] * knob("PJ_COWBELL_T60", 1.0));
      def.exc[k] = 1;
      // The sound ~ f x rad x the shape at the stroke: rad to the recorded
      // levels.
      // (The walls' modes 12 dB up: what the stick's ~0.1 ms contact gives
      // them less than the recording's harder or sharper stroke, fitted.)
      def.rad[k] = float(fastPow(10.0, kBellDb[k] / 20) / kBellRatio[k] * (k >= 4 ? 4.0 : 1.0));
      h = h * 1664525u + 1013904223u;
      def.side[k] = float(0.4 * def.rad[k] * (((h >> 9) & 1) ? 1 : -1));
    }
    set_.setVib(0, def);
    gain_ = knob("PJ_COWBELL_GAIN", 0.53);  // (a peak ~0.3 at velocity 100; the trim then small)
  }
  excite(0.5);
}

void MetalPercModel::excite(double where) {
  if (kind_ == kTriangle) {
    // A stroke across the bottom side (the middle third of the rod) at a
    // slant: in the plane mostly, a little across it (cos / sin of 25 deg);
    // the longitudinal modes through the corners (a fifth).
    const double u = std::clamp(where, 0.02, 0.98);
    for (int fam = 0; fam < 2; ++fam) {
      float shape[ModalSet::kModes] = {};
      const double w = fam == 0 ? std::cos(25 * kPi / 180) : std::sin(25 * kPi / 180);
      for (int k = 0; k < nBend_[fam]; ++k) shape[k] = float(w * freeShape(beta_[fam][k], u));
      if (fam == 0)
        for (int k = nBend_[0], j = 1; k < set_.def(0).n; ++k, ++j) shape[k] = float(0.2 * std::sqrt(2.0) * std::cos(j * kPi * u));
      set_.setExcitation(fam, shape);
    }
  } else {
    // Along the mouth's edge: the walls' modes change most with the place,
    // the mouth's breathing little.
    float shape[ModalSet::kModes] = {};
    const ModalSet::Def& d = set_.def(0);
    for (int k = 0; k < d.n; ++k) {
      const double m = 1 + std::floor(kBellRatio[k] * 1.3);
      // (The stick's shaft lies across a few cm of the edge: no wall mode is
      // ever wholly at a node.)
      shape[k] = float(k < 4 ? 1.0 : 0.6 + 0.8 * std::fabs(std::cos(m * kPi * where)));
    }
    set_.setExcitation(0, shape);
  }
}

void MetalPercModel::strike(double speed, double where) {
  excite(where);
  if (kind_ == kTriangle) {
    // A steel beater (20 g at its tip): steel on steel, crossed rods (Hertz,
    // K = 4/3 E* sqrt(R), R ~ 4 mm: ~9e9 N/m^1.5): over in tens of us.
    for (int fam = 0; fam < 2; ++fam)
      set_.strike(fam, 0.02, speed * (fam == 0 ? 1.0 : 1.0), 9e9, 1.5);
  } else {
    // A wooden stick's shaft on the iron edge (20 g at the shaft; hickory on
    // iron, stiff: K ~2e9 N/m^1.5, a contact of ~0.1 ms that reaches the
    // walls' modes at 2 - 4 kHz).
    set_.strike(0, knob("PJ_COWBELL_STICK", 0.02), speed, knob("PJ_COWBELL_K", 2e9), 1.5);
  }
}

void MetalPercModel::setHand(bool on) {
  if (on == hand_) return;
  hand_ = on;
  // The hand clasps the triangle: every mode dies within ~0.4 s (T60). (A
  // cowbell held in the hand is the cowbell's own shorter ring.)
  static const double rate = 6.91 / knob("PJ_TRI_MUTE_T60", 0.4);
  for (int i = 0; i < nVib_; ++i) set_.damp(i, on ? rate : 0);
}

float MetalPercModel::sample() {
  const float y = float(gain_) * set_.sample();
  side_.push_back(float(gain_ * outScale()) * set_.side());
  last_ = y;
  return finish(y);
}

void MetalPercModel::resetAir() {
  Tube::resetAir();
  set_.reset();
}

const float* MetalPercModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

MetalPercPlayer::MetalPercPlayer(MetalPercModel& m) : Performer(m), m_(m) {}

double MetalPercPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

double MetalPercPlayer::speed(int velocity, double dynamics) {
  // ~ velocity^1.65: General MIDI's ~30 dB from 16 to 127 (a steel or hickory
  // contact this stiff hardly brightens with speed: the level alone carries it).
  static const double top = knob("PJ_MP_VMAX", 3.0), ex = knob("PJ_MP_VEXP", 1.65);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return top * fastPow(v, ex) * (0.75 + 0.5 * std::clamp(dynamics, 0.0, 1.0));
}

void MetalPercPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  if (m_.kind() == MetalPercModel::kTriangle) {
    // 80: the hand clasps it (and chokes what rings); 81 (or any other key):
    // open, the hand off, unless the hold keeps it on.
    m_.setHand(midi == 80 || hold_);
    // The bottom side: the middle third of the rod, a little elsewhere each time.
    m_.strike(speed(velocity, dyn_) * (1 + 0.04 * (ur() - 0.5)), 0.42 + 0.12 * ur());
  } else {
    m_.strike(speed(velocity, dyn_) * 1.5 * (1 + 0.04 * (ur() - 0.5)), 0.2 + 0.6 * ur());
  }
}

void MetalPercPlayer::noteOff(int) {}

void MetalPercPlayer::setHold(bool h) {
  hold_ = h;
  if (m_.kind() == MetalPercModel::kTriangle && h) m_.setHand(true);
}

void MetalPercPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
}

}  // namespace pajupilli
