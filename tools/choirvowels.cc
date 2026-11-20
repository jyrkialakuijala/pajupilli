// Fits the choir's sung vowel rows ([u], [ʊ], [ɔ], [ɑ]) on the Singer's own
// line (src/singer.cc: 44 sections, 3 substeps, its losses and lip load), in
// the parametrisation of its kVowels rows, toward sung resonance targets
// (research/choir.md 2.1, 6.1, 9.2), for a male (17.5 cm) and a female
// (15.0 cm) tract. The method is puhuja's linefit (~/puhuja/linefit.cc): the
// formants are the lowest peaks of the line's impulse response, the shape is
// searched by Nelder-Mead in log areas.
//
//   g++ -O2 -std=c++17 -Isrc tools/choirvowels.cc -o choirvowels
//   ./choirvowels                 # fit and print
//   ./choirvowels -emit FILE      # ... and write the header (src/singer_vowels_data.h)
//   ./choirvowels -measure        # only measure the rows in src/singer_vowels_data.h
//
// The impulse response is the research note's (1.3): Singer::control with
// f0 0.01 Hz and no lung pressure (the folds closed but for the chink), a
// pressure kick into the first section, the lip flow over 80 ms (the
// radiated dU/dt divided by f).
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#define private public
#include "../src/singer.cc"
#undef private

#if __has_include("../src/singer_vowels_data.h")
#include "../src/singer_vowels_data.h"
#define HAVE_SUNG 1
#endif

using namespace pajupilli;

