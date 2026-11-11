#include "flute.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {
namespace {

constexpr double kRho = Bore::kRho, kC = Bore::kC;
constexpr double kMu = 1.85e-5;      // dynamic viscosity of air (Pa s)
constexpr double kCork = 0.0175;     // cork face to embouchure centre (m)
constexpr double kFoot = 0.6004;     // embouchure centre to foot end, C foot (m)
constexpr double kEmbLong = 11.6e-3; // embouchure hole along the bore axis (m)
constexpr double kEmbRin = 6.4e-3;   // undercut: equivalent radius at the bore (m)
constexpr double kEmbChimney = 4.3e-3;
constexpr double kHoleT = 1.9e-3;    // tube wall + drawn chimney, every tone hole
constexpr double kTongueArea = 50e-6;   // tongue valve fully open (m^2)
constexpr double kMouthVol = 60e-6;     // mouth cavity (m^3)
constexpr double kAirwayR = 2e5;        // lungs to mouth (Pa s / m^3)
constexpr double kChannel = 3e-3;       // lip channel length, jet inertia (m)

// Bore diameter (m) at distance x (m) from the cork face (Pearl PF-661).
double boreDiameter(double x) {
  const double e = (x - kCork) * 1e3;  // mm from the embouchure centre
  double d;
  if (e < 0) d = 16.8 + (17.07 - 16.8) * (e + 17.5) / 17.5;
  else if (e < 113.5) d = 17.07 + (18.8 - 17.07) * e / 113.5;
  else if (e < 150.5) d = 18.8;
  else if (e < 154.5) d = 19.8;  // the head joint socket
  else d = 19.0;
  return d * 1e-3;
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
};

// Inner end correction of a side hole (Dalmont / Nederveen), delta = b / a.
double innerCorrection(double b, double delta) {
  return b * (0.82 - 0.193 * delta - 1.09 * delta * delta + 1.27 * std::pow(delta, 3) -
              0.71 * std::pow(delta, 4));
}

double smoothstep(double s) { return s * s * (3 - 2 * s); }

// Pad height (fraction of the full opening) along a stroke, from the stroke time
// `open` (0 closed .. 1 open). A finger or spring pushing a key closed
// accelerates it into the seat (lands at 2 x the mean speed); an opening key
// leaves the seat and settles gently.
double padShape(double open, bool closing) {
  return closing ? 1 - (1 - open) * (1 - open) : smoothstep(open);
}

// Stroke time that gives the pad height s on a stroke (keeps the pad where it
// is when a key reverses mid-stroke).
double padTime(double s, bool closing) {
  if (closing) return 1 - std::sqrt(std::max(0.0, 1 - s));
  double a = 0, b = 1;
  for (int i = 0; i < 30; ++i) {
    const double m = 0.5 * (a + b);
    (smoothstep(m) < s ? a : b) = m;
  }
  return 0.5 * (a + b);
}

}  // namespace

