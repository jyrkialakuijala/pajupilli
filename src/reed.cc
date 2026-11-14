#include "reed.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {
namespace {

constexpr double kRho = Bore::kRho, kC = Bore::kC;
constexpr double kMouthVol = 60e-6;  // mouth cavity (m^3)
constexpr double kAirwayR = 2e5;     // lungs to mouth (Pa s / m^3)

double designDiameterAt(const Design& d, double x) {
  const double xm = x * 1e3;
  if (xm <= d.bore[0].x) return d.bore[0].d * 1e-3;
  for (int i = 1; i < d.nBore; ++i)
    if (xm <= d.bore[i].x) {
      const BorePoint &a = d.bore[i - 1], &b = d.bore[i];
      return 1e-3 * (a.d + (b.d - a.d) * (xm - a.x) / std::max(1e-9, b.x - a.x));
    }
  return d.bore[d.nBore - 1].d * 1e-3;
}

}  // namespace

Reed::Reed(const Instrument& inst) : Tube(inst), rp_(inst.design->reed) {
  const Design& d = *inst.design;
  {
    const double r = 48000 / fs_;
    glotA_ = 1 - std::pow(1 - 0.35, r);
    noiseA_ = 1 - std::pow(1 - 0.55, r);
    noiseHpA_ = 1 - std::pow(1 - 0.026, r);
  }
  kNoise_ = knob("PJ_REEDNOISE", 0.02);   // turbulence in the slit, x the flow
  kSweep_ = knob("PJ_REEDSWEEP", 1);      // the reed's swept flow
  kLipH_ = knob("PJ_LIPH", 0.6);          // lip force closes the reed: H ~ (lip)^-kLipH
  kGlotNoise_ = knob("PJ_GLOTNOISE", 0.5);
  kFr_ = knob("PJ_REEDFR", 1);  // the reed's resonance, x the design's
  kQr_ = knob("PJ_REEDQ", 1);   // its damping, x the design's
  // (Experiments: a double reed's duct loss, in place of the design's.)
  psi_ = rp_.psi;
  contact_ = knob("PJ_REEDCONTACT", rp_.contact);  // the blades' contact stiffness, x the reed's (0: the hard stop alone)
  contactAt_ = knob("PJ_REEDCONTACTAT", 0.5);  // where the blades start to meet (share of the closure)
  contactD_ = knob("PJ_REEDCONTACTD", 0.5);    // the contact's damping ratio
  meetAt_ = knob("PJ_REEDMEETAT", rp_.meetAt);  // where the blades begin to meet (share of the closure; 1: at the tip only)
  meet_ = knob("PJ_REEDMEET", 1);      // how the open width shrinks past it: (open share)^meet
  if (const double psi = knob("PJ_REEDPSI", -1); psi >= 0) psi_ = psi;
  // The reed at the top: the grid's first cell centre (the mouthpiece's or the
  // reed's own volume is the first stretch of the bore profile).
  const double half = 0.5 * kC / (Bore::kCourantMax * fs_);
  const Design* dp = &d;
  setGeometry([dp](double x) { return designDiameterAt(*dp, x); }, half, d.end * 1e-3, d.closedEnd);
  for (int i = 0; i < d.nHoles; ++i) {
    const HoleDef& h = d.holes[i];
    addHole(h.x * 1e-3, 0.5e-3 * h.d, h.t * 1e-3, h.padH * 1e-3, 0.5e-3 * h.cup, h.flags);
  }
  finishHoles();
  restHoles(d.notes[0].holes);
  addRadiators(false, 0);
  target_.tongue = 0;
  target_.lung = 0;
  cur_ = target_;
  controlStep();
}

void Reed::resetAir() {
  Tube::resetAir();
  x_ = v_ = dp_ = u_ = pm_ = 0;
  glotLp_ = noiseLp_ = noiseHp_ = 0;
}

void Reed::settle() {
  Tube::settle();
  // The tongue and the glottis where the attack starts (not closing from rest first).
  tongue_ = target_.tongue;
  cur_.glottis = target_.glottis;
  cur_.reed = target_.reed;  // the lips on the reed
  cur_.noise = target_.noise;
  cur_.air = target_.air;
}

