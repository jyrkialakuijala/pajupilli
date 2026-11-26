#include "shaker.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "instrument.h"
#include "knob.h"
#include "songclock.h"
#include "vmath.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;

Shaker::Type typeOf(const Instrument& inst) {
  if (std::strcmp(inst.name, "shaker") == 0) return Shaker::Type::kShaker;
  if (std::strcmp(inst.name, "cabasa") == 0) return Shaker::Type::kCabasa;
  if (std::strcmp(inst.name, "tambourine") == 0) return Shaker::Type::kTambourine;
  if (std::strcmp(inst.name, "jingle-bell") == 0) return Shaker::Type::kJingleBell;
  return Shaker::Type::kMaracas;
}
}  // namespace

bool shakerInstrument(const Instrument& inst) {
  for (const char* n : {"maracas", "shaker", "cabasa", "tambourine", "jingle-bell"})
    if (std::strcmp(inst.name, n) == 0) return true;
  return false;
}

float Shaker::ur() {
  rng_ ^= rng_ << 13;
  rng_ ^= rng_ >> 17;
  rng_ ^= rng_ << 5;
  return (rng_ >> 8) * (1.0f / 16777216.0f);
}

float Shaker::un() { return (ur() + ur() + ur() + ur() - 2.0f) * 1.7320508f; }

// A resonance by its frequency and its ring time (T60, s).
void Shaker::addMode(std::vector<Mode>& v, double hz, double t60, double gain) {
  Mode m;
  const double r = std::exp(-6.9078 / (t60 * fs_)), w = 2 * kPi * hz / fs_;
  m.pr = float(r * std::cos(w));
  m.pi = float(r * std::sin(w));
  m.g = float(gain);
  v.push_back(m);
}

// An impulse into modes [from, from + n), each by a random share (where on
// the body the hit lands).
void Shaker::excite(std::vector<Mode>& v, int from, int n, float J) {
  for (int k = from; k < from + n; ++k) v[k].re += J * (0.5f + 0.5f * ur());
}

// A jingle disc (a bell) struck at a random place on its rim: each of its
// modes (two, three, four nodal diameters: its first three) driven by its
// shape there, |cos m phi|: every collision a different mix of the same ring.
void Shaker::exciteRim(std::vector<Mode>& v, int from, int n, float J) {
  const float phi = float(2 * M_PI) * ur();
  for (int i = 0; i < n; ++i) {
    const int m = 2 + i % 3;
    v[from + i].re += J * 1.571f * std::fabs(std::cos(m * phi)) * (0.8f + 0.4f * ur());
  }
}

float Shaker::ring(std::vector<Mode>& v, int from, int n) {
  float y = 0;
  for (int k = from; k < from + n; ++k) {
    Mode& m = v[k];
    const float re = m.re * m.pr - m.im * m.pi, im = m.re * m.pi + m.im * m.pr;
    m.re = re;
    m.im = im;
    y += m.g * im;
  }
  return y;
}

