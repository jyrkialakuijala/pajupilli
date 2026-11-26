#include "scrapers.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
constexpr double kRho = 1.2;

// The guiro (research/scrapers_friction.md 1): notches 4 mm apart, 0.8 mm
// deep, each a gentle climb over three quarters of the spacing and a steep
// face; the tip (2 g) on the stick held by the hand's spring.
constexpr double kNotch = 0.004, kDepth = 0.0008, kClimb = 0.75;
// The notched length (m), x from the wide bottom (0) to the narrow top; the
// notches 5 mm apart at the bottom, 3 mm at the top (a linear taper).
constexpr double kGuiroL = 0.20, kS0 = 0.005, kS1 = 0.003;
constexpr double kTipM = 0.002, kHandK = 400, kTipDamp = 450;  // (the grip damps the stick: near critical)
constexpr double kContactK = 3e6, kContactLoss = 1.0;

// The surface's height at x and its slope: notches whose spacing tapers from
// kS0 to kS1 along the length (the count of notches below x is
// L / (kS1 - kS0) ln(s(x) / kS0)); idx: which notch.
double notch(double x, double* slope, int* idx = nullptr) {
  x = std::clamp(x, 0.0, kGuiroL);
  const double sp = kS0 + (kS1 - kS0) * x / kGuiroL;
  const double n = kGuiroL / (kS1 - kS0) * std::log(sp / kS0);
  double u = n - std::floor(n);
  if (idx) *idx = int(std::floor(n));
  if (u < kClimb) {
    *slope = kDepth / (kClimb * sp);
    return kDepth * u / kClimb;
  }
  *slope = -kDepth / ((1 - kClimb) * sp);
  return kDepth * (1 - (u - kClimb) / (1 - kClimb));
}

// The cuica's head (2): goat skin 0.3 mm (0.39 kg/m^2), 4470 N/m (Statsenko
// et al.'s model), 18 cm across, the stick (4 g) at its centre. Its (0,n)
// modes at the centre, each with the stick's mass on it.
constexpr double kCuicaR = 0.09, kCuicaSigma = 0.39, kCuicaT = 4470, kStickM = 0.004;
constexpr double kJ0[6] = {2.4048, 5.5201, 8.6537, 11.7915, 14.9309, 18.0711};
constexpr double kJ1at[6] = {0.5191, -0.3403, 0.2715, -0.2325, 0.2065, -0.1877};  // J1(j0n)
constexpr double kHeadT60[6] = {0.45, 0.2, 0.14, 0.1, 0.08, 0.06};
constexpr double kMuS = 0.6, kMuD = 0.3, kMuV = 0.05;  // a damp cloth on bamboo
}  // namespace

bool scraper(const Instrument& inst) {
  return std::strcmp(inst.name, "guiro") == 0 || std::strcmp(inst.name, "cuica") == 0 ||
         std::strcmp(inst.name, "vibraslap") == 0 || std::strcmp(inst.name, "pea-whistle") == 0;
}

void Ring::set(double fs, double hz, double q, double gain) {
  const double r = std::exp(-kPi * hz / (q * fs)), w = 2 * kPi * hz / fs;
  cr = float(r * std::cos(w));
  ci = float(r * std::sin(w));
  g = float(gain);
  re = im = 0;
}

