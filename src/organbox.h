// A pipe division's enclosure (research/church_organ.md §7a): a swell box
// (wooden, ~3 x 2.4 x 1.6 m, louvred shutters on its front) or an open case
// (the Positiv's), as a small room of its own.
//
//   - Its field: four delay lines (the round trips across its width, height
//     and depth, and an oblique one), mixed by a Householder matrix, each
//     losing its highs (Jot's one-pole: the low and high T60). Open shutters
//     let energy out: the box rings shorter.
//   - Each pipe's mouth at its place inside: it drives the lines as the box's
//     modes see it (an axial mode's shape, cos(pi x / L), at the mouth: a
//     mouth on a node does not excite that mode) and, by reciprocity, hears
//     the field there the same way (the jet deflected by it).
//   - Out through the opening: each pipe's direct sound (delayed by its
//     distance to the front) and the field, through the shutters (closed:
//     quieter and dark, the highs ~20 dB down; open: all of it); the walls
//     leak a little bass always. The church's pressure comes back in through
//     the opening.
#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <vector>

namespace pajupilli {

struct Vec3 {
  double x = 0, y = 0, z = 0;  // m: x across (left -), y up, z towards the church (+)
};

class Enclosure {
 public:
  struct Params {
    Vec3 dims{3.0, 2.4, 1.6};
    double t60Low = 0.6, t60High = 0.12;  // closed (s)
    double openT60 = 0.3;                 // the low T60 with the shutters fully open
    bool shutters = true;                 // a swell box (else an open case: always open)
    double leak = 0.06;                   // through the walls (the bass)
    double field = 0.5;                   // the box's field against the direct sound at the opening
    double outDelay = 0;                  // s: farther from the listener than the nearest division
    double pan = 0;                       // -1 left .. 1 right
    double directLp = 0;                  // its direct sound's low-pass (Hz; 0: none): behind a case
  };

  void setup(double fs, const Params& p) {
    fs_ = fs;
    p_ = p;
    const double c = 343;
    const double len[4] = {2 * p.dims.x / c, 2 * p.dims.y / c, 2 * p.dims.z / c,
                           0.9 * (p.dims.x + p.dims.y + p.dims.z) / c};
    for (int k = 0; k < 4; ++k) {
      const int n = std::max(8, int(std::lround(len[k] * fs)));
      line_[k].assign(n, 0.0f);
      pos_[k] = 0;
      auto damp = [&](double t60, float& g, float& b) {
        const double gl = std::pow(10.0, -3.0 * n / (fs * t60));
        const double alpha = std::min(1.0, p.t60High / t60);
        g = float(gl);
        // (Jot's one-pole; a long line with a short high T60 asks for a pole
        // past 1: held at 0.9, its highs then a little longer than asked)
        b = float(std::clamp(std::log(10.0) / 4 * std::log10(gl) * (1 - 1 / (alpha * alpha)), 0.0, 0.9));
      };
      damp(p.t60Low, gC_[k], bC_[k]);
      damp(p.shutters ? p.openT60 : p.t60Low, gO_[k], bO_[k]);
    }
    int d = 1;
    while (d < int(0.02 * fs) + 4) d <<= 1;
    direct_.assign(d, 0.0f);
    dmask_ = d - 1;
    int od = 1;
    const int odn = int(std::lround(p.outDelay * fs));
    while (od < odn + 2) od <<= 1;
    out_.assign(od, 0.0f);
    omask_ = od - 1;
    outN_ = odn;
    open_ = target_ = p.shutters ? 0.75 : 1.0;
    dirA_ = p.directLp > 0 ? float(1 - std::exp(-2 * M_PI * p.directLp / fs)) : 1.0f;
    retune();
  }