Shaker::Shaker(double fs, Type type, uint32_t seed) : type_(type), fs_(fs), dt_(1 / fs) {
  this->seed(seed);
  contactA_ = float(std::exp(-dt_ / 80e-6));  // a seed on a gourd (~80 us)
  auto shell = [&](int n, double mass, double e, double L, double pan) {
    Shell s;
    s.L = float(L);
    s.pan = float(pan);
    s.p.resize(n);
    for (Particle& p : s.p) {
      p.m = float(mass * (0.7 + 0.6 * ur()));
      p.e = float(std::clamp(e + 0.2 * (ur() - 0.5), 0.1, 0.9));
      p.x = float(L * ur());
    }
    shells_.push_back(s);
    return &shells_.back();
  };
  switch (type) {
    case Type::kMaracas: {
      // Two maracas, one a hand, a little apart: a gourd's broad resonances
      // (STK: 3.2 kHz, Q ~6), a low body.
      for (int h = 0; h < 2; ++h) {
        Shell* s = shell(60, 0.15e-3, 0.4, h ? 0.024 : 0.026, h ? 0.3 : -0.3);  // (the free path: the pile fills half the shell)
        const double k = h ? 1.09 : 1.0;
        addMode(s->modes, 3100 * k, 0.0045, 1.0);
        addMode(s->modes, 5200 * k, 0.0025, 0.6);
        addMode(s->modes, 950 * k, 0.006, 0.25);
      }
      gain_ = float(knob("PJ_SHAKER_GAIN", 1.0) * 1.0e4);
      break;
    }
    case Type::kShaker: {
      // A tube of grains: a longer flight, lighter and more particles, a
      // higher tube.
      Shell* s = shell(80, 0.05e-3, 0.35, 0.07, 0);
      addMode(s->modes, 4400, 0.0035, 1.0);
      addMode(s->modes, 7200, 0.0020, 0.7);
      addMode(s->modes, 1800, 0.004, 0.2);
      gain_ = float(knob("PJ_SHAKER_GAIN", 1.0) * 2.0e4);
      break;
    }
    case Type::kCabasa: {
      // The corrugated steel cylinder: broad metal resonances (STK: one very
      // broad 3 kHz pole); and the chains' swing at the twist's end.
      addMode(metal_, 3000, 0.0012, 1.0);
      addMode(metal_, 6500, 0.0010, 0.8);
      addMode(metal_, 9500, 0.0008, 0.6);
      Shell* s = shell(14, 0.5e-3, 0.5, 0.012, 0);
      addMode(s->modes, 3200, 0.0015, 1.0);
      addMode(s->modes, 7000, 0.0010, 0.7);
      // Struck against the palm: the wooden handle and the cylinder's body,
      // the palm damping them at once (tens of ms).
      addMode(thump_, 190, 0.030, 1.0);
      addMode(thump_, 430, 0.020, 0.6);
      addMode(thump_, 950, 0.012, 0.35);
      gain_ = float(knob("PJ_SHAKER_GAIN", 1.0) * 120);
      break;
    }
    case Type::kTambourine: {
      // The frame's knock (wood, the hand on it), and six pairs of jingles,
      // each disc its own three modes a few % off its neighbours'.
      addMode(frame_, 250, 0.05, 1.0);
      addMode(frame_, 520, 0.035, 0.7);
      addMode(frame_, 1100, 0.02, 0.5);
      for (int j = 0; j < 6; ++j) {
        Rattler r;
        r.masses = 2;
        r.gap = float(2e-3 * (0.8 + 0.4 * ur()));
        r.m = 6e-3f;
        r.e = 0.6f;
        r.mode0 = int(rattleModes_.size());
        for (int d = 0; d < 2; ++d) {
          const double k = 1 + 0.12 * (ur() - 0.5);
          addMode(rattleModes_, 3200 * k, 0.30 * (0.7 + 0.6 * ur()), 0.6);
          addMode(rattleModes_, 5600 * k * (1 + 0.04 * (ur() - 0.5)), 0.22 * (0.7 + 0.6 * ur()), 1.0);
          addMode(rattleModes_, 8100 * k * (1 + 0.04 * (ur() - 0.5)), 0.15 * (0.7 + 0.6 * ur()), 0.9);
        }
        r.nmode = 6;
        r.z[0] = -r.gap;
        r.z[1] = -r.gap;
        rat_.push_back(r);
      }
      gain_ = float(knob("PJ_SHAKER_GAIN", 1.0) * 20);
      break;
    }
    case Type::kJingleBell: {
      // Sleigh bells: a ball in each, each bell its few modes (STK: 2.5, 5.3,
      // 6.5, 8.3, 9.8 kHz).
      for (int j = 0; j < 10; ++j) {
        Rattler r;
        r.masses = 1;
        r.gap = float(3e-3 * (0.8 + 0.4 * ur()));
        r.m = 2e-3f;
        r.e = 0.5f;
        r.mode0 = int(rattleModes_.size());
        const double k = 1 + 0.10 * (ur() - 0.5);
        addMode(rattleModes_, 2500 * k, 0.40 * (0.7 + 0.6 * ur()), 0.8);
        addMode(rattleModes_, 5300 * k, 0.30 * (0.7 + 0.6 * ur()), 1.0);
        addMode(rattleModes_, 8300 * k, 0.20 * (0.7 + 0.6 * ur()), 0.6);
        r.nmode = 3;
        r.z[0] = -r.gap;
        rat_.push_back(r);
      }
      gain_ = float(knob("PJ_SHAKER_GAIN", 1.0) * 85);
      break;
    }
  }
}

