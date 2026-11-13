#include "flute.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {
namespace {

constexpr double kRho = Bore::kRho, kC = Bore::kC;
// The concert flute; the other instruments are resized from it.
constexpr double kCork = 0.0175;     // cork face to embouchure centre (m)
constexpr double kEmbLong = 11.6e-3; // embouchure hole along the bore axis (m)
constexpr double kEmbRin = 6.4e-3;   // undercut: equivalent radius at the bore (m)
constexpr double kEmbChimney = 4.3e-3;
constexpr double kHoleT = 1.9e-3;    // tube wall + drawn chimney, every tone hole
constexpr double kTongueArea = 50e-6;   // tongue valve fully open (m^2)
constexpr double kMouthVol = 60e-6;     // mouth cavity (m^3)
constexpr double kAirwayR = 2e5;        // lungs to mouth (Pa s / m^3)
constexpr double kChannel = 3e-3;       // lip channel length, jet inertia (m)

// Concert flute bore diameter (mm) at e mm from the embouchure centre (Pearl PF-661).
double fluteBore(double e) {
  double d;
  if (e < 0) d = 16.8 + (17.07 - 16.8) * (e + 17.5) / 17.5;
  else if (e < 113.5) d = 17.07 + (18.8 - 17.07) * e / 113.5;
  else if (e < 150.5) d = 18.8;
  else if (e < 154.5) d = 19.8;  // the head joint socket
  else d = 19.0;
  return d;
}

// Tone holes, Pearl PF-661 [Dickens]: x from the embouchure centre (mm),
// diameter, key opening, key cup diameter (mm), French perforation.
struct HoleSpec { double x, d, padH, cup; bool perf; };
constexpr HoleSpec kSpec[kHoles] = {
    {200.7, 7.4, 1.6, 13, false},   // 1  D#6 trill
    {218.0, 7.8, 1.6, 13, false},   // 2  D6 trill
    {234.4, 7.0, 1.9, 13, false},   // 3  C#5 / register vent
    {266.9, 13.2, 2.6, 19, false},  // 4  C5 (thumb)
    {286.1, 13.2, 2.2, 19, false},  // 5  B4
    {307.1, 13.2, 2.5, 19, true},   // 6  Bb4 (LH2)
    {330.3, 13.2, 2.4, 19, true},   // 7  A4 (LH3)
    {352.0, 13.4, 2.4, 19, false},  // 8  G#4 (G# key, normally closed)
    {352.5, 13.4, 2.5, 19, false},  // 9  G#4 duplicate
    {376.9, 14.2, 2.1, 19, false},  // 10 G4
    {402.6, 14.2, 2.3, 19, true},   // 11 F#4 (RH1)
    {429.9, 14.2, 2.5, 19, true},   // 12 F4 (RH2)
    {458.7, 14.2, 2.4, 19, true},   // 13 E4 (RH3)
    {490.3, 15.4, 2.8, 20.6, false},// 14 D#4 (Eb key, normally closed)
    {524.3, 15.4, 2.8, 20.6, false},// 15 D4
    {557.3, 15.4, 2.8, 20.6, false},// 16 C#4
    {592.3, 15.4, 2.8, 20.6, false},// 17 C4 (B foot only; the B key closes it)
};

}  // namespace

double Flute::boreDiameter(double x) const {
  return inst_.bore * 1e-3 * fluteBore((x - cork_) / inst_.scale * 1e3);
}

