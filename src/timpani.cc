#include "timpani.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2, kC = 343.0;
constexpr double kSigma = 0.26;  // the Mylar film, 0.19 mm (kg/m^2)

// The drums (research/timpani.md 12.2): head radius (m), the keys they take.
struct Size { double inch, radius; int lo, hi; };
constexpr Size kSizes[] = {
    {32, 0.406, 36, 44}, {29, 0.368, 45, 49}, {26, 0.330, 50, 54}, {23, 0.292, 55, 58}, {20, 0.254, 59, 64},
};

// The pitched family's measured ratios, (1,1) .. (6,1) [T] (2.3).
constexpr double kFamily[] = {1.0, 1.50, 1.98, 2.44, 2.90, 3.37};

// A mode's ratio to the (1,1) on the drum, from its ideal-membrane ratio: measured where there is
// a measurement (2.1, 2.2), otherwise the ideal one stretched by the air riding on the head, whose
// load falls with frequency (the effective density 0.26 + 0.09 (4 / r)^2 against the (1,1)'s
// 0.61: 12.1, a fixed point).
double ratioOf(int m, int n, double ideal) {
  if (n == 1 && m >= 1 && m <= 6) return kFamily[m - 1];
  struct Fixed { int m, n; double r; };
  static constexpr Fixed kFixed[] = {{0, 2, 1.65}, {0, 3, 2.70}, {1, 2, 2.05}, {2, 2, 2.80}, {3, 2, 3.27}, {1, 3, 3.38}, {4, 2, 3.80}};
  for (const Fixed& f : kFixed)
    if (f.m == m && f.n == n) return f.r;
  double r = ideal * 1.3;
  for (int it = 0; it < 8; ++it) r = ideal * std::sqrt(0.61 / (0.26 + 0.09 * (4 / r) * (4 / r)));
  return r;
}

double midiHz(double midi) { return 440 * std::exp2((midi - 69) / 12); }
}  // namespace

bool timpaniInstrument(const Instrument& inst) { return std::strcmp(inst.name, "timpani") == 0; }

Kettle::Kettle(double fs, int index) : fs_(fs), dt_(1 / fs) {
  const Size& z = kSizes[std::clamp(index, 0, 4)];
  radius_ = z.radius;
  lo_ = z.lo;
  hi_ = z.hi;
  pan_ = 0.12 * (index - 2);  // (the low drums on the left, as the audience sees an American setup's mirror)
  // The head's tension: the bare film's (1,1) where the loaded one is at the middle of the drum's
  // range (only the modes' count and the stretch depend on it: their frequencies are set below).
  const double fMid = midiHz(0.5 * (z.lo + z.hi));
  const double sig11 = kSigma + 0.35 * radius_ / 0.33;
  const double fBare = fMid * std::sqrt(sig11 / kSigma);
  HeadSpec h;
  h.radius = radius_;
  h.sigma = kSigma;
  const double c = 2 * kPi * radius_ * fBare / 3.832;
  h.tension = kSigma * c * c;
  static const double kModes = knob("PJ_TIMP_MODES", 500);
  h.top = std::clamp(std::sqrt(kModes) * c / (kPi * radius_), 600.0, 4000.0);
  h.airMass = 0;
  h.split = 0.004;  // (a well-cleared head: 12.1)
  h.stretch = knob("PJ_TIMP_STRETCH", 9e5);
  h.seed = 61 + uint32_t(index);
  head_.reset(new Membrane(fs, h));
  const int n = head_->modes();
  ideal_.resize(n);
  for (int k = 0; k < n; ++k) ideal_[k] = head_->frequency(k) / head_->split(k) / fBare;
  phi_.assign(n, 0.0f);
  tune(fMid);
}

void Kettle::tune(double f11) {
  if (f11 == f11_) return;
  f11_ = f11;
  retune();
}

