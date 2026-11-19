// Each organ pipe's own connection to the church (research/organ_room_coupling.md):
// every sounding pipe's mouth (and an open pipe's top, a reed's resonator top)
// has its own direct path and its own image sources to second order (the
// nave's side walls, the west wall, the vault, the floor, the gallery's floor,
// its case's back panel) to the two microphones, traced in the church's
// geometry with the cases around the pipes (an open front, the Rückpositiv's
// shadow, diffraction at the impost). Each path is one integer-delayed copy
// with its 1/r, its surfaces' loss, the microphone's pattern and the pipe's
// directivity, landing in one of 30 class rings (2 microphones x 5 classes x
// 3 shadow levels); each ring one short minimum-phase filter (the images'
// band for its order, the air over its paths, the shadow). The filters are
// linear and shared: summing first and filtering after is exact.
//
// The enclosures keep their near field (their modes, what each jet hears,
// the shutters); their field leaves as one source at each opening (a swell
// box's through its shutters, applied on its own ring set).
//
// The church: x across (north +, the listener's right), y up, z east from the
// west wall; a 50 x 20 x 20 m nave, the west gallery's floor at 7 m. The
// listener: two microphones 2 m apart, turned 55 degrees out, on the axis
// (src/organ.cc sets them: a recording's main pair on a tall stand, 4 m high,
// 10 m into the nave, ~10 m from the main case).
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <complex>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace pajupilli {

class OrganRoom {
 public:
  static constexpr int kBlock = 16;
  struct V3 {
    double x = 0, y = 0, z = 0;
  };
  struct Box {
    V3 lo, hi;
  };
  // The church's sets of rings: 0 the church, 1 and 2 the swell boxes' (read
  // through their shutters).
  static constexpr int kSets = 3;

  void setup(double fs, double listenerZ, double listenerX = 0, double listenerY = 1.2) {
    fs_ = fs;
    lis_ = makeListener(listenerZ, listenerX, listenerY);
    planes_ = roomPlanes();
    gallery_ = {{-kNaveW / 2, kGalleryY - 0.3, 0}, {kNaveW / 2, kGalleryY, kGalleryZ}};
    // The cases: the main case (the Hauptwerk; its impost below), the swell
    // boxes, the Rückpositiv (on its stand: a shadow), the pedal towers.
    cases_ = {
        {{{-2.0, 7.0, 1.6}, {2.0, 13.3, 3.0}}, 10.3, 13.3, 1.6, false},
        {{{-3.6, 13.4, 1.3}, {-1.0, 15.8, 2.9}}, 13.4, 15.8, 1.3, false},
        {{{1.0, 13.4, 1.3}, {3.6, 15.8, 2.9}}, 13.4, 15.8, 1.3, false},
        {{{-1.15, 7.0, 4.65}, {1.15, 9.3, 5.35}}, 7.9, 9.3, 4.65, true},
        {{{-4.5, 7.0, 1.1}, {-2.7, 13.2, 2.9}}, 7.2, 13.2, 1.1, false},
        {{{2.7, 7.0, 1.1}, {4.5, 13.2, 2.9}}, 7.2, 13.2, 1.1, false},
    };
    srcs_.clear();
  }
  enum CaseId { kMainCase = 0, kSwellCaseC = 1, kSwellCaseCis = 2, kPosCase = 3, kPedCaseC = 4, kPedCaseCis = 5, kNoCase = -1 };
  const Box& caseBox(int c) const { return cases_[c].box; }

  // A radiating point: at (in the church), in its case (or kNoCase: the
  // façade, a region's opening), the façade's pipes reflected by the main
  // case's front behind them (facade), occl 0..1 (the rows in front of it:
  // the share darkened), dirDark (a flue mouth's shadow behind it: 0 omni),
  // its ring set. Returns its index; its paths traced in finish().
  int addSource(V3 at, int caseId, bool facade, double occl, double dirDark, int set, double hz = 250) {
    Source s;
    s.at = at;
    s.hz = hz;
    s.caseId = caseId;
    if (caseId >= 0) {  // (inside its case, a little in from its walls)
      const Box& b = cases_[caseId].box;
      s.at.x = std::clamp(s.at.x, b.lo.x + 0.05, b.hi.x - 0.05);
      s.at.z = std::clamp(s.at.z, b.lo.z + 0.1, b.hi.z - 0.05);
      s.at.y = std::max(s.at.y, b.lo.y + 0.05);
      if (s.at.y >= b.hi.y) s.caseId = -1;  // (a top above its case's roof is in the open)
    }
    s.hasBack = true;
    const Case& c = cases_[caseId >= 0 ? caseId : 0];
    s.back = {2, caseId >= 0 ? c.backZ : c.box.hi.z, -1.0, true, {}, {}};
    s.back.lo[0] = c.box.lo.x;
    s.back.hi[0] = c.box.hi.x;
    s.back.lo[1] = c.box.lo.y;
    s.back.hi[1] = c.box.hi.y;
    s.back.lo[2] = -1e9;
    s.back.hi[2] = 1e9;
    s.hasBack = caseId >= 0 || facade;
    s.occl = std::clamp(occl, 0.0, 1.0);
    s.dirDark = dirDark;
    s.set = set;
    srcs_.push_back(s);
    return int(srcs_.size()) - 1;
  }
  int sources() const { return int(srcs_.size()); }

  // Every source's paths traced; the common delay, the classes' path splits,
  // their filters; the taps. (pipesN: the first pipesN sources are pipes'
  // mouths, which set the classes; the rest tops and regions.)
  void finish(int mouths) {
    std::vector<std::vector<Path>> ps[2];
    for (int m = 0; m < 2; ++m) ps[m].resize(srcs_.size());
    double rmin = 1e9;
    std::vector<double> r1, r2;
    for (size_t i = 0; i < srcs_.size(); ++i)
      for (int m = 0; m < 2; ++m) {
        ps[m][i] = paths(srcs_[i], lis_.mic[m]);
        for (const Path& p : ps[m][i]) {
          rmin = std::min(rmin, p.r);
          if (int(i) < mouths && m == 0) (p.order == 1 ? r1 : p.order == 2 ? r2 : r0_).push_back(p.r);
        }
      }
    if (r1.empty()) r1.push_back(20);
    if (r2.empty()) r2.push_back(30);
    std::sort(r1.begin(), r1.end());
    std::sort(r2.begin(), r2.end());
    rSplit_[1] = r1[r1.size() / 2];
    rSplit_[2] = r2[r2.size() / 2];
    d0_ = std::max(0, int(std::lround(rmin / kC * fs_)) - 4);
    double sum[kCls] = {}, cnt[kCls] = {};
    for (size_t i = 0; i < srcs_.size() && int(i) < mouths; ++i)
      for (const Path& p : ps[0][i]) {
        const int c = cls(p);
        sum[c] += p.r;
        cnt[c] += 1;
      }
    double clsR[kCls];
    for (int c = 0; c < kCls; ++c) clsR[c] = cnt[c] > 0 ? sum[c] / cnt[c] : 25;
    // The class filters: the image's band (research §7d.4, its top revised)
    // and the air of the class's mean path; a half / full shadow -4 / -8 dB
    // above 1.5 kHz.
    const double fo[6] = {1000, 2000, 4000, 8000, 12000, 16000};
    const double band1[6] = {-0.4, -0.4, -1.0, -3.0, -6.0, -10.0}, band2[6] = {-0.9, -0.7, -2.0, -6.0, -12.0, -20.0};
    for (int c = 0; c < kCls; ++c) {
      Chain ch;
      ch.fs = fs_;
      const int order = c == 0 ? 0 : c <= 2 ? 1 : 2;
      for (int k = 0; k < order; ++k) ch.s.push_back(hp1(300));
      double tgt[6];
      for (int q = 0; q < 6; ++q) {
        const double band = order == 0 ? 0 : order == 1 ? band1[q] : band2[q];
        tgt[q] = band - ch.magDb(fo[q]) - airDbPerM(fo[q]) * clsR[c];
      }
      fitTop(ch, fo, tgt, 6);
      chain_[c][0] = ch;
      Chain half = ch;
      half.s.push_back(shelf1(1500, -4));
      chain_[c][1] = half;
      ch.s.push_back(shelf1(1500, -8));
      chain_[c][2] = ch;
    }
    // The taps: each source's, per microphone.
    for (int m = 0; m < 2; ++m) taps_[m].assign(srcs_.size(), {});
    for (size_t i = 0; i < srcs_.size(); ++i)
      for (int m = 0; m < 2; ++m) {
        const auto& P = ps[m][i];
        double rm = 1e9;
        for (const Path& p : P) rm = std::min(rm, p.r);
        for (const Path& p : P) {
          if (p.r - rm > 0.080 * kC) continue;  // (later: the late field's)
          const int d = int(std::lround(p.r / kC * fs_)) - d0_;
          if (d < 0 || d >= kRingN - kBlock) continue;
          const double cosA = dot(p.arriveFrom, lis_.axis[m]);
          // (the direct sound PJ_ORGAN_DIRECT_DB over the physical balance, 4 dB: the stand's
          // pair nearer the case, as a recording's main pair is placed for clarity, against a
          // muffled wash of the reflections)
          static const double kDirect = std::pow(10.0, (std::getenv("PJ_ORGAN_DIRECT_DB") ? std::atof(std::getenv("PJ_ORGAN_DIRECT_DB")) : 4.0) / 20);
          const double g = std::pow(10.0, (p.reflDb - 3.0 * p.shadow - p.diffDb) / 20) / p.r * (lis_.pattern + (1 - lis_.pattern) * cosA) *
                           (cls(p) == 0 ? kDirect : 1.0);
          const Source& s = srcs_[i];
          // (bent round the edge, its harmonics more than its fundamental: darker)
          const double o = std::clamp(s.occl + s.dirDark * 0.5 * (1 - p.firstDir.z) + 0.7 * p.shadow + p.diffDb / 15, 0.0, 1.0);
          taps_[m][i].push_back({ringIndex(m, cls(p), int(std::lround(2 * o))), d, float(g)});
        }
      }
    for (RingSet& r : sets_) r.buf.assign(size_t(kRings) * kRingN, 0.0f);
    reset();
  }
  // How many taps a source has (both microphones): 0 if the church cannot
  // hear it directly (behind a wall: heard through its enclosure's field).
  int taps(int src) const { return int(taps_[0][src].size() + taps_[1][src].size()); }
  // (its taps' gains summed, by class: [0] direct, [1] the reflections; both microphones)
  std::array<double, 2> tapSum(int src) const {
    std::array<double, 2> r{};
    for (int m = 0; m < 2; ++m)
      for (const Tap& t : taps_[m][src]) r[t.ring / kDark % kCls == 0 ? 0 : 1] += t.g;
    return r;
  }
  // The first arrival's delay (samples, after the common delay) at a microphone.
  int firstDelay(int src, int mic) const {
    int d = 1 << 30;
    for (const Tap& t : taps_[mic][src]) d = std::min(d, t.delay);
    return d;
  }

  // One block of a source's sound (kBlock samples, its gain applied) to one
  // microphone's rings (the two microphones' rings apart: each on its own thread).
  void scatter(int src, int mic, const float* y) {
    RingSet& rs = sets_[srcs_[src].set];
    for (const Tap& t : taps_[mic][src]) {
      float* r = rs.ring(t.ring);
      const int p = (w_ + t.delay) & kRingMask;
      const float g = t.g;
      if (p + kBlock <= kRingN) {
        float* q = r + p;
        for (int i = 0; i < kBlock; ++i) q[i] += g * y[i];
      } else {
        for (int i = 0; i < kBlock; ++i) r[(p + i) & kRingMask] += g * y[i];
      }
    }
  }
  // A swell box's shutters for its ring set (as Enclosure::step: the two-pole
  // low-pass at its coefficient, its gain, the walls' leak).
  void setShutter(int set, float a, float gain, float leak) {
    shA_[set] = a;
    shG_[set] = gain;
    shLeak_[set] = leak;
  }
  // The block's end: each ring's block through its shutter (a swell's set)
  // and its class's filter, summed per microphone (one microphone: mic).
  void endBlock(int mic, float* out) {
    std::fill(out, out + kBlock, 0.0f);
    for (int c = 0; c < kCls; ++c)
      for (int d = 0; d < kDark; ++d) {
        const int rI = ringIndex(mic, c, d);
        float sum[kBlock] = {};
        for (int k = 0; k < kSets; ++k) {
          float* r = sets_[k].ring(rI) + w_;
          if (k == 0) {
            for (int i = 0; i < kBlock; ++i) sum[i] += r[i];
          } else {
            float& l1 = shL1_[k][rI];
            float& l2 = shL2_[k][rI];
            float& k1 = shK_[k][rI];
            for (int i = 0; i < kBlock; ++i) {
              const float x = r[i];
              l1 += shA_[k] * (x - l1);
              l2 += shA_[k] * (l1 - l2);
              k1 += 0.026f * (x - k1);
              sum[i] += shG_[k] * l2 + shLeak_[k] * k1;
            }
          }
          std::fill(r, r + kBlock, 0.0f);
        }
        ChainState& st = state_[mic][c][d];
        const Chain& ch = chain_[c][d];
        for (int i = 0; i < kBlock; ++i) out[i] += st.run(ch, sum[i]);
      }
  }
  // After both microphones' endBlock: the rings advance.
  void advance() { w_ = (w_ + kBlock) & kRingMask; }
  void reset() {
    for (RingSet& r : sets_) std::fill(r.buf.begin(), r.buf.end(), 0.0f);
    w_ = 0;
    for (auto& a : state_)
      for (int c = 0; c < kCls; ++c)
        for (int d = 0; d < kDark; ++d) a[c][d].init(chain_[c][d]);
    std::memset(shL1_, 0, sizeof shL1_);
    std::memset(shL2_, 0, sizeof shL2_);
    std::memset(shK_, 0, sizeof shK_);
  }

 private:
  static constexpr double kC = 343.2;  // (20 C)
  static constexpr double kNaveW = 20, kNaveH = 20, kNaveL = 50, kGalleryY = 7, kGalleryZ = 5.5;
  static constexpr int kRingN = 8192, kRingMask = kRingN - 1;
  static constexpr int kCls = 5;   // direct, 1st near, 1st far, 2nd near, 2nd far
  static constexpr int kDark = 3;  // bright, half (-4 dB above 1.5 kHz), dark (-8 dB)
  static constexpr int kRings = 2 * kCls * kDark;
  static int ringIndex(int mic, int cls, int dark) { return (mic * kCls + cls) * kDark + dark; }

  static V3 add(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
  static V3 sub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
  static V3 mul(double s, V3 a) { return {s * a.x, s * a.y, s * a.z}; }
  static double dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
  static double len(V3 a) { return std::sqrt(dot(a, a)); }
  static double get(const V3& v, int a) { return a == 0 ? v.x : a == 1 ? v.y : v.z; }
  static void set(V3& v, int a, double x) { (a == 0 ? v.x : a == 1 ? v.y : v.z) = x; }

  // ISO 9613-1: the air's absorption (dB/m) at 20 C, 58 % RH.
  static double airDbPerM(double f, double tC = 20, double rh = 58) {
    const double T = tC + 273.15, T0 = 293.15, T01 = 273.16;
    const double psat = std::pow(10.0, -6.8346 * std::pow(T01 / T, 1.261) + 4.6151);
    const double h = rh * psat;
    const double frO = 24 + 4.04e4 * h * (0.02 + h) / (0.391 + h);
    const double frN = std::pow(T / T0, -0.5) * (9 + 280 * h * std::exp(-4.170 * (std::pow(T / T0, -1.0 / 3) - 1)));
    const double f2 = f * f;
    return 8.686 * f2 *
           (1.84e-11 * std::sqrt(T / T0) +
            std::pow(T / T0, -2.5) * (0.01275 * std::exp(-2239.1 / T) / (frO + f2 / frO) + 0.1068 * std::exp(-3352.0 / T) / (frN + f2 / frN)));
  }
  // First-order sections (minimum phase).
  struct Sec {
    float b0 = 1, b1 = 0, a1 = 0;
    double magDb(double f, double fs) const {
      const std::complex<double> z1 = std::polar(1.0, -2 * M_PI * f / fs);
      return 20 * std::log10(std::abs((double(b0) + double(b1) * z1) / (1.0 + double(a1) * z1)));
    }
  };
  Sec hp1(double fc) const {
    const double K = std::tan(M_PI * fc / fs_);
    Sec s;
    s.b0 = float(1 / (1 + K));
    s.b1 = -s.b0;
    s.a1 = float((K - 1) / (K + 1));
    return s;
  }
  Sec lp1(double fc) const {
    const double K = std::tan(M_PI * std::min(fc, 0.45 * fs_) / fs_);
    Sec s;
    s.b0 = float(K / (1 + K));
    s.b1 = s.b0;
    s.a1 = float((K - 1) / (K + 1));
    return s;
  }
  Sec shelf1(double fc, double gDb) const {  // DC 1, the highs gDb
    const double G = std::pow(10.0, gDb / 20), K = std::tan(M_PI * fc / fs_);
    const double kz = K / std::sqrt(G), kp = K * std::sqrt(G), g = kp / kz;
    const double n0 = g * (1 + kz), n1 = g * (kz - 1), d0 = 1 + kp, d1 = kp - 1;
    Sec s;
    s.b0 = float(n0 / d0);
    s.b1 = float(n1 / d0);
    s.a1 = float(d1 / d0);
    return s;
  }
  struct Chain {
    std::vector<Sec> s;
    double fs = 48000;
    double magDb(double f) const {
      double d = 0;
      for (const Sec& x : s) d += x.magDb(f, fs);
      return d;
    }
  };
  struct ChainState {
    float x1[8] = {}, y1[8] = {};
    void init(const Chain&) {
      std::fill(x1, x1 + 8, 0.0f);
      std::fill(y1, y1 + 8, 0.0f);
    }
    float run(const Chain& c, float x) {
      for (size_t i = 0; i < c.s.size() && i < 8; ++i) {
        const Sec& q = c.s[i];
        const float y = q.b0 * x + q.b1 * x1[i] - q.a1 * y1[i];
        x1[i] = x;
        y1[i] = y;
        x = y;
      }
      return x;
    }
  };
  // The high end of a class: a one-pole low-pass and a shelf fitted to the target.
  void fitTop(Chain& c, const double* fOct, const double* target, int n) const {
    c.fs = fs_;
    double best = 1e30;
    Sec bl, bs;
    for (int i = 0; i < 40; ++i) {
      const double fl = 4000 * std::pow(23000 / 4000.0, i / 39.0);
      const Sec l = fl > 0.45 * fs_ ? Sec{} : lp1(fl);
      for (int j = 0; j < 24; ++j) {
        const double fsh = 2000 * std::pow(8.0, j / 23.0);
        if (fsh > 0.45 * fs_) continue;
        for (int k = 0; k <= 40; ++k) {
          const Sec s = shelf1(fsh, -20 + 0.5 * k);
          double e = 0;
          for (int q = 0; q < n; ++q) {
            if (fOct[q] > 0.45 * fs_) continue;
            const double d = l.magDb(fOct[q], fs_) + s.magDb(fOct[q], fs_) - target[q];
            e += d * d * (fOct[q] > 15000 ? 0.5 : 1);
          }
          if (e < best) {
            best = e;
            bl = l;
            bs = s;
          }
        }
      }
    }
    c.s.push_back(bl);
    c.s.push_back(bs);
  }

  struct Plane {
    int axis;
    double at;
    double reflDb;  // broadband, at 1 kHz
    bool finite;
    double lo[3], hi[3];
  };
  struct Case {  // solid but for its open front (z = hi.z) over the aperture's y range
    Box box;
    double apLoY, apHiY, backZ;
    bool shadowOnly;  // small: it shadows (diffraction), does not block
  };
  struct Listener {
    V3 mic[2], axis[2];
    double pattern = 0.6;
  };
  struct Source {
    V3 at;
    int caseId;
    bool hasBack;
    Plane back;
    double occl, dirDark;
    int set;
    double hz = 250;  // (its fundamental: the wavelength the gallery's edge bends)
  };
  struct Path {
    double r;
    int order;
    double reflDb;
    V3 firstDir, arriveFrom;
    int shadow = 0;
    double diffDb = 0;  // (bent round the gallery's front edge: its loss)
  };
  struct Tap {
    int ring, delay;
    float g;
  };
  struct RingSet {
    std::vector<float> buf;
    float* ring(int r) { return buf.data() + size_t(r) * kRingN; }
  };

  static Listener makeListener(double z, double x = 0, double y = 1.2, double spacing = 2, double angleDeg = 55) {
    Listener L;
    const double a = angleDeg * M_PI / 180;
    for (int m = 0; m < 2; ++m) {
      const double s = m ? 1 : -1;
      L.mic[m] = {x + s * spacing / 2, y, z};
      L.axis[m] = {s * std::sin(a), 0, -std::cos(a)};  // towards the organ (west), out to its side
    }
    return L;
  }
  // The walls: 70 % stone, 20 % windows, 10 % wood (-1.3 dB at 1 kHz); the
  // west wall -1.5; the vault -1.2; the floor with pews -4.2; the gallery's
  // floor (wood) -1.5 (research §7d.4).
  static std::vector<Plane> roomPlanes() {
    auto inf = [](int axis, double at, double db) {
      Plane p{axis, at, db, false, {-1e9, -1e9, -1e9}, {1e9, 1e9, 1e9}};
      return p;
    };
    std::vector<Plane> p = {inf(0, -kNaveW / 2, -1.3), inf(0, kNaveW / 2, -1.3), inf(2, 0, -1.5), inf(1, 0, -4.2), inf(1, kNaveH, -1.2)};
    Plane g = inf(1, kGalleryY, -1.5);
    g.finite = true;
    g.lo[2] = 0;
    g.hi[2] = kGalleryZ;
    p.push_back(g);
    return p;
  }
  static bool hits(const Box& bx, V3 a, V3 b) {  // the segment a -> b through the box's interior
    double t0 = 1e-4, t1 = 1 - 1e-4;
    for (int ax = 0; ax < 3; ++ax) {
      const double A = get(a, ax), B = get(b, ax), L = get(bx.lo, ax) + 1e-6, H = get(bx.hi, ax) - 1e-6, d = B - A;
      if (std::fabs(d) < 1e-12) {
        if (A <= L || A >= H) return false;
      } else {
        double u = (L - A) / d, v = (H - A) / d;
        if (u > v) std::swap(u, v);
        t0 = std::max(t0, u);
        t1 = std::min(t1, v);
        if (t0 >= t1) return false;
      }
    }
    return true;
  }
  static bool inside(const Box& bx, V3 p, double e) {
    return p.x > bx.lo.x - e && p.x < bx.hi.x + e && p.y > bx.lo.y - e && p.y < bx.hi.y + e && p.z > bx.lo.z - e && p.z < bx.hi.z + e;
  }
  static bool reflectPoint(V3 a, V3 b, const Plane& pl, V3& q) {
    const double A = get(a, pl.axis), B = get(b, pl.axis);
    if ((A - pl.at) * (B - pl.at) >= 0) return false;
    q = add(a, mul((pl.at - A) / (B - A), sub(b, a)));
    for (int ax = 0; ax < 3; ++ax)
      if (ax != pl.axis && (get(q, ax) < pl.lo[ax] || get(q, ax) > pl.hi[ax])) return false;
    return true;
  }
  static V3 mirror(V3 p, const Plane& pl) {
    set(p, pl.axis, 2 * pl.at - get(p, pl.axis));
    return p;
  }
  int cls(const Path& p) const { return p.order == 0 ? 0 : p.order == 1 ? (p.r < rSplit_[1] ? 1 : 2) : (p.r < rSplit_[2] ? 3 : 4); }

  // Every valid path from a source to a microphone, to second order.
  std::vector<Path> paths(const Source& s, V3 mic) const {
    std::vector<Plane> planes = planes_;
    if (s.hasBack) planes.push_back(s.back);
    std::vector<Path> out;
    const int np = int(planes.size());
    int shadow = 0;
    double diff = 0;
    // The gallery's front edge: a leg past it, from the gallery to the nave below or above, bent
    // round it (Kurze-Anderson's form of Maekawa's chart, at the source's fundamental: N = 2 delta /
    // lambda, delta the detour over the edge; in its shadow 5 + 20 log10(x / tanh x) dB, x =
    // sqrt(2 pi N), 5 dB on the line of sight; on the lit side within N = -0.2, 5 + 20 log10(x /
    // tan x), falling to nothing). A pedal pipe's wavelength of 3-5 m bends round it: a pipe a
    // little below the line of sight a few dB softer, not cut off. (Beyond 30 dB: blocked.)
    auto edge = [&](V3 a, V3 b, bool blocked) -> double {
      const double Z = kGalleryZ;
      if ((a.z - Z) * (b.z - Z) >= 0) return blocked ? -1 : 0;
      const V3 on = a.z < Z ? a : b;  // (the end over the gallery)
      const double t = (Z - a.z) / (b.z - a.z);
      const V3 q = add(a, mul(t, sub(b, a)));
      const double ey = on.y >= kGalleryY - 0.15 ? kGalleryY : kGalleryY - 0.3;
      if (!blocked && std::fabs(q.y - ey) > 3) return 0;  // (well clear)
      const V3 E{q.x, ey, Z};
      const double delta = len(sub(E, a)) + len(sub(b, E)) - len(sub(b, a));
      const double N = 2 * delta * s.hz / 343.0;
      if (blocked) {
        const double x = std::sqrt(2 * M_PI * N);
        const double A = 5 + 20 * std::log10(x > 1e-6 ? x / std::tanh(x) : 1.0);
        return A > 30 ? -1 : A;
      }
      // (lit: passing on the open side of the edge)
      const bool openSide = ey == kGalleryY ? q.y > ey : q.y < ey;
      if (!openSide || N >= 0.2) return 0;
      const double x = std::sqrt(2 * M_PI * N);
      return std::max(0.0, 5 + 20 * std::log10(x > 1e-6 ? x / std::tan(x) : 1.0));
    };
    // The same at a case's edges (its four upright corners and its top's four): the least detour
    // round one (closed form: the edge unfolded into the leg's plane), against the pipe's
    // wavelength; blocked, the shadow's loss; past it in the open within N = 0.2, the lit side's.
    auto caseEdge = [&](const Box& bx, V3 a, V3 b, bool blocked) -> double {
      const double ab = len(sub(b, a));
      double best = 1e9;
      auto tryEdge = [&](int ax, double u0, double v0, double lo, double hi) {
        // (an edge along axis ax, at the other two coordinates (u0, v0))
        const int au = (ax + 1) % 3, av = (ax + 2) % 3;
        const double da = std::hypot(get(a, au) - u0, get(a, av) - v0), db = std::hypot(get(b, au) - u0, get(b, av) - v0);
        if (da + db < 1e-9) return;
        const double A = get(a, ax), B = get(b, ax);
        const double w = std::clamp(A + (B - A) * da / (da + db), lo, hi);
        if (!blocked) {  // (lit: only an edge the leg passes beside, between its ends)
          const double t = da / (da + db);
          if (t < 0.05 || t > 0.95) return;
        }
        best = std::min(best, std::hypot(da, w - A) + std::hypot(db, B - w) - ab);
      };
      for (double x : {bx.lo.x, bx.hi.x})
        for (double z : {bx.lo.z, bx.hi.z}) tryEdge(1, z, x, bx.lo.y, bx.hi.y);  // (upright: along y; (z, x))
      for (double z : {bx.lo.z, bx.hi.z}) tryEdge(0, bx.hi.y, z, bx.lo.x, bx.hi.x);  // (the top's, along x: (y, z))
      for (double x : {bx.lo.x, bx.hi.x}) tryEdge(2, x, bx.hi.y, bx.lo.z, bx.hi.z);  // (along z: (x, y))
      if (best > 1e8) return blocked ? -1 : 0;
      const double N = 2 * std::max(best, 0.0) * s.hz / 343.0;
      const double x = std::sqrt(2 * M_PI * N);
      if (blocked) {
        const double A = 5 + 20 * std::log10(x > 1e-6 ? x / std::tanh(x) : 1.0);
        return A > 30 ? -1 : A;
      }
      if (N >= 0.2) return 0;
      return std::max(0.0, 5 + 20 * std::log10(x > 1e-6 ? x / std::tan(x) : 1.0));
    };
    // (a leg's losses: each edge it is bent round, summed; of the edges it passes in the open, the
    // nearest alone: the lit side's loss is one edge's)
    auto legOk = [&](V3 a, V3 b, bool& exited) {
      double lit = 0;
      {
        const bool blocked = hits(gallery_, a, b);  // (the gallery's floor, a slab)
        const double A = edge(a, b, blocked);
        if (A < 0) return false;
        if (blocked) diff += A;
        else lit = std::max(lit, A);
      }
      for (int k = 0; k < int(cases_.size()); ++k) {
        const Case& c = cases_[k];
        if (k == s.caseId && !exited) {
          // (inside its own case: the leg leaves through the open front, or stays inside)
          if (inside(c.box, b, 1e-3)) return true;
          const double A = a.z, B = b.z, fz = c.box.hi.z;
          if (B <= fz) return false;
          const V3 q = add(a, mul((fz - A) / (B - A), sub(b, a)));
          if (q.x < c.box.lo.x || q.x > c.box.hi.x) {
            // (out through a side wall: bent round the open front's upright edge instead)
            const double D = caseEdge(c.box, a, b, true);
            if (D < 0) return false;
            diff += D;
            exited = true;
            continue;
          }
          // (past the aperture's edges, the impost, the roof: diffracted within 0.6 m)
          const double past = std::max(c.apLoY - q.y, q.y - c.apHiY);
          if (past > 0.6) return false;
          if (past > 0) shadow += past > 0.3 ? 2 : 1;
          exited = true;
          continue;
        }
        const bool hit = hits(c.box, a, b);
        if (hit && c.shadowOnly) {
          ++shadow;
          continue;
        }
        if (k == s.caseId && hit) return false;  // (back into its own case)
        if (k == s.caseId) continue;
        const double A = caseEdge(c.box, a, b, hit);  // (round another case's edge)
        if (A < 0) return false;
        if (hit) diff += A;
        else lit = std::max(lit, A);
      }
      diff += lit;
      return true;
    };
    std::vector<std::vector<int>> seqs = {{}};
    for (int i = 0; i < np; ++i) seqs.push_back({i});
    for (int i = 0; i < np; ++i)
      for (int j = 0; j < np; ++j)
        if (i != j && (planes[i].axis != planes[j].axis || planes[i].at != planes[j].at)) seqs.push_back({i, j});
    for (const auto& seq : seqs) {
      std::vector<V3> img = {s.at};
      for (int k : seq) img.push_back(mirror(img.back(), planes[k]));
      std::vector<V3> pts(seq.size() + 2);
      pts.back() = mic;
      pts[0] = s.at;
      bool ok = true;
      V3 from = mic;
      for (int k = int(seq.size()) - 1; k >= 0 && ok; --k) {
        V3 q;
        if (!reflectPoint(from, img[k + 1], planes[seq[k]], q)) ok = false;
        if (ok && (q.x < -kNaveW / 2 - 1e-6 || q.x > kNaveW / 2 + 1e-6 || q.y < -1e-6 || q.y > kNaveH + 1e-6 || q.z < -1e-6 || q.z > kNaveL))
          ok = false;
        pts[k + 1] = q;
        from = q;
      }
      if (!ok) continue;
      bool exited = s.caseId < 0;
      shadow = 0;
      diff = 0;
      for (size_t k = 0; k + 1 < pts.size() && ok; ++k) ok = legOk(pts[k], pts[k + 1], exited);
      if (diff > 30) ok = false;
      if (!ok || !exited) continue;
      Path p;
      p.r = 0;
      for (size_t k = 0; k + 1 < pts.size(); ++k) p.r += len(sub(pts[k + 1], pts[k]));
      p.order = int(seq.size());
      p.reflDb = 0;
      for (int k : seq) p.reflDb += planes[k].reflDb;
      p.firstDir = mul(1 / len(sub(pts[1], pts[0])), sub(pts[1], pts[0]));
      const V3 back = sub(pts[pts.size() - 2], mic);
      p.arriveFrom = mul(1 / len(back), back);
      p.shadow = shadow;
      p.diffDb = diff;
      out.push_back(p);
    }
    return out;
  }

  double fs_ = 48000;
  Listener lis_;
  std::vector<Plane> planes_;
  Box gallery_;
  std::vector<Case> cases_;
  std::vector<Source> srcs_;
  std::vector<double> r0_;
  double rSplit_[3] = {0, 20, 30};
  int d0_ = 0, w_ = 0;
  std::vector<std::vector<Tap>> taps_[2];
  RingSet sets_[kSets];
  Chain chain_[kCls][kDark];
  ChainState state_[2][kCls][kDark];
  float shA_[kSets] = {1, 1, 1}, shG_[kSets] = {1, 1, 1}, shLeak_[kSets] = {};
  float shL1_[kSets][kRings] = {}, shL2_[kSets][kRings] = {}, shK_[kSets][kRings] = {};
};

}  // namespace pajupilli
