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
// Above them, the walls' higher plate modes near the closed end, which the
// shaft on the mouth's edge (a long contact, far from them) hardly reaches and
// the stick's tip on the crown does: what makes the crown's stroke a "ping"
// (research/triangle_cowbell.md 2.4; a design, no recording of a crown stroke).
const double kBellHiRatio[] = {8.9, 10.3, 11.8, 13.6};
const double kBellHiDb[] = {-30, -32, -34, -36};
const double kBellHiT60[] = {0.35, 0.3, 0.25, 0.2};
constexpr int kBellHiN = int(sizeof kBellHiRatio / sizeof kBellHiRatio[0]);

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
    def.n = kBellN + kBellHiN;
    Rng sides(777);
    for (int k = 0; k < def.n; ++k) {
      const bool hi = k >= kBellN;
      const double ratio = hi ? kBellHiRatio[k - kBellN] : kBellRatio[k], db = hi ? kBellHiDb[k - kBellN] : kBellDb[k];
      def.freq[k] = float(f0 * ratio);
      def.t60[k] = float((hi ? kBellHiT60[k - kBellN] : kBellT60[k]) * knob("PJ_COWBELL_T60", 1.0));
      def.exc[k] = 1;
      // The sound ~ f x rad x the shape at the stroke: rad to the recorded
      // levels.
      // (The walls' modes 12 dB up: what the stick's ~0.1 ms contact gives
      // them less than the recording's harder or sharper stroke, fitted.)
      def.rad[k] = float(fastPow(10.0, db / 20) / ratio * (k >= 4 ? 4.0 : 1.0));
      def.side[k] = float(0.4 * def.rad[k] * (sides.uniform() >= 0 ? 1 : -1));
    }
    set_.setVib(0, def);
    gain_ = knob("PJ_COWBELL_GAIN", 0.53);  // (a peak ~0.3 at velocity 100; the trim then small)
    // (the crossover: a 2nd-order Butterworth low-pass at 1.1 kHz; the high band is what is left)
    const double w = std::tan(kPi * knob("PJ_COWBELL_XOVER", 1100) / inst.fs), q = std::sqrt(2.0), nrm = 1 / (1 + q * w + w * w);
    xb_[0] = w * w * nrm; xb_[1] = 2 * xb_[0]; xb_[2] = xb_[0];
    xa_[0] = 2 * (w * w - 1) * nrm; xa_[1] = (1 - q * w + w * w) * nrm;
    swayW_ = 2 * kPi * 0.6 / inst.fs;
  }
  excite(0.5);
}