void Shaker::reset() {
  for (Shell& s : shells_) {
    for (Particle& p : s.p) { p.u = 0; p.moving = false; }
    for (Mode& m : s.modes) m.re = m.im = 0;
    for (int k = 0; k < 4; ++k) s.in[k] = s.lp[k] = 0;
    s.tick = s.tickLp = 0;
    s.moving = 0;
  }
  for (Rattler& r : rat_) {
    r.u[0] = r.u[1] = 0;
    r.z[0] = r.z[1] = -r.gap;
    r.moving = false;
  }
  for (auto* v : {&rattleModes_, &frame_, &metal_})
    for (Mode& m : *v) m.re = m.im = 0;
  twistOn_ = releasing_ = twistHeld_ = false;
  twistS_ = 0;
  side_ = prevY_ = prevS_ = 0;
  scrapeLp_ = scrapeHp_ = scrapeX_ = 0;
  asleep_ = true;
  quiet_ = 0;
}

// The hand stops: the particles fly on towards the front wall (dir +1: the
// far end, -1: the near one), the flight already under way (a MIDI note is
// the chick, not the start of the motion: research/shakers.md 3.1). A soft
// stroke throws fewer of them, spread wider. What still moves keeps a part of
// its motion.
void Shaker::throwShell(Shell& s, double speed, double dir, double spreadK, double frac) {
  const double f = frac >= 0 ? frac : std::clamp(0.2 + 0.8 * speed / 1.2, 0.2, 1.0);
  const double spreadT = spreadK * (0.004 + 0.012 * (1 - std::min(speed / 2.5, 1.0)));
  for (Particle& p : s.p) {
    if (ur() > f) continue;
    const double u = dir * speed * (0.85 + 0.3 * ur()) + 0.3 * p.u;
    // (Arrivals trail off: most land at once, the pile's last ones later.)
    const double d = std::min(double(s.L), std::fabs(u) * spreadT * 0.5 * -std::log(1e-3 + ur()));
    p.x = float(dir > 0 ? s.L - d : d);
    p.u = float(u);
    if (!p.moving) { p.moving = true; ++s.moving; }
  }
}

// A hit on the shell's wall: its main mode (the first listed) the same
// way wherever it lands, the others by their shapes there (any sign).
void Shaker::hit(Shell& s, float J) {
  const int nm = std::min(4, int(s.modes.size()));
  s.in[0] += J * (0.6f + 0.4f * ur());
  for (int k = 1; k < nm; ++k) s.in[k] += J * 0.8f * un();
}

float Shaker::stepShell(Shell& s) {
  if (s.moving > 0) {
    const int n = int(s.p.size());
    // (The seeds roll and rub on the pile: they settle in ~0.15 s.)
    const float drag = float(1 - dt_ / 0.07);
    for (Particle& p : s.p) {
      if (!p.moving) continue;
      p.u *= drag;
      p.x += p.u * float(dt_);
      if (p.x > s.L || p.x < 0) {
        const float J = p.m * (1 + p.e) * std::fabs(p.u);
        hit(s, p.x > s.L ? J : -J);
        p.x = p.x > s.L ? 2 * s.L - p.x : -p.x;
        p.u = -p.u * p.e * (0.85f + 0.3f * ur());
        p.x = std::clamp(p.x, 0.0f, s.L);
      }
      if (std::fabs(p.u) < 0.03f) { p.u = 0; p.moving = false; --s.moving; }
    }
    // Particles meet each other: by chance, as many meetings as there are
    // particles moving (two of them exchange their motion, with a loss, and
    // click).
    if (s.moving > 1 && ur() < float(8000 * dt_ * s.moving / n)) {
      Particle& a = s.p[size_t(ur() * n) % n];
      Particle& b = s.p[size_t(ur() * n) % n];
      if (&a != &b && a.moving && b.moving) {
        const float um = 0.5f * (a.u + b.u), e = 0.5f;
        const float ua = um - e * (a.u - um), ub = um - e * (b.u - um);
        hit(s, 0.6f * a.m * (a.u - b.u));
        a.u = ua;
        b.u = ub;
      }
    }
  }
  // The contact rounds each impulse.
  float y = 0;
  const int nm = std::min(4, int(s.modes.size()));
  for (int k = 0; k < nm; ++k) {
    Mode& m = s.modes[k];
    s.lp[k] = contactA_ * s.lp[k] + (1 - contactA_) * s.in[k];
    s.in[k] = 0;
    const float re = m.re * m.pr - m.im * m.pi + s.lp[k], im = m.re * m.pi + m.im * m.pr;
    m.re = re;
    m.im = im;
    y += m.g * im;
  }
  return y;
}

