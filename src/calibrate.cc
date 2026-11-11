#include "calibrate.h"

#include <algorithm>
#include <atomic>
#include <thread>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

extern char** environ;

#include "analysis.h"
#include "flute.h"
#include "player.h"
#include "voice.h"

namespace pajupilli {
namespace {

constexpr int kFs = 48000;  // Voice output

// Plays the note from silence; returns the error in cents (NAN if it sounds in
// the wrong register or not at all). wobble: level peak-to-peak / mean over the
// settled part (a pulsing, two-frequency state is not a usable note).
double tryNote(const Instrument& inst, int midi, double cover, double press, double a4, double dyn = 0.5,
               double jet = 1, double* wobble = nullptr) {
  Voice v(inst);
  Player& pl = v.player();
  pl.coverTrim[midi] = pl.coverLoTrim[midi] = pl.coverHiTrim[midi] = cover;
  pl.pressTrim[midi] = press;
  pl.jetTrim[midi] = jet;
  pl.setDynamics(dyn);
  pl.setWander(0);  // learn on a steady player
  // Low notes build up over a number of periods: listen longer (bass C3: 2x).
  const double f0 = noteFrequency(midi, a4);
  const double slow = std::max(1.0, 261.6 / f0);
  const int n = int((wobble ? 0.5 : 0.35) * slow * kFs);
  std::vector<float> x(n);
  pl.noteOn(midi, 64);
  v.render(x.data(), n);
  const int a = wobble ? int(0.2 * slow * kFs) : n / 2;
  if (rms(x.data() + a, n - a) < 1e-3) return NAN;
  double clarity = 0;
  const double f = estimatePitch(x.data() + a, n - a, kFs, f0 / 2.2, f0 * 2.2, &clarity);
  const double c = 1200 * std::log2(f / f0);
  if (wobble) {
    double lmin = 1e9, lmax = 0, lm = 0;
    int k = 0;
    const int fr = int(std::max(0.02, 4 / f0) * kFs);  // >= 4 periods (bass)
    for (int i = a; i + fr <= n; i += fr, ++k) {
      const double r = rms(x.data() + i, fr);
      lmin = std::min(lmin, r);
      lmax = std::max(lmax, r);
      lm += r;
    }
    *wobble = k ? (lmax - lmin) / (lm / k) : 1;
  }
  return std::fabs(c) < 150 && clarity > 0.6 ? c : NAN;
}

constexpr double kSteady = 0.2;  // level wobble above this: not a usable state

}  // namespace

// One note: for each breath pressure that sounds the right register, bisect
// the coverage (more coverage = flatter); keep the best. Gentler breath also
// flattens, so pressure is the second tuning control.
// Bisect the coverage (log scale, more coverage = flatter) for one pressure and
// dynamics, starting from x0. Returns the best |cents| found (NAN: never sounded)
// and its coverage.
static double bisectCover(const Instrument& inst, int m, double p, double dyn, double jet, double a4, double x0, double* cover) {
  const double f = noteFrequency(m - inst.transpose);  // the lips go by the written note
  const double t = std::clamp(std::log(f / 261.6) / std::log(1760 / 261.6), 0.0, 1.2);
  const double base = 6.0 - 2.5 * t;  // Coltman's uncovered width (concert-flute mm)
  const double cmin = 2.0 / base, cmax = 10.0 / base;
  double xg = x0, c = tryNote(inst, m, std::exp(x0), p, a4, dyn, jet);
  if (std::isnan(c)) return NAN;
  double a = std::log(cmin), b = std::log(cmax), bx = x0, bc = c;
  for (int it = 0; it < 10 && std::fabs(bc) > 2; ++it) {
    if (c > 0) b = xg; else a = xg;
    const double x = 0.5 * (a + b);
    const double cx = tryNote(inst, m, std::exp(x), p, a4, dyn, jet);
    if (std::isnan(cx)) {  // lost the register: x is a limit
      if (x < xg) a = x; else b = x;
      continue;
    }
    xg = x;
    c = cx;
    if (std::fabs(c) < std::fabs(bc)) { bx = x; bc = c; }
  }
  *cover = std::exp(bx);
  return bc;
}

// One note: for each breath pressure that sounds the right register, bisect
// the coverage; keep the best. Gentler breath also flattens, so pressure is the
// second tuning control. Then: the softest dynamics that still holds the
// register, and the coverage at soft and loud.
static void calibrateNote(const Instrument& inst, int m, double a4, Calibration* cal) {
  // Jet length: the player moves the lips closer to or further from the edge
  // until the note is steady (a note can sit where its harmonics beat against
  // a nearby resonance, e.g. D6 at Fletcher's length).
  double bestAbs = 1e9, bestCover = 1, bestPress = 1, bestJet = 1, bestC = NAN, bestWob = 1e9;
  for (double jet : {1.0, 0.9, 1.1, 0.8}) {
    double jAbs = 1e9, jCover = 1, jPress = 1, jC = NAN;
    for (double p : {1.0, 1.2, 0.85, 1.45, 0.7, 1.7, 0.6, 2.0}) {
      double cv = 1;
      const double bc = bisectCover(inst, m, p, 0.5, jet, a4, 0, &cv);
      if (std::isnan(bc)) continue;
      if (std::fabs(bc) < jAbs) { jAbs = std::fabs(bc); jCover = cv; jPress = p; jC = bc; }
      if (jAbs < 3) break;
    }
    if (std::isnan(jC)) continue;
    double wob = 1;
    const double cc = tryNote(inst, m, jCover, jPress, a4, 0.5, jet, &wob);
    if (std::isnan(cc)) continue;
    // Prefer steady, then in tune.
    const bool better = (wob < kSteady) != (bestWob < kSteady) ? wob < kSteady
                        : wob < kSteady ? jAbs < bestAbs : wob < bestWob;
    if (better) { bestAbs = jAbs; bestCover = jCover; bestPress = jPress; bestJet = jet; bestC = jC; bestWob = wob; }
    if (bestWob < kSteady && bestAbs < 3) break;
  }
  if (std::isnan(bestC)) return;
  cal->cover[m] = cal->coverLo[m] = cal->coverHi[m] = bestCover;
  cal->press[m] = bestPress;
  cal->jet[m] = bestJet;
  cal->cents[m] = float(bestC);
  cal->ok[m] = true;
  // Softest dynamics: every step from there up to mezzo-forte must hold the
  // register and be steady; a little margin for slurs.
  double floor = 0.5;
  for (double d = 0.4; d >= -1e-9; d -= 0.1) {
    double wob = 1;
    if (std::isnan(tryNote(inst, m, bestCover, bestPress, a4, std::max(0.0, d), bestJet, &wob)) ||
        wob > kSteady) break;
    floor = std::max(0.0, d);
  }
  cal->dynLo[m] = floor > 0 ? std::min(0.5, floor + 0.05) : 0.0;
  const double lo = std::max(cal->dynLo[m], 0.15);
  // Lips move only so far (Coltman: about +-1.5 mm); the rest of the pitch
  // change with breath stays, as on a real flute (loud a little sharp).
  auto limit = [bestCover](double cv) { return std::clamp(cv, 0.75 * bestCover, 1.35 * bestCover); };
  double cv = bestCover;
  if (!std::isnan(bisectCover(inst, m, bestPress, lo, bestJet, a4, std::log(bestCover), &cv))) cal->coverLo[m] = limit(cv);
  cv = bestCover;
  if (!std::isnan(bisectCover(inst, m, bestPress, kCalDynHi, bestJet, a4, std::log(bestCover), &cv))) cal->coverHi[m] = limit(cv);
}

static Calibration learn(const Instrument& inst, double a4, int lo, int hi) {
  Calibration cal;
  // Notes are independent: learn them in parallel.
  std::atomic<int> next{lo};
  const int nThreads = std::max(1, std::min(int(std::thread::hardware_concurrency()), hi - lo + 1));
  std::vector<std::thread> pool;
  for (int t = 0; t < nThreads; ++t)
    pool.emplace_back([&] {
      for (int m; (m = next.fetch_add(1)) <= hi;) calibrateNote(inst, m, a4, &cal);
    });
  for (std::thread& th : pool) th.join();
  return cal;
}

static void logCalibration(const Instrument& inst, const Calibration& cal, FILE* log, int lo, int hi) {
  for (int m = lo; m <= hi; ++m) {
    if (!cal.ok[m]) {
      std::fprintf(log, "calibrate: MIDI %d does not sound in its register\n", m);
      continue;
    }
    const double f = noteFrequency(m - inst.transpose);
    const double t = std::clamp(std::log(f / 261.6) / std::log(1760 / 261.6), 0.0, 1.2);
    const double mm = (6.0 - 2.5 * t) * inst.emb;
    std::fprintf(log, "calibrate: MIDI %d uncovered %.1f mm, pressure x%.2f, %+.0f cents; "
                 "jet x%.1f; softest dynamics %.2f; uncovered soft %.1f, loud %.1f mm\n", m,
                 mm * cal.cover[m], cal.press[m], cal.cents[m], cal.jet[m], cal.dynLo[m],
                 mm * cal.coverLo[m], mm * cal.coverHi[m]);
  }
}

// A calibration depends only on this binary and the PJ_* knobs: keep it in
// ~/.cache/pajupilli keyed on both, so live startup and make test skip the ~1 s
// of learning. PJ_NOCALCACHE=1 bypasses it.
static std::string cachePath(const Instrument& inst, double a4, int lo, int hi) {
  const char* home = std::getenv("HOME");
  struct stat st;
  if (std::getenv("PJ_NOCALCACHE") || !home || stat("/proc/self/exe", &st) != 0) return "";
  uint64_t h = 1469598103934665603ull;  // FNV-1a
  auto mix = [&h](const void* d, size_t n) {
    for (size_t i = 0; i < n; ++i) h = (h ^ static_cast<const unsigned char*>(d)[i]) * 1099511628211ull;
  };
  mix(&st.st_size, sizeof st.st_size);
  mix(&st.st_mtime, sizeof st.st_mtime);
  std::vector<std::string> env;
  for (char** e = environ; *e; ++e)
    if (std::strncmp(*e, "PJ_", 3) == 0) env.emplace_back(*e);
  std::sort(env.begin(), env.end());
  for (const std::string& e : env) mix(e.data(), e.size() + 1);
  mix(inst.name, std::strlen(inst.name) + 1);
  mix(&a4, sizeof a4);
  mix(&lo, sizeof lo);
  mix(&hi, sizeof hi);
  const std::string cache = std::string(home) + "/.cache", dir = cache + "/pajupilli";
  mkdir(cache.c_str(), 0755);
  mkdir(dir.c_str(), 0755);
  char name[40];
  std::snprintf(name, sizeof name, "/cal-%016llx.bin", static_cast<unsigned long long>(h));
  return dir + name;
}

Calibration calibrate(const Instrument& inst, double a4, FILE* log, int lo, int hi) {
  if (lo < 0) lo = inst.lo + inst.transpose;
  if (hi < 0) hi = inst.hi + inst.transpose;
  const std::string path = cachePath(inst, a4, lo, hi);
  Calibration cal;
  bool cached = false;
  if (!path.empty())
    if (FILE* f = std::fopen(path.c_str(), "rb")) {
      cached = std::fread(&cal, sizeof cal, 1, f) == 1;
      std::fclose(f);
    }
  if (!cached) {
    cal = learn(inst, a4, lo, hi);
    if (!path.empty()) {
      const std::string tmp = path + ".tmp" + std::to_string(getpid());
      if (FILE* f = std::fopen(tmp.c_str(), "wb")) {
        const bool ok = std::fwrite(&cal, sizeof cal, 1, f) == 1;
        std::fclose(f);
        if (ok) std::rename(tmp.c_str(), path.c_str());
        else std::remove(tmp.c_str());
      }
    }
  }
  if (log) logCalibration(inst, cal, log, lo, hi);
  return cal;
}

}  // namespace pajupilli