namespace {

constexpr double kFs = 48000;

// The 9 numbers of a row in the Singer's order.
using Row = VowelRow;

// The Singer's line with the shape of a row at the length len (cm):
// sections exactly as singer.cc's Table builds them (the shape over the row's
// own length, the line at len).
std::vector<double> response(const Row& v, double len, int n) {
  Singer s;
  s.init(kFs, 1, 0);
  s.control(0.01, 2, 0, 0.2, 0, 5, 60, 0.01);
  const double dx = v.len / Singer::kN;
  for (int i = 0; i < Singer::kN; ++i) {
    double a = 0;
    for (int j = 0; j < 8; ++j) a += std::exp(lnArea(v, (i + (j + 0.5) / 8) * dx));
    s.lnA_[i] = s.lnT_[i] = float(std::log(a / 8 * 1e-4));
  }
  s.len_ = s.lenTarget_ = 0.01 * len;
  s.coefs();
  std::fill(s.p_, s.p_ + Singer::kN, 0.0f);
  std::fill(s.u_, s.u_ + Singer::kN + 1, 0.0f);
  s.iL_ = s.uLipPrev_ = 0;
  s.phase_ = 0;
  s.kick(1.0);
  std::vector<double> y(n);
  for (int i = 0; i < n; ++i) {
    s.sample();
    y[i] = s.uLipPrev_;  // the lip flow (the substeps' mean): no radiation tilt
  }
  return y;
}

// Formants: the lowest peaks of the smoothed log spectrum (5 Hz steps,
// 100-5000 Hz, Hann window), at least 150 Hz apart (puhuja linefit).
std::vector<double> formants(const Row& v, double len, int want = 3) {
  const int n = int(0.08 * kFs);
  const std::vector<double> y = response(v, len, n);
  const double df = 5;
  const int nb = int(4900 / df) + 1;
  std::vector<double> mag(nb), yw(n);
  for (int i = 0; i < n; ++i) yw[i] = y[i] * (0.5 - 0.5 * std::cos(2 * M_PI * i / (n - 1)));
  for (int b = 0; b < nb; ++b) {
    const double w = 2 * M_PI * (100 + b * df) / kFs, cs = std::cos(w), sn = std::sin(w);
    double re = 0, im = 0, c = 1, s = 0;
    for (int i = 0; i < n; ++i) {
      re += yw[i] * c;
      im += yw[i] * s;
      const double c2 = c * cs - s * sn;
      s = s * cs + c * sn;
      c = c2;
    }
    mag[b] = std::log(re * re + im * im + 1e-60);
  }
  std::vector<double> sm(nb);
  for (int b = 0; b < nb; ++b) {
    double acc = 0;
    int cnt = 0;
    for (int k = -4; k <= 4; ++k)
      if (b + k >= 0 && b + k < nb) acc += mag[b + k], ++cnt;
    sm[b] = acc / cnt;
  }
  std::vector<double> out;
  for (int b = 2; b + 2 < nb && int(out.size()) < want; ++b) {
    if (sm[b] > sm[b - 1] && sm[b] >= sm[b + 1] && sm[b] > sm[b - 2] && sm[b] > sm[b + 2]) {
      const double d = 0.5 * (sm[b - 1] - sm[b + 1]) / (sm[b - 1] - 2 * sm[b] + sm[b + 1] + 1e-12);
      const double f = 100 + (b + d) * df;
      if (!out.empty() && f - out.back() < 150) {
        if (sm[b] > sm[int((out.back() - 100) / df)]) out.back() = f;
        continue;
      }
      out.push_back(f);
    }
  }
  while (int(out.size()) < want) out.push_back(0);
  return out;
}

struct Fit {
  const char* name;
  int start;          // the puhuja row to start from (index into kStart)
  double t[3];        // F1-F3 targets (Hz)
};

// puhuja's newer rows (puhuja.cc kLineVowels, 2026-11-04..09): u (4), ʊ (11),
// ɔ (12), ɑ (14).
const Row kStart[4] = {
    {0.31664242, 1.6350453, 1.61300569, 1.5114838, 12.107744, 2.6006039, 1.64632684, 0.6872507, 17.753508},  // u
    {0.5771, 1.413307, 1.2346811, 2.2046674, 3.906, 5.95411845, 1.4986856, 0.7425773, 17.515786},           // ʊ
    {0.40210719, 1.121, 4.320978, 2.616, 3.475302, 8.894092, 2.157, 2.4461907, 17.54159},                  // ɔ
    {0.6497856, 3.273415, 1.17150003, 3.578, 0.49922525, 8.4512574, 0.46970746, 0.6166946, 17.51},          // ɑ
};

// Sung targets (research/choir.md):
//  male  u 300/800/2250 (choral [u] 300/750-900, 2.1; puhuja's u 300/820/2250),
//        ʊ 440/1000/2350 (puhuja ʊ 462/1039),
//        ɔ 520/880/2550 (HSW11 "hoard" sung: T 545/900, Bar 495/880),
//        ɑ 730/1200/2500 (HSW11 "hard": T 700/1175, Bar 785/1235; choral F2 1100-1300, 2.5).
//  female u 360/900/2600 (choral [u] women 350-400/800-1000),
//        ʊ 490/1120/2600,
//        ɔ 570/980/2750 (HSW11 "hoard": S 585/1065, A 515/825),
//        ɑ 820/1280/2800 (HSW11 "hard": S 825/1340, A 830/1160).
const Fit kMaleFits[4] = {{"u", 0, {300, 800, 2250}}, {"ʊ", 1, {440, 1000, 2350}},
                          {"ɔ", 2, {520, 880, 2550}}, {"ɑ", 3, {730, 1200, 2500}}};
const Fit kFemaleFits[4] = {{"u", 0, {360, 900, 2600}}, {"ʊ", 1, {490, 1120, 2600}},
                            {"ɔ", 2, {570, 980, 2750}}, {"ɑ", 3, {820, 1280, 2800}}};

// Search variables (log areas) <-> a row; the row's own length is fixed (the
// voice's), so the shape scales with it.
constexpr int kDim = 8;
Row rowOf(const double* x, double len) {
  Row r;
  r.xc = std::clamp(x[0], 0.12, 0.95);
  r.ac = std::exp(x[1]);
  r.wc = std::clamp(std::exp(x[2]), 0.8, 6.0);
  r.aLip = std::exp(x[3]);
  r.aPh = std::exp(x[4]);
  r.aOr = std::exp(x[5]);
  r.lLip = std::clamp(x[6], 0.3, 3.0);
  r.aLar = std::clamp(std::exp(x[7]), 0.3, 3.0);
  r.len = len;
  return r;
}
void xOf(const Row& r, double* x) {
  x[0] = r.xc, x[1] = std::log(r.ac), x[2] = std::log(r.wc), x[3] = std::log(r.aLip);
  x[4] = std::log(r.aPh), x[5] = std::log(r.aOr), x[6] = r.lLip, x[7] = std::log(r.aLar);
}

double cost(const double* x, double len, const double* t) {
  const Row r = rowOf(x, len);
  const std::vector<double> f = formants(r, len);
  static const double kW[3] = {1.0, 1.0, 0.4};
  double c = 0;
  for (int k = 0; k < 3; ++k) {
    if (f[k] <= 0) return 10;
    const double d = std::log(f[k] / t[k]);
    c += kW[k] * d * d;
  }
  // Plausible shapes: areas 0.15-14 cm^2, a mild pull toward 3 cm^2; the lips
  // of a rounded vowel no smaller than 0.3 cm^2 (a sung tone needs the lips
  // open enough to radiate).
  for (int k = 1; k < 6; ++k) {
    if (k != 2) c += 0.001 * (x[k] - std::log(3.0)) * (x[k] - std::log(3.0));
    if (std::exp(x[k]) < 0.15) c += 0.5 * (std::log(0.15) - x[k]);
    if (std::exp(x[k]) > 14) c += 0.5 * (x[k] - std::log(14.0));
  }
  if (std::exp(x[3]) < 0.3) c += 0.5 * (std::log(0.3) - x[3]);
  if (x[0] < 0.12 || x[0] > 0.95) c += 1;
  if (x[6] < 0.3 || x[6] > 3) c += 0.5;
  return c;
}

std::vector<double> nelderMead(std::vector<double> x0, const std::vector<double>& step, int evals, double len,
                               const double* t, double* best) {
  const int n = int(x0.size());
  std::vector<std::vector<double>> pts(n + 1, x0);
  std::vector<double> fv(n + 1);
  for (int i = 1; i <= n; ++i) pts[i][i - 1] += step[i - 1];
  for (int i = 0; i <= n; ++i) fv[i] = cost(pts[i].data(), len, t);
  int used = n + 1;
  while (used < evals) {
    std::vector<int> idx(n + 1);
    for (int i = 0; i <= n; ++i) idx[i] = i;
    std::sort(idx.begin(), idx.end(), [&](int a, int b) { return fv[a] < fv[b]; });
    std::vector<std::vector<double>> P(n + 1);
    std::vector<double> F(n + 1);
    for (int i = 0; i <= n; ++i) P[i] = pts[idx[i]], F[i] = fv[idx[i]];
    pts = P, fv = F;
    std::vector<double> c(n, 0);
    for (int i = 0; i < n; ++i)
      for (int k = 0; k < n; ++k) c[k] += pts[i][k] / n;
    auto along = [&](double a) {
      std::vector<double> y(n);
      for (int k = 0; k < n; ++k) y[k] = c[k] + a * (pts[n][k] - c[k]);
      return y;
    };
    std::vector<double> xr = along(-1);
    const double fr = cost(xr.data(), len, t);
    ++used;
    if (fr < fv[0]) {
      std::vector<double> xe = along(-2);
      const double fe = cost(xe.data(), len, t);
      ++used;
      if (fe < fr) pts[n] = xe, fv[n] = fe;
      else pts[n] = xr, fv[n] = fr;
    } else if (fr < fv[n - 1]) {
      pts[n] = xr, fv[n] = fr;
    } else {
      std::vector<double> xc = along(fr < fv[n] ? -0.5 : 0.5);
      const double fc = cost(xc.data(), len, t);
      ++used;
      if (fc < std::min(fr, fv[n])) {
        pts[n] = xc, fv[n] = fc;
      } else {
        for (int i = 1; i <= n; ++i) {
          for (int k = 0; k < n; ++k) pts[i][k] = pts[0][k] + 0.5 * (pts[i][k] - pts[0][k]);
          fv[i] = cost(pts[i].data(), len, t);
          ++used;
        }
      }
    }
  }
  int b = 0;
  for (int i = 1; i <= n; ++i)
    if (fv[i] < fv[b]) b = i;
  *best = fv[b];
  return pts[b];
}

Row fit(const Row& start, double len, const double* t, double* bestCost) {
  std::vector<double> x(kDim);
  xOf(start, x.data());
  const std::vector<double> step = {0.08, 0.3, 0.3, 0.3, 0.3, 0.3, 0.4, 0.3};
  double best = 1e9;
  std::vector<double> bx = x;
  // Three restarts from the best so far (Nelder-Mead stalls).
  for (int r = 0; r < 3; ++r) {
    double c;
    std::vector<double> y = nelderMead(bx, step, 700, len, t, &c);
    if (c < best) best = c, bx = y;
  }
  *bestCost = best;
  return rowOf(bx.data(), len);
}

void printRow(FILE* f, const Row& r, const char* comment) {
  std::fprintf(f, "    {%.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.4f, %.2f},  // %s\n", r.xc, r.ac, r.wc, r.aLip,
               r.aPh, r.aOr, r.lLip, r.aLar, r.len, comment);
}

std::string fstr(const Row& r, double len) {
  const std::vector<double> f = formants(r, len);
  char b[64];
  std::snprintf(b, sizeof b, "%.0f/%.0f/%.0f", f[0], f[1], f[2]);
  return b;
}

}  // namespace