// The frame stops (the hand slaps it): the jingles (balls) fly on.
void Shaker::hitRattlers(double speed) {
  // A soft stroke leaves some at rest (they need more than gravity's pull to
  // leave their stops).
  // Every stroke finds them otherwise arranged: the frame held at its own
  // tilt (gravity along the pins), rocking on the blow (the pairs on one side
  // thrown harder than the other's), the discs at rest leaning on their pins
  // anywhere in their slots, some still trembling.
  const double f = std::clamp(0.25 + 0.75 * speed / 1.2, 0.25, 1.0);
  gEff_ = float(9.81 * (0.6 + 0.4 * ur()));
  const double rock = 0.6 * ur(), psi = 2 * M_PI * ur();
  const int n = int(rat_.size());
  for (int j = 0; j < n; ++j) {
    Rattler& r = rat_[j];
    if (!r.moving)
      for (int i = 0; i < r.masses; ++i) {
        r.z[i] = float(-r.gap + 0.4 * r.gap * ur());
        r.u[i] = float(0.05 * (ur() - 0.5));
      }
    if (ur() > f) continue;
    // The kick reaches the pairs over a few ms (the frame flexing under the
    // hand), each tilted on its pin its own way: its bounce and its free
    // travel differ from stroke to stroke.
    const double k = 1 + rock * std::cos(2 * M_PI * j / n - psi);
    r.kick = float(speed * k * (0.75 + 0.5 * ur()));
    r.delay = int(ur() * 0.008 / dt_);
    r.eS = float(r.e * (0.65 + 0.6 * ur()));
    r.gapS = float(r.gap * (0.6 + 0.4 * ur()));
    r.moving = true;
  }
  if (!frame_.empty()) excite(frame_, 0, int(frame_.size()), float(0.02 * speed));
}

float Shaker::stepRattlers() {
  const float dt = float(dt_);
  for (Rattler& r : rat_) {
    if (!r.moving) continue;
    if (r.delay > 0 && --r.delay == 0) {
      for (int i = 0; i < r.masses; ++i) r.u[i] += r.kick * float(0.9 + 0.2 * ur());
      r.kick = 0;
    } else if (r.kick != 0 && r.delay == 0) {  // (no delay: at once)
      for (int i = 0; i < r.masses; ++i) r.u[i] += r.kick * float(0.9 + 0.2 * ur());
      r.kick = 0;
    }
    const int nm = r.masses;
    const float gap = r.gapS, e0 = r.eS;
    for (int i = 0; i < nm; ++i) {
      r.u[i] -= gEff_ * dt;          // gravity draws them back to rest (along the pins: the tilt)
      r.u[i] *= 1 - 2.0f * dt;       // the pin's friction, the air
      r.z[i] += r.u[i] * dt;
    }
    const int top = nm - 1;
    // The stops: the top mass against the slot's top, the bottom one its floor.
    if (r.z[top] > gap && r.u[top] > 0) {
      const float J = r.m * (1 + e0) * r.u[top];
      exciteRim(rattleModes_, r.mode0 + 3 * top, 3, J);
      r.u[top] = -e0 * r.u[top] * (0.7f + 0.6f * ur());
      r.z[top] = gap;
    }
    if (r.z[0] < -gap && r.u[0] < 0) {
      const float J = -r.m * (1 + e0) * r.u[0];
      if (J > 1e-6f) exciteRim(rattleModes_, r.mode0, 3, J);
      r.u[0] = -e0 * r.u[0] * (0.6f + 0.5f * ur());
      r.z[0] = -gap;
    }
    // The two discs against each other: equal masses, with a loss.
    if (nm == 2 && r.z[0] > r.z[1] && r.u[0] > r.u[1]) {
      const float um = 0.5f * (r.u[0] + r.u[1]), dv = r.u[0] - r.u[1];
      const float J = r.m * 0.5f * (1 + e0) * dv;
      exciteRim(rattleModes_, r.mode0, 3, J);      // each disc where it was met
      exciteRim(rattleModes_, r.mode0 + 3, 3, J);
      const float e = e0 * (0.7f + 0.6f * ur());  // tilted on the pin: how squarely they meet
      r.u[0] = um - e * (r.u[0] - um);
      r.u[1] = um - e * (r.u[1] - um);
      r.z[0] = r.z[1] = 0.5f * (r.z[0] + r.z[1]);
    }
    bool rest = true;
    for (int i = 0; i < nm; ++i) rest &= std::fabs(r.u[i]) < 0.02f && r.z[i] < -gap + 1e-4f && r.kick == 0;
    if (rest) {
      r.moving = false;
      for (int i = 0; i < nm; ++i) { r.u[i] = 0; r.z[i] = -r.gap; }
    }
  }
  float y = ring(rattleModes_, 0, int(rattleModes_.size()));
  if (!frame_.empty()) y += ring(frame_, 0, int(frame_.size()));
  return y;
}

