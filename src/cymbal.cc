#include "cymbal.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>
#include <cstdio>
#include <cstdlib>

#include "knob.h"
#include "songclock.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2, kC = 343.0;
const char* const kNames[6] = {"ride-cymbal", "ride-cymbal-2", "crash-cymbal", "crash-cymbal-2", "splash-cymbal",
                               "china-cymbal"};
int whichOf(const Instrument& inst) {
  for (int i = 0; i < 6; ++i)
    if (std::strcmp(inst.name, kNames[i]) == 0) return i;
  return -1;
}
// A dome of height H over radius R, as one sphere: R^2 / (2 H).
double sphere(double R, double H) { return R * R / (2 * H); }
}  // namespace

bool cymbal(const Instrument& inst) { return whichOf(inst) >= 0; }

CymbalSpec cymbalSpec(int which) {
  CymbalSpec s;
  PlateSpec& p = s.plate;
  p.top = knob("PJ_CYM_TOP", 16000);
  p.rho = 8600;
  // The brush on bronze: many light wire ends, each bouncing on its own
  // (the fan's friction holds the fan as a whole, not every end to the
  // handle: a scatter of small landings, not one blow), on a hard surface
  // with lathing grooves.
  s.brush.tips = int(knob("PJ_CYM_BRUSHTIPS", 48));
  s.brush.tipMass = knob("PJ_CYM_TIPMASS", 2.5e-5);
  s.brush.contact = knob("PJ_CYM_BRUSHK", 1.2e4);
  s.brush.together = knob("PJ_CYM_TOGETHER", 0.1);
  s.brush.tipLoss = knob("PJ_CYM_TIPLOSS", 0.15);
  // The fan's own friction (the wires rubbing): the snare's 2 carries the
  // handle's whole momentum into the first few ends that stop, and a long
  // follow-through lands as one blow; on bronze 0.4, and the ends keep
  // landing one by one as the hand lets the fan through (pajupilli-78,
  // measured on the hi-hat: 2 -> 6 landings 5.3 N; 0.25 -> 13 over 29 ms).
  s.brush.fanLoss = knob("PJ_CYM_FANLOSS", 0.4);
  s.brush.rough = 3e-6;
  s.brush.grain = 0.3e-3;
  // The lathing: tonal grooves ~0.6 mm apart, the ridges between them a few
  // microns over the floor (an unlathed china: none).
  s.brush.groove = knob("PJ_CYM_GROOVE", 0.6e-3);
  s.brush.ridge = which == 5 ? 0.0 : knob("PJ_CYM_RIDGE", 4e-6);
  s.brush.reshape = knob("PJ_CYM_RESHAPE", 0.1e-3);
  // Losses (1/s) lossA w^0.6 + lossB: a ride's band T60s (FluidR3's recorded
  // ride: 3.9 s at 1-2 kHz, 2.6 s at 4 kHz, 1.65 s at 8 kHz, 1.06 s at
  // 12.5 kHz), a little longer in the lows (the user: free cymbals die
  // slowly); a crash on its stand ~3 s at 1 kHz, under 1 s above 10 kHz
  // (research/jazz_drums.md 3.3). The crashes ring longer than that, x PJ_CYM_CRASHRING (1.7: ~5.4 s
  // at 1 kHz, 2.8 s at 4 kHz, 1.5 s at 12.5 kHz): built to those figures they died away too soon
  // by ear (the user's, in manteca.mid); the plate's cascade that would keep a crash's shimmer up
  // is not built.
  static const double kCrashRing = knob("PJ_CYM_CRASHRING", 1.7);
  switch (which) {
    default:
    case 0:  // 22" thin jazz ride: hammered, a small bell, the bow 3 cm high
      p.radius = 0.2794; p.thickness = 1.15e-3; p.domeRadius = sphere(p.radius, 0.030);
      p.lossA = 0.0055; p.lossB = 0.5; p.split = 0.012; p.seed = 21;
      s.ride = true; s.bowAt = 0.62; s.bellR = 0.065;
      {  // (to FluidR3's ride: 500 Hz 5.6 -> 3.8 s, 2 kHz 2.5 -> 3.9 s, ...)
        const double f[7] = {250, 500, 1000, 2000, 4000, 8000, 12500};
        const double m[7] = {1.47, 1.47, 1.15, 0.64, 0.88, 1.0, 1.2};
        s.nFix = 7;
        std::copy(f, f + 7, s.fixF);
        std::copy(m, m + 7, s.fixMul);
      }
      break;
    case 1: {  // 20" medium ride, drier: heavier, more damped in the highs, a bigger bell
      p.radius = 0.254; p.thickness = 1.4e-3; p.domeRadius = sphere(p.radius, 0.028);
      p.lossA = 0.0075; p.lossB = 0.7; p.split = 0.008; p.seed = 22;
      s.ride = true; s.bowAt = 0.6; s.bellR = 0.075;
      const double f[6] = {950, 1460, 2010, 2560, 3240, 3990};
      std::copy(f, f + 6, s.bellF);
      break;
    }
    case 2: {  // 16" thin crash
      p.radius = 0.2032; p.thickness = 0.9e-3; p.domeRadius = sphere(p.radius, 0.025);
      p.lossA = 0.008; p.lossB = 0.7; p.split = 0.010; p.seed = 23;
      p.lossA /= kCrashRing; p.lossB /= kCrashRing;
      s.ride = false; s.bellR = 0.05; s.bellGain = 0.5;
      const double f[6] = {1150, 1760, 2420, 3090, 3900, 4810};
      std::copy(f, f + 6, s.bellF);
      break;
    }
    case 3: {  // 18" thin crash
      p.radius = 0.2286; p.thickness = 0.95e-3; p.domeRadius = sphere(p.radius, 0.026);
      p.lossA = 0.008; p.lossB = 0.65; p.split = 0.010; p.seed = 24;
      p.lossA /= kCrashRing; p.lossB /= kCrashRing;
      s.ride = false; s.bellR = 0.055; s.bellGain = 0.5;
      const double f[6] = {1000, 1530, 2110, 2690, 3400, 4190};
      std::copy(f, f + 6, s.bellF);
      break;
    }
    case 4: {  // 10" splash: thin, small, short
      p.radius = 0.127; p.thickness = 0.6e-3; p.domeRadius = sphere(p.radius, 0.015);
      p.lossA = 0.011; p.lossB = 1.8; p.split = 0.010; p.seed = 25;
      s.ride = false; s.bellR = 0.035; s.bellGain = 0.4;
      const double f[6] = {1800, 2750, 3790, 4840, 6100, 7500};
      const double t[6] = {1.5, 1.3, 1.1, 1.0, 0.9, 0.8};
      std::copy(f, f + 6, s.bellF);
      std::copy(t, t + 6, s.bellT60);
      s.micZ = 0.4;
      break;
    }
    case 5: {  // 18" china: thin, unlathed, heavily hammered, a flatter bow and a
               // cylindrical bell; trashy: wide-split pairs, more loss up high
      p.radius = 0.2286; p.thickness = 0.85e-3; p.domeRadius = sphere(p.radius, 0.018);
      p.lossA = 0.012; p.lossB = 0.9; p.split = 0.04; p.seed = 26;
      s.ride = false; s.bellR = 0.06; s.bellGain = 0.6;
      const double f[6] = {700, 980, 1370, 1690, 2210, 2730};
      const double t[6] = {2.5, 2.2, 2.0, 1.8, 1.5, 1.2};
      std::copy(f, f + 6, s.bellF);
      std::copy(t, t + 6, s.bellT60);
      break;
    }
  }
  // (The cup as a piston over the mode sum: set so a bell stroke is a few dB
  // over a bow stroke of the same velocity.)
  s.bellGain *= 0.015;
  // (One sphere's dome lifts every stretching mode just above one pitch, and
  // on an edge stroke their round members drown the rest. A cymbal is most
  // curved at the bell and nearly flat at the edge: the curvature tapers,
  // PJ_CYM_TAPER, and each mode is lifted by the curvature where it moves.
  // At the real dome height that still piles them up (the crash's 400-800 Hz
  // octave all but 0.2 dB of its energy), so these take a quarter of it,
  // PJ_CYM_DOME, until the plate has its true profile.)
  if (!s.ride) {
    p.domeTaper = knob("PJ_CYM_TAPER", 1.4);
    const double h = p.radius * p.radius / (2 * p.domeRadius) * knob("PJ_CYM_DOME", 0.25);
    p.domeRadius = sphere(p.radius, h);
    p.felt *= knob("PJ_CYM_FELT", 1);
  } else {
    p.domeTaper = knob("PJ_CYM_RIDETAPER", 0);
  }
  p.lossA *= knob("PJ_CYM_LOSSA", 1);
  p.lossB *= knob("PJ_CYM_LOSSB", 1);
  s.wood = knob("PJ_CYM_WOOD", s.wood);
  s.bellGain *= knob("PJ_CYM_BELL", 1);
  s.choke = knob("PJ_CYM_CHOKE", s.choke);
  s.press = knob("PJ_CYM_PRESS", s.press);
  s.taper = knob("PJ_CYM_THICK", s.taper);
  s.gain = knob("PJ_CYM_GAIN", 1);
  return s;
}