ScraperModel::ScraperModel(const Instrument& inst) : Tube(inst), dt_(1 / inst.fs) {
  const char* n = inst.name;
  kind_ = std::strcmp(n, "guiro") == 0 ? Kind::kGuiro
          : std::strcmp(n, "cuica") == 0 ? Kind::kCuica
          : std::strcmp(n, "vibraslap") == 0 ? Kind::kVibraslap
                                             : Kind::kWhistle;
  const double fs = inst.fs;
  // The gourd: its cavity (a Helmholtz resonance) and the shell's few
  // resonances (thin, lossy); the tip's own click above them.
  const double body[6][3] = {{330, 12, 1.0}, {1150, 10, 0.55}, {1850, 12, 0.6}, {2600, 14, 0.7}, {3700, 16, 0.5}, {5200, 18, 0.35}};
  for (const auto& b : body) {
    gBody_.emplace_back();
    gBody_.back().set(fs, b[0], b[1], b[2]);
  }
  gClick_.set(fs, 6000, 2, 1);
  // Each notch's ring: from ~0.9 kHz at the wide bottom to ~2.4 kHz at the
  // narrow top, each a little apart (a gourd is not even).
  {
    double sl;
    int nTop = 0;
    notch(kGuiroL, &sl, &nTop);
    uint32_t r = 4242u;
    for (int i = 0; i <= nTop + 1; ++i) {
      r = r * 1664525u + 1013904223u;
      const double a = std::min(1.0, double(i) / std::max(1, nTop));
      gNotchHz_.push_back(900 * fastPow(2400.0 / 900, a) * (1 + 0.06 * ((r >> 8) / 16777216.0 - 0.5)));
    }
  }
  // The cuica's head.
  for (int k = 0; k < kHead; ++k) {
    const double J1 = kJ1at[k];
    const double mh = kCuicaSigma * kPi * kCuicaR * kCuicaR * J1 * J1;  // its mass at the centre
    const double f0 = kJ0[k] / (2 * kPi * kCuicaR) * std::sqrt(kCuicaT / kCuicaSigma);
    hm_[k] = mh + kStickM;
    hw0_[k] = 2 * kPi * f0 * std::sqrt(mh / hm_[k]);  // the stick's mass lowers it
    hg_[k] = 6.91 / kHeadT60[k];
    hvol_[k] = 2 * kPi * kCuicaR * kCuicaR * J1 / kJ0[k];  // the volume it sweeps per m at the centre
  }
  // The vibraslap's box (wood) and pins (steel).
  const double box[3][3] = {{650, 8, 1.0}, {1350, 10, 0.8}, {2300, 12, 0.6}};
  for (const auto& b : box) {
    sBox_.emplace_back();
    sBox_.back().set(fs, b[0], b[1], b[2]);
  }
  const double pin[3][3] = {{3800, 40, 0.5}, {5100, 45, 0.4}, {6900, 50, 0.3}};
  for (const auto& p : pin) {
    sPin_.emplace_back();
    sPin_.back().set(fs, p[0], p[1], p[2]);
  }
  for (int i = 0; i < kPins; ++i) {
    pGap_[i] = 1.0e-3 + 0.8e-3 * ur();
    pM_[i] = 0.4e-3 + 0.3e-3 * ur();
    px_[i] = (ur() - 0.5) * pGap_[i];
  }
  wNoise_.set(fs, 3200, 4, 1);
}

double ScraperModel::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void ScraperModel::resetAir() {
  Tube::resetAir();
  for (Ring& r : gBody_) r.re = r.im = 0;
  for (Ring& r : sBox_) r.re = r.im = 0;
  for (Ring& r : sPin_) r.re = r.im = 0;
  gClick_.re = gClick_.im = wNoise_.re = wNoise_.im = 0;
  gOn_ = cOn_ = false;
  for (int k = 0; k < kHead; ++k) hx_[k] = hv_[k] = 0;
  sx_ = sv_ = 0;
  for (int i = 0; i < kPins; ++i) pv_[i] = 0;
  wp_ = wq_ = wBreath_ = wTarget_ = 0;
  asleep_ = true;
}

