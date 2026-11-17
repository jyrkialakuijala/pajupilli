// A noise source worth its name: xoshiro256** (Blackman & Vigna 2018; period
// 2^256 - 1, every output bit passing BigCrush and PractRand), its state
// seeded through splitmix64 so nearby seeds (pipe 36, pipe 37 ...) give
// unrelated streams. pair() takes one 64-bit output as two independent
// 32-bit halves: two noises per sample from one step, neither a successor of
// the other (an LCG stepped twice gives pairs on its lattice).
#pragma once

#include <cstdint>

namespace pajupilli {

// A seed for one of many sources (an organ's pipe: its stop, its key):
// distinct for every pair by construction; the generator's splitmix64 makes
// their streams unrelated.
inline uint64_t randomSeed(int stop, int key) { return (uint64_t(uint32_t(stop)) << 8) | uint64_t(key & 0xff); }

class Rng {
 public:
  explicit Rng(uint64_t seed = 1) { reseed(seed); }
  void reseed(uint64_t seed) {
    uint64_t z = seed;
    for (uint64_t& w : s_) {
      z += 0x9E3779B97F4A7C15ull;
      uint64_t x = z;
      x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
      x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
      w = x ^ (x >> 31);
    }
  }
  uint64_t next() {
    const uint64_t r = rotl(s_[1] * 5, 7) * 9, t = s_[1] << 17;
    s_[2] ^= s_[0];
    s_[3] ^= s_[1];
    s_[1] ^= s_[2];
    s_[0] ^= s_[3];
    s_[2] ^= t;
    s_[3] = rotl(s_[3], 45);
    return r;
  }
  // Uniform in [-1, 1) (24 bits: a float's mantissa), one or two at a time.
  float uniform() { return float(next() >> 40) * (2.0f / 16777216.0f) - 1; }
  void pair(float& a, float& b) {
    const uint64_t r = next();
    a = float(r >> 40) * (2.0f / 16777216.0f) - 1;
    b = float((r >> 8) & 0xFFFFFF) * (2.0f / 16777216.0f) - 1;
  }

 private:
  static uint64_t rotl(uint64_t x, int k) { return (x << k) | (x >> (64 - k)); }
  uint64_t s_[4];
};

}  // namespace pajupilli