void Kettle::retune() {
  Membrane& h = *head_;
  const int n = h.modes();
  static const double kDecay = knob("PJ_TIMP_DECAY", 1.0);
  const double air = radius_ / 0.33;  // (the air's share scales with the head)
  // The main tone's decay first: the fifth and the octave are held at least 1.5 and 2.1 times it
  // (the fits below were made at ka under ~1.6: at a small drum's top, beyond their data, they
  // fell under the main tone; 12.3's table for B3 and F#3 keeps them ~1.5 and ~2.1 times).
  const double ka11 = 2 * kPi * f11_ * radius_ / kC;
  const double tMain = std::max(0.6, 1 / (0.25 + 0.83 * std::pow(ka11, 8)));
  for (int k = 0; k < n; ++k) {
    const int m = h.diameters(k), c = h.circles(k);
    double hz, sigRef;
    if (m == 0 && c == 1) {  // the kettle's air spring holds it: it hardly follows the tuning
      hz = 65 * 0.35 / radius_ + 0.46 * f11_;
      sigRef = 0.31;
    } else {
      const double r = ratioOf(m, c, ideal_[k]);
      hz = r * f11_;
      sigRef = 0.61 * (ideal_[k] / r) * (ideal_[k] / r);
    }
    const double sig = std::max(kSigma, kSigma + (sigRef - kSigma) * air);
    // Its decay (3.1, 12.3): ka the mode's wavenumber times the radius.
    const double ka = 2 * kPi * hz * radius_ / kC;
    const double t31 = 1 / (0.085 + 0.0045 * std::pow(ka, 6));
    double t;
    if (m == 0) {
      t = (c == 1 && f11_ < 80) ? 0.7 : 0.3;
    } else if (c == 1) {
      const double t3 = std::max(t31, 2.1 * tMain);
      if (m == 1) t = std::max(0.6, 1 / (0.25 + 0.83 * std::pow(ka, 8)));
      else if (m == 2) t = std::max(1.5 * tMain, 1 / (0.083 + 0.036 * std::pow(ka, 6)));
      else if (m == 3) t = t3;
      else t = std::clamp(std::min(t3, 4.5) * 0.9 * fastPow(0.85, m - 4), 0.8, 4.5);
    } else {
      t = m == 1 && c == 2 ? 1.2 : m == 2 && c == 2 ? 0.7 : 0.5;
    }
    if (hz > 600) t = std::min(t, std::max(0.15, 600 / hz));
    if (m == 1 && c == 1) t11_ = t * kDecay;
    h.setMode(k, hz, t * kDecay, sig / kSigma);
  }
  h.update();
  // The far field toward the hall (+x: the player stands at -x and strikes the near side), at
  // 1 m: a baffled head's p = rho / (2 pi) d/dt of its surface's speed, each patch's phase by its
  // place along the direction; the two channels 12 degrees either side, 10 degrees up.
  static const double kThump = knob("PJ_TIMP_THUMP", 1.0);
  const int kR = 12, kT = 32;
  std::vector<float> ph(n);
  std::vector<std::complex<double>> g[2] = {std::vector<std::complex<double>>(n), std::vector<std::complex<double>>(n)};
  const double el = 10 * kPi / 180, az[2] = {12 * kPi / 180, -12 * kPi / 180};
  for (int ir = 0; ir < kR; ++ir)
    for (int it = 0; it < kT; ++it) {
      const double r = radius_ * (ir + 0.5) / kR, th = 2 * kPi * (it + 0.5) / kT;
      const double dA = 2 * kPi * r * (radius_ / kR) / kT;
      h.shape(r, th, ph.data());
      const double x = r * std::cos(th), y = r * std::sin(th);
      for (int e = 0; e < 2; ++e) {
        const double along = std::cos(el) * (x * std::cos(az[e]) + y * std::sin(az[e]));
        for (int k = 0; k < n; ++k) {
          const double kk = 2 * kPi * h.frequency(k) / kC;
          g[e][k] += double(ph[k]) * dA * std::polar(1.0, kk * along);
        }
      }
    }
  for (int e = 0; e < 2; ++e) {
    wRe_[e].assign((n + 15) / 16 * 16, 0.0f);
    wIm_[e].assign((n + 15) / 16 * 16, 0.0f);
    for (int k = 0; k < n; ++k) {
      const double s = kRho / (2 * kPi) * (h.diameters(k) == 0 ? kThump : 1.0), w = 2 * kPi * h.frequency(k);
      // (the term g v: Re g times the speed, and Im g times the speed a quarter period on, w q)
      wRe_[e][k] = float(s * g[e][k].real());
      wIm_[e][k] = float(-s * g[e][k].imag() * w);
    }
  }
}