int main(int argc, char** argv) {
  const char* emit = nullptr;
  bool measureOnly = false;
  for (int i = 1; i < argc; ++i) {
    if (!std::strcmp(argv[i], "-emit") && i + 1 < argc) emit = argv[++i];
    else if (!std::strcmp(argv[i], "-measure")) measureOnly = true;
  }
  const double lenM = 17.5, lenF = 15.0;
  // The old rows on this line (the research note's 1.3, as a check).
  std::printf("old kVowels: u %s  o %s  a %s  (17.5 cm)\n", fstr(kVowels[0], 17.5).c_str(),
              fstr(kVowels[1], 17.5).c_str(), fstr(kVowels[2], 17.5).c_str());
  Row male[4], female[4];
  double cm[4] = {}, cf[4] = {};
  if (measureOnly) {
#ifdef HAVE_SUNG
    for (int k = 0; k < 4; ++k) {
      const SungVowelRow& a = kSungVowelsMale[k];
      const SungVowelRow& b = kSungVowelsFemale[k];
      male[k] = {a.xc, a.ac, a.wc, a.aLip, a.aPh, a.aOr, a.lLip, a.aLar, a.len};
      female[k] = {b.xc, b.ac, b.wc, b.aLip, b.aPh, b.aOr, b.lLip, b.aLar, b.len};
    }
#else
    return 1;
#endif
  } else {
    for (int k = 0; k < 4; ++k) {
      Row s = kStart[kMaleFits[k].start];
      s.len = lenM;
      male[k] = fit(s, lenM, kMaleFits[k].t, &cm[k]);
      std::printf("male   %s  %s  target %.0f/%.0f/%.0f  cost %.4f\n", kMaleFits[k].name,
                  fstr(male[k], lenM).c_str(), kMaleFits[k].t[0], kMaleFits[k].t[1], kMaleFits[k].t[2], cm[k]);
      std::fflush(stdout);
      // The female row starts from the male one (and from puhuja's, the better of the two).
      Row s1 = male[k], s2 = kStart[kFemaleFits[k].start];
      s1.len = s2.len = lenF;
      double c1, c2;
      const Row f1 = fit(s1, lenF, kFemaleFits[k].t, &c1);
      const Row f2 = fit(s2, lenF, kFemaleFits[k].t, &c2);
      female[k] = c1 <= c2 ? f1 : f2;
      cf[k] = std::min(c1, c2);
      std::printf("female %s  %s  target %.0f/%.0f/%.0f  cost %.4f\n", kFemaleFits[k].name,
                  fstr(female[k], lenF).c_str(), kFemaleFits[k].t[0], kFemaleFits[k].t[1], kFemaleFits[k].t[2],
                  cf[k]);
      std::fflush(stdout);
    }
  }
  // Reached formants over the parts' lengths (the row's shape scaled).
  const double lensM[3] = {16.8, 17.5, 17.8}, lensF[3] = {14.5, 15.0, 15.5};
  std::string notes[2][4];
  for (int k = 0; k < 4; ++k) {
    std::string m, f;
    for (double l : lensM) m += " " + fstr(male[k], l) + " @" + std::to_string(l).substr(0, 4);
    for (double l : lensF) f += " " + fstr(female[k], l) + " @" + std::to_string(l).substr(0, 4);
    notes[0][k] = m, notes[1][k] = f;
    std::printf("%s male:%s\n%s female:%s\n", kMaleFits[k].name, m.c_str(), kMaleFits[k].name, f.c_str());
  }
  if (!emit) return 0;
  FILE* f = std::fopen(emit, "w");
  if (!f) return 1;
  std::fprintf(f,
               "// The choir's sung vowel rows on the Singer's own line: GENERATED by\n"
               "// tools/choirvowels.cc (-emit); edit there, not here.\n"
               "//\n"
               "// Each row is a Singer kVowels row (src/singer.cc VowelRow, the same 9\n"
               "// numbers in the same order: the tongue hump's place (share of the\n"
               "// length), area (cm^2) and half-width (cm); the lips' area; the pharynx's\n"
               "// and the mouth's areas (cm^2); the lip tube's length (cm); the larynx\n"
               "// tube's area (cm^2); the length (cm)). Fitted by Nelder-Mead in log\n"
               "// areas (puhuja's linefit) from puhuja's newer rows (kLineVowels u, ʊ, ɔ,\n"
               "// ɑ; 2026-11-04..09) toward sung resonance targets (research/choir.md\n"
               "// 2.1, 6.1, 9.2: Henrich, Smith & Wolfe 2011, the choral [u], the dark\n"
               "// choral [ɑ]), separately for a male (17.5 cm) and a female (15.0 cm)\n"
               "// tract: women's F2/F1 ratios differ, not only the scale.\n"
               "//\n"
               "// The formants reached are the lowest peaks of the Singer's impulse\n"
               "// response (the folds closed but for the chink, a pressure kick at the\n"
               "// glottis, the lip flow over 80 ms; research/choir.md 1.3). F1/F2/F3 (Hz)\n"
               "// at the part lengths (cm; the shape scaled), target in brackets:\n");
  for (int k = 0; k < 4; ++k)
    std::fprintf(f, "//   %s male   (%.0f/%.0f/%.0f):%s\n", kMaleFits[k].name, kMaleFits[k].t[0], kMaleFits[k].t[1],
                 kMaleFits[k].t[2], notes[0][k].c_str());
  for (int k = 0; k < 4; ++k)
    std::fprintf(f, "//   %s female (%.0f/%.0f/%.0f):%s\n", kFemaleFits[k].name, kFemaleFits[k].t[0],
                 kFemaleFits[k].t[1], kFemaleFits[k].t[2], notes[1][k].c_str());
  std::fprintf(f,
               "// (For comparison, the old kVowels on this line at 17.5 cm: u %s, o %s,\n"
               "// a %s.)\n"
               "#pragma once\n\n"
               "namespace pajupilli {\n\n"
               "struct SungVowelRow { double xc, ac, wc, aLip, aPh, aOr, lLip, aLar, len; };\n"
               "// The order: u (the choral \"ooh\"), ʊ (a laxer, opener ooh), ɔ (the high\n"
               "// sopranos' opened ooh), ɑ (the choral \"aah\").\n"
               "enum SungVowel { kSungU = 0, kSungUh = 1, kSungO = 2, kSungA = 3, kSungVowelN = 4 };\n\n",
               fstr(kVowels[0], 17.5).c_str(), fstr(kVowels[1], 17.5).c_str(), fstr(kVowels[2], 17.5).c_str());
  std::fprintf(f, "inline constexpr SungVowelRow kSungVowelsMale[kSungVowelN] = {\n");
  for (int k = 0; k < 4; ++k) printRow(f, male[k], kMaleFits[k].name);
  std::fprintf(f, "};\n\ninline constexpr SungVowelRow kSungVowelsFemale[kSungVowelN] = {\n");
  for (int k = 0; k < 4; ++k) printRow(f, female[k], kFemaleFits[k].name);
  std::fprintf(f, "};\n\n}  // namespace pajupilli\n");
  std::fclose(f);
  std::printf("wrote %s\n", emit);
  return 0;
}
