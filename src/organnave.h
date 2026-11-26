// The organ's own nave: the late reverberation of a large stone church
// (research/church_organ.md §7b.2, research/organ_room_coupling.md §3.6), for
// the organ alone. The early sound (each pipe's direct path and its images to
// second order) is src/organroom.h's; this is what follows it: the diffuse
// field, fed by the organ's sound at 1 m, heard by the two microphones 15 m
// into the nave and acting back on the pipes in their case.
//
// - A feedback delay network of 32 lines (a fast Hadamard mix), the lines 0.3
//   to 1.3 mean free paths long (4V/S: 16.5 m, 48 ms in a 50 x 18.3 x 21.5 m nave),
//   prime lengths. Each line loses per pass what the nave loses in its length,
//   per frequency: a broadband gain and seven sections (low shelves at 70 and
//   400 Hz, a peak at 140 Hz, high shelves at 1.5, 3.5, 7 and 13 kHz) fitted to the T60 per octave by least
//   squares over third octaves.
// - The T60 targets (s; 63 Hz .. 8 kHz, 16 kHz the air's cap):
//       63    125   250   500   1k    2k    4k    8k    16k
//       7.0   8.0   6.5   5.8   5.3   4.2   3.0   1.6   0.5
//   a large, empty stone nave of ~20 000 m^3: long in the bass, falling
//   steeply above 2 kHz, where the air over the long paths and the
//   congregation's and pews' absorption take the highs. From published
//   measurements, not from recordings:
//     * St Paul's Cathedral, London (152 000 m^3, empty): "as much as 12 s at
//       low frequencies", ~11 s at 500 Hz, ~7 s at 2 kHz, falling with air
//       absorption above (Brüel & Kjær, Application Note BO 0116, "RASTI
//       measurements in St. Paul's Cathedral, London").
//     * The Papal Basilicas of Rome, T30 per octave by ISO 3382-1: St Paul
//       outside the Walls 7.1 s at mid frequencies, the others longer, all
//       rising towards 125 Hz (F. Martellotta, "Understanding the acoustics
//       of Papal Basilicas in Rome by means of a coupled-volumes approach",
//       J. Sound Vib. 382, 413-427, 2016).
//     * Wells Cathedral (23 000 m^3) 5.1 s at mid frequencies (P. Mapp, "An
//       acoustic and audio survey of some English churches", Proc. IOA 2019).
//     * Notre-Dame de Paris (~80 000 m^3) ~6 s at mid frequencies before the
//       fire, empty (B. F. G. Katz, A. Weber, "An acoustic survey of the
//       Cathedral of Notre-Dame de Paris before and after the fire of 2019",
//       Acoustics 2, 791-802, 2020).
//     * The rise of T60 towards 125 Hz in stone churches (the walls' and
//       vaults' low absorption, the pews' and people's absorption above 250 Hz)
//       and T30 against volume: E. Cirillo, F. Martellotta, "Worship,
//       Acoustics, and Architecture" (Multi-Science, 2006); J. Meyer,
//       "Kirchenakustik" (Bochinsky, 2003): 20 000 m^3 of stone, empty, 5-8 s
//       at mid frequencies.
//   The top: ISO 9613-1's air at 20 C / 58 % RH (organroom.h's airDbPerM)
//   alone caps T60 at ~6.6 s at 4 kHz, ~1.9 s at 8 kHz and ~0.5 s at 16 kHz
//   in any room (Sabine's air term, research/church_organ.md §7d.4).
// - Its level by diffuse-field theory: p_rev^2 = 16 pi / A p(1 m)^2, A = 0.161
//   V / T60 per band. The input's gain is the same for every band (the
//   field's energy and its loss per pass both scale with T60).
// - The microphones (pattern a + (1 - a) cos, a = 0.6): the diffuse field's
//   energy at each a^2 + (1 - a)^2 / 3. Two microphones d = 2 m apart in a
//   diffuse field have the coherence sinc(kd) (Cook et al., JASA 27, 1955):
//   1 below ~50 Hz, ~0 above ~100 Hz. Each channel is a low band shared by
//   both (one tap of the network) plus a high band of its own (two more,
//   orthogonal), split by a power-complementary 2nd-order Butterworth pair at
//   kd = 1.9 (where sinc(kd) = 0.5: 52 Hz at 2 m).
// - The onset: the organ's sound enters after a predelay and three allpass
//   diffusers; the lines' first outputs arrive from `onsetMs` (20 ms) after
//   the direct sound, one after another to ~70 ms: the late field fades in
//   under the early images (§7d.4's ramp: a third of it at 20-30 ms, all of
//   it from 60-70 ms).
// - The pressure at the organ: one more tap, orthogonal to the microphones',
//   no pattern (Pa): what every pipe's enclosure hears of the nave.
// - The lens (lensDepth > 0, off by default): warm air layered under the
//   vault, moving slowly, bends and delays the paths a little: each line's
//   length wanders by up to lensDepth samples over lensSecs seconds (read
//   by cubic interpolation).
#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <vector>

