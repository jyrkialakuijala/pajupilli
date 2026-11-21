// The C library's vector versions of the elementary functions (glibc's libmvec: sixteen floats
// or eight doubles a call), for loops over arrays.
//
// The compiler uses them by itself only under -ffast-math whole (__FAST_MATH__); this project
// builds with -fno-finite-math-only (the output guard's NaN checks), which switches that off. So
// the functions are declared here again with their vector versions, as <math.h> itself would
// have: a loop `for (i...) y[i] = pajupilli::vexpf(x[i]);` then becomes calls of 16 at a time.
// Within ~1 ulp of the scalar functions (not the same bits).
#pragma once

#include <cmath>

extern "C" {
__attribute__((simd("notinbranch"))) float expf(float) noexcept;
__attribute__((simd("notinbranch"))) float tanhf(float) noexcept;
__attribute__((simd("notinbranch"))) float expm1f(float) noexcept;
__attribute__((simd("notinbranch"))) float sinf(float) noexcept;
__attribute__((simd("notinbranch"))) float cosf(float) noexcept;
}

namespace pajupilli {
// tanh by one exponential (the library's goes by expm1 and takes three times as long): within
// 2e-16 of it; small arguments by their series (the quotient would lose their last digits).
inline double fastTanh(double x) {
  const double a = std::fabs(x);
  if (a < 0.02) {
    const double x2 = x * x;
    return x * (1 + x2 * (-1.0 / 3 + x2 * (2.0 / 15 + x2 * (-17.0 / 315))));
  }
  if (a > 19) return x > 0 ? 1.0 : -1.0;
  const double e = std::exp(2 * x);
  return (e - 1) / (e + 1);
}
inline float vexpf(float x) { return ::expf(x); }
inline float vtanhf(float x) { return ::tanhf(x); }
inline float vexpm1f(float x) { return ::expm1f(x); }
inline float vsinf(float x) { return ::sinf(x); }
inline float vcosf(float x) { return ::cosf(x); }
}  // namespace pajupilli