Flute::Flute(const Instrument& inst) : Tube(inst) {
  {  // noise filters (set at 48 kHz) for this rate
    const double r = 48000 / fs_;
    noiseA_ = 1 - std::pow(1 - 0.55, r);
    noiseHpA_ = 1 - std::pow(1 - 0.026, r);
    glotA_ = 1 - std::pow(1 - 0.35, r);
  }
  kJetSign_ = knob("PJ_JET_SIGN", 1);
  kEtaSign_ = knob("PJ_ETA_SIGN", 1);
  kEmbLen_ = knob("PJ_EMB_LEN", 2) * 1e-3 * inst.emb;
  kAlpha_ = knob("PJ_ALPHA", 0.3);
  kConv_ = knob("PJ_CONV", 0.175);
  kNoiseGain_ = knob("PJ_NOISE", 0.3);
  kJetModel_ = int(knob("PJ_JETMODEL", 1));
  kRel_ = knob("PJ_REL", 0);
  kXiHp_ = knob("PJ_XIHP", 100);
  kVortex_ = knob("PJ_VORTEX", 1);
  kJetDisp_ = int(knob("PJ_JETDISP", 1));
  kCpHi_ = knob("PJ_CPHI", 0.45);
  {  // the all-pass's share of the delay at the reference (see sample())
    const double phi = M_PI * (1 - std::min(0.95, kConv_ / kCpHi_));
    const double x = std::tan(0.5 * phi);   // w T at the reference: 2 atan(w T) = phi
    kDispT_ = x / (M_PI * kConv_);          // T = kDispT W / Uj
  }
  kAgc_ = knob("PJ_AGC", 2);  // swing limit, in jet widths
  agcA_ = 1 - std::exp(-1 / (knob("PJ_AGCT", 4) * 1e-3 * fs_));
  kTurbIn_ = knob("PJ_TURBIN", 4);  // jet turbulence gain (it now passes the jet's low-pass)
  kEdge_ = knob("PJ_EDGE", 4);      // direct edge hiss
  kSpread_ = knob("PJ_SPREAD", 0.1);
  kJetLp_ = knob("PJ_JETLP", 2);
  kEtaMax_ = knob("PJ_ETAMAX", 3) * 1e-3 * inst.emb;  // a hard limit for safety (PJ_AGC limits the swing)
  kEmbR_ = knob("PJ_EMBR", 0.01);
  kGlotNoise_ = knob("PJ_GLOTNOISE", 0.5);
  if (inst.design) {
    buildDesign();
  } else {
    cork_ = kCork * inst.scale;
    head_ = knob("PJ_HEAD", inst.head) * 1e-3;
    foot_ = knob("PJ_FOOT", inst.foot) * 1e-3 * inst.scale - head_;
    embLong_ = kEmbLong * inst.emb;
    embRin_ = kEmbRin * inst.emb;
    embChimney_ = kEmbChimney * inst.emb;
    holeT_ = kHoleT * inst.bore;
    channel_ = kChannel;
    embA_ = 0.5 * boreDiameter(cork_);
    setGeometry([this](double x) { return boreDiameter(x); }, cork_, cork_ + foot_, false);
    for (int i = 0; i < kHoles; ++i) {
      const HoleSpec& sp = kSpec[i];
      // Normally closed keys: the trill keys, G# and Eb.
      const int flags = (sp.perf ? kPerforated : 0) | (i == 0 || i == 1 || i == 7 || i == 13 ? kNormallyClosed : 0);
      addHole(cork_ + sp.x * 1e-3 * inst.scale - head_, 0.5e-3 * sp.d * inst.bore, holeT_,
              sp.padH * 1e-3 * inst.bore, 0.5e-3 * sp.cup * inst.bore, flags,
              !(inst.dFoot && i >= 14) && (i < 16 || inst.bFoot));
    }
    finishHoles();
    // At rest the player holds G: the left hand down, the right hand up (and the
    // little finger on the D# key), so a first note moves fewer keys.
    restHoles("xxxxxxxxxooooooo");
    // Radiators: the embouchure, the foot's open end, each hole cell.
    addRadiators(true, cork_);
  }
  cur_ = target_;
  embouchureGeometry();
  jetCoefs();
}

// The bore of a design: piecewise linear (mm), constant beyond its ends.
double designDiameter(const Design& d, double x) {
  const double xm = x * 1e3;
  if (xm <= d.bore[0].x) return d.bore[0].d * 1e-3;
  for (int i = 1; i < d.nBore; ++i)
    if (xm <= d.bore[i].x) {
      const BorePoint &a = d.bore[i - 1], &b = d.bore[i];
      return 1e-3 * (a.d + (b.d - a.d) * (xm - a.x) / std::max(1e-9, b.x - a.x));
    }
  return d.bore[d.nBore - 1].d * 1e-3;
}