#include "vmath.h"

namespace pajupilli {

class OrganNave {
 public:
  static constexpr int kLines = 32;
  static constexpr int kBands = 9;  // 63 Hz .. 16 kHz
  struct Params {
    double volume = 19673;  // m^3 (50 x 18.3 x 21.5 m)
    double surface = 4767;  // m^2
    double t60[kBands] = {7.0, 8.0, 6.5, 5.8, 5.3, 4.2, 3.0, 1.6, 0.5};
    double onsetMs = 20;     // the first late energy after the direct sound
    double latency = 0;      // samples the input already arrives late
    double pattern = 0.6;    // the microphones: a + (1 - a) cos
    double spacing = 2;      // m between them
    double lensDepth = 0;    // samples (0: off)
    double lensSecs = 4;     // s, the lens's wandering
    uint32_t seed = 0x5EED7A7Eu;  // (the directions' draw)
  };
  static double bandHz(int b) { return 62.5 * std::exp2(b); }

  void setup(double fs, const Params& p) {
    fs_ = fs;
    p_ = p;
    const double c = 343.2, mfp = 4 * p.volume / p.surface;
    // The lines: 0.3 .. 1.3 mean free paths, spread evenly (their first
    // outputs evenly in time: the onset), each moved at random by up to 0.45
    // of the spacing (evenly spaced lengths make many paths through two
    // lines equally long: the taps correlate), primes.
    int maxLen = 0;
    uint32_t js = 0x1E9971u;
    for (int k = 0; k < kLines; ++k) {
      const double u = (k + (k == 0 ? 0.0 : 0.45 * rand1(js))) / double(kLines - 1);
      int n = int(mfp / c * fs * (0.3 + 1.0 * u));
      while (!isPrime(n)) ++n;
      len_[k] = n;
      maxLen = std::max(maxLen, n);
    }
    const int extra = int(std::ceil(p.lensDepth * 2)) + 4;
    ring_ = 1;
    while (ring_ < maxLen + extra + 1) ring_ <<= 1;
    line_.assign(size_t(kLines) * ring_, 0.0f);
    // The curve the lines are fitted to: an octave band's T30 (Schroeder,
    // -5 .. -35 dB) is held up by the slower end of the band where T60 falls
    // steeply (8 kHz: 5.6 kHz's 2.2 s against 11 kHz's 0.9): the design
    // values are set so that each band, measured so, meets its target.
    std::copy(p.t60, p.t60 + kBands, design_);
    for (int it = 0; it < 12; ++it)
      for (int b = 0; b < kBands; ++b) design_[b] *= fastPow(p.t60[b] / bandT30(b), 0.7);
    // Each line's loss per pass, fitted.
    for (int k = 0; k < kLines; ++k) fitLine(k);
    // The input's gain: the diffuse field's level (at 1 kHz; the same in
    // every band, see above), the energy spread evenly over the lines.
    const double tRef = p.t60[4], A = 0.161 * p.volume / tRef;
    double loss = 0;
    for (int k = 0; k < kLines; ++k) {
      const double g = fastPow(10.0, lineDb(k, 1000) / 20);
      loss += 1 - g * g;
    }
    loss /= kLines;
    inGain_ = float(std::sqrt(kLines * 16 * M_PI / A * loss));
    micDiffuse_ = float(std::sqrt(p.pattern * p.pattern + (1 - p.pattern) * (1 - p.pattern) / 3));
    // Directions: the input and four taps (the low band, the left's and the
    // right's highs, the organ), all of equal magnitude on their lines with
    // signs from rows of a Hadamard matrix scrambled by a random sign per line
    // (orthogonal, and not along the mixing matrix's rows, which would fold
    // back into one line). Equal magnitudes: every line's first output
    // carries the same energy (random weights would make the onset lumpy:
    // the products of two random weights are heavy-tailed). The left's and
    // the right's highs on disjoint lines, even and odd: the lines' powers
    // differ a little, and two taps over the same lines then still correlate
    // by sum(a_k b_k P_k).
    uint32_t seed = p.seed;
    double sgn[kLines];
    for (int k = 0; k < kLines; ++k) sgn[k] = rand1(seed) < 0 ? -1 : 1;
    auto row = [&](float* v, int q, int parity) {
      const int m = parity < 0 ? kLines : kLines / 2;
      for (int k = 0; k < kLines; ++k)
        v[k] = parity >= 0 && k % 2 != parity ? 0.0f
                                              : float(sgn[k] * (__builtin_popcount(q & k) % 2 ? -1 : 1) / std::sqrt(double(m)));
    };
    row(in_, 5, -1);
    row(tap_[0], 9, -1);
    row(tap_[1], 12, 0);
    row(tap_[2], 19, 1);
    row(tap_[3], 22, -1);
    // The coherence split: kd = 1.9.
    const double fc = 1.9 * c / (2 * M_PI * std::max(p.spacing, 0.1));
    lpBq_ = Bq::pass(false, fc, fs);
    hpBq_ = Bq::pass(true, fc, fs);
    lpU_ = lpBq_;
    hpV_ = hpW_ = hpBq_;
    // The predelay: the first line's output at onsetMs after the direct sound.
    pre_ = std::max(1, int(std::lround(p.onsetMs * 1e-3 * fs - p.latency - len_[0])));
    preRing_.assign(size_t(pre_) + 1, 0.0f);
    // The diffusers: 1.1, 2.3, 3.7 ms, 0.6.
    const double dm[3] = {1.1, 2.3, 3.7};
    for (int j = 0; j < 3; ++j) {
      int n = int(dm[j] * 1e-3 * fs);
      while (!isPrime(n)) ++n;
      ap_[j].assign(n, 0.0f);
      apPos_[j] = 0;
    }
    // The lens.
    lens_ = p.lensDepth > 0;
    for (int k = 0; k < kLines; ++k) {
      lensA_[k] = float(rand1(seed));
      lensB_[k] = float(rand1(seed));
      lensPh_[k] = float(0.5 * (1 + rand1(seed)));
      lensRate_[k] = float(1.0 / (std::max(p.lensSecs, 0.1) * fs * (0.7 + 0.3 * (1 + rand1(seed)))));
    }
    lensSeed_ = seed;
    reset();
  }

