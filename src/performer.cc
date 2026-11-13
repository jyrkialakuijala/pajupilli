#include "performer.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "knob.h"

namespace pajupilli {

Performer::Performer(Tube& t) : tube_(t), inst_(t.instrument()), fs_(t.fs()) {
  for (int i = 0; i < 128; ++i) coverTrim[i] = coverLoTrim[i] = coverHiTrim[i] = pressTrim[i] = jetTrim[i] = 1;
  for (int& f : fingering) f = 0;
  for (double& d : dynLo) d = 0;
  for (double& d : dynHi) d = 1;
  kRetongue_ = knob("PJ_RETONGUE", 100);  // an overlapped key this hard is tongued
  kDvTongue_ = knob("PJ_DVTONGUE", 30);   // ... or this much harder than the key before
  kDvGlide_ = knob("PJ_DVGLIDE", 30);     // an overlapped key this much softer ...
  kGlideVel_ = knob("PJ_GLIDEVEL", 45);   // ... and this soft at most glides (a deliberate touch)
  kGlide_ = knob("PJ_GLIDE", 1);          // the glide's time (x; 0: never glide)
  kStaccato_ = knob("PJ_STACCATO", 0.12); // held shorter than this (s): a tongue stop
  kDetache_ = knob("PJ_DETACHE", 0.08);   // a gap shorter than this (s): "du"
  release_ = knob("PJ_RELEASE", 0.030);   // the breath's release (s; knob 7 live)
}

Performer::Articulation Performer::articulate(int midi, int velocity, bool overlap) {
  const Articulation a = decide(midi, velocity, overlap);
  lastArt_ = a;
  return a;
}

Performer::Articulation Performer::decide(int midi, int velocity, bool overlap) {
  const int prev = note_;
  const double dv = velocity - lastVel_;
  lastVel_ = velocity;
  lastOnAt_ = now_;
  stopping_ = false;
  cancelGlide();
  const double hard = std::max(0.3, hardness_);
  if (overlap && prev >= 0) {
    if (velocity >= kRetongue_ / hard || dv >= kDvTongue_) return kArtTongueLegato;
    // A glide is a gesture: the new key pressed very softly while the old one
    // is held (ordinary legato varies by 30 in velocity: it must not glide).
    if (kGlide_ > 0 && dv <= -kDvGlide_ && velocity <= kGlideVel_ && prev != midi && std::abs(midi - prev) <= 12) {
      // Can the pitch glide? A pan flute's pipes cannot; a jet's register
      // change on the same fingering is a jump, not a glide (a brass player's
      // lips glide through the partials: a rip).
      const NoteInfo *a = played(prev), *b = played(midi);
      const bool pipes = inst_.design && inst_.design->nPipes > 0;
      const bool jet = inst_.kind == Kind::kFlute || inst_.kind == Kind::kDuct || inst_.kind == Kind::kEndBlown;
      const bool same = a && b && std::strcmp(a->holes, b->holes) == 0;
      if (!pipes && !(jet && same)) {
        glideFrom_ = prev;
        glideT_ = 0;
        glideEar_ = earCorr_;
        const double soft = std::clamp((-dv - kDvGlide_) / 40, 0.0, 1.0);
        glideDur_ = kGlide_ * (0.06 + 0.025 * std::abs(midi - prev)) * (1 + soft);
        holeGlide_ = midi > prev ? glideDur_ : -glideDur_;
        return kArtGlide;
      }
    }
    return kArtSlur;
  }
  if (blowing_ || now_ - lastOffAt_ < kDetache_) return velocity > 90 / hard ? kArtTu : kArtDu;
  if (velocity < 35 * hardness_) return kArtBreath;
  if (velocity > 90 / hard) return kArtAccent;
  return kArtTu;
}

void Performer::released() {
  lastOffAt_ = now_;
  if (now_ - lastOnAt_ < kStaccato_) {
    stopping_ = true;
    stopClock_ = 0;
  }
}

double Performer::glideMix() const {
  if (!gliding()) return 1;
  const double t = glideT_ / glideDur_;
  return t * t * (3 - 2 * t);
}

double Performer::glideCover(int note, double dyn) const {
  if (!gliding()) return 1;
  return std::exp((1 - glideMix()) * (std::log(coverAt(glideFrom_, dyn)) - std::log(coverAt(note, dyn))));
}

double Performer::coverAt(int note, double dyn) const {
  const double lo = std::max(dynLo[note], 0.15);
  const double cl = std::log(coverLoTrim[note]), cm = std::log(coverTrim[note]), ch = std::log(coverHiTrim[note]);
  const double ct = dyn < 0.5 ? cl + (cm - cl) * std::clamp((dyn - lo) / std::max(0.5 - lo, 1e-3), 0.0, 1.0)
                              : cm + (ch - cm) * std::min(1.0, (dyn - 0.5) / (kCalDynHi - 0.5));
  return std::exp(ct);
}

// The pitch of the note, from a band-pass around it: each upward zero crossing
// gives one period; the control follows the error (the lips move slowly, like
// an ear correcting, so a fast trill is barely touched and a held note settles).
void Performer::listen(const float* x, int m, double& corr, double cents, double clock, double slope,
                       double tau, double dead, double range) {
  const double f0 = noteFrequency(note_), w = 2 * M_PI * f0 / fs_;
  const double ft = f0 * std::exp2(cents / 1200);  // where the ear wants it
  const double r = std::exp(-M_PI * f0 / (3 * fs_));  // Q 3
  const double a1 = 2 * r * std::cos(w), a2 = -r * r;
  for (int k = 0; k < m; ++k) {
    const double y = (1 - r) * x[k] + a1 * bp1_ + a2 * bp2_;
    bp2_ = bp1_;
    bp1_ = y;
    const double t = clock * fs_ - m + k;  // samples on the note
    if (prevBp_ < 0 && y >= 0) {
      const double tc = t - 1 + prevBp_ / (prevBp_ - y);
      if (lastCross_ >= 0 && clock > 0.02) {  // past the note's own start
        const double f = fs_ / (tc - lastCross_);
        if (f > f0 / 1.3 && f < f0 * 1.3) {
          double err = 1200 * std::log2(f / ft);
          heard_ = err;
          if (std::fabs(err) > range) err = 0;
          err = err > dead ? err - dead : err < -dead ? err + dead : 0;
          corr -= err / slope * (1 / f) / tau;
          corr = std::clamp(corr, -0.4, 0.4);
        }
      }
      lastCross_ = tc;
    }
    prevBp_ = y;
  }
}

double Vibrato::step(double dt, double rate, double sinceOn, double support, double wander) {
  static const double kNoise = knob("PJ_VIBNOISE", 1);
  auto smooth = [](double x) { x = std::clamp(x, 0.0, 1.0); return x * x * (3 - 2 * x); };
  if (sinceOn < last_) {  // an attack: this note's own onset
    delay_ = 0.3 * (1 + 0.3 * urand());
    fade_ = 0.4 * (1 + 0.3 * urand());
    curve_ = std::exp2(0.6 * urand());
    start_ = urand() < -0.8 ? 0.675 + 0.125 * urand() : 1.0;
  }
  last_ = sinceOn;
  // Breathing: two low-passed noises (unit std), the speed's and the depth's.
  // The speed's within a few pulses (0.15 s: ~1 Hz), the depth's slower.
  static const double kRateTau = knob("PJ_VIBRATETAU", 0.15);
  constexpr double kTau = 0.6;
  const double kr = std::sqrt(6 * dt / kRateTau) * std::sqrt(2.0) * kNoise * wander;
  const double k = std::sqrt(6 * dt / kTau) * std::sqrt(2.0) * kNoise * wander;
  n1_ += kr * urand() - dt / kRateTau * n1_;
  rateN_ += (n1_ - rateN_) * dt / kRateTau;
  d1_ += k * urand() - dt / kTau * d1_;
  depthN_ += (d1_ - depthN_) * dt / kTau;
  const double t = sinceOn - delay_;
  env_ = std::pow(smooth(t / fade_), curve_);
  const double speed = rate * rateMul * (start_ + (1 - start_) * smooth(t / (1.5 * fade_))) *
                       std::max(0.3, 1 + 0.15 * rateN_) * (1 + 0.06 * (support - 0.5));
  const double a = 2 * M_PI * speed * dt, c = std::cos(a), s = std::sin(a);
  const double re = re_ * c - im_ * s;
  im_ = re_ * s + im_ * c;
  re_ = re;
  const double m = 1.5 - 0.5 * (re_ * re_ + im_ * im_);
  re_ *= m;
  im_ *= m;
  return re_ * std::max(0.0, 1 + 0.20 * depthN_);
}

}  // namespace pajupilli
