#include "kick.h"

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
}  // namespace

bool jazzKick(const Instrument& inst) {
  return std::strncmp(inst.name, "jazz-kick", 9) == 0 || std::strcmp(inst.name, "studio-kick") == 0;
}

// The bass drums a drummer may bring (research/kick.md, research/jazz_kick_sound.md), each a size,
// a tuning and a damping; which one a piece gets: chooseKick (src/drumkit.cc).
//   jazz-kick-16  16" x 14": the small room, the trio with brushes; high, tight, open.
//   jazz-kick     18" x 14": the bebop kick; tuned high, both heads, no port, open.
//   jazz-kick-20  20" x 14": post-bop, organ trios, horns in front: a little lower and rounder.
//   jazz-kick-22  22" x 14": the big band; lower, a felt strip on each head (a shorter ring under
//                 the section's hits, the bombs still deep).
//   studio-kick   22" x 16": the pop, funk and disco kick: a looser batter, a ported resonant
//                 head (the air let out), a pillow inside, a hard felt beater at the middle, the
//                 microphone at the port: a short punch with the beater's click.
namespace {
struct KickKind {
  const char* name;
  double inch, deepInch, tension, reso;
  double zeta01, zeta0, zeta, zeta2;  // the felt strips' or pillow's damping, as the heads' zeta
  double vent, feltK, where, click, micZ;
};
constexpr KickKind kKicks[] = {
    {"jazz-kick-16", 16, 14, 760, 1.05, 0.018, 0.012, 0.012, 0.010, 4, 1.5e8, 0.12, 1.0, 0.6},
    {"jazz-kick", 18, 14, 690, 1.0, 0.018, 0.012, 0.012, 0.010, 4, 1.5e8, 0.12, 1.0, 0.6},
    {"jazz-kick-20", 20, 14, 640, 1.0, 0.018, 0.012, 0.012, 0.010, 4, 1.5e8, 0.12, 1.0, 0.6},
    {"jazz-kick-22", 22, 14, 600, 1.0, 0.030, 0.020, 0.020, 0.016, 4, 1.5e8, 0.12, 1.0, 0.6},
    {"studio-kick", 22, 16, 520, 1.0, 0.100, 0.080, 0.060, 0.080, 40, 4.0e8, 0.06, 2.0, 0.25},
};
const KickKind& kickKind(const char* name) {
  for (const KickKind& k : kKicks)
    if (std::strcmp(k.name, name) == 0) return k;
  return kKicks[1];
}
}  // namespace

KickSpec kickSpec(const char* name) {
  const KickKind& k = kickKind(name);
  KickSpec s;
  // A coated single-ply batter, a single-ply resonant head (research/kick.md 2); the size, tuning
  // and damping the kind's. The PJ_KICK_* knobs move every kind alike, as factors on its own.
  const double R = k.inch * 0.0254 / 2;
  s.batter.radius = s.reso.radius = R;
  s.batter.sigma = knob("PJ_KICK_SIGMA", 0.26);
  s.reso.sigma = knob("PJ_KICK_SIGMA2", 0.20);
  s.batter.tension = k.tension * knob("PJ_KICK_TENSION", 1.0);
  s.reso.tension = k.tension * knob("PJ_KICK_TENSION2", 1.0) * k.reso * knob("PJ_KICK_RESO", 1.0);
  s.batter.top = knob("PJ_KICK_TOP", 1400);
  s.reso.top = knob("PJ_KICK_TOP2", 700);
  const double damp = knob("PJ_KICK_DAMP", 1.0);
  s.batter.zeta01 = s.reso.zeta01 = k.zeta01 * damp;
  s.batter.zeta0 = s.reso.zeta0 = k.zeta0 * damp;
  s.batter.zeta = k.zeta * damp;
  s.reso.zeta = k.zeta2 * damp;
  s.batter.lossKnee = s.reso.lossKnee = 1500;
  s.batter.stretch = knob("PJ_KICK_STRETCH", 1.1e6);
  s.reso.stretch = knob("PJ_KICK_STRETCH2", 0.8e6);
  s.batter.seed = 11;
  s.reso.seed = 12;
  s.depth = k.deepInch * 0.0254 * knob("PJ_KICK_DEPTH", 1.0);
  s.air.volume = kPi * R * R * s.depth;
  s.air.vent = k.vent * knob("PJ_KICK_VENT", 1.0);  // the shell's air hole; the studio kick's port
  s.beaterMass = knob("PJ_KICK_MASS", 0.07);
  s.feltK = k.feltK * knob("PJ_KICK_FELTK", 1.0);
  s.feltAlpha = knob("PJ_KICK_FELTALPHA", 2.3);
  s.feltLoss = knob("PJ_KICK_FELTLOSS", 0.6);
  s.beaterR = k.where * knob("PJ_KICK_WHERE", 1.0);
  s.buryForce = knob("PJ_KICK_BURYF", 12);
  s.footDamp = knob("PJ_KICK_FOOTDAMP", 60);
  s.micZ = k.micZ * knob("PJ_KICK_MICZ", 1.0);
  s.click = k.click * knob("PJ_KICK_CLICK", 1);
  s.gain = knob("PJ_KICK_GAIN", 1);
  return s;
}

