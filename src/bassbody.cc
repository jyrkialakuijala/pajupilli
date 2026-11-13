#include "bassbody.h"

#include <cmath>
#include <cstring>

namespace pajupilli {

using namespace bassdata;

BassBody::BassBody(double fs) : dt_(float(1 / fs)) {
  sideA_ = float(1 - std::exp(-2 * M_PI * 250 / fs));
  sideB_ = float(1 - std::exp(-2 * M_PI * 700 / fs));
}

void BassBody::reset() {
  std::memset(w_, 0, sizeof w_);
  std::memset(v_, 0, sizeof v_);
  std::memset(f_, 0, sizeof f_);
  std::memset(fPrev_, 0, sizeof fPrev_);
  plugW_ = plugV_ = 0;
  side_ = side1_ = dipPrev_ = 0;
  std::memset(slugW_, 0, sizeof slugW_);
  std::memset(slugV_, 0, sizeof slugV_);
}

float BassBody::step(float bassFoot, float trebleFoot) {
  const float (*K[2])[kNY][kNX] = {kTopK, kBackK};
  const float (*invM[2])[kNX] = {kTopInvM, kBackInvM};
  // --- Collect the forces. The plates' stiffness: each mass and its 13
  // neighbours (rows of 16: one register each). The air cells' volumes: the
  // plates' rows summed down each column (one register).
  alignas(64) float vol[kNX];
  for (int i = 0; i < kNX; ++i) vol[i] = 0;
  for (int p = 0; p < 2; ++p) {
    for (int j = 0; j < kNY; ++j) {
      float* f = f_[p][j];
      for (int i = 0; i < kNX; ++i) f[i] = 0;
      for (int o = 0; o < kOffsets; ++o) {
        const float* c = K[p][o][j];
        const float* w = &w_[p][j + kPad + kOffJ[o]][kPad + kOffI[o]];
        for (int i = 0; i < kNX; ++i) f[i] -= c[i] * w[i];
      }
      const float* w = &w_[p][j + kPad][kPad];
      for (int i = 0; i < kNX; ++i) vol[i] += kNodeArea * w[i];
    }
  }
  // The wood's internal friction: the stiffness force's rate of change.
  for (int p = 0; p < 2; ++p)
    for (int j = 0; j < kNY; ++j) {
      float* f = f_[p][j];
      float* fp = fPrev_[p][j];
      for (int i = 0; i < kNX; ++i) {
        const float fk = f[i];
        f[i] = fk + stiff_ / dt_ * (fk - fp[i]);
        fp[i] = fk;
      }
    }
  // The air: 16 cells along the body. Each cell's volume: the plates over it,
  // the slugs of air leaving it for its neighbours, the f-holes' plugs (at
  // the bridge's cell). Its pressure pushes the plates, the slugs, the plugs.
  alignas(64) float press[kNX + 1];
  for (int i = 0; i < kNX; ++i)
    vol[i] += kSlugArea[i] * slugW_[i + 1] - (i > 0 ? kSlugArea[i - 1] : 0.0f) * slugW_[i];
  vol[kBridgeI] += kPlugArea * plugW_;
  for (int i = 0; i < kNX; ++i) press[i] = -kAirK[i] * vol[i];
  press[kNX] = 0;
  for (int p = 0; p < 2; ++p)
    for (int j = 0; j < kNY; ++j) {
      float* f = f_[p][j];
      const float* im = invM[p][j];
      for (int i = 0; i < kNX; ++i) f[i] += im[i] > 0 ? press[i] * kNodeArea : 0.0f;
    }
  // The bridge's feet.
  f_[0][kBassJ][kBridgeI] += bassFoot;
  f_[0][kTrebleJ][kBridgeI] += trebleFoot;
  // The sound post, a little behind the treble foot: rigid. Its ends (a point
  // of the top, the point of the back under it) move together: the top going
  // in is the back going out (both displacements are outward-positive, so
  // their sum there stays 0). The post's force is whatever keeps it so after
  // this step, with all the other forces known.
  {
    const int r = kTrebleJ + kPad, c = kBridgeI + kPad;
    const float a = 1 - kPostBehind, b = kPostBehind;
    const float keep = 1 - rate_ * dt_;
    float g = 0, gv = 0, ga = 0, cmc = 0;
    for (int p = 0; p < 2; ++p) {
      const float i0 = invM[p][kTrebleJ][kBridgeI], i1 = invM[p][kTrebleJ][kBridgeI + 1];
      g += a * w_[p][r][c] + b * w_[p][r][c + 1];
      gv += a * v_[p][r][c] + b * v_[p][r][c + 1];
      ga += a * f_[p][kTrebleJ][kBridgeI] * i0 + b * f_[p][kTrebleJ][kBridgeI + 1] * i1;
      cmc += a * a * i0 + b * b * i1;
    }
    const float lambda = (-g / (dt_ * dt_) - keep * gv / dt_ - ga) / cmc;
    for (int p = 0; p < 2; ++p) {
      f_[p][kTrebleJ][kBridgeI] += lambda * a;
      f_[p][kTrebleJ][kBridgeI + 1] += lambda * b;
    }
  }
  // --- Velocities, then displacements; the radiated sound: the volume's
  // acceleration.
  const float keep = 1 - rate_ * dt_;
  float accel = 0, dipY = 0, dipX = 0;
  for (int p = 0; p < 2; ++p)
    for (int j = 0; j < kNY; ++j) {
      const float* f = f_[p][j];
      const float* im = invM[p][j];
      float* v = &v_[p][j + kPad][kPad];
      float* w = &w_[p][j + kPad][kPad];
      float s = 0, sx = 0;
      for (int i = 0; i < kNX; ++i) {
        const float a = f[i] * im[i];
        s += a;
        sx += a * kColX[i];
        v[i] = keep * v[i] + a * dt_;
        w[i] += v[i] * dt_;
      }
      accel += kNodeArea * s;
      if (p == 0) {  // where on the top the motion is: across it, along it
        dipY += kNodeArea * s * kRowY[j];
        dipX += kNodeArea * sx;
      }
    }
  // To one side of the bass the top's near half is heard a little earlier
  // than its far half: the sound there is the whole volume's (above) plus
  // the rate of change of the motion's moment across the top, over the speed
  // of sound (the first term of the top's directivity; with a share of the
  // moment along the top, for ears or microphones not level with its middle).
  // Each resonance of the plates has its own moment, so each overtone its own
  // balance between the two sides.
  {
    const float m = dipY + 0.5f * dipX;
    // (That first term holds while the top is small against the wavelength:
    // up to ~250 Hz. Above, the difference stops growing: a low-pass there.)
    side1_ += sideA_ * ((m - dipPrev_) / (dt_ * 349.5f) - side1_);
    // (Higher still, the top's patches are many and small: their differences
    // average out towards the sides: a second, gentler low-pass at 700 Hz.)
    side_ += sideB_ * (side1_ - side_);
    dipPrev_ = m;
  }
  // (The air along the body rubs on the plates and ribs: more loss than the
  // plugs in the f-holes.)
  const float keepAir = 1 - airRate_ * dt_, keepSlug = 1 - 4 * airRate_ * dt_;
  for (int i = 0; i + 1 < kNX; ++i) {  // slug i: between cells i and i + 1 (slugW_[i + 1])
    const float a = (press[i] - press[i + 1]) * kSlugInv;
    slugV_[i + 1] = keepSlug * slugV_[i + 1] + a * dt_;
    slugW_[i + 1] += slugV_[i + 1] * dt_;
  }
  {
    const float a = press[kBridgeI] * kPlugArea * kPlugInvM;
    accel += kPlugArea * a;
    fhole_ = kPlugArea * a;
    plugV_ = keepAir * plugV_ + a * dt_;
    plugW_ += plugV_ * dt_;
  }
  return accel;
}

}  // namespace pajupilli