Cymbal::Cymbal(double fs, const CymbalSpec& spec, uint32_t seed)
    : s_(spec), fs_(fs), dt_(1 / fs), plate_(fs, spec.plate) {
  // Each mode's loss times a correction by pitch (log-interpolated), fitted
  // to the band decays (the ride's to FluidR3's recorded ride).
  if (s_.nFix > 0) {
    static const double kFix = knob("PJ_CYM_FIX", 1);
    for (int k = 0; k < plate_.modes(); ++k) {
      const double lf = std::log(plate_.frequency(k));
      double c = s_.fixMul[0];
      if (lf >= std::log(s_.fixF[s_.nFix - 1])) c = s_.fixMul[s_.nFix - 1];
      else
        for (int i = 0; i + 1 < s_.nFix; ++i)
          if (lf >= std::log(s_.fixF[i]) && lf < std::log(s_.fixF[i + 1])) {
            const double a = (lf - std::log(s_.fixF[i])) / (std::log(s_.fixF[i + 1]) - std::log(s_.fixF[i]));
            c = std::exp((1 - a) * std::log(s_.fixMul[i]) + a * std::log(s_.fixMul[i + 1]));
          }
      plate_.setLoss(k, plate_.loss(k) * std::pow(c, kFix));
    }
    plate_.retune();
  }
  // The wire ends feel the modes below ~12 kHz (above, the plate's own; most
  // of the cost). (7 kHz left a swirl near the bell no brighter than one at
  // the bow: the shimmer it brings out is the modes above that.)
  BrushSpec bs = spec.brush;
  if (bs.modes == 0) {
    int n = 0;
    while (n < plate_.modes() && plate_.frequency(n) < knob("PJ_CYM_BRUSHTOP", 12000)) ++n;
    bs.modes = n;
  }
  // (Brushed, the cymbal is heard at its own level, as a kit's brushed
  // sounds are set against its sticks: the brush's pushes reach the plate
  // PJ_CYM_BRUSHDB louder than the brush feels them. The level is in the
  // stroke, so nothing already ringing changes.) 13 dB: with the fan's
  // friction low (fanLoss 0.4) more of its ends land, and 20 dB left the
  // brush only 4-5 dB under a stick; 13 puts it ~11 dB under.)
  // (A crash, splash or china takes the sweep across its edge at the plate's
  // loudest place: at 13 dB its brushed stroke measured 0-2 dB under the stick's
  // (tools/gmcompare.py kits), where the ride's and the toms' are 8-10 dB under
  // theirs. PJ_CYM_CRASHBRUSHDB 5: ~9 dB under.)
  static const double kRideBrushDb = knob("PJ_CYM_BRUSHDB", 13), kCrashBrushDb = knob("PJ_CYM_CRASHBRUSHDB", 5);
  brushGain_ = float(std::pow(10.0, (spec.ride ? kRideBrushDb : kCrashBrushDb) / 20));
  // (And a swirl, the brush lying on the plate, at its own level against
  // the strokes: PJ_CYM_SWIRLDB more.)
  // (A crash's swirl stays where it was: what its strokes gave up, back.)
  swirlGain_ = float(std::pow(10.0, (knob("PJ_CYM_SWIRLDB", 12) + (spec.ride ? 0 : kRideBrushDb - kCrashBrushDb)) / 20));
  feel_.reset(new Feel(plate_));
  brush_.reset(new Brush(fs, *feel_, bs, seed));
  if (const char* tr = std::getenv("PJ_CYM_TRACE")) trace_ = std::fopen(tr, "wb");
  sPhi_.assign((plate_.modes() + 15) / 16 * 16, 0.0f);
  // The bell's own modes, and the stick's (40 cm hickory, free-free, the hand
  // damping them in ~40 ms).
  for (int k = 0; k < s_.bellN; ++k) {
    const double w = 2 * kPi * s_.bellF[k], d = std::exp(-6.91 / s_.bellT60[k] * dt_);
    bPr_[k] = float(d * std::cos(w * dt_));
    bPi_[k] = float(d * std::sin(w * dt_));
    bW_[k] = float(w);
  }
  const double wf[3] = {324, 892, 1749};
  for (int k = 0; k < 3; ++k) {
    const double w = 2 * kPi * wf[k], d = std::exp(-6.91 / 0.04 * dt_);
    wPr_[k] = float(d * std::cos(w * dt_));
    wPi_[k] = float(d * std::sin(w * dt_));
  }
  // The hand on the edge: each mode by how much it moves there, against the
  // middle mode's (the edge's own modes, many diameters, go first).
  edge_.assign((plate_.modes() + 15) / 16 * 16, 0.0f);
  plate_.ringShare(0.95 * plate_.radius(), edge_.data());
  {
    std::vector<float> sorted(edge_.begin(), edge_.begin() + plate_.modes());
    std::nth_element(sorted.begin(), sorted.begin() + sorted.size() / 2, sorted.end());
    const double mid = std::max(double(sorted[sorted.size() / 2]), 1e-12);
    mid_ = mid;
    // (A gripping hand covers more than one ring: every mode loses at least
    // 0.3 of the middle one's share.)
    for (float& x : edge_) x = float(std::clamp(x / mid, 0.3, 20.0));
  }
  // The microphones (as the hi-hat's): each mode by its shape, summed over
  // the plate with every patch's distance and delay at its pitch; below the
  // coincidence pitch only the edge sounds (the faces cancel).
  int maxM = 0;
  for (int k = 0; k < plate_.modes(); ++k) maxM = std::max(maxM, plate_.diameters(k));
  const int kR = 20, kT = std::clamp(2 * maxM + 16, 64, 256);
  const PlateSpec& ps = s_.plate;
  const double fCoin = kC * kC * std::sqrt(12.0) / (2 * kPi * ps.thickness * std::sqrt(ps.E / (ps.rho * (1 - ps.nu * ps.nu))));
  const int n = plate_.modes();
  std::vector<float> pm(n);
  std::vector<std::complex<double>> g[2];
  g[0].assign(n, 0.0);
  g[1].assign(n, 0.0);
  std::vector<double> kk(n);
  for (int k = 0; k < n; ++k) kk[k] = 2 * kPi * plate_.frequency(k) / kC;
  for (int ir = 0; ir < kR; ++ir)
    for (int it = 0; it < kT; ++it) {
      const double r = plate_.radius() * (ir + 0.5) / kR, th = 2 * kPi * (it + 0.5) / kT;
      const double dA = 2 * kPi * r * (plate_.radius() / kR) / kT;
      plate_.shape(r, th, pm.data());
      for (int mic = 0; mic < 2; ++mic) {
        const double mx = (mic ? 1 : -1) * s_.micX, mz = s_.micZ, d0 = std::hypot(mx, mz);
        const double ddx = r * std::cos(th) - mx, ddy = r * std::sin(th);
        const double d = std::sqrt(ddx * ddx + ddy * ddy + mz * mz);
        const double a = dA / d;
        for (int k = 0; k < n; ++k) {
          const double ph = -kk[k] * (d - d0);
          g[mic][k] += double(pm[k]) * a * std::complex<double>(std::cos(ph), std::sin(ph));
        }
      }
    }
  for (int mic = 0; mic < 2; ++mic) {
    mic_[mic].assign((n + 15) / 16 * 16, 0.0f);
    for (int k = 0; k < n; ++k) {
      const double eff = std::min(1.0, std::sqrt(plate_.frequency(k) / fCoin));
      mic_[mic][k] = float((g[mic][k].real() < 0 ? -1 : 1) * std::abs(g[mic][k]) * kRho / (2 * kPi) * eff);
    }
  }
  press_.assign(edge_.size(), 0.0f);
  {
    const double t = 1 + s_.taper * (1 - 2 * s_.bowAt);
    bowStiff_ = t * t;
  }
  reset();
}