Kick::Kick(double fs, const KickSpec& spec)
    : s_(spec), fs_(fs), dt_(1 / fs), bat_(fs, spec.batter), res_(fs, spec.reso), air_(spec.air) {
  phi_.assign(bat_.modes(), 0.0f);
  place(s_.beaterR);
  // The microphones in front of the resonant head: each mode of a head by its
  // shape, summed over the head with each patch's distance and its delay at
  // the mode's own pitch. The resonant head faces them (moving towards them is
  // q < 0: the heads' q are both towards the drummer); the batter is heard
  // round the shell, as a source in the open on the far side.
  const int kR = 10, kT = 24;
  for (int head = 0; head < 2; ++head) {
    Membrane& h = head ? res_ : bat_;
    std::vector<float> ph(h.modes());
    for (int mic = 0; mic < 2; ++mic) {
      std::vector<std::complex<double>> g(h.modes());
      const double mx = mic ? s_.micX : 0.0, mz = s_.micZ + (head ? 0.0 : s_.depth);
      const double d0 = std::hypot(mx, s_.micZ);
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
      // (Baffled by the drum, the resonant head radiates rho / 2 pi; the
      // batter, round the shell into the open, about half that.)
      const double k0 = head ? -kRho / (2 * kPi) : 0.5 * kRho / (2 * kPi);
      for (int k = 0; k < h.modes(); ++k) out[k] = float((g[k].real() < 0 ? -1 : 1) * std::abs(g[k]) * k0);
    }
  }
}

void Kick::place(double r) {
  // The felt's patch: its middle and six points round it, the shapes averaged
  // (the felt's width takes the edge off the highest modes).
  std::fill(phi_.begin(), phi_.end(), 0.0f);
  std::vector<float> ph(bat_.modes());
  const double R = bat_.radius(), rc = r * R, a = s_.feltRadius;
  const double th0 = -kPi / 2;  // below the middle
  const double x0 = rc * std::cos(th0), y0 = rc * std::sin(th0);
  for (int i = 0; i < 7; ++i) {
    const double x = x0 + (i ? 0.6 * a * std::cos(2 * kPi * i / 6) : 0), y = y0 + (i ? 0.6 * a * std::sin(2 * kPi * i / 6) : 0);
    bat_.shape(std::hypot(x, y), std::atan2(y, x), ph.data());
    for (int k = 0; k < bat_.modes(); ++k) phi_[k] += ph[k] / 7;
  }
}

void Kick::strike(double speed, double r, bool bury) {
  if (std::fabs(r - s_.beaterR) > 1e-9) { s_.beaterR = r; place(r); }
  // Thrown from just over the head where it is now: the stroke meets its swing.
  const double h = bat_.height(phi_.data());
  zb_ = h + 2e-4;
  vb_ = -std::fabs(speed);
  rest_ = 0.05;  // the pedal's rest, 5 cm off the head
  active_ = true;
  left_ = false;
  hit_ = false;
  bury_ = bury;
  asleep_ = false;
  quiet_ = 0;
}

void Kick::reset() {
  bat_.reset();
  res_.reset();
  air_.leak = 0;
  active_ = false;
  zb_ = 1; vb_ = 0;
  prev_[0] = prev_[1] = 0;
  hpC_ = lastC_ = 0;
  asleep_ = true;
}

void Kick::setOutside(float pBatter, float pReso) {
  pOutB_ = pBatter;
  pOutR_ = pReso;
  // (Asleep, the drum wakes for a room loud enough to move it audibly.)
  if (asleep_ && std::max(std::fabs(pBatter), std::fabs(pReso)) > 2e-3f) { asleep_ = false; quiet_ = 0; }
}

