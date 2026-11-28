#include "rotary.h"
#include "vmath.h"
#include "heard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

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

// A peaking section (RBJ): gain dB at fc, its width q; b, a with a[0] = 1 dropped.
void peak(double fs, double fc, double q, double db, double* b, double* a) {
  const double A = fastPow(10.0, db / 40), w = 2 * M_PI * fc / fs, al = std::sin(w) / (2 * q), c = std::cos(w);
  const double a0 = 1 + al / A;
  b[0] = (1 + al * A) / a0;
  b[1] = -2 * c / a0;
  b[2] = (1 - al * A) / a0;
  a[0] = 1;
  a[1] = -2 * c / a0;
  a[2] = (1 - al / A) / a0;
}

// A 2nd-order section of its own q (bilinear high- or low-pass).
void pass2(double fs, double fc, double q, bool high, double* b, double* a) {
  const double w = 2 * M_PI * fc / fs, al = std::sin(w) / (2 * q), c = std::cos(w), a0 = 1 + al;
  if (high) { b[0] = (1 + c) / 2 / a0; b[1] = -(1 + c) / a0; b[2] = b[0]; }
  else { b[0] = (1 - c) / 2 / a0; b[1] = (1 - c) / a0; b[2] = b[0]; }
  a[0] = 1;
  a[1] = -2 * c / a0;
  a[2] = (1 - al) / a0;
}

double biquad(double x, const double* b, const double* a, double* s) {
  // (transposed direct form II; s[0], s[1])
  const double y = b[0] * x + s[0];
  s[0] = b[1] * x - a[1] * y + s[1];
  s[1] = b[2] * x - a[2] * y;
  return y;
}

}  // namespace