void Cymbal::brushFeel() {
  // Where the brush is: the plate's stiffness under its ends (its thickness
  // there against the bow's: the bow's is the brush's own), and, lying on
  // the plate, its press: each mode's share round the circle it is on (the
  // brush goes round it), the loss by how hard it is pressed.
  const double R = plate_.radius(), r = std::min(brushRadius(), R);
  const double t = 1 + s_.taper * (1 - 2 * r / R);
  brush_->stiffness(t * t / bowStiff_);
  const double load = brush_->load();
  pressRate_ = float(s_.press * load);
  if (load > 0) {
    plate_.ringShare(r, press_.data());
    const float k = float(1 / mid_);
    for (int i = 0, n = plate_.modes(); i < n; ++i) press_[i] = std::min(press_[i] * k, 20.0f);
  }
}

void Cymbal::reset() {
  plate_.reset();
  z_ = vz_ = ax_ = wx_ = ay_ = wy_ = 0;
  stickOn_ = false;
  std::fill(bRe_, bRe_ + 6, 0.0f);
  std::fill(bIm_, bIm_ + 6, 0.0f);
  std::fill(wRe_, wRe_ + 3, 0.0f);
  std::fill(wIm_, wIm_ + 3, 0.0f);
  prev_[0] = prev_[1] = bPrev_ = 0;
  choke_ = 0;
  asleep_ = true;
  quiet_ = 0;
}

void Cymbal::stick(double speed, double r, double theta, bool shoulder) {
  wake();
  r = std::clamp(r, 0.0, 0.99 * plate_.radius());
  sx_ = r * std::cos(theta);
  sy_ = r * std::sin(theta);
  plate_.shape(r, theta, sPhi_.data());
  double g = 0;
  for (int k = 0; k < plate_.modes(); ++k) g += double(sPhi_[k]) * sPhi_[k];
  sm_ = shoulder ? 0.030 : 0.012;   // what of the stick is behind the blow (kg)
  sK_ = shoulder ? 4e8 : 1.2e9;     // hickory on bronze, 4/3 E sqrt(R) (N/m^1.5)
  // Inside the bell's rim the cup takes part of the blow (its own modes).
  sBell_ = r < s_.bellR ? std::pow(std::cos(0.5 * kPi * r / s_.bellR), 2) : 0.0;
  const double m = plate_.mass(), I = m * plate_.radius() * plate_.radius() / 4;
  sG_ = dt_ * dt_ * (g + 1 / m + r * r / I + sBell_ * sBell_ * s_.bellN / s_.bellMass);
  sz_ = z_ + ax_ * sx_ + ay_ * sy_ + plate_.height(sPhi_.data()) + 0.05e-3;
  sv_ = -speed;
  stickOn_ = true;
  sAge_ = 0;
}