void Flute::buildDesign() {
  const Design& d = *inst_.design;
  const JetParams& j = d.jet;
  channel_ = inst_.kind == Kind::kDuct ? j.channel : kChannel;
  if (d.pipes) {  // a pan flute: the pipe is chosen per note
    buildPipe(0);
    return;
  }
  // The window (a duct: the window cut in the head; end-blown: the open top,
  // partly covered by the lower lip) at d.excite from the top.
  cork_ = d.excite * 1e-3;
  embChimney_ = j.chimney;
  const Design* dp = &d;
  setGeometry([dp](double x) { return designDiameter(*dp, x); }, cork_, d.end * 1e-3, d.closedEnd);
  embA_ = 0.5 * designDiameter(d, cork_);
  embLong_ = inst_.kind == Kind::kDuct ? j.w : 2 * embA_;
  // The window's opening at the bore: a duct's window as cut; an end's rim: the bore.
  embRin_ = inst_.kind == Kind::kDuct ? std::sqrt(j.w * j.window / M_PI) : embA_;
  for (int i = 0; i < d.nHoles; ++i) {
    const HoleDef& h = d.holes[i];
    addHole(h.x * 1e-3, 0.5e-3 * h.d, h.t * 1e-3, h.padH * 1e-3, 0.5e-3 * h.cup, h.flags);
  }
  finishHoles();
  restHoles(d.notes[0].holes);
  addRadiators(true, cork_);
}

void Flute::buildPipe(int k) {
  const Design& d = *inst_.design;
  const PipeDef& p = d.pipes[k];
  pipe_ = k;
  // (A bottle: the jet meets the neck's rim; the neck widens into the body over 2 mm.)
  const double r = 0.5e-3 * (p.neck > 0 ? p.neckD : p.d);
  // The jet meets the rim at the top; the window branch sits half a cell in.
  cork_ = 0.5 * Bore::kC / (Bore::kCourantMax * fs_);
  embA_ = r;
  embRin_ = r;
  embLong_ = 2 * r;
  embChimney_ = d.jet.chimney;
  const double dd = p.d * 1e-3, nl = p.neck * 1e-3, nd = p.neckD * 1e-3;
  setGeometry([dd, nl, nd](double x) {
    if (nl <= 0 || x >= nl + 2e-3) return dd;
    return x <= nl ? nd : nd + (dd - nd) * (x - nl) / 2e-3;
  }, cork_, cork_ + p.len * 1e-3, true);
  nHoles_ = 0;
  finishHoles();
  addRadiators(true, cork_);
}

void Flute::selectPipe(int midi) {
  const Design* d = inst_.design;
  if (!d || !d->pipes) return;
  int k = 0;
  for (int i = 0; i < d->nPipes; ++i)
    if (d->pipes[i].midi == midi - inst_.transpose) k = i;
  if (k == pipe_) return;
  // Another pipe under the lips: it starts from rest (the old one rings on
  // briefly in the room, not here).
  buildPipe(k);
  std::memset(&bore_.s, 0, sizeof(bore_.s));
  eq_ = el_ = 0;
  std::fill(radPrev_, radPrev_ + kMaxHoles + 2, 0.0f);
  std::fill(radPrev2_, radPrev2_ + kMaxHoles + 2, 0.0f);
  embouchureGeometry();
}

