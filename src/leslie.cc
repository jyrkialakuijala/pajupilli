#include "leslie.h"

#include <algorithm>
#include <cmath>

#include "knob.h"

namespace pajupilli {

namespace {

constexpr double kC = 343;

// A 2nd-order Butterworth section (bilinear): b, a with a[0] = 1 dropped.
void butter(double fs, double fc, bool high, double* b, double* a) {
  const double k = std::tan(M_PI * fc / fs), q = M_SQRT1_2, n = 1 / (1 + k / q + k * k);
  if (high) { b[0] = n; b[1] = -2 * n; b[2] = n; }
  else { b[0] = k * k * n; b[1] = 2 * b[0]; b[2] = b[0]; }
  a[0] = 1;
  a[1] = 2 * (k * k - 1) * n;
  a[2] = (1 - k / q + k * k) * n;
}

double biquad(double x, const double* b, const double* a, double* s) {
  // (transposed direct form II; s[0], s[1])
  const double y = b[0] * x + s[0];
  s[0] = b[1] * x - a[1] * y + s[1];
  s[1] = b[2] * x - a[2] * y;
  return y;
}

}  // namespace

float Leslie::Ring::read(double delay) const {
  const double rp = double(w) - delay;
  const int i = int(std::floor(rp));
  const double t = rp - i;
  const float y0 = b[(i - 1) & mask], y1 = b[i & mask], y2 = b[(i + 1) & mask], y3 = b[(i + 2) & mask];
  // (4-point Lagrange)
  const double c0 = -t * (t - 1) * (t - 2) / 6, c1 = (t + 1) * (t - 1) * (t - 2) / 2;
  const double c2 = -(t + 1) * t * (t - 2) / 2, c3 = (t + 1) * t * (t - 1) / 6;
  return float(c0 * y0 + c1 * y1 + c2 * y2 + c3 * y3);
}

void Leslie::setup(double fs, uint32_t seed) {
  fs_ = fs;
  dt_ = 1 / fs;
  rng_ = seed * 2654435761u + 99u;
  auto urand = [this] { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; };
  // Each cabinet its own pulleys and belts: the speeds ±3 %.
  hornSlow_ = knob("PJ_LESLIE_HORNSLOW", 0.80) * (1 + 0.03 * urand());
  hornFast_ = knob("PJ_LESLIE_HORNFAST", 6.67) * (1 + 0.03 * urand());
  drumSlow_ = knob("PJ_LESLIE_DRUMSLOW", 0.67) * (1 + 0.03 * urand());
  drumFast_ = knob("PJ_LESLIE_DRUMFAST", 5.90) * (1 + 0.03 * urand());
  hornW_ = hornSlow_;
  drumW_ = drumSlow_;
  hornPh_ = M_PI * (1 + urand());
  drumPh_ = M_PI * (1 + urand());
  // The cabinet's flaws (research/leslie.md §15, the user's "imperfections related to the Leslie"):
  age_ = knob("PJ_LESLIE_AGE", 1.0);
  noise_ = knob("PJ_LESLIE_NOISE", 1.0);
  // - an off-centre pulley and a horn not quite balanced: each rotor runs a few % faster on one
  //   side of its turn than the other; its belt (~2.6 x the rotor's period, its seam and stretch)
  //   wobbles it on top, at a rate unrelated to the turn;
  hornEcc_ = age_ * 0.03 * (0.5 + 0.5 * (1 + urand()));
  hornEccPh_ = M_PI * (1 + urand());
  drumEcc_ = age_ * 0.012 * (0.5 + 0.5 * (1 + urand()));
  drumEccPh_ = M_PI * (1 + urand());
  beltEcc_[0] = age_ * 0.01 * (1 + urand());
  beltEcc_[1] = age_ * 0.006 * (1 + urand());
  // - the microphones never quite symmetric (±4°, ±3 cm);
  for (int m = 0; m < 2; ++m) {
    // (at the horn compartment's sides, in front of their louvres, as players mike it)
    const double ang = (80 + 4 * urand()) * M_PI / 180, d = knob("PJ_LESLIE_MIC", 0.65) + 0.03 * urand();
    micX_[m] = d * std::cos(ang);
    micY_[m] = (m == 0 ? 1 : -1) * d * std::sin(ang);
    micEl_[m] = (m == 0 ? 12 : -8) * (1 + 0.2 * urand()) * M_PI / 180;  // (one a little above the horn, one below)
  }
  // - a loose panel or louvre: a resonance near 150-220 Hz that buzzes against its clearance under
  //   a loud bass;
  rattleW_ = 2 * M_PI * (150 + 70 * (1 + urand()) / 2);
  rattleQ_ = 15 + 10 * (1 + urand()) / 2;
  rattleGap_ = knob("PJ_LESLIE_RATTLEGAP", 0.52) / std::max(age_, 0.05);
  // - the crossover's capacitors aged: a few % off 800 Hz.
  const double xover = 800 * (1 + 0.08 * age_ * urand());
  butter(fs, xover, false, xa_, xb_);  // (the low-pass: xa_; both share the denominator xb_)
  double a[3];
  butter(fs, xover, true, xhb_, a);
  aLo_ = 1 - std::exp(-2 * M_PI * 1500 / fs);
  aMid_ = 1 - std::exp(-2 * M_PI * 4000 / fs);
  aDrum_ = 1 - std::exp(-2 * M_PI * 200 / fs);
  for (Ring& r : horn_) r.init(int(0.01 * fs) + 8);
  hornAll_.init(int(0.01 * fs) + 8);
  drum_.init(int(0.01 * fs) + 8);
  static const int kLen[4] = {223, 293, 367, 449};
  for (int i = 0; i < 4; ++i) {
    fdnLen_[i] = int(kLen[i] * fs / 48000);
    fdn_[i].assign(fdnLen_[i], 0.0f);
    fdnG_[i] = std::pow(10.0, -3.0 * fdnLen_[i] / (fs * knob("PJ_LESLIE_CABT60", 0.04)));
  }
}

double Leslie::normal() {
  rng_ = rng_ * 1664525u + 1013904223u;
  const double a = std::max((rng_ >> 8) * (1.0 / 16777216.0), 1e-12);
  rng_ = rng_ * 1664525u + 1013904223u;
  return std::sqrt(-2 * std::log(a)) * std::cos(2 * M_PI * (rng_ >> 8) * (1.0 / 16777216.0));
}

void Leslie::process(float in, float& left, float& right, float* rays) {
  static const double kHornR = knob("PJ_LESLIE_HORNR", 0.17), kDrumR = 0.20, kMic = knob("PJ_LESLIE_MIC", 0.4);
  static const double kDummy = knob("PJ_LESLIE_DUMMY", 0.16);
  static const double kBack[3] = {-4, -12, -18};  // the mouth's loss behind it, low / mid / high (dB)
  // The rotors: their motors pull them towards the set speed through belts and their own inertia.
  {
    // The motors hunt: never quite their speed, the weaker slow (chorale) motor less steady (~3 %)
    // than the fast (~1 %); wandering over half a second.
    if (++ctl_ >= 48) {
      ctl_ = 0;
      const double cdt = 48 * dt_, sigma = age_ * (fast_ ? 0.01 : 0.03);
      huntH_ += -huntH_ * cdt / 0.5 + sigma * std::sqrt(2 * cdt / 0.5) * normal();
      huntD_ += -huntD_ * cdt / 0.8 + sigma * 0.7 * std::sqrt(2 * cdt / 0.8) * normal();
    }
    const double hT = brake_ ? 0 : (fast_ ? hornFast_ : hornSlow_) * (1 + huntH_);
    const double dT = brake_ ? 0 : (fast_ ? drumFast_ : drumSlow_) * (1 + huntD_);
    static const double hUp = knob("PJ_LESLIE_HORNUP", 0.5), hDown = knob("PJ_LESLIE_HORNDOWN", 0.8);
    static const double dUp = knob("PJ_LESLIE_DRUMUP", 3.5), dDown = knob("PJ_LESLIE_DRUMDOWN", 2.5);
    hornW_ += (hT - hornW_) * dt_ / (hT > hornW_ ? hUp : brake_ ? 1.0 : hDown);
    drumW_ += (dT - drumW_) * dt_ / (dT > drumW_ ? dUp : brake_ ? 4.0 : dDown);
    hornPh_ += 2 * M_PI * hornW_ * dt_ * (1 + hornEcc_ * std::cos(hornPh_ + hornEccPh_) + beltEcc_[0] * std::cos(beltPh_[0]));
    drumPh_ += 2 * M_PI * drumW_ * dt_ * (1 + drumEcc_ * std::cos(drumPh_ + drumEccPh_) + beltEcc_[1] * std::cos(beltPh_[1]));
    beltPh_[0] = std::fmod(beltPh_[0] + 2 * M_PI * hornW_ * 2.6 * dt_, 2 * M_PI);
    beltPh_[1] = std::fmod(beltPh_[1] + 2 * M_PI * drumW_ * 2.3 * dt_, 2 * M_PI);
    if (hornPh_ > 2 * M_PI) hornPh_ -= 2 * M_PI;
    if (drumPh_ > 2 * M_PI) drumPh_ -= 2 * M_PI;
  }
  // The amp: a soft clip, a little asymmetric (the push-pull pair a little off balance), its
  // drive easing as the supply sags under a loud passage.
  double x = in * drive_ / (1 + 0.6 * sag_);
  const double y = (std::tanh(x + 0.12) - std::tanh(0.12)) / (1 - std::tanh(0.12) * std::tanh(0.12));
  sag_ += (std::fabs(y) - sag_) * dt_ / 0.1;
  // (a harder-driven amp: louder, and compressed as it saturates: the output grows only as
  // drive^0.7 at a low level, not drive)
  const double amp = y / std::pow(std::max(drive_, 1e-3), 0.3);
  // The crossover.
  const double lo = biquad(amp, xa_, xb_, lp1_);
  const double hi = biquad(amp, xhb_, xb_, hp1_);
  // The horn's own wind: air swished off the spinning horn, a hiss of 1.5-5 kHz growing with
  // the square of its speed, sent out through the horn (so it swirls with it).
  double wind = 0;
  {
    const double sp = hornW_ / hornFast_;
    windLp_ += (1 - std::exp(-2 * M_PI * 5000 * dt_)) * (normal() - windLp_);
    windHp_ += (1 - std::exp(-2 * M_PI * 1500 * dt_)) * (windLp_ - windHp_);
    wind = noise_ * 4e-4 * sp * sp * (windLp_ - windHp_);
  }
  // The horn's bands, into their delay lines.
  hornLo_ += aLo_ * (hi - hornLo_);
  const double rest = hi + wind - hornLo_;
  hornMid_ += aMid_ * (rest - hornMid_);
  horn_[0].push(float(hornLo_));
  horn_[1].push(float(hornMid_));
  horn_[2].push(float(rest - hornMid_));
  hornAll_.push(float(hi + wind));
  // The drum: its fixed low part, its turning upper part.
  drumLo_ += aDrum_ * (lo - drumLo_);
  drum_.push(float(lo - drumLo_));
  // A loose panel: driven by the deep bass (under 200 Hz), buzzing where it swings past its
  // clearance (x'' + (w / Q) x' + w^2 x = w^2 bass: Q times the bass at its resonance).
  double buzz = 0;
  {
    const double a = rattleW_ * rattleW_ * (drumLo_ - rattleX_) - rattleW_ / rattleQ_ * rattleV_;
    rattleV_ += a * dt_;
    rattleX_ += rattleV_ * dt_;
    const double over = rattleX_ - std::clamp(rattleX_, -rattleGap_, rattleGap_);
    rattleHp_ += (1 - std::exp(-2 * M_PI * 800 * dt_)) * (over - rattleHp_);
    buzz = 0.05 * (over - rattleHp_);
  }
  const double hc = std::cos(hornPh_), hs = std::sin(hornPh_), dc = std::cos(drumPh_), ds = std::sin(drumPh_);
  const double hx = kHornR * hc, hy = kHornR * hs;
  // The horn's images in the inside of its compartment (743 wide, 524 deep, ~300 mm high, the horn
  // in the middle): the sound off each surface comes out as if from the horn mirrored in it,
  // turning the other way round for the walls (its Doppler opposite), the same way for the lid and
  // the motor board (later by their path). The solid back panel reflects most, the louvred sides
  // and front little (their slats let the sound out), the lid and the board between
  // (PJ_LESLIE_INSIDE scales them). Each its own delay line read: the overtones meet their own
  // mirrored, oppositely shifted copies, a comb moving with the turn.
  // The walls (the user's look at a 122's horn compartment): louvres on all four sides, of
  // their sound ~25 % out, ~25 % turned up and down by the slats (to the lid and the board), ~50 %
  // back into the box; the four corners solid, thick wood: nothing out there, all back in. So a
  // path's way out depends on where it meets the wall: through a louvre (amplitude sqrt 0.25), or
  // a corner post (none, its edges softened over a centimetre). Four times a turn the horn's beam
  // goes into a corner and its direct sound is gone; what it lost comes round off the walls. The
  // long sides (front and back) are narrowed in their middle too: six openings a turn.
  struct Img { double x, y, z, fx, fy, r; };
  static const double kInside = knob("PJ_LESLIE_INSIDE", 1.0);
  // (the cabinet 737 x 533 mm outside, 29 x 21 in (b3world.com); its walls ~19 mm thick; the horn's
  // axis off the middle, PJ_LESLIE_OFFX / _OFFY (m, back and left): its walls nearer on one side,
  // each side its own delays)
  static const double kOffX = knob("PJ_LESLIE_OFFX", -0.03), kOffY = knob("PJ_LESLIE_OFFY", 0.04);
  static const double kFront = 0.248 - kOffX, kBackW = -0.248 - kOffX, kLeft = 0.35 - kOffY, kRight = -0.35 - kOffY;
  static const double kHalfW = 0.35, kHalfD = 0.248, kLid = 0.12, kBoard = 0.12;
  static const double kPost = knob("PJ_LESLIE_POST", 0.06);  // (a corner post's reach along each wall, m)
  static const double kOut = std::sqrt(0.25), kBack_ = std::sqrt(0.5), kSlat = std::sqrt(0.25);
  // How much of a path from (sx, sy) toward (dx, dy) gets out where it meets the wall.
  static const double kMid = knob("PJ_LESLIE_MIDPOST", 0.05);  // (the long sides' middle narrowing, half its width, m)
  auto out = [&](double sx, double sy, double dx, double dy) {
    double tx = 1e9, ty = 1e9;
    if (dx > 1e-12) tx = (kFront - sx) / dx;
    if (dx < -1e-12) tx = (kBackW - sx) / dx;
    if (dy > 1e-12) ty = (kLeft - sy) / dy;
    if (dy < -1e-12) ty = (kRight - sy) / dy;
    const bool longSide = tx < ty;  // (the front or the back: the long sides)
    const double t = std::min(tx, ty);
    // (where it meets the wall, from the cabinet's own middle)
    const double ex = std::fabs(sx + t * dx + kOffX), ey = std::fabs(sy + t * dy + kOffY);
    // (along the wall from the nearest corner, less the corner post's reach; on a long side also
    // from its middle, less the narrowing's: six openings a turn)
    double clear = (longSide ? kHalfW - ey : kHalfD - ex) - kPost;
    if (longSide) clear = std::min(clear, ey - kMid);
    const double edge = std::clamp(clear / 0.01 + 0.5, 0.0, 1.0);
    return kOut * edge * edge * (3 - 2 * edge);
  };
  const Img img[6] = {
      {hx, 2 * kLeft - hy, 0, hc, -hs, kBack_ * kInside},      // the left side
      {hx, 2 * kRight - hy, 0, hc, -hs, kBack_ * kInside},     // the right side
      {2 * kBackW - hx, hy, 0, -hc, hs, kBack_ * kInside},     // the back
      {2 * kFront - hx, hy, 0, -hc, hs, kBack_ * kInside},     // the front
      {hx, hy, 2 * kLid, hc, hs, kSlat * kInside},             // the lid (the slats' share)
      {hx, hy, -2 * kBoard, hc, hs, kSlat * kInside},          // the motor board
  };
  float mic[2] = {0, 0};
  for (int m = 0; m < 2 && !rays; ++m) {  // (the close microphones: not when radiating into a room)
    const double mx = micX_[m], my = micY_[m];
    (void)kMic;
    double sum = 0;
    // The horn: the direct path and the two side walls' images (mirrored in y, turning the
    // other way), each with its Doppler and the mouth's beam.
    {  // the horn's mouth: its direct sound, its beam in three bands
      const double vx = mx - hx, vy = my - hy, dist = std::sqrt(vx * vx + vy * vy);
      const double cosPsi = (hc * vx + hs * vy) / dist;
      const double delay = dist / kC * fs_ + 2;
      const double g = 0.4 / std::max(dist, 0.1) * out(hx, hy, vx / dist, vy / dist) / kOut;  // (a louvre's way out: 1)
      for (int b = 0; b < 2; ++b) sum += g * std::pow(10.0, kBack[b] * (1 - cosPsi) / 40) * horn_[b].read(delay);
      // (the highs through the louvre's slots, 5 cm apart one over another: a microphone a little
      // above or below the horn hears them at slightly different times, a grating's comb)
      static const double kSlot = knob("PJ_LESLIE_SLOT", 0.05);
      const double step = kSlot * std::sin(micEl_[m]) / kC * fs_;
      double hiSum = 0;
      for (int k = 0; k < 6; ++k) hiSum += horn_[2].read(delay + (k - 2.5) * step);
      sum += g * std::pow(10.0, kBack[2] * (1 - cosPsi) / 40) * hiSum / 6;
    }
    for (const Img& im : img) {  // its images inside the cabinet (the mouth's beam, as the mid band's)
      const double vx = mx - im.x, vy = my - im.y, vz = -im.z, dist = std::sqrt(vx * vx + vy * vy + vz * vz);
      const double cosPsi = (im.fx * vx + im.fy * vy) / dist;
      sum += im.r * 0.4 / std::max(dist, 0.1) * std::pow(10.0, kBack[1] * (1 - cosPsi) / 40) * hornAll_.read(dist / kC * fs_ + 2);
    }
    // The twin horn's dummy bell, at the rotor's other end: sealed, but its throat leaks a little
    // (~-16 dB), a second source turning the other way round the listener (its Doppler opposite).
    {
      const double vx = mx + hx, vy = my + hy, dist = std::sqrt(vx * vx + vy * vy);
      const double cosPsi = (-hc * vx - hs * vy) / dist;
      const double g = kDummy * 0.4 / std::max(dist, 0.1) * out(-hx, -hy, vx / dist, vy / dist) / kOut;
      const double delay = dist / kC * fs_ + 2;
      for (int b = 0; b < 3; ++b) sum += g * std::pow(10.0, kBack[b] * (1 - cosPsi) / 40) * horn_[b].read(delay);
    }
    // The base's leakage, not turning.
    static const double kLeak = knob("PJ_LESLIE_LEAK", 0.25);
    sum += kLeak * (horn_[0].read(60) + horn_[1].read(60) + horn_[2].read(60));
    // The drum's scoop.
    {
      const double dx = kDrumR * dc, dy = kDrumR * ds;
      const double vx = mx - dx, vy = my - dy, dist = std::sqrt(vx * vx + vy * vy);
      const double cosPsi = (dc * vx + ds * vy) / dist;
      sum += (0.3 + 0.7 * 0.5 * (1 + cosPsi)) * 1.2 * drum_.read(dist / kC * fs_ + 2);
    }
    mic[m] = float(sum);
  }
  // The drum's fixed low part, a little later (the lower compartment), to both.
  const float fixedLo = drumFixed_[(dfw_ - 60) & 2047];
  drumFixed_[dfw_] = float(drumLo_);
  dfw_ = (dfw_ + 1) & 2047;
  // The cabinet's tail (Householder FDN).
  double o[4], s = 0;
  for (int i = 0; i < 4; ++i) {
    o[i] = fdn_[i][fdnPos_[i]];
    s += o[i];
  }
  const double feed = rays ? 0.3 * rayMean_ : 0.15 * (mic[0] + mic[1]);  // (in a room: what the rays carried)
  for (int i = 0; i < 4; ++i) {
    fdn_[i][fdnPos_[i]] = float(fdnG_[i] * (o[i] - 0.5 * s) + feed);
    fdnPos_[i] = (fdnPos_[i] + 1) % fdnLen_[i];
  }
  // The machinery: the running motor's hum (120 Hz, its rotation's rumble), the amp's hum, and the
  // speed relay's click when the switch moves.
  t_ += dt_;
  if (fast_ != lastFast_) {
    click_ = 1;
    lastFast_ = fast_;
  }
  double mech = noise_ * (3e-5 * (fast_ ? 1.0 : 0.5) * std::sin(2 * M_PI * 120 * t_) + 2e-5 * std::sin(2 * M_PI * 28.75 * t_ + 1) +
                          1.5e-5 * std::sin(2 * M_PI * 60 * t_ + 2));
  if (click_ > 1e-4) {
    clickPh_ += 2 * M_PI * 2300 * dt_;
    mech += noise_ * 0.004 * click_ * (std::sin(clickPh_) + 0.5 * normal());
    click_ *= 1 - dt_ / 0.004;
  }
  left = float(mic[0] + fixedLo + 0.5 * (o[0] - o[2]) + buzz + mech);
  right = float(mic[1] + fixedLo + 0.5 * (o[1] - o[3]) + 0.8 * buzz + mech);
  // Into the room: toward each direction, the horn as seen from far along it (its distance along
  // the direction: the Doppler; the angle between its mouth and the direction: its beam), the
  // dummy bell opposite, the drum's scoop, the base's leakage, the fixed bass, the cabinet's tail.
  if (rays) {
    static const double kLeakR = knob("PJ_LESLIE_LEAK", 0.25), kDummyR = knob("PJ_LESLIE_DUMMY", 0.16);
    const double base = 0.6 / kC * fs_;  // (a common lag: every ray's moving delay stays positive)
    for (int k = 0; k < kRays; ++k) {
      const double ux = std::cos(rayAngle(k)), uy = std::sin(rayAngle(k));
      double sum = 0;
      {  // the horn's mouth (out through a louvre, or not at a corner post)
        const double along = hx * ux + hy * uy, cosPsi = hc * ux + hs * uy;
        const double delay = base - along / kC * fs_ + 2, w = out(hx, hy, ux, uy) / kOut;
        for (int b = 0; b < 3; ++b) sum += w * std::pow(10.0, kBack[b] * (1 - cosPsi) / 40) * horn_[b].read(delay);
      }
      {  // the dummy bell, at the other end of the rotor, facing away
        const double along = -(hx * ux + hy * uy), cosPsi = -(hc * ux + hs * uy);
        const double delay = base - along / kC * fs_ + 2, w = out(-hx, -hy, ux, uy) / kOut;
        for (int b = 0; b < 3; ++b) sum += w * kDummyR * std::pow(10.0, kBack[b] * (1 - cosPsi) / 40) * horn_[b].read(delay);
      }
      for (const Img& im : img) {  // the horn's images inside the cabinet
        const double along = im.x * ux + im.y * uy, cosPsi = im.fx * ux + im.fy * uy;
        const double delay = base - along / kC * fs_ + std::fabs(im.z) / kC * fs_ + 2;
        sum += im.r * std::pow(10.0, kBack[1] * (1 - cosPsi) / 40) * hornAll_.read(delay);
      }
      sum += kLeakR * (horn_[0].read(60) + horn_[1].read(60) + horn_[2].read(60));
      {  // the drum's scoop
        const double along = kDrumR * (dc * ux + ds * uy), cosPsi = dc * ux + ds * uy;
        sum += (0.3 + 0.7 * 0.5 * (1 + cosPsi)) * 1.2 * drum_.read(base - along / kC * fs_ + 2);
      }
      rays[k] = float(0.4 * sum + fixedLo + 0.25 * (o[k % 4] - o[(k + 2) % 4]) + buzz + mech);
    }
    double mean = 0;
    for (int k = 0; k < kRays; ++k) mean += rays[k];
    rayMean_ = mean / kRays;
  }
}

}  // namespace pajupilli