  void reset() {
    std::fill(line_.begin(), line_.end(), 0.0f);
    std::fill(preRing_.begin(), preRing_.end(), 0.0f);
    for (auto& a : ap_) std::fill(a.begin(), a.end(), 0.0f);
    for (Zone& Z : zones_) {
      std::fill(Z.pre.begin(), Z.pre.end(), 0.0f);
      for (auto& a : Z.ap) std::fill(a.begin(), a.end(), 0.0f);
      Z.pos = Z.apPos[0] = Z.apPos[1] = Z.apPos[2] = Z.left = 0;
    }
    for (int k = 0; k < kLines; ++k)
      for (Bq& q : eq_[k]) q.clear();
    for (Bq* q : {&lpU_, &hpV_, &hpW_}) q->clear();
    pos_ = prePos_ = 0;
  }

  // n samples: in, the organ's sound at 1 m (Pa); out (added to): the
  // microphones' l and r (Pa), the pressure at the organ (Pa; null: none).
  // Inputs by place (the puffs, enclosure by enclosure): each enters the lines by its own weights
  // through its own predelay and diffusers. What is given here is also in process()'s `in`, so
  // the common weights are taken off it: the same energy, in its own place in the church.
  void setZones(int n) {
    zones_.assign(n, Zone{});
    uint32_t seed = 0x51ed27u;
    for (int z = 0; z < n; ++z) {
      Zone& Z = zones_[z];
      double nn = 0;
      for (int k = 0; k < kLines; ++k) {
        seed = seed * 1664525u + 1013904223u;
        const double v = (seed >> 8) / 16777216.0 < 0.5 ? -1 : 1;
        Z.v[k] = float(v);
        nn += v * v;
      }
      for (int k = 0; k < kLines; ++k) Z.v[k] = float(Z.v[k] / std::sqrt(nn)) - in_[k];
      Z.pre.assign(preRing_.size(), 0.0f);
      for (int j = 0; j < 3; ++j) Z.ap[j].assign(ap_[j].size(), 0.0f);
    }
  }
  // A line's length moved by a static offset (samples; an air event: a draft,
  // a warm plume in its part of the church), read fractionally while any is set.
  void setStatic(int k, double samples) {
    if (k < 0 || k >= kLines) return;
    static_[k] = samples;
    anyStatic_ = false;
    for (int j = 0; j < kLines; ++j) anyStatic_ |= std::fabs(static_[j]) > 1e-4;
  }
  static constexpr int lines() { return kLines; }
  void process(const float* in, int n, float* l, float* r, float* atOrgan, const float* const* zoneIn = nullptr) {
    const int mask = ring_ - 1;
    for (int i = 0; i < n; ++i) {
      // The input: predelayed, diffused.
      float d = preRing_[prePos_];
      preRing_[prePos_] = inGain_ * in[i];
      prePos_ = prePos_ + 1 == int(preRing_.size()) ? 0 : prePos_ + 1;
      for (int j = 0; j < 3; ++j) {
        std::vector<float>& a = ap_[j];
        const float dl = a[apPos_[j]];
        const float w = d + 0.6f * dl;
        a[apPos_[j]] = w;
        d = dl - 0.6f * w;
        apPos_[j] = apPos_[j] + 1 == int(a.size()) ? 0 : apPos_[j] + 1;
      }
      // The lines' outputs, each through its loss.
      double x[kLines];
      if (!lens_ && !anyStatic_) {
        for (int k = 0; k < kLines; ++k) x[k] = line_[size_t(k) * ring_ + ((pos_ - len_[k]) & mask)];
      } else {
        for (int k = 0; k < kLines; ++k) x[k] = lensRead(k, mask);
      }
      for (int k = 0; k < kLines; ++k)
        for (Bq& q : eq_[k]) x[k] = q.run(x[k]);
      double t[kTaps] = {};
      for (int q = 0; q < kTaps; ++q)
        for (int k = 0; k < kLines; ++k) t[q] += tap_[q][k] * x[k];
      const double lo = lpU_.run(t[0]);
      l[i] += micDiffuse_ * float(lo + hpV_.run(t[1]));
      r[i] += micDiffuse_ * float(lo + hpW_.run(t[2]));
      if (atOrgan) atOrgan[i] += float(t[3]);
      // Mix: a fast Hadamard transform, / sqrt(32) (orthogonal); the input in.
      for (int h = 1; h < kLines; h <<= 1)
        for (int k = 0; k < kLines; k += 2 * h)
          for (int j = k; j < k + h; ++j) {
            const double a = x[j], b = x[j + h];
            x[j] = a + b;
            x[j + h] = a - b;
          }
      const double s = 1 / std::sqrt(double(kLines));
      for (int k = 0; k < kLines; ++k) line_[size_t(k) * ring_ + (pos_ & mask)] = float(s * x[k] + in_[k] * d);
      if (zoneIn)
        for (size_t z = 0; z < zones_.size(); ++z) {
          Zone& Z = zones_[z];
          const float u = zoneIn[z] ? zoneIn[z][i] : 0.0f;
          if (u != 0) Z.left = int(Z.pre.size() + 12 * (Z.ap[0].size() + Z.ap[1].size() + Z.ap[2].size()));
          if (Z.left <= 0) continue;  // (nothing in it: its weights not run)
          --Z.left;
          float e = Z.pre[Z.pos];
          Z.pre[Z.pos] = inGain_ * u;
          Z.pos = Z.pos + 1 == int(Z.pre.size()) ? 0 : Z.pos + 1;
          for (int j = 0; j < 3; ++j) {
            std::vector<float>& a = Z.ap[j];
            const float dl = a[Z.apPos[j]];
            const float w = e + 0.6f * dl;
            a[Z.apPos[j]] = w;
            e = dl - 0.6f * w;
            Z.apPos[j] = Z.apPos[j] + 1 == int(a.size()) ? 0 : Z.apPos[j] + 1;
          }
          float* out = &line_[pos_ & mask];
          for (int k = 0; k < kLines; ++k) out[size_t(k) * ring_] += Z.v[k] * e;
        }
      pos_ = (pos_ + 1) & mask;
    }
  }

