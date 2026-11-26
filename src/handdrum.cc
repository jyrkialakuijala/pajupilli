#include "handdrum.h"

#include "songclock.h"

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

// Tension for the drum's note on its (0,1) mode as the air loads it (the
// mode that radiates, struck anywhere: research/hand_drums.md 6). The air's
// share of the mass does not depend on the tension, so f goes as sqrt(T): one
// trial head gives it.
// A pipe under the head (pipeL > 0) moves the note: its (0,1) q and the
// pipe's modes p_n, q'' + w0^2 q = vol p, p_n'' + w_n^2 p_n = D vol q'', give
// at the note w: w0^2 = w^2 (1 + kappa sum 1 / (w_n^2 - w^2)), kappa = -D vol^2.
// The head alone is tuned to that w0.
double tensionFor(HeadSpec h, double f01, double pipeL = 0, double footA = 0) {
  const double c = 2 * kPi * h.radius * f01 / 2.4048;
  h.tension = h.sigma * c * c;
  h.top = 1.2 * f01;
  h.stretch = 0;
  const Membrane trial(48000, h);
  double want = f01;
  if (pipeL > 0) {
    const double L = pipeL + 0.6 * footA, A = kPi * h.radius * h.radius;
    const double kappa = 2 * kRho * kC * kC / (A * L) * double(trial.volume()[0]) * trial.volume()[0];
    const double w = 2 * kPi * f01;
    double sum = 0;
    for (int n = 0; n < 3; ++n) {
      const double wn = (2 * n + 1) * kPi * kC / (2 * L);
      sum += 1 / (wn * wn - w * w);
    }
    const double r = 1 + kappa * sum;
    if (r > 0.25) want = f01 * std::sqrt(r);
  }
  const double f = trial.frequency(0);
  return h.tension * (want / f) * (want / f);
}

