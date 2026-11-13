#include "bass.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

#include "knob.h"

namespace pajupilli {

namespace {
// Gut strings, E1 A1 D2 G2 on a 106 cm scale (first guesses, to be fitted to
// recordings): tension (N), stiffness, losses, stretch stiffness, action.
const BassString kGut[4] = {{41.203, 1.06, 230, 3e-4, 0.55, 0.34, 1.5e4, 0.0095},
                            {55.0, 1.06, 240, 2e-4, 0.50, 0.31, 1.3e4, 0.0085},
                            {73.416, 1.06, 240, 1.2e-4, 0.45, 0.28, 1.1e4, 0.0075},
                            {97.999, 1.06, 230, 8e-5, 0.45, 0.25, 0.95e4, 0.0065}};
// The bridge's top (the part above the waist): 60 g on its legs; up and down
// ~1500 Hz, sideways on the waist ~440 Hz (the bridge's own resonance), both
// well damped.
constexpr float kBridgeM = 0.06f;
const float kKy = kBridgeM * float(std::pow(2 * M_PI * 1500, 2)), kCy = kBridgeM * 2 * float(M_PI) * 1500 * 0.15f;
const float kKz = kBridgeM * float(std::pow(2 * M_PI * 440, 2)), kCz = kBridgeM * 2 * float(M_PI) * 440 * 0.45f;
// The tailpiece: 250 g hanging on its cord and the strings: ~48 Hz, Q ~12.
constexpr float kTailM = 0.25f;
const float kTailK = kTailM * float(std::pow(2 * M_PI * 48, 2)), kTailC = kTailM * 2 * float(M_PI) * 48 / 12;
// Along the strings: the bridge's top tipping (its legs bend: 2e5 N/m, well
// damped), the tailpiece on its cord (1.5e5 N/m), the strings' angles over
// the bridge (6 degrees on the long side, 15 behind).
constexpr float kBridgeKx = 2e5f, kBridgeCx = 60.0f, kCordK = 1.5e5f, kCordC = 30.0f, kSinA = 0.10f, kSinB = 0.26f;
// The fingerboard against the string: a stiff, lossy one-sided spring.
constexpr float kBoardK = 3e4f, kBoardC = 1.5f;
}  // namespace

Bass::Bass(double fs) : fs_(fs), dt_(1 / fs), body_(fs) {
  // The body's losses: its resonances ring for a few tenths of a second (Q
  // about 45 at the wood resonance, 45 at the air resonance).
  body_.setDamping(16.0, 1.6e-6, 9.0);
  kSymp_ = knob("PJ_BASS_SYMP", 1) > 0;
  kAfter_ = knob("PJ_BASS_AFTER", 1) > 0;
  kTail_ = knob("PJ_BASS_TAIL", 1) > 0;
  kBridge_ = knob("PJ_BASS_BRIDGE", 1) > 0;
  kTmod_ = knob("PJ_BASS_TMOD", 1) > 0;
  kBoard_ = knob("PJ_BASS_BOARD", 1) > 0;
  kAlive_ = knob("PJ_BASS_ALIVE", 1);
  width_ = float(knob("PJ_BASS_WIDTH", 0.7));  // the two sides' difference (0: mono)
  for (int s = 0; s < kStrings; ++s) {
    // The notches: 28 mm apart, the E string on the bass side.
    str_[s].notch = float(0.028 * (s - 1.5));
    // PJ_BASS_OLDGUT (0..1): plain gut as on 1950s records, where the
    // fundamental dies at 20-34 dB/s (measured: Ella Fitzgerald's "Body and
    // Soul", Louis Armstrong's "On the Sunny Side of the Street"); the
    // default strings ring about twice as long.
    BassString def = kGut[s];
    const double old = knob("PJ_BASS_OLDGUT", 0);
    def.loss0 *= 1 + 1.6 * old;
    def.loss1 *= 1 + 0.5 * old;
    setStringDef(s, def);
  }
  sumEAL_ = sumEALa_ = 0;
  for (const Str& s : str_) {
    sumEAL_ += float(s.def.ea / s.def.length);
    sumEALa_ += float(s.def.ea / kAfterLen_);
  }
}

void Bass::setStringDef(int si, const BassString& def) {
  Str& s = str_[si];
  s.def = def;
  s.len = s.lenFrom = s.lenTo = def.length;
  s.stopped = false;
  std::memset(s.finger, 0, sizeof s.finger);
  std::memset(s.q, 0, sizeof s.q);
  std::memset(s.v, 0, sizeof s.v);
  for (int k = 1; k <= kModes; ++k)
    // The bridge end moves by u: y = (fixed-end modes) + (x / L) u, and x / L
    // in sines is sum 2 (-1)^(k+1) / (k pi) sin(k pi x / L).
    s.bridgeIn[k - 1] = float(-2 * ((k & 1) ? 1.0 : -1.0) / (k * M_PI));
  tune(s);
  // Behind the bridge: the same string, kAfterLen long, from the tailpiece
  // (x = 0) to the bridge (x = La); wrapped and short: it dies fast.
  const double mu = def.tension / std::pow(2 * def.length * def.f0, 2);
  const double fa = def.f0 * def.length / kAfterLen_;
  for (int k = 1; k <= kAfter; ++k) {
    const double w = 2 * M_PI * k * fa * std::sqrt(1 + 4 * def.stiff * k * k);
    const bool on = w < 2 * M_PI * 0.45 * fs_;
    s.aw[k - 1] = float(w);
    s.acs[k - 1] = float(std::cos(w * dt_));
    s.asn[k - 1] = float(std::sin(w * dt_));
    s.adecay[k - 1] = on ? float(std::exp(-(5.0 + 4.0 * k) * dt_)) : 0.0f;
    s.aInB[k - 1] = float(-2 * ((k & 1) ? 1.0 : -1.0) / (k * M_PI));
    s.aInT[k - 1] = float(-2 / (k * M_PI));
    s.aSlopeB[k - 1] = float(k * M_PI / kAfterLen_ * ((k & 1) ? -1.0 : 1.0));
    s.aSlopeT[k - 1] = float(k * M_PI / kAfterLen_);
    s.aq[k - 1] = s.av[k - 1] = 0;
  }
  s.aInvModalMass = float(2 / (mu * kAfterLen_));
}

void Bass::tune(Str& s) {
  const BassString& d = s.def;
  const double len = s.len * std::exp2(-s.bendCents / 1200);
  const double mu = d.tension / std::pow(2 * d.length * d.f0, 2);  // kg/m
  const double f1 = d.f0 * d.length / len;
  // Each overtone's own character (fixed per string and overtone).
  auto rnd = [&](int k, int salt) {
    uint32_t h = uint32_t(k * 2654435761u) ^ uint32_t((int(d.f0 * 16) + salt) * 40503u);
    h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    return (h >> 8) * (2.0 / 16777216.0) - 1;  // -1 .. 1
  };
  const double open = s.stopped ? 1.0 : 0.33;
  for (int k = 1; k <= kModes; ++k) {
    const int i = k - 1;
    // A gut string is not uniform: every overtone a little off (up to 3 c).
    const double f = k * f1 * std::sqrt(1 + d.stiff * k * k) * (1 + kAlive_ * 0.0017 * rnd(k, 1));
    // The string's own losses (each overtone's a little its own); a stopped
    // string ends on a fingertip, which takes more of every mode, the high
    // ones most.
    // (An open string ends on the nut, hard wood: it rings longest.)
    const double loss = ((d.loss0 + d.loss1 * std::pow(k, 1.2)) * (s.stopped ? 1.2 : 0.7) + (s.stopped ? 0.3 + 0.12 * k : 0.0)) *
                        (1 + kAlive_ * 0.25 * rnd(k, 2));
    for (int dir = 0; dir < 2; ++dir) {
      // Along the top (z) the ends give differently than towards it (y):
      // z's pitch is up to 0.3 % off y's (the low overtones least), and it loses a little less (the
      // bridge passes it to the body less well).
      // (Most of the difference is the soft fingertip's: an open string, on
      // the nut, has a third of it, and its pitch stands still.)
      const double fd = dir == 0 ? f : f * (1 + kAlive_ * open * (0.0012 + 0.0008 * rnd(k, 3)) * std::min(1.6, 0.4 + 0.2 * k));
      const double w = 2 * M_PI * fd;
      const bool on = fd < 0.45 * fs_;
      s.w[dir][i] = float(w);
      s.wdt[dir][i] = float(w * dt_);
      s.cs[dir][i] = float(std::cos(w * dt_));
      s.sn[dir][i] = float(std::sin(w * dt_));
      s.decay[dir][i] = on ? float(std::exp(-loss * (dir == 0 ? 1.0 : 0.85) * dt_)) : 0.0f;
    }
    // The plane of the swing turns slowly: up to 0.3 turns a second, either
    // way (the low overtones least: the note itself must not come and go).
    s.turn[i] = float(kAlive_ * open * 2 * M_PI * (0.20 + 0.12 * rnd(k, 4)) * std::min(1.0, 0.1 + 0.15 * k) *
                      (rnd(k, 5) > 0 ? 1 : -1) * dt_);
    s.slope[i] = float(k * M_PI / len * ((k & 1) ? -1.0 : 1.0));  // d/dx sin(k pi x / L) at x = L
    // Stretch: dT / T = (EA / T) / (2 L) * integral of slope^2 = sum of this q^2.
    s.stretch[i] = float(d.ea / d.tension * 0.25 * std::pow(k * M_PI / len, 2));
  }
  s.invModalMass = float(2 / (mu * len));
  // The fingerboard under the string: from the stopping finger (or the nut)
  // to its end, kBoardEnd from the bridge; the string's height over it grows
  // from the finger to the action at its end. kBoard points along it.
  const double span = len - kBoardEnd_;
  for (int p = 0; p < kBoard; ++p) {
    // (Closer together near the finger, where the string lies lowest: there a
    // low action lets it graze the board through the note.)
    const double u = (p + 1.0) * (p + 1.0) / (kBoard * kBoard);
    const double x = span > 0.05 ? span * u : -1;
    s.clear[p] = x > 0 ? float(0.0002 + action_ * d.action * (span / (d.length - kBoardEnd_)) * u) : 1e9f;
    for (int k = 1; k <= kModes; ++k) s.board[p][k - 1] = x > 0 ? float(std::sin(k * M_PI * x / len)) : 0.0f;
  }
}

void Bass::reset() {
  for (Str& s : str_) {
    std::memset(s.q, 0, sizeof s.q);
    std::memset(s.v, 0, sizeof s.v);
    std::memset(s.aq, 0, sizeof s.aq);
    std::memset(s.av, 0, sizeof s.av);
    s.fingerClock = 1e9;
    s.active = false;
  }
  uy_ = vy_ = uz_ = vz_ = ay_ = az_ = ut_ = vt_ = at_ = prevVy_ = prevVz_ = 0;
  ux_ = vx_ = xt_ = vxt_ = 0;
  body_.reset();
}

void Bass::pluck(int si, double stoppedLength, double depth, double where, double angle, int finger) {
  Str& s = str_[si];
  const double len = stoppedLength > 0 ? stoppedLength : s.def.length;
  if (std::fabs(len - s.len) > 1e-6 || s.stopped != (stoppedLength > 0) || s.bendCents != 0) {
    // (A new place for the left hand: what still sounds there is kept by the
    // modes' amplitudes; the finger landing damps it anyway.)
    s.len = s.lenFrom = s.lenTo = len;
    s.glideT = 1e9;
    s.bendCents = 0;
    s.stopped = stoppedLength > 0;
    tune(s);
  }
  if (finger < 0) {  // the index and the middle fingertips take turns
    finger = nextFinger_;
    nextFinger_ ^= 1;
  }
  // The right hand's ways (calloused skin: narrower, quicker, less damping):
  //   0, 1  a fingertip, index or middle (across the string: fast, light,
  //         clean): a ~9 mm pad, off in ~1 ms; the middle reaches 12 mm
  //         further from the bridge and pulls ~10 % deeper;
  //   2     the jazz pull: the side of the index backed by the middle, as one:
  //         a ~22 mm pad of flesh, a deeper pull, off in ~1.6 ms: big and
  //         round, most of it in the low overtones;
  //   3     the thumb's pad, further up the fingerboard: ~28 mm, soft, off
  //         in ~4.5 ms: dark and blooming, no click;
  //   4     the flat finger: the side of the index along the string.
  double reach = 0, pad = 0.015 - 0.006 * skin_;
  s.slip = 0.003 - 0.002 * skin_;
  s.cut = 0.00015 + 0.0006 * (1 - skin_);  // the last step off the pad's edge (s)
  s.holdDamp = 260 - 80 * skin_;  // the finger landing on a sounding string stops it
  if (finger == 1) {
    reach = 0.012;
    depth *= 1.1;
  } else if (finger == 2) {
    reach = 0.006;
    pad = 0.026 - 0.004 * skin_;
    s.slip = 0.0036 - 0.002 * skin_;
    s.cut *= 1.5;
    depth *= 1.25;
  } else if (finger == 3) {
    reach = 0.08 * len;
    pad = 0.028;
    s.slip = 0.0045;
    s.cut = 0.0014;  // (the thumb's soft pad: no click)
    s.holdDamp = 300;
    depth *= 1.7;  // (the thumb is strong: as loud as the fingers for the same key)
  } else if (finger == 4) {
    // The flat finger: the whole side of the index lying along the string
    // (~35 mm of it), pulling sideways: the most string moved, warm and
    // woody; the flesh lets go a little slower than a fingertip.
    reach = 0.01;
    pad = 0.038 - 0.004 * skin_;
    s.slip = 0.0042 - 0.002 * skin_;
    s.cut = 0.0005 + 0.0006 * (1 - skin_);
    depth *= 1.4;
  }
  // The pad at `where` from the bridge: each mode's shape averaged over it.
  // The force that holds the string `depth` aside there:
  // F = T depth L / (x (L - x)).
  const double x = std::clamp(len * (1 - where) - reach, 0.05 * len, 0.97 * len);
  for (int k = 1; k <= kModes; ++k) {
    const double a = k * M_PI * pad / (2 * len);
    s.finger[k - 1] = float(std::sin(k * M_PI * x / len) * (a > 1e-9 ? std::sin(a) / a : 1.0));
  }
  s.fingerTarget = s.def.tension * depth * len / (x * (len - x));
  s.fingerAngle = angle;
  s.fingerClock = 0;
  s.touchClock = 1e9;
  s.boardClock = 0;
  s.damping = false;
  s.active = true;
}

void Bass::setLength(int si, double stoppedLength, double glide) {
  Str& s = str_[si];
  s.lenFrom = s.len;
  s.lenTo = stoppedLength > 0 ? stoppedLength : s.def.length;
  s.stopped = stoppedLength > 0;
  s.glideT = 0;
  s.glideDur = std::max(1e-3, glide);
  s.damping = false;
}

void Bass::touch(int si, int n, double secs) {
  Str& s = str_[si];
  for (int k = 1; k <= kModes; ++k) {
    // The fingertip's flesh at L / n takes each mode by its motion there.
    const double m = std::sin(k * M_PI / n);
    s.touch[k - 1] = float(std::exp(-900 * m * m * dt_));
  }
  s.touchClock = 0;
  s.touchDur = secs;
}

void Bass::pluckBehind(int si, double depth) {
  Str& s = str_[si];
  // Let go from `depth` aside at its middle: a triangle, in its modes.
  for (int k = 1; k <= kAfter; ++k) {
    s.aq[k - 1] = float(8 * depth / (k * k * M_PI * M_PI) * std::sin(k * M_PI / 2));
    s.av[k - 1] = 0;
  }
  s.active = true;
  s.behindClock = 0;
}

void Bass::setAction(double scale) {
  if (std::fabs(scale - action_) < 0.01) return;
  action_ = scale;
  for (Str& s : str_) tune(s);
}

void Bass::bend(int si, double cents) {
  Str& s = str_[si];
  if (!s.stopped) cents = 0;
  if (std::fabs(cents - s.bendCents) < 0.05) return;
  s.bendCents = cents;
  tune(s);
}

void Bass::damp(int si, double rate) {
  str_[si].dampRate = rate;
  str_[si].damping = true;
  str_[si].dampClock = 0;
}

void Bass::control() {
  const double bdt = 16 * dt_;
  for (Str& s : str_) {
    if (s.glideT < s.glideDur) {  // the left hand slides
      s.glideT += bdt;
      const double u = std::min(1.0, s.glideT / s.glideDur);
      s.len = s.lenFrom * std::pow(s.lenTo / s.lenFrom, 0.5 - 0.5 * std::cos(M_PI * u));
      tune(s);
    }
    // The left hand, having stopped a string, leaves it: it is open again
    // and free to ring along with the others.
    if (s.damping && (s.dampClock += bdt) > std::max(0.25, 6 / s.dampRate)) {
      s.damping = false;
      if (s.stopped || s.bendCents != 0) {
        s.stopped = false;
        s.bendCents = 0;
        s.len = s.lenFrom = s.lenTo = s.def.length;
        s.glideT = 1e9;
        tune(s);
      }
    }
    // Does it sound? (A string at rest is not computed, until the bridge
    // moves enough to wake it.)
    float e = 0;
    for (int i = 0; i < 16; ++i) e = std::max(e, std::fabs(s.v[0][i]) + std::fabs(s.v[1][i]));
    const bool finger = s.fingerClock < kRise_ + kHold_ + s.slip + s.cut + 0.01 || s.behindClock < 1.0;
    s.behindClock += bdt;
    s.active = finger || e > 1e-7f || std::fabs(ay_) + std::fabs(az_) > 1e-3f;
    // (A low action: the string may graze the board for as long as it swings wide.)
    s.boardOn = kBoard_ && s.boardClock < (action_ < 0.8 ? 1.5 : 0.4);
    s.boardClock += bdt;
  }
}

float Bass::sample(float testForce) {
  if (ctl_ == 0) control();
  ctl_ = (ctl_ + 1) & 15;
  const float dt = float(dt_);
  const float lever = float(kBridgeH_ / kFeet_);
  float Fy = 0, Fz = 0, moment = 0, Ftail = 0, stretchPull = 0;
  for (int si = 0; si < kStrings; ++si) {
    Str& s = str_[si];
    if (!s.active) continue;
    // The finger's force: up over kRise (a raised cosine), held, off over slip.
    double ff = 0;
    const double c = s.fingerClock;
    if (c < kRise_) ff = s.fingerTarget * 0.5 * (1 - std::cos(M_PI * c / kRise_));
    else if (c < kRise_ + kHold_) ff = s.fingerTarget;
    else if (c < kRise_ + kHold_ + s.slip + s.cut) {
      // The string slides along the pad (the force easing by a third), then
      // leaves its edge all at once (calloused skin: within ~0.15 ms; soft
      // flesh rounds it): that last step is the pluck's definition, its
      // overtones up to several kHz.
      const double u = (c - kRise_ - kHold_) / s.slip;
      ff = u < 1 ? s.fingerTarget * (1 - 0.35 * u)
                 : s.fingerTarget * 0.65 * 0.5 * (1 + std::cos(M_PI * (c - kRise_ - kHold_ - s.slip) / s.cut));
      // The skin slides off the string's winding: it catches and lets go on
      // the way (the pluck's small rasp), most for dry, calloused skin.
      rng_ = rng_ * 1664525u + 1013904223u;
      ff *= 1 + (0.25 + 0.35 * skin_) * ((rng_ >> 8) * (2.0 / 16777216.0) - 1);
    }
    const bool held = c < kRise_ + kHold_;
    s.fingerClock += dt_;
    const float drive[2] = {float(ff * std::sin(s.fingerAngle)) * s.invModalMass,
                            float(ff * std::cos(s.fingerAngle)) * s.invModalMass};
    // The finger on the string damps it while it holds, and the left hand
    // when it lies on it.
    const float extra = s.damping ? float(std::exp(-s.dampRate * dt_)) : held ? float(std::exp(-s.holdDamp * dt_)) : 1.0f;
    // The stretch: the tension's rise turns every mode a little faster.
    float eps = 0;
    if (kTmod_) {
      float e = 0;
      for (int i = 0; i < kModes; ++i) e += s.stretch[i] * (s.q[0][i] * s.q[0][i] + s.q[1][i] * s.q[1][i]);
      eps = 0.5f * std::min(e, 0.06f);  // (at most ~+50 c)
    }
    // The fingerboard (y only, on the side towards the body: y negative).
    alignas(64) float boardF[kModes];
    bool touching = false;
    if (s.boardOn) {
      for (int p = 0; p < kBoard; ++p) {
        if (s.clear[p] > 1) continue;
        float y = 0, vy = 0;
        for (int i = 0; i < kModes; ++i) {
          y += s.q[0][i] * s.board[p][i];
          vy += s.v[0][i] * s.board[p][i];
        }
        const float pen = -y - s.clear[p];
        if (pen > 0) {
          const float f = std::max(0.0f, kBoardK * pen - kBoardC * vy) * s.invModalMass;  // pushes towards +y
          if (!touching) {
            for (int i = 0; i < kModes; ++i) boardF[i] = 0;
            touching = true;
          }
          for (int i = 0; i < kModes; ++i) boardF[i] += f * s.board[p][i];
        }
      }
      if (touching) s.boardClock = 0;
    }
    // The bridge's motion drives the string (without PJ_BASS_SYMP: only a
    // string plucked lately: the others do not ring along).
    const bool touched = s.touchClock < s.touchDur;
    s.touchClock += dt_;
    const float acc[2] = {ay_, az_};
    const float accOn = kSymp_ || s.fingerClock < 10.0 ? 1.0f : 0.0f;
    float slope[2] = {0, 0};
    for (int d = 0; d < 2; ++d) {
      float* q = s.q[d];
      float* v = s.v[d];
      float sl = 0;
      const float a = acc[d] * accOn;
      for (int i = 0; i < kModes; ++i) {
        // Forces as a step in velocity, then the mode's own rotation and decay.
        float vv = v[i] + dt * (drive[d] * s.finger[i] + s.bridgeIn[i] * a);
        if (d == 0 && touching) vv += dt * boardF[i];
        const float x = q[i] * s.w[d][i];
        // (The extra turn del, kept a pure rotation: c^2 + s^2 = 1.)
        const float del = s.wdt[d][i] * eps;
        const float nrm = 1 / std::sqrt(1 + del * del);
        const float cc = (s.cs[d][i] - s.sn[d][i] * del) * nrm, ss = (s.sn[d][i] + s.cs[d][i] * del) * nrm;
        const float g = s.decay[d][i] * extra * (touched ? s.touch[i] : 1.0f);
        q[i] = g * (x * cc + vv * ss) / s.w[d][i];
        v[i] = g * (vv * cc - x * ss);
        sl += q[i] * s.slope[i];
      }
      slope[d] = sl;
    }
    // The plane of each mode's swing turns a little (y into z and back).
    for (int i = 0; i < kModes; ++i) {
      const float t = s.turn[i];
      const float qy = s.q[0][i], qz = s.q[1][i], vy = s.v[0][i], vz = s.v[1][i];
      s.q[0][i] = qy - t * qz;
      s.q[1][i] = qz + t * qy;
      s.v[0][i] = vy - t * vz;
      s.v[1][i] = vz + t * vy;
    }
    // The string pulls on the bridge: -T dy/dx at the bridge (with the
    // bridge's own displacement's share u / L).
    const float T = float(s.def.tension) * (1 + 2 * eps);
    stretchPull += float(s.def.tension) * 2 * eps;
    const float fy = -T * (slope[0] + uy_ / float(s.len));
    const float fz = -T * (slope[1] + uz_ / float(s.len));
    Fy += fy;
    Fz += fz;
    moment += fy * s.notch;
    // Behind the bridge.
    if (kAfter_) {
      float sb = 0, st = 0;
      for (int i = 0; i < kAfter; ++i) {
        const float vv = s.av[i] + dt * (s.aInB[i] * ay_ + s.aInT[i] * at_);
        const float x = s.aq[i] * s.aw[i];
        s.aq[i] = s.adecay[i] * (x * s.acs[i] + vv * s.asn[i]) / s.aw[i];
        s.av[i] = s.adecay[i] * (vv * s.acs[i] - x * s.asn[i]);
        sb += s.aq[i] * s.aSlopeB[i];
        st += s.aq[i] * s.aSlopeT[i];
      }
      const float lean = (uy_ - ut_) / float(kAfterLen_);
      const float fb = -float(s.def.tension) * (sb + lean);
      Fy += fb;
      moment += fb * s.notch;
      Ftail += float(s.def.tension) * (st + lean);
    }
  }
  // The tailpiece on its cord.
  if (kAfter_ && kTail_) {
    at_ = (Ftail - kTailK * ut_ - kTailC * vt_) / kTailM;
    vt_ += at_ * dt;
    ut_ += vt_ * dt;
  } else {
    at_ = 0;
  }
  // Along the strings: the bridge's top tips towards the neck or the tail
  // (ux, towards the tail positive), and the tailpiece moves on its cord
  // (xt). Every string is a spring along its length (E A / L): the long
  // parts pull the top towards the neck, the short parts behind the bridge
  // (stiff: 1/6 of the length) tie it to the tailpiece. The strings bend over
  // the bridge, so a higher tension presses it down, and the bridge rising
  // stretches them: the tailpiece, a mass between those springs and its
  // cord, takes energy from the bridge near its own resonance (the bass's
  // dead and live notes), and a hard pluck's stretch presses the bridge at
  // twice the note's frequency.
  if (kTail_) {
    const float dTa = sumEALa_ * (xt_ - ux_ + kSinB * uy_);   // the short parts' tension, all strings
    const float dTm = sumEAL_ * (ux_ + kSinA * uy_) + stretchPull;  // the long parts'
    const float ax = (dTa - dTm - kBridgeKx * ux_ - kBridgeCx * vx_) / kBridgeM;
    vx_ += ax * dt;
    ux_ += vx_ * dt;
    const float axt = (-dTa - kCordK * xt_ - kCordC * vxt_) / kTailM;
    vxt_ += axt * dt;
    xt_ += vxt_ * dt;
    Fy -= dTa * kSinB + dTm * kSinA;
  }
  // The bridge. Its feet's motion: y their mean, z their difference through
  // the bridge's height (it rocks).
  const float by = 0.5f * (body_.bassFootW() + body_.trebleFootW());
  const float bz = lever * (body_.bassFootW() - body_.trebleFootW());
  const float vby = 0.5f * (body_.bassFootV() + body_.trebleFootV());
  const float vbz = lever * (body_.bassFootV() - body_.trebleFootV());
  float feetY, feetZ;  // what goes down the legs
  if (kBridge_) {
    const float sy = -kKy * (uy_ - by) - kCy * (vy_ - vby);
    const float sz = -kKz * (uz_ - bz) - kCz * (vz_ - vbz);
    ay_ = (Fy + sy) / kBridgeM;
    az_ = (Fz + sz) / kBridgeM;
    vy_ += ay_ * dt;
    uy_ += vy_ * dt;
    vz_ += az_ * dt;
    uz_ += vz_ * dt;
    feetY = -sy;
    feetZ = -sz;
  } else {  // rigid, without mass: the top is where the feet put it
    ay_ = float((vby - prevVy_) * fs_);
    az_ = float((vbz - prevVz_) * fs_);
    prevVy_ = vby;
    prevVz_ = vbz;
    uy_ = by;
    uz_ = bz;
    feetY = Fy;
    feetZ = Fz;
  }
  // y presses both feet (each string more on its side); z rocks the bridge.
  const float tilt = moment / float(kFeet_);
  footForce_ = 0.5f * feetY + lever * feetZ - tilt;
  const float far = body_.step(footForce_ + testForce, 0.5f * feetY - lever * feetZ + tilt);
  // What is heard, as a jazz bass is recorded: a pickup at the bridge's bass
  // foot (its velocity), a microphone close to an f-hole, and a little of
  // the room (far away: the whole body's volume flow, where the plates and
  // the f-holes partly cancel under the air resonance). Each scaled to a
  // like level (a 6 mm pluck of an open string: ~1).
  // The two sides of the bass differ (bassbody.h: sideSound): left = this +
  // side, right = this - side.
  side_ = width_ * body_.sideSound() / 22.0f;
  // A microphone close to the top by the bridge hears the top's acceleration
  // there (the pluck's definition, the fingerboard's click).
  const float v = body_.bassFootV();
  const float topAcc = (v - prevFootV_) * float(fs_);
  prevFootV_ = v;
  return micTop_ * topAcc / 9.0f + micFar_ * far / 22.0f + micHole_ * body_.fholeSound() / 3.5f + micPickup_ * body_.bassFootV() / 0.047f;
}

}  // namespace pajupilli
