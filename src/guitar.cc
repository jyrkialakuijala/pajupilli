#include "guitar.h"

#include <algorithm>
#include <cmath>

#include "knob.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
double urand(uint32_t& s) {
  s = s * 1664525u + 1013904223u;
  return (s >> 8) * (1.0 / 16777216.0);
}
}  // namespace

GuitarSpec classicalGuitar() {
  GuitarSpec g;
  g.name = "nylon-guitar";
  g.scale = 0.650;
  g.frets = 19;
  // The strings (research/guitars.md 1.1): three of plain nylon, three of
  // silvered copper wound on nylon floss. Nylon loses more than steel, the
  // high overtones most; the wound basses ring longer.
  const double lossT = knob("PJ_GUITAR_LOSS", 1.0), lossHigh = knob("PJ_GUITAR_LOSSHIGH", 1.0);
  const double f[6] = {82.41, 110.0, 146.83, 196.0, 246.94, 329.63};
  const double T[6] = {62, 68, 70, 61, 62, 83};
  const double B[6] = {1.0e-5, 1.0e-5, 1.0e-5, 9.3e-5, 3.7e-5, 1.6e-5};
  const double l0[6] = {1.1, 1.2, 1.3, 1.6, 1.8, 2.0}, l1[6] = {0.10, 0.11, 0.12, 0.30, 0.33, 0.36};
  for (int i = 0; i < 6; ++i) g.str[i] = {f[i], T[i], B[i], lossT * l0[i], lossT * lossHigh * l1[i]};
  // The body (3.1): the air with the top, the top's own, the back's, the
  // next plate and air modes; then the plates' many more, each lighter in
  // the sound and heavier under the bridge the higher it lies.
  const double massK = knob("PJ_GUITAR_BODYMASS", 1.0);
  g.body = {{100, 25, 0.9 * massK, 0.75, 0.05, 0.05},  {200, 22, 0.30 * massK, 1.00, -0.08, 0.08}, {236, 30, 1.1 * massK, 0.25, 0.10, -0.30},
            {290, 28, 0.60 * massK, 0.40, 0.20, 0.45}, {400, 32, 0.90 * massK, 0.40, -0.25, -0.20}};
  uint32_t rng = 1905;
  double fq = 455;
  for (int i = 0; i < 23; ++i) {
    const double t = i / 22.0;
    g.body.push_back({fq * (1 + 0.04 * (urand(rng) - 0.5)), 30 + 25 * urand(rng), massK * (0.56 + 1.8 * t) * (0.7 + 0.6 * urand(rng)),
                      (urand(rng) < 0.5 ? -1 : 1) * (0.5 - 0.32 * t) * (0.5 + urand(rng)), (urand(rng) - 0.5) * 0.6,
                      (urand(rng) < 0.5 ? -1 : 1) * (0.1 + 0.5 * urand(rng))});
    fq *= 1.092;  // (to 3.2 kHz)
  }
  g.alongDetune = knob("PJ_GUITAR_ALONGDETUNE", 0.0015);
  g.ring = knob("PJ_GUITAR_RING", 0.8);
  g.ringOvertones = knob("PJ_GUITAR_RING23", 0.55);
  g.ringOvertones45 = knob("PJ_GUITAR_RING45", 0.78);
  g.radiate = 9.6e-3 * knob("PJ_GUITAR_OUT", 1.0);
  return g;
}

Guitar::Guitar(double fs, const GuitarSpec& spec) : s_(spec), fs_(fs), dt_(1 / fs) {
  for (const GuitarSpec::Mode& m : s_.body) {
    Body b;
    const double w0 = 2 * kPi * m.f, a = w0 / (2 * m.q), w = std::sqrt(w0 * w0 - a * a);
    const double g = std::exp(-a * dt_), c = std::cos(w * dt_), sn = std::sin(w * dt_);
    b.a11 = float(g * (c + a / w * sn));
    b.a12 = float(g * sn / w);
    b.a21 = float(-g * (w0 * w0 / w) * sn);
    b.a22 = float(g * (c - a / w * sn));
    b.invMass = float(1 / m.mass);
    b.rad = float(s_.radiate * m.rad);
    b.side = float(s_.radiate * m.side);
    b.along = float(m.along);
    body_.push_back(b);
  }
  for (int i = 0; i < kStrings; ++i) tune(str_[i], i);
}