float Kick::sample() {
  if (asleep_) { side_ = 0; return 0; }
  // The beater against the batter (heights towards the drummer; the felt
  // pushes the head in, the head pushes the felt back).
  float F = 0;
  if (active_) {
    const double h = bat_.height(phi_.data()), vh = bat_.speed(phi_.data());
    const double pen = h - zb_;
    if (pen > 0) {
      const double rate = vh - vb_;
      F = float(std::max(0.0, s_.feltK * fastPow(pen, s_.feltAlpha) * (1 + s_.feltLoss * rate)));
      touch_ = true;
      hit_ = true;
    } else {
      touch_ = false;
    }
    // The pedal's spring pulls the beater back to its rest; the foot, if it
    // stays on, leans it into the head.
    const double w = 2 * kPi * s_.returnHz;
    // (Until it lands the foot drives it: no spring on the way in.)
    const bool spring = hit_ && !bury_;
    double Fb = F - (spring ? s_.beaterMass * w * w * (zb_ - (h + rest_)) : 0.0);
    // Buried, the foot holds the beater against the head: its weight, and the
    // leg's damping (a leg does not bounce on the felt).
    if (bury_) Fb -= s_.buryForce + (hit_ ? s_.footDamp * vb_ : 0.0);
    const double ab = Fb / s_.beaterMass;
    vb_ += ab * dt_;
    zb_ += vb_ * dt_;
    if (F > 0) bat_.push(phi_.data(), -F);
    // Off the head by more than a few mm, going back: the stroke is over (the
    // beater never comes down again by itself).
    if (!bury_ && pen < -3e-3 && vb_ > 0) { left_ = true; active_ = false; }
  }
  lastF_ = F;
  // The room presses on both heads from outside, against the cavity's push
  // (DrumAir: the inside pushes the batter by +p vol, the resonant head by
  // -p vol). The same pressure on both squeezes the air inside; their
  // difference (the far head hears it later) moves the pair together, the
  // drum's note.
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
  // The felt's thump above what the modes carry: the contact force on the
  // light film moves the air at once (rho F / (2 pi sigma r)), heard round the
  // shell; only its part above the batter's top.
  const float aC = float(std::exp(-2 * kPi * s_.batter.top * dt_));
  hpC_ = aC * (hpC_ + F - lastC_);
  lastC_ = F;
  const float rMic = float(s_.micZ + s_.depth);
  const float click = float(s_.click * 0.5 * kRho / (2 * kPi * s_.batter.sigma * rMic)) * hpC_;
  const float g = float(s_.gain);
  const float l = g * (y[0] + click), r = g * (y[1] + click);
  side_ = 0.5f * (r - l);
  const float mono = 0.5f * (l + r);
  if (!std::isfinite(mono)) { reset(); return 0; }
  if (std::fabs(mono) < 1e-6f && !active_) {
    if (++quiet_ > int(0.3 * fs_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return mono;
}

KickModel::KickModel(const Instrument& inst)
    : Tube(inst), kick_(inst.fs, kickSpec(inst.name)) {}

void KickModel::resetAir() {
  Tube::resetAir();
  kick_.reset();
}

const float* KickModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

float KickModel::sample() {
  static const float g = float(knob("PJ_KICK_OUT", 1.0 / 8));
  // The room's sound here (Pa; none without a room): the resonant head faces
  // the band, the batter the drummer, a shell's depth later (sound along the
  // drum's axis: the most its two heads can differ). PJ_KICK_ROOM scales it:
  // 0.3, as the room model is a reverberator, not a passive room at 70 Hz on
  // two 18" heads. At full strength a studio returns the drum's own ring
  // about as loud as the drum (loop gain near 1). At 0.3 it is 18 dB under it
  // in the studio and 37-39 dB in the hall and church.
  static const float kRoom = float(knob("PJ_KICK_ROOM", 0.3));
  if (roomPos_ < roomN_) {
    pRoom_ = room_[roomPos_++];
    if (delay_.empty()) delay_.assign(std::max(1, int(std::lround(kick_.spec().depth / 343.0 * fs()))), 0.0f);
    const float far = delay_[delayPos_];
    delay_[delayPos_] = pRoom_;
    delayPos_ = (delayPos_ + 1) % int(delay_.size());
    kick_.setOutside(kRoom * far, kRoom * pRoom_);
  }
  last_ = g * kick_.sample();
  side_.push_back(g * float(outScale()) * kick_.side());
  return finish(last_);
}

KickPlayer::KickPlayer(KickModel& m) : Performer(m), m_(m) {
  if (knob("PJ_KICK_BURY", 0) > 0) m_.kick().setBury(true);
}

double KickPlayer::ur() { return 0.5 * (rng_.uniform() + 1); }

double KickPlayer::beaterSpeed(int velocity, double dynamics) {
  // ~ velocity^1.25 (General MIDI's 30 dB from 16 to 127; the drum is linear
  // in the beater's speed, but for its stretch and the felt): feathered keys
  // (20 .. 70) 0.2 .. 1.1 m/s, a bomb 4.5 m/s.
  static const double top = knob("PJ_KICK_VMAX", 4.5), ex = knob("PJ_KICK_VEXP", 1.25);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return top * fastPow(v, ex) * (0.75 + 0.5 * std::clamp(dynamics, 0.0, 1.0));
}

void KickPlayer::setHold(bool h) {
  hold_ = h;
  static const bool always = knob("PJ_KICK_BURY", 0) > 0;
  m_.kick().setBury(h || always);
}

void KickPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  static const bool always = knob("PJ_KICK_BURY", 0) > 0;
  // 35 a little lower on the head (more ring), 36 nearer the middle; each
  // stroke a little elsewhere (the foot is not a machine).
  const double r = (midi == 35 ? 0.22 : 0.12) + 0.03 * (ur() - 0.5);
  m_.kick().strike(beaterSpeed(velocity, dyn_) * (1 + 0.04 * (ur() - 0.5)), r, hold_ || always);
}

void KickPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.kick().asleep();
}

}  // namespace pajupilli
