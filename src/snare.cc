#include "snare.h"

#include <algorithm>
#include <cmath>
#include <complex>

#include "knob.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2, kC = 343.0;
}  // namespace

SnareSpec snareSpec() {
  SnareSpec s;
  // A 14" x 5.5" jazz snare: a coated batter tuned high, a thin snare head
  // tuned higher still.
  s.batter.radius = s.snareHead.radius = 0.178;
  s.batter.sigma = knob("PJ_SNARE_SIGMA", 0.26);
  s.batter.tension = knob("PJ_SNARE_TENSION", 3000);
  s.batter.top = knob("PJ_SNARE_TOP", 4500);
  s.batter.zeta01 = knob("PJ_SNARE_ZETA01", 0.03);
  s.batter.zeta0 = knob("PJ_SNARE_ZETA0", 0.012);
  s.batter.zeta = knob("PJ_SNARE_ZETA", 0.008);
  s.batter.stretch = knob("PJ_SNARE_STRETCH", 1.1e6);
  s.batter.seed = 1;
  s.snareHead.sigma = knob("PJ_SNARE_SIGMA2", 0.106);
  s.snareHead.tension = knob("PJ_SNARE_TENSION2", 2600);
  s.snareHead.top = knob("PJ_SNARE_TOP2", 4500);
  s.snareHead.zeta01 = knob("PJ_SNARE_ZETA01", 0.03);
  s.snareHead.zeta0 = knob("PJ_SNARE_ZETA0", 0.012);
  s.snareHead.zeta = knob("PJ_SNARE_ZETA2", 0.012);
  s.snareHead.seed = 2;
  s.air.volume = kPi * 0.178 * 0.178 * knob("PJ_SNARE_DEPTH", 0.14);
  s.wires = int(knob("PJ_SNARE_WIRES", 10));
  s.wireMu = knob("PJ_SNARE_WIREMU", 0.015);
  s.wireTension = knob("PJ_SNARE_WIRETENSION", 8);
  s.wireLoss = knob("PJ_SNARE_WIRELOSS", 0.15);
  s.wirePress = knob("PJ_SNARE_WIREPRESS", 0.06e-3);
  s.wireContact = knob("PJ_SNARE_WIREK", 6e4);
  s.wireModes = int(knob("PJ_SNARE_WIREMODES", 8));
  s.wirePoints = int(knob("PJ_SNARE_WIREPOINTS", 8));
  s.wireContactLoss = knob("PJ_SNARE_WIRECLOSS", 3.0);
  s.under = knob("PJ_SNARE_UNDER", 0.45);
  s.wireDirect = knob("PJ_SNARE_WIREDIRECT", 1);
  s.tipDirect = knob("PJ_SNARE_TIPDIRECT", 1);
  s.aim = knob("PJ_SNARE_AIM", 0.5);      // the directional parts (0: the drum sounds alike all round; 1: all the far field's)
  s.tipAim = knob("PJ_SNARE_TIPAIM", 0.6);
  s.gain = knob("PJ_SNARE_GAIN", 1);
  s.brush.tips = int(knob("PJ_BRUSH_TIPS", 24));
  s.brush.fan = knob("PJ_BRUSH_FAN", 400);
  s.brush.fanLoss = knob("PJ_BRUSH_FANLOSS", 2.0);
  s.brush.handle = knob("PJ_BRUSH_HANDLE", 0.020);
  s.brush.grip = knob("PJ_BRUSH_GRIP", 2000);
  s.brush.tipMass = knob("PJ_BRUSH_TIPMASS", 5e-5);
  s.brush.contact = knob("PJ_BRUSH_CONTACT", 2e4);
  s.brush.grain = knob("PJ_BRUSH_GRAIN", 0.18e-3);
  s.brush.rough = knob("PJ_BRUSH_ROUGH", 8e-6);
  s.brush.bite = knob("PJ_BRUSH_BITE", 0.15);
  s.brush.drag = knob("PJ_BRUSH_DRAG", 0.5);
  return s;
}