// The cabasa's twist: the bead chains over the cylinder's ridges (2 mm
// apart), 80 beads touching. The clicks come at speed x 80 / 2 mm a second,
// each as hard as the speed: a dense scrape (its mean and, now and then, a
// bead jumping a ridge harder), into the cylinder's broad resonances and
// straight out (the beads' own clicks).
float Shaker::stepScraper() {
  if (twistOn_) {
    twistT_ += dt_;
    const double half = 0.5 * twistDur_;
    if (releasing_) {
      const double a = std::clamp((twistT_ - releaseT_) / 0.04, 0.0, 1.0);
      twistS_ = releaseFrom_ * 0.5 * (1 + std::cos(kPi * a));
      if (a >= 1) { twistOn_ = false; twistS_ = 0; }
    } else if (twistT_ < half) {
      twistS_ = twistPeak_ * std::sin(0.5 * kPi * twistT_ / half);
    } else if (twistHeld_) {
      // Held: the twist eases to a slower turn and keeps going.
      const double a = std::min(1.0, (twistT_ - half) / 0.1);
      twistS_ = twistPeak_ * (1 - 0.55 * a);
    } else {
      const double a = std::clamp((twistT_ - half) / half, 0.0, 1.0);
      twistS_ = twistPeak_ * std::cos(0.5 * kPi * a);
      if (a >= 1) { twistOn_ = false; twistS_ = 0; }
    }
    if (!twistOn_ && !shells_.empty()) throwShell(shells_[0], 0.5 * twistPeak_, ur() < 0.5f ? 1 : -1);
  }
  float in = 0;
  if (twistS_ > 1e-4) {
    const float n = float(twistS_ * 80 / 2e-3 * dt_);  // clicks this sample
    in = float(twistS_) * std::sqrt(n) * un() * 2e-3f;
    if (ur() < 0.03f * n) in += float(twistS_) * 6e-3f * (ur() < 0.5f ? 1 : -1);
  }
  // Into the cylinder, and the clicks' own sound above 4 kHz.
  float y = 0;
  for (Mode& m : metal_) {
    const float re = m.re * m.pr - m.im * m.pi + in, im = m.re * m.pi + m.im * m.pr;
    m.re = re;
    m.im = im;
    y += m.g * im;
  }
  const float a = 0.59f;  // 4 kHz one-pole high-pass
  scrapeHp_ = a * (scrapeHp_ + in - scrapeX_);
  scrapeX_ = in;
  return y + 0.1f * scrapeHp_;
}

void Shaker::stroke(double speed, int hand) {
  asleep_ = false;
  quiet_ = 0;
  switch (type_) {
    case Type::kMaracas:
      throwShell(shells_[hand & 1], speed, 1);
      break;
    case Type::kShaker:
      throwShell(shells_[0], speed, hand & 1 ? -1 : 1);
      break;
    case Type::kCabasa:
      twistOn_ = true;
      twistHeld_ = true;
      releasing_ = false;
      twistT_ = 0;
      twistPeak_ = 0.45 * speed;  // the cylinder's surface speed (m/s)
      twistDur_ = 0.15 - 0.035 * std::min(speed, 2.5);  // a harder twist is quicker
      break;
    case Type::kTambourine:
    case Type::kJingleBell:
      hitRattlers(speed);
      break;
  }
}

