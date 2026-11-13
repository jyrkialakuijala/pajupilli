// One instrument ready to play at 48 kHz: the model at its own rate (piccolo
// 96 kHz, alto flute 36 kHz, bass flute 24 kHz, ...), its player, and a
// polyphase resampler to the output rate.
#pragma once

#include <memory>
#include <vector>

#include "performer.h"
#include "tube.h"

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
  // The model and player for an instrument, at the model's rate.
  static std::unique_ptr<Tube> makeModel(const Instrument& inst);
  static std::unique_ptr<Performer> makePlayer(Tube& t);
  Tube& model() { return *model_; }
  Performer& player() { return *player_; }
  const Instrument& instrument() const { return model_->instrument(); }
  // 48 kHz. room (optional): the room's pressure at the instrument, n samples
  // at 48 kHz (Pa), taken to the model's rate.
  void render(float* out, int n, const float* room = nullptr);
  void seed(unsigned s) { model_->seed(s); player_->seed(s); }

 private:
  std::unique_ptr<Tube> model_;
  std::unique_ptr<Performer> player_;
  Resampler rs_;
  std::vector<float> in_, room_;
  float roomLast_ = 0;
};

}  // namespace pajupilli
