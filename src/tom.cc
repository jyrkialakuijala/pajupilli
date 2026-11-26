#include "tom.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>
#include <map>
#include <mutex>
#include <string>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2, kC = 343.0;

// The drums: size (in), shell depth (in), note (Hz). Jazz tuning, high and
// open: the 12" near E3, the 14" floor tom near A2 (research/toms.md 2.1).
struct Size { const char* name; double inch, deep, note; };
constexpr Size kSizes[] = {
    {"tom-10", 10, 8, 196.0},  {"tom-12", 12, 8, 164.8},  {"tom-13", 13, 9, 146.8},
    {"tom-14", 14, 14, 110.0}, {"tom-16", 16, 16, 92.5}, {"tom-16-low", 16, 16, 73.4},
};

// Modes up to the frequency where a head has about `budget` of them (a
// membrane has ~ (pi R f / c)^2 below f).
double topFor(const HeadSpec& h, int budget) {
  const double c = std::sqrt(h.tension / h.sigma);
  return std::clamp(std::sqrt(double(budget)) * c / (kPi * h.radius), 600.0, 4000.0);
}

// The drum's note as the model has it: the pair struck at the middle, the
// heads' motion together (both the same way: the in-phase member) listened
// to for half a second, its strongest frequency near the target.
double pairNote(double fs, const TomSpec& s) {
  Membrane b(fs, s.batter), r(fs, s.reso);
  DrumAir air = s.air;
  std::vector<float> phi(b.modes());
  b.shape(0, 0, phi.data());
  b.push(phi.data(), -1.0f);
  const int n = int(0.5 * fs);
  std::vector<float> y(n);
  const double dt = 1 / fs;
  for (int i = 0; i < n; ++i) {
    air.couple(b, r, dt);
    b.step();
    r.step();
    double sum = 0;
    const float *qb = b.q(), *vb = b.volume(), *qr = r.q(), *vr = r.volume();
    for (int k = 0; k < b.modes(); ++k) sum += double(vb[k]) * qb[k];
    for (int k = 0; k < r.modes(); ++k) sum += double(vr[k]) * qr[k];
    y[i] = float(sum);
  }
  auto power = [&](double f) {  // Goertzel
    const double w = 2 * kPi * f / fs, c = 2 * std::cos(w);
    double s0 = 0, s1 = 0, s2 = 0;
    for (int i = 0; i < n; ++i) {
      const double win = 0.5 - 0.5 * std::cos(2 * kPi * i / (n - 1));
      s0 = y[i] * win + c * s1 - s2;
      s2 = s1;
      s1 = s0;
    }
    return s1 * s1 + s2 * s2 - c * s1 * s2;
  };
  double best = s.note, bp = -1;
  for (double f = 0.55 * s.note; f < 1.45 * s.note; f += 0.5) {
    const double p = power(f);
    if (p > bp) { bp = p; best = f; }
  }
  const double a = power(best - 0.5), c = power(best + 0.5), d = a - 2 * bp + c;
  return d < 0 ? best + 0.25 * (a - c) / d : best;
}

TomSpec tuned(double fs, TomSpec s) {
  // Both heads' tension scaled together until the note is the drum's (two
  // passes; remembered for the next drum of the same kind).
  static std::mutex mu;
  static std::map<std::string, double> memo;
  char key[160];
  std::snprintf(key, sizeof key, "%.0f %.6f %.6f %.3f %.6f %.3f %.1f %.4f", fs, s.batter.radius, s.batter.sigma,
                s.batter.tension, s.reso.sigma, s.reso.tension, s.note, s.depth);
  double k = 1;
  {
    std::lock_guard<std::mutex> lk(mu);
    auto it = memo.find(key);
    if (it != memo.end()) k = it->second;
  }
  if (k == 1) {
    for (int pass = 0; pass < 2; ++pass) {
      TomSpec t = s;
      t.batter.tension *= k;
      t.reso.tension *= k;
      t.batter.top = t.reso.top = 600;  // (the low modes are what matter here: quick)
      const double f = pairNote(fs, t);
      k *= (s.note / f) * (s.note / f);
    }
    std::lock_guard<std::mutex> lk(mu);
    memo[key] = k;
  }
  s.batter.tension *= k;
  s.reso.tension *= k;
  s.batter.top = topFor(s.batter, int(knob("PJ_TOM_MODES", 260)));
  s.reso.top = topFor(s.reso, int(knob("PJ_TOM_MODES2", 120)));
  return s;
}
}  // namespace