// 0 conga, 1 bongo, 2 timbale; high: the drum of the pair (research/hand_drums.md 2, 3).
HandDrumSpec handSpec(int kind, bool high) {
  HandDrumSpec s;
  static const double tune = std::exp2(knob("PJ_HD_TUNE", 0) / 12);  // semitones, all drums
  if (kind == 0) {  // congas: rawhide over a 76 cm barrel
    s.head.radius = high ? 0.149 : 0.159;
    s.head.sigma = knob("PJ_CONGA_SIGMA", 1.6);
    const double f = (high ? knob("PJ_CONGA_HI", 262) : knob("PJ_CONGA_LO", 196)) * tune;
    s.head.top = knob("PJ_CONGA_TOP", 2200);
    s.head.zeta01 = 0.03; s.head.zeta0 = 0.02; s.head.zeta = knob("PJ_CONGA_ZETA", 0.005);
    s.head.lossKnee = knob("PJ_CONGA_KNEE", 1500);
    s.head.split = 0.012;
    s.head.stretch = knob("PJ_CONGA_STRETCH", 4.5e6);
    s.head.seed = high ? 21 : 22;
    s.pipeL = knob("PJ_CONGA_PIPE", 0.76);
    s.footA = 0.115;
    s.head.tension = tensionFor(s.head, f, s.pipeL, s.footA);
    s.pipeQ = 10;
    s.mass = 0.03; s.K = 1e6; s.alpha = 1.5; s.loss = 2.0;
    s.where = 0.87; s.band = 0.035; s.returnHz = 12; s.vmax = 6;
    s.micZ = 0.4; s.micX = 0.15;
  } else if (kind == 1) {  // bongos: thin rawhide, short open shells
    s.head.radius = high ? 0.089 : 0.108;
    s.head.sigma = knob("PJ_BONGO_SIGMA", 1.0);
    const double f = (high ? knob("PJ_BONGO_HI", 523) : knob("PJ_BONGO_LO", 392)) * tune;
    s.head.tension = tensionFor(s.head, f);
    s.head.top = knob("PJ_BONGO_TOP", 3500);
    s.head.zeta01 = 0.03; s.head.zeta0 = 0.02; s.head.zeta = knob("PJ_BONGO_ZETA", 0.006);
    s.head.lossKnee = knob("PJ_BONGO_KNEE", 2000);
    s.head.split = 0.012;
    s.head.stretch = knob("PJ_BONGO_STRETCH", 2.8e6);
    s.head.seed = high ? 23 : 24;
    s.pipeL = knob("PJ_BONGO_PIPE", 0);  // (15 cm deep, as wide: no pipe, as the timbale)
    s.footA = 0.045;
    s.pipeQ = 8;
    s.mass = 0.015; s.K = 1.5e6; s.alpha = 1.5; s.loss = 2.0;
    s.where = 0.85; s.band = 0.022; s.returnHz = 15; s.vmax = 5;
    s.micZ = 0.35; s.micX = 0.12;
  } else {  // timbales: thin plastic heads, steel shells, sticks
    s.head.radius = high ? 0.178 : 0.190;
    s.head.sigma = 0.22;
    const double f = (high ? knob("PJ_TIMBAL_HI", 330) : knob("PJ_TIMBAL_LO", 247)) * tune;
    s.head.tension = tensionFor(s.head, f);
    s.head.top = knob("PJ_TIMBAL_TOP", 3500);
    s.head.zeta01 = 0.02; s.head.zeta0 = 0.01; s.head.zeta = knob("PJ_TIMBAL_ZETA", 0.006);
    s.head.lossKnee = 3000;
    s.head.split = 0.004;
    s.head.stretch = knob("PJ_TIMBAL_STRETCH", 0.9e6);
    s.head.seed = high ? 25 : 26;
    // (A 16 cm shell 36 cm across is no pipe: its air is mostly a mass on the
    // head, which the head's own air loading stands for. With the pipe model
    // its "first mode" at 315 Hz pulled the 330 Hz note to 224 Hz.)
    s.pipeL = knob("PJ_TIMBAL_PIPE", 0);
    s.footA = high ? 0.178 : 0.19;
    s.pipeQ = 6;
    s.mass = 0.02; s.K = knob("PJ_TIMBAL_K", 3e8); s.alpha = 1.5; s.loss = 0.3;
    s.where = 0.75; s.band = 0; s.returnHz = 25; s.vmax = 8;
    // The steel shell's ring (research/hand_drums.md 3.4: to be tuned by ear).
    const double sc = high ? 1.0 : 0.93;
    const double F[4] = {1870, 2630, 3480, 4920}, T[4] = {1.4, 1.1, 0.8, 0.6};
    s.shellN = 4;
    for (int i = 0; i < 4; ++i) { s.shellF[i] = F[i] * sc; s.shellT60[i] = T[i]; }
    s.shellMass = 0.45; s.shellRad = 0.01;
    s.rimShare = knob("PJ_TIMBAL_RIM", 0.15);
    s.micZ = 0.5; s.micX = 0.2;
  }
  return s;
}

int kindOf(const Instrument& inst) {
  if (std::strcmp(inst.name, "bongo") == 0) return 1;
  if (std::strcmp(inst.name, "timbale") == 0) return 2;
  return 0;
}
}  // namespace

bool handDrum(const Instrument& inst) {
  return std::strcmp(inst.name, "conga") == 0 || std::strcmp(inst.name, "bongo") == 0 ||
         std::strcmp(inst.name, "timbale") == 0;
}