  // A pipe's mouth at rel (from the box's centre): its index here.
  int addSource(Vec3 rel) {
    Src s;
    const double r[3] = {rel.x, rel.y, rel.z}, dim[3] = {p_.dims.x, p_.dims.y, p_.dims.z};
    double e = 0;
    for (int k = 0; k < 3; ++k) {
      s.g[k] = float(std::cos(M_PI * (std::clamp(r[k] / dim[k], -0.5, 0.5) + 0.5)));
      e += s.g[k] * s.g[k];
    }
    s.g[3] = 0.6f;  // (the oblique line: the diffuse part, everywhere)
    e += 0.36;
    for (float& g : s.g) g = float(g / std::sqrt(e));
    // The direct sound to the opening (the front face) and on down to the
    // listener in the nave, ~17 degrees below the gallery: a higher source
    // is farther (an open pipe's top ~0.3 x its length behind its mouth: the
    // two ends' even harmonics, in opposite phase, then do not cancel).
    const double dist = std::max(0.1, p_.dims.z / 2 - rel.z) + 0.15 + 0.3 * (rel.y + p_.dims.y / 2);
    s.delay = int(std::lround(dist / 343 * fs_));
    s.gain = float(std::min(1.0, 0.5 / dist));
    src_.push_back(s);
    return int(src_.size()) - 1;
  }

  void setOpening(double s) { target_ = p_.shutters ? std::clamp(s, 0.03, 1.0) : 1.0; }

  // The box's pressure at source i now (the lines' current outputs).
  // The field at source i over the last kHist samples, analysed at the given
  // frequencies (Hz): each one's complex amplitude, its phase as it is now
  // (a Hann-windowed DFT: what a resonator there tuned to it would have
  // gathered: sympathetic resonance, a pipe's head start).
  static constexpr int kHist = 2048;
  void fieldAt(int i, const double* hz, int n, std::complex<double>* out) const {
    const Src& s = src_[i];
    for (int k = 0; k < n; ++k) out[k] = 0;
    double wsum = 0;
    std::vector<std::complex<double>> rot(n), ph(n);
    for (int k = 0; k < n; ++k) {
      rot[k] = std::polar(1.0, 2 * M_PI * hz[k] / fs_);
      ph[k] = std::polar(1.0, -2 * M_PI * hz[k] / fs_ * (kHist - 1));  // (the oldest sample's phase against now)
    }
    for (int t = 0; t < kHist; ++t) {
      const int at = (histPos_ + t) & (kHist - 1);  // (oldest first)
      const double f = s.g[0] * hist_[0][at] + s.g[1] * hist_[1][at] + s.g[2] * hist_[2][at] + s.g[3] * hist_[3][at];
      const double w = 0.5 - 0.5 * std::cos(2 * M_PI * (t + 0.5) / kHist);
      wsum += w;
      for (int k = 0; k < n; ++k) {
        out[k] += w * f * std::conj(ph[k]);
        ph[k] *= rot[k];
      }
    }
    for (int k = 0; k < n; ++k) out[k] *= 2 / wsum;
  }
  float heard(int i) const {
    const Src& s = src_[i];
    return s.g[0] * o_[0] + s.g[1] * o_[1] + s.g[2] * o_[2] + s.g[3] * o_[3];
  }
  // Source i's sound this sample.
  // (field: the share of it into the box's field, the part the jets there
  // hear; its direct sound whole)
  void add(int i, float y, float field = 1) {
    const Src& s = src_[i];
    for (int k = 0; k < 4; ++k) in_[k] += s.g[k] * field * y;
    direct_[(dw_ + s.delay) & dmask_] += s.gain * y;
    energy_ = std::max(energy_, std::fabs(y));
  }
  // The sample's end: the lines advance; out: the sound leaving the opening
  // (delayed to the listener), the church's pressure coming in.
  float step(float church) {
    if (--ctl_ <= 0) retune();
    open_ += (target_ - open_) * 0.0004f;  // (the shutters' travel: ~50 ms)
    const float sum = 0.5f * (o_[0] + o_[1] + o_[2] + o_[3]);
    const float s = open_;
    float fieldOut = 0;
    for (int k = 0; k < 4; ++k) {
      const float g = gC_[k] + (gO_[k] - gC_[k]) * s, b = bC_[k] + (bO_[k] - bC_[k]) * s;
      lp_[k] = (1 - b) * (o_[k] - sum) + b * lp_[k];
      line_[k][pos_[k]] = g * lp_[k] + kIn * in_[k] + 0.25f * s * church;
      in_[k] = 0;
      pos_[k] = pos_[k] + 1 == int(line_[k].size()) ? 0 : pos_[k] + 1;
      o_[k] = line_[k][pos_[k]];
      hist_[k][histPos_] = o_[k];
      fieldOut += o_[k];
    }
    histPos_ = (histPos_ + 1) & (kHist - 1);
    float dir = direct_[dw_];
    if (dirA_ < 1) {
      dl1_ += dirA_ * (dir - dl1_);
      dl2_ += dirA_ * (dl1_ - dl2_);
      dir = dl2_;
    }
    const float x = dir + float(p_.field) * 0.5f * fieldOut;
    direct_[dw_] = 0;
    dw_ = (dw_ + 1) & dmask_;
    // The shutters: louder and brighter as they open; the walls' leak.
    l1_ += a_ * (x - l1_);
    l2_ += a_ * (l1_ - l2_);
    k1_ += 0.026f * (x - k1_);  // (~200 Hz)
    const float y = (p_.shutters ? gainOpen_ * l2_ : x) + float(p_.leak) * k1_;
    // To the listener.
    out_[(ow_ + outN_) & omask_] = y;
    const float r = out_[ow_];
    out_[ow_] = 0;
    ow_ = (ow_ + 1) & omask_;
    energy_ *= 0.9995f;
    energy_ = std::max(energy_, std::fabs(r));
    return r;
  }
  bool quiet() const { return energy_ < 1e-6f; }
  double pan() const { return p_.pan; }
  double opening() const { return open_; }
  void reset() {
    for (int k = 0; k < 4; ++k) {
      std::fill(line_[k].begin(), line_[k].end(), 0.0f);
      o_[k] = in_[k] = lp_[k] = 0;
    }
    std::fill(direct_.begin(), direct_.end(), 0.0f);
    for (auto& h : hist_) std::fill(std::begin(h), std::end(h), 0.0f);
    std::fill(out_.begin(), out_.end(), 0.0f);
    l1_ = l2_ = k1_ = energy_ = dl1_ = dl2_ = 0;
  }