void ScraperModel::scrape(double len, double dur, double press, bool connected) {
  // A new stroke the other way along the notches (up, then down), in pairs
  // over the lower notches, then the upper ones: low-low, high-high.
  gDir_ = -gDir_;
  len = std::min(len, 0.9 * kGuiroL);
  const bool high = (gCount_ / 2) % 2 == 1;
  ++gCount_;
  // Where it runs: around the low or the high third's middle (a long stroke
  // over most of it, wherever); connected, it goes on from where the stick
  // is (unless that would run off the end: then it is lifted and set there).
  const double mid = len > 0.5 * kGuiroL ? 0.5 * kGuiroL : (high ? 0.68 : 0.32) * kGuiroL;
  double x0 = mid - gDir_ * 0.5 * len;
  const bool stay = connected && gConnected_ && gOn_ == false && gTouch_;
  if (stay) {
    const double end = gx_ + gDir_ * len;
    if (end >= 0 && end <= kGuiroL && std::fabs(gx_ - x0) < 0.5 * kGuiroL) x0 = gx_;
  }
  const bool moved = std::fabs(x0 - gx_) > 1e-4;
  gx_ = std::clamp(x0, 0.0, kGuiroL);
  gConnected_ = connected;
  gLen_ = len;
  gDur_ = std::max(dur, 0.01);
  gt_ = 0;
  gPress_ = press;
  if (moved) gOn_ = false;  // (set down anew where the stroke starts)
  // The hand holds the stick so that its tip, free, would sit press / k
  // below the ridges' tops: pressed lightly it skims them (and barely drops
  // into each notch), pressed hard it falls to the bottoms and beyond.
  gHand_ = kDepth - press / kHandK;
  double sl;
  if (stay) {
    // Joined: the tip is on the notches already and only turns round.
  } else if (connected) {
    gy_ = notch(gx_, &sl) + 2e-4;  // set down gently, just over the surface
    gvy_ = 0;
  } else {
    // Separate: the stick comes back down out of the air onto the notches,
    // landing with its own blow (harder strokes from higher and faster:
    // ~0.1 .. 0.6 m/s), then scrapes on.
    static const double kLand = knob("PJ_GUIRO_LAND", 1.0);
    const double v = kLand * (0.1 + 0.25 * std::sqrt(press));
    gy_ = notch(gx_, &sl) + 2e-3;
    gvy_ = -v;
  }
  gOn_ = true;
  asleep_ = false;
  quiet_ = 0;
}

void ScraperModel::rub(double speed, double grip, double dur, double thumb0, double thumb1, double damp) {
  cSpeed_ = speed;
  cGrip_ = grip;
  cDur_ = std::max(dur, 0.02);
  ct_ = 0;
  thumb0_ = thumb0;
  thumb1_ = thumb1;
  thumb_ = thumb0;
  cDamp_ = damp;
  cOn_ = true;
  asleep_ = false;
  quiet_ = 0;
}

void ScraperModel::slap(double speed) {
  // The palm slaps the ball: the wire swings the box (its speed added to
  // whatever swing is left); the slap's own knock on the box.
  // (The palm meets the swing that is left and takes most of it: a second
  // slap restarts the swing rather than piling onto it.)
  const double before = sv_;
  sv_ = speed + 0.3 * sv_;
  for (int i = 0; i < kPins; ++i) pv_[i] -= sv_ - before;  // (the pins stay behind: the box moves off them)
  for (Ring& r : sBox_) r.step(float(0.02 * speed));
  asleep_ = false;
  quiet_ = 0;
}

void ScraperModel::blow(double breath, double dur) {
  wTarget_ = std::clamp(breath, 0.0, 1.0);
  wDur_ = dur;
  wt_ = 0;
  if (std::fabs(wp_) < 1e-6 && std::fabs(wq_) < 1e-6) wp_ = 1e-4;  // the jet's first wobble
  asleep_ = false;
  quiet_ = 0;
}