Snare::Snare(double fs, const SnareSpec& spec)
    : s_(spec), fs_(fs), dt_(1 / fs), bat_(fs, spec.batter), sn_(fs, spec.snareHead), air_(spec.air) {
  brush_[0].reset(new Brush(fs, bat_, spec.brush, 1));
  brush_[1].reset(new Brush(fs, bat_, spec.brush, 2));
  // The wires: side by side across the snare head, each a string between
  // the strainer's ends near the hoop.
  nw_ = s_.wires; nm_ = s_.wireModes; np_ = s_.wirePoints; nc_ = nw_ * np_;
  nh_ = 0;
  while (nh_ < sn_.modes() && sn_.frequency(nh_) < s_.wireTop) ++nh_;
  nh_ = (nh_ + 15) / 16 * 16;
  nh_ = std::min(nh_, sn_.modes());
  wq_.assign(size_t(nw_) * nm_, 0); wv_ = wq_; ww2_ = wq_; wa_ = wq_;
  wShape_.assign(size_t(np_) * nm_, 0);
  cPhi_.assign(size_t(nc_) * nh_, 0);
  cz_.assign(nc_, 0); cv_ = cz_; cf_ = cz_;
  std::vector<float> phi(sn_.modes());
  uint32_t rng = 777;
  auto ur = [&]() { rng = rng * 1664525u + 1013904223u; return (rng >> 8) * (1.0 / 16777216.0); };
  const double R = s_.snareHead.radius;
  double meanL = 0;
  for (int w = 0; w < nw_; ++w) {
    const double y = (w - 0.5 * (nw_ - 1)) * s_.wireSpacing;
    const double a = 0.94 * std::sqrt(R * R - y * y), L = 2 * a;
    meanL += L / nw_;
    const double f1 = std::sqrt(s_.wireTension * (0.9 + 0.2 * ur()) / s_.wireMu) / (2 * L);
    for (int i = 0; i < nm_; ++i) {
      const double f = (i + 1) * f1 * (1 + 0.01 * (i + 1) * ur());  // (a coil is not quite a string)
      const double wr = 2 * kPi * f;
      ww2_[size_t(w) * nm_ + i] = float(wr * wr);
      wa_[size_t(w) * nm_ + i] = float(2 * s_.wireLoss * wr * (1 + f / 1500));
    }
    for (int p = 0; p < np_; ++p) {
      const double sPos = (p + 1.0) / (np_ + 1), x = -a + sPos * L;
      sn_.shape(std::hypot(x, y), std::atan2(y, x), phi.data());
      std::copy(phi.begin(), phi.begin() + nh_, cPhi_.begin() + size_t(w * np_ + p) * nh_);
      if (w == 0)
        for (int i = 0; i < nm_; ++i) wShape_[size_t(p) * nm_ + i] = float(std::sin((i + 1) * kPi * sPos));
    }
  }
  wMass_ = float(s_.wireMu * meanL / 2);
  // The head bulges a little into the wires' line (the snare beds lower its
  // edge where they cross): an arch, so the wires lie on it all along with a
  // light, even press: their tension x the arch's curvature.
  press_.resize(np_);
  for (int p = 0; p < np_; ++p) press_[p] = float(s_.wirePress * std::sin(kPi * (p + 1.0) / (np_ + 1)));
  // The microphones: each mode of a head by its shape, summed over the head
  // with every patch's distance (and its delay, at the mode's own pitch).
  const int kR = 10, kT = 24;
  for (int head = 0; head < 2; ++head) {
    Membrane& h = head ? sn_ : bat_;
    std::vector<float> ph(h.modes());
    for (int mic = 0; mic < 2; ++mic) {
      std::vector<std::complex<double>> g(h.modes());
      const double mx = (mic ? 1 : -1) * s_.micX, mz = s_.micZ + (head ? 0.25 : 0.0);
      const double d0 = std::hypot(mx, mz);
      for (int ir = 0; ir < kR; ++ir)
        for (int it = 0; it < kT; ++it) {
          const double r = h.radius() * (ir + 0.5) / kR, th = 2 * kPi * (it + 0.5) / kT;
          const double dA = 2 * kPi * r * (h.radius() / kR) / kT;
          h.shape(r, th, ph.data());
          const double d = std::sqrt((r * std::cos(th) - mx) * (r * std::cos(th) - mx) + r * std::sin(th) * r * std::sin(th) + mz * mz);
          for (int k = 0; k < h.modes(); ++k) {
            const double kk = 2 * kPi * h.frequency(k) / kC;
            g[k] += double(ph[k]) * dA * std::exp(std::complex<double>(0, -kk * (d - d0))) / d;
          }
        }
      std::vector<float>& out = head ? micS_[mic] : micB_[mic];
      out.assign((h.modes() + 15) / 16 * 16, 0.0f);
      for (int k = 0; k < h.modes(); ++k)
        out[k] = float((g[k].real() < 0 ? -1 : 1) * std::abs(g[k]) * kRho / (2 * kPi) * (head ? -s_.under : 1.0));
    }
  }
  // The sideways parts, far off and level with the head: towards +x a mode's patches arrive
  // each by its own path, exp(i k x); the same towards -x with x mirrored. Half the difference
  // is the integral of the shape x sin(k x): nothing for a mode alike on both sides, the two
  // halves' difference for the (1, n)s (a pair: one member along its own line, one across).
  // To the microphones' scale (their distance).
  for (int head = 0; head < 2; ++head) {
    Membrane& h = head ? sn_ : bat_;
    std::vector<float> ph(h.modes());
    std::vector<double> gx(h.modes(), 0.0), gy(h.modes(), 0.0);
    const double d0 = std::hypot(s_.micX, s_.micZ);
    const int nR = 24, nT = 72;  // (finer than the microphones': sin(k x) has its own ripples)
    for (int ir = 0; ir < nR; ++ir)
      for (int it = 0; it < nT; ++it) {
        const double r = h.radius() * (ir + 0.5) / nR, th = 2 * kPi * (it + 0.5) / nT;
        const double dA = 2 * kPi * r * (h.radius() / nR) / nT;
        h.shape(r, th, ph.data());
        for (int k = 0; k < h.modes(); ++k) {
          const double kk = 2 * kPi * h.frequency(k) / kC;
          gx[k] += double(ph[k]) * dA * std::sin(kk * r * std::cos(th));
          gy[k] += double(ph[k]) * dA * std::sin(kk * r * std::sin(th));
        }
      }
    for (int ax = 0; ax < 2; ++ax) {
      std::vector<float>& out = head ? dipS_[ax] : dipB_[ax];
      out.assign((h.modes() + 15) / 16 * 16, 0.0f);
      for (int k = 0; k < h.modes(); ++k)
        out[k] = float(s_.aim * (ax ? gy[k] : gx[k]) * kRho / (2 * kPi * d0) * (head ? -s_.under : 1.0));
    }
  }
  // The cross-stick. A hickory stick, 40 cm, free at both ends: 324, 892,
  // 1749 Hz, the hand and the head on it (dead in 40 ms). The hoop, a steel
  // ring bending out of its plane: n (n^2 - 1) / sqrt(n^2 + 1) x 137 Hz for
  // n = 2 .. 6, held by the lugs and the head (60 .. 25 ms). (Their loudness:
  // by ear, a first form: research/jazz_drums.md 5.5.)
  {
    const double f[8] = {324, 892, 1749, 368, 1040, 1995, 3226, 4730};
    const double t60[8] = {0.040, 0.035, 0.030, 0.060, 0.055, 0.045, 0.035, 0.025};
    const double g[8] = {0.5, 1.0, 0.8, 0.35, 0.8, 1.0, 0.7, 0.4};
    for (int i = 0; i < 8; ++i) {
      const double w = 2 * kPi * f[i], a = 6.91 / t60[i], gd = std::exp(-a * dt_), c = std::cos(w * dt_), sn = std::sin(w * dt_);
      Knock k;
      k.a11 = float(gd * c); k.a12 = float(gd * sn / w); k.a21 = float(-gd * w * sn); k.a22 = float(gd * c);
      k.gain = float(knob("PJ_SNARE_KNOCK", 1.0) * 0.3 * g[i]);
      k.pan = float(i < 3 ? 0.1 : (i % 2 ? 0.25 : -0.25));
      knock_.push_back(k);
    }
    xPhi_.assign(bat_.modes(), 0.0f);
    bat_.shape(0.45 * s_.batter.radius, 2.6, xPhi_.data());
  }
  settle();
}