Flute::Flute(double fs) : fs_(fs), bore_(fs) {
  kJetSign_ = knob("PJ_JET_SIGN", 1);
  kEtaSign_ = knob("PJ_ETA_SIGN", 1);
  kEmbLen_ = knob("PJ_EMB_LEN", 2) * 1e-3;
  kAlpha_ = knob("PJ_ALPHA", 0.3);
  kConv_ = knob("PJ_CONV", 0.175);
  kNoiseGain_ = knob("PJ_NOISE", 0.3);
  kOut_ = knob("PJ_OUT", 0.3);
  kJetModel_ = int(knob("PJ_JETMODEL", 1));
  kRel_ = knob("PJ_REL", 0);
  kXiHp_ = knob("PJ_XIHP", 100);
  kVortex_ = knob("PJ_VORTEX", 1);
  kTurbIn_ = knob("PJ_TURBIN", 4);  // jet turbulence gain (it now passes the jet's low-pass)
  kEdge_ = knob("PJ_EDGE", 4);      // direct edge hiss
  // Pad pumping, as a share of a rigid pad landing flat: the felt and skin
  // compress and meet the rim unevenly, so most of the last puff leaks; 0.25
  // puts a medium key click ~24 dB under a mezzo-forte note (by ear: 0..1).
  kPadPump_ = knob("PJ_PADPUMP", 0.25);
  kKeyClick_ = knob("PJ_KEYCLICK", 1);
  {  // key impact modes: key arm / cup and the tube wall (~2.4, 5.3, 8.2 kHz)
    static const double f[kTickModes] = {2400, 5300, 8200}, tau[kTickModes] = {0.004, 0.0025, 0.0015};
    for (int m = 0; m < kTickModes; ++m) {
      tickCos_[m] = std::cos(2 * M_PI * f[m] / fs);
      tickSin_[m] = std::sin(2 * M_PI * f[m] / fs);
      tickDecay_[m] = std::exp(-1 / (tau[m] * fs));
    }
  }
  kSpread_ = knob("PJ_SPREAD", 0.1);
  kJetLp_ = knob("PJ_JETLP", 2);
  kEtaMax_ = knob("PJ_ETAMAX", 1.5) * 1e-3;
  kEmbR_ = knob("PJ_EMBR", 0.01);
  kGlotNoise_ = knob("PJ_GLOTNOISE", 0.5);
  kRadOnly_ = int(knob("PJ_RADONLY", 0));  // -1 embouchure only, 1 all but the embouchure
  bore_.setBore(boreDiameter, kCork + kFoot);
  const double dx = bore_.dx();
  ec_ = int(kCork / dx);
  for (int i = 0; i < kHoles; ++i) {
    Hole& h = hole_[i];
    const HoleSpec& sp = kSpec[i];
    h.x = kCork + sp.x * 1e-3;
    h.b = 0.5e-3 * sp.d;
    h.padH = sp.padH * 1e-3;
    h.padR = 0.5e-3 * sp.cup;
    h.perforated = sp.perf;
    h.cell = int(h.x / dx);
    const double a = 0.5 * boreDiameter(h.x), delta = h.b / a;
    const double tm = h.b * delta * (1 + 0.207 * std::pow(delta, 3)) / 8;
    // The grid puts the hole at the cell centre. A hole moved up by d sounds
    // like one with a chimney longer by d S_h / S: keep the note.
    const double d = h.x - (h.cell + 0.5) * dx;
    const double sh = M_PI * h.b * h.b, sb = M_PI * a * a;
    h.tSeries = std::max(1e-3, kHoleT + innerCorrection(h.b, delta) + tm + d * sh / sb);
    h.tRad = 0.6133 * h.b;
  }
  setHoles("xxxxxxxxxxxxxxxx");
  for (Hole& h : hole_) h.open = h.target;
  updateHoles(0);
  // Radiators: each hole cell, the embouchure, the foot (horn outlet).
  // Listener 1 m in front of the player's face (x along the flute from the cork
  // face, y forward). In the second register the embouchure and the open holes
  // move air in antiphase (mode 2): opposite the middle of the flute they cancel
  // the fundamental by ~14 dB and those notes sounded thin.
  const double lx = knob("PJ_LX", 0.0), ly = knob("PJ_LY", 1.0);
  auto addRad = [&](int cell, double x, double extra) {
    const double dist = std::hypot(x - lx, ly);
    radCell_[nRad_] = cell;
    radDelay_[nRad_] = std::max(0, int(std::lround((dist - extra) / kC * fs_)));
    radGain_[nRad_] = float(kRho / (4 * M_PI * dist) * fs_);
    ++nRad_;
  };
  addRad(-1, kCork, 0);  // embouchure
  addRad(-2, kCork + kFoot, horn_r());
  {  // the foot's gain: R / d (pressure at the horn's last cell, see sample())
    const double dist = std::hypot(kCork + kFoot - lx, ly);
    radGain_[nRad_ - 1] = float(horn_r() / dist);
  }
  for (int i = 0; i < kHoles; ++i) {
    bool dup = false;
    for (int j = 0; j < i; ++j) dup |= hole_[j].cell == hole_[i].cell;
    if (!dup) addRad(hole_[i].cell, hole_[i].x, 0);
  }
  cur_ = target_;
  embouchureGeometry();
}

double Flute::horn_r() const { return bore_.hornRadius(); }