float ScraperModel::guiro() {
  float F = 0;
  if (gOn_ || gTouch_ || std::fabs(gvy_) > 1e-4) {
    // The hand's stroke: a raised cosine of position over the stroke's time.
    double v = 0;
    if (gOn_) {
      v = gDir_ * kPi * gLen_ / (2 * gDur_) * std::sin(kPi * std::min(gt_ / gDur_, 1.0));
      gt_ += dt_;
      if (gt_ >= gDur_) {  // done: the stick lifts away, or (connected) rests on the notches
        gOn_ = false;
        if (!gConnected_) gHand_ = 0.01;
      }
    }
    // The tip against the notches, four steps a sample (the contact is stiff).
    constexpr int kSub = 4;
    const double h = dt_ / kSub;
    double Fs = 0;
    for (int s = 0; s < kSub; ++s) {
      gx_ += v * h;
      double slope;
      int idx = 0;
      gx_ = std::clamp(gx_, 0.0, kGuiroL);
      const double sx = notch(gx_, &slope, &idx), pen = sx - gy_;
      double Fc = 0;
      if (pen > 0) {
        const double rate = slope * v - gvy_;  // the surface rising under the tip
        Fc = std::max(0.0, kContactK * std::pow(pen, 1.5) * (1 + kContactLoss * rate));
      }
      if (Fc > 0 && !gTouch_) {
        ++impacts_;
        // A new notch: its own ring takes the blows (the next of a few, so
        // the last ones ring on).
        if (idx != gNotch_ && !gNotchHz_.empty()) {
          gNotch_ = idx;
          gRing_ = (gRing_ + 1) & 3;
          static const double kQ = knob("PJ_GUIRO_RIDGEQ", 18), kG = knob("PJ_GUIRO_RIDGE", 0.8);
          gRidge_[gRing_].set(1 / dt_, gNotchHz_[std::clamp(idx, 0, int(gNotchHz_.size()) - 1)], kQ, kG);
        }
      }
      gTouch_ = Fc > 0;
      const double a = (-kHandK * (gy_ - gHand_) + Fc) / kTipM - kTipDamp * gvy_;
      gvy_ += a * h;
      gy_ += gvy_ * h;
      Fs += Fc;
    }
    F = float(Fs / kSub);
    if (!gOn_ && gy_ > kDepth + 2e-3) { gTouch_ = false; gvy_ = 0; }  // the stick is off
  }
  // The force on the gourd (less the hand's steady press) into its cavity and
  // shell; the tip's click (the force's change) above.
  static const float kOut = float(knob("PJ_GUIRO_OUT", 0.0013));
  const float f = F - float(gOn_ ? gPress_ : 0.0);
  float y = 0;
  for (Ring& r : gBody_) y += r.step(f);
  for (int k = 0; k < 4; ++k) y += gRidge_[k].step(k == gRing_ ? f : 0.0f);
  y += 0.15f * gClick_.step(F - gLastF_);
  gLastF_ = F;
  return kOut * y;
}

float ScraperModel::cuica() {
  // The thumb: the pitch's ratio along the stroke (smoothly), and after it
  // held where it ended.
  double u = 0, Fn = 0;
  if (cOn_) {
    const double s = std::min(ct_ / cDur_, 1.0);
    const double e = s * s * (3 - 2 * s);
    thumb_ = thumb0_ + (thumb1_ - thumb0_) * e;
    // Open: push in and pull out (one full swing of the cloth); mute: one pull.
    u = cSpeed_ * (thumb0_ > 1.5 ? std::sin(kPi * s) : std::sin(2 * kPi * s));
    Fn = cGrip_ * std::min(1.0, 5 * std::sin(kPi * s));  // the grip: on at once, eased at the ends
    ct_ += dt_;
    if (ct_ >= cDur_) cOn_ = false;
  }
  // The head's free step, then the friction solved against it (as a bow: the
  // force that keeps the stick with the cloth if static friction can, else the
  // sliding friction at the speed it leaves).
  double vf[kHead], vc = 0, Y = 0;
  for (int k = 0; k < kHead; ++k) {
    const double w = hw0_[k] * thumb_, g = hg_[k] * cDamp_ * (cOn_ ? 1.0 : 1.0);
    vf[k] = hv_[k] + dt_ * (-w * w * hx_[k] - 2 * g * hv_[k]);
    vc += vf[k];
    Y += dt_ / hm_[k];
  }
  double F = 0;
  if (Fn > 0) {
    const double Fstick = (u - vc) / Y;
    if (std::fabs(Fstick) <= kMuS * Fn) {
      F = Fstick;
    } else {
      const double sg = Fstick > 0 ? 1 : -1;
      double lo = 0, hi = kMuS * Fn;
      for (int it = 0; it < 24; ++it) {
        const double m = 0.5 * (lo + hi);
        const double dv = std::fabs(u - vc - Y * sg * m);
        const double mu = kMuD + (kMuS - kMuD) / (1 + dv / kMuV);
        if (m < Fn * mu) lo = m; else hi = m;
      }
      F = sg * 0.5 * (lo + hi);
    }
  }
  double acc = 0;
  for (int k = 0; k < kHead; ++k) {
    const double v1 = vf[k] + dt_ * F / hm_[k];
    acc += hvol_[k] * (v1 - hv_[k]) / dt_;
    hv_[k] = v1;
    hx_[k] += dt_ * v1;
  }
  // The head's volume acceleration, radiated from the open drum: Pa at 1 m.
  const double p = kRho / (4 * kPi) * acc;
  const double y = 0.5 * (p + prevA_);  // (a half-sample smoothing of the acceleration)
  prevA_ = p;
  return float(y);
}