void Rotary::setup(double fs, uint32_t seed, double spread, bool log, double speed, double ratioSlow, double ratioFast) {
  fs_ = fs;
  dt_ = 1 / fs;
  a5000_ = 1 - std::exp(-2 * M_PI * 5000 * dt_);
  a1500_ = 1 - std::exp(-2 * M_PI * 1500 * dt_);
  a800_ = 1 - std::exp(-2 * M_PI * 800 * dt_);
  log_ = log;
  rng_ = seed * 2654435761u + 99u;
  auto urand = [this] { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; };
  // (a cabinet's parts off nominal, each by up to `spread` either way, its own draws: the motors'
  // strength, the crossover's parts, the horn's and the drum's radii, the
  // cabinet's inner reverberation)
  uint32_t srng = seed * 40503u + 12345u;
  auto off = [&srng, spread] { srng = srng * 1664525u + 1013904223u; return 1 + spread * ((srng >> 8) * (2.0 / 16777216.0) - 1); };
  // (the speeds are set outright per cabinet, the user's ratios: not spread, only the pulleys' +-3 %
  // below; their draws kept so the others stay as they were)
  off(), off(), off(), off();
  const double sHornSlow = 1, sHornFast = 1, sDrumSlow = 1, sDrumFast = 1;
  accH_ = off();
  accD_ = off();
  const double sXover = off();
  hornR_ = knob("PJ_ROTARY_HORNR", 0.17) * off();
  drumR_ = 0.20 * off();
  const double sT60 = off();
  // Each cabinet its own pulleys and belts: the speeds ±3 % (several cabinets: their speeds set
  // outright, chosen away from simple ratios to each other, which a few % would undo: exact).
  const double pulley = spread > 0 ? 0.0 : 0.03;
  hornSlow_ = knob("PJ_ROTARY_HORNSLOW", 0.80) * speed * (1 + pulley * urand()) * sHornSlow;
  hornFast_ = knob("PJ_ROTARY_HORNFAST", 6.67) * speed * (1 + pulley * urand()) * sHornFast;
  // (their own motors: the drum turns ~1.6 x slower than the horn, both on chorale and on tremolo;
  // PJ_ROTARY_RATIO, or the drum's speeds set outright)
  static const double kRatio = knob("PJ_ROTARY_RATIO", 1.6);
  const double rs = ratioSlow > 0 ? ratioSlow : kRatio, rf = ratioFast > 0 ? ratioFast : kRatio;
  drumSlow_ = knob("PJ_ROTARY_DRUMSLOW", 0.80 / rs) * speed * (1 + pulley * urand()) * sDrumSlow;
  drumFast_ = knob("PJ_ROTARY_DRUMFAST", 6.67 / rf) * speed * (1 + pulley * urand()) * sDrumFast;
  hornW_ = hornSlow_;
  drumW_ = drumSlow_;
  hornPh_ = M_PI * (1 + urand());
  drumPh_ = M_PI * (1 + urand());
  // The cabinet's flaws (research/rotary.md §15, the user's "imperfections related to the Rotary"):
  age_ = knob("PJ_ROTARY_AGE", 1.0);
  noise_ = knob("PJ_ROTARY_NOISE", 1.0);
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
    const double ang = (80 + 4 * urand()) * M_PI / 180, d = knob("PJ_ROTARY_MIC", 0.65) + 0.03 * urand();
    micX_[m] = d * std::cos(ang);
    micY_[m] = (m == 0 ? 1 : -1) * d * std::sin(ang);
    micEl_[m] = (m == 0 ? 12 : -8) * (1 + 0.2 * urand()) * M_PI / 180;  // (one a little above the horn, one below)
  }
  // - a loose panel or louvre: a resonance near 150-220 Hz that buzzes against its clearance under
  //   a loud bass;
  rattleW_ = 2 * M_PI * (150 + 70 * (1 + urand()) / 2);
  rattleQ_ = 15 + 10 * (1 + urand()) / 2;
  rattleGap_ = knob("PJ_ROTARY_RATTLEGAP", 0.52) / std::max(age_, 0.05);
  // - the crossover's capacitors aged: a few % off 800 Hz.
  const double xover = 800 * (1 + 0.08 * age_ * urand()) * sXover;
  butter(fs, xover, false, xa_, xb_);  // (the low-pass: xa_; both share the denominator xb_)
  double a[3];
  butter(fs, xover, true, xhb_, a);
  // The horn's own response on its axis (research/rotary.md §3.1: "±5 dB from 400 Hz to 10 kHz"),
  // a fixed filter: the source does not play with the horn (the driver is driven, nothing comes back
  // to the organ), so no wave in its bell is simulated, only what it does to the sound.
  //   - the flare's cutoff: below ~650 Hz the mouth hardly radiates (12 dB an octave, a little hump
  //     at it), under the crossover's own slope;
  //   - the mouth's reflection: part of the wave goes back down the ~0.3 m horn and returns, peaks
  //     and dips a few dB, ~500 Hz apart, over 0.9-2.7 kHz, fading as the mouth radiates better
  //     (ka toward 1);
  //   - the driver running out: 2nd order from ~7 kHz.
  // Each cabinet its own (±4 % in frequency x PJ_ROTARY_AGE: the driver's, the diffuser's seat);
  // PJ_ROTARY_HORNEQ scales the ripple and the hump (0: flat, as before).
  {
    static const double kEq = knob("PJ_ROTARY_HORNEQ", 1.0);
    const double v = 1 + 0.04 * age_ * urand();
    int n = 0;
    double a3[3];
    pass2(fs, 650 * v, 0.7 + 0.25 * kEq, true, hornEqB_[n], a3);
    std::copy(a3, a3 + 3, hornEqA_[n++]);
    pass2(fs, std::min(7000.0 * v, 0.45 * fs), 0.75, false, hornEqB_[n], a3);
    std::copy(a3, a3 + 3, hornEqA_[n++]);
    static const double kRip[4][3] = {{1000, 3.0, 3.0}, {1450, 3.0, -2.5}, {1950, 3.5, 2.0}, {2600, 3.5, -1.5}};
    for (const auto& r : kRip) {
      peak(fs, r[0] * v * (1 + 0.02 * age_ * urand()), r[1], kEq * r[2], hornEqB_[n], a3);
      std::copy(a3, a3 + 3, hornEqA_[n++]);
    }
  }
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
    fdnG_[i] = fastPow(10.0, -3.0 * fdnLen_[i] / (fs * knob("PJ_ROTARY_CABT60", 0.04) * sT60));
  }
}