void Snare::crossStick(double speed) {
  wake();
  // The butt's blow: every mode set swinging at once (a contact far shorter
  // than their periods), harder ones a little brighter.
  for (size_t i = 0; i < knock_.size(); ++i)
    knock_[i].v += float(speed * (i >= 5 ? std::min(1.0, 0.5 + 0.25 * speed) : 1.0));
  // The tip end rocks on the head: a soft push, a millisecond long.
  xForce_ = 1.5 * speed;
  xT_ = 0;
  xLie_ = 0.12;  // the stick lies on the head this long: its ring is short
  bat_.setDamping(4);
}

void Snare::settle() {
  // The wires come to rest against the head (their line lies a little into it).
  const bool on = wiresOn_;
  std::vector<float> keep = wa_;
  for (float& a : wa_) a = std::max(a * 20, 600.0f);
  bat_.setDamping(30);
  sn_.setDamping(30);
  asleep_ = false;
  for (int i = 0; i < int(0.25 * fs_); ++i) sample();
  wa_ = keep;
  bat_.setDamping(1);
  sn_.setDamping(1);
  hpW_ = hpT_ = 0;  // (the wires' resting press is no sound)
  wiresOn_ = on;
  asleep_ = true;
  quiet_ = 0;
}

void Snare::reset() {
  bat_.reset();
  sn_.reset();
  air_.leak = 0;
  dprev_[0] = dprev_[1] = dip_[0] = dip_[1] = hpH_[0] = hpH_[1] = lastH_[0] = lastH_[1] = 0;
  std::fill(wq_.begin(), wq_.end(), 0.0f);
  std::fill(wv_.begin(), wv_.end(), 0.0f);
  settle();
}