float Cymbal::sample() {
  if (asleep_) {
    if (!stickOn_ && brush_->up() && choke_ <= 0) { side_ = 0; return 0; }
    asleep_ = false;
    quiet_ = 0;
  }
  const double m = plate_.mass(), I = m * plate_.radius() * plate_.radius() / 4;
  double fz = -s_.seatK * z_ - 2 * s_.seatLoss * std::sqrt(s_.seatK * m) * vz_;
  double tx = -s_.tiltK * ax_ - 2 * s_.tiltLoss * std::sqrt(s_.tiltK * I) * wx_;
  double ty = -s_.tiltK * ay_ - 2 * s_.tiltLoss * std::sqrt(s_.tiltK * I) * wy_;
  // The stick: its tip against the plate where it is now, the force found
  // for the end of the sample (a hard contact: the plate gives under it).
  float F = 0;
  if (stickOn_) {
    const double h = z_ + ax_ * sx_ + ay_ * sy_ + plate_.height(sPhi_.data());
    const double hv = vz_ + wx_ * sx_ + wy_ * sy_ + plate_.speed(sPhi_.data());
    const double free = (h - sz_) + dt_ * (hv - sv_), G = sG_ + dt_ * dt_ / sm_;
    double f = 0;
    if (free > 0) {
      double lo = 0, hi = free;
      for (int it = 0; it < 30; ++it) {
        const double x = 0.5 * (lo + hi);
        if (x + G * sK_ * x * std::sqrt(x) > free) hi = x; else lo = x;
      }
      const double x = 0.5 * (lo + hi);
      f = sK_ * x * std::sqrt(x);
    }
    sv_ += dt_ * f / sm_;
    sz_ += dt_ * sv_;
    if (f > 0) {
      plate_.push(sPhi_.data(), float(-f));
      fz -= f; tx -= f * sx_; ty -= f * sy_;
      // The bell's modes (velocity phasors, unit... their mass) and the stick's own.
      const float db = float(sBell_ * f * dt_ / s_.bellMass);
      for (int k = 0; k < s_.bellN; ++k) bRe_[k] -= db;
      for (int k = 0; k < 3; ++k) wRe_[k] += float(f * dt_ / 0.01);
      F = float(f);
    }
    if (++sAge_ > int(0.02 * fs_) || (f == 0 && sv_ > 0)) stickOn_ = false;
  }
  if (!brush_->up() && --pressAge_ <= 0) { brushFeel(); pressAge_ = 32; }
  brush_->step();
  const float bg = brush_->load() > 0 ? brushGain_ * swirlGain_ : brushGain_;
  fz -= bg * brush_->force();
  if (!brush_->up()) {
    // Inside the bell's rim (and the fan's half-length round it) the cup
    // feels the wire ends too: its few clear modes, the glassy ring of a
    // swirl drawn near the bell.
    const double r = brushRadius(), bw = s_.bellR + 0.03;
    if (r < bw) {
      const double c = std::cos(0.5 * kPi * r / bw);
      const float db = float(c * c * bg * brush_->force() * dt_ / s_.bellMass);
      for (int k = 0; k < s_.bellN; ++k) bRe_[k] -= db;
    }
    if (pressRate_ > 0) plate_.rub(press_.data(), pressRate_);
  } else {
    pressRate_ = 0;
  }
  if (!brush_->up() || feel_->dirty) {
    float* f = plate_.force();
    float* g = feel_->f.data();
    for (int k = 0, n = plate_.modes(); k < n; ++k) { f[k] += bg * g[k]; g[k] = 0; }
    feel_->dirty = !brush_->up();
  }
  if (trace_) {
    const float bf = brush_->force();
    std::fwrite(&bf, sizeof bf, 1, trace_);
  }
  if (choke_ > 0) plate_.rub(edge_.data(), float(choke_ * s_.choke));
  vz_ += dt_ * fz / m; z_ += dt_ * vz_;
  wx_ += dt_ * tx / I; ax_ += dt_ * wx_;
  wy_ += dt_ * ty / I; ay_ += dt_ * wy_;
  plate_.step();
  // The bell's and the stick's modes turn.
  float bell = 0, wood = 0;
  for (int k = 0; k < s_.bellN; ++k) {
    const float re = bRe_[k], im = bIm_[k];
    bRe_[k] = bPr_[k] * re - bPi_[k] * im;
    bIm_[k] = bPi_[k] * re + bPr_[k] * im;
    bell += bRe_[k];
  }
  for (int k = 0; k < 3; ++k) {
    const float re = wRe_[k], im = wIm_[k];
    wRe_[k] = wPr_[k] * re - wPi_[k] * im;
    wIm_[k] = wPi_[k] * re + wPr_[k] * im;
    wood += wRe_[k];
  }
  (void)F;
  // The microphones: the plate's modes by their weights (pressure from the
  // surface's acceleration); the cup as a small piston; the stick's ring.
  const float* v = plate_.v();
  const int n = plate_.modes();
  const float rMic = float(std::hypot(s_.micX, s_.micZ));
  const float cupArea = float(kPi * s_.bellR * s_.bellR);
  const float bellP = float(s_.bellGain * kRho * cupArea / (2 * kPi * rMic)) * (bell - bPrev_) * float(fs_);
  bPrev_ = bell;
  const float woodP = float(s_.wood) * wood;
  float y[2];
  for (int mic = 0; mic < 2; ++mic) {
    float s = 0;
    const float* w = mic_[mic].data();
    for (int k = 0; k < n; ++k) s += w[k] * v[k];
    y[mic] = (s - prev_[mic]) * float(fs_) + bellP + woodP;
    prev_[mic] = s;
  }
  const float gn = float(s_.gain);
  if (keepGroups_) {
    if (micMono_.size() != mic_[0].size()) {
      micMono_.resize(mic_[0].size());
      for (size_t k = 0; k < micMono_.size(); ++k) micMono_[k] = 0.5f * (mic_[0][k] + mic_[1][k]);
    }
    float acc[kGroups] = {};
    const float* w = micMono_.data();
    int k = 0;
    for (; k + kGroups <= n; k += kGroups)
      for (int j = 0; j < kGroups; ++j) acc[j] += w[k + j] * v[k + j];
    for (; k < n; ++k) acc[k % kGroups] += w[k] * v[k];
    for (int j = 0; j < kGroups; ++j) {
      group_[j] = gn * (acc[j] - gPrev_[j]) * float(fs_);
      gPrev_[j] = acc[j];
    }
  }
  const float l = gn * y[0], r = gn * y[1];
  side_ = 0.5f * (l - r);
  const float mono = 0.5f * (l + r);
  if (!std::isfinite(mono) || !std::isfinite(z_)) { reset(); return 0; }
  if (std::fabs(mono) < 1e-6f && !stickOn_ && brush_->up()) {
    if (++quiet_ > int(0.3 * fs_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return mono;
}

CymbalModel::CymbalModel(const Instrument& inst)
    : Tube(inst), which_(std::max(0, whichOf(inst))), cym_(inst.fs, cymbalSpec(which_), 31u + which_) {
  // (PJ_CYM_PARTS=0: the cymbal as one point in the room)
  if (knob("PJ_CYM_PARTS", 1) == 0) return;
  cym_.keepGroups(true);
  const double R = cym_.plate().radius();
  uint32_t h = 977u + 131u * uint32_t(which_);
  auto rnd = [&h]() { h = h * 1664525u + 1013904223u; return (h >> 8) / 16777216.0 * 2 - 1; };
  for (int j = 0; j < Cymbal::kGroups; ++j) {
    // (round the plate, 0.6 of its radius out, the plate about level)
    const double a = 2 * M_PI * j / Cymbal::kGroups;
    partPlace_.insert(partPlace_.end(), {float(0.6 * R * std::cos(a)), float(0.6 * R * std::sin(a)), 0.0f});
    // (its lobe: out round the cymbal on a bearing of its own, a little raised, full strength:
    // each group throws its highs towards its own part of the room)
    const double b = a + 0.3 * rnd();
    double d[3] = {std::cos(b), std::sin(b), 0.35};
    const double nn = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    partLobe_.insert(partLobe_.end(), {float(d[0] / nn), float(d[1] / nn), float(d[2] / nn), 1.0f});
  }
  partPlace_.insert(partPlace_.end(), {0.0f, 0.0f, 0.0f});
  partLobe_.insert(partLobe_.end(), {0.0f, 0.0f, 1.0f, 0.0f});
  part_.resize(Cymbal::kGroups + 1);
  std::fill(aimW_, aimW_ + Cymbal::kGroups, 1.0f);
  std::fill(aimTo_, aimTo_ + Cymbal::kGroups, 1.0f);
}

const float* CymbalModel::takePart(int n, int j) {
  if (part_.empty()) return Tube::takePart(n, j);
  if (j < 0 || j >= int(part_.size())) return nullptr;
  std::vector<float>& p = part_[j];
  partOut_.assign(n, 0.0f);
  const int have = int(p.size()), m = std::min(n, have);
  std::copy(p.end() - m, p.end(), partOut_.end() - m);
  p.clear();
  return partOut_.data();
}

void CymbalModel::resetAir() {
  Tube::resetAir();
  cym_.reset();
}

float CymbalModel::sample() {
  static const float g = float(knob("PJ_CYM_OUT", 1.0 / 3));
  // The highs 3 dB down (from ~4 kHz up: a first-order shelf at 3 kHz, PJ_CYM_HIGHS -4.3 dB: -3 dB over 6-14 kHz): the cymbals heard
  // from a little further off, not from over the drummer's shoulder (by ear, the user's).
  static const float kHi = float(std::pow(10.0, knob("PJ_CYM_HIGHS", -4.3) / 20) - 1);
  static const float kA = float(1 - std::exp(-2 * M_PI * 3000 / fs_));
  const float x = cym_.sample(), sd = cym_.side();
  shelfM_ += kA * (x - shelfM_);
  shelfS_ += kA * (sd - shelfS_);
  last_ = g * (x + kHi * (x - shelfM_));
  side_.push_back(g * float(outScale()) * (sd + kHi * (sd - shelfS_)));
  const float out = finish(last_);
  if (!part_.empty()) {
    if (part_[0].size() >= 1 << 15)  // (nobody takes them: played without a room)
      for (auto& p : part_) p.clear();
    // A brush lying on the plate (the swirl): its hiss is made under the wires and is loudest
    // off the plate there, before it has gone round and into the whole. The groups' highs
    // (over ~3 kHz) are dealt towards the parts on the brush's side, more the further out it
    // lies, their power kept: as the hand goes along its arc and wanders in and out, the shine
    // leaves the plate from another side, and each wall gets another share of it.
    // (PJ_CYM_SWIRLAIM: how much, 0: none; PJ_CYM_SWIRLBEAM: how narrowly.)
    static const float kAim = float(knob("PJ_CYM_SWIRLAIM", 0.65)), kBeam = float(knob("PJ_CYM_SWIRLBEAM", 2.0));
    if (--aimAge_ <= 0) {
      aimAge_ = 64;
      const Brush& br = cym_.brush();
      const double bx = br.x(), by = br.y(), rr = std::hypot(bx, by);
      const double d = kAim > 0 && br.load() > 0 ? kAim * std::min(1.0, 1.4 * rr / cym_.plate().radius()) : 0;
      // The cymbal rocks on its tilter after each stroke (~1.5 Hz, dying over ~15 s, longer than
      // it sounds): its highs leave the plate in narrow lobes (a few degrees wide at 10 kHz on a
      // plate this size), so even its small sway sweeps them across the room. Each group's highs
      // swell and fade with the tilt through its own lobe pattern, one lobe every 0.5 degrees
      // (PJ_CYM_ROCK: how deep, 0: not; PJ_CYM_ROCKDEG: the lobes' spacing: narrower than the
      // plate's own at 10 kHz, ~3 degrees, so that the model's small sway, ~0.1 degree under a
      // ride pattern, carries the shine round the room as a real cymbal's wider one does).
      static const double kRock = knob("PJ_CYM_ROCK", 0.6), kLobe = 2 * M_PI / (knob("PJ_CYM_ROCKDEG", 0.5) * M_PI / 180);
      const double tx = cym_.tilt(0), ty = cym_.tilt(1);
      double w[Cymbal::kGroups], p2 = 0;
      for (int j = 0; j < Cymbal::kGroups; ++j) {
        const double a = 2 * M_PI * j / Cymbal::kGroups;
        const double c = rr > 0 ? (bx * std::cos(a) + by * std::sin(a)) / rr : 0;
        w[j] = 1 - d + d * std::exp(kBeam * (c - 1));
        if (kRock > 0) {
          uint32_t hs = 2654435761u * uint32_t(j + 1 + 17 * which_);
          hs ^= hs >> 15;
          const double ua = (hs & 1023) / 1023.0, ub = ((hs >> 10) & 1023) / 1023.0, uc = ((hs >> 20) & 1023) / 1023.0;
          const double dir = 2 * M_PI * ua, k = kLobe * (0.7 + 0.6 * ub);
          w[j] *= 1 + kRock * std::sin(k * (std::cos(dir) * tx + std::sin(dir) * ty) + 2 * M_PI * uc);
        }
        p2 += w[j] * w[j];
      }
      const double nrm = std::sqrt(Cymbal::kGroups / p2);
      for (int j = 0; j < Cymbal::kGroups; ++j) aimTo_[j] = float(w[j] * nrm);
    }
    float sum = 0;
    for (int j = 0; j < Cymbal::kGroups; ++j) {
      const float gx = cym_.group(j);
      gShelf_[j] += kA * (gx - gShelf_[j]);
      aimW_[j] += 0.002f * (aimTo_[j] - aimW_[j]);  // (~10 ms)
      const float hi = g * float(outScale()) * (1 + kHi) * (gx - gShelf_[j]);
      const float b = g * float(outScale()) * (gx + kHi * (gx - gShelf_[j]));
      part_[j].push_back(b + (aimW_[j] - 1) * hi);
      sum += b;
    }
    part_[Cymbal::kGroups].push_back(out - sum);
  }
  return out;
}

CymbalPlayer::CymbalPlayer(CymbalModel& m) : Performer(m), m_(m) {
  home_ = bow_ = bowTo_ = m_.cymbal().spec().bowAt;
  held_ = 10;
}

double CymbalPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

double CymbalPlayer::stickSpeed(int velocity, double dynamics) {
  // ~ velocity^1.9: General MIDI's ~30 dB from 16 to 127 (the plate is linear
  // in the stick's speed; a faster stick is also brighter): 0.3 m/s (a
  // whisper on the bow) .. 5 m/s (a crash).
  static const double top = knob("PJ_CYM_VMAX", 5.0), ex = knob("PJ_CYM_VEXP", 1.9);
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return (0.06 + top * std::pow(v, ex)) * (0.7 + 0.6 * std::clamp(dynamics, 0.0, 1.0));
}

double CymbalPlayer::bowPlace() {
  // The hand's place on the bow drifts by phrases (stroke_placement.md 7.2):
  // held for 8 .. 16 strokes, then it moves (over a few strokes), two times
  // in three to the other side of home, by up to a tenth of the radius.
  if (--held_ <= 0) {
    held_ = 8 + int(9 * ur());
    if (ur() < 0.65) side_ = -side_;
    bowTo_ = home_ + side_ * (0.03 + 0.07 * ur());
  }
  bow_ += 0.35 * (bowTo_ - bow_);
  // ... and between those moves it wanders a little, stroke by stroke (a
  // slow random walk, held near its place), and each stroke lands within a
  // few mm of it: never the same spot twice.
  wander_ = 0.85 * wander_ + 0.012 * (ur() - 0.5);
  return bow_ + wander_ + 0.02 * (ur() - 0.5);
}

double CymbalPlayer::angle() {
  // The side facing the drummer: wandering a few degrees, each stroke a
  // degree or two off that.
  thWander_ = 0.9 * thWander_ + 0.03 * (ur() - 0.5);
  return 2 * kPi * 0.1 + thWander_ + 0.04 * (ur() - 0.5);
}

double CymbalPlayer::lean(int velocity) {
  // How much the stroke is leaned on, 0 .. 1: the velocity, and, in a song,
  // where it falls in the bar: 1 and 3 leaned on, 2 and 4 light, what is
  // between them in between. Where the bar is, the ride's own pattern says
  // (a song may begin with a pickup): its quarters fall on the most common
  // quarter-beat phase, and 2 and 4 are the beats a skip note leads into.
  // Until it has seen enough, 4/4 from the song's beat 0.
  double w = std::clamp((velocity - 20) / 90.0, 0.0, 1.0);
  const SongClock& clk = SongClock::get();
  static const bool kBeat = knob("PJ_CYM_BEAT", 1) > 0;
  if (!clk.valid || !kBeat) return w;
  const double q = clk.beat;
  phaseCount_[int(std::lround(q * 4)) & 3] += 1;
  int ph = 0;
  for (int i = 1; i < 4; ++i)
    if (phaseCount_[i] > phaseCount_[ph]) ph = i;
  const double pos = q - ph / 4.0;                 // beats, the quarters on whole numbers
  const long n = std::lround(pos);
  const bool onBeat = std::fabs(pos - n) < 0.2;
  if (onBeat) {
    const double gap = q - lastBeat_;               // a skip note just before: 1/4 .. 1/2 a beat
    const bool skip = gap > 0.15 && gap < 0.55;
    beatCount_[n & 1] += 1;
    skipCount_[n & 1] += skip ? 1 : 0;
  }
  lastBeat_ = q;
  // Which parity is 2 and 4: the one more often led into by a skip note
  // (enough seen), else the odd beats (counting from 0).
  int two = 1;
  if (beatCount_[0] + beatCount_[1] >= 8) {
    const double r0 = skipCount_[0] / std::max(1.0, beatCount_[0]), r1 = skipCount_[1] / std::max(1.0, beatCount_[1]);
    if (std::fabs(r0 - r1) > 0.15) two = r0 > r1 ? 0 : 1;
  }
  const double beat = !onBeat ? 0.5 : (n & 1) == two ? 0.0 : 1.0;
  return 0.5 * w + 0.5 * beat;
}

void CymbalPlayer::noteOn(int midi, int velocity) {
  static const double kEdge = knob("PJ_CYM_EDGE", 0.9), kBrushV = knob("PJ_CYM_BRUSHV", 1.5);
  static const bool kShoulder = knob("PJ_CYM_SHOULDER", 1) > 0;
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  Cymbal& c = m_.cymbal();
  if (midi == 84) {  // the swirl, held
    static const double kLoad = knob("PJ_CYM_SWIRLLOAD", 1.0);
    swLoad_ = kLoad * (0.15 + 0.85 * std::pow(std::clamp(velocity, 1, 127) / 127.0, 1.3)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0));
    if (!swirl_) {
      swirl_ = true;
      swirling_ = false;
      swPause_ = 0;
      swR_ = swRTo_ = c.spec().ride ? c.spec().bowAt + 0.05 : 0.7;
      swHold_ = 1;
      swTh_ = 2 * kPi * (0.1 + 0.05 * (ur() - 0.5));
      swThTo_ = swThHome_ = swTh_;
      swAim_ = swNow_ = Turn();
    }
    c.wake();
    return;
  }
  // (A stroke while the swirl is held: the hand leaves the plate for it and
  // comes back to swirl on.)
  if (swirl_) {
    swPause_ = 0.25;
    swirling_ = false;
    c.brush().rest(0);
  }
  static const bool allBrush = knob("PJ_CYM_BRUSH", 0) > 0;
  const bool brush = allBrush || midi >= 73;
  const int k = midi >= 73 ? midi - 24 : midi;
  const CymbalSpec& s = c.spec();
  const double R = c.plate().radius(), speed = stickSpeed(velocity, dyn_) * (1 + 0.05 * (ur() - 0.5));
  const double th = 2 * kPi * (0.1 + 0.08 * (ur() - 0.5));  // the side of the cymbal facing the drummer
  noteChoke_ = 0;
  if (brush) {
    // A brush: a tap that lands and is drawn on a little (on a crash, a sweep
    // across the edge): quiet, no ping. (A hand with a brush: at most 3 m/s.)
    const bool bell = k == 53 && s.ride;
    const double tb = s.ride ? angle() : th;
    const double r = bell ? 0.6 * s.bellR : s.ride ? bowPlace() * R : (0.82 + 0.06 * (ur() - 0.5)) * R;
    const double x = r * std::cos(tb), y = r * std::sin(tb);
    c.brush().jump(x, y, tb + 0.3);
    if (s.ride) {
      // On the ride, how the brush comes down follows how much the stroke is
      // leaned on (lean(): the velocity and, in a song, 1 and 3 against 2
      // and 4), continuously: a light stroke is slowed before it lands, only
      // the tip comes down (a share of the wire ends, on a smaller
      // footprint) and lifts at once: brief, local, brighter, less damping;
      // a leaned-on one is thrown, the whole fan lands and follows through.
      // In a song 1 and 3 are also a little louder, 2 and 4 a little softer.
      // The hand's speed at the cymbal: its own law (a brush is not thrown as
      // a stick is), ~velocity^1.3, 0.45 .. 2.65 m/s before the slowing.
      const double w = lean(velocity);
      static const double kBv = knob("PJ_CYM_BRUSHVMAX", 2.2), kBe = knob("PJ_CYM_BRUSHVEXP", 1.3);
      double v = (0.45 + kBv * std::pow(std::clamp(velocity, 1, 127) / 127.0, kBe)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) *
                 (1 + 0.05 * (ur() - 0.5));
      static const bool kBeat = knob("PJ_CYM_BEAT", 1) > 0;
      if (SongClock::get().valid && kBeat) v *= 0.8 + 0.4 * w;
      // (The fan lies flat: its ends land over 6-9 mm of height; the slowed tip
      // lands over ~2 mm and lifts at once. The hand lets the fan through by
      // ~0.7 of that height (PJ_CYM_FOLLOW), so the ends keep landing one by
      // one; with the fan's friction low (fanLoss above) they do not run
      // together into one push.)
      const double share = 0.4 + 0.6 * w;
      static const double kFollow = knob("PJ_CYM_FOLLOW", 0.7), kSpread = knob("PJ_CYM_SPREAD", 1);
      const double spread = kSpread * (2 + (4 + 3 * ur()) * w) * 1e-3, follow = kFollow * spread * w;
      c.brush().strike(v * (0.6 + 0.4 * w), spread, follow, 0.0, share);
      c.brush().place(x - 0.01 * w * std::sin(tb), y + 0.01 * w * std::cos(tb), tb + 0.3);
    } else {
      c.brush().strike(std::min(kBrushV * speed, 3.0), (6 + 3 * ur()) * 1e-3, (3 + 2 * ur()) * 1e-3, 0.03);
      c.brush().place(x - 0.05 * std::sin(th), y + 0.05 * std::cos(th), th + 0.3);
    }
    c.wake();
    return;
  }
  if (s.ride) {
    if (k == 53) {
      c.stick(speed, (0.45 + 0.15 * ur()) * s.bellR, th, false);           // the bell, with the tip
    } else if (k == 51 || k == 59) {
      if (velocity >= 112) c.stick(0.9 * speed, (bowPlace() + 0.15) * R, th, true);  // an accent: the shoulder
      else c.stick(speed, bowPlace() * R, th, false);                            // the bow, with the tip
    } else if (k == 49 || k == 57 || k == 55 || k == 52) {
      c.stick(speed, 0.97 * R, th, true);                                    // crashed
    } else {
      // Live: the keys from the edge (low) to the bell (high).
      const double u = std::clamp((k - 36) / 48.0, 0.0, 1.0);
      c.stick(speed, (0.97 - 0.9 * u) * R, th, false);
    }
  } else {
    if (k == 53) c.stick(speed, 0.5 * s.bellR, th, false);
    else if (velocity < 48) c.stick(speed, (0.9 + 0.04 * (ur() - 0.5)) * R, th, false);  // soft: the tip near the edge
    else c.stick(speed, (kEdge + 0.02 * (ur() - 0.5)) * R, th, kShoulder);               // the shoulder on the edge
  }
}

void CymbalPlayer::swirl(double dt) {
  Cymbal& c = m_.cymbal();
  Brush& b = c.brush();
  if (!swirl_) {
    if (swirling_) { b.rest(0); swirling_ = false; }
    return;
  }
  if (swPause_ > 0) { swPause_ -= dt; return; }
  const CymbalSpec& s = c.spec();
  const double R = c.plate().radius();
  // A turn every two beats (one a beat when slow), as the snare's left hand.
  const SongClock& clk = SongClock::get();
  const double B = clk.valid ? clk.beatLen : 0.6;
  const double P = 2 * B > 2.6 ? B : 2 * B < 0.8 ? 4 * B : 2 * B;
  static const double kVary = knob("PJ_CYM_SWIRLVARY", 0.6);
  // (the hand hurries through one part of the turn and eases round the rest, each turn its own)
  swPhase_ += 2 * kPi * dt / P * (1 + swNow_.pull * std::cos(swPhase_ - swNow_.at));
  if (swPhase_ >= 2 * kPi) {
    swPhase_ -= 2 * kPi;
    // The loops' middle wanders: held two to four turns, then moved, more
    // often than not to the other side: out towards the edge (the wash, a
    // lower roar), in towards the bell (higher, glassier).
    if (--swHold_ <= 0) {
      swHold_ = 2 + int(3 * ur());
      if (ur() < 0.65) swSide_ = -swSide_;
      const double home = s.ride ? s.bowAt : 0.62;
      swRTo_ = std::clamp(home + swSide_ * (0.12 + 0.12 * ur()), 0.3, 0.86);
    }
    // No two turns alike: each has its own arc (short dabs, long strokes round a third of the
    // plate), its own width across the grooves (a line along them, or an open loop), weight, pace
    // and slant; now and then (every sixth turn or so) one long light sweep. The arc's middle
    // goes round the plate with them, within the arm's reach of where it began.
    ++swTurn_;
    const double v = kVary;
    const bool sweep = v > 0 && swTurn_ % 6 == 5 && ur() < 0.6 * v;
    swAim_.arc = sweep ? 1.9 : 1 + v * (ur() < 0.3 ? -0.45 * ur() : 0.9 * (ur() - 0.4));
    swAim_.wid = sweep ? 0.6 : 1 + v * 1.6 * (ur() * ur() - 0.33);
    swAim_.load = sweep ? 0.9 : 1 - v * 0.2 * ur();  // (never heavier than the plain turn)
    swAim_.pull = v * 0.45 * ur();
    swAim_.at = 2 * kPi * ur();
    swAim_.slant = v * 0.5 * (ur() - 0.5);
    static const double kRound = knob("PJ_CYM_SWIRLROUND", 0.6);
    swThTo_ = std::clamp(swThTo_ + kRound * (0.15 + 0.75 * v * ur()) * (ur() < 0.5 ? -1 : 1) * (ur() < 0.45 ? 1.0 : 0.25),
                         swThHome_ - 1.1, swThHome_ + 1.1);
  }
  static const double kAt = knob("PJ_CYM_SWIRLAT", 0);  // (the loops' middle held there, a share of the radius; 0: wandering)
  if (kAt > 0) swRTo_ = kAt;
  swR_ += (swRTo_ - swR_) * std::min(1.0, dt / P);
  {
    const double e = std::min(1.0, 3 * dt / P);  // (a third of a turn to get there)
    swNow_.arc += (swAim_.arc - swNow_.arc) * e; swNow_.wid += (swAim_.wid - swNow_.wid) * e;
    swNow_.load += (swAim_.load - swNow_.load) * e; swNow_.pull += (swAim_.pull - swNow_.pull) * e;
    swNow_.slant += (swAim_.slant - swNow_.slant) * e;
    swNow_.at = swAim_.at;
    swTh_ += (swThTo_ - swTh_) * std::min(1.0, 1.5 * dt / P);
  }
  // The loop: long along the grooves (an arc of ~70 degrees), narrow across
  // them, the fan's wires pointing in towards the bell.
  const double ph = swPhase_;
  const double r = std::clamp(swR_ + 0.05 * swNow_.wid * std::cos(ph), 0.22, 0.93) * R, th = swTh_ + 0.6 * swNow_.arc * std::sin(ph);
  const double x = r * std::cos(th), y = r * std::sin(th), ang = th + kPi + 0.3 * std::cos(ph) + swNow_.slant;
  // The press eases and leans: a little more weight as the hand comes back
  // round towards the drummer (on the beat).
  // (Lighter in towards the bell, where a pressed brush sounds pinched;
  // leaning in out at the edge, for its roar.)
  // (The variety is in the hand's way, not in the loudness: a turn that takes the brush faster
  // over the plate than the plain one (a longer arc, a wider loop, the hurried part) is that much
  // lighter, PJ_CYM_SWIRLEVEN: its power; no turn is louder than the plain swirl, some a little
  // under it.)
  static const double kEven = knob("PJ_CYM_SWIRLEVEN", 0.5);
  const double quick = std::hypot(0.6 * swNow_.arc * swR_, 0.05 * swNow_.wid) / std::hypot(0.6 * swR_, 0.05) *
                       (1 + swNow_.pull * std::cos(ph - swNow_.at));
  // (... and the whole a little lighter, PJ_CYM_SWIRLUNDER x the variety: it stays just under the plain swirl)
  static const double kUnder = knob("PJ_CYM_SWIRLUNDER", 0.4);
  const double even = (quick > 1 ? std::pow(quick, -kEven) : 1.0) * (1 - kUnder * kVary);
  const double load = swLoad_ * (0.8 + 0.4 * std::pow(0.5 + 0.5 * std::sin(ph), 2)) * (0.35 + 0.8 * std::min(r / R, 1.0)) * swNow_.load * even;
  static const double kShare = knob("PJ_CYM_SWIRLFAN", 0.25);
  if (!swirling_ || b.up()) {
    b.jump(x, y, ang);
    swirling_ = true;
  }
  b.place(x, y, ang);
  b.rest(load, kShare);
  c.wake();
}

void CymbalPlayer::noteOff(int midi) {
  if (midi == 84) { swirl_ = false; return; }
  static const bool chokes = knob("PJ_CYM_NOTEOFF", 0) > 0;
  if (chokes) noteChoke_ = 1;
}

void CymbalPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  Cymbal& c = m_.cymbal();
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    // The hand on the edge: the breath (pressure) grabs it; a choking
    // note-off holds on until the next stroke.
    const double want = std::max(std::clamp((breathCtl_ - 0.15) / 0.35, 0.0, 1.0), noteChoke_ > 0 ? 1.0 : 0.0);
    choked_ += (want - choked_) * (1 - std::exp(-m / fs_ / 0.01));
    c.choke(choked_ > 1e-3 ? choked_ : 0.0);
    swirl(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !c.asleep();
}

}  // namespace pajupilli