double Rotary::normal() {
  rng_ = rng_ * 1664525u + 1013904223u;
  const double a = std::max((rng_ >> 8) * (1.0 / 16777216.0), 1e-12);
  rng_ = rng_ * 1664525u + 1013904223u;
  return std::sqrt(-2 * std::log(a)) * std::cos(2 * M_PI * (rng_ >> 8) * (1.0 / 16777216.0));
}

void Rotary::process(float in, float& left, float& right, float* rays) {
  const double kHornR = hornR_, kDrumR = drumR_;
  static const double kMic = knob("PJ_ROTARY_MIC", 0.4);
  static const double kDummy = knob("PJ_ROTARY_DUMMY", 0.16);
  static const double kBack[3] = {-4, -12, -18};  // the mouth's loss behind it, low / mid / high (dB)
  // (the beam 10^(kBack (1 - cos) / 40), as an exponential: half a power's work)
  static const double kBeam[3] = {kBack[0] * M_LN10 / 40, kBack[1] * M_LN10 / 40, kBack[2] * M_LN10 / 40};
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
    const double hT = brake_ ? 0 : (fast_ ? hornFast_ : hornSlow_ * boost_) * (1 + huntH_);
    const double dT = brake_ ? 0 : (fast_ ? drumFast_ : drumSlow_ * boost_) * (1 + huntD_);
    // Each rotor a mass on a belt, its motor's torque set by a slow PI control of its speed: the
    // error and its integral, the acceleration limited by what the motor can give (the horn 6 Hz
    // a second, the drum 1.2; no winding up against that limit). Speeding up the motor pulls
    // (natural frequency: horn 3 rad/s, drum 0.8); slowing down it is the friction and the
    // coasting, gentler (2 and 0.7); the brake firmer. Damped at 1.8 (PJ_ROTARY_ZETA): it eases
    // in and settles, barely past its speed (a PI on a mass undershoots more than its damping
    // says: at 0.85 the horn dipped to 0 on its way down to its slow speed). The horn reaches
    // its fast speed in ~1 s, the drum in ~4-5 s.
    static const double zeta = knob("PJ_ROTARY_ZETA", 1.8);
    auto rotor = [this, zeta = zeta](double& w, double& integ, double target, double wn, double wnDown, double maxAcc) {
      const bool up = target > w;
      const double n = up ? wn : wnDown;
      const double e = target - w, kp = 2 * zeta * n, ki = n * n;
      double a = kp * e + ki * (integ + e * dt_);
      const double lim = brake_ && !up ? 2 * maxAcc : maxAcc;
      const bool limited = std::fabs(a) > lim;
      a = std::clamp(a, -lim, lim);
      if (!limited) integ += e * dt_;
      w = std::max(0.0, w + a * dt_);
    };
    static const double hWn = knob("PJ_ROTARY_HORNWN", 3.0), hWnDown = knob("PJ_ROTARY_HORNWNDOWN", 2.0);
    static const double dWn = knob("PJ_ROTARY_DRUMWN", 0.8), dWnDown = knob("PJ_ROTARY_DRUMWNDOWN", 0.7);
    static const double hAcc = knob("PJ_ROTARY_HORNACC", 6), dAcc = knob("PJ_ROTARY_DRUMACC", 1.2);
    rotor(hornW_, hornI_, hT, hWn, hWnDown, hAcc * accH_);
    rotor(drumW_, drumI_, dT, dWn, dWnDown, dAcc * accD_);
    static const bool kLog = std::getenv("PJ_ROTARY_LOG") != nullptr;
    if (kLog && log_ && std::fmod(t_, 0.1) < dt_) say(stderr, "rotary t %.2f horn %.3f drum %.3f\n", t_, hornW_, drumW_);
    static const bool kLog2 = std::getenv("PJ_ROTARY_LOG2") != nullptr;  // (the second cabinet's)
    if (kLog2 && !log_ && std::fmod(t_, 0.1) < dt_) say(stderr, "rotary2 t %.2f horn %.3f drum %.3f\n", t_, hornW_, drumW_);
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
  static const double kT12 = std::tanh(0.12);  // (what does not change is found once: this, the one-poles' steps,
  // the drive's power, the rays' directions: they were every sample's)
  const double y = (std::tanh(x + 0.12) - kT12) / (1 - kT12 * kT12);
  sag_ += (std::fabs(y) - sag_) * dt_ / 0.1;
  // (a harder-driven amp: louder, and compressed as it saturates: the output grows only as
  // drive^0.7 at a low level, not drive)
  if (drive_ != driveFor_) {
    driveFor_ = drive_;
    drivePow_ = fastPow(std::max(drive_, 1e-3), 0.3);
  }
  const double amp = y / drivePow_;
  // The crossover.
  const double lo = biquad(amp, xa_, xb_, lp1_);
  double hi = biquad(amp, xhb_, xb_, hp1_);
  for (int i = 0; i < kHornEq; ++i) hi = biquad(hi, hornEqB_[i], hornEqA_[i], hornEqS_[i]);  // (the horn's response)
  // The horn's own wind: air swished off the spinning horn, a hiss of 1.5-5 kHz growing with
  // the square of its speed, sent out through the horn (so it swirls with it).
  double wind = 0;
  {
    const double sp = hornW_ / hornFast_;
    windLp_ += a5000_ * (normal() - windLp_);
    windHp_ += a1500_ * (windLp_ - windHp_);
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
    rattleHp_ += a800_ * (over - rattleHp_);
    buzz = 0.05 * (over - rattleHp_);
  }
  // (the two rotors turn opposite ways, as in a 122: the horn counter-clockwise seen from above,
  // the drum clockwise; their Doppler shifts and beams sweep against each other. PJ_ROTARY_SAMEWAY=1:
  // both one way)
  static const double kDrumDir = knob("PJ_ROTARY_SAMEWAY", 0) > 0 ? 1.0 : -1.0;
  const double hc = std::cos(hornPh_), hs = std::sin(hornPh_), dc = std::cos(drumPh_), ds = kDrumDir * std::sin(drumPh_);
  const double hx = kHornR * hc, hy = kHornR * hs;
  // The horn's images in the inside of its compartment (743 wide, 524 deep, ~300 mm high, the horn
  // in the middle): the sound off each surface comes out as if from the horn mirrored in it,
  // turning the other way round for the walls (its Doppler opposite), the same way for the lid and
  // the motor board (later by their path). The solid back panel reflects most, the louvred sides
  // and front little (their slats let the sound out), the lid and the board between
  // (PJ_ROTARY_INSIDE scales them). Each its own delay line read: the overtones meet their own
  // mirrored, oppositely shifted copies, a comb moving with the turn.
  // The walls (the user's look at a 122's horn compartment): louvres on all four sides, of
  // their sound ~25 % out, ~25 % turned up and down by the slats (to the lid and the board), ~50 %
  // back into the box; the four corners solid, thick wood: nothing out there, all back in. So a
  // path's way out depends on where it meets the wall: through a louvre (amplitude sqrt 0.25), or
  // a corner post (none, its edges softened over a centimetre). Four times a turn the horn's beam
  // goes into a corner and its direct sound is gone; what it lost comes round off the walls. The
  // long sides (front and back) are narrowed in their middle too: six openings a turn.
  struct Img { double x, y, z, fx, fy, r; };
  static const double kInside = knob("PJ_ROTARY_INSIDE", 1.0);
  // (the cabinet 737 x 533 mm outside, 29 x 21 in (b3world.com); its walls ~19 mm thick; the horn's
  // axis off the middle, PJ_ROTARY_OFFX / _OFFY (m, back and left): its walls nearer on one side,
  // each side its own delays)
  static const double kOffX = knob("PJ_ROTARY_OFFX", -0.03), kOffY = knob("PJ_ROTARY_OFFY", 0.04);
  static const double kFront = 0.248 - kOffX, kBackW = -0.248 - kOffX, kLeft = 0.35 - kOffY, kRight = -0.35 - kOffY;
  static const double kHalfW = 0.35, kHalfD = 0.248, kLid = 0.12, kBoard = 0.12;
  static const double kPost = knob("PJ_ROTARY_POST", 0.06);  // (a corner post's reach along each wall, m)
  static const double kOut = std::sqrt(0.25), kBack_ = std::sqrt(0.5), kSlat = std::sqrt(0.25);
  // How much of a path from (sx, sy) toward (dx, dy) gets out where it meets the wall.
  static const double kMid = knob("PJ_ROTARY_MIDPOST", 0.05);  // (the long sides' middle narrowing, half its width, m)
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
  const double leak = horn_[0].read(60) + horn_[1].read(60) + horn_[2].read(60);  // (the base's, not turning)
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
      for (int b = 0; b < 2; ++b) sum += g * std::exp(kBeam[b] * (1 - cosPsi)) * horn_[b].read(delay);
      // (the highs through the louvre's slots, 5 cm apart one over another: a microphone a little
      // above or below the horn hears them at slightly different times, a grating's comb)
      static const double kSlot = knob("PJ_ROTARY_SLOT", 0.05);
      const double step = kSlot * std::sin(micEl_[m]) / kC * fs_;
      double hiSum = 0;
      for (int k = 0; k < 6; ++k) hiSum += horn_[2].read(delay + (k - 2.5) * step);
      sum += g * std::exp(kBeam[2] * (1 - cosPsi)) * hiSum / 6;
    }
    for (const Img& im : img) {  // its images inside the cabinet (the mouth's beam, as the mid band's)
      const double vx = mx - im.x, vy = my - im.y, vz = -im.z, dist = std::sqrt(vx * vx + vy * vy + vz * vz);
      const double cosPsi = (im.fx * vx + im.fy * vy) / dist;
      sum += im.r * 0.4 / std::max(dist, 0.1) * std::exp(kBeam[1] * (1 - cosPsi)) * hornAll_.read(dist / kC * fs_ + 2);
    }
    // The twin horn's dummy bell, at the rotor's other end: sealed, but its throat leaks a little
    // (~-16 dB), a second source turning the other way round the listener (its Doppler opposite).
    {
      const double vx = mx + hx, vy = my + hy, dist = std::sqrt(vx * vx + vy * vy);
      const double cosPsi = (-hc * vx - hs * vy) / dist;
      const double g = kDummy * 0.4 / std::max(dist, 0.1) * out(-hx, -hy, vx / dist, vy / dist) / kOut;
      const double delay = dist / kC * fs_ + 2;
      for (int b = 0; b < 3; ++b) sum += g * std::exp(kBeam[b] * (1 - cosPsi)) * horn_[b].read(delay);
    }
    // The base's leakage, not turning.
    static const double kLeak = knob("PJ_ROTARY_LEAK", 0.25);
    sum += kLeak * leak;
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
    // (by ear, the user's: 40 dB under what it was, out of the way; PJ_ROTARY_RELAY, dB over that)
    static const double kRelay = 0.004 * fastPow(10.0, (knob("PJ_ROTARY_RELAY", 0) - 40) / 20);
    mech += noise_ * kRelay * click_ * (std::sin(clickPh_) + 0.5 * normal());
    click_ *= 1 - dt_ / 0.004;
  }
  left = float(mic[0] + fixedLo + 0.5 * (o[0] - o[2]) + buzz + mech);
  right = float(mic[1] + fixedLo + 0.5 * (o[1] - o[3]) + 0.8 * buzz + mech);
  // Into the room: toward each direction, the horn as seen from far along it (its distance along
  // the direction: the Doppler; the angle between its mouth and the direction: its beam), the
  // dummy bell opposite, the drum's scoop, the base's leakage, the fixed bass, the cabinet's tail.
  if (rays) {
    static const double kLeakR = knob("PJ_ROTARY_LEAK", 0.25), kDummyR = knob("PJ_ROTARY_DUMMY", 0.16);
    const double base = 0.6 / kC * fs_;  // (a common lag: every ray's moving delay stays positive)
    static const double kUx[kRays] = {std::cos(rayAngle(0)), std::cos(rayAngle(1)), std::cos(rayAngle(2)),
                                      std::cos(rayAngle(3)), std::cos(rayAngle(4)), std::cos(rayAngle(5))};
    static const double kUy[kRays] = {std::sin(rayAngle(0)), std::sin(rayAngle(1)), std::sin(rayAngle(2)),
                                      std::sin(rayAngle(3)), std::sin(rayAngle(4)), std::sin(rayAngle(5))};
    static_assert(kRays == 6, "kUx");
    // The beams first, every ray's twelve (the mouth's and the dummy bell's three bands, the six
    // images') as one run of exponentials: sixteen at a time, not 72 calls a sample.
    constexpr int kPer = 12;
    alignas(64) float beamArg[kRays * kPer], beam[kRays * kPer];
    static const float kBeamF[3] = {float(kBeam[0]), float(kBeam[1]), float(kBeam[2])};
    for (int k = 0; k < kRays; ++k) {
      const double ux = kUx[k], uy = kUy[k];
      const float cosPsi = float(hc * ux + hs * uy);
      float* a = beamArg + k * kPer;
      for (int b = 0; b < 3; ++b) {
        a[b] = kBeamF[b] * (1 - cosPsi);      // the mouth
        a[3 + b] = kBeamF[b] * (1 + cosPsi);  // the dummy bell, facing away
      }
      for (int q = 0; q < 6; ++q) a[6 + q] = kBeamF[1] * (1 - float(img[q].fx * ux + img[q].fy * uy));
    }
    for (int q = 0; q < kRays * kPer; ++q) beam[q] = vexpf(beamArg[q]);  // (src/vmath.h)
    for (int k = 0; k < kRays; ++k) {
      const double ux = kUx[k], uy = kUy[k];
      const float* bm = beam + k * kPer;
      double sum = 0;
      {  // the horn's mouth (out through a louvre, or not at a corner post)
        const double along = hx * ux + hy * uy;
        const double delay = base - along / kC * fs_ + 2, w = out(hx, hy, ux, uy) / kOut;
        for (int b = 0; b < 3; ++b) sum += w * bm[b] * horn_[b].read(delay);
      }
      {  // the dummy bell, at the other end of the rotor, facing away
        const double along = -(hx * ux + hy * uy);
        const double delay = base - along / kC * fs_ + 2, w = out(-hx, -hy, ux, uy) / kOut;
        for (int b = 0; b < 3; ++b) sum += w * kDummyR * bm[3 + b] * horn_[b].read(delay);
      }
      for (int q = 0; q < 6; ++q) {  // the horn's images inside the cabinet
        const Img& im = img[q];
        const double along = im.x * ux + im.y * uy;
        const double delay = base - along / kC * fs_ + std::fabs(im.z) / kC * fs_ + 2;
        sum += im.r * bm[6 + q] * hornAll_.read(delay);
      }
      sum += kLeakR * leak;
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

// The paths (src/rotary.h RotaryRoom; moved here from src/tonewheel.cc as it was): for each image
// of the cabinet (the walls 4 m either side, the back wall 1.5 m behind, its corners, the floor 1
// m down), the direction leaving the cabinet toward it (the path to the microphone, mirrored back
// by its walls), in the cabinet's own frame (its facing): the two rays either side of it blended
// (they are 60 degrees apart).
void RotaryRoom::setup(double fs, const std::vector<Place>& places) {
  cab_.assign(places.size(), Cab{});
  for (size_t c = 0; c < places.size(); ++c) {
    cab_[c].p = places[c];
    for (auto& ring : cab_[c].ring) ring.assign(2048, 0.0f);
  }
  pos_ = 0;
  static const double kD = knob("PJ_ROTARY_LISTENER", 4.0), kC = 343, kW = 4.0, kH = 1.0;
  // (the back wall 1.5 m behind the rearmost cabinet)
  double kB = 1.5;
  for (const Cab& cb : cab_) kB = std::max(kB, 1.5 - cb.p.x);
  static const double kRefl[7] = {1.0, 0.75, 0.75, 0.75, 0.56, 0.56, 0.6 * 0.8};
  // (the rays' angles in order round the cabinet, -180 .. 180, and which ray each is)
  static const double kAng[7] = {-180, -120, -60, 0, 60, 120, 180};
  static const int kRay[7] = {5, 4, 2, 0, 1, 3, 5};
  const int len = int(cab_[0].ring[0].size());
  for (Cab& cb : cab_)
    for (int m = 0; m < 2; ++m) {
      const double mx = kD, my = m == 0 ? 1 : -1;
      for (int p = 0; p < 7; ++p) {
        // the image: mirrored in y (side walls), in x (the back wall), both (corners)
        const bool fy = p == 1 || p == 2 || p == 4 || p == 5, fx = p == 3 || p == 4 || p == 5;
        const double wy = (p == 1 || p == 4) ? kW : -kW;
        const double ix = fx ? -2 * kB - cb.p.x : cb.p.x, iy = fy ? 2 * wy - cb.p.y : cb.p.y;
        double vx = mx - ix, vy = my - iy;
        const double flat = std::hypot(vx, vy);
        const double dist = p == 6 ? std::hypot(flat, 2 * kH) : flat;
        // (the microphone aimed out to its side: between omni and cardioid)
        const double arrive = std::atan2(iy - my, ix - mx) - (m == 0 ? 1 : -1) * 55 * M_PI / 180 - M_PI;
        const double aim = 0.6 + 0.4 * std::cos(arrive);
        // (the direction the sound leaves the cabinet: the path mirrored back by its walls)
        if (fy) vy = -vy;
        if (fx) vx = -vx;
        double a = (std::atan2(vy, vx) - cb.p.rot) * 180 / M_PI;
        a = std::remainder(a, 360.0);
        int seg = 0;
        while (seg < 5 && a > kAng[seg + 1]) ++seg;
        const double w = std::clamp((a - kAng[seg]) / 60, 0.0, 1.0);
        const double gain = kRefl[p] * aim / dist;
        cb.dly[p][m] = std::min(int(dist / kC * fs), len - 1);
        cb.ray[p][m][0] = kRay[seg];
        cb.ray[p][m][1] = kRay[seg + 1];
        cb.g[p][m][0] = gain * (1 - w);
        cb.g[p][m][1] = gain * w;
      }
    }
}

void RotaryRoom::step(const float (*rays)[Rotary::kRays], int nc, double& el, double& er, double& sum) {
  const int len = int(cab_[0].ring[0].size()), mask = len - 1;
  for (int c = 0; c < nc; ++c) {
    Cab& cb = cab_[c];
    for (int k = 0; k < Rotary::kRays; ++k) {
      cb.ring[k][pos_] = rays[c][k];
      sum += rays[c][k];
    }
    for (int p = 0; p < 7; ++p)
      for (int m = 0; m < 2; ++m) {
        const int at = (pos_ - cb.dly[p][m]) & mask;
        (m == 0 ? el : er) += cb.g[p][m][0] * cb.ring[cb.ray[p][m][0]][at] + cb.g[p][m][1] * cb.ring[cb.ray[p][m][1]][at];
      }
  }
  pos_ = (pos_ + 1) & mask;
}

void RotaryRoom::reset() {
  for (Cab& cb : cab_)
    for (auto& ring : cb.ring) std::fill(ring.begin(), ring.end(), 0.0f);
}

}  // namespace pajupilli
