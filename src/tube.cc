#include "tube.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {
namespace {

constexpr double kRho = Bore::kRho, kC = Bore::kC;
constexpr double kMu = 1.85e-5;  // dynamic viscosity of air (Pa s)

// The series length t_a of a side hole (negative), in bore units: Dubos et al.
// 1999 (eq. 74), t the chimney height; closed holes use coth.
double seriesLength(double b, double a, double t, bool closed) {
  const double d = b / a, th = std::tanh(1.84 * t / b);
  return -b * d * d / (1.78 * (closed ? 1 / th : th) + 0.940 + 0.540 * d + 0.285 * d * d);
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

// Inner end correction of a side hole (Dalmont / Nederveen), delta = b / a.
double innerCorrection(double b, double delta) {
  return b * (0.82 - 0.193 * delta - 1.09 * delta * delta + 1.27 * std::pow(delta, 3) -
              0.71 * std::pow(delta, 4));
}

Tube::Tube(const Instrument& inst) : inst_(inst), fs_(inst.fs), bore_(inst.fs) {
  {
    const double r = 48000 / fs_;
    noiseNorm_ = std::sqrt(1 / r);
    dcPole_ = float(std::pow(0.99935, r));
  }
  kOut_ = knob("PJ_OUT", 0.3);
  kSeries_ = knob("PJ_SERIES", 1);  // closed holes' series length t_a (Dubos)
  // Pad pumping, as a share of a rigid pad landing flat: the felt and skin
  // compress and meet the rim unevenly, so most of the last puff leaks; 0.25
  // puts a medium key click ~24 dB under a mezzo-forte note (by ear: 0..1).
  kPadPump_ = knob("PJ_PADPUMP", 0.25);
  kKeyClick_ = knob("PJ_KEYCLICK", 1);
  kKeySpread_ = knob("PJ_KEYSPREAD", 3) * 1e-3;
  // The landing thud: the pad goes on into its felt and springs back, pushing
  // the air in the (now sealed) hole into the bore, both ways along the tube.
  // Key cup + finger on the felt ring at ~400 Hz, Q ~1.5 (felt is lossy). Only
  // the pad's middle moves with the cup: 0.15 of a rigid piston (by ear) puts
  // a medium thud ~2 dB under the pad's puff, ~28 dB under a mezzo-forte note.
  kThud_ = 0.15 * knob("PJ_THUD", 1);
  {
    const double f = knob("PJ_THUDF", 400), q = 1.5;
    // Release: 0.05 mm of felt preload, peak skin speed x0 w (m/s).
    kRelease_ = knob("PJ_RELEASE", 1) * 0.05e-3 * 2 * M_PI * f;
    thudCos_ = float(std::cos(2 * M_PI * f / fs_));
    thudSin_ = float(std::sin(2 * M_PI * f / fs_));
    thudDecay_ = float(std::exp(-M_PI * f / (q * fs_)));
  }
  {  // key impact modes: key arm / cup and the tube wall (~2.4, 5.3, 8.2 kHz)
    static const double f[kTickModes] = {2400, 5300, 8200}, tau[kTickModes] = {0.004, 0.0025, 0.0015};
    for (int m = 0; m < kTickModes; ++m) {
      const double fm = std::min(f[m], 0.45 * fs_);
      tickCos_[m] = std::cos(2 * M_PI * fm / fs_);
      tickSin_[m] = std::sin(2 * M_PI * fm / fs_);
      tickDecay_[m] = std::exp(-1 / (tau[m] * fs_));
    }
  }
  kRadOnly_ = int(knob("PJ_RADONLY", 0));  // -1 embouchure only, 1 all but the embouchure
  kHalf_ = knob("PJ_HALFHOLE", 0.25);
}

void Tube::setGeometry(const std::function<double(double)>& diameter, double exciteX, double endX, bool closed) {
  diam_ = diameter;
  endX_ = endX;
  bore_.setBore(diameter, exciteX, endX, closed, inst_.design != nullptr);
  ec_ = bore_.embCell();
}

void Tube::addHole(double x, double b, double t, double padH, double padR, int flags, bool present) {
  Hole& h = hole_[nHoles_++];
  h = Hole();
  h.x = x;
  h.b = b;
  h.t = t;
  h.padH = padH;
  h.padR = padR;
  h.perforated = flags & kPerforated;
  h.finger = flags & kFingerHole;
  h.normallyClosed = flags & kNormallyClosed;
  h.present = present;
}

void Tube::finishHoles() {
  const double dx = bore_.dx(), x0 = bore_.x0();
  const double kHoleComp = knob("PJ_HOLECOMP", 2), kOpenTa = knob("PJ_OPENTA", 0.8);
  for (int i = 0; i < nHoles_; ++i) {
    Hole& h = hole_[i];
    h.cell = h.present ? std::clamp(int((h.x - x0) / dx), ec_ + 1, bore_.footCell() - 1) : -1;
    if (h.present) h.a = 0.5 * diam_(h.x);
  }
  for (int i = 0; i < nHoles_; ++i) {
    Hole& h = hole_[i];
    if (!h.present) continue;
    const double a = h.a, delta = h.b / a;
    const double tm = h.b * delta * (1 + 0.207 * std::pow(delta, 3)) / 8;
    // The grid puts the hole at the cell centre. A hole moved up by d sounds
    // like one with a chimney longer by ~d S_h / S: keep the note. An open
    // hole's series length t_a (negative, Dubos) can't shorten the waveguide's
    // interfaces (stability), but for the first open hole it acts much as the
    // hole moved up by ~|t_a| (kOpenTa): it joins the offset. kHoleComp: with a
    // lattice of open holes below, d S_h / S alone corrects only about half.
    // Both fitted with tools/fitgrid.py against tools/tmm.py: 1.4 cents rms
    // over the first three resonances of every instrument's fingerings.
    const double d = h.x - (x0 + (h.cell + 0.5) * dx);
    const double sh = M_PI * h.b * h.b, sb = M_PI * a * a;
    const double taOpen = seriesLength(h.b, a, h.t, false);
    h.tSeries = std::max(1e-3 * inst_.bore, h.t + innerCorrection(h.b, delta) + tm +
                                                kHoleComp * (d + kOpenTa * taOpen) * sh / sb);
    h.tClosed = h.t + tm - 0.05 * h.b;  // closed chimney, with its matching volume
    h.taClosed = seriesLength(h.b, a, h.t, true);
    h.tRad = 0.6133 * h.b;
  }
  // The cells with holes, for their nonlinear losses (the designed instruments;
  // the flute family is calibrated without them).
  // x2: the flow separates at both ends of the chimney (Dalmont et al. 2002 give a
  // loss per end; by ear and the clarinet's upper clarion).
  kHoleNl_ = inst_.design ? knob("PJ_HOLENL", 2) : 0;
  nLossCells_ = 0;
  for (int i = 0; i < nHoles_; ++i) {
    if (!hole_[i].present || kHoleNl_ <= 0) continue;
    bool dup = false;
    for (int k = 0; k < nLossCells_; ++k) dup |= lossCell_[k] == hole_[i].cell;
    if (!dup) { lossR_[nLossCells_] = lossAcc_[nLossCells_] = 0; lossCell_[nLossCells_++] = hole_[i].cell; }
  }
}

void Tube::restHoles(const char* pattern) {
  setHoles(pattern);
  for (int i = 0; i < nHoles_; ++i) hole_[i].open = hole_[i].target;
  updateHoles(0);
}

void Tube::addRadiators(bool window, double windowX) {
  // Listener 1 m in front of the player's face (x along the instrument from
  // the top, y forward). In the second register the embouchure and the open holes
  // move air in antiphase (mode 2): opposite the middle of the flute they cancel
  // the fundamental by ~14 dB and those notes sounded thin.
  const double lx = knob("PJ_LX", 0.0), ly = knob("PJ_LY", 1.0);
  nRad_ = 0;
  auto addRad = [&](int cell, double x) {
    const double dist = std::hypot(x - lx, ly);
    radCell_[nRad_] = cell;
    radDelay_[nRad_] = std::max(0, int(std::lround(dist / kC * fs_)));
    radGain_[nRad_] = float(kRho / (4 * M_PI * dist) * fs_);
    ++nRad_;
  };
  if (window) addRad(-1, windowX);
  if (bore_.branch(bore_.footCell()).g > 0) addRad(bore_.footCell(), endX_);
  for (int i = 0; i < nHoles_; ++i) {
    if (!hole_[i].present) continue;
    bool dup = false;
    for (int j = 0; j < i; ++j) dup |= hole_[j].present && hole_[j].cell == hole_[i].cell;
    if (!dup) addRad(hole_[i].cell, hole_[i].x);
  }
}

void Tube::setHoles(const char* pattern, double fingerSpeed) {
  fingerSpeed_ = std::clamp(fingerSpeed, 0.25, 4.0);
  // Holes left out of the pattern are open (a flute fingering without the 17th
  // hole leaves the B foot's C key open).
  const int n = int(std::strlen(pattern));
  for (int i = 0; i < nHoles_; ++i) {
    if (!hole_[i].present) continue;
    const char c = i < n ? pattern[i] : 'o';
    hole_[i].target = c == 'x' ? 0 : c == 'h' ? (hole_[i].finger ? kHalf_ : 0.5) : 1;
    hole_[i].glideStroke = 0;
  }
  if (glide_ != 0) {
    // The holes starting a stroke, in the order the fingers reach them.
    int idx[64], k = 0;
    for (int i = 0; i < nHoles_ && k < 64; ++i) {
      const Hole& h = hole_[i];
      if (!h.present || h.target == h.open) continue;
      if (h.closing == (h.target > h.open)) idx[k++] = i;
    }
    const bool up = glide_ > 0;
    std::sort(idx, idx + k, [&](int a, int b) { return up ? hole_[a].x > hole_[b].x : hole_[a].x < hole_[b].x; });
    const double g = std::abs(glide_);
    for (int j = 0; j < k; ++j) {
      hole_[idx[j]].glideWait = g * j / k;
      hole_[idx[j]].glideStroke = std::max(0.010, 1.5 * g / k);
    }
    glide_ = 0;
  }
}

Branch Tube::holeBranch(const Hole& h) const {
  Branch br;
  const double sh = M_PI * h.b * h.b;
  const double vc = sh * h.tClosed;  // closed chimney volume
  const double s = padShape(h.open, h.closing);
  if (s < 1e-4) { br.vol = vc; return br; }
  if (h.finger) {
    // A finger slides off (or half-holes) the hole: the open part is a smaller
    // hole of the same chimney, the rest stays a closed volume. Its inertance
    // grows as the opening narrows (~1 / area): smooth from closed to open.
    const double be = h.b * std::sqrt(s), she = sh * s;
    const double delta = be / h.a;
    const double ts = h.tSeries + innerCorrection(be, delta) - innerCorrection(h.b, h.b / h.a);
    br.g = she / (kRho * std::max(ts, 1e-4));
    br.rr = 1.5 * kRho * kC / she;
    br.hl = she / (kRho * 0.6133 * be);
    br.vol = vc * (1 - s);
    return br;
  }
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

double Tube::keyDelay() {
  // Scrambled: the next LCG output is correlated with the stroke jitter.
  keyRng_ = keyRng_ * 1664525u + 1013904223u;
  uint32_t r = keyRng_ ^ (keyRng_ >> 16);
  r *= 0x7feb352du;
  r ^= r >> 15;
  r *= 0x846ca68bu;
  r ^= r >> 16;
  return kKeySpread_ * ((r >> 8) * (1.0 / 16777216.0));
}

void Tube::updateHoles(double dtBlock) {
  bool moved = dtBlock == 0;
  nPumping_ = 0;
  // Several fingers put down (or lifted) "together" move 0..3 ms apart: each
  // stroke that starts with others in the same direction waits a random delay.
  int closers = 0, openers = 0;
  for (int i = 0; i < nHoles_; ++i) {
    const Hole& h = hole_[i];
    closers += h.target < h.open && !h.closing;
    // (A pad closed since the start opens on its own: a note from silence is
    // fragile at the top of the bass, and the calibration starts from there.)
    openers += h.target > h.open && h.closing;
  }
  for (int i = 0; i < nHoles_; ++i) {
    Hole& h = hole_[i];
    h.padIn = 0;
    if (!h.present) continue;
    if (h.open == h.target) { h.wait = 0; continue; }
    // A finger pushes a key in ~10 ms (faster when the key is struck harder), a
    // spring moves it back in ~16 ms (Almeida et al. 2009). Normally closed keys
    // (trills, G#, Eb) open by finger and close by spring. Fingers on holes
    // move both ways.
    const bool opening = h.target > h.open;
    const bool byFinger = h.finger || opening == h.normallyClosed;
    if (h.closing == opening || dtBlock == 0) {  // a new stroke (or reversal): keep the pad where it is
      const double sNow = padShape(h.open, h.closing);
      h.closing = !opening;
      h.open = padTime(sNow, h.closing);
      // Fingers never land together: each stroke differs by up to +-15 %.
      keyRng_ = keyRng_ * 1664525u + 1013904223u;
      const double jitter = 1 + 0.3 * ((keyRng_ >> 8) * (1.0 / 16777216.0) - 0.5);
      h.stroke = (byFinger ? 0.010 / fingerSpeed_ : 0.016) * jitter;
      h.wait = 0;
      if ((h.closing ? closers : openers) > 1 && dtBlock > 0) h.wait = keyDelay();
      if (h.glideStroke > 0) {  // a glide: slowly, one finger after another
        h.stroke = h.glideStroke * jitter;
        h.wait = h.glideWait;
        h.glideStroke = 0;
      }
    }
    if (h.wait > 0) {
      h.wait -= dtBlock;
      continue;
    }
    const double o0 = h.open;
    if (opening && h.held && dtBlock > 0 && kThud_ > 0 && !h.finger) {
      // The pad leaves its seat: the felt the finger (or the spring) held
      // compressed ~0.05 mm springs back and the skin pulls air out of the
      // hole: the thud's mirror, a small suction both ways along the tube.
      // (A pad resting closed since the start is only lightly held.)
      const double sh0 = M_PI * h.b * h.b;
      h.thRe = 0;
      h.thIm = float(kThud_ * kRelease_ * sh0);
      thudLeft_ = int(0.03 * fs_);
    }
    h.held = false;
    const double step = dtBlock / h.stroke;
    h.open = opening ? std::min(h.target, h.open + step) : std::max(h.target, h.open - step);
    moved = true;
    if (dtBlock <= 0) continue;
    // Pad pumping: the air between the pad and the hole's rim can leave only
    // through the slot under the pad; the rest goes into the bore. Near the seat
    // the slot's viscous resistance (~1 / h^3) wins and the stroke's last
    // fraction of a millimetre pushes (or, opening, pulls) a puff into the tube.
    // (A finger: the soft pad of the fingertip, the same way.)
    const double sh = M_PI * h.b * h.b;
    const double a = h.a;
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
    h.padIn = kPadPump_ * (h.finger ? 0.5 : 1) * vin / dtBlock;
    ++nPumping_;
    if (h.finger) continue;  // no felt, no metal
    // The pad lands: a tick from the key and the tube, ~ the landing speed.
    if (h.open == 0 && h.closing && kThud_ > 0) {
      const double v = 2 * h.padH / h.stroke;  // landing speed (m/s)
      h.thRe = float(kThud_ * v * sh);
      h.thIm = 0;
      h.held = true;
      thudLeft_ = int(0.03 * fs_);  // 30 ms: ~-80 dB
    }
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
  for (int i = 0; i < nHoles_; ++i) {
    const int c = hole_[i].cell;
    if (c < 0 || !hole_[i].present) continue;
    bool first = true;
    for (int j = 0; j < i; ++j) first &= !(hole_[j].present && hole_[j].cell == c);
    if (!first) continue;
    bool any = false;
    const Branch sum = cellBranch(c, &any);
    if (!any) {
      bore_.s.q[c] = 0;
      bore_.s.l[c] = 0;
    }
    bore_.setBranch(c, sum);
  }
  bore_.update();
}

Branch Tube::cellBranch(int c, bool* any) {
  Branch sum, part[kMaxHoles];
  int idx[kMaxHoles];
  int n = 0;
  for (int j = 0; j < nHoles_; ++j)
    if (hole_[j].present && hole_[j].cell == c) { idx[n] = j; part[n++] = holeBranch(hole_[j]); }
  for (int j = 0; j < n; ++j) { sum.g += part[j].g; sum.vol += part[j].vol; }
  {  // A closed hole's negative series length, in step with its chimney
    // volume (which keeps the cell within the scheme's stability limit):
    // fading with openness alone, a closing pad made the bass blow up.
    double ta = 0;
    for (int k = 0; k < n; ++k) {
      const Hole& h = hole_[idx[k]];
      ta += h.taClosed * part[k].vol / (M_PI * h.b * h.b * h.tClosed);
    }
    bore_.setSeries(c, kSeries_ * ta);
  }
  *any = sum.g > 0;
  if (sum.g > 0) {
    double il = 0, area = 0;
    for (int j = 0; j < n; ++j) {
      if (part[j].g == 0) continue;
      const double w = part[j].g / sum.g;
      sum.rs += w * w * part[j].rs;
      sum.rr += w * w * part[j].rr;
      il += w * w / part[j].hl;
      area += kRho * kC / part[j].rr * 1.5;  // open area (rr = 1.5 rho c / S)
    }
    sum.hl = 1 / il;
    for (int k = 0; k < nLossCells_; ++k)
      if (lossCell_[k] == c) {
        // Cd 0.6 (vena contracta); the open area of the cell's holes.
        lossK_[k] = kHoleNl_ * kRho / (2 * 0.36 * area * area) * 8 / (3 * M_PI);
        sum.rs += lossR_[k];
      }
  }
  return sum;
}

void Tube::holeLosses() {
  if (kHoleNl_ <= 0) return;
  for (int k = 0; k < nLossCells_; ++k) {
    // Mean |q| over the block x pi / 2: the flow's amplitude.
    const double amp = lossAcc_[k] / 16 * M_PI / 2;
    lossAcc_[k] = 0;
    const double r = lossK_[k] * amp;
    if (std::fabs(r - lossR_[k]) <= 0.1 * lossR_[k] + 1e3) continue;
    lossR_[k] = r;
    bool any = false;
    const Branch b = cellBranch(lossCell_[k], &any);
    if (any) bore_.setBranchNow(lossCell_[k], b);
  }
}

void Tube::process(float* out, int n) {
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

void Tube::resetAir() {
  std::memset(&bore_.s, 0, sizeof(bore_.s));
  // The holes' nonlinear losses live in the coefficients: start them afresh too.
  for (int k = 0; k < nLossCells_; ++k) {
    lossAcc_[k] = lossR_[k] = 0;
    bool any = false;
    const Branch b = cellBranch(lossCell_[k], &any);
    bore_.setBranchNow(lossCell_[k], b);
  }
  std::fill(outRing_, outRing_ + kOutRing, 0.0f);
  std::fill(radPrev_, radPrev_ + kMaxHoles + 2, 0.0f);
  std::fill(radPrev2_, radPrev2_ + kMaxHoles + 2, 0.0f);
  dcX_ = dcY_ = 0;
}

void Tube::stepBore(float* pOld, float* jIn) {
  const double dt = 1 / fs_;
  if (roomPos_ < roomN_) pRoom_ = room_[roomPos_++];
  bore_.step(ec_, pOld, jIn, pRoom_);
  for (int k = 0; k < nLossCells_; ++k) lossAcc_[k] += std::fabs(bore_.s.q[lossCell_[k]]);
  if (nPumping_ > 0)  // pads pushing air into the tube (key clicks)
    for (int i = 0; i < nHoles_; ++i) {
      const Hole& hl = hole_[i];
      if (hl.padIn != 0) bore_.s.p()[hl.cell] += float(hl.padIn * dt / bore_.compliance(hl.cell));
    }
  if (thudLeft_ > 0) {  // landing thuds
    --thudLeft_;
    for (int i = 0; i < nHoles_; ++i) {
      Hole& hl = hole_[i];
      if (hl.thRe == 0 && hl.thIm == 0) continue;
      bore_.s.p()[hl.cell] += float(hl.thRe * dt / bore_.compliance(hl.cell));
      const float re = hl.thRe, im = hl.thIm;
      hl.thRe = thudDecay_ * (re * thudCos_ - im * thudSin_);
      hl.thIm = thudDecay_ * (re * thudSin_ + im * thudCos_);
      if (thudLeft_ == 0) hl.thRe = hl.thIm = 0;
    }
  }
}

float Tube::radiate(double windowQ) {
  for (int r = 0; r < nRad_; ++r) {
    const int c = radCell_[r];
    if (kRadOnly_ != 0 && (kRadOnly_ == -1) != (c == -1)) continue;
    float add;
    {
      // d/dt of the mean of the last two flows: a zero at Nyquist, so a pad
      // closing (stiff, trapezoidal) does not click at 24 kHz.
      const float q = c == -1 ? float(windowQ) : bore_.s.q[c];
      add = radGain_[r] * 0.5f * (q - radPrev2_[r]);
      radPrev2_[r] = radPrev_[r];
      radPrev_[r] = q;
    }
    outRing_[(outPos_ + radDelay_[r]) & (kOutRing - 1)] += add;
  }
  const float y = outRing_[outPos_];
  outRing_[outPos_] = 0;
  return y;
}

float Tube::finish(float y) {
  for (int m = 0; m < kTickModes; ++m) {  // key impact ticks
    const double re = tickRe_[m], im = tickIm_[m];
    tickRe_[m] = tickDecay_[m] * (re * tickCos_[m] - im * tickSin_[m]);
    tickIm_[m] = tickDecay_[m] * (re * tickSin_[m] + im * tickCos_[m]);
    y += float(im);
  }
  outPos_ = (outPos_ + 1) & (kOutRing - 1);
  // DC blocker (~5 Hz).
  const float yd = y - dcX_ + dcPole_ * dcY_;
  dcX_ = y;
  dcY_ = yd;
  return float(yd * kOut_);
}

}  // namespace pajupilli
