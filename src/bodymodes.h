// The bowed instruments' body as its modes (bassbody.h: BassBody runs on these unless
// PJ_BODY_GRID=1).
//
// The grid of bassbody.h is linear and does not change while it plays: the plates' masses and
// springs, the air cells, slugs and plugs, the post (a linear constraint), the losses (all
// proportional to a displacement or a velocity) and the instrument's own build (setVariation)
// are fixed once the instrument is made. So it is a linear time-invariant system from the two
// feet's forces to everything that is read from it (the radiated sound, the f-hole, the two
// sides' difference, the three directional parts, the feet's motion), and it can be run as its
// modes: each an independent resonance (a complex pole), driven by the feet in its own shares and
// heard in each output in its own shares.
//
// How they are found, at start-up, for each design, sample rate, damping and build (cached: a
// section's instruments of one seat share them):
//   1. The grid's equations, as matrices (masses, stiffness, losses, the feet, the outputs).
//      The post leaves one degree of freedom fewer (eliminated in the mass-weighted coordinates).
//   2. The undamped modes: a symmetric eigenproblem (Householder tridiagonal, implicit QL).
//   3. The losses are not proportional (the plates, the air slugs and the plugs lose at their own
//      rates, the wood's friction acts on the plates' stiffness alone, the air's on its own): in
//      the undamped modes they couple modes near each other. The grid's step (one explicit step of
//      the leapfrog, or its substeps) is written in these modes, each mode's own 2 x 2 step solved
//      exactly, and the couplings removed by 2 x 2 similarity transforms (a non-symmetric Jacobi
//      sweep on the pairs that matter, then a first-order correction of the rest).
//   4. The substeps of one sample are composed: one complex pole a sample per mode. The sides'
//      and the directional parts' low-passes are partial fractions: a part in each mode and a few
//      states of their own, driven by the feet.
//   5. The modes that add nothing above -70 dB to any output, for either foot, are dropped.
// The body is then about 200 modes, run in float32 16 at a time (AVX-512).
#pragma once

#include <memory>
#include <vector>

namespace pajupilli {

struct BodyData;

// What a body's modes are made from: its design and everything set on it.
struct BodyParams {
  const BodyData* data;
  double fs;
  float rate, stiff, air;  // BassBody::setDamping
  float plateK, airKs;     // BassBody::setVariation
  float postBehind;        // the post's place (the design's, or PJ_POST_BEHIND)
  bool sides;              // PJ_AIR_SIDES
  bool operator<(const BodyParams& o) const;
};

struct BodyModes {
  typedef float v16 __attribute__((vector_size(64)));
  static constexpr int kOut = 10;  // the radiated sound, f-hole, sides, dipole 0..2, bass and treble foot w, v
  static constexpr int kGlobal = 5;  // the low-passes' own states
  // 16 modes: the pole a sample (re, im), the feet's shares (bass, treble; re, im), each output's
  // share (re, im; heard: re(share x state)).
  struct alignas(64) Group {
    v16 lr, li, b0r, b0i, b1r, b1i;
    v16 wr[kOut], wi[kOut];
  };
  std::vector<Group> groups;
  int modes = 0, total = 0;  // (kept; all of them)
  float d[kOut][2] = {};               // the feet's forces straight to each output
  float e[kOut][kGlobal] = {};         // the low-passes' own states in each output
  float ps[kGlobal][kGlobal] = {};     // ... their step a sample
  float qs[kGlobal][2] = {};           // ... driven by the feet
};

// The modes for these parameters (built once per process for each, shared; thread-safe).
std::shared_ptr<const BodyModes> bodyModes(const BodyParams& p);

}  // namespace pajupilli