void Flute::embouchureGeometry() {
  // The lower lip covers part of the hole: the open window is
  // uncovered x 11.6 mm at the top; the undercut makes it wider at the bore.
  const double sw = std::max(4e-6 * inst_.emb * inst_.emb, cur_.uncovered * embLong_);
  const double sbot = M_PI * embRin_ * embRin_;
  const double rw = std::sqrt(sw / M_PI);
  const double a = embA_;
  const double m = kRho * (embChimney_ / std::sqrt(sw * sbot) +
                           innerCorrection(embRin_, embRin_ / a) / sbot + kEmbLen_ * cur_.face / sw);
  esw_ = sw;
  eg_ = 1 / m;
  // Loss in the window: the lower lip over the hole leaves a lossy slit
  // (viscous / turbulent); brings the played resonances' Q to ~25-40.
  ers_ = kEmbR_ * kRho * kC / sw;
  err_ = 1.35 * kRho * kC / sw;            // face and lip flange the window
  ehl_ = sw / (kRho * 0.82 * rw);
  ecnl_ = kVortex_ * kRho / (2 * 0.6 * 0.6 * sw * sw);  // vortex shedding (vena contracta 0.6)
}

void Flute::controlStep() {
  const double blockDt = 16 / fs_;
  const double a = 1 - std::exp(-blockDt / 0.002);  // 2 ms smoothing
  auto glide = [a](double& c, double t) { c += a * (t - c); };
  glide(cur_.lung, target_.lung);
  glide(cur_.lipH, target_.lipH);
  glide(cur_.lipWidth, target_.lipWidth);
  glide(cur_.jetLen, target_.jetLen);
  glide(cur_.uncovered, target_.uncovered);
  glide(cur_.y0, target_.y0);
  glide(cur_.face, target_.face);
  glide(cur_.noise, target_.noise);
  glide(cur_.air, target_.air);
  cur_.glottis += (1 - std::exp(-blockDt / 0.0005)) * (target_.glottis - cur_.glottis);
  const double at = 1 - std::exp(-blockDt / 0.001);
  tongue_ += at * (target_.tongue - tongue_);
  if (duct()) {  // the windway and the window are the instrument's
    const JetParams& j = inst_.design->jet;
    cur_.lipH = j.h;
    cur_.lipWidth = j.width;
    cur_.jetLen = j.w;
    cur_.uncovered = j.window;
    cur_.face = 0;
  }
  embouchureGeometry();
  updateHoles(blockDt);
  holeLosses();
  jetCoefs();
}

// The jet lowpass's step x and the edge hiss's centre a (rad per sample), from
// the jet's speed.
static inline double jetLpX(double kJetLp, double kConv, double ujc, double b, double fs) {
  const double fc = kJetLp * kConv * ujc / (2 * M_PI * b);
  return 2 * M_PI * std::min(fc, 0.4 * fs) / fs;
}
static inline double hissA(double uj, double lipH, double fs) {
  return M_PI * std::clamp(0.12 * uj / std::max(lipH, 2e-4), 1500.0, 7000.0) / fs;
}

void Flute::jetCoefs() {
  const double h = cur_.lipH, W = cur_.jetLen;
  // Bickley half-width at the edge: 0.4 h at the lips, the jet spreads as it
  // goes; an airy (unfocused) embouchure spreads it more.
  jB_ = 0.4 * h + kSpread_ * W * (1 + 0.8 * (cur_.air - 0.35));
  jGain_ = std::exp(std::min(kAlpha_ / h * W, 5.0));
  jCg_ = std::cosh(std::min(kAlpha_ / h * W, 6.0));
  jDd_ = 4 / M_PI * std::sqrt(2 * h * W);
  lpX0_ = jetLpX(kJetLp_, kConv_, std::max(uj_, 0.3), jB_, fs_);
  lpE0_ = std::exp(-lpX0_);
  hissA0_ = hissA(uj_, cur_.lipH, fs_);
  hissT0_ = std::tan(hissA0_);
}

void Flute::settle() {
  Tube::settle();
  // The lips and the jet where the new note wants them; the tongue and the
  // glottis where the attack starts (a tongued note: the tongue sealed, as on
  // a flute never played; it did not rest open and have to close first). The
  // breath still comes as an attack does.
  tongue_ = target_.tongue;
  cur_.glottis = target_.glottis;
  cur_.lipH = target_.lipH;
  cur_.lipWidth = target_.lipWidth;
  cur_.jetLen = target_.jetLen;
  cur_.uncovered = target_.uncovered;
  cur_.y0 = target_.y0;
  cur_.face = target_.face;
  cur_.noise = target_.noise;
  cur_.air = target_.air;
}

