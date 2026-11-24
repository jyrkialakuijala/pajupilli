#include "velline.h"

#include <algorithm>
#include <cmath>

namespace pajupilli {

std::vector<VelReading> readVelocities(const std::vector<VelNote>& part, int tpq) {
  constexpr int kDead = 6;  // (velocity steps: under this a difference is the line's own)
  std::vector<VelReading> out(part.size());
  // The moments: notes struck together, the loudest key reading for all.
  struct Moment {
    long tick, off;
    int vel;
    size_t first, last;  // notes [first, last)
    float line = 0, lineEnd = 0;
  };
  std::vector<Moment> m;
  for (size_t i = 0; i < part.size(); ++i) {
    if (!m.empty() && part[i].tick - m.back().tick < tpq / 16 + 1) {
      m.back().vel = std::max(m.back().vel, part[i].vel);
      m.back().off = std::max(m.back().off, part[i].tickOff);
      m.back().last = i + 1;
    } else {
      m.push_back({part[i].tick, part[i].tickOff, part[i].vel, i, i + 1});
    }
  }
  for (size_t a = 0; a < m.size();) {
    size_t b = a + 1;  // the phrase [a, b)
    while (b < m.size() && m[b].tick - m[b - 1].off < tpq) ++b;
    const int n = int(b - a);
    auto v = [&](int k) { return m[a + std::clamp(k, 0, n - 1)].vel; };
    for (int k = 0; k < n; ++k) {
      // (at the phrase's ends the line is continued from inside: a ramp's first and last notes
      // are on it, a loud first note over a level line is an accent)
      const int pre = k > 0 ? v(k - 1) : n >= 3 ? 2 * v(1) - v(2) : v(k);
      const int post = k + 1 < n ? v(k + 1) : n >= 3 ? 2 * v(n - 2) - v(n - 3) : v(k);
      int three[3] = {pre, v(k), k == 0 ? v(1) : k + 1 == n ? v(n - 2) : post};
      if (k == 0) three[0] = pre;
      if (k + 1 == n && n > 1) three[2] = post, three[0] = v(n - 2);
      std::sort(three, three + 3);
      float line = float(n <= 2 ? v(k) : three[1]);
      if (std::abs(v(k) - line) < kDead) line = float(v(k));
      m[a + k].line = m[a + k].lineEnd = line;
    }
    for (int k = 0; k + 1 < n; ++k) {  // the glides: inside a run of three or more one way
      const float d = m[a + k + 1].line - m[a + k].line;
      if (std::abs(d) < kDead) continue;
      const bool before = k > 0 && (m[a + k].line - m[a + k - 1].line) * d > 0 && std::abs(m[a + k].line - m[a + k - 1].line) >= kDead;
      const bool after = k + 2 < n && (m[a + k + 2].line - m[a + k + 1].line) * d > 0 && std::abs(m[a + k + 2].line - m[a + k + 1].line) >= kDead;
      const bool bound = m[a + k + 1].tick - m[a + k].off < tpq / 4 + 1;
      if ((before || after) && bound) m[a + k].lineEnd = m[a + k + 1].line;
    }
    a = b;
  }
  for (const Moment& mo : m)
    for (size_t i = mo.first; i < mo.last; ++i) {
      // (a chord's softer keys keep their balance under its loudest: the same line, their own shortfall)
      out[i] = {mo.line, mo.lineEnd, float(mo.vel) - mo.line + (float(part[i].vel) - float(mo.vel))};
    }
  return out;
}

}  // namespace pajupilli