HandDrum::HandDrum(double fs, const HandDrumSpec& spec) : s_(spec), fs_(fs), dt_(1 / fs), head_(fs, spec.head) {
  phi_.assign(head_.modes(), 0.0f);
  place(s_.where, -kPi / 2);
  // The pipe: quarter waves of the shell closed by the head, open at the foot.
  if (s_.pipeL > 0) {
    np_ = std::min(4, s_.pipeModes);
    const double L = s_.pipeL + 0.6 * s_.footA;
    for (int n = 0; n < np_; ++n) {
      pw_[n] = (2 * n + 1) * kPi * kC / (2 * L);
      pg_[n] = pw_[n] / s_.pipeQ;
      pmean_[n] = 2.0 * (n % 2 ? -1 : 1) / ((2 * n + 1) * kPi);
    }
    const double A = kPi * s_.head.radius * s_.head.radius;
    drive_ = -2 * kRho * kC * kC / (A * L);
    // The foot as a point source, rho / (4 pi d) dU/dt, a little later than the head.
    const double d0 = std::hypot(s_.micX, s_.micZ), df = std::hypot(s_.micX, s_.micZ + s_.pipeL);
    footK_ = kRho / (4 * kPi * df);
    footDelay_.assign(std::max(1, int(std::lround((df - d0) / kC * fs_))), 0.0f);
  }
  for (int i = 0; i < s_.shellN; ++i) {
    sw_[i] = 2 * kPi * s_.shellF[i];
    sg_[i] = 6.91 / s_.shellT60[i];
  }
  shellK_ = kRho * s_.shellRad / (4 * kPi * std::hypot(s_.micX, s_.micZ));
  // The microphone over the head: each mode by its shape, summed over the
  // head with each patch's distance and its delay at the mode's own pitch
  // (as kick.cc); the head radiates on its open side.
  const int kR = 10, kT = 24;
  std::vector<std::complex<double>> g(head_.modes());
  std::vector<float> ph(head_.modes());
  const double d0 = std::hypot(s_.micX, s_.micZ);
  for (int ir = 0; ir < kR; ++ir)
    for (int it = 0; it < kT; ++it) {
      const double r = head_.radius() * (ir + 0.5) / kR, th = 2 * kPi * (it + 0.5) / kT;
      const double dA = 2 * kPi * r * (head_.radius() / kR) / kT;
      head_.shape(r, th, ph.data());
      const double x = r * std::cos(th) - s_.micX, y = r * std::sin(th);
      const double d = std::sqrt(x * x + y * y + s_.micZ * s_.micZ);
      for (int k = 0; k < head_.modes(); ++k) {
        const double kk = 2 * kPi * head_.frequency(k) / kC;
        g[k] += double(ph[k]) * dA * std::exp(std::complex<double>(0, -kk * (d - d0))) / d;
      }
    }
  mic_.assign((head_.modes() + 15) / 16 * 16, 0.0f);
  const double k0 = kRho / (4 * kPi);  // (unbaffled: the shell's rim is a small baffle)
  for (int k = 0; k < head_.modes(); ++k) mic_[k] = float((g[k].real() < 0 ? -1 : 1) * std::abs(g[k]) * k0);
  // The contact's own sound above the head's modes: the force on the film
  // moves the air at once (rho F / (2 pi sigma d)).
  clickK_ = s_.click * 0.5 * kRho / (2 * kPi * s_.head.sigma * d0);
}

void HandDrum::shapeAt(double r, double theta, double band, std::vector<float>& phi, double& g) {
  // The fingers across the rim (five points along the arc), or the stick's tip.
  phi.assign(head_.modes(), 0.0f);
  std::vector<float> ph(head_.modes());
  const double R = head_.radius(), rc = std::clamp(r, 0.0, 0.97) * R;
  const int n = band > 0 ? 5 : 1;
  for (int i = 0; i < n; ++i) {
    const double off = n > 1 ? band * (i - 2) / 2.0 : 0.0;
    head_.shape(rc, theta + off / std::max(rc, 1e-3), ph.data());
    for (int k = 0; k < head_.modes(); ++k) phi[k] += ph[k] / n;
  }
  double q = 0;
  for (int k = 0; k < head_.modes(); ++k) q += double(phi[k]) * phi[k];
  g = dt_ * dt_ * q;
}

void HandDrum::place(double r, double theta, double band) {
  shapeAt(r, theta, band < 0 ? s_.band : band, phi_, gPhi_);
}

void HandDrum::strike(double speed, double r, double theta, bool mute) {
  Gesture g;
  if (mute) { g.muteForce = 1; g.muteTime = 1; }
  strike(speed, r, theta, g);
}

