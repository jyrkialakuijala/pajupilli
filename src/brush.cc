#include "brush.h"

#include <algorithm>
#include <cmath>

namespace pajupilli {

namespace {
constexpr int kTex = 8192;
double urand(uint32_t& s) {
  s = s * 1664525u + 1013904223u;
  return (s >> 8) * (1.0 / 16777216.0);
}
}  // namespace

Brush::Brush(double fs, Surface& head, const BrushSpec& spec, uint32_t seed)
    : head_(head), s_(spec), fs_(fs), dt_(1 / fs),
      n_(spec.modes > 0 ? std::min(spec.modes, head.modes()) : head.modes()), rng_(4711u + 104729u * seed) {
  full_.assign(head.modes(), 0.0f);
  tip_.resize(s_.tips);
  phi_.assign(size_t(s_.tips) * n_, 0.0f);
  // The grain: heights at one grain's spacing, each a little like its
  // neighbour (the wire end is as wide as a grain or two), unit rms.
  tex_.resize(kTex);
  uint32_t r = 31337u + seed;
  double y = 0, sum2 = 0;
  for (int i = 0; i < kTex + 64; ++i) {
    const double w = urand(r) + urand(r) + urand(r) - 1.5;
    y = 0.5 * y + w;
    if (i >= 64) { tex_[i - 64] = float(y); sum2 += y * y; }
  }
  const float g = float(1 / std::sqrt(sum2 / kTex));
  for (float& t : tex_) t *= g;
  layout();
}

void Brush::layout(double share) {
  // (The tip alone: the ends that reach the head lie close together; the
  // rest of the fan stays in the air.)
  const double k = std::clamp(share, 0.1, 1.0), foot = 0.25 + 0.75 * k;
  active_ = std::max(2, int(std::lround(k * s_.tips)));
  for (Tip& t : tip_) {
    t.u = (urand(rng_) - 0.5) * s_.length * foot;
    t.w = (urand(rng_) - 0.5) * s_.width * foot;
    t.h = 0;
    t.z = t.v = 0;
    t.travel = urand(rng_) * kTex;
    t.on = false;
  }
}

double Brush::heightAt(int i) const {
  const float *phi = &phi_[size_t(i) * n_], *q = head_.q();
  float z = 0;
  for (int k = 0; k < n_; ++k) z += phi[k] * q[k];
  return z;
}

void Brush::shapes(int i) {
  Tip& t = tip_[i];
  const double c = std::cos(ang_), s = std::sin(ang_);
  // (across the fan: at right angles to the wires; along: with them.)
  const double px = x_ - s * t.u + c * t.w, py = y_ + c * t.u + s * t.w;
  const double r = std::min(std::hypot(px, py), 0.97 * head_.radius());
  head_.shape(r, std::atan2(py, px), full_.data());
  std::copy(full_.begin(), full_.begin() + n_, phi_.begin() + size_t(i) * n_);
}

void Brush::place(double x, double y, double angle) { xTo_ = x; yTo_ = y; angTo_ = angle; }

void Brush::jump(double x, double y, double angle) {
  x_ = xTo_ = x; y_ = yTo_ = y; ang_ = angTo_ = angle;
}

void Brush::strike(double speed, double spread, double follow, double dwell, double share) {
  layout(share);
  x_ = xTo_; y_ = yTo_; ang_ = angTo_;
  double low = 0;
  for (int i = 0; i < s_.tips; ++i) {
    Tip& t = tip_[i];
    // The tilt: the ends hang lower along the fan, each a little off the line.
    t.h = spread * ((t.u / s_.length + 0.5) * 0.8 + 0.2 * urand(rng_));
    if (i >= active_) { t.h = -0.05; continue; }  // (held back: 5 cm up, never down)
    shapes(i);
    low = std::max(low, heightAt(i) + t.h);
  }
  // The lowest end a hair over the head as it is now, all coming down together.
  zHandle_ = zHand_ = low + 0.2e-3;
  vHandle_ = vHand_ = -speed;
  for (Tip& t : tip_) { t.z = zHandle_ - t.h; t.v = -speed; }
  speed_ = speed; follow_ = follow; dwell_ = dwell;
  mode_ = kDown;
  t_ = 0;
}

void Brush::rest(double load, double share) {
  if (load <= 0) {
    if (mode_ == kRest) { mode_ = kLift; speed_ = 0.3; }
    return;
  }
  if (mode_ != kRest) {
    layout(share);
    x_ = xTo_; y_ = yTo_; ang_ = angTo_;
    double low = 0;
    for (int i = 0; i < s_.tips; ++i) {
      Tip& t = tip_[i];
      if (i >= active_) { t.h = -0.05; continue; }  // (held back: in the air)
      t.h = 2.0e-3 * urand(rng_);  // (a light hand: only the longest ends touch; a heavy one: all)
      shapes(i);
      low = std::max(low, heightAt(i) + t.h);
    }
    zHandle_ = zHand_ = low + 0.3e-3;
    vHandle_ = vHand_ = 0;
    for (Tip& t : tip_) { t.z = zHandle_ - t.h; t.v = 0; }
    zTouch_ = low;
    mode_ = kRest;
  }
  load_ = load;
}

void Brush::step() {
  force_ = scrape_ = 0;
  if (mode_ == kUp) return;
  // The hand's path over the head: towards where it was sent, within a few ms.
  const double k = 1 - std::exp(-dt_ / 0.004);
  const double dx = (xTo_ - x_) * k, dy = (yTo_ - y_) * k;
  x_ += dx; y_ += dy;
  ang_ += (angTo_ - ang_) * k;
  const double moved = std::hypot(dx, dy);
  // Which way it is drawn against the wires' lie: +1 pushed point first, -1 pulled.
  const double lead = moved > 1e-9 ? (dx * std::cos(ang_) + dy * std::sin(ang_)) / moved : 0;
  drawn_ += moved;
  if (moved > 1e-7 && drawn_ >= s_.reshape) {  // one end's shares a sample, in turn (those that are down)
    drawn_ = 0;
    if (next_ >= active_) next_ = 0;
    shapes(next_);
    next_ = (next_ + 1) % std::max(active_, 1);
  }
  const double ca = std::cos(ang_), sa = std::sin(ang_);
  // The hand's height.
  t_ += dt_;
  switch (mode_) {
    case kDown:
      if (touching_ > 0) { mode_ = kFollow; zTouch_ = zHand_; }
      break;
    case kFollow: {
      // Slowing over the follow distance, then stopping.
      const double left = follow_ - (zTouch_ - zHand_);
      vHand_ = -speed_ * std::clamp(left / std::max(follow_, 1e-5), 0.0, 1.0) - 0.02;
      if (left <= 0) { mode_ = kDwell; t_ = 0; vHand_ = 0; }
      break;
    }
    case kDwell:
      if (t_ >= dwell_) { mode_ = kLift; t_ = 0; }
      break;
    case kLift:
      vHand_ += (0.5 * speed_ + 0.3 - vHand_) * (1 - std::exp(-dt_ / 0.003));
      if (touching_ == 0 && zHand_ > zTouch_ + 4e-3) { mode_ = kUp; return; }
      break;
    case kRest: {
      // The hand leans on the fan until it carries the load.
      const double want = load_ / s_.fan;
      vHand_ = ((zTouch_ - want) - zHand_) / 0.02;
      break;
    }
    default: break;
  }
  zHand_ += vHand_ * dt_;
  // The handle on the grip; the fan from the handle to each end.
  const int nt = s_.tips;
  const double kf = s_.fan / nt, cf = 2 * s_.fanLoss * std::sqrt(s_.fan * s_.handle) / nt;
  const double cg = 2 * s_.gripLoss * std::sqrt(s_.grip * s_.handle);
  double fHandle = s_.grip * (zHand_ - zHandle_) + cg * (vHand_ - vHandle_);
  // The fan's friction: the wires rub on each other as the fan is squeezed,
  // which holds back the fan as a whole (its ends' mean) far more than it
  // ties any one wire end to the handle: left to themselves the ends each
  // bounce and chatter on what they strike (together: the share of that
  // friction that does act end by end).
  double vbar = 0;
  for (int i = 0; i < active_; ++i) vbar += tip_[i].v / std::max(active_, 1);
  const double cOwn = s_.together * cf, cAll = (1 - s_.together) * cf;
  const double cTip = 2 * s_.tipLoss * std::sqrt(kf * s_.tipMass);
  touching_ = 0;
  for (int i = 0; i < nt; ++i) {
    Tip& t = tip_[i];
    const float* phi = &phi_[size_t(i) * n_];
    if (i >= active_) continue;  // (held back in the air)
    const double fFan = kf * (zHandle_ - t.h - t.z) + (cOwn + cTip) * (vHandle_ - t.v) + cAll * (vHandle_ - vbar);
    fHandle -= fFan;
    // The grain under the end, read as it is drawn along.
    t.travel += moved / s_.grain;
    if (t.travel >= kTex) t.travel -= kTex;
    const int j = int(t.travel);
    const double fr = t.travel - j;
    double g = s_.rough * (1 + s_.bite * lead) * (tex_[j] + fr * (tex_[(j + 1) % kTex] - tex_[j]));
    if (s_.ridge > 0) {
      // The lathing under this end, at its own radius: a ridge, rounded,
      // every groove's width (the end, ~0.3 mm, rides over its crest).
      const double px = x_ - sa * t.u + ca * t.w, py = y_ + ca * t.u + sa * t.w;
      const double c = std::hypot(px, py) / s_.groove;
      const double f = c - std::floor(c) - 0.5;
      g += s_.ridge * (1 / 3.0 - 4 * f * f);  // (its mean 0)
    }
    float zs = 0, vs = 0;
    {
      const float *q = head_.q(), *v = head_.v();
      for (int k = 0; k < n_; ++k) { zs += phi[k] * q[k]; vs += phi[k] * v[k]; }
    }
    const double zh = zs, vh = vs;
    const double pen = zh + g - t.z;  // how far the end is into the film
    double fc = 0;
    if (pen > 0) {
      fc = stiff_ * s_.contact * pen * std::max(0.0, 1 + s_.contactLoss * (vh - t.v));
      // Drawn along, the wire leans on the grain it meets: its press on
      // the head rises and falls with the grain, in proportion to the press.
      const int j2 = (j * 7 + 1234) % kTex;
      fc *= std::max(0.0, 1 + s_.drag * (1 + s_.bite * lead) * (tex_[j2] + fr * (tex_[(j2 + 7) % kTex] - tex_[j2])) * std::min(1.0, moved * fs_ / 0.05));
      ++touching_;
      scrape_ += float(stiff_ * s_.contact * g);
    }
    t.on = pen > 0;
    t.v += dt_ * (fFan + fc) / s_.tipMass;
    t.z += dt_ * t.v;
    if (fc > 0) {
      float* f = head_.force();
      const float g = float(-fc);
      for (int k = 0; k < n_; ++k) f[k] += phi[k] * g;
    }
    force_ += float(fc);
  }
  vHandle_ += dt_ * fHandle / s_.handle;
  zHandle_ += dt_ * vHandle_;
}

}  // namespace pajupilli