  // The fitted decay (s) of line k at f, and its target.
  double t60Of(int k, double f) const { return -60.0 * len_[k] / (fs_ * lineDb(k, f)); }
  double target(double f) const { return interp(p_.t60, f); }
  double designT(double f) const { return interp(design_, f); }
  double diffuseDb(double f) const { return 10 * std::log10(16 * M_PI / (0.161 * p_.volume / target(f))); }
  int length(int k) const { return len_[k]; }
  int predelay() const { return pre_; }

 private:
  static constexpr int kTaps = 4;  // the low band, the left's highs, the right's, the organ
  struct Bq {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double run(double x) {  // (transposed direct form II)
      const double y = b0 * x + z1;
      z1 = b1 * x - a1 * y + z2;
      z2 = b2 * x - a2 * y;
      return y;
    }
    void clear() { z1 = z2 = 0; }
    double magDb(double f, double fs) const {
      const std::complex<double> z1c = std::polar(1.0, -2 * M_PI * f / fs), z2c = z1c * z1c;
      return 20 * std::log10(std::abs((b0 + b1 * z1c + b2 * z2c) / (1.0 + a1 * z1c + a2 * z2c)));
    }
    static Bq norm(double b0, double b1, double b2, double a0, double a1, double a2) {
      Bq q;
      q.b0 = b0 / a0, q.b1 = b1 / a0, q.b2 = b2 / a0, q.a1 = a1 / a0, q.a2 = a2 / a0;
      return q;
    }
    static Bq shelf(bool high, double f0, double gDb, double fs) {  // (RBJ, S = 1)
      const double A = fastPow(10.0, gDb / 40), w = 2 * M_PI * f0 / fs, cw = std::cos(w), sw = std::sin(w);
      const double sA = 2 * std::sqrt(A) * sw / 2 * std::sqrt(2.0);
      if (!high)
        return norm(A * ((A + 1) - (A - 1) * cw + sA), 2 * A * ((A - 1) - (A + 1) * cw), A * ((A + 1) - (A - 1) * cw - sA),
                    (A + 1) + (A - 1) * cw + sA, -2 * ((A - 1) + (A + 1) * cw), (A + 1) + (A - 1) * cw - sA);
      return norm(A * ((A + 1) + (A - 1) * cw + sA), -2 * A * ((A - 1) + (A + 1) * cw), A * ((A + 1) + (A - 1) * cw - sA),
                  (A + 1) - (A - 1) * cw + sA, 2 * ((A - 1) - (A + 1) * cw), (A + 1) - (A - 1) * cw - sA);
    }
    static Bq peak(double f0, double Q, double gDb, double fs) {  // (RBJ)
      const double A = fastPow(10.0, gDb / 40), w = 2 * M_PI * f0 / fs, cw = std::cos(w), al = std::sin(w) / (2 * Q);
      return norm(1 + al * A, -2 * cw, 1 - al * A, 1 + al / A, -2 * cw, 1 - al / A);
    }
    static Bq pass(bool high, double f0, double fs) {  // (Butterworth, Q 0.707)
      const double w = 2 * M_PI * f0 / fs, cw = std::cos(w), al = std::sin(w) / std::sqrt(2.0);
      if (!high) return norm((1 - cw) / 2, 1 - cw, (1 - cw) / 2, 1 + al, -2 * cw, 1 - al);
      return norm((1 + cw) / 2, -(1 + cw), (1 + cw) / 2, 1 + al, -2 * cw, 1 - al);
    }
  };
  // The sections: 0 a low shelf, 1 a peak (Q 0.8), 2 a high shelf.
  static constexpr int kShelves = 7;
  static constexpr double kShelfHz[kShelves] = {70, 140, 400, 1500, 3500, 7000, 13000};
  static constexpr int kShelfKind[kShelves] = {0, 1, 0, 2, 2, 2, 2};
  static Bq section(int j, double gDb, double fs) {
    return kShelfKind[j] == 1 ? Bq::peak(kShelfHz[j], 0.8, gDb, fs) : Bq::shelf(kShelfKind[j] == 2, kShelfHz[j], gDb, fs);
  }