double Guitar::frequency(int s) const { return str_[s].f1; }

void Guitar::tune(Str& st, int index) {
  const GuitarString& d = s_.str[index];
  st.len = s_.scale * std::exp2(-st.fret / 12.0);
  st.f1 = d.f0 * s_.scale / st.len * std::exp2(st.bend / 1200);
  const double mu = d.tension / std::pow(2 * s_.scale * d.f0, 2);
  // (A fretted string ends on metal with a finger behind it: a little more
  // loss than on the nut.)
  const double end = st.fret > 0 ? 1.15 : 1.0;
  for (int k = 1; k <= kModes; ++k) {
    const int i = k - 1;
    const double f = k * st.f1 * std::sqrt(1 + d.B * k * k);
    const bool on = f < 0.45 * fs_;
    for (int dir = 0; dir < 2; ++dir) {
      const double w = 2 * kPi * f * (dir ? 1 + s_.alongDetune : 1.0);
      st.w[dir][i] = float(w);
      st.cs[dir][i] = float(std::cos(w * dt_));
      st.sn[dir][i] = float(std::sin(w * dt_));
    }
    // (By ear: the notes ring a little longer than the first numbers gave,
    // the first and second overtones most.)
    const double loss = (d.loss0 + d.loss1 * std::pow(k, 1.2)) * end * s_.ring *
                        (k == 2 || k == 3 ? s_.ringOvertones : k == 4 || k == 5 ? s_.ringOvertones45 : 1.0);
    // (Along the top the string's own loss is the same; it only keeps more
    // of its sound from the body.)
    st.decay[0][i] = on ? float(std::exp(-loss * dt_)) : 0.0f;
    st.decay[1][i] = on ? float(std::exp(-loss * dt_)) : 0.0f;
    st.slope[i] = float(d.tension * k * kPi / st.len * ((k & 1) ? -1.0 : 1.0));   // its pull on the bridge per unit swing (N/m)
    st.bridgeIn[i] = float(2.0 / (k * kPi) * ((k & 1) ? -1.0 : 1.0));              // the bridge's acceleration into it
  }
  st.invMass = float(2 / (mu * st.len));
}

void Guitar::shapeAt(Str& st, float* shape, double fromBridge, double width, double apart) {
  // (apart: the string held at two places that far from each other, each `width` wide.)
  const double x = std::clamp(1 - fromBridge / st.len, 0.02, 0.98);
  for (int k = 1; k <= kModes; ++k) {
    const double a = k * kPi * width / (2 * st.len);
    shape[k - 1] = float(std::sin(k * kPi * x) * (a > 1e-6 ? std::sin(a) / a : 1.0) * std::cos(k * kPi * apart / (2 * st.len)));
  }
}

void Guitar::fret(int s, int fret) {
  Str& st = str_[s];
  fret = std::clamp(fret, 0, s_.frets);
  if (fret == st.fret) return;
  st.fret = fret;
  st.bend = 0;
  tune(st, s);
}

void Guitar::bend(int s, double cents) {
  Str& st = str_[s];
  if (std::fabs(cents - st.bend) < 0.05) return;
  st.bend = cents;
  tune(st, s);
}

