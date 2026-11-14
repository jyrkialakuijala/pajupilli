#include "horn.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "knob.h"

namespace pajupilli {
namespace {

constexpr double kRho = Bore::kRho, kC = Bore::kC;
constexpr double kMouthVol = 60e-6;  // mouth cavity (m^3)
constexpr double kAirwayR = 2e5;     // lungs to mouth (Pa s / m^3)

double diameterAt(const Design& d, double x) {
  const double xm = x * 1e3;
  if (xm <= d.bore[0].x) return d.bore[0].d * 1e-3;
  for (int i = 1; i < d.nBore; ++i)
    if (xm <= d.bore[i].x) {
      const BorePoint &a = d.bore[i - 1], &b = d.bore[i];
      return 1e-3 * (a.d + (b.d - a.d) * (xm - a.x) / std::max(1e-9, b.x - a.x));
    }
  return d.bore[d.nBore - 1].d * 1e-3;
}

double smoothstep(double s) { return s * s * (3 - 2 * s); }

}  // namespace

void Horn::Mode::set(double w, double g, double dt) {
  // (I - dt/2 A)^-1 (I + dt/2 A), A = [[0, 1], [-w^2, -g]]; the force enters v.
  const double hd = 0.5 * dt, w2 = w * w;
  const double det = 1 + hd * g + hd * hd * w2;
  const double i00 = (1 + hd * g) / det, i01 = hd / det, i10 = -hd * w2 / det, i11 = 1 / det;
  t00 = i00 + i01 * (-hd * w2);
  t01 = i00 * hd + i01 * (1 - hd * g);
  t10 = i10 + i11 * (-hd * w2);
  t11 = i10 * hd + i11 * (1 - hd * g);
  b0 = i01 * dt;
  b1 = i11 * dt;
}

Horn::Horn(const Instrument& inst) : Tube(inst), lp_(*inst.design->lip) {
  const Design& d = *inst.design;
  {
    const double r = 48000 / fs_;
    glotA_ = 1 - std::pow(1 - 0.35, r);
    noiseA_ = 1 - std::pow(1 - 0.55, r);
    noiseHpA_ = 1 - std::pow(1 - 0.026, r);
  }
  kNoise_ = knob("PJ_LIPNOISE", 0.02);    // turbulence in the lip opening, x the flow
  kSweep_ = knob("PJ_LIPSWEEP", 1);       // the lips' swept flow
  kGlotNoise_ = knob("PJ_GLOTNOISE", 0.5);
  kQ_ = knob("PJ_LIPQ", 1);               // the lips' Q, x the design's
  kQExp_ = knob("PJ_LIPQEXP", 0.4);       // ... rising as (fl / 300 Hz)^this above 300 Hz
  kEps_ = knob("PJ_LIPEPS", 0.02) * 1e-3;  // the opening's smoothing near shut (m)
  kStroke_ = knob("PJ_VALVESTROKE", 0.007);  // a valve's press (s): a slurring finger is quick (slower, the half-valve between the notes is a hole in the legato)
  kRelease_ = knob("PJ_VALVEUP", 1.3);    // a valve's spring: the return stroke, x the press
  kMu_ = knob("PJ_LIPMU", 1);             // the lips' mass per area, x the design's
  kHk_ = knob("PJ_LIPHK", 1);             // their rest opening, x the design's
  kW_ = knob("PJ_LIPW", 1);               // their width, x the design's
  // The second, inward-striking mode: x the design's resonance ratio (0: none), Q, share.
  kIn2_ = knob("PJ_LIP2R", 1) * lp_.r2;
  kInQ_ = knob("PJ_LIP2Q", 1) * lp_.q2;
  kInA_ = knob("PJ_LIP2A", 1) * lp_.a2;
  // The lips at the first cell's centre: the cup's volume there, the throat
  // and backbore from it on.
  const double dx = bore_.dx(), half = 0.5 * dx;
  const Design* dp = &d;
  auto diam = [dp](double x) { return diameterAt(*dp, x); };
  setGeometry(diam, half, d.end * 1e-3, d.closedEnd);
  auto area = [&](double x) { const double r = 0.5 * diam(x); return M_PI * r * r; };
  // The cup, and the half cell of throat below the lips (the cell's own
  // volume, from the profile, stands for the rest of it).
  {
    double v = 0;
    constexpr int kN = 64;
    for (int j = 0; j < kN; ++j) v += area(half + (j + 0.5) / kN * half) * half / kN;
    bore_.addVolume(0, lp_.cup + v - bore_.area(0) * dx);
  }
  // Each interface's mass from the profile itself: rho / S integrated from one
  // cell centre to the next (a throat narrower than its cells has far more
  // mass than its cells' mean area gives).
  for (int i = 0; i + 1 < bore_.footCell(); ++i) {
    double inv = 0;
    constexpr int kN = 32;
    for (int j = 0; j < kN; ++j) inv += dx / kN / area((i + 0.5 + (j + 0.5) / kN) * dx);
    bore_.setInterfaceLength(i, bore_.interfaceArea(i) * inv - dx);
  }
  // The valves: each loop a bore of its own (cylindrical), n cells and two
  // links, so that the path from the port's cell a through the loop to a + 1
  // is the loop's length plus dx (the port's own interface).
  nValves_ = std::min(lp_.nValves, 4);
  for (int k = 0; k < nValves_; ++k) {
    const ValveDef& vd = lp_.valves[k];
    Valve& v = valve_[k];
    const double len = vd.len * 1e-3, dl = vd.d * 1e-3;
    v.a = std::clamp(int(vd.x * 1e-3 / dx - 0.5), 1, bore_.footCell() - 2);
    v.n = std::max(2, int(len / dx));
    v.link = 0.5 * (len - (v.n - 2) * dx);  // dx .. 1.5 dx
    v.area = 0.25 * M_PI * dl * dl;
    v.loop.reset(new Bore(fs_));
    v.loop->setBore([dl](double) { return dl; }, half, v.n * dx, true, true);
    // The links' share of the loop's volume (they carry its mass already).
    const double extraV = 0.5 * (len - v.n * dx) * v.area;
    v.loop->addVolume(0, extraV);
    v.loop->addVolume(v.n - 1, extraV);
    v.loop->update();
    setValveCoefs(v);
  }
  // The slide's cells: those wholly inside its tubes.
  if (lp_.slide) {
    slideA_ = std::max(1, int(std::ceil(lp_.slideX0 * 1e-3 / dx)));
    slideB_ = std::min(bore_.footCell() - 1, int(lp_.slideX1 * 1e-3 / dx));
    slideLen_ = (slideB_ - slideA_) * dx;
  }
  if (lp_.nlLen > 0) {
    nlCell_ = std::clamp(int(lp_.nlX * 1e-3 / dx), 1, bore_.footCell() - 1);
    // A simple wave's sample at pressure p+ arrives earlier by L beta p+ / (rho c^3) (in samples x fs).
    nlK_ = knob("PJ_BRASSY", 1) * lp_.nlLen * 1.2 / (kRho * kC * kC * kC) * fs_;
    // The linear wave's time from here to the bell (to line up with it).
    nlD0_ = (d.end * 1e-3 - (nlCell_ + 0.5) * dx) / kC * fs_;
    // Above the bell's cut-off the wave passes out whole: its flow grows as
    // sqrt(S) along the flare (its power kept).
    const double sc = bore_.area(nlCell_), sb = M_PI * 0.25 * std::pow(diameterAt(d, d.end * 1e-3), 2);
    nlGain_ = std::sqrt(sb / sc) * sc / (kRho * kC);
    nlLpA_ = 1 - std::exp(-2 * M_PI * 2500 / fs_);
    nlLpB_ = 1 - std::exp(-2 * M_PI * std::min(8000.0, 0.3 * fs_) / fs_);
  }
  bore_.update();
  addRadiators(false, 0);
  target_.tongue = 0;
  target_.lung = 0;
  target_.reed = 200;
  cur_ = target_;
  controlStep();
}

void Horn::setValveCoefs(Valve& v) {
  if (v.s <= 1e-3) {
    v.cw = v.cp = 0;
    v.w1 = v.w2 = 0;
    return;
  }
  v.awake = true;
  // As Bore's interfaces: L dw/dt = dp - R w (trapezoidal in R).
  const double L = kRho * v.link / (v.area * v.s), R = 0.5 * kRho * kC / v.area * (1 / v.s - 1);
  const double d = L * fs_ + 0.5 * R;
  v.cw = float((L * fs_ - 0.5 * R) / d);
  v.cp = float(1 / d);
}

void Horn::setHoles(const char* pattern, double fingerSpeed) {
  fingerSpeed = std::clamp(fingerSpeed, 0.25, 4.0);
  if (lp_.slide && ((*pattern >= '1' && *pattern <= '7') || (*pattern >= 'A' && *pattern <= 'G'))) {
    // The slide: a quick arm, ~60 ms from 1st to 7th, a little longer the
    // further it goes; a glide: the player's own time (a glissando).
    // '1'..'7': the positions; 'A'..'G': the same with the trigger's loop in
    // (the positions further apart).
    // A shaded position: "+12" after the valves, mm of tube more (or less).
    double to = *pattern <= '7' ? lp_.slide[*pattern - '1'] : lp_.slide[7 + *pattern - 'A'];
    ++pattern;
    if (const char* sh = std::strpbrk(pattern, "+-")) to = std::max(0.0, to + std::atof(sh));
    if (to != slideTo_) {
      slideFrom_ = slideNow_;
      slideTo_ = to;
      slideT_ = 0;
      const double reach = std::fabs(to - slideFrom_) / std::max(1.0, lp_.slide[6]);
      slideDur_ = (0.03 + 0.06 * std::sqrt(reach)) / std::sqrt(fingerSpeed);
      if (glide_ != 0) slideDur_ = std::max(slideDur_, std::abs(glide_));
    }
  }
  const int n = int(std::strlen(pattern));
  {
    // The valves' slides, set before the note speaks: a trumpet's ring finger
    // kicks the 3rd slide out for 1+3 and 1+2+3; a shade ("+40" after the
    // valves: mm of tube) pulls the slide of the last valve pressed (a tuba
    // player's for a sharp combination).
    auto down = [&](int k) { return k < n && pattern[k] == 'x'; };
    double extra[4] = {};
    if (nValves_ >= 3 && down(0) && down(2)) extra[2] = down(1) ? lp_.ring123 : lp_.ring13;
    if (!lp_.slide)
      if (const char* sh = std::strpbrk(pattern, "+-"))
        for (int k = nValves_ - 1; k >= 0; --k)
          if (down(k)) { extra[k] += std::atof(sh); break; }
    for (int k = 0; k < nValves_; ++k) {
      Valve& v = valve_[k];
      // (Only out: a loop shorter than its cells would break the Courant limit.)
      const double st = std::max(1.0, 1 + extra[k] * 1e-3 / (v.n * bore_.dx()));
      if (v.loop->stretch(0) == st) continue;
      for (int i = 0; i < v.n; ++i) v.loop->setStretch(i, st);
      v.loop->update();
    }
  }
  for (int k = 0; k < nValves_; ++k) {
    Valve& v = valve_[k];
    const int t = k < n && pattern[k] == 'x';
    if (t == v.target) continue;
    v.target = t;
    // A finger presses in ~7 ms; the spring returns it a little slower.
    v.stroke = kStroke_ / fingerSpeed * (t ? 1 : kRelease_);
    // A glide: the valve moves slowly, through half-valve.
    if (glide_ != 0) v.stroke = std::max(v.stroke, std::abs(glide_));
  }
  glide_ = 0;
}

void Horn::settle() {
  // The valves down or up, the lips at the new note's tension.
  for (int k = 0; k < nValves_; ++k) {
    Valve& v = valve_[k];
    if (v.t == v.target) continue;
    v.t = v.target;
    v.s = v.t;
    bore_.setGate(v.a, 1 - v.s);
    setValveCoefs(v);
  }
  if (lp_.slide && slideT_ < 1) {
    slideT_ = 1;
    setSlide(slideTo_);
  }
  bore_.update();
  // The tongue and the glottis where the attack starts (not closing from rest first).
  tongue_ = target_.tongue;
  cur_.glottis = target_.glottis;
  cur_.reed = target_.reed;
  cur_.lipPress = target_.lipPress;
  cur_.noise = target_.noise;
  cur_.air = target_.air;
}

void Horn::resetAir() {
  Tube::resetAir();
  for (int k = 0; k < nValves_; ++k) {
    std::memset(&valve_[k].loop->s, 0, sizeof(valve_[k].loop->s));
    valve_[k].w1 = valve_[k].w2 = 0;
    valve_[k].awake = valve_[k].cp != 0;
  }
  out_.x = out_.v = in_.x = in_.v = 0;
  dp_ = u_ = pm_ = 0;
  glotLp_ = noiseLp_ = noiseHp_ = 0;
  nlIn_ = nlOut_ = 0;
  nlMax_ = -1e9;
  nlPrev_ = nlPrev2_ = nlLp1_ = nlLp2_ = 0;
}

// Still: the flow and the loss states below ~1 mPa of plane-wave pressure,
// the pressure even to ~1 mPa (-160 dB re the bore's few kPa). A shut loop
// keeps the pressure it was cut off at (its air is trapped); stepping a still
// loop changes nothing, so it can stand until the valve opens again.
bool Horn::still(const Bore& b, double area) {
  const float* p = b.s.p();
  const float* u = b.s.u();
  const float zu = float(kRho * kC / area), lim = 1e-3f;
  for (int i = 0; i <= b.footCell(); ++i)
    if (std::fabs(p[i] - p[0]) > lim) return false;
  for (int i = 0; i < b.cells(); ++i) {
    float f = std::fabs(u[i]);
    for (int j = 0; j < kLossSections; ++j) f = std::max(f, std::fabs(b.s.psi[j][i]));
    if (f * zu > lim) return false;
  }
  return true;
}

void Horn::setSlide(double mm) {
  slideNow_ = mm;
  if (slideB_ <= slideA_) return;
  const double st = 1 + mm * 1e-3 / slideLen_;
  for (int i = slideA_; i < slideB_; ++i) bore_.setStretch(i, st);
}

// Burgers without dispersion: each sample of the outgoing wave travels at
// c + beta u; samples overtaken merge (a shock: the arrival times kept
// monotonic). Returns the warped wave on the output's clock, less the linear.
double Horn::brassy(double pp) {
  const long in = nlIn_++;
  // Arrival times (samples, less the common travel time): earlier by K p+.
  nlMax_ = std::max(nlMax_, in - nlK_ * pp);
  nlT_[in & (kNlRing - 1)] = nlMax_;
  nlVal_[in & (kNlRing - 1)] = float(pp);
  // Read kNlLook samples behind (a compression, arriving early, is in by then).
  const double T = double(in - kNlLook);
  if (T < 1) return 0;
  if (nlOut_ < in - kNlRing + 2) nlOut_ = in - kNlRing + 2;
  while (nlOut_ + 1 < in && nlT_[(nlOut_ + 1) & (kNlRing - 1)] <= T) ++nlOut_;
  const int a = int(nlOut_ & (kNlRing - 1)), b = int((nlOut_ + 1) & (kNlRing - 1));
  const double ta = nlT_[a], tb = nlT_[b];
  const double f = tb > ta ? std::clamp((T - ta) / (tb - ta), 0.0, 1.0) : 1.0;
  const double warped = nlVal_[a] + f * (nlVal_[b] - nlVal_[a]);
  return warped - nlVal_[(in - kNlLook) & (kNlRing - 1)];
}

void Horn::controlStep() {
  const double blockDt = 16 / fs_;
  const double a = 1 - std::exp(-blockDt / 0.002);  // 2 ms smoothing
  auto glide = [a](double& c, double t) { c += a * (t - c); };
  glide(cur_.lung, target_.lung);
  glide(cur_.noise, target_.noise);
  glide(cur_.air, target_.air);
  // The lips retune within a few ms (a slur: the tension moves first).
  cur_.reed *= std::exp((1 - std::exp(-blockDt / 0.004)) * std::log(target_.reed / cur_.reed));
  cur_.glottis += (1 - std::exp(-blockDt / 0.0005)) * (target_.glottis - cur_.glottis);
  tongue_ += (1 - std::exp(-blockDt / 0.001)) * (target_.tongue - tongue_);
  // The lips: resonance fl, rest opening hk / fl, mass mu per area.
  const double fl = std::clamp(cur_.reed, 20.0, 2000.0);
  cur_.lipPress += (1 - std::exp(-blockDt / 0.02)) * (target_.lipPress - cur_.lipPress);
  H_ = std::clamp(kHk_ * lp_.hk / fl * cur_.lipPress, 0.0, 3e-3);  // (a tuba's low notes: 2.5 mm)
  // The lips' Q: higher as their tension rises (the top notes need a sharper
  // lip to speak; lower down a sharp one splits the slot in two regimes).
  const double q = lp_.q * kQ_ * std::pow(std::max(1.0, fl / 300), kQExp_);
  const double wl = 2 * M_PI * fl, g = wl / q;
  // Low notes: more of the lips in the cup, slacker: heavier (Elliott & Bowsher:
  // the vibrating mass falls as the tension rises).
  invMu_ = 1 / (kMu_ * lp_.mu * std::pow(std::max(1.0, 190 / fl), 0.7));
  out_.set(wl, g, 1 / fs_);
  if (kIn2_ > 0) in_.set(kIn2_ * wl, kIn2_ * wl / kInQ_, 1 / fs_);
  // The valves move: the port shuts as the loop's links open.
  bool moved = false;
  for (int k = 0; k < nValves_; ++k) {
    Valve& v = valve_[k];
    if (v.t == v.target) continue;
    const double step = blockDt / v.stroke;
    v.t = v.target ? std::min(1.0, v.t + step) : std::max(0.0, v.t - step);
    v.s = smoothstep(v.t);
    bore_.setGate(v.a, 1 - v.s);
    setValveCoefs(v);
    moved = true;
  }
  if (lp_.slide && slideT_ < 1) {
    slideT_ = std::min(1.0, slideT_ + blockDt / slideDur_);
    setSlide(slideFrom_ + (slideTo_ - slideFrom_) * smoothstep(slideT_));
    moved = true;
  }
  if (moved) bore_.update();
  // A valve up: once its loop has rung down it stands still (skipped until pressed).
  for (int k = 0; k < nValves_; ++k) {
    Valve& v = valve_[k];
    if (v.cp == 0 && v.awake && still(*v.loop, v.area)) v.awake = false;
  }
}

float Horn::sample() {
  const double dt = 1 / fs_;
  // --- The lips: the pressure across them (last sample's) pushes the outward
  // mode open and the inward one shut.
  {
    const double f = dp_ * invMu_;
    out_.step(f);
    // Shut, they press together: no further than 0.2 mm past closing.
    if (out_.x < -H_ - 0.2e-3) {
      out_.x = -H_ - 0.2e-3;
      if (out_.v < 0) out_.v = 0;
    }
    if (kIn2_ > 0) in_.step(f);
  }
  // The opening, smoothed near shut (softplus over kEps_); the tongue behind
  // the teeth stops the air.
  {
    const double hr = H_ + out_.x - kInA_ * in_.x, e = kEps_;
    h_ = (hr > 20 * e ? hr : e * std::log1p(std::exp(hr / e))) * tongue_;
  }
  // --- The mouth: lungs -> glottis -> mouth, drained by the lips' flow.
  const double ag = std::max(cur_.glottis, 1e-7);
  const double dpg = std::max(std::fabs(cur_.lung - pm_), 1.0);
  const double gg = ag * std::sqrt(2 / (kRho * dpg));
  const double gIn = 1 / (1 / gg + kAirwayR);
  const double cm = kMouthVol / (kRho * kC * kC);
  const double vg = gIn * (cur_.lung - pm_) / ag;
  glotLp_ += glotA_ * (whiteNoise() - glotLp_);
  const double hiss = kGlotNoise_ * 0.5 * kRho * vg * vg * glotLp_;
  pm_ = (cm / dt * pm_ + gIn * (cur_.lung + hiss) - u_) / (cm / dt + gIn);
  // --- The valve links (from this step's pressures), the bores, the links' flows.
  float* p = bore_.s.p();
  for (int k = 0; k < nValves_; ++k) {
    Valve& v = valve_[k];
    if (v.cp == 0) continue;
    const float* q = v.loop->s.p();
    v.w1 = v.cw * v.w1 + v.cp * (p[v.a] - q[0]);
    v.w2 = v.cw * v.w2 + v.cp * (q[v.n - 1] - p[v.a + 1]);
  }
  float pOld, jIn;
  stepBore(&pOld, &jIn);
  for (int k = 0; k < nValves_; ++k) {
    Valve& v = valve_[k];
    if (!v.awake) continue;
    float d0, d1;
    v.loop->step(0, &d0, &d1, 0);
    if (v.cp == 0) continue;
    bore_.inject(v.a, v.w1);
    v.loop->inject(0, -v.w1);
    v.loop->inject(v.n - 1, v.w2);
    bore_.inject(v.a + 1, -v.w2);
  }
  // --- The cup with the lips' flow (trapezoidal, the square root in closed form).
  const double c = bore_.compliance(ec_), beta = 0.5 * dt / c;
  const double ur = kSweep_ * lp_.sr * out_.v;  // the lips moving out push air into the cup
  const double A = std::sqrt(2 / kRho) * kW_ * lp_.width * h_;
  // (Tests: a small flow impulse, 1e-8 m^3/s per unit.)
  const double R = pm_ - pOld - beta * (jIn + ur + 1e-8 * testDp_);
  const double bA = beta * A;
  const double s = 0.5 * (-bA + std::sqrt(bA * bA + 4 * std::fabs(R)));
  const double D = R >= 0 ? s * s : -s * s;
  const double u = A * (R >= 0 ? s : -s);
  testDp_ = 0;
  // Turbulence in the opening.
  noiseLp_ += noiseA_ * (whiteNoise() - noiseLp_);
  noiseHp_ += noiseHpA_ * (noiseLp_ - noiseHp_);
  const double un = kNoise_ * (0.5 + cur_.air) * cur_.noise / 0.01 * (noiseLp_ - noiseHp_) * u;
  const double pmid = pm_ - D + beta * un;
  p[ec_] = float(2 * pmid - pOld);
  u_ = u + un;
  dp_ = pm_ - pmid;
  float y = radiate(0);
  if (nlCell_ >= 0) {
    const float* pb = bore_.s.p();
    const float* ub = bore_.s.u();
    const int c = nlCell_;
    const double pPlus = 0.5 * (pb[c] + kRho * kC / bore_.area(c) * 0.5 * (ub[c - 1] + ub[c]));
    // The difference's flow out of the bell, radiated as the bell's (d/dt),
    // when the linear wave from here gets there (it was read kNlLook late).
    // A shock is a jump within a sample here; differentiated, a spike. Real
    // ones lose their tops in the bell (the wave spreads, the losses) and
    // radiate ~ -10 dB/oct above a 2..3 kHz transition (Fletcher & Tarnopolsky
    // 1999): a pole at 2.5 kHz and one at 8.
    nlLp1_ += nlLpA_ * (brassy(pPlus) - nlLp1_);
    nlLp2_ += nlLpB_ * (nlLp1_ - nlLp2_);
    const double q = nlGain_ * nlLp2_;
    const int at = std::max(1, radDelay_[0] + int(std::lround(nlD0_)) - kNlLook);
    outRing_[(outPos_ + at) & (kOutRing - 1)] += float(radGain_[0] * 0.5 * (q - nlPrev2_));
    nlPrev2_ = nlPrev_;
    nlPrev_ = q;
  }
  return finish(y);
}

}  // namespace pajupilli