void MetalPercModel::excite(double where, double place) {
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
    // Two zones. The mouth (place < 0.5: the side of the stick on the open
    // edge): the walls' modes change most with where along the edge, the
    // mouth's breathing little, a little less a little further in. The top
    // (place >= 0.5: the tip on the body, toward the closed end): the
    // mouth's breathing fades (its motion is greatest at the open end, least
    // where the top holds the walls: ~-8 dB on the body, ~-14 dB at the
    // crown), and the walls' high modes near the closed end come in. (A larger
    // shape is also a lighter bell under the stick, which then takes less:
    // these are kept near 1, the level comes from the stroke.)
    float shape[ModalSet::kModes] = {};
    const ModalSet::Def& d = set_.def(0);
    const double p = std::clamp(place, 0.0, 1.0);
    const bool top = p >= 0.5;
    const double u = top ? (p - 0.5) * 2 : p * 2;  // where within the zone, 0 .. 1
    static const double kLowBody = knob("PJ_COWBELL_BODY_LOW", 0.4), kLowCrown = knob("PJ_COWBELL_CROWN_LOW", 0.2),
                        kHiTop = knob("PJ_COWBELL_TOP_HI", 1.0);
    const double low = top ? kLowBody + (kLowCrown - kLowBody) * u : 1 - 0.2 * u;
    for (int k = 0; k < d.n; ++k) {
      if (k >= kBellN) {
        shape[k] = float(top ? kHiTop * (0.5 + 0.5 * u) * (0.7 + 0.6 * std::fabs(std::cos((k - kBellN + 2) * kPi * where))) : 0.0);
        continue;
      }
      const double m = 1 + std::floor(kBellRatio[k] * 1.3);
      // (The stick's shaft lies across a few cm of the edge: no wall mode is
      // ever wholly at a node; the tip is a point, so a little more varied.)
      const double wall = top ? 0.4 + 1.0 * std::fabs(std::cos(m * kPi * where)) : 0.6 + 0.8 * std::fabs(std::cos(m * kPi * where));
      shape[k] = float(k < 4 ? low : wall);
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

void MetalPercModel::strikeBell(double speed, double where, double place, double mute) {
  const double p = std::clamp(place, 0.0, 1.0), mu = std::clamp(mute, 0.0, 1.0);
  // The hand: a finger on the broad face damps the mouth's breathing most
  // (the face moves most in it), the walls' modes less. Fully pressed: T60
  // ~0.1 s on the lowest four, ~0.25 s above; a kit's bell with tape inside
  // is about a third of that.
  static const double kLowT = knob("PJ_COWBELL_MUTE_LOW_T60", 0.1), kHiT = knob("PJ_COWBELL_MUTE_HI_T60", 0.25);
  float rate[ModalSet::kModes] = {};
  // (a light touch takes little: the loss grows as the pressure^1.5)
  const double press = mu * std::sqrt(mu);
  for (int k = 0; k < set_.def(0).n; ++k) rate[k] = float(press * 6.91 / (k < 4 ? kLowT : kHiT));
  set_.dampModes(0, mu > 0 ? rate : nullptr);
  excite(where, p);
  // The side of the stick (20 g at the edge, a ~0.1 ms contact) or the tip (lighter, ~7
  // g, and a stiffer local contact: a shorter one that reaches the walls'
  // high modes). The tip is further out on the stick than the shoulder: it
  // moves faster for the same swing (x 1.4).
  static const double kTipSpeed = knob("PJ_COWBELL_TIP_SPEED", 1.4), kTipMass = knob("PJ_COWBELL_TIP", 0.007);
  const bool top = p >= 0.5;
  const double mass = top ? kTipMass : knob("PJ_COWBELL_STICK", 0.02), K = knob("PJ_COWBELL_K", 2e9) * (top ? 2 : 1);
  set_.strike(0, mass, speed * (top ? kTipSpeed : 1), K, 1.5);
}

void MetalPercModel::turn(double degrees, double seconds, bool held) {
  angTo_ = held ? degrees : 0;
  // (the hand turns it as the stick comes in: most of the way there at the stroke, the rest through the ring)
  static const double kAhead = knob("PJ_COWBELL_TURN_AHEAD", 0.9);
  if (held) ang_ += kAhead * (angTo_ - ang_);
  angA_ = held ? 1 - std::exp(-1 / (std::max(0.01, seconds) * fs())) : 1;
  swayAmp_ = held ? knob("PJ_COWBELL_SWAY", 4.0) : 0;
  if (!held) ang_ = 0;
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
  float y = float(gain_) * set_.sample();
  float sideX = 0;
  if (kind_ == kCowbell) {
    // The crossover (direct form II transposed), then the upper band beamed:
    // along the mouth's axis full, off it down by PJ_COWBELL_BEAM (12) dB x
    // (1 - cos): -6 dB tipped up 60 degrees; turned, a little of it to the side.
    const double lo = xb_[0] * y + lp1_;
    lp1_ = xb_[1] * y - xa_[0] * lo + lp2_;
    lp2_ = xb_[2] * y - xa_[1] * lo;
    const double hi = y - lo;
    ang_ += (angTo_ - ang_) * angA_;
    sway_ += swayW_;
    if (sway_ > 2 * kPi) sway_ -= 2 * kPi;
    const double a = (ang_ + swayAmp_ * std::sin(sway_)) * (kPi / 180);
    static const double kBeam = knob("PJ_COWBELL_BEAM", 12.0) / 20 * std::log(10.0);
    const double g = std::exp(-kBeam * (1 - std::cos(a)));
    y = float(lo + g * hi);
    sideX = float(0.3 * sideDir_ * std::sin(a) * hi);
  }
  side_.push_back(float(gain_ * outScale()) * set_.side() + float(outScale()) * sideX);
  last_ = y;
  return finish(y);
}

void MetalPercModel::resetAir() {
  Tube::resetAir();
  set_.reset();
  lp1_ = lp2_ = ang_ = angTo_ = 0;
}

const float* MetalPercModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

MetalPercPlayer::MetalPercPlayer(MetalPercModel& m) : Performer(m), m_(m) {}

double MetalPercPlayer::ur() { return 0.5 * (rng_.uniform() + 1); }

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
    // Key 56 (and any key outside the player's strokes): the open mouth
    // stroke, General MIDI's one cowbell. Keys 60 .. 75: a stroke chosen by
    // the player (src/cowbellplan.h): 4 places x 4 mutes. The hand is never
    // exact: the place and the finger's pressure scatter a little.
    double place = 0.08 * ur(), mute = 0;
    const bool held = midi >= kCowbellStroke0 && midi < kCowbellStroke0 + 16, mounted = midi >= kCowbellMounted0 && midi < kCowbellMounted0 + 16;
    if (held || mounted) {
      const int c = midi - (held ? kCowbellStroke0 : kCowbellMounted0);
      // (0, 1: the mouth, at the edge's middle or further along it; 2, 3: the
      // top, on the body or at the crown; the scatter stays in its zone)
      static const double kPlace[4] = {0.06, 0.36, 0.62, 0.92};
      const int z = c / 4;
      place = std::clamp(kPlace[z] + 0.12 * (ur() - 0.5), z < 2 ? 0.0 : 0.5, z < 2 ? 0.49 : 1.0);
      mute = (c % 4) == 0 ? 0 : std::clamp((c % 4) / 3.0 + 0.1 * (ur() - 0.5), 0.0, 1.0);
    }
    // Held, the bell turns with the stroke: the top struck with the mouth
    // toward the listener, the mouth with it tipped up ~60 degrees; it is
    // nearly there as the stick lands and turns on through the ring (~70 ms), its partials projecting
    // differently as it goes.
    if (held) m_.turn(place < 0.5 ? 60 + 10 * (ur() - 0.5) : 8 * (ur() - 0.5), 0.07 * (0.7 + 0.6 * ur()), true);
    else m_.turn(0, 0.07, false);
    m_.strikeBell(speed(velocity, dyn_) * 1.5 * (1 + 0.04 * (ur() - 0.5)), 0.2 + 0.6 * ur(), place, mute);
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
