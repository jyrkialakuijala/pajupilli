#include "bassbody.h"

#include <cmath>
#include <cstring>

#include "cellobody_data.h"
#include "knob.h"
#include "violabody_data.h"
#include "violinbody_data.h"

namespace pajupilli {

using namespace bassdata;

extern const BodyData kBassBodyData = {kTopK, kTopInvM, kBackK, kBackInvM, kNodeArea, kRowY, kColX, kAirK, kSlugArea,
                                       kSlugInv, kPlugArea, kPlugInvM, kPostBehind, kBridgeI, kBassJ, kTrebleJ, 250, 700};
static_assert(cellodata::kNX == kNX && cellodata::kNY == kNY && cellodata::kOffsets == kOffsets, "the same grid");
// (A cello's top is 0.65 x the bass's: its sides differ from 0.65 x shorter waves on.)
extern const BodyData kCelloBodyData = {cellodata::kTopK, cellodata::kTopInvM, cellodata::kBackK, cellodata::kBackInvM,
                                        cellodata::kNodeArea, cellodata::kRowY, cellodata::kColX, cellodata::kAirK,
                                        cellodata::kSlugArea, cellodata::kSlugInv, cellodata::kPlugArea, cellodata::kPlugInvM,
                                        cellodata::kPostBehind, cellodata::kBridgeI, cellodata::kBassJ, cellodata::kTrebleJ,
                                        385, 1080, 1, 0.117f};
static_assert(violadata::kNX == kNX && violadata::kNY == kNY && violadata::kOffsets == kOffsets, "the same grid");
// (The viola's top is 0.54 x the cello's: its sides differ from 1.85 x the cello's frequencies on.)
extern const BodyData kViolaBodyData = {violadata::kTopK, violadata::kTopInvM, violadata::kBackK, violadata::kBackInvM,
                                        violadata::kNodeArea, violadata::kRowY, violadata::kColX, violadata::kAirK,
                                        violadata::kSlugArea, violadata::kSlugInv, violadata::kPlugArea, violadata::kPlugInvM,
                                        // (The post 17 mm behind the treble foot, 0.65 of a node, not the design's 7 mm
                                        // (violadata::kPostBehind 0.26): on this grid a post that near pins the foot, and the
                                        // A string, which presses that foot most, came out weak: the open A 5 dB under the
                                        // range's median, 2.4 dB at 17 mm. Viola.md, finding 11.)
                                        0.65f, violadata::kBridgeI, violadata::kBassJ, violadata::kTrebleJ,
                                        710, 2000, 3, 0.038f, float(knob("PJ_AIR_HF", 0.7e-6)),  // (Q 46 at 4.9 kHz, 1000 at A0)
                                        int(knob("PJ_VIOLA_HOLES", 3))};
static_assert(violindata::kNX == kNX && violindata::kNY == kNY && violindata::kOffsets == kOffsets, "the same grid");
// (The violin's top is 0.75 x the viola's: its sides differ from 1.15 x the viola's frequencies on.)
extern const BodyData kViolinBodyData = {violindata::kTopK, violindata::kTopInvM, violindata::kBackK, violindata::kBackInvM,
                                         violindata::kNodeArea, violindata::kRowY, violindata::kColX, violindata::kAirK,
                                         violindata::kSlugArea, violindata::kSlugInv, violindata::kPlugArea, violindata::kPlugInvM,
                                         violindata::kPostBehind, violindata::kBridgeI, violindata::kBassJ, violindata::kTrebleJ,
                                         820, 2300, 4, 0.031f, float(knob("PJ_AIR_HF", 2.2e-6))};

BassBody::BassBody(double fs, const BodyData& data) : d_(data), dt_(float(1 / (fs * data.substeps))) {
  fs_ = fs;
  sideA_ = float(1 - std::exp(-2 * M_PI * d_.sideLo * dt_));
  sideB_ = float(1 - std::exp(-2 * M_PI * d_.sideHi * dt_));
  // The air crossing between the body's two halves, in each cell: a slug as long as half the
  // body's width there (centre to centre), through the body's depth.
  sides_ = knob("PJ_AIR_SIDES", 1) > 0;
  const float dx = d_.colX[1] - d_.colX[0], dy = d_.rowY[1] - d_.rowY[0];
  crossArea_ = dx * d_.depth;
  for (int i = 0; i < kNX; ++i) {
    int rows = 0;
    for (int j = 0; j < kNY; ++j) rows += d_.topInvM[j][i] > 0;
    crossInv_[i] = rows >= 2 ? d_.slugInv * dx / (0.5f * rows * dy) : 0.0f;
  }
  // The f-holes: one each side of the bridge, about a fifth of the body's width from its middle.
  holeY_ = 0.2f * kNY * dy;
}

namespace {
// (PJ_POST_BEHIND: the post's place behind the treble foot, as a share of a node's spacing,
// instead of the design's: to try it nearer or further.)
float postBehind(const BodyData& d) {
  static const float kPostKnob = float(knob("PJ_POST_BEHIND", -1));
  return kPostKnob >= 0 ? kPostKnob : d.postBehind;
}
}  // namespace

void BassBody::prepare() {
  dirty_ = false;
  static const bool kGrid = knob("PJ_BODY_GRID", 0) > 0;
  if (grid_ < 0 ? kGrid : grid_ > 0) {
    modes_.reset();
    return;
  }
  modes_ = bodyModes({&d_, fs_, rate_, stiff_, airRate_, plateK_, airKs_, postBehind(d_), sides_});
  zr_.assign(modes_->groups.size(), BodyModes::v16{});
  zi_.assign(modes_->groups.size(), BodyModes::v16{});
  std::memset(phi_, 0, sizeof phi_);
  std::memset(feet_, 0, sizeof feet_);
  fhole_ = side_ = 0;
  std::memset(dip_, 0, sizeof dip_);
}

float BassBody::stepModes(float bassFoot, float trebleFoot) {
  using v16 = BodyModes::v16;
  const BodyModes& m = *modes_;
  constexpr int kG = BodyModes::kGlobal, kO = BodyModes::kOut;
  // The low-passes' own states (the sides', the directions'), driven by the feet.
  float phi[kG];
  for (int r = 0; r < kG; ++r) {
    float s = m.qs[r][0] * bassFoot + m.qs[r][1] * trebleFoot;
    for (int c = 0; c < kG; ++c) s += m.ps[r][c] * phi_[c];
    phi[r] = s;
  }
  std::memcpy(phi_, phi, sizeof phi);
  // The modes, 16 at a time: each output's share of their states, then their step.
  v16 acc[kO] = {};
  const int ng = int(m.groups.size());
  for (int g = 0; g < ng; ++g) {
    const BodyModes::Group& c = m.groups[g];
    const v16 zr = zr_[g], zi = zi_[g];
    for (int o = 0; o < kO; ++o) acc[o] += c.wr[o] * zr - c.wi[o] * zi;
    zr_[g] = c.lr * zr - c.li * zi + c.b0r * bassFoot + c.b1r * trebleFoot;
    zi_[g] = c.lr * zi + c.li * zr + c.b0i * bassFoot + c.b1i * trebleFoot;
  }
  float y[kO];
  for (int o = 0; o < kO; ++o) {
    float s = m.d[o][0] * bassFoot + m.d[o][1] * trebleFoot;
    for (int l = 0; l < 16; ++l) s += acc[o][l];
    for (int r = 0; r < kG; ++r) s += m.e[o][r] * phi[r];
    y[o] = s;
  }
  fhole_ = y[1];
  side_ = y[2];
  dip_[0] = y[3];
  dip_[1] = y[4];
  dip_[2] = y[5];
  feet_[0] = y[6];
  feet_[1] = y[7];
  feet_[2] = y[8];
  feet_[3] = y[9];
  return y[0];
}

void BassBody::reset() {
  for (auto& z : zr_) z = BodyModes::v16{};
  for (auto& z : zi_) z = BodyModes::v16{};
  std::memset(phi_, 0, sizeof phi_);
  std::memset(feet_, 0, sizeof feet_);
  std::memset(w_, 0, sizeof w_);
  std::memset(v_, 0, sizeof v_);
  std::memset(f_, 0, sizeof f_);
  std::memset(fPrev_, 0, sizeof fPrev_);
  std::memset(plugW_, 0, sizeof plugW_);
  std::memset(plugV_, 0, sizeof plugV_);
  std::memset(crossW_, 0, sizeof crossW_);
  std::memset(crossV_, 0, sizeof crossV_);
  std::memset(dip_, 0, sizeof dip_);
  std::memset(dip1_, 0, sizeof dip1_);
  std::memset(momPrev_, 0, sizeof momPrev_);
  std::memset(pressPrev_, 0, sizeof pressPrev_);
  side_ = side1_ = dipPrev_ = 0;
  std::memset(slugW_, 0, sizeof slugW_);
  std::memset(slugV_, 0, sizeof slugV_);
}

float BassBody::stepOne(float bassFoot, float trebleFoot) {
  // (the design's numbers as locals, under the names the generated header gives them)
  const float (*K[2])[kNY][kNX] = {d_.topK, d_.backK};
  const float (*invM[2])[kNX] = {d_.topInvM, d_.backInvM};
  const float kNodeArea = d_.nodeArea, kSlugInv = d_.slugInv, kPlugArea = d_.plugArea, kPlugInvM = d_.plugInvM;
  const float kPostBehind = postBehind(d_);
  const float* const kRowY = d_.rowY;
  const float* const kColX = d_.colX;
  const float* const kAirK = d_.airK;
  const float* const kSlugArea = d_.slugArea;
  const int kBridgeI = d_.bridgeI, kBassJ = d_.bassJ, kTrebleJ = d_.trebleJ;
  // --- Collect the forces. The plates' stiffness: each mass and its 13
  // neighbours (rows of 16: one register each). The air cells' volumes: the
  // plates' rows summed down each column (one register).
  alignas(64) float vol[2][kNX];  // [the bass side's half, the treble side's]
  for (int i = 0; i < kNX; ++i) vol[0][i] = vol[1][i] = 0;
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
      float* vs = vol[j >= kNY / 2];
      for (int i = 0; i < kNX; ++i) vs[i] += kNodeArea * w[i];
    }
  }
  // The wood's internal friction: the stiffness force's rate of change.
  for (int p = 0; p < 2; ++p)
    for (int j = 0; j < kNY; ++j) {
      float* f = f_[p][j];
      float* fp = fPrev_[p][j];
      for (int i = 0; i < kNX; ++i) {
        const float fk = plateK_ * f[i];
        f[i] = fk + stiff_ / dt_ * (fk - fp[i]);
        fp[i] = fk;
      }
    }
  // The air: 16 cells along the body. Each cell's volume: the plates over it,
  // the slugs of air leaving it for its neighbours, the f-holes' plugs (at
  // the bridge's cell). Its pressure pushes the plates, the slugs, the plugs.
  // (Each half of a cell has half its volume and half its passages; the air crosses between
  // the halves. Moved alike, the two halves are the one column of before.)
  alignas(64) float press[2][kNX + 1];
  for (int s = 0; s < 2; ++s) {
    for (int i = 0; i < kNX; ++i)
      vol[s][i] += 0.5f * (kSlugArea[i] * slugW_[s][i + 1] - (i > 0 ? kSlugArea[i - 1] : 0.0f) * slugW_[s][i]) +
                   (s == 0 ? crossArea_ : -crossArea_) * crossW_[i];
    if (d_.holeCells >= 3) {
      vol[s][kBridgeI - 1] += 0.125f * kPlugArea * plugW_[s];
      vol[s][kBridgeI] += 0.25f * kPlugArea * plugW_[s];
      vol[s][kBridgeI + 1] += 0.125f * kPlugArea * plugW_[s];
    } else {
      vol[s][kBridgeI] += 0.5f * kPlugArea * plugW_[s];
    }
  }
  for (int i = 0; i < kNX; ++i) {
    if (sides_) {
      press[0][i] = -2 * airKs_ * kAirK[i] * vol[0][i];
      press[1][i] = -2 * airKs_ * kAirK[i] * vol[1][i];
    } else {
      press[0][i] = press[1][i] = -airKs_ * kAirK[i] * (vol[0][i] + vol[1][i]);
    }
  }
  press[0][kNX] = press[1][kNX] = 0;
  for (int p = 0; p < 2; ++p)
    for (int j = 0; j < kNY; ++j) {
      float* f = f_[p][j];
      const float* im = invM[p][j];
      const float* pr = press[j >= kNY / 2];
      for (int i = 0; i < kNX; ++i) f[i] += im[i] > 0 ? pr[i] * kNodeArea : 0.0f;
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
  float mom[3] = {0, 0, 0};  // the displaced volume's moments (both plates): across, along, out of the top
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
      mom[0] += kNodeArea * s * kRowY[j];
      mom[1] += kNodeArea * sx;
      mom[2] += (p == 0 ? 0.5f : -0.5f) * d_.depth * kNodeArea * s;
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
  // (The air's losses that grow with frequency: BodyData::airHf. The plates feel the pressure
  // itself, above.)
  if (d_.airHf > 0) {
    const float h = d_.airHf / dt_;
    for (int s = 0; s < 2; ++s)
      for (int i = 0; i < kNX; ++i) {
        const float p = press[s][i];
        press[s][i] = p + h * (p - pressPrev_[s][i]);
        pressPrev_[s][i] = p;
      }
  }
  for (int s = 0; s < 2; ++s)
    for (int i = 0; i + 1 < kNX; ++i) {  // slug i: between cells i and i + 1 (slugW_[i + 1])
      const float a = (press[s][i] - press[s][i + 1]) * kSlugInv;
      slugV_[s][i + 1] = keepSlug * slugV_[s][i + 1] + a * dt_;
      slugW_[s][i + 1] += slugV_[s][i + 1] * dt_;
    }
  if (sides_)
    for (int i = 0; i < kNX; ++i) {  // across, from the bass side's half to the treble side's
      const float a = (press[0][i] - press[1][i]) * crossInv_[i];
      crossV_[i] = keepSlug * crossV_[i] + a * dt_;
      crossW_[i] += crossV_[i] * dt_;
    }
  // The two f-holes, each from its own half: the air out of both together (the air resonance:
  // heard all round), or in at one and out at the other (heard to the sides, with opposite signs).
  fhole_ = 0;
  for (int s = 0; s < 2; ++s) {
    // (a long hole feels its cells' pressures as it takes its air from them)
    const float ph = d_.holeCells >= 3 ? 0.25f * press[s][kBridgeI - 1] + 0.5f * press[s][kBridgeI] + 0.25f * press[s][kBridgeI + 1]
                                       : press[s][kBridgeI];
    const float a = ph * kPlugArea * kPlugInvM;
    const float q = 0.5f * kPlugArea * a;
    accel += q;
    fhole_ += q;
    mom[0] += (s == 0 ? -holeY_ : holeY_) * q;
    mom[1] += kColX[kBridgeI] * q;
    mom[2] += 0.5f * d_.depth * q;
    plugV_[s] = keepAir * plugV_[s] + a * dt_;
    plugW_[s] += plugV_[s] * dt_;
  }
  // Each moment's sound along its axis: its rate of change over the speed of sound, while the
  // body is small against the wavelength; above (the first low-pass of the sides' difference)
  // it stops growing and stays as large as the moment's own volume is: there the body's patches
  // send the sound out in lobes, each overtone in its own, and the moments carry what a
  // listener to one side gets of that and one to the other does not. (Not the second low-pass:
  // the directions matter most where the overtones are many.)
  for (int k = 0; k < 3; ++k) {
    dip1_[k] += sideA_ * ((mom[k] - momPrev_[k]) / (dt_ * 349.5f) - dip1_[k]);
    dip_[k] = dip1_[k];
    momPrev_[k] = mom[k];
  }
  return accel;
}

}  // namespace pajupilli