void Flute::setHoles(const char* pattern, double fingerSpeed) {
  fingerSpeed_ = std::clamp(fingerSpeed, 0.25, 4.0);
  for (int i = 0; i < kHoles && pattern[i]; ++i) hole_[i].target = pattern[i] == 'o' ? 1 : 0;
}

Branch Flute::holeBranch(const Hole& h) const {
  Branch br;
  const double sh = M_PI * h.b * h.b;
  const double vc = sh * kHoleT;  // closed chimney volume
  const double s = padShape(h.open, h.closing);
  if (s < 1e-4) { br.vol = vc; return br; }
  const double hp = s * h.padH;
  // Pad as a radial slot of height hp between the cup and the rim.
  const double lnr = std::log(h.padR / h.b);
  double dpad = h.b * h.b * lnr / (2 * hp);
  if (h.perforated) {  // Dalmont 2001: the perforation shortens the pad's effect
    const double q = 3.4e-3, e = 3.6e-3;
    const double de = 1.64 * h.b / q - 0.15 * h.b / h.padR - 1.1 + e * h.b / (q * q);
    dpad /= 1 + 5 * std::pow(de, -1.35) * std::pow(hp / h.b, -0.2);
  }
  const double ts = h.tSeries + dpad;
  br.g = sh / (kRho * ts);
  br.rs = 6 * kMu * lnr / (M_PI * hp * hp * hp);
  br.rr = 1.5 * kRho * kC / sh;
  br.hl = sh / (kRho * h.tRad);
  br.vol = vc * dpad / (dpad + h.tSeries + h.tRad);
  return br;
}

void Flute::updateHoles(double dtBlock) {
  bool moved = dtBlock == 0;
  nPumping_ = 0;
  for (int i = 0; i < kHoles; ++i) {
    Hole& h = hole_[i];
    h.padIn = 0;
    if (h.open == h.target) continue;
    // A finger pushes a key in ~10 ms (faster when the key is struck harder), a
    // spring moves it back in ~16 ms (Almeida et al. 2009). Normally closed keys
    // (trills, G#, Eb) open by finger and close by spring.
    const bool normallyClosed = i == 0 || i == 1 || i == 7 || i == 13;
    const bool opening = h.target > h.open;
    const bool byFinger = opening == normallyClosed;
    if (h.closing == opening || dtBlock == 0) {  // a new stroke (or reversal): keep the pad where it is
      const double sNow = padShape(h.open, h.closing);
      h.closing = !opening;
      h.open = padTime(sNow, h.closing);
      // Fingers never land together: each stroke differs by up to +-15 %.
      keyRng_ = keyRng_ * 1664525u + 1013904223u;
      const double jitter = 1 + 0.3 * ((keyRng_ >> 8) * (1.0 / 16777216.0) - 0.5);
      h.stroke = (byFinger ? 0.010 / fingerSpeed_ : 0.016) * jitter;
    }
    const double o0 = h.open;
    const double step = dtBlock / h.stroke;
    h.open = opening ? std::min(h.target, h.open + step) : std::max(h.target, h.open - step);
    moved = true;
    if (dtBlock <= 0) continue;
    // Pad pumping: the air between the pad and the hole's rim can leave only
    // through the slot under the pad; the rest goes into the bore. Near the seat
    // the slot's viscous resistance (~1 / h^3) wins and the stroke's last
    // fraction of a millimetre pushes (or, opening, pulls) a puff into the tube.
    const double sh = M_PI * h.b * h.b;
    const double a = 0.5 * boreDiameter(h.x);
    const double zin = kRho * kC / (2 * M_PI * a * a);  // the bore, both ways
    const double lnr = std::log(h.padR / h.b);
    double vin = 0, hPrev = padShape(o0, h.closing) * h.padH;
    constexpr int kSub = 8;
    for (int k = 1; k <= kSub; ++k) {
      const double hk = padShape(o0 + (h.open - o0) * k / kSub, h.closing) * h.padH;
      const double hm = std::max(0.5 * (hk + hPrev), 1e-6);
      const double rs = 6 * kMu * lnr / (M_PI * hm * hm * hm);
      vin += -sh * (hk - hPrev) * rs / (rs + zin);
      hPrev = hk;
    }
    h.padIn = kPadPump_ * vin / dtBlock;
    ++nPumping_;
    // The pad lands: a tick from the key and the tube, ~ the landing speed.
    if (h.open == 0 && h.closing && kKeyClick_ > 0) {
      const double v = 2 * h.padH / h.stroke;   // m/s
      const double size = h.padR / 9.5e-3;      // bigger keys ring lower and louder
      for (int m = 0; m < kTickModes; ++m) {
        static const double kAmp[kTickModes] = {1.0, 0.6, 0.35};
        tickIm_[m] += kKeyClick_ * 0.01 * v * size * kAmp[m];
      }
    }
  }
  if (!moved) return;
  for (int i = 0; i < kHoles; ++i) {
    const int c = hole_[i].cell;
    bool first = true;
    for (int j = 0; j < i; ++j) first &= hole_[j].cell != c;
    if (!first) continue;
    Branch sum, part[kHoles];
    int n = 0;
    for (int j = i; j < kHoles; ++j)
      if (hole_[j].cell == c) part[n++] = holeBranch(hole_[j]);
    for (int j = 0; j < n; ++j) { sum.g += part[j].g; sum.vol += part[j].vol; }
    if (sum.g > 0) {
      double il = 0;
      for (int j = 0; j < n; ++j) {
        if (part[j].g == 0) continue;
        const double w = part[j].g / sum.g;
        sum.rs += w * w * part[j].rs;
        sum.rr += w * w * part[j].rr;
        il += w * w / part[j].hl;
      }
      sum.hl = 1 / il;
    } else {
      bore_.s.q[c] = 0;
      bore_.s.l[c] = 0;
    }
    bore_.setBranch(c, sum);
  }
  bore_.update();
}