bool jazzTom(const Instrument& inst) { return std::strncmp(inst.name, "tom-", 4) == 0; }

TomSpec tomSpec(const char* name) {
  TomSpec s;
  Size z = kSizes[3];
  for (const Size& x : kSizes)
    if (std::strcmp(x.name, name) == 0) z = x;
  const double R = z.inch * 0.0254 / 2;
  s.note = z.note * knob("PJ_TOM_TUNE", 1.0);
  s.depth = z.deep * 0.0254;
  s.batter.radius = s.reso.radius = R;
  s.batter.sigma = knob("PJ_TOM_SIGMA", 0.26);   // coated single ply, 10 mil
  s.reso.sigma = knob("PJ_TOM_SIGMA2", 0.18);    // clear, 7 mil
  // Both heads at the same lug pitch, 1.75 x the note (research/toms.md 2.2):
  // the lug pitch is a head's own (1,1). (The tension is then solved for.)
  const double lug = 1.75 * s.note;
  auto tension = [&](double sigma, double ratio) { const double c = 2 * kPi * R * lug * ratio / 3.832; return sigma * c * c; };
  s.batter.tension = tension(s.batter.sigma, 1.0);
  s.reso.tension = tension(s.reso.sigma, knob("PJ_TOM_RESO", 1.0));  // the resonant head's lug, against the batter's
  s.batter.zeta01 = s.reso.zeta01 = knob("PJ_TOM_ZETA01", 0.010);
  s.batter.zeta0 = s.reso.zeta0 = knob("PJ_TOM_ZETA0", 0.010);
  s.batter.zeta = knob("PJ_TOM_ZETA", 0.010);
  s.reso.zeta = knob("PJ_TOM_ZETA2", 0.008);
  s.batter.lossKnee = s.reso.lossKnee = 2000;
  s.batter.stretch = knob("PJ_TOM_STRETCH", 1.1e6);
  s.reso.stretch = knob("PJ_TOM_STRETCH2", 0.8e6);
  s.batter.seed = 21 + uint32_t(z.inch);
  s.reso.seed = 41 + uint32_t(z.inch);
  s.air.volume = kPi * R * R * s.depth;
  s.air.vent = knob("PJ_TOM_VENT", 6);
  s.stickMass = knob("PJ_TOM_STICKMASS", 0.015);
  s.stickK = knob("PJ_TOM_STICKK", 2e8);
  s.micZ = knob("PJ_TOM_MICZ", 0.4);
  s.under = knob("PJ_TOM_UNDER", 0.5);
  s.click = knob("PJ_TOM_CLICK", 1);
  s.tss = knob("PJ_TOM_TSS", 1);
  s.gain = knob("PJ_TOM_GAIN", 1);
  // The brush: as the snare's (research/toms.md 4.5), held firmly (the
  // pull-out stroke).
  s.brush.tips = int(knob("PJ_BRUSH_TIPS", 24));
  s.brush.fan = knob("PJ_BRUSH_FAN", 400);
  s.brush.fanLoss = knob("PJ_BRUSH_FANLOSS", 2.0);
  s.brush.handle = knob("PJ_BRUSH_HANDLE", 0.020);
  s.brush.grip = knob("PJ_TOM_GRIP", 2000);
  s.brush.tipMass = knob("PJ_BRUSH_TIPMASS", 5e-5);
  s.brush.contact = knob("PJ_BRUSH_CONTACT", 2e4);
  s.brush.grain = knob("PJ_BRUSH_GRAIN", 0.18e-3);
  s.brush.rough = knob("PJ_BRUSH_ROUGH", 8e-6);
  s.brush.bite = knob("PJ_BRUSH_BITE", 0.15);
  s.brush.drag = knob("PJ_BRUSH_DRAG", 0.5);
  return s;
}