void Reed::controlStep() {
  const double blockDt = 16 / fs_;
  const double a = 1 - std::exp(-blockDt / 0.002);  // 2 ms smoothing
  auto glide = [a](double& c, double t) { c += a * (t - c); };
  glide(cur_.lung, target_.lung);
  glide(cur_.reed, target_.reed);
  glide(cur_.noise, target_.noise);
  glide(cur_.air, target_.air);
  cur_.glottis += (1 - std::exp(-blockDt / 0.0005)) * (target_.glottis - cur_.glottis);
  tongue_ += (1 - std::exp(-blockDt / 0.001)) * (target_.tongue - tongue_);
  // The lips: more force closes the reed (smaller rest opening) and stiffens
  // and damps it; the tongue on the reed damps it further.
  const double lip = std::clamp(cur_.reed, 0.2, 3.0);
  H_ = rp_.h0 * std::pow(lip, -kLipH_);
  const double fr = rp_.fr * kFr_;
  const double wr = 2 * M_PI * fr * std::pow(lip, 0.25);
  const double mu = rp_.k / (2 * M_PI * fr) / (2 * M_PI * fr);  // mass per area (kg/m^2)
  invMu_ = 1 / mu;
  w2_ = wr * wr;
  // (The tongue rests on the reed once the player has the reed in the mouth;
  // at rest, never blown, the reed is free: the passive response, -imp. After
  // a note, a staccato tongue stop held a lightly damped reed (the shawm's)
  // closed but free to move: its swept flow and the bore rang up from silence.)
  if (cur_.lung > 1) played_ = true;
  g_ = kQr_ * rp_.qr * std::sqrt(lip) * wr + (played_ ? (1 - tongue_) * 3 * wr : 0);
  const double dt = 1 / fs_, hd = 0.5 * dt;
  const double det = 1 + hd * g_ + hd * hd * w2_;
  // (I - dt/2 A)^-1 (I + dt/2 A), A = [[0, 1], [-w2, -g]]; F enters v.
  const double i00 = (1 + hd * g_) / det, i01 = hd / det, i10 = -hd * w2_ / det, i11 = 1 / det;
  t00_ = i00 + i01 * (-hd * w2_);
  t01_ = i00 * hd + i01 * (1 - hd * g_);
  t10_ = i10 + i11 * (-hd * w2_);
  t11_ = i10 * hd + i11 * (1 - hd * g_);
  b0_ = i01 * dt;
  b1_ = i11 * dt;
  // The same on the lay (x past the contact point): stiffer and more damped.
  if (contact_ > 0) {
    const double w2c = w2_ * (1 + contact_), gc = g_ + 2 * contactD_ * std::sqrt(contact_ * w2_);
    const double detc = 1 + hd * gc + hd * hd * w2c;
    const double j00 = (1 + hd * gc) / detc, j01 = hd / detc, j10 = -hd * w2c / detc, j11 = 1 / detc;
    c00_ = j00 + j01 * (-hd * w2c);
    c01_ = j00 * hd + j01 * (1 - hd * gc);
    c10_ = j10 + j11 * (-hd * w2c);
    c11_ = j10 * hd + j11 * (1 - hd * gc);
    cb0_ = j01 * dt;
    cb1_ = j11 * dt;
  }
  updateHoles(blockDt);
  holeLosses();
}