void HandDrum::strike(double speed, double r, double theta, const Gesture& gs) {
  place(r, theta, s_.band * gs.band);
  mass_ = s_.mass * gs.mass;
  K_ = s_.K * gs.K;
  lift_ = s_.returnHz * gs.lift;
  clickG_ = gs.click;
  // Fingers left on the head after the blow: a closed tone's press (where the
  // hand struck), or a light touch of the fingertips at the edge.
  mute_ = gs.muteForce > 0;
  muteForce_ = s_.muteForce * gs.muteForce;
  muteLeft_ = s_.muteTime * gs.muteTime;
  muteDamp_ = s_.muteDamp * gs.muteDamp;
  if (gs.muteR >= 0) shapeAt(gs.muteR, theta, s_.band * 0.6, phiMute_, gMute_);
  else { phiMute_ = phi_; gMute_ = gPhi_; }
  // Thrown from just over the head where it is now: the stroke meets its swing.
  zb_ = head_.height(phi_.data()) + 2e-4;
  vb_ = -std::fabs(speed);
  active_ = true;
  hit_ = false;
  asleep_ = false;
  quiet_ = 0;
}

void HandDrum::reset() {
  head_.reset();
  active_ = false;
  zb_ = 1; vb_ = 0;
  for (int n = 0; n < 4; ++n) pp_[n] = pv_[n] = sx_[n] = sv_[n] = 0;
  pbar1_ = 0;
  prev_ = hpC_ = lastC_ = 0;
  footPrev_ = shellPrev_ = 0;
  std::fill(footDelay_.begin(), footDelay_.end(), 0.0f);
  asleep_ = true;
}

