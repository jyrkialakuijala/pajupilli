#include "room.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

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

}  // namespace

const RoomSpec* findRoom(const std::string& name) {
  for (const RoomSpec& r : kRooms)
    if (name == r.name) return &r;
  return nullptr;
}

Room::Room(const RoomSpec& spec, const std::vector<double>& x, double fs) : spec_(spec), x_(x), fs_(fs) {
  const int ns = int(x.size());
  // A shoebox 2 : 1.4 : 0.7 (length, width, height), its surface and mean free path.
  const double v = std::max(spec.volume, 1.0), a = std::cbrt(v / (2 * 1.4 * 0.7));
  const double len = 2 * a, wid = 1.4 * a, hei = 0.7 * a;
  const double surface = 2 * (len * wid + len * hei + wid * hei);
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
  // Diffuse field: p_rev^2 = 16 pi / A p(1 m)^2. An input of unit energy
  // circulates as 1 / (1 - g^2) over the lines; a unit tap sees 1/16 of it.
  const double absorption = 0.161 * v / rt;
  inGain_ = spec.volume > 0 ? float(4 * std::sqrt(16 * M_PI / absorption * (1 - g2))) : 0.0f;
  // Random unit directions over the lines (fixed seed): no source or listener
  // is aligned with the mixing matrix's own rows (a Hadamard row folds back
  // into a single line after one pass).
  unsigned seed = 12345;
  auto gauss = [&]() {
    double t = 0;
    for (int j = 0; j < 4; ++j) { seed = seed * 1664525u + 1013904223u; t += (seed >> 8) / double(1 << 24) - 0.5; }
    return t;
  };
  auto unitRows = [&](std::vector<float>& m, int rows) {
    m.resize(size_t(rows) * kLines);
    for (int r = 0; r < rows; ++r) {
      double nn = 0;
      for (int k = 0; k < kLines; ++k) { const double g = gauss(); m[r * kLines + k] = float(g); nn += g * g; }
      for (int k = 0; k < kLines; ++k) m[r * kLines + k] = float(m[r * kLines + k] / std::sqrt(nn));
    }
  };
  unitRows(inDir_, ns);
  unitRows(outDir_, ns + 2);
  {  // The two microphones: orthogonal taps (an uncorrelated reverberant field).
    float* l = &outDir_[ns * kLines];
    float* r = &outDir_[(ns + 1) * kLines];
    double d = 0, nn = 0;
    for (int k = 0; k < kLines; ++k) d += l[k] * r[k];
    for (int k = 0; k < kLines; ++k) { r[k] = float(r[k] - d * l[k]); nn += r[k] * r[k]; }
    for (int k = 0; k < kLines; ++k) r[k] = float(r[k] / std::sqrt(nn));
  }
  // Diffusers: 0.11, 0.23, 0.37 mean free paths (prime).
  const double df[kDiff] = {0.11, 0.23, 0.37};
  for (int j = 0; j < kDiff; ++j) {
    int n = std::clamp(int(mfp / kC * fs * df[j]), 17, kDiffRing - 1);
    while (!isPrime(n)) ++n;
    diffLen_[j] = n;
  }
  diff_.assign(size_t(ns) * kDiff * kDiffRing, 0.0f);
  ring_.assign(size_t(ns) * kRing, 0.0f);
  // Between players: direct sound.
  dDelay_.assign(ns * ns, 0);
  dGain_.assign(ns * ns, 0.0f);
  for (int to = 0; to < ns; ++to)
    for (int from = 0; from < ns; ++from) {
      if (to == from) continue;  // its own sound is in its own model
      const double r = std::max(0.6, std::fabs(x[to] - x[from]));
      // Players hear one block late: shorten their paths by a block.
      dDelay_[to * ns + from] = std::clamp(int(std::lround(r / kC * fs)) - kBlock, 0, kRing - kBlock - 1);
      dGain_[to * ns + from] = float(1 / r);
    }
  // To the microphones: image sources up to second order. Stage: the players
  // 3 m from the front wall (at most a fifth of the length), 1.2 m high; the
  // microphones 1.5 m high, the listener distance further, 2 m apart, aimed
  // 55 degrees out to their sides (a + (1 - a) cos: a reflection off the right
  // wall is louder in the right channel, the left wall's in the left: the
  // early sound mirrors the stage). Walls absorb differently: hard side walls
  // (0.4 x the mean), front, back and ceiling 0.8 x, the audience floor 2.5 x.
  if (spec.volume > 0) {
    const double ys = std::min(3.0, 0.2 * len), ym = std::min(ys + spec.listener, len - 1);
    const double alpha = std::min(0.9, absorption / surface);
    auto refl = [&](double k) { return std::sqrt(1 - std::min(0.95, k * alpha)); };
    const double bSide = refl(0.4), bEnd = refl(0.8), bCeil = refl(0.8), bFloor = refl(2.5);
    auto image = [](int u, double c, double d) { return u % 2 == 0 ? u * d + c : (u + 1) * d - c; };
    for (int s = 0; s < ns; ++s)
      for (int mic = 0; mic < 2; ++mic) {
        const double sx = std::clamp(wid / 2 + x[s], 0.3, wid - 0.3), sy = ys, sz = std::min(1.2, hei / 2);
        const double mx = wid / 2 + (mic ? 1.0 : -1.0), my = ym, mz = std::min(1.5, hei / 2);
        const double ax = std::sin(kMicAngle) * (mic ? 1 : -1), ay = -std::cos(kMicAngle);  // towards the stage
        for (int u = -2; u <= 2; ++u)
          for (int w = -2; w <= 2; ++w)
            for (int z = -2; z <= 2; ++z) {
              const int order = std::abs(u) + std::abs(w) + std::abs(z);
              if (order > 2) continue;
              const double dx = image(u, sx, wid) - mx, dy = image(w, sy, len) - my, dz = image(z, sz, hei) - mz;
              const double r = std::sqrt(dx * dx + dy * dy + dz * dz);
              const int d = int(std::lround(r / kC * fs));
              if (d >= kRing - kBlock) continue;
              double g = std::pow(bSide, std::abs(u)) * std::pow(bEnd, std::abs(w));
              if (z == -1 || std::abs(z) == 2) g *= bFloor;
              if (z == 1 || std::abs(z) == 2) g *= bCeil;
              const double cosT = (dx * ax + dy * ay) / std::max(r, 1e-9);
              g *= kMicPattern + (1 - kMicPattern) * cosT;
              early_.push_back({s, mic, d, float(g / std::max(r, 0.5)), u, w, z});
            }
      }
    // The reverberant field reaches a directional microphone weaker than an
    // omni: energy a^2 + (1 - a)^2 / 3.
    micDiffuse_ = float(std::sqrt(kMicPattern * kMicPattern + (1 - kMicPattern) * (1 - kMicPattern) / 3));
    const double on = kMicPattern + (1 - kMicPattern) * std::cos(kMicAngle);  // the stage's centre, about
    micGain_ = float(spec.listener /
                     std::sqrt(on * on + 16 * M_PI / absorption * spec.listener * spec.listener * micDiffuse_ * micDiffuse_));
  }
}

