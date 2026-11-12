#include "room.h"

#include <algorithm>
#include <cmath>

namespace pajupilli {

namespace {

constexpr RoomSpec kRooms[] = {
    {"dry", 0, 0, 2},
    {"studio", 150, 0.5, 2},      // a small studio, damped
    {"hall", 2500, 1.8, 3},       // a chamber-music hall
    {"church", 15000, 4.0, 6},    // a stone church
};
constexpr double kC = 343;

bool isPrime(int n) {
  if (n < 2) return false;
  for (int d = 2; d * d <= n; ++d)
    if (n % d == 0) return false;
  return true;
}

// Sylvester Hadamard sign, entry (r, c) of the 16 x 16 matrix.
float had(int r, int c) { return __builtin_popcount(unsigned(r & c)) & 1 ? -1.0f : 1.0f; }

}  // namespace

const RoomSpec* findRoom(const std::string& name) {
  for (const RoomSpec& r : kRooms)
    if (name == r.name) return &r;
  return nullptr;
}

Room::Room(const RoomSpec& spec, const std::vector<double>& x, double fs) : spec_(spec), x_(x), fs_(fs) {
  const int ns = int(x.size());
  // A shoebox 2 : 1.4 : 0.7, its surface and mean free path.
  const double v = std::max(spec.volume, 1.0), a = std::cbrt(v / (2 * 1.4 * 0.7));
  const double lx = 2 * a, ly = 1.4 * a, lz = 0.7 * a;
  const double surface = 2 * (lx * ly + lx * lz + ly * lz);
  const double mfp = 4 * v / surface;
  const double rt = std::max(spec.rt60, 0.05), rtHigh = rt / 3;  // walls and air take the highs
  double g2 = 0;
  for (int k = 0; k < kLines; ++k) {
    // Spread 0.5 .. 1.6 mean free paths, prime lengths (no common echoes).
    int n = int(mfp / kC * fs * (0.5 + 1.1 * k / (kLines - 1)));
    n = std::max(n, 2 * kBlock + 1);
    while (!isPrime(n)) ++n;
    n = std::min(n, kLineRing - 1);
    len_[k] = n;
    const double gl = std::pow(10.0, -3.0 * n / (fs * rt));  // per pass, low
    const double gh = std::pow(10.0, -3.0 * n / (fs * rtHigh));
    // One pole: DC gain gl, Nyquist gain gh.
    a1_[k] = float((gl - gh) / (gl + gh));
    b0_[k] = float(gl * (1 - a1_[k]));
    g2 += gl * gl;
  }
  g2 /= kLines;
  line_.assign(kLines * kLineRing, 0.0f);
  // Diffuse field: p_rev^2 = 16 pi / A p(1 m)^2. A tap sums the lines with
  // signs / 4 (unit gain over the orthogonal network), each line's energy is
  // b^2 / (1 - g^2) per unit input.
  const double absorption = 0.161 * v / rt;
  inGain_ = spec.volume > 0 ? float(std::sqrt(16 * M_PI / absorption * (1 - g2))) : 0.0f;
  inSign_.resize(ns * kLines);
  for (int s = 0; s < ns; ++s)
    for (int k = 0; k < kLines; ++k) inSign_[s * kLines + k] = had((s * 5 + 3) & 15, k) * 0.25f;
  outSign_.resize((ns + 2) * kLines);
  for (int t = 0; t < ns + 2; ++t)
    for (int k = 0; k < kLines; ++k)
      // Rows unlike the input rows; a fixed sign pattern decorrelates further.
      outSign_[t * kLines + k] = had((t * 7 + 1) & 15, k) * (((k * 37 + t * 11) % 7) < 3 ? -0.25f : 0.25f);
  // Direct sound: the listener's microphones 1 m either side of centre.
  ring_.assign(ns * kRing, 0.0f);
  dDelay_.assign((ns + 2) * ns, 0);
  dGain_.assign((ns + 2) * ns, 0.0f);
  for (int to = 0; to < ns + 2; ++to)
    for (int from = 0; from < ns; ++from) {
      if (to == from) continue;  // its own sound is in its own model
      double r;
      if (to < ns) r = std::max(0.6, std::fabs(x[to] - x[from]));
      else r = std::hypot(x[from] - (to == ns ? -1.0 : 1.0), spec.listener);
      // Players hear one block late: shorten their paths by a block.
      const int d = int(std::lround(r / kC * fs)) - (to < ns ? kBlock : 0);
      dDelay_[to * ns + from] = std::clamp(d, 0, kRing - kBlock - 1);
      dGain_[to * ns + from] = float(1 / r);
    }
}

void Room::process(const float* const* src, int n, float* const* atSource, float* left, float* right,
                   const float* listenGain) {
  const int ns = int(x_.size());
  const float lg = float(spec_.listener);  // the listener at the 1 m level
  for (int i = 0; i < n; ++i) {
    // The sources' sound now, for the direct paths.
    const int now = (rpos_ + i) & (kRing - 1);
    for (int s = 0; s < ns; ++s) ring_[s * kRing + now] = src[s][i];
    float x[kLines], y[kLines];
    for (int k = 0; k < kLines; ++k) {
      const float raw = line_[k * kLineRing + ((pos_ - len_[k]) & (kLineRing - 1))];
      lp_[k] = b0_[k] * raw + a1_[k] * lp_[k];
      x[k] = lp_[k];
    }
    for (int t = 0; t < ns + 2; ++t) {
      const float* sg = &outSign_[t * kLines];
      float acc = 0;
      for (int k = 0; k < kLines; ++k) acc += sg[k] * x[k];
      // The others' direct sound.
      for (int s = 0; s < ns; ++s) {
        const float g = dGain_[t * ns + s];
        if (g == 0) continue;
        const float d = ring_[s * kRing + ((now - dDelay_[t * ns + s]) & (kRing - 1))];
        acc += (t < ns ? 1.0f : listenGain[s]) * g * d;
      }
      if (t < ns) atSource[t][i] = acc;
      else if (t == ns) left[i] += lg * acc;
      else right[i] += lg * acc;
    }
    // Fast Hadamard transform, / 4: orthogonal.
    for (int k = 0; k < kLines; ++k) y[k] = x[k];
    for (int h = 1; h < kLines; h <<= 1)
      for (int k = 0; k < kLines; k += 2 * h)
        for (int j = k; j < k + h; ++j) {
          const float p = y[j], q = y[j + h];
          y[j] = p + q;
          y[j + h] = p - q;
        }
    for (int s = 0; s < ns; ++s) {
      const float in = inGain_ * src[s][i];
      const float* sg = &inSign_[s * kLines];
      for (int k = 0; k < kLines; ++k) y[k] += sg[k] * in;
    }
    for (int k = 0; k < kLines; ++k) line_[k * kLineRing + (pos_ & (kLineRing - 1))] = 0.25f * y[k];
    pos_ = (pos_ + 1) & (kLineRing - 1);
  }
  rpos_ = (rpos_ + n) & (kRing - 1);
}

}  // namespace pajupilli