float ScraperModel::vibraslap() {
  // The wire's swing (22 Hz, dying over ~1.4 s), the box on it.
  static const double w = 2 * kPi * 22, g = 6.91 / 1.4 / 2;
  const double a = -w * w * sx_ - 2 * g * sv_;
  sv_ += a * dt_;
  sx_ += sv_ * dt_;
  // The pins fly free inside; the box's walls catch them (restitution 0.45).
  float in = 0, pin = 0;
  for (int i = 0; i < kPins; ++i) {
    pv_[i] -= a * dt_;  // (in the box's frame the box's acceleration acts on them)
    px_[i] += pv_[i] * dt_;
    const double half = 0.5 * pGap_[i];
    if (std::fabs(px_[i]) > half && px_[i] * pv_[i] > 0) {
      const double J = pM_[i] * 1.45 * std::fabs(pv_[i]);
      pv_[i] = -0.45 * pv_[i];
      px_[i] = px_[i] > 0 ? half : -half;
      ++impacts_;
      in += float(J);
      pin += float(J * (0.6 + 0.8 * ur()));
    }
  }
  static const float kOut = float(knob("PJ_VIBRASLAP_OUT", 25));
  float y = 0;
  for (Ring& r : sBox_) y += r.step(in);
  for (Ring& r : sPin_) y += r.step(pin);
  return kOut * y;
}

float ScraperModel::whistle() {
  // The breath: up in 8 ms, held, away in 15 ms.
  wt_ += dt_;
  const double target = wt_ < wDur_ ? wTarget_ : 0.0;
  wBreath_ += (target - wBreath_) * (1 - std::exp(-dt_ / (target > wBreath_ ? 0.008 : 0.015)));
  const double b = wBreath_;
  // The pea: blown round the chamber, faster with more breath, unevenly.
  peaRate_ += (22 + 18 * b - peaRate_) * 0.001 + 0.3 * (ur() - 0.5);
  peaPh_ += 2 * kPi * peaRate_ * dt_;
  const double fm = 1 + 0.025 * std::sin(peaPh_), am = 1 + 0.25 * std::sin(peaPh_ + 1);
  // The cavity driven by the jet (a self-sustained oscillator: the jet gives
  // energy while the swing is small, saturates as it grows), two steps a
  // sample. Its pressure in Pa at 1 m: ~2 Pa rms blown hard (100 dB).
  // (The jet's turbulence drives the cavity too: there is always a little
  // to grow from, and the saturation never chokes the start.)
  const double A0 = std::max(1.0 * std::pow(b, 2.5), 1e-3);
  const double w = 2 * kPi * 3200 * fm, Q = 25, kappa = 0.2;
  const double h = dt_ / 2;
  for (int s = 0; s < 2; ++s) {
    const double drive = kappa * w * (b * am - std::min(wp_ * wp_ / (A0 * A0), 4.0));
    const double acc = -w * w * wp_ - (w / Q - drive) * wq_ + 30.0 * w * b * A0 * (2 * ur() - 1);
    wq_ += acc * h;
    wp_ += wq_ * h;
  }
  if (!std::isfinite(wp_) || std::fabs(wp_) > 50) { wp_ = wq_ = 0; }
  const float noise = wNoise_.step(float(0.004 * b * (2 * ur() - 1)));
  // (The cavity's mouth is not symmetric: a little second harmonic.)
  return float(wp_ + 0.3 * wp_ * wp_ / A0 * (b > 0.05 ? 1 : 0)) + noise;
}