Tom::Tom(double fs, const TomSpec& spec)
    : s_(tuned(fs, spec)), fs_(fs), dt_(1 / fs), bat_(fs, s_.batter), res_(fs, s_.reso), air_(s_.air) {
  brush_.reset(new Brush(fs, bat_, s_.brush, uint32_t(s_.note)));
  phi_.assign(bat_.modes(), 0.0f);
  // The microphones over the batter: each mode of a head by its shape,
  // summed over the head with each patch's distance and its delay at the
  // mode's own pitch; the resonant head round the shell, weaker and the other
  // way round.
  const int kR = 10, kT = 24;
  for (int head = 0; head < 2; ++head) {
    Membrane& h = head ? res_ : bat_;
    std::vector<float> ph(h.modes());
    for (int mic = 0; mic < 2; ++mic) {
      std::vector<std::complex<double>> g(h.modes());
      const double mx = (mic ? 1 : -1) * s_.micX, mz = s_.micZ + (head ? s_.depth : 0.0);
      const double d0 = std::hypot(mx, mz);
      for (int ir = 0; ir < kR; ++ir)
        for (int it = 0; it < kT; ++it) {
          const double r = h.radius() * (ir + 0.5) / kR, th = 2 * kPi * (it + 0.5) / kT;
          const double dA = 2 * kPi * r * (h.radius() / kR) / kT;
          h.shape(r, th, ph.data());
          const double x = r * std::cos(th) - mx, y = r * std::sin(th);
          const double d = std::sqrt(x * x + y * y + mz * mz);
          for (int k = 0; k < h.modes(); ++k) {
            const double kk = 2 * kPi * h.frequency(k) / kC;
            g[k] += double(ph[k]) * dA * std::exp(std::complex<double>(0, -kk * (d - d0))) / d;
          }
        }
      std::vector<float>& out = head ? micR_[mic] : micB_[mic];
      out.assign((h.modes() + 15) / 16 * 16, 0.0f);
      for (int k = 0; k < h.modes(); ++k)
        out[k] = float((g[k].real() < 0 ? -1 : 1) * std::abs(g[k]) * kRho / (2 * kPi) * (head ? -s_.under : 1.0));
    }
  }
}

void Tom::stick(double speed, double r, double theta) {
  bat_.shape(std::min(r, 0.95) * bat_.radius(), theta, phi_.data());
  double g = 0;
  for (int k = 0; k < bat_.modes(); ++k) g += double(phi_[k]) * phi_[k];
  sG_ = dt_ * dt_ * g;  // (unit-mass modes: one sample's give per unit force)
  // Thrown from just over the head where it is now: the stroke meets its swing.
  zs_ = bat_.height(phi_.data()) + 0.05e-3;
  vs_ = -std::fabs(speed);
  stickOn_ = true;
  sAge_ = 0;
  asleep_ = false;
  quiet_ = 0;
}

void Tom::brush(double speed, double r, double theta) {
  const double R = bat_.radius() * std::min(r, 0.85);
  uint32_t h = uint32_t(speed * 1e4) * 2654435761u + uint32_t(theta * 1e4);
  const double ang = theta + 1.2 + 0.3 * ((h >> 8) * (1.0 / 16777216.0) - 0.5);
  brush_->jump(R * std::cos(theta), R * std::sin(theta), ang);
  // The pull-out: the fan tilted (its ends land over 1.5 .. 3 mm of height),
  // a hair further after the first touch, no dwell: the hand is already on
  // its way up.
  brush_->strike(speed, (1.5 + 1.5 * ((h >> 16 & 255) / 255.0)) * 1e-3, 0.3e-3, 0.0);
  dT_ = -1;
  asleep_ = false;
  quiet_ = 0;
}