void Flute::embouchureGeometry() {
  // The lower lip covers part of the hole: the open window is
  // uncovered x 11.6 mm at the top; the undercut makes it wider at the bore.
  const double sw = std::max(4e-6, cur_.uncovered * kEmbLong);
  const double sbot = M_PI * kEmbRin * kEmbRin;
  const double rw = std::sqrt(sw / M_PI);
  const double a = 0.5 * boreDiameter(kCork);
  const double m = kRho * (kEmbChimney / std::sqrt(sw * sbot) +
                           innerCorrection(kEmbRin, kEmbRin / a) / sbot + kEmbLen_ / sw);
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
  glide(cur_.noise, target_.noise);
  glide(cur_.air, target_.air);
  cur_.glottis += (1 - std::exp(-blockDt / 0.0005)) * (target_.glottis - cur_.glottis);
  const double at = 1 - std::exp(-blockDt / 0.001);
  tongue_ += at * (target_.tongue - tongue_);
  embouchureGeometry();
  updateHoles(blockDt);
}

void Flute::process(float* out, int n) {
  bool finite = true;
  for (int i = 0; i < n; ++i) {
    if (ctl_ == 0) controlStep();
    ctl_ = (ctl_ + 1) & 15;
    out[i] = sample();
    finite &= std::isfinite(out[i]);
  }
  if (!finite) {  // a NaN / Inf would stay in the state forever: start the air afresh
    resetAir();
    std::fill(out, out + n, 0.0f);
    ++resets_;
  }
}

void Flute::resetAir() {
  std::memset(&bore_.s, 0, sizeof(bore_.s));
  bore_.resetTermination();
  eq_ = el_ = ef_ = 0;
  pm_ = uj_ = 0;
  xi_ = jl1_ = jl2_ = xiLow_ = 0;
  noiseLp_ = noiseHp_ = glotLp_ = edgeLow_ = edgeBand_ = 0;
  std::fill(vHist_, vHist_ + kHist, 0.0f);
  std::fill(outRing_, outRing_ + kOutRing, 0.0f);
  std::fill(radPrev_, radPrev_ + kHoles + 2, 0.0f);
  std::fill(radPrev2_, radPrev2_ + kHoles + 2, 0.0f);
  dcX_ = dcY_ = 0;
}