void Shaker::cabasa(double speed, Cabasa how, int dir) {
  asleep_ = false;
  quiet_ = 0;
  Shell& chains = shells_[0];
  switch (how) {
    case Cabasa::kTurn:
      // On the same way: still turning, the twist picks up from its speed now
      // (no fresh start, no reversal: the chains keep leaning and sliding).
      twistPeak_ = std::max(0.45 * speed, twistS_);
      twistT_ = twistOn_ ? 0.25 * twistDur_ : 0;
      twistDur_ = 0.15 - 0.035 * std::min(speed, 2.5);
      twistOn_ = twistHeld_ = true;
      releasing_ = false;
      break;
    case Cabasa::kTwist:
      // Back and forth: the reversal throws the chains over the cylinder
      // (their "chk"), then they scrape the other way.
      if (dir != twistDir_ || !twistOn_) throwShell(chains, 0.6 * speed, dir);
      twistOn_ = twistHeld_ = true;
      releasing_ = false;
      twistT_ = 0;
      twistPeak_ = 0.45 * speed;
      twistDur_ = 0.15 - 0.035 * std::min(speed, 2.5);
      break;
    case Cabasa::kShake:
      // Shaken in the air: the chains thrown along the cylinder, a short
      // scrape as they settle; no turn.
      throwShell(chains, 1.1 * speed, dir);
      twistOn_ = true;
      twistHeld_ = false;
      releasing_ = false;
      twistT_ = 0;
      twistPeak_ = knob("PJ_CABASA_SHAKEK", 0.7) * speed;
      twistDur_ = 0.06;
      break;
    case Cabasa::kSlap:
      // Against the other hand: the palm's thump on the handle and cylinder,
      // every chain thrown (the swish), the beads sliding on the steel.
      excite(thump_, 0, int(thump_.size()), float(knob("PJ_CABASA_THUMP", 0.075) * speed));
      throwShell(chains, 1.6 * speed, dir);
      twistOn_ = true;
      twistHeld_ = false;
      releasing_ = false;
      twistT_ = 0;
      twistPeak_ = knob("PJ_CABASA_SLAPK", 0.6) * speed;
      twistDur_ = 0.08;
      break;
  }
  twistDir_ = dir;
}

void Shaker::maraca(int h, double speed, Maraca how, double lead, double gap) {
  asleep_ = false;
  quiet_ = 0;
  Gest& g = gest_[h & 1];
  const bool rolling = g.rolling;
  g = Gest();
  g.rolling = rolling && how == Maraca::kSlap;
  g.on = true;
  g.how = how;
  g.lead = lead;
  g.gap = gap;
  g.speed = speed;
  g.dir = 1;
  if (lead <= 0) stepGesture(h & 1);  // at once
}

void Shaker::maracaRoll(int h, double speed) {
  asleep_ = false;
  quiet_ = 0;
  Gest& g = gest_[h & 1];
  g.rolling = true;
  g.rollSpeed = speed;
}

void Shaker::maracaStop(int h) { gest_[h & 1].rolling = false; }

// One sample of a maraca's gesture: the woosh before a chest stroke, the
// slap when it is due, a double's second slap, the loops' rolling.
void Shaker::stepGesture(int h) {
  Gest& g = gest_[h];
  if (h >= int(shells_.size())) return;
  Shell& s = shells_[h];
  const int n = int(s.p.size());
  auto nudge = [&](double u, int dir) {  // a seed set sliding
    Particle& p = s.p[size_t(ur() * n) % n];
    p.u = float(dir * u * (0.5 + ur()));
    if (!p.moving) { p.moving = true; ++s.moving; }
  };
  if (g.on) {
    if (!g.thrown && g.t < g.lead) {
      if (g.how == Maraca::kChest) {
        // Moving off from the chest: the seeds begin to slide along the
        // gourd, more and faster as the hand speeds up (a soft rising woosh).
        const double a = g.t / g.lead;
        if (ur() < float(2500 * dt_ * a * a)) nudge(0.08 + 0.35 * a * g.speed, g.dir);
      }
    } else if (!g.thrown) {
      g.thrown = true;
      switch (g.how) {
        case Maraca::kFlick: {  // (tighter than a slap, as hard; PJ_MARACA_FLICK: how tight)
          static const double kTight = knob("PJ_MARACA_FLICK", 0.45), kV = knob("PJ_MARACA_FLICKV", 0.85);
          throwShell(s, kV * g.speed, g.dir, kTight, 1.0);
          break;
        }
        case Maraca::kDouble: throwShell(s, g.speed, g.dir); break;
        default: throwShell(s, g.speed, g.dir); break;
      }
    } else if (g.how == Maraca::kDouble && !g.second && g.t >= g.lead + g.gap) {
      g.second = true;  // the hand comes back: the seeds slap the other wall
      throwShell(s, 0.55 * g.speed, -g.dir);
    }
    g.t += dt_;
    if (g.thrown && (g.how != Maraca::kDouble || g.second)) g.on = false;
  }
  if (g.rolling) {
    // Turned in loops (~4.5 a second): the seeds roll round the wall, most
    // of them going one way as the gourd turns, now one wall, now the other.
    static const double kLoop = 4.5;
    rollT_[h] += dt_;
    const double ph = std::sin(2 * M_PI * kLoop * rollT_[h]);
    static const double kRoll = knob("PJ_MARACA_ROLL", 0.35);
    if (ur() < float(1200 * dt_ * (1 + 0.6 * ph))) nudge(kRoll * (0.3 + 0.5 * g.rollSpeed), ph > 0 ? 1 : -1);
  }
}