void Flute::resetAir() {
  Tube::resetAir();
  eq_ = el_ = ef_ = 0;
  pm_ = uj_ = 0;
  xi_ = jl1_ = jl2_ = xiLow_ = dl1_ = dl2_ = agcEnv_ = 0;
  noiseLp_ = noiseHp_ = glotLp_ = edgeLow_ = edgeBand_ = 0;
  std::fill(vHist_, vHist_ + kHist, 0.0f);
}

float Flute::sample() {
  const double dt = 1 / fs_;
  // --- The player: lungs -> mouth -> tongue -> lip slit -> jet.
  const double h = cur_.lipH, W = cur_.jetLen;
  const double aj = 0.8 * h * cur_.lipWidth;  // jet flux area (Bickley: 2b H)
  const double at = tongue_ * kTongueArea;
  const double pf = std::max(0.0, pm_) * at * at / (at * at + aj * aj + 1e-30);
  // Jet inertia: rho l dUj/dt = pf - rho Uj^2 / 2.
  uj_ = (uj_ + dt / (kRho * channel_) * pf) / (1 + dt / (2 * channel_) * uj_);
  const double qj = aj * uj_;
  // Lungs -> trachea (resistance) -> glottis (Bernoulli orifice) -> mouth.
  // The glottis conductance is linearised at the last pressure drop.
  const double ag = std::max(cur_.glottis, 1e-7);
  const double dpg = std::max(std::fabs(cur_.lung - pm_), 1.0);
  const double gg = ag * std::sqrt(2 / (kRho * dpg));  // Q = gg * dp
  const double gIn = 1 / (1 / gg + kAirwayR);
  const double cm = kMouthVol / (kRho * kC * kC);
  // Turbulence in a narrow glottis: an "h" hiss as a pressure source in the
  // mouth, ~ rho v^2 / 2 of the glottal jet, strong only when narrow.
  const double vg = gIn * (cur_.lung - pm_) / ag;
  glotLp_ += glotA_ * (whiteNoise() - glotLp_);
  const double hiss = kGlotNoise_ * 0.5 * kRho * vg * vg * glotLp_;
  pm_ = (cm / dt * pm_ + gIn * (cur_.lung + hiss) - qj) / (cm / dt + gIn);
  // --- The jet: convected disturbance, deflection at the labium, flow split.
  const double ujc = std::max(uj_, 0.3);
  // Dispersive jet: short disturbances convect faster. A pure delay at the
  // fast speed (cp ~ 0.45 Uj, kb >~ 1) plus an all-pass that adds the rest of
  // the delay at low kb (see below).
  double tau = W / ((kJetDisp_ ? kCpHi_ : kConv_) * ujc) * fs_;
  tau = std::min(std::max(tau, 2.0), double(kHist - 4));
  const double rp = hPos_ - tau;
  const int i0 = int(std::floor(rp));
  const double fr = rp - i0;
  const float v0 = vHist_[(i0 + kHist) & (kHist - 1)], v1 = vHist_[(i0 + 1 + kHist) & (kHist - 1)];
  const double vdel = v0 + fr * (v1 - v0);
  const double b = jB_;  // the jet's half-width at the edge
  // Turbulence: white noise band-limited to ~0.2 .. 6 kHz, scaled by the jet speed.
  const double white = whiteNoise();
  noiseLp_ += noiseA_ * (white - noiseLp_);
  noiseHp_ += noiseHpA_ * (noiseLp_ - noiseHp_);
  // A narrow glottis sends turbulent air up to the lips: the lip jet carries
  // more turbulence (the "h" of a breath attack); ~1 when the glottis is open.
  const double glotTurb = 1 + kGlotNoise_ * std::min(25.0, (vg / 10) * (vg / 10));
  const double turb = (noiseLp_ - noiseHp_) * kNoiseGain_ * cur_.noise * uj_ * glotTurb;
  double eta;
  if (kJetModel_ == 1) {
    // Fletcher: the jet follows the acoustic displacement xi of the air in the
    // window, the disturbance grows as cosh(mu x) while it convects to the edge;
    // the edge sees the jet relative to the moving air: eta = cosh(mu W) xi(t - tau) - xi(t).
    // The jet amplifies only disturbances longer than its width (Rayleigh: k b
    // below ~1); shorter ones decay. Two poles at cp / (2 pi b).
    // The history holds the displacement in this model. The jet's own
    // turbulence enters here, so the jet's response shapes it (no 8-24 kHz fizz).
    double xd = kTurbIn_ * turb / (2 * M_PI * 1000);
    if (kJetDisp_) {
      // The sound's disturbance on a dispersive jet (Rayleigh / Bickley: cp
      // rises with kb; de la Cuadra 2007). Its delay at a played note's
      // fundamental stays W / (0.175 Uj), a half period (Fletcher's operating
      // point), but higher harmonics see relatively less delay, so they leave
      // the (2n+1) pi phase locks: no octave below the fundamental's threshold,
      // overblowing only at several times the pressure. A first-order all-pass
      // of time constant T ~ W / Uj adds 2 atan(w T) to the fast convection's
      // delay: the rest of the delay at the reference w = pi cp0 Uj / W, less
      // above it. All-pass: it favours no frequency's gain (poles did: a lower
      // mode won the second register).
      const double K = 2 * kDispT_ * W / ujc * fs_;
      const double c = (1 - K) / (1 + K);
      const double y = c * vdel + dl1_ - c * dl2_;
      dl1_ = vdel;
      dl2_ = y;
      xd += y;
    } else {
      xd += vdel;
    }
    if (kJetLp_ > 0) {  // the jet's width: Rayleigh, k b above ~1 decays
      // 1 - exp(-x) = 1 - exp(-x0) exp(-d) from the block's start, to the second
      // order in d (error < d^3 / 6 < 2e-6); a jump (an attack) exactly.
      const double x = jetLpX(kJetLp_, kConv_, ujc, b, fs_), d = x - lpX0_;
      const double al = std::fabs(d) < 0.02 ? 1 - lpE0_ * (1 - d * (1 - 0.5 * d)) : -std::expm1(-x);
      jl1_ += al * (xd - jl1_);
      jl2_ += al * (jl1_ - jl2_);
      xd = jl2_;
    }
    double cg = jCg_;
    if (kAgc_ > 0) {
      // Growth-limited jet: once the swing reaches ~ its width, the jet rolls up
      // into vortices and stops amplifying (over a few cycles, not within one):
      // the gain falls with the swing's envelope, so the swing stays smooth and
      // the edge, aimed off-centre by y0, makes the harmonics (Fletcher &
      // Douglas 1980), not a clipped square wave.
      const double lin = cg * xd;
      agcEnv_ += agcA_ * (lin * lin - agcEnv_);
      const double a = std::sqrt(2 * agcEnv_) / (kAgc_ * b);
      cg /= std::sqrt(1 + a * a);
    }
    eta = kEtaSign_ * (cg * xd - kRel_ * xi_);
  } else {
    eta = kEtaSign_ * (h / ujc) * jGain_ * (vdel + turb);
  }
  // Large swings: the jet rolls up into vortices instead of growing on; its
  // swing at the edge saturates at a few millimetres.
  if (kEtaMax_ > 0) eta = kEtaMax_ * std::tanh(eta / kEtaMax_);
  const double fn = std::tanh((eta - cur_.y0) / b);
  const double dd = jDd_;
  const double kj = kRho * dd * b * uj_ / W;
  // --- The bore, then the window branch at the embouchure cell (implicit).
  float pOld, jIn;
  stepBore(&pOld, &jIn);
  {
    const double c = bore_.compliance(ec_), w = 2 / dt;
    const double g = eg_, rs = ers_ + ecnl_ * std::fabs(eq_), rr = err_, hl = ehl_;
    // (w I - A) xbar = w x + e,  x = (p, Q, l)
    const double m00 = w, m01 = 1 / c, m02 = 0;
    const double m10 = -g, m11 = w + g * (rs + rr), m12 = -g * rr;
    const double m20 = 0, m21 = -hl * rr, m22 = w + hl * rr;
    const double r0 = w * pOld + jIn / c;
    const double r1 = w * eq_ - kJetSign_ * g * kj * (fn - ef_) / dt + g * testDp_ * 2 - g * pRoom_;
    testDp_ = 0;
    const double r2 = w * el_;
    // Cramer's rule.
    const double det = m00 * (m11 * m22 - m12 * m21) - m01 * (m10 * m22 - m12 * m20) +
                       m02 * (m10 * m21 - m11 * m20);
    const double x0 = (r0 * (m11 * m22 - m12 * m21) - m01 * (r1 * m22 - m12 * r2) +
                       m02 * (r1 * m21 - m11 * r2)) / det;
    const double x1 = (m00 * (r1 * m22 - m12 * r2) - r0 * (m10 * m22 - m12 * m20) +
                       m02 * (m10 * r2 - r1 * m20)) / det;
    const double x2 = (m00 * (m11 * r2 - r1 * m21) - m01 * (m10 * r2 - r1 * m20) +
                       r0 * (m10 * m21 - m11 * m20)) / det;
    bore_.s.p()[ec_] = float(2 * x0 - pOld);
    eq_ = 2 * x1 - eq_;
    el_ = 2 * x2 - el_;
    ef_ = fn;
  }
  // Acoustic displacement in the window (leaky integral of the velocity, 15 Hz),
  // high-passed at 100 Hz: a slowly drifting flow deflects a real jet only in
  // proportion to its speed (~ v W / Uj), not by a growing displacement. Without
  // this, the jet's mean aim and the window flow formed a slow loop that pumped
  // the tone at ~7 Hz on soft slurs (C5, F#5, G5 ...).
  xi_ = xi_ * (1 - 2 * M_PI * 15 * dt) + dt * eq_ / esw_;
  xiLow_ += 2 * M_PI * kXiHp_ * dt * (xi_ - xiLow_);
  vHist_[hPos_] = float(kJetModel_ == 1 ? xi_ - xiLow_ : eq_ / esw_);
  hPos_ = (hPos_ + 1) & (kHist - 1);
  // --- Radiation: d(flow)/dt of each opening, delayed to the listener.
  float y = radiate(eq_);
  {  // Edge hiss: the turbulent jet on the labium radiates directly (a dipole,
     // intensity ~ U^6), centred near St = f h / Uj ~ 0.12, airier = more.
    // Trapezoidal state-variable band-pass, Q 0.8 (ic1 = edgeBand_, ic2 = edgeLow_);
    // g = tan(a) from the block's start, to the second order in d (a jump: exactly).
    const double a = hissA(uj_, cur_.lipH, fs_), d = a - hissA0_, t2 = 1 + hissT0_ * hissT0_;
    const double g = std::fabs(d) < 0.02 ? hissT0_ + t2 * d * (1 + hissT0_ * d) : std::tan(a), k = 1 / 0.8;
    const double a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const double v3 = whiteNoise() - edgeLow_;
    const double v1 = a1 * edgeBand_ + a2 * v3, v2 = edgeLow_ + a2 * edgeBand_ + a3 * v3;
    edgeBand_ = 2 * v1 - edgeBand_;
    edgeLow_ = 2 * v2 - edgeLow_;
    const double bp = v1;
    const double u = uj_ / 25;
    y += float(kEdge_ * 4e-3 * (0.25 + cur_.air * cur_.air * 2) * u * u * u * bp);
  }
  return finish(y);
}

}  // namespace pajupilli