  static bool isPrime(int n) {
    if (n < 2) return false;
    for (int d = 2; d * d <= n; ++d)
      if (n % d == 0) return false;
    return true;
  }
  static double rand1(uint32_t& s) {  // -1 .. 1
    s = s * 1664525u + 1013904223u;
    return (s >> 8) / double(1 << 23) - 1;
  }
  static double interp(const double* t, double f) {  // (log T over log f)
    const double b = std::log2(std::max(f, 1.0) / 62.5);
    if (b <= 0) return t[0];
    if (b >= kBands - 1) return t[kBands - 1];
    const int i = int(b);
    const double u = b - i;
    return std::exp((1 - u) * std::log(t[i]) + u * std::log(t[i + 1]));
  }
  // Band b's T30 as measured (white noise in, its energy decay summed over
  // the octave, -5 .. -35 dB) when each frequency decays by designT.
  double bandT30(int b) const {
    const double f0 = bandHz(b), lo = f0 / std::sqrt(2.0), hi = std::min(f0 * std::sqrt(2.0), 0.49 * fs_);
    double w[64], T[64], sum = 0;
    for (int i = 0; i < 64; ++i) {
      const double f = lo * fastPow(hi / lo, (i + 0.5) / 64);
      w[i] = f;  // (equal energy per Hz, log-spaced points)
      T[i] = designT(f);
      sum += w[i];
    }
    auto when = [&](double db) {
      double a = 0, z = 100;
      for (int it = 0; it < 60; ++it) {
        const double t = 0.5 * (a + z);
        double e = 0;
        for (int i = 0; i < 64; ++i) e += w[i] * std::exp(-13.8155 * t / T[i]);
        (10 * std::log10(e / sum) > db ? a : z) = t;
      }
      return 0.5 * (a + z);
    };
    return 60 * (when(-35) - when(-5)) / 30;
  }
  double lineDb(int k, double f) const {
    double d = 20 * std::log10(gain_[k]);
    for (const Bq& q : eq_[k]) d += q.magDb(f, fs_);
    return d;
  }
  // Line k's loss per pass: the broadband gain and the shelves' gains (dB),
  // Gauss-Newton on the target's dB per pass at third octaves 40 Hz .. 20 kHz
  // (above 16 kHz weighted down); then never above 0 dB anywhere.
  void fitLine(int k) {
    std::vector<double> f, want, wt;
    for (double fr = 40; fr < 0.49 * fs_ && fr <= 20000; fr *= std::exp2(1.0 / 3)) {
      f.push_back(fr);
      want.push_back(-60.0 * len_[k] / (fs_ * designT(fr)));
      wt.push_back(fr > 16500 ? 0.3 : 1.0);
    }
    const int np = 1 + kShelves, nf = int(f.size());
    double x[np] = {};
    x[0] = want[f.size() / 2];
    auto resp = [&](const double* p, std::vector<double>& out) {
      out.assign(nf, p[0]);
      for (int j = 0; j < kShelves; ++j) {
        const Bq q = section(j, p[1 + j], fs_);
        for (int i = 0; i < nf; ++i) out[i] += q.magDb(f[i], fs_);
      }
    };
    std::vector<double> r0, r1;
    for (int it = 0; it < 40; ++it) {
      resp(x, r0);
      double J[64][np];
      for (int j = 0; j < np; ++j) {
        double xp[np];
        std::copy(x, x + np, xp);
        xp[j] += 0.01;
        resp(xp, r1);
        for (int i = 0; i < nf; ++i) J[i][j] = (r1[i] - r0[i]) / 0.01;
      }
      double M[np][np + 1] = {};
      for (int a = 0; a < np; ++a) {
        for (int b = 0; b < np; ++b) {
          for (int i = 0; i < nf; ++i) M[a][b] += wt[i] * J[i][a] * J[i][b];
          if (a == b) M[a][b] += 1e-6;
        }
        for (int i = 0; i < nf; ++i) M[a][np] -= wt[i] * J[i][a] * (r0[i] - want[i]);
      }
      for (int a = 0; a < np; ++a) {  // (Gauss-Jordan, partial pivot)
        int piv = a;
        for (int b = a + 1; b < np; ++b)
          if (std::fabs(M[b][a]) > std::fabs(M[piv][a])) piv = b;
        for (int c = 0; c <= np; ++c) std::swap(M[a][c], M[piv][c]);
        for (int b = 0; b < np; ++b) {
          if (b == a) continue;
          const double m = M[b][a] / M[a][a];
          for (int c = a; c <= np; ++c) M[b][c] -= m * M[a][c];
        }
      }
      for (int a = 0; a < np; ++a) x[a] += std::clamp(M[a][np] / M[a][a], -6.0, 6.0);
    }
    gain_[k] = fastPow(10.0, x[0] / 20);
    eq_[k].clear();
    for (int j = 0; j < kShelves; ++j) eq_[k].push_back(section(j, x[1 + j], fs_));
    double top = -1e9;  // (stable: under unity everywhere)
    for (double fr = 10; fr < 0.5 * fs_; fr *= 1.02) top = std::max(top, lineDb(k, fr));
    if (top > -0.001) gain_[k] *= fastPow(10.0, (-0.001 - top) / 20);
    // The broadband gain folded into the first section.
    eq_[k][0].b0 *= gain_[k];
    eq_[k][0].b1 *= gain_[k];
    eq_[k][0].b2 *= gain_[k];
    gain_[k] = 1;
  }
  // The lens: line k's length wandering between random points (raised
  // cosine), read by 4-point Lagrange interpolation.
  double lensRead(int k, int mask) {
    lensPh_[k] += lensRate_[k];
    if (lensPh_[k] >= 1) {
      lensPh_[k] -= 1;
      lensA_[k] = lensB_[k];
      lensB_[k] = float(rand1(lensSeed_));
    }
    const double u = 0.5 - 0.5 * std::cos(M_PI * lensPh_[k]);
    const double v = lensA_[k] + u * (lensB_[k] - lensA_[k]);
    const double dly = std::max(2.0, len_[k] + p_.lensDepth * (1 + v) + static_[k]);
    const int di = int(dly);
    const double fr = dly - di;
    const float* L = &line_[size_t(k) * ring_];
    const double xm1 = L[(pos_ - di + 1) & mask], x0 = L[(pos_ - di) & mask], x1 = L[(pos_ - di - 1) & mask],
                 x2 = L[(pos_ - di - 2) & mask];
    const double c0 = -fr * (fr - 1) * (fr - 2) / 6, c1 = (fr + 1) * (fr - 1) * (fr - 2) / 2,
                 c2 = -(fr + 1) * fr * (fr - 2) / 2, c3 = (fr + 1) * fr * (fr - 1) / 6;
    return c0 * xm1 + c1 * x0 + c2 * x1 + c3 * x2;
  }

  double fs_ = 48000;
  Params p_;
  double design_[kBands] = {};
  int len_[kLines] = {}, ring_ = 1, pos_ = 0;
  std::vector<float> line_;
  double gain_[kLines] = {};
  std::vector<Bq> eq_[kLines];
  float inGain_ = 0, micDiffuse_ = 1;
  float in_[kLines] = {}, tap_[kTaps][kLines] = {};
  Bq lpBq_, hpBq_, lpU_, hpV_, hpW_;  // (the split's design; its three filters)
  int pre_ = 1, prePos_ = 0;
  std::vector<float> preRing_;
  std::vector<float> ap_[3];
  int apPos_[3] = {};
  struct Zone {
    float v[kLines] = {};
    std::vector<float> pre, ap[3];
    int pos = 0, apPos[3] = {}, left = 0;
  };
  std::vector<Zone> zones_;
  bool lens_ = false, anyStatic_ = false;
  double static_[kLines] = {};
  float lensA_[kLines] = {}, lensB_[kLines] = {}, lensPh_[kLines] = {}, lensRate_[kLines] = {};
  uint32_t lensSeed_ = 1;
};

}  // namespace pajupilli