void Kettle::stroke(double speed, double r, double theta, double hard) {
  hard_ = hard;
  Membrane& h = *head_;
  h.shape(std::clamp(r, 0.0, 0.95) * radius_, theta, phi_.data());
  double g = 0;
  for (int k = 0; k < h.modes(); ++k) g += double(phi_[k]) * phi_[k];
  sG_ = dt_ * dt_ * g;
  zs_ = h.height(phi_.data()) + 0.05e-3;  // (thrown from just over the head where it is now)
  vs_ = -std::fabs(speed);
  // The felt's loss (Hunt-Crossley): ~15 % of the stroke's energy, a restitution of ~0.92.
  lambda_ = 0.12 / std::max(0.05, std::fabs(speed));
  lastX_ = 0;
  stickOn_ = true;
  sAge_ = 0;
  asleep_ = false;
  quiet_ = 0;
}

void Kettle::damp(bool on) {
  if (on) {
    // Three fingertips on the playing spot: the main tone down 60 dB in ~0.12 s, over 40 ms.
    handTo_ = std::max(1.0, t11_ / 0.12);
    handCount_ = 0;
  } else if (handTo_ != 1 || handX_ != 1) {
    handTo_ = handX_ = 1;
    head_->setDamping(1);
  }
}

float Kettle::sample(float& side) {
  side = 0;
  if (asleep_) return 0;
  Membrane& h = *head_;
  if (handX_ != handTo_ && ++handCount_ % 64 == 0) {
    const double step = fastPow(handTo_, 64.0 / (0.04 * fs_));
    handX_ = std::min(handTo_, handX_ * step);
    h.setDamping(handX_);
  }
  if (stickOn_) {
    static const double kK = knob("PJ_TIMP_K", 1.6e8), kA = knob("PJ_TIMP_ALPHA", 2.54), kM = knob("PJ_TIMP_MASS", 0.028);
    const double hh = h.height(phi_.data()), hv = h.speed(phi_.data());
    const double free = (hh - zs_) + dt_ * (hv - vs_), G = sG_ + dt_ * dt_ / kM;
    double f = 0, x = 0;
    if (free > 0) {
      auto force = [&](double xx) {
        const double rate = (xx - lastX_) / dt_;
        return kK * hard_ * fastPow(xx, kA) * std::max(0.0, 1 + lambda_ * rate);
      };
      double lo = 0, hi = free;
      for (int it = 0; it < 30; ++it) {
        const double mid = 0.5 * (lo + hi);
        if (mid + G * force(mid) > free) hi = mid; else lo = mid;
      }
      x = 0.5 * (lo + hi);
      f = force(x);
    }
    lastX_ = x;
    vs_ += dt_ * f / kM;
    zs_ += dt_ * vs_;
    if (f > 0) h.push(phi_.data(), float(-f));
    if (++sAge_ > int(0.03 * fs_) || (f == 0 && vs_ > 0)) stickOn_ = false;  // (the quick rebound: gone)
  }
  h.step();
  const int n = h.modes();
  const float *q = h.q(), *v = h.v();
  float y[2];
  for (int e = 0; e < 2; ++e) {
    const float *a = wRe_[e].data(), *b = wIm_[e].data();
    float s = 0;
    for (int k = 0; k < n; ++k) s += a[k] * v[k] + b[k] * q[k];
    y[e] = (s - prev_[e]) * float(fs_);
    prev_[e] = s;
  }
  const float mono = 0.5f * (y[0] + y[1]);
  side = 0.5f * (y[0] - y[1]) + float(pan_) * mono;
  if (!std::isfinite(mono)) {
    h.reset();
    asleep_ = true;
    side = 0;
    return 0;
  }
  if (std::fabs(mono) < 1e-7f && !stickOn_) {
    if (++quiet_ > int(0.3 * fs_)) {
      asleep_ = true;
      h.reset();
      damp(false);
    }
  } else {
    quiet_ = 0;
  }
  return mono;
}

TimpaniModel::TimpaniModel(const Instrument& inst) : Tube(inst) {
  for (int i = 0; i < 5; ++i) k_.emplace_back(new Kettle(inst.fs, i));
}

void TimpaniModel::resetAir() {
  Tube::resetAir();
  for (auto& k : k_) {
    k->head().reset();
    k->damp(false);
  }
}

