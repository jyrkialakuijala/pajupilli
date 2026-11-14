#include "sfx.h"

#include <algorithm>
#include <cmath>

namespace pajupilli {

namespace {
double hz(double midi) { return 440 * std::exp2((midi - 69) / 12); }

// The ringer's two gongs (stamped steel bowls, ~70 and ~60 mm, on a centre
// stud), 16 modes each: {frequency (Hz), t60 (s), the mode's shape where the
// ball strikes (per unit modal mass), its radiation}. Eight families (m
// nodal diameters, n nodal circles), as a bowl's: (2,0) 1, (3,0) 2.74,
// (2,1) 3.92, (3,1) 4.61, (4,0) 5.18, (4,1) 6.62, (5,0) 8.46, (5,1) 9.90
// (between Rayleigh's ring, 1 : 2.83 : 5.42 : 8.77, and a handbell's
// measured, untuned); each a doublet (a stamped bowl is not round: 0.4 to
// 1.8 % apart), the ball striking at 20 deg (big) or 35 deg (small) from
// the pair's axes (cos m phi, sin m phi); radiation as a multipole of order
// m-1 at ka (a = 35, 30 mm) its efficiency (ka)^(m-1) / (1 + (ka)^(m-1)).
// Q ~1000 (the steel's, the stud's). The small gong ~1.19 x the big one's
// pitch (no measurement found: a ringer's familiar clash).
const float kGong[2][16][4] = {
    {{1177.6f, 2.266f, 0.766f, 0.430f}, {1182.4f, 2.134f, 0.643f, 0.431f},
     {3223.5f, 0.978f, 0.375f, 0.810f}, {3242.9f, 0.921f, 0.650f, 0.812f},
     {4604.8f, 0.567f, 0.230f, 0.747f}, {4646.4f, 0.533f, 0.193f, 0.749f},
     {5420.8f, 0.515f, 0.200f, 0.924f}, {5458.8f, 0.485f, 0.346f, 0.925f},
     {6081.8f, 0.464f, 0.096f, 0.983f}, {6143.0f, 0.436f, 0.542f, 0.984f},
     {7764.7f, 0.330f, 0.052f, 0.992f}, {7858.5f, 0.310f, 0.295f, 0.992f},
     {9912.9f, 0.258f, -0.061f, 0.999f}, {10052.7f, 0.242f, 0.345f, 0.999f},
     {11588.5f, 0.206f, -0.035f, 1.000f}, {11775.5f, 0.194f, 0.197f, 1.000f}},
    {{1400.7f, 2.266f, 0.342f, 0.435f}, {1407.7f, 2.134f, 0.940f, 0.436f},
     {3839.8f, 0.978f, -0.194f, 0.817f}, {3855.2f, 0.921f, 0.724f, 0.818f},
     {5474.2f, 0.567f, 0.103f, 0.751f}, {5534.7f, 0.533f, 0.282f, 0.753f},
     {6447.5f, 0.515f, -0.104f, 0.926f}, {6499.3f, 0.485f, 0.386f, 0.927f},
     {7241.0f, 0.464f, -0.421f, 0.984f}, {7306.5f, 0.436f, 0.354f, 0.985f},
     {9235.4f, 0.330f, -0.230f, 0.992f}, {9356.2f, 0.310f, 0.193f, 0.993f},
     {11808.3f, 0.258f, -0.349f, 0.999f}, {11950.8f, 0.242f, 0.031f, 0.999f},
     {13776.5f, 0.206f, -0.199f, 1.000f}, {14026.7f, 0.194f, 0.017f, 1.000f}}};

// The ringer's armature and clapper: the ball and the arm's inertia at the
// ball (12 g) on the armature's spring (resonant ~18 Hz, just under the ring
// current's 20: a ringer is built to swing with it), lightly damped; the
// gongs 1.2 mm either side of its rest; each gong's modal mass at the rim
// (8 g); steel ball (6 mm) on steel: Hertz K = 4/3 E* sqrt(R) ~ 8e9.
constexpr double kClapM = 0.012, kClapF0 = 18, kClapZeta = 0.1, kGap = 2e-3, kGongM = 0.008, kBallK = 8e9;

// F = K (c0 - G F)^1.5 (the contact's force through this step: c0 the
// overlap without it, G how far a unit force moves the two apart).
double hertz(double c0, double G, double K) {
  if (c0 <= 0) return 0;
  double lo = 0, hi = c0 / G, F = 0.5 * hi;
  for (int it = 0; it < 40; ++it) {
    const double c = std::max(c0 - G * F, 0.0);
    const double r = F - K * c * std::sqrt(c);
    if (r > 0) hi = F; else lo = F;
    const double dr = 1 + 1.5 * K * std::sqrt(std::max(c, 1e-15)) * G;
    double Fn = F - r / dr;
    if (Fn <= lo || Fn >= hi) Fn = 0.5 * (lo + hi);
    if (std::fabs(Fn - F) < 1e-10 * (1 + F)) return Fn;
    F = Fn;
  }
  return F;
}
}  // namespace

void Sfx::tuneGongs(double scale) {
  for (int g = 0; g < 2; ++g) {
    Gong& G = gong_[g];
    for (int k = 0; k < Gong::kN; ++k) {
      const double f = kGong[g][k][0] * scale, t60 = kGong[g][k][1] / std::sqrt(scale);
      const double w = 2 * M_PI * std::min(f, 0.45 * fs_), s = 6.907755 / t60;
      const double wd = std::sqrt(std::max(w * w - s * s, 1e-6)), e = std::exp(-s * dt_);
      const double c = std::cos(wd * dt_), sn = std::sin(wd * dt_);
      G.c00[k] = float(e * (c + s / wd * sn));
      G.c01[k] = float(e * sn / wd);
      G.c10[k] = float(-e * w * w * sn / wd);
      G.c11[k] = float(e * (c - s / wd * sn));
      G.exc[k] = kGong[g][k][2];
      G.rad[k] = f < 0.45 * fs_ ? kGong[g][k][3] : 0;
    }
  }
}

Sfx::Sfx(Type t, double fs) : t_(t), fs_(fs), dt_(1 / fs), set_(fs) {
  switch (t_) {
    case kFret: {
      // A wound E string (82.4 Hz, 0.65 m, ~70 N) on a guitar's top (its air
      // and plate modes at the bridge).
      ModalSet::Def d;
      const double f0 = 82.4, L = 0.65, T = 70, mu = T / (4 * L * L * f0 * f0);
      d.n = 64;
      d.modalMass = float(mu * L / 2);
      d.split = 0.002f;
      for (int k = 1; k <= d.n; ++k) {
        const double f = k * f0 * std::sqrt(1 + 1e-4 * k * k);
        d.freq[k - 1] = float(f);
        d.t60[k - 1] = float(3 / (1 + (f / 3000) * (f / 3000)));
        d.exc[k - 1] = float(stringShape(k, 0.4 * L, L));
        d.bridge[k - 1] = float(k * M_PI * T / L);
        d.back[k - 1] = float(-2 / (k * M_PI));
      }
      set_.setVib(0, d);
      const float f[6] = {100, 200, 230, 400, 550, 800}, t60[6] = {0.4f, 0.3f, 0.3f, 0.2f, 0.15f, 0.1f},
                  m[6] = {0.3f, 0.2f, 0.2f, 0.15f, 0.15f, 0.12f}, r[6] = {1, 1, 0.8f, 0.8f, 0.7f, 0.6f},
                  sd[6] = {0.2f, -0.3f, 0.3f, -0.2f, 0.2f, -0.2f};
      set_.setBody(6, f, t60, m, r, sd);
      break;
    }
    case kBreath:
      singer_.init(fs, 3, 0);
      singer_.setWhisper(1);
      break;
    case kSea:
      bub_.resize(384);
      for (Bubble& b : bub_) b.on = false;
      break;
    case kPhone:
      for (Gong& g : gong_) {
        std::fill(g.q, g.q + Gong::kN, 0.0f);
        std::fill(g.v, g.v + Gong::kN, 0.0f);
      }
      tuneGongs(1);
      break;
    case kApplause: {
      // 40 rows of 24 seats (0.5 m apart, rows 0.9 m), ordered nearest the
      // front centre first (a small crowd sits close).
      constexpr int kRows = 40, kSeats = 24;
      std::vector<std::pair<double, std::pair<float, float>>> seat;
      for (int r = 0; r < kRows; ++r)
        for (int s = 0; s < kSeats; ++s) {
          const float x = 0.5f * (s - (kSeats - 1) / 2.0f), y = 2 + 0.9f * r;
          seat.push_back({std::hypot(x, y), {x, y}});
        }
      std::sort(seat.begin(), seat.end());
      cl_.resize(seat.size());
      for (size_t i = 0; i < cl_.size(); ++i) {
        Clapper& c = cl_[i];
        const float x = seat[i].second.first, y = seat[i].second.second;
        // Hands: the cupped palms' air (0.7..2.5 kHz, Q 3..10), each clapper's own.
        const double f = 700 * std::pow(2500.0 / 700, u01()), Q = 3 + 7 * u01();
        const double r = std::exp(-M_PI * f / Q / fs);
        c.a1 = float(2 * r * std::cos(2 * M_PI * f / fs));
        c.a2 = float(-r * r);
        c.b0 = float(1 - r);
        c.y1 = c.y2 = 0;
        // ~40 % seek the beat (each as keenly as he will), the rest keep
        // their own: faster, uncoupled.
        if (u01() < 0.4) {
          c.k = 2 + 6 * u01();  // rad/s per unit phase difference
          c.rate = 2.0 + 0.6 * u01();
        } else {
          c.k = 0;
          c.rate = 3.5 + 2.5 * u01();
        }
        c.beat = c.rate;
        c.ph = u01();
        // Farther: quieter (1/r), panned by where they sit.
        c.amp = float(std::exp(0.6 * urand()) * 3 / std::hypot(x, y));
        c.pan = 0.9f * x / std::hypot(x, y);
        c.on = false;
        c.t = 1e9;
        c.x = 0;
        c.stopAt = 1e9;
      }
      // Each seeker's six nearest seekers (whom he follows; the others'
      // claps are noise to him).
      for (size_t i = 0; i < cl_.size(); ++i) {
        const float xi = seat[i].second.first, yi = seat[i].second.second;
        std::pair<float, int> best[6];
        for (auto& b : best) b = {1e9f, int(i)};
        if (cl_[i].k == 0) {
          for (int k = 0; k < 6; ++k) cl_[i].nb[k] = int(i);
          continue;
        }
        for (size_t j = 0; j < cl_.size(); ++j) {
          if (j == i || cl_[j].k == 0) continue;
          const float d = std::hypot(seat[j].second.first - xi, seat[j].second.second - yi);
          if (d < best[5].first) {
            best[5] = {d, int(j)};
            std::sort(best, best + 6);
          }
        }
        for (int k = 0; k < 6; ++k) cl_[i].nb[k] = best[k].second;
      }
      break;
    }
    default:
      break;
  }
}

void Sfx::reset() {
  set_.reset();
  singer_.reset();
  for (Bubble& b : bub_) b.on = false;
  for (Clapper& c : cl_) { c.y1 = c.y2 = 0; c.on = false; c.t = 1e9; }
  ringing_.clear();
  for (Shout& s : shout_) {
    s.v.reset();
    s.t = 1e9;
  }
  bx_ = by_ = tract1_ = tract2_ = 0;
  shotT_ = 1e9;
  env_ = spin_ = 0;
  for (Gong& g : gong_) {
    std::fill(g.q, g.q + Gong::kN, 0.0f);
    std::fill(g.v, g.v + Gong::kN, 0.0f);
  }
  clapX_ = clapV_ = ringEnv_ = 0;
}

bool Sfx::applauding() const {
  if (t_ != kApplause) return false;
  for (const Clapper& c : cl_)
    if (c.on) return true;
  return false;
}

void Sfx::bravo(double strength) {
  const uint32_t keep = rng_;
  rng_ = shoutRng_;
  const int n = strength > 0.95 ? 3 : strength > 0.85 ? 2 : 1;
  for (int i = 0; i < n; ++i) {
    Shout& s = shout_[nextShout_];
    nextShout_ = (nextShout_ + 1) % 3;
    // Most of the shouters men (shouting ~150-190 Hz, an 18 cm tract), some
    // women (~260-330 Hz, 15 cm); each from his own seat, a little later than
    // the first.
    const bool woman = u01() < 0.3;
    s.v.init(fs_, rng_ >> 8, 5 + i);
    s.note = woman ? 70 : 45;
    s.f = woman ? 260 + 70 * u01() : 150 + 40 * u01();
    s.lung = 1300 + 900 * strength * (0.8 + 0.2 * u01());
    s.t = -(i == 0 ? 0.0 : 0.08 + 0.25 * u01());
    s.pan = 0.8f * urand();
    s.gain = float(i == 0 ? 1.0 : 0.5 + 0.4 * u01());
  }
  shoutRng_ = rng_;
  rng_ = keep;
}

void Sfx::noteOn(int midi, double vel) {
  key_ = midi;
  vel_ = std::clamp(vel, 0.02, 1.0);
  held_ = true;
  clock_ = 0;
  switch (t_) {
    case kFret:
      // A slide: up or down the string, faster for higher keys.
      slideT_ = 0;
      slideDur_ = 0.12 + 0.25 * (1 - vel_);
      vMax_ = 0.25 + 1.2 * std::clamp((midi - 36) / 48.0, 0.0, 1.0);
      break;
    case kSea:
      waveT_ = 0;
      wavePeriod_ = 7 + 4 * u01();
      waveSize_ = 0.7 + 0.5 * u01();
      break;
    case kBird:
      syl_ = 0;
      sylN_ = 3 + int(5 * vel_);
      sylT_ = 0;
      fHi_ = std::clamp(hz(midi + 36) * 2, 1500.0, 10000.0);  // (middle C: ~4 kHz down to ~2: a small songbird)
      fLo_ = fHi_ * (0.45 + 0.15 * u01());
      sylDur_ = 0.045 + 0.04 * u01();
      gap_ = 0.03 + 0.03 * u01();
      break;
    case kPhone:
      // (The big gong's (2,0) at the key two octaves up: C4 rings 1047 Hz.)
      ringT_ = 0;
      tuneGongs(hz(midi + 24) / kGong[0][0][0]);
      break;
    case kHeli:
      rotHz_ = 4 + 2 * std::clamp((midi - 36) / 48.0, 0.0, 1.0);
      break;
    case kApplause: {
      // The key: the crowd's size (C2 80 hands' worth .. C6 all 960).
      const int n = std::clamp(80 + (midi - 36) * 22, 80, int(cl_.size()));
      for (int i = 0; i < int(cl_.size()); ++i) {
        Clapper& c = cl_[i];
        c.on = i < n;
        c.stopAt = 1e9;
        c.ph = 1 - 0.5 * u01() * c.rate / 4;  // they join within a beat or two
        c.beat = c.rate;
      }
      break;
    }
    case kGun:
      shotT_ = 0;
      shotT0_ = std::clamp(0.0015 + 0.0025 * (72 - midi) / 36.0, 0.0012, 0.0045);
      shotP_ = vel_;
      break;
    default:
      break;
  }
}

void Sfx::noteOff(int midi) {
  if (midi != key_) return;
  held_ = false;
  if (t_ == kApplause)
    for (Clapper& c : cl_)
      if (c.on) c.stopAt = clock_ + 1.5 * u01();
}

float Sfx::sample(float* side) {
  clock_ += dt_;
  float y = 0, sd = 0;
  switch (t_) {
    case kFret: {
      // The finger's speed along the string: up and down a sine over the slide.
      if (slideT_ < slideDur_) {
        const double v = vMax_ * std::sin(M_PI * slideT_ / slideDur_);
        windPh_ += v / 0.00025 * dt_ * (1 + 0.1 * urand());
        if (windPh_ >= 1) {  // a winding knocks the fingertip and the string
          windPh_ -= 1;
          set_.pluck(0, 0.08 * (0.4 + vel_) * (1 + 0.3 * urand()), dt_, 0, dt_, 1.2);
        }
        if (++ctl_ >= 64) {  // the finger moves along: where it touches
          ctl_ = 0;
          float sh[ModalSet::kModes];
          const double x = 0.3 + 0.2 * slideT_ / slideDur_;
          for (int k = 0; k < ModalSet::kModes; ++k) sh[k] = float(stringShape(k + 1, x, 1));
          set_.setExcitation(0, sh);
        }
      }
      slideT_ += dt_;
      y = set_.sample();
      sd = set_.side();
      break;
    }
    case kBreath: {
      env_ += (1 - std::exp(-dt_ / (held_ ? 0.08 : 0.15))) * ((held_ ? 1.0 : 0.0) - env_);
      if (++ctl_ >= 16) {
        ctl_ = 0;
        const double vowel = std::clamp((key_ - 40) / 10.0, 0.0, 4.0);
        singer_.control(110, vowel, env_ * (300 + 900 * vel_), 1, 0, 5, key_, 16 * dt_);
      }
      y = float(singer_.sample() * 2e-3);
      break;
    }
    case kSea: {
      // The wave's cycle: the crash (a fast rise), the wash (a slow fall), the backwash.
      if (held_ || env_ > 1e-3) {
        waveT_ += dt_;
        if (waveT_ > wavePeriod_ && held_) {
          waveT_ = 0;
          wavePeriod_ = 7 + 4 * u01();
          waveSize_ = 0.7 + 0.5 * u01();
        }
      }
      const double w = waveT_ < 0.4 ? waveT_ / 0.4 : std::exp(-(waveT_ - 0.4) / 1.4) + 0.08 * std::exp(-(waveT_ - 3) * (waveT_ - 3));
      env_ = held_ ? w * waveSize_ : env_ * (1 - dt_ / 2);
      // New bubbles at a rate following the wave (up to ~4000/s at the crash).
      double expect = 4000 * vel_ * env_ * dt_;
      while (expect > 0 && u01() < expect) {
        expect -= 1;
        for (Bubble& b : bub_) {
          if (b.on) continue;
          const double rmin = 0.3e-3, rmax = 8e-3, a = 1.5;
          const double R = std::pow(std::pow(rmin, -a) - u01() * (std::pow(rmin, -a) - std::pow(rmax, -a)), -1 / a) *
                           std::exp2((60 - key_) / 24.0);
          const double f = 3.26 / R, d = 0.043 * f + 0.0014 * std::pow(f, 1.5);
          if (f > 0.45 * fs_) break;
          b.re = 0;
          b.im = 1;
          b.c = float(std::cos(2 * M_PI * f * dt_));
          b.s = float(std::sin(2 * M_PI * f * dt_));
          b.decay = float(std::exp(-d * dt_));
          b.chirp = float(1 + 0.1 * d * dt_);  // the radius shrinking: the pitch rises
          b.amp = float(R / 1e-3 * (0.5 + u01()));
          b.pan = 0.9f * urand();
          b.on = true;
          break;
        }
      }
      for (Bubble& b : bub_) {
        if (!b.on) continue;
        const float re = b.re * b.c - b.im * b.s, im = b.re * b.s + b.im * b.c;
        b.re = re * b.decay;
        b.im = im * b.decay;
        // (chirp: the rotation's angle grows a little each sample)
        const float s2 = std::min(b.s * b.chirp, 0.95f);
        b.c = std::sqrt(1 - s2 * s2);
        b.s = s2;
        const float o = b.amp * b.re;
        y += o;
        sd += b.pan * o;
        if (b.re * b.re + b.im * b.im < 1e-8f) b.on = false;
      }
      // The froth: bubbles too small and many to follow, a hiss.
      const float n = urand();
      hissLp_ += 0.6 * (n - hissLp_);
      hissHp_ += 0.05 * (hissLp_ - hissHp_);
      y += float(0.15 * std::sqrt(vel_ * env_) * (hissLp_ - hissHp_));
      y *= 0.38f;
      sd *= 0.38f;
      break;
    }
    case kBird: {
      // The gestures: syllables (a pressure pulse with a falling tension sweep), gaps.
      double p = -0.5, f = fLo_;
      if (syl_ < sylN_) {
        if (sylT_ < sylDur_) {
          const double u = sylT_ / sylDur_;
          p = std::sin(M_PI * u);
          f = fHi_ * std::pow(fLo_ / fHi_, u) * (1 + 0.15 * std::exp(-u / 0.08));  // a little hook up at the start
        }
        sylT_ += dt_;
        if (sylT_ > sylDur_ + gap_) {
          sylT_ = 0;
          ++syl_;
          fHi_ *= 1 + 0.03 * urand();
        }
      } else if (held_) {  // held: the phrase again after a breath
        sylT_ += dt_;
        if (sylT_ > 0.4) { syl_ = 0; sylT_ = 0; }
      }
      // The labia: van der Pol, 8 substeps.
      const double w = 2 * M_PI * f, mu = 0.25 * w, h = dt_ / 8;
      by_ += 1e-3 * w * urand() * h;  // (the air's turbulence: what starts the labia)
      for (int s = 0; s < 8; ++s) {
        by_ += h * (mu * (p - bx_ * bx_) * by_ - w * w * bx_);
        bx_ += h * by_;
      }
      if (!std::isfinite(bx_) || std::fabs(bx_) > 10) bx_ = by_ = 0;
      // The tract (the beak's opening follows the note): a resonance at f, Q 4.
      const double r = std::exp(-M_PI * f / 4 / fs_), a1 = 2 * r * std::cos(w * dt_), a2 = -r * r;
      const double o = (1 - r) * bx_ + a1 * tract1_ + a2 * tract2_;
      tract2_ = tract1_;
      tract1_ = o;
      y = float(0.3 * vel_ * o);
      break;
    }
    case kPhone: {
      // The ring current: 20 Hz, 2 s on and 4 s off, through the ringer's
      // capacitor and coils (its envelope builds and dies in ~15 ms); its
      // force on the armature (a polarised ringer: as the current), stronger
      // for a harder key (the line's ringing voltage).
      if (held_) ringT_ += dt_;
      const bool on = held_ && std::fmod(ringT_, 6.0) < 2.0;
      ringEnv_ += (1 - std::exp(-dt_ / 0.015)) * ((on ? 1.0 : 0.0) - ringEnv_);
      ringPh_ = std::fmod(ringPh_ + 20 * dt_, 1.0);
      const double wc = 2 * M_PI * kClapF0;
      const double Fd = (0.3 + 0.9 * vel_) * ringEnv_ * std::sin(2 * M_PI * ringPh_);
      const double Fc = Fd - kClapM * wc * wc * clapX_ - 2 * kClapZeta * kClapM * wc * clapV_;
      // The clapper through this step without the gongs, each gong's rim
      // where the ball strikes without the ball; the contacts solved with
      // the step (a steel ball on a steel rim: ~0.1 ms).
      const double v1 = clapV_ + Fc / kClapM * dt_, x1 = clapX_ + v1 * dt_;
      double F[2];
      for (int g = 0; g < 2; ++g) {
        Gong& G = gong_[g];
        double y = 0, cg = 0;
        for (int k = 0; k < Gong::kN; ++k) {
          y += G.exc[k] * (G.c00[k] * G.q[k] + G.c01[k] * G.v[k]);
          cg += G.exc[k] * G.exc[k] * G.c01[k];
        }
        const double over = (g == 0 ? x1 : -x1) - (kGap + y);
        F[g] = hertz(over, cg * dt_ / kGongM + dt_ * dt_ / kClapM, kBallK);
      }
      clapV_ = v1 - (F[0] - F[1]) / kClapM * dt_;
      clapX_ += clapV_ * dt_;
      for (int g = 0; g < 2; ++g) {
        Gong& G = gong_[g];
        const float imp = float(F[g] * dt_ / kGongM);
        float o = 0;
        for (int k = 0; k < Gong::kN; ++k) {
          const float v0 = G.v[k] + imp * G.exc[k];
          const float q = G.c00[k] * G.q[k] + G.c01[k] * v0;
          G.v[k] = G.c10[k] * G.q[k] + G.c11[k] * v0;
          G.q[k] = q;
          o += G.rad[k] * G.v[k];
        }
        // The big gong a little left, the small one right.
        y += 0.5f * o;
        sd += g ? 0.15f * o : -0.15f * o;
      }
      y *= 0.38f;
      sd *= 0.38f;
      break;
    }
    case kHeli: {
      spin_ += (1 - std::exp(-dt_ / (held_ ? 1.5 : 3.0))) * ((held_ ? 1.0 : 0.0) - spin_);
      if (spin_ < 1e-4) break;
      const double rot = rotHz_ * spin_;
      // Four blades: a passage each quarter turn.
      const double prev = rotPh_;
      rotPh_ += 4 * rot * dt_;
      if (std::floor(rotPh_) != std::floor(prev)) {  // a blade meets its predecessor's vortex
        for (int i = 7; i > 0; --i) pulseAmp_[i] = pulseAmp_[i - 1];
        pulseAmp_[0] = float(spin_ * spin_ * (0.85 + 0.3 * u01()));
      }
      const double tp = (rotPh_ - std::floor(rotPh_)) / (4 * std::max(rot, 0.1));  // s since the passage
      const double tau = 0.0012, t0 = 3 * tau, z = (tp - t0) / tau;
      double o = pulseAmp_[0] * z * std::exp(-z * z);                          // the slap (BVI)
      const double tz = (tp - 0.012) / 0.006;
      o -= 0.35 * pulseAmp_[0] * std::exp(-tz * tz);                           // thickness noise
      tailPh_ += 2 * 6.5 * rot * dt_;                                           // the tail rotor (2 blades)
      const double tt = (tailPh_ - std::floor(tailPh_)) / (2 * 6.5 * std::max(rot, 0.1)), zt = (tt - 0.002) / 0.0006;
      o += 0.25 * spin_ * zt * std::exp(-zt * zt);
      whinePh_ += 6800 * spin_ * dt_;
      o += 0.015 * spin_ * std::sin(2 * M_PI * whinePh_) * (1 + 0.3 * urand());  // the turbine
      heliLp_ += 0.45 * (o - heliLp_);  // (the air between: the highs go)
      y = float(0.5 * vel_ * heliLp_);
      break;
    }
    case kApplause: {
      // Every 32 samples the clappers move on: each one's phase through the
      // block (the beat seekers pulled toward their neighbours' claps, dphi
      // = k / 2 pi sum sin(2 pi (phi_j - phi_i)) / n, and the hall's beat
      // as a whole: the seekers' mean phase, weaker); a clap within it is
      // put at its sample.
      constexpr int kBlk = 32;
      if (clapCtl_-- <= 0) {
        clapCtl_ = kBlk - 1;
        double zr = 0, zi = 0;
        int nz = 0;
        for (Clapper& c : cl_)
          if (c.on && c.k > 0) {
            c.cph = std::cos(2 * M_PI * c.ph);
            c.sph = std::sin(2 * M_PI * c.ph);
            zr += c.cph;
            zi += c.sph;
            ++nz;
          }
        if (nz) zr /= nz, zi /= nz;
        for (size_t i = 0; i < cl_.size(); ++i) {
          Clapper& c = cl_[i];
          if (!c.on) continue;
          double dph = c.beat * kBlk * dt_;
          if (c.k > 0) {
            double pull = 0;
            int n = 0;
            // (sin(b - a) = sin b cos a - cos b sin a: this block's cosines and sines.)
            for (int j : c.nb)
              if (cl_[j].on && j != int(i)) {
                pull += cl_[j].sph * c.cph - cl_[j].cph * c.sph;
                ++n;
              }
            if (n) pull /= n;
            pull += 0.5 * (zi * c.cph - zr * c.sph);  // |Z| sin(arg Z - a)
            dph = std::max(dph + c.k / (2 * M_PI) * pull * kBlk * dt_, 0.0);
          }
          if (c.ph + dph >= 1) {
            if (clock_ >= c.stopAt) {
              c.on = false;
              continue;
            }
            // (Its sample within the block: t counts up from minus that.)
            if (c.t > 0.015) ringing_.push_back(int(i));
            c.t = -(1 - c.ph) / dph * kBlk * dt_;
            c.ph += dph - 1;
            c.beat = c.rate * (1 + 0.08 * urand()) * (0.9 + 0.2 * vel_);  // the next beat's jitter
          } else {
            c.ph += dph;
          }
        }
      }
      // The impact: a ~0.6 ms burst into the palms' air, which rings
      // ~15 ms; only those ringing are stepped.
      for (size_t r = 0; r < ringing_.size();) {
        Clapper& c = cl_[ringing_[r]];
        float x = 0;
        if (c.t >= 0 && c.t < 0.0006) x = c.amp * urand() * float(1 - c.t / 0.0006);
        c.t += dt_;
        const float o = c.b0 * x + c.a1 * c.y1 + c.a2 * c.y2;
        c.y2 = c.y1;
        c.y1 = o;
        y += o;
        sd += c.pan * o;
        if (c.t > 0.015) {
          c.y1 = c.y2 = 0;
          ringing_[r] = ringing_.back();
          ringing_.pop_back();
        } else {
          ++r;
        }
      }
      y *= float(0.3 * vel_);
      sd *= float(0.3 * vel_);
      // The shouts: "BRA-vo", ~0.65 s: the lips closed (b: the voice
      // under them), released; the tongue tip's tap (r, ~18 ms); a long
      // stressed a, its pitch rising and falling; the lower lip to the teeth
      // (v: a slit, voiced and hissing); o, falling, the breath let go.
      const bool ctl = ++shoutCtl_ >= 32;
      if (ctl) shoutCtl_ = 0;
      for (Shout& s : shout_) {
        if (s.t > 0.8) continue;
        if (s.t < 0) {
          s.t += dt_;
          continue;
        }
        if (ctl || s.t < dt_ * 1.5) {
          const double t = s.t;
          double f0 = t < 0.15 ? 0.95 + 0.25 * t / 0.15 : t < 0.3 ? 1.2 - 0.15 * (t - 0.15) / 0.15
                                                                    : 1.05 - 0.25 * std::min((t - 0.3) / 0.32, 1.0);
          const double lung = s.lung * std::clamp(t / 0.02, 0.0, 1.0) * std::clamp((0.65 - t) / 0.1, 0.0, 1.0);
          if (t < 0.035) s.v.setConstriction(1.0, 0.002);          // b
          else if (t < 0.05) s.v.setConstriction(1.0, 0);
          else if (t < 0.068) s.v.setConstriction(0.88, 0.01);     // r: the tap on the ridge
          else if (t < 0.30) s.v.setConstriction(0.88, 0);
          else if (t < 0.37) s.v.setConstriction(1.0, 0.08);      // v: lip and teeth
          else s.v.setConstriction(1.0, 0);
          s.v.control(s.f * f0, t < 0.31 ? 2.0 : 1.0, lung, 0.1, 0, 5, s.note, 32 * dt_);
        }
        const float o = float(s.v.sample() * 4e-3) * s.gain;
        y += o;
        sd += s.pan * o;
        s.t += dt_;
      }
      break;
    }
    case kGun: {
      if (shotT_ > 0.3) break;
      const double t = shotT_, T = shotT0_;
      // The bullet's N-wave, then the blast (Friedlander) and its ground reflection.
      const double tn = 0.0003;
      double o = t < tn ? 0.4 * (1 - 2 * t / tn) : 0;
      auto blast = [&](double tb) { return tb >= 0 ? (1 - tb / T) * std::exp(-1.8 * tb / T) : 0.0; };
      o += blast(t - 0.004) + 0.5 * blast(t - 0.0065);
      heliLp_ += 0.85 * (o - heliLp_);  // (a little band limit)
      y = float(shotP_ * heliLp_);
      shotT_ += dt_;
      break;
    }
  }
  *side = sd;
  return y;
}

}  // namespace pajupilli