float Flute::sample() {
  const double dt = 1 / fs_;
  // --- The player: lungs -> mouth -> tongue -> lip slit -> jet.
  const double h = cur_.lipH, W = cur_.jetLen;
  const double aj = 0.8 * h * cur_.lipWidth;  // jet flux area (Bickley: 2b H)
  const double at = tongue_ * kTongueArea;
  const double pf = std::max(0.0, pm_) * at * at / (at * at + aj * aj + 1e-30);
  // Jet inertia: rho l dUj/dt = pf - rho Uj^2 / 2.
  uj_ = (uj_ + dt / (kRho * kChannel) * pf) / (1 + dt / (2 * kChannel) * uj_);
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
  glotLp_ += 0.35 * (whiteNoise() - glotLp_);
  const double hiss = kGlotNoise_ * 0.5 * kRho * vg * vg * glotLp_;
  pm_ = (cm / dt * pm_ + gIn * (cur_.lung + hiss) - qj) / (cm / dt + gIn);
  // --- The jet: convected disturbance, deflection at the labium, flow split.
  const double ujc = std::max(uj_, 0.3);
  double tau = W / (kConv_ * ujc) * fs_;
  tau = std::min(std::max(tau, 2.0), double(kHist - 4));
  const double rp = hPos_ - tau;
  const int i0 = int(std::floor(rp));
  const double fr = rp - i0;
  const float v0 = vHist_[(i0 + kHist) & (kHist - 1)], v1 = vHist_[(i0 + 1 + kHist) & (kHist - 1)];
  const double vdel = v0 + fr * (v1 - v0);
  // Bickley half-width at the edge: 0.4 h at the lips, the jet spreads as it
  // goes; an airy (unfocused) embouchure spreads it more.
  const double b = 0.4 * h + kSpread_ * W * (1 + 0.8 * (cur_.air - 0.35));
  // Turbulence: white noise band-limited to ~0.2 .. 6 kHz, scaled by the jet speed.
  const double white = whiteNoise();
  noiseLp_ += 0.55 * (white - noiseLp_);
  noiseHp_ += 0.026 * (noiseLp_ - noiseHp_);
  // A narrow glottis sends turbulent air up to the lips: the lip jet carries
  // more turbulence (the "h" of a breath attack); ~1 when the glottis is open.
  const double glotTurb = 1 + kGlotNoise_ * std::min(25.0, (vg / 10) * (vg / 10));
  const double turb = (noiseLp_ - noiseHp_) * kNoiseGain_ * cur_.noise * uj_ * glotTurb;
  const double gain = std::exp(std::min(kAlpha_ / h * W, 5.0));
  double eta;
  if (kJetModel_ == 1) {
    // Fletcher: the jet follows the acoustic displacement xi of the air in the
    // window, the disturbance grows as cosh(mu x) while it convects to the edge;
    // the edge sees the jet relative to the moving air: eta = cosh(mu W) xi(t - tau) - xi(t).
    // The jet amplifies only disturbances longer than its width (Rayleigh: k b
    // below ~1); shorter ones decay. Two poles at cp / (2 pi b).
    // The history holds the displacement in this model. The jet's own
    // turbulence enters here, so the jet's response shapes it (no 8-24 kHz fizz).
    double xd = vdel + kTurbIn_ * turb / (2 * M_PI * 1000);
    if (kJetLp_ > 0) {
      const double fc = kJetLp_ * kConv_ * ujc / (2 * M_PI * b);
      const double al = 1 - std::exp(-2 * M_PI * std::min(fc, 0.4 * fs_) / fs_);
      jl1_ += al * (xd - jl1_);
      jl2_ += al * (jl1_ - jl2_);
      xd = jl2_;
    }
    eta = kEtaSign_ * (std::cosh(std::min(kAlpha_ / h * W, 6.0)) * xd - kRel_ * xi_);
  } else {
    eta = kEtaSign_ * (h / ujc) * gain * (vdel + turb);
  }
  // Large swings: the jet rolls up into vortices instead of growing on; its
  // swing at the edge saturates at a few millimetres.
  if (kEtaMax_ > 0) eta = kEtaMax_ * std::tanh(eta / kEtaMax_);
  const double fn = std::tanh((eta - cur_.y0) / b);
  const double dd = 4 / M_PI * std::sqrt(2 * h * W);
  const double kj = kRho * dd * b * uj_ / W;
  // --- The bore, then the window branch at the embouchure cell (implicit).
  float pOld, jIn;
  bore_.step(ec_, &pOld, &jIn);
  if (nPumping_ > 0)  // pads pushing air into the tube (key clicks)
    for (const Hole& hl : hole_)
      if (hl.padIn != 0) bore_.s.p()[hl.cell] += float(hl.padIn * dt / bore_.compliance(hl.cell));
  {
    const double c = bore_.compliance(ec_), w = 2 / dt;
    const double g = eg_, rs = ers_ + ecnl_ * std::fabs(eq_), rr = err_, hl = ehl_;
    // (w I - A) xbar = w x + e,  x = (p, Q, l)
    const double m00 = w, m01 = 1 / c, m02 = 0;
    const double m10 = -g, m11 = w + g * (rs + rr), m12 = -g * rr;
    const double m20 = 0, m21 = -hl * rr, m22 = w + hl * rr;
    const double r0 = w * pOld + jIn / c;
    const double r1 = w * eq_ - kJetSign_ * g * kj * (fn - ef_) / dt + g * testDp_ * 2;
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
  for (int r = 0; r < nRad_; ++r) {
    const int c = radCell_[r];
    if (kRadOnly_ != 0 && (kRadOnly_ == -1) != (c == -1)) continue;
    float add;
    if (c == -2) {
      // The foot: the horn's last cell already holds the spherical wave at
      // radius R; the listener gets p(R) R / d. (Its volume flow there is not
      // the source strength: U(R) grows as |1 + jkR|.)
      add = radGain_[r] * bore_.s.p()[kCells - 1];
    } else {
      // d/dt of the mean of the last two flows: a zero at Nyquist, so a pad
      // closing (stiff, trapezoidal) does not click at 24 kHz.
      const float q = c == -1 ? float(eq_) : bore_.s.q[c];
      add = radGain_[r] * 0.5f * (q - radPrev2_[r]);
      radPrev2_[r] = radPrev_[r];
      radPrev_[r] = q;
    }
    outRing_[(outPos_ + radDelay_[r]) & (kOutRing - 1)] += add;
  }
  float y = outRing_[outPos_];
  outRing_[outPos_] = 0;
  {  // Edge hiss: the turbulent jet on the labium radiates directly (a dipole,
     // intensity ~ U^6), centred near St = f h / Uj ~ 0.12, airier = more.
    const double fe = std::clamp(0.12 * uj_ / std::max(cur_.lipH, 2e-4), 1500.0, 7000.0);
    // Trapezoidal state-variable band-pass, Q 0.8 (ic1 = edgeBand_, ic2 = edgeLow_).
    const double g = std::tan(M_PI * fe / fs_), k = 1 / 0.8;
    const double a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
    const double v3 = whiteNoise() - edgeLow_;
    const double v1 = a1 * edgeBand_ + a2 * v3, v2 = edgeLow_ + a2 * edgeBand_ + a3 * v3;
    edgeBand_ = 2 * v1 - edgeBand_;
    edgeLow_ = 2 * v2 - edgeLow_;
    const double bp = v1;
    const double u = uj_ / 25;
    y += float(kEdge_ * 4e-3 * (0.25 + cur_.air * cur_.air * 2) * u * u * u * bp);
  }
  for (int m = 0; m < kTickModes; ++m) {  // key impact ticks
    const double re = tickRe_[m], im = tickIm_[m];
    tickRe_[m] = tickDecay_[m] * (re * tickCos_[m] - im * tickSin_[m]);
    tickIm_[m] = tickDecay_[m] * (re * tickSin_[m] + im * tickCos_[m]);
    y += float(im);
  }
  outPos_ = (outPos_ + 1) & (kOutRing - 1);
  // DC blocker (~5 Hz).
  const float yd = y - dcX_ + 0.99935f * dcY_;
  dcX_ = y;
  dcY_ = yd;
  return float(yd * kOut_);
}

}  // namespace pajupilli