 private:
  static constexpr float kIn = 0.35f;  // a mouth's flow into the box's lines
  struct Src {
    float g[4];
    int delay;
    float gain;
  };
  // The shutters' low-pass (two poles, 600 Hz closed .. 16 kHz open) and
  // gain (closed ~-12 dB), updated every 32 samples.
  void retune() {
    ctl_ = 32;
    const double s = open_;
    const double fc = 600 * std::pow(16000 / 600.0, s);
    a_ = float(1 - std::exp(-2 * M_PI * fc / fs_));
    gainOpen_ = float(0.25 + 0.75 * std::pow(s, 0.7));
  }
  double fs_ = 48000;
  Params p_;
  std::vector<float> line_[4];
  int pos_[4] = {};
  float o_[4] = {}, in_[4] = {}, lp_[4] = {};
  float gC_[4] = {}, bC_[4] = {}, gO_[4] = {}, bO_[4] = {};
  std::vector<Src> src_;
  std::vector<float> direct_, out_;
  int dmask_ = 0, dw_ = 0, omask_ = 0, ow_ = 0, outN_ = 0, ctl_ = 0;
  float hist_[4][kHist] = {};
  float dirA_ = 1, dl1_ = 0, dl2_ = 0;
  int histPos_ = 0;
  float open_ = 1, target_ = 1, a_ = 1, gainOpen_ = 1, l1_ = 0, l2_ = 0, k1_ = 0, energy_ = 0;
};

}  // namespace pajupilli