float Reed::sample() {
  const double dt = 1 / fs_;
  // --- The reed: the pressure across it (last sample's) pushes it shut.
  {
    // (Experiments, off by default: PJ_REEDCONTACT.) A double reed's blades
    // need not slam shut: past contactAt of the closure they meet
    // progressively and the reed stiffens and is damped (Avanzini & van
    // Walstijn 2004: a reed curling onto its lay). A spring of contact x the
    // reed's own stiffness from the contact point on (implicit, trapezoidal:
    // stable however stiff). The hard stop below stays the last resort.
    // (On the oboe it took harmonics 6-10 down by 2-5 dB but sent 2-7 of 28
    // notes to another register or a reed squeal.)
    const double f = dp_ * invMu_;
    double x, v;
    const double xc = contactAt_ * H_;
    if (contact_ > 0 && x_ > xc) {
      const double fc = f + contact_ * w2_ * xc;
      x = c00_ * x_ + c01_ * v_ + cb0_ * fc;
      v = c10_ * x_ + c11_ * v_ + cb1_ * fc;
    } else {
      x = t00_ * x_ + t01_ * v_ + b0_ * f;
      v = t10_ * x_ + t11_ * v_ + b1_ * f;
    }
    x_ = x;
    v_ = v;
    // It beats on the lay (the other blade): it stops there.
    if (x_ > H_) {
      x_ = H_;
      if (v_ > 0) v_ = 0;
    }
    // Pulled open no further than about twice its rest opening.
    if (x_ < -2 * H_) {
      x_ = -2 * H_;
      if (v_ < 0) v_ = 0;
    }
  }
  // The tongue on the reed closes the slit.
  h_ = std::max(0.0, H_ - x_) * tongue_;
  // --- The mouth: lungs -> trachea -> glottis -> mouth, drained by the reed flow.
  const double ag = std::max(cur_.glottis, 1e-7);
  const double dpg = std::max(std::fabs(cur_.lung - pm_), 1.0);
  const double gg = ag * std::sqrt(2 / (kRho * dpg));
  const double gIn = 1 / (1 / gg + kAirwayR);
  static const double kMouthMul = knob("PJ_MOUTHVOL", 1);  // (experiments: the mouth's volume, x)
  const double cm = kMouthMul * kMouthVol / (kRho * kC * kC);
  const double vg = gIn * (cur_.lung - pm_) / ag;
  glotLp_ += glotA_ * (whiteNoise() - glotLp_);
  const double hiss = kGlotNoise_ * 0.5 * kRho * vg * vg * glotLp_;
  pm_ = (cm / dt * pm_ + gIn * (cur_.lung + hiss) - u_) / (cm / dt + gIn);
  // --- The bore, then the mouthpiece cell with the reed flow (trapezoidal).
  float pOld, jIn;
  stepBore(&pOld, &jIn);
  const double c = bore_.compliance(ec_), beta = 0.5 * dt / c;
  // Closing pushes air into the mouthpiece. (Experiments, off by default:
  // PJ_REEDMEETAT.) A double reed's blades meeting from the sides past
  // meetAt of the closure: the parts in contact sweep no more and pass no
  // flow, so the slit narrows and the flow fades out instead of stopping
  // dead at the tip. (On the oboe: 1-3 dB off harmonics 6-10; the reed still
  // crosses the last quarter of its closure within a sample or two.)
  double sweep = kSweep_ * rp_.sr, width = rp_.width;
  if (meetAt_ < 1) {
    const double xc = meetAt_ * H_;
    if (x_ > xc) {
      const double open = std::max(0.0, (H_ - x_) / (H_ - xc + 1e-12));
      sweep *= open;
      width *= std::pow(open, meet_);
    }
  }
  const double ur = sweep * v_;
  // Flow coefficient: Bernoulli through the slit, plus a double reed's duct.
  const double wh = width * h_;
  const double A = wh > 0 ? std::sqrt(2 / kRho) / std::sqrt(1 / (wh * wh) + psi_ / (rp_.sd * rp_.sd + 1e-30)) : 0;
  // (Tests: a small flow impulse, 1e-8 m^3/s per unit: the holes stay linear.)
  const double R = pm_ - pOld - beta * (jIn + ur + 1e-8 * testDp_);
  const double bA = beta * A;
  const double s = 0.5 * (-bA + std::sqrt(bA * bA + 4 * std::fabs(R)));
  const double D = R >= 0 ? s * s : -s * s;
  double u = A * (R >= 0 ? s : -s);
  testDp_ = 0;
  // Turbulence in the slit: a little noise on the flow.
  noiseLp_ += noiseA_ * (whiteNoise() - noiseLp_);
  noiseHp_ += noiseHpA_ * (noiseLp_ - noiseHp_);
  const double un = kNoise_ * (0.5 + cur_.air) * cur_.noise / 0.01 * (noiseLp_ - noiseHp_) * u;
  const double pmid = pm_ - D + beta * un;
  bore_.s.p()[ec_] = float(2 * pmid - pOld);
  u_ = u + un;
  dp_ = pm_ - pmid;
  return finish(radiate(0));
}

}  // namespace pajupilli
