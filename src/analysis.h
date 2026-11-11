// Small offline helpers: pitch estimation (YIN) and WAV writing.
#pragma once

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <vector>

namespace pajupilli {

// YIN pitch of x[0..n) between fmin and fmax. clarity: 1 - the normalised
// difference at the chosen lag (1 = perfectly periodic).
inline double estimatePitch(const float* x, int n, double fs, double fmin, double fmax,
                            double* clarity = nullptr) {
  const int tmin = std::max(2, int(fs / fmax)), tmax = std::min(n / 2, int(fs / fmin) + 1);
  const int w = n - tmax - 1;
  if (w <= 0 || tmin >= tmax) return 0;
  std::vector<double> d(tmax + 2, 0.0);
  for (int t = 1; t <= tmax + 1; ++t) {
    double s = 0;
    for (int i = 0; i < w; ++i) { const double e = x[i] - x[i + t]; s += e * e; }
    d[t] = s;
  }
  // Cumulative mean normalised difference.
  std::vector<double> dn(tmax + 2, 1.0);
  double run = 0;
  for (int t = 1; t <= tmax + 1; ++t) { run += d[t]; dn[t] = run > 0 ? d[t] * t / run : 1; }
  int best = -1;
  for (int t = tmin; t <= tmax; ++t)
    if (dn[t] < 0.15 && dn[t] <= dn[t - 1] && dn[t] <= dn[t + 1]) { best = t; break; }
  if (best < 0) {
    best = tmin;
    for (int t = tmin; t <= tmax; ++t) if (dn[t] < dn[best]) best = t;
  }
  const double a = dn[best - 1], b = dn[best], c = dn[best + 1];
  const double den = a - 2 * b + c;
  const double off = den != 0 ? 0.5 * (a - c) / den : 0;
  if (clarity) *clarity = 1 - b;
  return fs / (best + off);
}

inline double rms(const float* x, int n) {
  double s = 0;
  for (int i = 0; i < n; ++i) s += double(x[i]) * x[i];
  return n ? std::sqrt(s / n) : 0;
}

inline bool writeWav(const char* path, const std::vector<float>& x, int fs) {
  FILE* f = std::fopen(path, "wb");
  if (!f) return false;
  const uint32_t data = uint32_t(x.size() * 4);
  auto u32 = [f](uint32_t v) { std::fwrite(&v, 4, 1, f); };
  auto u16 = [f](uint16_t v) { std::fwrite(&v, 2, 1, f); };
  std::fwrite("RIFF", 1, 4, f); u32(36 + data); std::fwrite("WAVEfmt ", 1, 8, f);
  u32(16); u16(3); u16(1); u32(fs); u32(fs * 4); u16(4); u16(32);
  std::fwrite("data", 1, 4, f); u32(data);
  std::fwrite(x.data(), 4, x.size(), f);
  std::fclose(f);
  return true;
}

}  // namespace pajupilli