void Shaker::release() {
  if (type_ != Type::kCabasa || !twistOn_) return;
  twistHeld_ = false;
  // Past its first half the twist eases off now; before, it finishes as
  // a short twist would.
  if (twistT_ >= 0.5 * twistDur_ && !releasing_) {
    releasing_ = true;
    releaseT_ = twistT_;
    releaseFrom_ = twistS_;
  }
}

float Shaker::sample() {
  if (asleep_) { side_ = 0; return 0; }
  float y = 0, s = 0;
  if (type_ == Type::kMaracas)
    for (int h = 0; h < 2; ++h)
      if (gest_[h].on || gest_[h].rolling) stepGesture(h);
  for (Shell& sh : shells_) {
    const float v = stepShell(sh);
    y += v;
    s += sh.pan * v;
  }
  if (!rat_.empty()) y += stepRattlers();
  if (type_ == Type::kCabasa) y += stepScraper() + (thump_.empty() ? 0.0f : ring(thump_, 0, int(thump_.size())));
  // A small closed body radiates no steady push: its sound rises with
  // frequency (a zero at DC; the hits all push one way during a chick, and a
  // broad resonance would pass their mean).
  const float dy = y - prevY_, ds = s - prevS_;
  prevY_ = y;
  prevS_ = s;
  y = dy * gain_;
  side_ = ds * gain_;
  if (!std::isfinite(y)) { reset(); return 0; }
  bool moving = twistOn_ || gest_[0].on || gest_[1].on || gest_[0].rolling || gest_[1].rolling;
  for (const Shell& sh : shells_) moving |= sh.moving > 0;
  for (const Rattler& r : rat_) moving |= r.moving;
  if (!moving && std::fabs(y) < 1e-7f) {
    if (++quiet_ > int(0.5 * fs_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return y;
}

ShakerModel::ShakerModel(const Instrument& inst)
    : Tube(inst), sh_(inst.fs, typeOf(inst), uint32_t(std::strlen(inst.name) * 7919u + 13u)) {}

void ShakerModel::resetAir() {
  Tube::resetAir();
  sh_.reset();
}

const float* ShakerModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

float ShakerModel::sample() {
  last_ = sh_.sample();
  side_.push_back(float(outScale()) * sh_.side());
  return finish(last_);
}

ShakerPlayer::ShakerPlayer(ShakerModel& m) : Performer(m), m_(m) {}

double ShakerPlayer::handSpeed(int velocity, double dynamics, double ex) {
  static const double top = knob("PJ_SHAKER_VMAX", 2.5);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return top * fastPow(v, ex) * (0.75 + 0.5 * std::clamp(dynamics, 0.0, 1.0));
}

void ShakerPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  // The speed's law by instrument, for General MIDI's ~30 dB from 16 to 127:
  // a maraca's hits grow with the speed and with the seeds thrown; the
  // shaker's lighter grains, the jingles and bells need a steeper hand.
  const Shaker::Type t = m_.shaker().type();
  const double ex = t == Shaker::Type::kMaracas ? 1.4 : t == Shaker::Type::kShaker ? 1.35 : t == Shaker::Type::kCabasa ? 1.2 : 1.4;
  double v = handSpeed(velocity, dyn_, ex);
  // The shaker's pull is the weaker half of its stroke.
  if (m_.shaker().type() == Shaker::Type::kShaker && (hand_ & 1)) v *= 0.7;
  if (t == Shaker::Type::kMaracas) {
    // The maracas' ways, by velocity (PJ_MARACA_CHEST/SLAP/DOUBLE: where each
    // ends): the softest from the chest (a small woosh, then the slap), then
    // the plain slap, then two slaps in one movement (a sixteenth apart), the
    // hardest a flick. In a song whose player sends these notes early
    // (noteLead()), every slap lands that much after its note, on the beat,
    // and the chest's woosh fills the time before it; live there is no time
    // before a note: the chest stroke is a soft slap.
    static const double kChest = knob("PJ_MARACA_CHEST", 50), kSlap = knob("PJ_MARACA_SLAP", 99),
                        kDouble = knob("PJ_MARACA_DOUBLE", 110);
    const SongClock& clk = SongClock::get();
    static const double kLiveLead = knob("PJ_MARACA_LIVELEAD", 0);  // (to hear the woosh live: the slaps that late)
    const double lead = clk.valid && noteLeadsApplied() ? noteLead(m_.instrument()) : kLiveLead;
    const Shaker::Maraca how = velocity <= kChest ? Shaker::Maraca::kChest
                               : velocity <= kSlap ? Shaker::Maraca::kSlap
                               : velocity <= kDouble ? Shaker::Maraca::kDouble
                                                     : Shaker::Maraca::kFlick;
    const double gap = clk.valid ? 0.25 * clk.beatLen : 0.09;
    m_.shaker().maraca(hand_, v, how, lead, gap);
    marHand_ = hand_;
    marHeld_ = 0;
    marSpeed_ = v;
    marRolling_ = false;
    hand_ ^= 1;
    return;
  }
  if (t == Shaker::Type::kCabasa) {
    // The cabasa's ways, by velocity (PJ_CABASA_TURN/TWIST/SHAKE: where each
    // ends): the softest turned on one way, then twisted back and forth, then
    // shaken in the air (a 1-2-3-4 in the bar: 1 the strongest, 3 a little
    // under it, 2 under 3, 4 the weakest; in a song from its beat, live by
    // counting the notes), the hardest struck against the other hand.
    static const double kTurn = knob("PJ_CABASA_TURN", 45), kTwist = knob("PJ_CABASA_TWIST", 80),
                        kShake = knob("PJ_CABASA_SHAKE", 105);
    Shaker::Cabasa how = velocity <= kTurn ? Shaker::Cabasa::kTurn
                         : velocity <= kTwist ? Shaker::Cabasa::kTwist
                         : velocity <= kShake ? Shaker::Cabasa::kShake
                                              : Shaker::Cabasa::kSlap;
    if (how != Shaker::Cabasa::kTurn) cabDir_ = -cabDir_;
    if (how == Shaker::Cabasa::kShake) {
      const SongClock& clk = SongClock::get();
      const int beat = clk.valid ? int(std::floor(clk.beat + 0.25)) & 3 : cabCount_ & 3;
      static const double kAccent[4] = {1.0, 0.8, 0.9, 0.7};
      v *= kAccent[beat];
    }
    ++cabCount_;
    m_.shaker().cabasa(v, how, cabDir_);
    hand_ ^= 1;
    return;
  }
  m_.shaker().stroke(v, hand_);
  hand_ ^= 1;
}

void ShakerPlayer::noteOff(int) {
  m_.shaker().release();
  if (marHand_ >= 0) {
    if (marRolling_) m_.shaker().maracaStop(marHand_);
    marHand_ = -1;
    marRolling_ = false;
  }
}

void ShakerPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    // A maraca's key held on past a note's length (0.4 s; in a song 3/4 of a
    // beat): the maraca turned in loops until the key is let go.
    if (marHand_ >= 0 && !marRolling_) {
      marHeld_ += m / fs_;
      const SongClock& clk = SongClock::get();
      if (marHeld_ > (clk.valid ? 0.75 * clk.beatLen : 0.4)) {
        marRolling_ = true;
        m_.shaker().maracaRoll(marHand_, marSpeed_);
      }
    }
    m_.process(out + i, m);
  }
  blowing_ = !m_.shaker().asleep();
}

}  // namespace pajupilli