void Guitar::pluck(int s, double depth, double fromBridge, double angle, double nail, int strum, double clack) {
  Str& st = str_[s];
  st.plant = strum ? 0.0015 : kPlant_;
  st.flesh = strum == 2 ? 0.0038 : strum ? 0.0022 : kFlesh_;
  st.nailSlip = strum == 2 ? 0.0014 : strum ? 0.0008 : kNail_;
  // (What the stroke takes of what the string still sounded: a picking
  // finger stops it; the fingers' backs going down hardly touch it; the pad
  // coming up lies on it a moment and eats some.)
  st.hold = strum == 2 ? 200 : strum ? 25 : 300;
  st.wFlesh = strum == 2 ? 0.016 : strum ? 0.011 : 0.009;   // the pad going up; the fingers' backs going down
  st.wNail = strum == 2 ? 0.006 : strum ? 0.003 : 0.0015;   // the nail's face, not its edge
  st.fromBridge = std::clamp(fromBridge, 0.02, 0.5 * st.len);
  // The force that holds the string that far aside there.
  st.force = s_.str[s].tension * depth * st.len / (st.fromBridge * (st.len - st.fromBridge));
  st.sinA = std::sin(angle);
  st.cosA = std::cos(angle);
  st.nail = std::clamp(nail, 0.0, 1.0);
  // (A strumming hand never knocks. A nail is curved: picking, the string
  // meets its hollow side and its edge, stiff as an arch; strumming, the
  // string meets its rounded back, which flexes and slides.)
  st.clack = strum ? 0 : std::clamp(clack, 0.0, 1.0);
  shapeAt(st, st.shape, st.fromBridge, st.wFlesh);
  // Picking, the string lies in the nail's hollow side: the nail's curve
  // holds it sharply at two places a few millimetres apart. Strumming, it
  // meets the nail's rounded back: one broad, soft place.
  shapeAt(st, st.shapeN, st.fromBridge, st.wNail, strum ? 0 : 0.003);
  st.clock = 0;
  st.dampRate = 0;
  st.active = true;
  asleep_ = false;
  quiet_ = 0;
}

void Guitar::damp(int s, double rate) { str_[s].dampRate = rate; }

void Guitar::reset() {
  for (Str& st : str_) {
    for (int d = 0; d < 2; ++d) { std::fill(st.q[d], st.q[d] + kModes, 0.0f); std::fill(st.v[d], st.v[d] + kModes, 0.0f); }
    st.clock = 1e9;
    st.active = false;
  }
  for (Body& b : body_) b.x = b.v = 0;
  bridgeV_ = bridgeVz_ = 0;
  side_ = 0;
  asleep_ = true;
}