std::string Room::lateral(int s) const {
  std::string out;
  for (int u : {-1, 1}) {
    double t = 0, g[2] = {0, 0};
    for (const Tap& tp : early_)
      if (tp.src == s && tp.u == u && tp.w == 0 && tp.z == 0) {
        g[tp.mic] = tp.gain;
        if (tp.mic == (u > 0)) t = tp.delay * 1000.0 / fs_;  // at the microphone on its side
      }
    char b[128];
    std::snprintf(b, sizeof b, "%s%s wall %.0f ms, right - left %+.1f dB", out.empty() ? "" : "; ",
                  u < 0 ? "left" : "right", t, 20 * std::log10(g[1] / g[0]));
    out += b;
  }
  return out;
}

void Room::process(const float* const* src, int n, float* const* atSource, float* left, float* right,
                   const float* listenGain) {
  const int ns = int(x_.size());
  const float lg = micGain_;
  for (int i = 0; i < n; ++i) {
    const int now = (rpos_ + i) & (kRing - 1);
    for (int s = 0; s < ns; ++s) ring_[size_t(s) * kRing + now] = src[s][i];
    float x[kLines], y[kLines];
    for (int k = 0; k < kLines; ++k) {
      const float raw = line_[k * kLineRing + ((pos_ - len_[k]) & (kLineRing - 1))];
      lp_[k] = b0_[k] * raw + a1_[k] * lp_[k];
      x[k] = lp_[k];
    }
    // The reverberant field at each listener.
    for (int t = 0; t < ns + 2; ++t) {
      const float* dir = &outDir_[t * kLines];
      float acc = 0;
      for (int k = 0; k < kLines; ++k) acc += dir[k] * x[k];
      if (t < ns) {
        for (int s = 0; s < ns; ++s) {  // the others' direct sound
          const float g = dGain_[t * ns + s];
          if (g != 0) acc += g * ring_[size_t(s) * kRing + ((now - dDelay_[t * ns + s]) & (kRing - 1))];
        }
        atSource[t][i] = acc;
      } else {
        (t == ns ? left : right)[i] += lg * micDiffuse_ * acc;
      }
    }
    // Early sound at the microphones.
    float e[2] = {0, 0};
    for (const Tap& tp : early_)
      e[tp.mic] += listenGain[tp.src] * tp.gain * ring_[size_t(tp.src) * kRing + ((now - tp.delay) & (kRing - 1))];
    left[i] += lg * e[0];
    right[i] += lg * e[1];
    // Mixing: fast Hadamard transform, / 4 (orthogonal).
    for (int k = 0; k < kLines; ++k) y[k] = x[k];
    for (int h = 1; h < kLines; h <<= 1)
      for (int k = 0; k < kLines; k += 2 * h)
        for (int j = k; j < k + h; ++j) {
          const float p = y[j], q = y[j + h];
          y[j] = p + q;
          y[j + h] = p - q;
        }
    for (int k = 0; k < kLines; ++k) y[k] *= 0.25f;
    // The players' sound in, through the diffusers (Schroeder allpasses, 0.6).
    const int dp = (rpos_ + i) & (kDiffRing - 1);
    for (int s = 0; s < ns; ++s) {
      float in = inGain_ * src[s][i];
      for (int j = 0; j < kDiff; ++j) {
        float* buf = &diff_[(size_t(s) * kDiff + j) * kDiffRing];
        const float dl = buf[(dp - diffLen_[j]) & (kDiffRing - 1)];
        const float w = in + 0.6f * dl;
        buf[dp] = w;
        in = dl - 0.6f * w;
      }
      const float* dir = &inDir_[s * kLines];
      for (int k = 0; k < kLines; ++k) y[k] += dir[k] * in;
    }
    for (int k = 0; k < kLines; ++k) line_[k * kLineRing + (pos_ & (kLineRing - 1))] = y[k];
    pos_ = (pos_ + 1) & (kLineRing - 1);
  }
  rpos_ = (rpos_ + n) & (kRing - 1);
}

}  // namespace pajupilli
