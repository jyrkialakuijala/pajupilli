#include "voice.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "flute.h"
#include "horn.h"
#include "hornplayer.h"
#include "player.h"
#include "reed.h"
#include "reedplayer.h"

namespace pajupilli {
namespace {

double besselI0(double x) {
  double s = 1, t = 1;
  for (int k = 1; k < 30; ++k) {
    t *= (x / (2 * k)) * (x / (2 * k));
    s += t;
  }
  return s;
}

}  // namespace

Resampler::Resampler(int inRate, int outRate) {
  const int g = std::gcd(inRate, outRate);
  L_ = outRate / g;
  M_ = inRate / g;
  if (identity()) return;
  // Prototype at L x the input rate; cut-off at 0.45 of the lower rate.
  const int n = L_ * kTaps;
  const double fc = 0.45 * std::min(inRate, outRate) / (double(inRate) * L_);  // cycles / sample
  const double beta = 8.0, c = 0.5 * (n - 1);
  std::vector<double> p(n);
  double sum = 0;
  for (int i = 0; i < n; ++i) {
    const double t = i - c;
    const double sinc = t == 0 ? 2 * fc : std::sin(2 * M_PI * fc * t) / (M_PI * t);
    const double r = t / c;
    p[i] = sinc * besselI0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / besselI0(beta);
    sum += p[i];
  }
  h_.resize(n);
  for (int ph = 0; ph < L_; ++ph)
    for (int k = 0; k < kTaps; ++k) h_[ph * kTaps + k] = float(p[k * L_ + ph] * L_ / sum);
  buf_.assign(kTaps, 0.0f);
}

int Resampler::needed(int n) const {
  if (identity()) return n;
  return int((j_ + n - 1) * M_ / L_ + 1 - nin_);
}

void Resampler::process(const float* in, float* out, int n) {
  if (identity()) {
    std::copy(in, in + n, out);
    return;
  }
  const int need = needed(n);
  // History of kTaps: an upsampling call can start one input back.
  const int hist = kTaps;
  buf_.resize(hist + need);
  std::copy(in, in + need, buf_.begin() + hist);
  // buf_[hist + m] is input nin_ + m.
  for (int o = 0; o < n; ++o, ++j_) {
    const long long u = j_ * M_;
    const long long i = u / L_;
    const int ph = int(u - i * L_);
    const float* x = buf_.data() + hist + (i - nin_);
    const float* hp = h_.data() + ph * kTaps;
    float s = 0;
    for (int k = 0; k < kTaps; ++k) s += hp[k] * x[-k];
    out[o] = s;
  }
  nin_ += need;
  std::copy(buf_.end() - hist, buf_.end(), buf_.begin());
  buf_.resize(hist);
}

std::unique_ptr<Tube> Voice::makeModel(const Instrument& inst) {
  if (inst.reed()) return std::unique_ptr<Tube>(new Reed(inst));
  if (inst.lip()) return std::unique_ptr<Tube>(new Horn(inst));
  return std::unique_ptr<Tube>(new Flute(inst));
}

std::unique_ptr<Performer> Voice::makePlayer(Tube& t) {
  if (t.instrument().reed()) return std::unique_ptr<Performer>(new ReedPlayer(static_cast<Reed&>(t)));
  if (t.instrument().lip()) return std::unique_ptr<Performer>(new HornPlayer(static_cast<Horn&>(t)));
  return std::unique_ptr<Performer>(new Player(static_cast<Flute&>(t)));
}

Voice::Voice(const Instrument& inst)
    : model_(makeModel(inst)), player_(makePlayer(*model_)), rs_(int(inst.fs), 48000) {}

void Voice::render(float* out, int n, const float* room) {
  const float g = float(instrument().gain);
  if (rs_.identity()) {
    if (room) model_->setRoom(room, n);
    player_->render(out, n);
  } else {
    const int need = rs_.needed(n);
    in_.resize(need);
    if (room) {
      // Linear interpolation to the model's rate (the room's sound is mostly
      // below 5 kHz: the bass flute's 24 kHz sees little aliasing).
      room_.resize(need);
      for (int i = 0; i < need; ++i) {
        const double t = (i + 1) * double(n) / need - 1;  // in input samples
        const int k = int(std::floor(t));
        const double f = t - k;
        const float a = k < 0 ? roomLast_ : room[std::min(k, n - 1)];
        const float b = room[std::min(k + 1, n - 1)];
        room_[i] = float(a + f * (b - a));
      }
      roomLast_ = room[n - 1];
      model_->setRoom(room_.data(), need);
    }
    player_->render(in_.data(), need);
    rs_.process(in_.data(), out, n);
  }
  if (g != 1)
    for (int i = 0; i < n; ++i) out[i] *= g;
}

}  // namespace pajupilli
