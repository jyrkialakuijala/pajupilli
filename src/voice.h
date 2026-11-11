// One instrument ready to play at 48 kHz: the flute model at its own rate
// (piccolo 96 kHz, alto 36 kHz, bass 24 kHz), the player, and a polyphase
// resampler to the output rate.
#pragma once

#include <memory>
#include <vector>

#include "flute.h"
#include "player.h"

namespace pajupilli {

// Rational L/M resampler: Kaiser-windowed sinc, 32 taps per phase at the
// input rate (latency 16 input samples: 0.17 ms piccolo .. 0.67 ms bass).
class Resampler {
 public:
  Resampler(int inRate, int outRate);
  bool identity() const { return L_ == M_; }
  // Inputs needed for the next n outputs.
  int needed(int n) const;
  // Consume `needed(n)` inputs from `in`, write n outputs.
  void process(const float* in, float* out, int n);

 private:
  int L_, M_;
  static constexpr int kTaps = 32;
  std::vector<float> h_;    // h_[phase * kTaps + k]
  std::vector<float> buf_;  // the last kTaps inputs, then the new ones
  long long j_ = 0;         // next output index
  long long nin_ = 0;       // inputs consumed so far
};

class Voice {
 public:
  explicit Voice(const Instrument& inst = concertFlute());
  Flute& flute() { return *flute_; }
  Player& player() { return *player_; }
  const Instrument& instrument() const { return flute_->instrument(); }
  void render(float* out, int n);  // 48 kHz

 private:
  std::unique_ptr<Flute> flute_;
  std::unique_ptr<Player> player_;
  Resampler rs_;
  std::vector<float> in_;
};

}  // namespace pajupilli