float ScraperModel::sample() {
  if (asleep_) return finish(0);
  float y = 0;
  switch (kind_) {
    case Kind::kGuiro: y = guiro(); break;
    case Kind::kCuica: y = cuica(); break;
    case Kind::kVibraslap: y = vibraslap(); break;
    case Kind::kWhistle: y = whistle(); break;
  }
  if (!std::isfinite(y)) { resetAir(); y = 0; }
  last_ = y;
  const bool busy = gOn_ || cOn_ || (kind_ == Kind::kWhistle && wt_ < wDur_);
  if (std::fabs(y) < 1e-5f && !busy) {
    if (++quiet_ > int(0.3 / dt_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return finish(y);
}

ScraperPlayer::ScraperPlayer(ScraperModel& m) : Performer(m), m_(m) {}

double ScraperPlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void ScraperPlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  const double v = std::clamp(velocity, 1, 127) / 127.0, j = 1 + 0.08 * (ur() - 0.5);
  switch (m_.kind()) {
    case ScraperModel::Kind::kGuiro: {
      // 74 the long stroke, 73 the short one; harder: faster and pressed
      // harder (deeper drops).
      const bool lng = midi == 74 || (midi != 73 && midi % 2 == 0);
      // (The short stroke 80 % longer than first set, at the same speed: the
      // same click rate over more notches.)
      const double len = (lng ? 0.15 : 0.063) * j, dur = (lng ? 0.24 : 0.099) * (1.8 - 1.2 * v) * j;
      // Soft strokes are joined (the stick stays on, the next goes on from
      // where it is); harder ones are separate, the stick lifted between them.
      static const double kJoin = knob("PJ_GUIRO_JOIN", 50);
      m_.scrape(len, dur, (0.005 + 4.0 * fastPow(v, 2.3)) * j, velocity <= kJoin);
      break;
    }
    case ScraperModel::Kind::kCuica: {
      // 79 open: a push-pull, the thumb easing up about five semitones at the
      // end ("oo-wah"); 78 mute: a short pull, the thumb pressing (an octave
      // up, damped).
      const bool open = midi != 78;
      // (The grip grows with the cloth's speed, as a bow's force must with its
      // speed: too light for the speed, the stick slips twice a period, an
      // octave up, or slides without catching.)
      const double speed = (0.01 + 0.32 * fastPow(v, 1.8)) * j, grip = (0.6 + 24 * speed) * j;
      if (open) m_.rub(speed, grip, 0.34 * j, 1.0, 1.33, 1.0);
      else m_.rub(speed, grip, 0.13 * j, 1.9, 2.0, 3.0);
      break;
    }
    case ScraperModel::Kind::kVibraslap:
      m_.slap((0.08 + 2.1 * std::pow(v, 2.0)) * j);
      break;
    case ScraperModel::Kind::kWhistle:
      // 72 long, 71 short; the breath from the velocity.
      m_.blow((0.27 + 0.73 * v) * std::min(j, 1.0), midi == 71 ? 0.11 : 0.45);
      break;
  }
}

void ScraperPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