float HandDrum::sample() {
  if (asleep_) return 0;
  // The hand (or stick) against the head: heights upward, the striker above.
  float F = 0;
  if (active_) {
    const double h = head_.height(phi_.data()), vh = head_.speed(phi_.data());
    // Implicit against the head's own give (as hihat.cc's stick): the dent x
    // with x + G F(x) = what the next sample would bring without the force,
    // G the head's and the striker's give per unit force in one sample.
    const double free = (h - zb_) + dt_ * (vh - vb_), G = gPhi_ + dt_ * dt_ / mass_;
    if (free > 0) {
      const double lossF = std::max(0.0, 1 + s_.loss * (vh - vb_));
      double lo = 0, hi = free;
      for (int it = 0; it < 30; ++it) {
        const double x = 0.5 * (lo + hi);
        if (x + G * K_ * fastPow(x, s_.alpha) * lossF > free) hi = x; else lo = x;
      }
      F = float(K_ * fastPow(0.5 * (lo + hi), s_.alpha) * lossF);
      if (!hit_) hitAt_ = 0;
      hit_ = true;
    }
    double Fb = F;
    if (mute_) {
      Fb -= muteForce_;  // the fingers pressed on
      if ((muteLeft_ -= dt_) <= 0) mute_ = false;
    } else if (hit_) {
      // The arm takes it back up, towards 5 cm over the head.
      const double w = 2 * kPi * lift_;
      Fb -= mass_ * w * w * (zb_ - (h + 0.05)) + 2 * mass_ * w * vb_;
    }
    vb_ += Fb / mass_ * dt_;
    zb_ += vb_ * dt_;
    if (F > 0) head_.push(phi_.data(), -F);
    // Fingers left on the head: once the blow is over (2 ms), they damp what
    // moves under them (the flesh, a damper to the arm), the force implicit
    // too: the head's speed there cannot be pushed past zero.
    if (mute_ && hit_ && (hitAt_ += dt_) > 2e-3) {
      const double vm = head_.speed(phiMute_.data());
      const double c = muteDamp_, fd = -c * vm / (1 + c * gMute_ * fs_);
      head_.push(phiMute_.data(), float(fd));
    }
    if (!mute_ && hit_ && h - zb_ < -3e-3 && vb_ > 0) active_ = false;
  }
  // The rim carries a share of a stroke near the edge into the steel shell.
  double shellOut = 0;
  if (s_.shellN > 0) {
    const double edge = std::clamp((s_.where - 0.6) / 0.4, 0.0, 1.0);
    const double Fr = F * s_.rimShare * edge;
    double a = 0;
    for (int i = 0; i < s_.shellN; ++i) {
      const double acc = -sw_[i] * sw_[i] * sx_[i] - 2 * sg_[i] * sv_[i] + Fr / s_.shellMass;
      sv_[i] += acc * dt_;
      sx_[i] += sv_[i] * dt_;
      a += sv_[i];
    }
    shellOut = shellK_ * (a - shellPrev_) * fs_;
    shellPrev_ = a;
  }
  // The pipe's pressure at the head pushes it (as the air in a shell does).
  double U = 0;
  if (np_ > 0) {
    double p = 0;
    for (int n = 0; n < np_; ++n) p += pp_[n];
    float* f = head_.force();
    const float* vol = head_.volume();
    for (int k = 0, m = head_.modes(); k < m; ++k) f[k] += float(p) * vol[k];
  }
  head_.step();
  // The pipe's modes, driven by the volume the head sweeps per second (U,
  // exact from the modes' speeds): p'' + g p' + w^2 p = D U', as the first
  // order pair p' = r + D U, r' = -g p' - w^2 p (no differencing).
  double footOut = 0;
  if (np_ > 0) {
    const float *v = head_.v(), *vol = head_.volume();
    for (int k = 0, m = head_.modes(); k < m; ++k) U += double(vol[k]) * v[k];
    double pbar = 0;
    for (int n = 0; n < np_; ++n) {
      const double dp = pv_[n] + drive_ * U;
      pp_[n] += dt_ * dp;
      pv_[n] += dt_ * (-pg_[n] * dp - pw_[n] * pw_[n] * pp_[n]);
      pbar += pmean_[n] * pp_[n];
    }
    // Out of the foot: what the head draws in, less what the air in the pipe
    // took by its springiness.
    const double A = kPi * s_.head.radius * s_.head.radius, L = s_.pipeL + 0.6 * s_.footA;
    const double Uout = -U + A * L / (kRho * kC * kC) * (pbar - pbar1_) * fs_;
    pbar1_ = pbar;
    const double s = footK_ * Uout;
    const float y = float((s - footPrev_) * fs_);
    footPrev_ = s;
    footOut = footDelay_[footPos_];
    footDelay_[footPos_] = y;
    footPos_ = (footPos_ + 1) % int(footDelay_.size());
  }
  // The head, at the microphone.
  float sh = 0;
  const float* v = head_.v();
  for (int k = 0, m = head_.modes(); k < m; ++k) sh += mic_[k] * v[k];
  const float yHead = (sh - prev_) * float(fs_);
  prev_ = sh;
  // The contact's own sound above the head's top.
  const float aC = float(std::exp(-2 * kPi * s_.head.top * dt_));
  hpC_ = aC * (hpC_ + F - lastC_);
  lastC_ = F;
  const float out = float(s_.gain * (yHead + footOut + shellOut + clickK_ * clickG_ * hpC_));
  if (!std::isfinite(out)) { reset(); return 0; }
  if (std::fabs(out) < 1e-6f && !active_) {
    if (++quiet_ > int(0.3 * fs_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return out;
}

HandDrumModel::HandDrumModel(const Instrument& inst) : Tube(inst), kind_(kindOf(inst)) {
  drums_.emplace_back(new HandDrum(inst.fs, handSpec(kind_, true)));
  drums_.emplace_back(new HandDrum(inst.fs, handSpec(kind_, false)));
}

void HandDrumModel::resetAir() {
  Tube::resetAir();
  for (auto& d : drums_) d->reset();
}

float HandDrumModel::sample() {
  static const float g = float(knob("PJ_HD_OUT", 1.0 / 8));
  last_ = g * (drums_[0]->sample() + drums_[1]->sample());
  return finish(last_);
}

HandDrumPlayer::HandDrumPlayer(HandDrumModel& m) : Performer(m), m_(m) {}

double HandDrumPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void HandDrumPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  // Which drum, and how: General MIDI's keys; others alternate high and low.
  int drum = midi % 2 ? 1 : 0;
  bool mute = false;
  switch (m_.kind()) {
    case 0:  // 62 mute hi, 63 open hi, 64 low
      if (midi >= 62 && midi <= 64) { drum = midi == 64; mute = midi == 62; }
      break;
    case 1:  // 60 hi, 61 low
      if (midi == 60 || midi == 61) drum = midi == 61;
      break;
    default:  // 65 high, 66 low
      if (midi == 65 || midi == 66) drum = midi == 66;
      break;
  }
  HandDrum& d = m_.drum(drum);
  // Velocity ~ speed^1.25 (General MIDI's ~30 dB from 16 to 127); each stroke
  // a little elsewhere, a little harder or softer.
  static const double ex = knob("PJ_HD_VEXP", 1.25);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  const double speed = d.spec().vmax * fastPow(v, ex) * (0.75 + 0.5 * std::clamp(dyn_, 0.0, 1.0)) * (1 + 0.04 * (ur() - 0.5));
  // The stroke as played this time (a stick on the timbal: only its place).
  // The hand lands a little elsewhere each time, more open or more closed; on
  // open tones the fingertips sometimes stay a moment at the edge (a touch
  // of damping), and now and then the hand drops further in, heavier, and is
  // lifted at once: a rebound off the head, a bigger, rounder sound (more
  // often on accents). A closed tone presses a little harder or longer.
  // Which hand: in quick succession on one drum (under 0.25 s) the other one; after a pause the
  // lead (right) hand. Each its own side and weight.
  const double gap = now_ - lastT_[drum];
  const int hand = m_.kind() == 2 ? 0 : gap < 0.25 ? 1 - lastHand_[drum] : 0;
  const bool repeat = gap < 0.3 && std::abs(velocity - lastVel_[drum]) <= 8;
  lastT_[drum] = now_;
  lastHand_[drum] = hand;
  lastVel_[drum] = velocity;
  // Where it lands: on a bongo's small head ~1 cm either way already changes its overtones
  // (nearer the rim brighter and ringier, further in drier): +-0.1 of its radius there, +-0.04
  // on a conga's. The left hand a little further in.
  const double spread = m_.kind() == 1 ? 0.2 : 0.08;
  double r = d.spec().where - (hand ? 0.04 : 0.0) + spread * (ur() - 0.5);
  const double th = -kPi / 2 + (hand ? -0.35 : 0.35) + 0.4 * (ur() - 0.5);
  HandDrum::Gesture g;
  // Two strokes in a row on a drum, written the same: rarely played the same. The one off the
  // beat (-play: the song's clock) becomes the lighter partner, further in and drier, now and
  // then a heel's muted thud (heel-toe, the martillo's motion); the accent stays open.
  bool partner = false;
  double partnerSpeed = 1;
  if (repeat && m_.kind() != 2) {
    const SongClock& clk = SongClock::get();
    if (clk.valid && clk.playing) {
      const double f = clk.beat - std::floor(clk.beat);
      const bool onBeat = f < 0.08 || f > 0.92;
      if (!onBeat) {
        partner = true;
        partnerSpeed = 0.7 + 0.15 * ur();
      }
    }
  }
  // Two more strokes of the hands (research/hand_drums.md):
  //   - the slap: the fingers, loose, whipped onto the head just inside the rim, light and hard
  //     (bone under little flesh), narrow, lifted at once, while the base of the hand lands on
  //     the rim a moment and chokes the low modes: a high crack, the drum's own pitch held under;
  //   - the bass tone: the flat palm near the middle, heavy, soft and wide, lifted at once: the
  //     deep fundamental, few overtones.
  // General MIDI has no keys for them: in -play the player chooses them as a conguero would, an
  // accent on two or four (the tumbao's slap) mostly on the high drum, a soft stroke on one or
  // three on a conga now and then a bass tone. Measured (velocity 100, the first 30 ms): the slap's
  // centroid 780-1000 Hz against the open tone's 230-540, 8-14 % of it over 1.5 kHz against ~1 %;
  // the conga's bass tone darker than its open tone, nothing over 1.5 kHz; on a bongo the palm
  // gives no bass tone (brighter, if anything): not played there. (PJ_HD_STROKE: 1 every stroke a slap, 2 a bass tone, 0 none:
  // for tests.)
  enum { kTone, kSlap, kBass } stroke = kTone;
  if (m_.kind() != 2 && !mute) {
    static const double kForce = knob("PJ_HD_STROKE", -1);
    const double mean = velMean_[drum];
    velMean_[drum] += 0.15 * (velocity - velMean_[drum]);
    if (kForce >= 0) {
      stroke = kForce == 1 ? kSlap : kForce == 2 ? kBass : kTone;
    } else {
      const SongClock& clk = SongClock::get();
      if (clk.valid && clk.playing) {
        const double b = clk.beat - 4 * std::floor(clk.beat / 4);  // (in the bar, 0 .. 4)
        const int on = int(std::floor(b + 0.08)) % 4;
        const bool onBeat = std::fabs(b - std::round(b)) < 0.08;
        const bool accent = velocity >= mean + 12 || velocity >= 105;
        const bool soft = velocity <= mean - 10;
        const double slapP = (m_.kind() == 0 ? 0.6 : 0.35) * (drum == 0 ? 1.0 : 0.4);
        if (accent && onBeat && (on == 1 || on == 3) && ur() < slapP) stroke = kSlap;
        else if (m_.kind() == 0 && soft && onBeat && (on == 0 || on == 2) && ur() < 0.4) stroke = kBass;  // (not on a bongo: its small head has no bass tone under the palm)
      }
    }
  }
  if (m_.kind() != 2 && stroke != kTone) {
    if (stroke == kSlap) {
      r = d.spec().where - 0.06 + 0.04 * (ur() - 0.5);
      g.mass = 0.45 * (0.9 + 0.2 * ur());
      g.K = 2.2;
      g.band = 0.55;
      g.lift = 2.0;  // (the fingertips snap off: a short, hard contact)
      static const double kSlapClick = knob("PJ_HD_SLAPCLICK", 8);
      g.click = kSlapClick * (0.8 + 0.4 * ur());  // (the crack: the cupped fingers' air squeezed out)
      partnerSpeed *= 1.1;
    } else {
      r = 0.08 + 0.12 * ur();
      g.mass = 2.6 * (0.9 + 0.2 * ur());
      g.K = 0.45;
      g.band = 2.2;
      g.lift = 1.8;
      partnerSpeed *= 0.9;
    }
  } else if (m_.kind() != 2) {
    const double open = ur();  // 0 closed, cupped fingers .. 1 a flat open hand
    g.band = 0.6 + 0.8 * open;
    g.K = 1.5 - 0.8 * open;
    g.mass = 0.85 + 0.3 * ur();
    g.lift = 0.8 + 0.5 * ur();
    if (hand) g.mass *= 0.92;  // (the weaker hand)
    if (mute) {
      g.muteForce = 0.6 + 0.8 * ur();
      g.muteTime = 0.6 + 0.7 * ur();
      g.muteDamp = 0.7 + 0.6 * ur();
    } else if (partner && ur() < 0.35) {  // the heel: the hand's base dropped in, left a moment
      r = d.spec().where - 0.35 - 0.1 * ur();
      g.mass *= 1.8;
      g.K *= 0.6;
      g.band *= 1.4;
      g.muteForce = 0.4 + 0.3 * ur();
      g.muteTime = 0.2 + 0.2 * ur();
      g.muteDamp = 0.6 + 0.4 * ur();
      g.muteR = std::max(0.2, r);
    } else if (partner) {  // a lighter tone, further in: drier
      r -= 0.08 + 0.08 * ur();
    } else if (ur() < 0.12 + 0.25 * v) {  // a rebound stroke into the head
      r = d.spec().where - 0.15 - 0.15 * ur();
      g.mass *= 1.6 + 0.6 * ur();
      g.K *= 0.7;
      g.band *= 1.3;
      g.lift *= 1.6;
    } else if (ur() < 0.25) {  // fingertips left at the edge a moment
      g.muteForce = 0.15 + 0.2 * ur();
      g.muteTime = 0.15 + 0.35 * ur();
      g.muteDamp = 0.15 + 0.3 * ur();
      g.muteR = 0.92 + 0.04 * ur();
    }
  } else if (mute) {
    g.muteForce = 1; g.muteTime = 1;
  }
  d.strike(speed * partnerSpeed, r, th, g);
}

void HandDrumPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  now_ += n / fs_;
  blowing_ = !(m_.drum(0).asleep() && m_.drum(1).asleep());
}

}  // namespace pajupilli