bool TimpaniModel::asleep() const {
  for (const auto& k : k_)
    if (!k->asleep()) return false;
  return true;
}

const float* TimpaniModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

float TimpaniModel::sample() {
  static const float g = float(knob("PJ_TIMP_OUT", 1.0 / 16));
  float y = 0, s = 0;
  for (auto& k : k_) {
    float side;
    y += k->sample(side);
    s += side;
  }
  last_ = g * y;
  side_.push_back(g * float(outScale()) * s);
  return finish(last_);
}

TimpaniPlayer::TimpaniPlayer(TimpaniModel& m) : Performer(m), m_(m) {
  std::fill(drumOf_, drumOf_ + 128, -1);
  std::fill(dampAt_, dampAt_ + 8, -1.0);
  std::fill(lastStroke_, lastStroke_ + 8, -1.0);
  std::fill(rollKey_, rollKey_ + 8, -1);
}

double TimpaniPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

int TimpaniPlayer::drumFor(int midi) {
  for (int i = 0; i < 5; ++i)
    if (midi <= kSizes[i].hi) return i;
  return 4;
}

double TimpaniPlayer::malletSpeed(int velocity) {
  // research/timpani.md 12.5: pp 0.3-0.7 m/s .. fff 8-11, between the bands' ends in the log.
  static const double kV = knob("PJ_TIMP_VMAX", 1.0);
  static constexpr double kVel[] = {1, 24, 44, 62, 80, 100, 118, 127};
  static constexpr double kSpd[] = {0.3, 0.7, 1.3, 2.2, 3.5, 5.5, 8.0, 11.0};
  const double v = std::clamp(double(velocity), 1.0, 127.0);
  int i = 0;
  while (i < 6 && v > kVel[i + 1]) ++i;
  const double u = (v - kVel[i]) / (kVel[i + 1] - kVel[i]);
  return kV * std::exp(std::log(kSpd[i]) + u * (std::log(kSpd[i + 1]) - std::log(kSpd[i])));
}

// The two hands' differences x (timpani.h).
static const double kHands = knob("PJ_TIMP_HANDS", 1.0);

double TimpaniPlayer::rollRate(int d) const {
  const double level = std::clamp(dyn_ + breathCtl_, 0.0, 1.0);
  return 10 * fastPow(m_.kettle(d).pitch() / 110, 0.4) * (0.85 + 0.35 * level);
}

void TimpaniPlayer::strike(int d, double speed, double r, double theta, double hard) {
  Kettle& k = m_.kettle(d);
  k.damp(false);
  dampAt_[d] = -1;
  k.stroke(speed, r, theta, hard);
  lastStroke_[d] = now_;
}

void TimpaniPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  const int d = drumFor(midi);
  Kettle& k = m_.kettle(d);
  const double f = midiHz(midi);
  const bool same = std::fabs(k.pitch() / f - 1) < 1e-6;
  k.tune(f);
  if (pend_[d].due >= 0) {  // (a written roll's stroke still waiting: it goes now)
    strike(d, pend_[d].speed, pend_[d].r, pend_[d].th, pend_[d].hard);
    pend_[d].due = -1;
  }
  // The spot (12.5): r/a 0.80 at pp to 0.69 at fff, +-1 cm; the two hands a little apart.
  static const double kSpot = knob("PJ_TIMP_SPOT", 1.0);
  const double a = kSizes[d].radius, gap = now_ - lastStroke_[d];
  const bool rolled = same && lastStroke_[d] >= 0 && gap < 0.14;  // (a written-out roll)
  const double r = kSpot * (0.80 - 0.11 * std::clamp((velocity - 12) / 111.0, 0.0, 1.0)) + (rolled ? 0.03 : 0.02) * (ur() - 0.5) / a;
  // (the same key again: the other hand; another key: the leading hand)
  if (midi != lastKey_) hand_ = 0;
  lastKey_ = midi;
  // The hands: a little apart for single strokes; in a roll 12 cm apart round the ring.
  const double half = rolled ? 0.06 / (r * a) : 0.12;
  const double th = kPi + (hand_ ? half : -half) + (rolled ? 0.1 : 0.06) * (ur() - 0.5);
  // The two hands are not alike (timpani.h): the spot, the speed, the mallet. The same key again:
  // the other hand; another key: the leading hand.
  const double hs = hand_ ? 1 : -1;
  const double r2 = r + 0.02 * kHands * hs, hard = fastPow(1.3, -kHands * hs), handSpeed = 1 - 0.04 * kHands * hs;
  hand_ ^= 1;
  const double level = 0.85 + 0.3 * std::clamp(dyn_, 0.0, 1.0);
  double speed = malletSpeed(velocity) * (1 + 0.04 * (ur() - 0.5)) * level;
  if (rolled) {
    // Evened (the file's velocities followed slowly, +-6 % at random: ~+-0.5 dB), and each
    // stroke a few ms late (up to 8 % of the spacing, at most 8 ms).
    evenSpeed_[d] = evenSpeed_[d] > 0 ? 0.6 * evenSpeed_[d] + 0.4 * speed : speed;
    speed = evenSpeed_[d] * (1 + 0.12 * (ur() - 0.5));
    pend_[d] = {now_ + std::min(0.008, 0.08 * gap) * ur(), speed * handSpeed, r2, th, hard};
  } else {
    evenSpeed_[d] = speed;
    strike(d, speed * handSpeed, r2, th, hard);
  }
  lastStroke_[d] = now_;
  onAt_[midi] = now_;
  drumOf_[midi] = d;
  // A held key's roll: with the mod wheel up at once; or, with PJ_TIMP_ROLL, after 0.6 s held.
  static const bool kHeldRoll = knob("PJ_TIMP_ROLL", 0) > 0;
  if (flutter_ > 0.05 || kHeldRoll) {
    rollKey_[d] = midi;
    rollVel_[d] = velocity;
    rollNext_[d] = now_ + (flutter_ > 0.05 ? 1 / rollRate(d) : 0.6);
  } else {
    rollKey_[d] = -1;
  }
}

void TimpaniPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  const int d = drumOf_[midi];
  if (d < 0) return;
  if (rollKey_[d] == midi) rollKey_[d] = -1;
  if (hold_) return;
  // A note held long enough for the hand to get there is damped, unless the drum is struck
  // again first; a short one rings on (fast passages are not damped stroke by stroke).
  if (now_ - onAt_[midi] >= 0.22 && lastStroke_[d] <= onAt_[midi] + 1e-9) dampAt_[d] = now_ + 0.15;
}

void TimpaniPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    for (int d = 0; d < m_.kettles(); ++d) {
      if (pend_[d].due >= 0 && now_ >= pend_[d].due) {
        strike(d, pend_[d].speed, pend_[d].r, pend_[d].th, pend_[d].hard);
        pend_[d].due = -1;
      }
      if (rollKey_[d] >= 0 && now_ >= rollNext_[d]) {
        // The roll's next stroke: the hands alternating 12 cm apart, a little softer than the
        // first, at the level now (the expression; the mod wheel's depth when it rolls).
        const double a = kSizes[d].radius;
        const double r = 0.76 + 0.03 * (ur() - 0.5) / a, half = 0.06 / (r * a);
        const double th = kPi + (hand_ ? half : -half) + 0.1 * (ur() - 0.5);
        const double hs = hand_ ? 1 : -1;
        hand_ ^= 1;
        const double level = 0.85 + 0.3 * std::clamp(dyn_ + breathCtl_, 0.0, 1.0);
        const double wheel = flutter_ > 0.05 ? 0.4 + 0.75 * flutter_ : 1.0;
        strike(d, 0.85 * malletSpeed(rollVel_[d]) * level * wheel * (1 + 0.12 * (ur() - 0.5)) * (1 - 0.04 * kHands * hs), r + 0.02 * kHands * hs, th, fastPow(1.3, -kHands * hs));
        rollNext_[d] = now_ + (1 + 0.1 * (ur() - 0.5)) / rollRate(d);
      }
      if (dampAt_[d] >= 0 && now_ >= dampAt_[d]) {
        if (lastStroke_[d] < dampAt_[d] - 0.15) m_.kettle(d).damp(true);
        dampAt_[d] = -1;
      }
    }
    tick(m / fs_);
    m_.process(out + i, m);
    now_ += m / fs_;
  }
  bool busy = !m_.asleep();
  for (int d = 0; d < m_.kettles(); ++d) busy = busy || rollKey_[d] >= 0 || pend_[d].due >= 0;
  blowing_ = busy;
}

}  // namespace pajupilli
