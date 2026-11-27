#include "room.h"

#include "knob.h"
#include "vmath.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdio>

namespace pajupilli {

namespace {

constexpr RoomSpec kRooms[] = {
    {"dry", 0, 0, 2},
    {"booth", 40, 0.3, 1.5},      // a small, dead room (a drum booth)
    {"studio", 150, 0.5, 2},      // a small studio, damped
    {"hall", 2500, 1.8, 3},       // a chamber-music hall
    {"concert", 18000, 2.0, 8},   // a symphony hall: where the timpani belong (research/timpani.md 12.8b)
    {"church", 15000, 4.0, 6},    // a stone church
    {"gated", 3000, 4.0, 3, 0.5},  // a big live room, its ambience compressed and gated (80s snare, toms)
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
  // (PJ_CONCERT_BASSRT: the symphony hall's reverberation time in the bass, x the middle's; 1: as the middle)
  const double bassRt = std::strcmp(spec.name, "concert") == 0 ? std::max(1.0, knob("PJ_CONCERT_BASSRT", 1.0)) : 1.0;
  bassA_ = float(1 - std::exp(-2 * M_PI * knob("PJ_ROOM_BASSHZ", 200) / fs));
  double g2 = 0;
  for (int k = 0; k < kLines; ++k) {
    // Spread 0.5 .. 1.6 mean free paths, prime lengths (no common echoes).
    int n = int(mfp / kC * fs * (0.5 + 1.1 * k / (kLines - 1)));
    n = std::max(n, 2 * kBlock + 1);
    while (!isPrime(n)) ++n;
    n = std::min(n, kLineRing - 1);
    len_[k] = n;
    const double gl = fastPow(10.0, -3.0 * n / (fs * rt));  // per pass, low
    const double gh = fastPow(10.0, -3.0 * n / (fs * rtHigh));
    // One pole: DC gain gl, Nyquist gain gh.
    a1_[k] = float((gl - gh) / (gl + gh));
    b0_[k] = float(gl * (1 - a1_[k]));
    // (the bass rings longer in a large hall, its warmth: under PJ_ROOM_BASSHZ the decay time x bassRt)
    bassC_[k] = float(fastPow(10.0, -3.0 * n / (fs * rt * bassRt)) / gl - 1);
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
  ring_.assign(size_t(ns) * kSpan, 0.0f);
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
    // (PJ_CONCERT_LISTENER: the symphony hall's main microphones, m from the players; further back,
    // less of the direct sound against the reflections and the reverberation)
    const double listener = std::strcmp(spec.name, "concert") == 0 ? knob("PJ_CONCERT_LISTENER", spec.listener) : spec.listener;
    const double ys = std::min(3.0, 0.2 * len), ym = std::min(ys + listener, len - 1);
    const double alpha = std::min(0.9, absorption / surface);
    wid_ = wid, len_m_ = len, hei_ = hei, ys_ = ys, alpha_ = alpha;
    auto refl = [&](double k) { return std::sqrt(1 - std::min(0.95, k * alpha)); };
    const double bSide = refl(0.4), bEnd = refl(0.8), bCeil = refl(0.8), bFloor = refl(2.5);
    auto image = [](int u, double c, double d) { return u % 2 == 0 ? u * d + c : (u + 1) * d - c; };
    for (int s = 0; s < ns; ++s)
      for (int mic = 0; mic < 2; ++mic) {
        const double sx = std::clamp(wid / 2 + x[s], 0.3, wid - 0.3), sy = ys, sz = std::min(1.2, hei / 2);
        const double mx = wid / 2 + (mic ? 1.0 : -1.0), my = ym, mz = std::min(1.5, hei / 2);
        const double ax = std::sin(kMicAngle) * (mic ? 1 : -1), ay = -std::cos(kMicAngle);  // towards the stage
        // (PJ_ROOM_ORDER: how many bounces the paths are followed for, 2 .. 5.)
        static const int kOrder = std::clamp(int(knob("PJ_ROOM_ORDER", 2)), 2, 5);
        for (int u = -kOrder; u <= kOrder; ++u)
          for (int w = -kOrder; w <= kOrder; ++w)
            for (int z = -kOrder; z <= kOrder; ++z) {
              const int order = std::abs(u) + std::abs(w) + std::abs(z);
              if (order > kOrder) continue;
              const double dx = image(u, sx, wid) - mx, dy = image(w, sy, len) - my, dz = image(z, sz, hei) - mz;
              const double r = std::sqrt(dx * dx + dy * dy + dz * dz);
              const int d = int(std::lround(r / kC * fs));
              if (d >= kRing - kBlock) continue;
              double g = fastPow(bSide, std::abs(u)) * fastPow(bEnd, std::abs(w));
              // (the height's bounces alternate: an image above met the ceiling first, one below the floor)
              const int hits = std::abs(z), first = (hits + 1) / 2, second = hits / 2;
              g *= fastPow(bFloor, z < 0 ? first : second);
              g *= fastPow(bCeil, z > 0 ? first : second);
              const double cosT = (dx * ax + dy * ay) / std::max(r, 1e-9);
              g *= kMicPattern + (1 - kMicPattern) * cosT;
              Tap tp{s, mic, d, float(g / std::max(r, 0.5)), u, w, z};
              // (The image is the source mirrored: the path leaves the source itself in the
              // image's direction with each hit wall's component turned round.)
              tp.n[0] = float(-dx / std::max(r, 1e-9)) * (u & 1 ? -1 : 1);
              tp.n[1] = float(-dy / std::max(r, 1e-9)) * (w & 1 ? -1 : 1);
              tp.n[2] = float(-dz / std::max(r, 1e-9)) * (z & 1 ? -1 : 1);
              // A large hall's walls are not mirrors: balcony fronts, boxes and panels throw part
              // of each first and second reflection back a little later and from nearby, a cluster
              // around the mirror path. Half of its energy (PJ_ROOM_SCATTER) goes there, the side
              // walls' into six arrivals, the others' into three, each up to 0.6 mean free paths
              // later, its own for each microphone: the sound mixes before the reverberation.
              static const double kScatter = std::clamp(knob("PJ_ROOM_SCATTER", 0.5), 0.0, 0.95);
              const int nScat = spec.volume >= 1000 && spec.gate == 0 && kScatter > 0 && order >= 1 && order <= 2 ? (u != 0 ? 6 : 3) : 0;
              const float whole = tp.gain;
              if (nScat) tp.gain = float(whole * std::sqrt(1 - kScatter));
              early_.push_back(tp);
              unsigned h = unsigned(s + 1) * 2654435761u ^ unsigned(mic + 1) * 40503u ^ unsigned(u + 8) * 6151u ^ unsigned(w + 8) * 769u ^ unsigned(z + 8) * 97u;
              for (int q = 0; q < nScat; ++q) {
                h = h * 1664525u + 1013904223u;
                const double extra = mfp * (0.08 + 0.52 * ((h >> 8) / 16777216.0));
                const int ds = int(std::lround((r + extra) / kC * fs));
                if (ds >= kRing - kBlock) continue;
                Tap sc = tp;
                sc.delay = ds;
                sc.gain = float(whole * std::sqrt(kScatter / nScat) * r / (r + extra));
                sc.scattered = true;
                early_.push_back(sc);
              }
            }
      }
    // The reverberant field reaches a directional microphone weaker than an
    // omni: energy a^2 + (1 - a)^2 / 3.
    micDiffuse_ = float(std::sqrt(kMicPattern * kMicPattern + (1 - kMicPattern) * (1 - kMicPattern) / 3));
    const double on = kMicPattern + (1 - kMicPattern) * std::cos(kMicAngle);  // the stage's centre, about
    micGain_ = float(listener /
                     std::sqrt(on * on + 16 * M_PI / absorption * listener * listener * micDiffuse_ * micDiffuse_));
    directShare_ = on * on / (on * on + 16 * M_PI / absorption * listener * listener * micDiffuse_ * micDiffuse_);
  }
}

bool Room::ambienceArray(int count, double near, double far) {
  const int ns = int(x_.size());
  if (count <= 0 || spec_.volume <= 0 || ambMics_) return false;
  const double margin = 0.5, cx = wid_ / 2, cy = ys_;
  auto inside = [&](double x, double y) { return x > margin && x < wid_ - margin && y > margin && y < len_m_ - margin; };
  // (the farthest any angle reaches within the walls: the room must hold the near distance somewhere)
  if (!inside(cx, cy + far) || !inside(cx + near, cy) || !inside(cx - near, cy)) return false;
  const double golden = M_PI * (3 - std::sqrt(5.0)), mz = std::min(2.5, hei_ - 0.5);
  const double sz = std::min(1.2, hei_ / 2);
  auto refl = [&](double k) { return std::sqrt(1 - std::min(0.95, k * alpha_)); };
  const double bSide = refl(0.4), bEnd = refl(0.8), bCeil = refl(0.8), bFloor = refl(2.5);
  auto image = [](int u, double c, double d) { return u % 2 == 0 ? u * d + c : (u + 1) * d - c; };
  struct Mic { double x, y, wl, wr; };
  std::vector<Mic> mics;
  double nearest = 1e9, farthest = 0;
  for (int i = 0; i < count; ++i) {
    // (the distance: van der Corput in base 2 over near .. far; the angle: the golden angle's)
    double vdc = 0, f = 0.5;
    for (int b = i + 1; b; b >>= 1, f *= 0.5) vdc += (b & 1) * f;
    double d = near + (far - near) * vdc;
    const double a0 = std::fmod(i * golden, 2 * M_PI);
    double mx = 0, my = 0;
    bool placed = false;
    for (; !placed && d >= near * 0.5; d *= 0.97)  // (a small room: in, a little at a time)
      for (int step = 0; step <= 180 && !placed; ++step)
        for (int sgn : {1, -1}) {
          const double a = a0 + sgn * step * M_PI / 180;
          // (angle 0: straight into the hall, away from the stage's wall; positive: towards the right)
          const double x = cx + d * std::sin(a), y = cy + d * std::cos(a);
          if (inside(x, y)) { mx = x; my = y; placed = true; break; }
        }
    if (!placed) continue;
    const double r = std::hypot(mx - cx, my - cy), lat = std::clamp((mx - cx) / far, -1.0, 1.0);
    nearest = std::min(nearest, r);
    farthest = std::max(farthest, r);
    mics.push_back({mx, my, 0.5 * (1 - lat), 0.5 * (1 + lat)});
  }
  if (mics.empty()) return false;
  double sl = 0, sr = 0;
  for (const Mic& m : mics) { sl += m.wl; sr += m.wr; }
  // (a channel the mics' weighted mean: their direct sounds add up in step in the deep bass,
  // which so stays at one mic's level; higher up they arrive apart and add as power, quieter)
  const double nl = 1 / std::max(sl, 1e-9), nr = 1 / std::max(sr, 1e-9);
  // (the array mixed in under the main pair: PJ_ROOM_AMB_DB on all of it, -10; PJ_ROOM_AMB_EARLY_DB
  // more on its direct sound and reflections alone)
  // (in the symphony hall -5: the owner's ear on a concerto, 2026-11-25, more of the room)
  const double level = fastPow(10.0, knob("PJ_ROOM_AMB_DB", std::strcmp(spec_.name, "concert") == 0 ? -5.0 : -10.0) / 20);
  const double early = level * fastPow(10.0, knob("PJ_ROOM_AMB_EARLY_DB", 0.0) / 20);
  amb_.clear();
  ambFrom_.assign(ns + 1, 0);
  for (int s = 0; s < ns; ++s) {
    ambFrom_[s] = int(amb_.size());
    const double sx = std::clamp(wid_ / 2 + x_[s], 0.3, wid_ - 0.3), sy = ys_;
    for (const Mic& m : mics)
      for (int u = -1; u <= 1; ++u)
        for (int w = -1; w <= 1; ++w)
          for (int z = -1; z <= 1; ++z) {
            if (std::abs(u) + std::abs(w) + std::abs(z) > 1) continue;  // (the direct sound and the six walls once)
            const double dx = image(u, sx, wid_) - m.x, dy = image(w, sy, len_m_) - m.y, dz = image(z, sz, hei_) - mz;
            const double r = std::sqrt(dx * dx + dy * dy + dz * dz);
            const int dl = int(std::lround(r / kC * fs_));
            if (dl >= kRing - kBlock) continue;
            double g = (u ? bSide : 1) * (w ? bEnd : 1) * (z < 0 ? bFloor : z > 0 ? bCeil : 1) / std::max(r, 0.5);
            g *= early;
            amb_.push_back({s, dl, float(g * m.wl * nl), float(g * m.wr * nr)});
          }
  }
  ambFrom_[ns] = int(amb_.size());
  // The reverberant field: each mic its own random tap; a channel's is their weighted sum, as
  // long as one tap (an omni's level); left and right as alike as the weights make them.
  unsigned seed = 777;
  auto gauss = [&]() {
    double t = 0;
    for (int j = 0; j < 4; ++j) { seed = seed * 1664525u + 1013904223u; t += (seed >> 8) / double(1 << 24) - 0.5; }
    return t;
  };
  double L[kLines] = {}, R[kLines] = {};
  for (const Mic& m : mics) {
    double row[kLines], nn = 0;
    for (double& v : row) { v = gauss(); nn += v * v; }
    for (int k = 0; k < kLines; ++k) {
      L[k] += m.wl * row[k] / std::sqrt(nn);
      R[k] += m.wr * row[k] / std::sqrt(nn);
    }
  }
  double ll = 0, rr = 0, lr = 0;
  for (int k = 0; k < kLines; ++k) { ll += L[k] * L[k]; rr += R[k] * R[k]; lr += L[k] * R[k]; }
  for (int k = 0; k < kLines; ++k) {
    ambDir_[k] = float(level * L[k] / std::sqrt(ll));
    ambDir_[kLines + k] = float(level * R[k] / std::sqrt(rr));
  }
  ambMics_ = int(mics.size());
  std::fprintf(stderr, "  room mics: %d mono omnis %.1f .. %.1f m from the stage, %.1f .. %.1f m across; their reverb's left-right correlation %.2f\n",
               ambMics_, nearest, farthest,
               std::min_element(mics.begin(), mics.end(), [](const Mic& a, const Mic& b) { return a.x < b.x; })->x - cx,
               std::max_element(mics.begin(), mics.end(), [](const Mic& a, const Mic& b) { return a.x < b.x; })->x - cx,
               lr / std::sqrt(ll * rr));
  return true;
}

void Room::setAxes(int s, const float* axes) {
  const int ns = int(x_.size());
  if (hasDip_.empty()) {
    hasDip_.assign(ns, 0);
    dring_.assign(size_t(3) * ns * kSpan, 0.0f);
    // (their own random directions into the network, apart from the sources' own)
    unsigned seed = 4242;
    inDirDip_.resize(size_t(3) * ns * kLines);
    for (int r = 0; r < 3 * ns; ++r) {
      double nn = 0;
      for (int k = 0; k < kLines; ++k) {
        double t = 0;
        for (int j = 0; j < 4; ++j) { seed = seed * 1664525u + 1013904223u; t += (seed >> 8) / double(1 << 24) - 0.5; }
        inDirDip_[r * kLines + k] = float(t);
        nn += t * t;
      }
      for (int k = 0; k < kLines; ++k) inDirDip_[r * kLines + k] = float(inDirDip_[r * kLines + k] / std::sqrt(nn));
    }
  }
  hasDip_[s] = 1;
  for (Tap& tp : early_)
    if (tp.src == s)
      for (int k = 0; k < 3; ++k)
        tp.dg[k] = tp.gain * (axes[3 * k] * tp.n[0] + axes[3 * k + 1] * tp.n[1] + axes[3 * k + 2] * tp.n[2]);
}

// Each of source s's np parts' delay and gain on each of its taps (part by part, in early_'s order).
void Room::partPaths(int s, int np, const float* places, const float* lobes, std::vector<int>& dl,
                     std::vector<float>& gn) const {
  for (int j = 0; j < np; ++j) {
    const float* o = places + 3 * j;
    for (const Tap& tp : early_) {
      if (tp.src != s) continue;
      const double r = tp.delay * kC / fs_;  // (the path's length)
      const double along = tp.n[0] * o[0] + tp.n[1] * o[1] + tp.n[2] * o[2];  // (shorter by this)
      const double rp = std::max(0.3, r - along);
      dl.push_back(std::clamp(int(std::lround(rp / kC * fs_)), 1, kRing - kBlock - 1));
      double lobe = 1;
      if (lobes) {
        const float* L = lobes + 4 * j;
        lobe = std::max(0.0, 1.0 + L[3] * double(tp.n[0] * L[0] + tp.n[1] * L[1] + tp.n[2] * L[2]));
      }
      gn.push_back(float(lobe * tp.gain * std::max(r, 0.5) / std::max(rp, 0.5)));
    }
  }
}

// Where source s's parts' paths start in pDelay_ / pGain_.
size_t Room::partTapAt(int s) const {
  size_t tapAt = 0;
  for (int q = 0; q < s; ++q) {
    int nt = 0;
    for (const Tap& tp : early_) nt += tp.src == q;
    tapAt += size_t(partFrom_[q + 1] - partFrom_[q]) * nt;
  }
  return tapAt;
}

void Room::setParts(int s, int np, const float* places, const float* lobes) {
  const int ns = int(x_.size());
  if (partFrom_.empty()) partFrom_.assign(ns + 1, 0);
  // (parts are added source by source in order: those of later sources move up)
  const int old = partFrom_[s + 1] - partFrom_[s];
  if (old != 0 || np <= 0) return;
  std::vector<int> dl;
  std::vector<float> gn;
  partPaths(s, np, places, lobes, dl, gn);
  // Insert after the parts of the sources before s.
  const int at = partFrom_[s];
  const size_t tapAt = partTapAt(s);
  pDelay_.insert(pDelay_.begin() + long(tapAt), dl.begin(), dl.end());
  pGain_.insert(pGain_.begin() + long(tapAt), gn.begin(), gn.end());
  if (!pDelayFrom_.empty()) {  // (a source already moving: its ramps move up with the rest)
    pDelayFrom_.insert(pDelayFrom_.begin() + long(tapAt), dl.begin(), dl.end());
    pGainFrom_.insert(pGainFrom_.begin() + long(tapAt), gn.begin(), gn.end());
    pDelayNext_.insert(pDelayNext_.begin() + long(tapAt), dl.begin(), dl.end());
    pGainNext_.insert(pGainNext_.begin() + long(tapAt), gn.begin(), gn.end());
  }
  pring_.insert(pring_.begin() + long(size_t(at) * kSpan), size_t(np) * kSpan, 0.0f);
  pquiet_.insert(pquiet_.begin() + at, np, 1 << 30);
  for (int q = s + 1; q <= ns; ++q) partFrom_[q] += np;
}

void Room::moveParts(int s, const float* places, const float* lobes) {
  if (partFrom_.empty()) return;
  const int np = partFrom_[s + 1] - partFrom_[s];
  if (np <= 0) return;
  if (pDelayFrom_.empty()) {
    pDelayFrom_ = pDelayNext_ = pDelay_;
    pGainFrom_ = pGainNext_ = pGain_;
    pMove_.assign(x_.size(), 0);
  }
  std::vector<int> dl;
  std::vector<float> gn;
  partPaths(s, np, places, lobes, dl, gn);
  const size_t tapAt = partTapAt(s);
  std::copy(dl.begin(), dl.end(), pDelayNext_.begin() + long(tapAt));
  std::copy(gn.begin(), gn.end(), pGainNext_.begin() + long(tapAt));
  pMove_[s] = 1;
}

std::string Room::lateral(int s) const {
  std::string out;
  for (int u : {-1, 1}) {
    double t = 0, g[2] = {0, 0};
    for (const Tap& tp : early_)
      if (tp.src == s && !tp.scattered && tp.u == u && tp.w == 0 && tp.z == 0) {
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

// A block at a time (kBlock samples or the shortest diffuser, whichever is less: within one, no
// delay reads what the block itself writes), each part a plain loop over the block's samples:
// the players' rings, the diffusers, the delay network, then every path (player to player, the
// images to the microphones) a run of consecutive samples out of a ring. A player silent for
// longer than its rings and diffusers remember costs nothing but its own ear.
void Room::process(const float* const* src, int n, float* const* atSource, float* left, float* right,
                   const float* listenGain) {
  const int step = std::min(kBlock, std::min(diffLen_[0], std::min(diffLen_[1], diffLen_[2])));
  // (parts moved since the last call: from where they were to where they are now, across this one)
  for (size_t s = 0; s < pMove_.size(); ++s) {
    if (pMove_[s] == 1) {
      const int nt = int(std::count_if(early_.begin(), early_.end(), [&](const Tap& tp) { return tp.src == int(s); }));
      const size_t a = partTapAt(int(s)), b = a + size_t(partFrom_[s + 1] - partFrom_[s]) * nt;
      std::copy(pDelay_.begin() + long(a), pDelay_.begin() + long(b), pDelayFrom_.begin() + long(a));
      std::copy(pGain_.begin() + long(a), pGain_.begin() + long(b), pGainFrom_.begin() + long(a));
      std::copy(pDelayNext_.begin() + long(a), pDelayNext_.begin() + long(b), pDelay_.begin() + long(a));
      std::copy(pGainNext_.begin() + long(a), pGainNext_.begin() + long(b), pGain_.begin() + long(a));
      pMove_[s] = 2;
    }
  }
  rampN_ = n;
  for (int o = 0; o < n; o += step) block(src, o, std::min(step, n - o), atSource, left, right, listenGain);
  for (char& m : pMove_) if (m == 2) m = 0;
}

namespace {
// out[i] += g * ring[at + i], i < n (the ring's first block stands again past its end: no wrap).
// A whole block is two vectors' work.
inline void addRun(float* __restrict out, const float* __restrict ring, int at, int n, float g) {
  const float* __restrict r = ring + at;
  if (n == Room::kBlock) {
    for (int i = 0; i < Room::kBlock; ++i) out[i] += g * r[i];
  } else {
    for (int i = 0; i < n; ++i) out[i] += g * r[i];
  }
}
// out[i] += g(i) * ring[at + i], the gain going from g0 towards g1 (reached one sample past the run).
struct RampTable {
  alignas(64) float t[Room::kBlock];
  RampTable() { for (int i = 0; i < Room::kBlock; ++i) t[i] = float(i) / Room::kBlock; }
};
const RampTable kRamp;
inline void addRamp(float* __restrict out, const float* __restrict ring, int at, int n, float g0, float g1) {
  const float* __restrict r = ring + at;
  if (n == Room::kBlock) {
    const float d = g1 - g0;
    for (int i = 0; i < Room::kBlock; ++i) out[i] += (g0 + d * kRamp.t[i]) * r[i];
  } else {
    const float d = (g1 - g0) / float(n);
    for (int i = 0; i < n; ++i) out[i] += (g0 + d * float(i)) * r[i];
  }
}
// The same run into left and right at once (one read of the ring for both).
inline void addRun2(float* __restrict l, float* __restrict r, const float* __restrict ring, int at, int n, float gl, float gr) {
  const float* __restrict s = ring + at;
  if (n == Room::kBlock) {
    for (int i = 0; i < Room::kBlock; ++i) { l[i] += gl * s[i]; r[i] += gr * s[i]; }
  } else {
    for (int i = 0; i < n; ++i) { l[i] += gl * s[i]; r[i] += gr * s[i]; }
  }
}
}  // namespace

void Room::block(const float* const* src, int o, int n, float* const* atSource, float* left, float* right,
                 const float* listenGain) {
  const int ns = int(x_.size());
  const float lg = micGain_;
  const bool dips = !hasDip_.empty();
  if (int(quiet_.size()) != ns) {
    quiet_.assign(ns, 0);
    // (a diffuser's state has left the floats after 171 passes at 0.6; the rings after kRing)
    // (PJ_ROOM_SKIP=0: none forgotten, to check that forgetting changes nothing)
    forget_ = knob("PJ_ROOM_SKIP", 1) != 0 ? kRing + 200 * std::max(diffLen_[0], std::max(diffLen_[1], diffLen_[2])) : 1 << 30;
    din_.assign(size_t(ns) * kBlock, 0.0f);
    // (each player's paths to the microphones: one after another in early_, as they were made)
    tapFrom_.assign(ns + 1, int(early_.size()));
    for (int k = int(early_.size()) - 1; k >= 0; --k) tapFrom_[early_[k].src] = k;
    for (int q = ns - 1; q >= 0; --q) tapFrom_[q] = std::min(tapFrom_[q], tapFrom_[q + 1]);
    pTap_.assign(ns + 1, 0);
    for (int q = 0; q < ns; ++q)
      pTap_[q + 1] = pTap_[q] + (partFrom_.empty() ? 0 : (partFrom_[q + 1] - partFrom_[q]) * (tapFrom_[q + 1] - tapFrom_[q]));
  }
  // The players' sound into their rings; who is still heard.
  for (int s = 0; s < ns; ++s) {
    const float* in = src[s] + o;
    float* ring = &ring_[size_t(s) * kSpan];
    float any = 0;
    for (int i = 0; i < n; ++i) {
      const int at = (rpos_ + i) & (kRing - 1);
      ring[at] = in[i];
      if (at < kBlock) ring[kRing + at] = in[i];  // (the ring's start again past its end: a run never wraps)
      any += std::fabs(in[i]);
    }
    if (dips && hasDip_[s])
      for (int k = 0; k < 3; ++k) {
        const float* d = dip_ ? dip_[3 * s + k] : nullptr;
        float* dr = &dring_[size_t(3 * s + k) * kSpan];
        for (int i = 0; i < n; ++i) {
          const float v = d ? d[o + i] : 0.0f;
          const int at = (rpos_ + i) & (kRing - 1);
          dr[at] = v;
          if (at < kBlock) dr[kRing + at] = v;
          any += std::fabs(v);
        }
      }
    if (!partFrom_.empty())
      for (int j = partFrom_[s]; j < partFrom_[s + 1]; ++j) {
        const float* p = part_ ? part_[j] : nullptr;
        float* pr = &pring_[size_t(j) * kSpan];
        float pa = 0;
        for (int i = 0; i < n; ++i) {
          const float v = p ? p[o + i] : 0.0f;
          const int at = (rpos_ + i) & (kRing - 1);
          pr[at] = v;
          if (at < kBlock) pr[kRing + at] = v;
          pa += std::fabs(v);
        }
        if (pa != 0) pquiet_[j] = 0;
        else if (pquiet_[j] < kRing + kBlock) pquiet_[j] += n;
      }
    if (any != 0) {
      quiet_[s] = 0;
    } else if (quiet_[s] < forget_) {
      quiet_[s] += n;
      if (quiet_[s] >= forget_)
        std::fill_n(&diff_[size_t(s) * kDiff * kDiffRing], size_t(kDiff) * kDiffRing, 0.0f);
    }
  }
  auto heard = [&](int s) { return quiet_[s] < forget_; };
  // The players' sound through the diffusers (Schroeder allpasses, 0.6).
  for (int s = 0; s < ns; ++s) {
    if (!heard(s)) continue;
    float* in = &din_[size_t(s) * kBlock];
    for (int i = 0; i < n; ++i) in[i] = inGain_ * src[s][o + i];
    for (int j = 0; j < kDiff; ++j) {
      float* buf = &diff_[(size_t(s) * kDiff + j) * kDiffRing];
      for (int i = 0; i < n;) {
        const int wi = (rpos_ + i) & (kDiffRing - 1), ri = (wi - diffLen_[j]) & (kDiffRing - 1);
        const int run = std::min(n - i, kDiffRing - std::max(wi, ri));
        float* w = buf + wi;
        const float* r = buf + ri;
        float* x = in + i;
#pragma GCC ivdep
        for (int q = 0; q < run; ++q) {
          const float dl = r[q];
          const float v = x[q] + 0.6f * dl;
          w[q] = v;
          x[q] = dl - 0.6f * v;
        }
        i += run;
      }
    }
  }
  // The delay network, sample by sample.
  for (int i = 0; i < n; ++i) {
    const int now = (rpos_ + i) & (kRing - 1);
    alignas(64) float x[kLines], y[kLines];
    for (int k = 0; k < kLines; ++k) {
      const float raw = line_[k * kLineRing + ((pos_ - len_[k]) & (kLineRing - 1))];
      lp_[k] = b0_[k] * raw + a1_[k] * lp_[k];
      bass_[k] += bassA_ * (lp_[k] - bass_[k]);
      x[k] = lp_[k] + bassC_[k] * bass_[k];
    }
    // The gate's side chain, keyed by the dry sound: a hit (its fast level
    // jumping over its slow one, within 20 dB of the recent peaks) opens it
    // for spec.gate seconds; then it shuts in ~10 ms, whatever still rings.
    float g = 1;
    if (spec_.gate > 0) {
      float in = 0;
      for (int s = 0; s < ns; ++s) in += std::fabs(src[s][o + i]);
      gateFast_ = std::max(in, gateFast_ * float(std::exp(-1 / (0.002 * fs_))));
      gateSlow_ += (gateFast_ - gateSlow_) * float(1 - std::exp(-1 / (0.05 * fs_)));
      gatePeak_ = std::max(gateFast_, gatePeak_ * float(std::exp(-1 / (3.0 * fs_))));
      if (gateFast_ > 2 * gateSlow_ && gateFast_ > std::max(1e-5f, 0.1f * gatePeak_)) gateHold_ = float(spec_.gate);
      else gateHold_ = std::max(0.0f, gateHold_ - float(1 / fs_));
      const float target = gateHold_ > 0 ? 1.0f : 0.0f;
      gateGain_ += (target - gateGain_) * float(1 - std::exp(-1 / ((target > gateGain_ ? 0.0005 : 0.010) * fs_)));
      g = gateGain_;
    }
    // The reverberant field at each listener: the players, then the microphones.
    float amb[2] = {0, 0};
    for (int t = 0; t < ns + 2; ++t) {
      const float* dir = &outDir_[t * kLines];
      float acc = 0;
      for (int k = 0; k < kLines; ++k) acc += dir[k] * x[k];
      if (t < ns) atSource[t][o + i] = acc;
      else amb[t - ns] = lg * micDiffuse_ * acc;
    }
    if (ambMics_)  // (the room microphones' reverberant field, under the pair's)
      for (int c = 0; c < 2; ++c) {
        float acc = 0;
        for (int k = 0; k < kLines; ++k) acc += ambDir_[c * kLines + k] * x[k];
        amb[c] += lg * acc;
      }
    if (spec_.gate > 0) {
      // The ambience compressed hard (8:1 over 20 dB under its recent peak,
      // 2 ms attack, 150 ms release; made up 3x): the body holds up while
      // the gate is open, as from squashed room mics.
      const float lvl = std::max(std::fabs(amb[0]), std::fabs(amb[1]));
      compEnv_ += (lvl - compEnv_) * float(1 - std::exp(-1 / ((lvl > compEnv_ ? 0.002 : 0.15) * fs_)));
      compPeak_ = std::max(compEnv_, compPeak_ * float(std::exp(-1 / (3.0 * fs_))));
      const float th = std::max(1e-9f, 0.1f * compPeak_);
      const float cg = compEnv_ > th ? fastPow(compEnv_ / th, 1.0f / 8 - 1) : 1.0f;
      g *= 3 * cg;
    }
    left[o + i] += g * amb[0];
    right[o + i] += g * amb[1];
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
    // The players' diffused sound in.
    for (int s = 0; s < ns; ++s) {
      if (!heard(s)) continue;
      const float in = din_[size_t(s) * kBlock + i];
      const float* dir = &inDir_[s * kLines];
      for (int k = 0; k < kLines; ++k) y[k] += dir[k] * in;
      // (The directional parts' power into the reverberant field: each a third of what the
      // same sound sent all round would give.)
      if (dips && hasDip_[s])
        for (int c = 0; c < 3; ++c) {
          const float d = 0.57735f * inGain_ * dring_[size_t(3 * s + c) * kSpan + now];
          const float* dd = &inDirDip_[size_t(3 * s + c) * kLines];
          for (int k = 0; k < kLines; ++k) y[k] += dd[k] * d;
        }
    }
    for (int k = 0; k < kLines; ++k) line_[k * kLineRing + (pos_ & (kLineRing - 1))] = y[k];
    pos_ = (pos_ + 1) & (kLineRing - 1);
  }
  // The others' direct sound at each player.
  for (int s = 0; s < ns; ++s) {
    if (!heard(s)) continue;
    const float* ring = &ring_[size_t(s) * kSpan];
    for (int t = 0; t < ns; ++t) {
      const float g = dGain_[t * ns + s];
      if (g != 0) addRun(atSource[t] + o, ring, (rpos_ - dDelay_[t * ns + s]) & (kRing - 1), n, g);
    }
  }
  // Early sound at the microphones.
  alignas(64) float e[2][kBlock] = {};
  for (int s = 0; s < ns; ++s) {
    const float gl = listenGain[s];
    if (!heard(s) || gl == 0) continue;
    if (!partFrom_.empty() && partFrom_[s + 1] > partFrom_[s]) {
      // (its parts, each by its own paths; a part silent longer than its ring holds: nothing)
      const int nt = tapFrom_[s + 1] - tapFrom_[s];
      for (int j = partFrom_[s]; j < partFrom_[s + 1]; ++j) {
        if (pquiet_[j] >= kRing) continue;
        const float* pr = &pring_[size_t(j) * kSpan];
        const int* dl = &pDelay_[size_t(pTap_[s] + (j - partFrom_[s]) * nt)];
        const float* gn = &pGain_[size_t(pTap_[s] + (j - partFrom_[s]) * nt)];
        if (!pMove_.empty() && pMove_[s] == 2) {
          // (moving: each path's gain from where the part was to where it is, across the call; a
          // path whose length changed crossfades its old delay into its new one)
          const size_t q = size_t(pTap_[s] + (j - partFrom_[s]) * nt);
          const int* d0 = &pDelayFrom_[q];
          const float* g0 = &pGainFrom_[q];
          const float r0 = float(o) / float(rampN_), r1 = float(o + n) / float(rampN_);
          for (int k = 0; k < nt; ++k) {
            if (early_[tapFrom_[s] + k].scattered) continue;
            float* ek = e[early_[tapFrom_[s] + k].mic];
            const float a = gl * g0[k], b = gl * gn[k];
            if (d0[k] == dl[k] && a == b) {
              addRun(ek, pr, (rpos_ - dl[k]) & (kRing - 1), n, b);
            } else if (d0[k] == dl[k]) {
              addRamp(ek, pr, (rpos_ - dl[k]) & (kRing - 1), n, a + (b - a) * r0, a + (b - a) * r1);
            } else {
              addRamp(ek, pr, (rpos_ - d0[k]) & (kRing - 1), n, a * (1 - r0), a * (1 - r1));
              addRamp(ek, pr, (rpos_ - dl[k]) & (kRing - 1), n, b * r0, b * r1);
            }
          }
          continue;
        }
        for (int k = 0; k < nt; ++k)
          if (!early_[tapFrom_[s] + k].scattered)
            addRun(e[early_[tapFrom_[s] + k].mic], pr, (rpos_ - dl[k]) & (kRing - 1), n, gl * gn[k]);
      }
      // (the walls' scattered clusters: diffuse, they take the whole source's sound, once)
      {
        const float* whole = &ring_[size_t(s) * kSpan];
        for (int k = tapFrom_[s]; k < tapFrom_[s + 1]; ++k)
          if (early_[k].scattered)
            addRun(e[early_[k].mic], whole, (rpos_ - early_[k].delay) & (kRing - 1), n, gl * early_[k].gain);
      }
      continue;
    }
    const float* ring = &ring_[size_t(s) * kSpan];
    const bool dip = dips && hasDip_[s];
    for (int k = tapFrom_[s]; k < tapFrom_[s + 1]; ++k) {
      const Tap& tp = early_[k];
      const int at = (rpos_ - tp.delay) & (kRing - 1);
      addRun(e[tp.mic], ring, at, n, gl * tp.gain);
      if (dip)
        for (int c = 0; c < 3; ++c) addRun(e[tp.mic], &dring_[size_t(3 * s + c) * kSpan], at, n, gl * tp.dg[c]);
    }
  }
  // The room microphones' direct sound and first reflections, already mixed to left and right.
  if (ambMics_)
    for (int s = 0; s < ns; ++s) {
      const float gl = listenGain[s];
      if (!heard(s) || gl == 0) continue;
      const float* ring = &ring_[size_t(s) * kSpan];
      for (int k = ambFrom_[s]; k < ambFrom_[s + 1]; ++k) {
        const AmbTap& t = amb_[k];
        const int at = (rpos_ - t.delay) & (kRing - 1);
        addRun2(e[0], e[1], ring, at, n, gl * t.gl, gl * t.gr);
      }
    }
  for (int i = 0; i < n; ++i) {
    left[o + i] += lg * e[0][i];
    right[o + i] += lg * e[1][i];
  }
  rpos_ = (rpos_ + n) & (kRing - 1);
}

}  // namespace pajupilli