float Guitar::sample() {
  if (asleep_) { side_ = 0; return 0; }
  const float dt = float(dt_);
  // The strings' pull on the bridge: towards the top, and along it.
  float fy = 0, fz = 0;
  for (Str& st : str_) {
    if (!st.active) continue;
    float a = 0, b = 0;
    for (int i = 0; i < kModes; ++i) { a += st.slope[i] * st.q[0][i]; b += st.slope[i] * st.q[1][i]; }
    fy -= a;
    fz -= b;
  }
  // The body under the bridge: each resonance takes its own mix of the two
  // pulls, and moves the bridge its own way.
  float bv = 0, bz = 0, out = 0, sd = 0;
  for (Body& b : body_) {
    const float v = b.v + dt * (fy + b.along * fz) * b.invMass, x = b.x;
    b.x = b.a11 * x + b.a12 * v;
    const float vn = b.a21 * x + b.a22 * v;
    out += b.rad * (vn - b.v);
    sd += b.side * (vn - b.v);
    b.v = vn;
    bv += vn;
    bz += b.along * vn;
  }
  const float ab = (bv - bridgeV_) * float(fs_), az = (bz - bridgeVz_) * float(fs_);  // the bridge's acceleration, both ways
  bridgeV_ = bv;
  bridgeVz_ = bz;
  // The strings.
  bool any = false;
  for (Str& st : str_) {
    if (!st.active) continue;
    // The finger: it takes the string (what the string still sounded dies
    // under it), pulls, and lets go: off the flesh, then off the nail.
    // (fF: what the flesh holds of the string; fN: what the nail's edge holds.)
    float fF = 0, fN = 0, hold = 1;
    if (st.clock < 1e8) {
      const double t = st.clock;
      st.clock += dt_;
      const double fleshEnd = st.plant + st.flesh * (1 - 0.6 * st.nail), nailEnd = fleshEnd + st.nailSlip;
      if (t < st.plant) {
        fF = float(st.force * (0.5 - 0.5 * std::cos(kPi * t / st.plant)));
        hold = float(std::exp(-st.hold * dt_));
        // The nail's knock as it meets the string: against the pull, 0.6 ms.
        if (st.clack > 0 && t < 0.0006) fN = float(-1.6 * st.clack * st.force * std::sin(kPi * t / 0.0006));
      } else if (t < fleshEnd) {
        // The flesh gives way: the string comes to lie on the nail's edge alone.
        const double u = 0.5 - 0.5 * std::cos(kPi * (t - st.plant) / (fleshEnd - st.plant));
        fF = float(st.force * (1 - st.nail) * (1 - u) + st.force * st.nail * (1 - u));
        fN = float(st.force * st.nail * u);
        hold = float(std::exp(-0.4 * st.hold * (1 - u) * dt_));
      } else if (t < nailEnd) {
        const double u = (t - fleshEnd) / st.nailSlip;
        fN = float(st.force * st.nail * (0.5 + 0.5 * std::cos(kPi * u)));
      } else {
        st.clock = 1e9;
      }
    }
    const float sy = float(st.sinA) * st.invMass * dt, sz = float(st.cosA) * st.invMass * dt;
    const float along = az * dt, lay = st.dampRate > 0 ? float(std::exp(-st.dampRate * dt_)) : 1.0f;
    const float keep = hold * lay;
    float e = 0;
    for (int i = 0; i < kModes; ++i) {
      // Towards the top.
      const float pull = fF * st.shape[i] + fN * st.shapeN[i];
      float v = st.v[0][i] + sy * pull + st.bridgeIn[i] * ab * dt, q = st.q[0][i];
      float qn = st.cs[0][i] * q + st.sn[0][i] / st.w[0][i] * v, vn = -st.sn[0][i] * st.w[0][i] * q + st.cs[0][i] * v;
      st.q[0][i] = qn * st.decay[0][i] * keep;
      st.v[0][i] = vn * st.decay[0][i] * keep;
      // Along it.
      v = st.v[1][i] + sz * pull + st.bridgeIn[i] * along;
      q = st.q[1][i];
      qn = st.cs[1][i] * q + st.sn[1][i] / st.w[1][i] * v;
      vn = -st.sn[1][i] * st.w[1][i] * q + st.cs[1][i] * v;
      st.q[1][i] = qn * st.decay[1][i] * keep;
      st.v[1][i] = vn * st.decay[1][i] * keep;
      e += st.v[0][i] * st.v[0][i] + st.v[1][i] * st.v[1][i];
    }
    if (e < 1e-14f && st.clock > 1e8 && std::fabs(ab) <= 1e-4f) {
      st.active = false;
      for (int d = 0; d < 2; ++d) { std::fill(st.q[d], st.q[d] + kModes, 0.0f); std::fill(st.v[d], st.v[d] + kModes, 0.0f); }
    } else {
      any = true;
    }
  }
  // The other strings ring along: woken by the bridge.
  if (std::fabs(ab) > 1e-4f)
    for (Str& st : str_) st.active = true;
  out *= float(fs_);
  side_ = sd * float(fs_);
  if (!std::isfinite(out)) { reset(); return 0; }
  if (!any && std::fabs(out) < 1e-7f) {
    if (++quiet_ > int(0.2 * fs_)) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  return out;
}

}  // namespace pajupilli
