#include "performer.h"

#include <algorithm>
#include <cmath>

namespace pajupilli {

Performer::Performer(Tube& t) : tube_(t), inst_(t.instrument()), fs_(t.fs()) {
  for (int i = 0; i < 128; ++i) coverTrim[i] = coverLoTrim[i] = coverHiTrim[i] = pressTrim[i] = jetTrim[i] = 1;
  for (int& f : fingering) f = 0;
  for (double& d : dynLo) d = 0;
  for (double& d : dynHi) d = 1;
}

double Performer::coverAt(int note, double dyn) const {
  const double lo = std::max(dynLo[note], 0.15);
  const double cl = std::log(coverLoTrim[note]), cm = std::log(coverTrim[note]), ch = std::log(coverHiTrim[note]);
  const double ct = dyn < 0.5 ? cl + (cm - cl) * std::clamp((dyn - lo) / std::max(0.5 - lo, 1e-3), 0.0, 1.0)
                              : cm + (ch - cm) * std::min(1.0, (dyn - 0.5) / (kCalDynHi - 0.5));
  return std::exp(ct);
}

// The pitch of the note, from a band-pass around it: each upward zero crossing
// gives one period; the control follows the error (the lips move slowly, like
// an ear correcting, so a fast trill is barely touched and a held note settles).
void Performer::listen(const float* x, int m, double& corr, double cents, double clock, double slope,
                       double tau, double dead) {
  const double f0 = noteFrequency(note_), w = 2 * M_PI * f0 / fs_;
  const double ft = f0 * std::exp2(cents / 1200);  // where the ear wants it
  const double r = std::exp(-M_PI * f0 / (3 * fs_));  // Q 3
  const double a1 = 2 * r * std::cos(w), a2 = -r * r;
  for (int k = 0; k < m; ++k) {
    const double y = (1 - r) * x[k] + a1 * bp1_ + a2 * bp2_;
    bp2_ = bp1_;
    bp1_ = y;
    const double t = clock * fs_ - m + k;  // samples on the note
    if (prevBp_ < 0 && y >= 0) {
      const double tc = t - 1 + prevBp_ / (prevBp_ - y);
      if (lastCross_ >= 0 && clock > 0.02) {  // past the note's own start
        const double f = fs_ / (tc - lastCross_);
        if (f > f0 / 1.3 && f < f0 * 1.3) {
          double err = 1200 * std::log2(f / ft);
          heard_ = err;
          err = err > dead ? err - dead : err < -dead ? err + dead : 0;
          corr -= err / slope * (1 / f) / tau;
          corr = std::clamp(corr, -0.4, 0.4);
        }
      }
      lastCross_ = tc;
    }
    prevBp_ = y;
  }
}

}  // namespace pajupilli