float Snare::sample() {
  if (asleep_) {
    if (brush_[0]->up() && brush_[1]->up()) { side_ = 0; dip_[0] = dip_[1] = 0; return 0; }
    asleep_ = false;
    quiet_ = 0;
  }
  const float dt = float(dt_);
  brush_[0]->step();
  brush_[1]->step();
  if (xT_ >= 0) {
    if (xT_ < 0.001) bat_.push(xPhi_.data(), float(-xForce_ * std::sin(kPi * xT_ / 0.001)));
    xT_ += dt_;
    if (xT_ > xLie_) { xT_ = -1; bat_.setDamping(1); }
  }
  air_.couple(bat_, sn_, dt_);
  // The wires against the snare head.
  float wireF = 0;
  if (wiresOn_) {
    const float *q = sn_.q(), *v = sn_.v();
    for (int c = 0; c < nc_; ++c) {
      const float* ph = &cPhi_[size_t(c) * nh_];
      float z = 0, zv = 0;
      for (int k = 0; k < nh_; ++k) { z += ph[k] * q[k]; zv += ph[k] * v[k]; }
      cz_[c] = z; cv_[c] = zv;
    }
    // (The contact gives more force while closing than while parting, and
    // none at the touch itself: no jump as a wire lands or leaves.)
    const float K = float(s_.wireContact), lossV = float(s_.wireContactLoss);
    for (int w = 0; w < nw_; ++w) {
      float* wq = &wq_[size_t(w) * nm_];
      float* wv = &wv_[size_t(w) * nm_];
      float fm[32] = {};
      for (int p = 0; p < np_; ++p) {
        const float* sh = &wShape_[size_t(p) * nm_];
        float z = 0, zv = 0;
        for (int i = 0; i < nm_; ++i) { z += sh[i] * wq[i]; zv += sh[i] * wv[i]; }
        const int c = w * np_ + p;
        const float pen = z + press_[p] - cz_[c];
        float f = 0;
        if (pen > 0) f = K * pen * std::max(0.0f, 1 + lossV * (zv - cv_[c]));
        cf_[c] = f;
        wireF += f;
        for (int i = 0; i < nm_; ++i) fm[i] -= sh[i] * f;
      }
      const float* w2 = &ww2_[size_t(w) * nm_];
      const float* wa = &wa_[size_t(w) * nm_];
      for (int i = 0; i < nm_; ++i) {
        wv[i] += dt * (fm[i] / wMass_ - w2[i] * wq[i] - wa[i] * wv[i]);
        wq[i] += dt * wv[i];
      }
    }
    float* f = sn_.force();
    for (int c = 0; c < nc_; ++c) {
      if (cf_[c] == 0) continue;
      const float* ph = &cPhi_[size_t(c) * nh_];
      const float fc = cf_[c];
      for (int k = 0; k < nh_; ++k) f[k] += ph[k] * fc;
    }
  }
  wireF_ = wireF;
  bat_.step();
  sn_.step();
  // The microphones: the heads' volume acceleration by each mode's shape.
  float y[2];
  for (int mic = 0; mic < 2; ++mic) {
    float s = 0;
    const float *vb = bat_.v(), *mb = micB_[mic].data();
    for (int k = 0; k < bat_.modes(); ++k) s += mb[k] * vb[k];
    const float *vs = sn_.v(), *ms = micS_[mic].data();
    for (int k = 0; k < sn_.modes(); ++k) s += ms[k] * vs[k];
    y[mic] = (s - prev_[mic]) * float(fs_);
    prev_[mic] = s;
  }
  // (the sideways parts, as the microphones')
  float dp[2];
  for (int ax = 0; ax < 2; ++ax) {
    float s = 0;
    const float *vb = bat_.v(), *mb = dipB_[ax].data();
    for (int k = 0; k < bat_.modes(); ++k) s += mb[k] * vb[k];
    const float *vs = sn_.v(), *ms = dipS_[ax].data();
    for (int k = 0; k < sn_.modes(); ++k) s += ms[k] * vs[k];
    dp[ax] = (s - dprev_[ax]) * float(fs_);
    dprev_[ax] = s;
  }
  // What the modes do not carry: the wires' slaps and the wire ends' ticks
  // and scrape, above ~2 kHz, as their own small sources.
  // A force f on a light film sets the air moving as a whole by f / sigma
  // (the film's own mass is all that resists at first): rho f / (2 pi sigma r)
  // at the microphone. The modes carry that up to their top; this is the rest.
  const float tipF = brush_[0]->force() + brush_[1]->force();
  const float aW = float(std::exp(-2 * kPi * s_.wireTop * dt_)), aT = float(std::exp(-2 * kPi * s_.batter.top * dt_));
  hpW_ = aW * (hpW_ + wireF - lastW_); lastW_ = wireF;
  hpT_ = aT * (hpT_ + tipF - lastT_); lastT_ = tipF;
  const float rMic = float(std::hypot(s_.micX, s_.micZ));
  const float direct = float(s_.wireDirect * s_.under * kRho / (2 * kPi * s_.snareHead.sigma * (rMic + 0.25))) * hpW_ -
                       float(s_.tipDirect * kRho / (2 * kPi * s_.batter.sigma * rMic)) * hpT_;
  // (The wire ends' hiss comes off the patch of head under the brush: on the side the brush is
  // on there is only the rim in its way, to the other side the whole head and the other hand.)
  {
    const float kTip = float(s_.aim * s_.tipAim * s_.tipDirect * kRho / (2 * kPi * s_.batter.sigma * rMic) / bat_.radius());
    for (int h = 0; h < 2; ++h) {
      const float f = brush_[h]->force();
      hpH_[h] = aT * (hpH_[h] + f - lastH_[h]); lastH_[h] = f;
      dp[0] -= kTip * float(brush_[h]->x()) * hpH_[h];
      dp[1] -= kTip * float(brush_[h]->y()) * hpH_[h];
    }
  }
  float kn = 0, knSide = 0;
  for (Knock& k : knock_) {
    const float x = k.a11 * k.x + k.a12 * k.v, v = k.a21 * k.x + k.a22 * k.v;
    k.x = x; k.v = v;
    kn += k.gain * v;
    knSide += k.gain * k.pan * v;
  }
  const float g = float(s_.gain);
  const float l = g * (y[0] + direct + kn + knSide), r = g * (y[1] + direct + kn - knSide);
  side_ = 0.5f * (l - r);
  dip_[0] = g * (dp[0] + float(s_.aim) * knSide);
  dip_[1] = g * dp[1];
  const float mono = 0.5f * (l + r);
  if (!std::isfinite(mono)) { reset(); return 0; }
  // Asleep once both hands are up and nothing is left ringing.
  if (std::fabs(mono) < 1e-6f && brush_[0]->up() && brush_[1]->up()) {
    if (++quiet_ > int(0.3 * fs_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return mono;
}

}  // namespace pajupilli