void Tom::drag(double load, double r, double theta, double dist, double dur) {
  const double R = bat_.radius() * std::min(r, 0.8);
  dX_ = R * std::cos(theta);
  dY_ = R * std::sin(theta);
  const double dir = theta + 1.4;
  dDx_ = dist * std::cos(dir);
  dDy_ = dist * std::sin(dir);
  dAng_ = theta + 1.2;
  dDur_ = std::max(dur, 0.01);
  dT_ = 0;
  brush_->jump(dX_, dY_, dAng_);
  brush_->rest(load);
  asleep_ = false;
  quiet_ = 0;
}

void Tom::setOutside(float pBatter, float pReso) {
  pOutB_ = pBatter;
  pOutR_ = pReso;
  if (asleep_ && std::max(std::fabs(pBatter), std::fabs(pReso)) > 2e-3f) { asleep_ = false; quiet_ = 0; }
}

void Tom::reset() {
  bat_.reset();
  res_.reset();
  air_.leak = 0;
  stickOn_ = false;
  zs_ = 1; vs_ = 0;
  sAge_ = 0;
  dT_ = -1;
  brush_->rest(0);
  prev_[0] = prev_[1] = 0;
  hpC_ = lastC_ = 0;
  asleep_ = true;
}

float Tom::sample() {
  if (asleep_) { side_ = 0; return 0; }
  // The drag: the brush drawn along its path, then lifted.
  if (dT_ >= 0) {
    dT_ += dt_;
    const double u = std::min(1.0, dT_ / dDur_), d = u * (2 - u);  // (slowing)
    brush_->place(dX_ + dDx_ * d, dY_ + dDy_ * d, dAng_);
    if (dT_ >= dDur_) { brush_->rest(0); dT_ = -1; }
  }
  brush_->step();
  // The stick: its tip against the batter where it is, the force found for
  // the end of the sample as the head gives under it (as the hi-hat's).
  float F = 0;
  if (stickOn_) {
    const double h = bat_.height(phi_.data()), hv = bat_.speed(phi_.data());
    const double free = (h - zs_) + dt_ * (hv - vs_), G = sG_ + dt_ * dt_ / s_.stickMass;
    double f = 0;
    if (free > 0) {
      double lo = 0, hi = free;  // the dent left once the force has pushed both apart
      for (int it = 0; it < 30; ++it) {
        const double x = 0.5 * (lo + hi);
        if (x + G * s_.stickK * x * std::sqrt(x) > free) hi = x; else lo = x;
      }
      const double x = 0.5 * (lo + hi);
      f = s_.stickK * x * std::sqrt(x);
    }
    vs_ += dt_ * f / s_.stickMass;
    zs_ += dt_ * vs_;
    if (f > 0) bat_.push(phi_.data(), float(-f));
    F = float(f);
    // Off the head and on its way up (or after 30 ms in any case): the
    // stroke is over; the hand takes the stick away.
    if (++sAge_ > int(0.03 * fs_) || (f == 0 && vs_ > 0)) stickOn_ = false;
  }
  lastF_ = F;
  // The room presses on both heads from outside (as on the kick).
  if (pOutB_ != 0 || pOutR_ != 0) {
    float *fb = bat_.force(), *fr = res_.force();
    const float *vb = bat_.volume(), *vr = res_.volume();
    for (int k = 0; k < bat_.modes(); ++k) fb[k] -= pOutB_ * vb[k];
    for (int k = 0; k < res_.modes(); ++k) fr[k] += pOutR_ * vr[k];
  }
  air_.couple(bat_, res_, dt_);
  bat_.step();
  res_.step();
  float y[2];
  for (int mic = 0; mic < 2; ++mic) {
    float s = 0;
    const float *vb = bat_.v(), *mb = micB_[mic].data();
    for (int k = 0; k < bat_.modes(); ++k) s += mb[k] * vb[k];
    const float *vr = res_.v(), *mr = micR_[mic].data();
    for (int k = 0; k < res_.modes(); ++k) s += mr[k] * vr[k];
    y[mic] = (s - prev_[mic]) * float(fs_);
    prev_[mic] = s;
  }
  // What the modes do not carry: the stick's click and the wire ends'
  // ticks, above the batter's top (a force on a light film moves the air at
  // once: rho F / (2 pi sigma r)).
  const float Fc = float(s_.click) * F + float(s_.tss) * brush_->force();
  const float aC = float(std::exp(-2 * kPi * s_.batter.top * dt_));
  hpC_ = aC * (hpC_ + Fc - lastC_);
  lastC_ = Fc;
  const float rMic = float(std::hypot(s_.micX, s_.micZ));
  const float click = -float(kRho / (2 * kPi * s_.batter.sigma * rMic)) * hpC_;
  const float g = float(s_.gain);
  const float l = g * (y[0] + click), r = g * (y[1] + click);
  side_ = 0.5f * (l - r);
  const float mono = 0.5f * (l + r);
  if (!std::isfinite(mono)) { reset(); return 0; }
  if (std::fabs(mono) < 1e-6f && !stickOn_ && brush_->up() && dT_ < 0) {
    if (++quiet_ > int(0.3 * fs_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return mono;
}

TomModel::TomModel(const Instrument& inst) : Tube(inst), tom_(inst.fs, tomSpec(inst.name)) {}

void TomModel::resetAir() {
  Tube::resetAir();
  tom_.reset();
}

const float* TomModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

float TomModel::sample() {
  static const float g = float(knob("PJ_TOM_OUT", 1.0 / 8));
  // The room's sound at the drum (as the kick's: the far head a shell's depth
  // later; 0.3 of it, PJ_TOM_ROOM: the room model is not passive at the low end).
  static const float kRoom = float(knob("PJ_TOM_ROOM", 0.3));
  if (roomPos_ < roomN_) {
    pRoom_ = room_[roomPos_++];
    if (delay_.empty()) delay_.assign(std::max(1, int(std::lround(tom_.spec().depth / kC * fs()))), 0.0f);
    const float far = delay_[delayPos_];
    delay_[delayPos_] = pRoom_;
    delayPos_ = (delayPos_ + 1) % int(delay_.size());
    tom_.setOutside(kRoom * pRoom_, kRoom * far);
  }
  last_ = g * tom_.sample();
  side_.push_back(g * float(outScale()) * tom_.side());
  return finish(last_);
}

TomPlayer::TomPlayer(TomModel& m) : Performer(m), m_(m) {}

double TomPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

double TomPlayer::stickSpeed(int velocity, double dynamics) {
  // ~ velocity^1.5: General MIDI's ~30 dB from 16 to 127 (the drum is about
  // linear in the stick's speed; a harder stick is a little brighter).
  static const double top = knob("PJ_TOM_VMAX", 4.0), ex = knob("PJ_TOM_VEXP", 1.5);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return top * fastPow(v, ex) * (0.75 + 0.5 * std::clamp(dynamics, 0.0, 1.0));
}

double TomPlayer::brushSpeed(int velocity, double dynamics) {
  static const double ex = knob("PJ_TOM_BEXP", 1.5);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return (0.12 + 2.6 * fastPow(v, ex)) * (0.7 + 0.6 * std::clamp(dynamics, 0.0, 1.0));
}

void TomPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  Tom& t = m_.tom();
  // Between the middle and halfway out, each stroke a little elsewhere.
  const double r = 0.33 + 0.08 * (ur() - 0.5), th = -0.6 + 0.5 * (ur() - 0.5);
  static const int dragBelow = int(knob("PJ_TOM_DRAGVEL", 24));
  if (midi >= 60) {  // brushed
    if (velocity < dragBelow) {
      const double s = std::clamp(velocity, 1, 127) / double(dragBelow);
      t.drag(0.15 + 0.5 * s, r, th, (0.03 - 0.015 * s) * (0.8 + 0.4 * ur()), (0.15 - 0.10 * s) * (0.8 + 0.4 * ur()));
    } else {
      t.brush(brushSpeed(velocity, dyn_) * (1 + 0.06 * (ur() - 0.5)), r, th);
    }
  } else {
    t.stick(stickSpeed(velocity, dyn_) * (1 + 0.04 * (ur() - 0.5)), r, th);
  }
}

void TomPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.tom().asleep();
}

}  // namespace pajupilli
