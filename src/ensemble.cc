#include "ensemble.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace pajupilli {

namespace {

// Interval classes (semitones mod 12): the nearest simple ratio's offset from
// equal temperament (cents) and how much it matters (simpler = more).
struct Just { double cents, weight; };
constexpr Just kJust[12] = {
    {0.0, 1.0},     // 1/1, 2/1
    {11.7, 0.1},    // 16/15
    {3.9, 0.15},    // 9/8
    {15.6, 0.4},    // 6/5
    {-13.7, 0.5},   // 5/4
    {-2.0, 0.6},    // 4/3
    {-9.8, 0.05},   // 45/32
    {2.0, 0.8},     // 3/2
    {13.7, 0.35},   // 8/5
    {-15.6, 0.45},  // 5/3
    {-3.9, 0.15},   // 16/9
    {-11.7, 0.1},   // 15/8
};
constexpr double kToTempered = 0.05;  // pull of a new note to equal temperament
constexpr double kStay = 1.0;         // pull of a sounding note to where it is
constexpr double kMaxCents = 30;

const char* name(int midi) {
  static const char* n[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  return n[midi % 12];
}

}  // namespace

Ensemble::Ensemble(const Instrument& inst, const Calibration& cal, int maxFlutes, bool verbose)
    : verbose_(verbose), ji_(std::getenv("PJ_JI") ? std::atof(std::getenv("PJ_JI")) != 0 : true) {
  std::fill(owner_, owner_ + 128, -1);
  f_.resize(std::max(1, maxFlutes));
  for (int i = 0; i < int(f_.size()); ++i) {
    f_[i].v.reset(new Voice(inst));
    f_[i].v->seed(unsigned(i));  // each flute its own breath and wander
    f_[i].v->player().setCalibration(cal);
  }
}

int Ensemble::holding() const {
  int h = 0;
  for (const Slot& s : f_) h += s.held > 0;
  return h;
}

int Ensemble::pickFree(int midi) const {
  // Near in pitch and recently played: a line keeps to its flute (one
  // semitone weighs as much as 0.1 s since the flute's last key went up).
  int best = -1;
  double bestCost = 1e300;
  for (int i = 0; i < int(f_.size()); ++i) {
    const Slot& s = f_[i];
    if (s.held) continue;
    const double cost = s.lastNote < 0 ? 1e9 + i : std::abs(s.lastNote - midi) + 10 * (now_ - s.lastOff);
    if (cost < bestCost) { bestCost = cost; best = i; }
  }
  return best;
}

int Ensemble::nearestHeld(int midi) const {
  int best = -1, bestD = 1 << 30;
  for (int i = 0; i < int(f_.size()); ++i) {
    const Slot& s = f_[i];
    if (!s.held) continue;
    const int d = std::abs(s.v->player().sounding() - midi);
    if (d < bestD) { bestD = d; best = i; }
  }
  return best;
}

void Ensemble::take(int flute, int midi, int velocity) {
  Slot& s = f_[flute];
  owner_[midi] = flute;
  pressT_[midi] = now_;
  vel_[midi] = velocity;
  ++s.held;
  s.lastNote = midi;
  s.active = true;
  s.tunedNote = -1;  // a fresh note: it fits the chord, not its old place
  s.v->player().noteOn(midi, velocity);
}

void Ensemble::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  if (owner_[midi] >= 0) noteOff(midi);
  bool chord = false;
  for (int k = 0; k < 128; ++k)
    if (owner_[k] >= 0 && now_ - pressT_[k] < kChord) chord = true;
  const int h = holding();
  int f = -1;
  // Silence, a chord, or more lines expected than flutes holding: a flute of
  // its own; otherwise a slur on the nearest held flute.
  if (h == 0 || chord || h < lines_) f = pickFree(midi);
  if (f < 0) f = nearestHeld(midi);
  if (f < 0) f = 0;
  take(f, midi, velocity);
}

void Ensemble::noteOff(int midi) {
  if (midi < 0 || midi > 127 || owner_[midi] < 0) return;
  Slot& s = f_[owner_[midi]];
  owner_[midi] = -1;
  --s.held;
  s.lastOff = now_;
  s.v->player().noteOff(midi);
}

// Two keys held together on one flute for longer than a slur's overlap: the
// older note is another line; give it a flute of its own.
void Ensemble::split() {
  for (int i = 0; i < int(f_.size()); ++i) {
    Slot& s = f_[i];
    if (s.held < 2) continue;
    const int cur = s.v->player().sounding();
    if (cur < 0 || owner_[cur] != i || now_ - pressT_[cur] < kSplit) continue;
    for (int k = 0; k < 128; ++k) {
      if (owner_[k] != i || k == cur) continue;
      const int nf = pickFree(k);
      if (nf < 0) return;
      --s.held;
      s.v->player().noteOff(k);  // not sounding there: only leaves its key stack
      const double t = pressT_[k];
      take(nf, k, vel_[k]);
      pressT_[k] = t;
      if (verbose_) std::fprintf(stderr, "poly: note %d moves to flute %d\n", k, nf + 1);
    }
  }
}

void Ensemble::retune() {
  int idx[64], note[64], n = 0;
  long long sig = 0;
  for (int i = 0; i < int(f_.size()) && n < 64; ++i) {
    const Player& p = f_[i].v->player();
    if (!f_[i].held || !p.busy() || p.sounding() < 0) continue;
    idx[n] = i;
    note[n] = p.sounding();
    sig = sig * 1000003 + i * 131 + note[n] + 1;
    ++n;
  }
  if (sig == tuneSig_) return;
  tuneSig_ = sig;
  // Least squares: sum_pairs w (c_j - c_i - d_ij)^2 + sum_i l_i (c_i - a_i)^2.
  double A[64][64] = {}, b[64] = {};
  for (int i = 0; i < n; ++i) {
    Slot& s = f_[idx[i]];
    const bool stays = s.tunedNote == note[i];
    const double l = stays ? kStay : kToTempered, a = stays ? s.cents : 0;
    A[i][i] += l;
    b[i] += l * a;
  }
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) {
      if (note[j] <= note[i] && !(note[j] == note[i] && j > i)) continue;
      const int semis = note[j] - note[i];
      const Just& J = kJust[semis % 12];
      const double w = J.weight / (1 + 0.5 * (semis / 12)), d = J.cents;
      // c_j - c_i = d
      A[i][i] += w; A[j][j] += w; A[i][j] -= w; A[j][i] -= w;
      b[j] += w * d; b[i] -= w * d;
    }
  // Gaussian elimination (A is symmetric positive definite).
  for (int k = 0; k < n; ++k)
    for (int r = k + 1; r < n; ++r) {
      const double f = A[r][k] / A[k][k];
      for (int c = k; c < n; ++c) A[r][c] -= f * A[k][c];
      b[r] -= f * b[k];
    }
  double c[64];
  for (int k = n - 1; k >= 0; --k) {
    double v = b[k];
    for (int j = k + 1; j < n; ++j) v -= A[k][j] * c[j];
    c[k] = std::clamp(v / A[k][k], -kMaxCents, kMaxCents);
  }
  for (int i = 0; i < n; ++i) {
    Slot& s = f_[idx[i]];
    s.cents = c[i];
    s.tunedNote = note[i];
    s.v->player().setTuning(c[i]);
  }
  if (n > 1) {
    std::fprintf(stderr, "tune:");
    for (int i = 0; i < n; ++i) std::fprintf(stderr, " %s%d %+.0f", name(note[i]), note[i] / 12 - 1, c[i]);
    std::fprintf(stderr, "\n");
  }
}

void Ensemble::render(float* out, int n) {
  if (f_.size() > 1) split();
  if (f_.size() > 1 && ji_) retune();
  // How many lines: the most seen lately, back down after kForget with fewer.
  const int h = holding();
  if (h >= lines_) {
    calm_ = 0;
    if (h > lines_) {
      lines_ = h;
      std::fprintf(stderr, "flutes: %d\n", lines_);
    }
  } else if ((calm_ += n / 48000.0) > kForget) {
    calm_ = 0;
    lines_ = std::max(1, h);
    std::fprintf(stderr, "flutes: %d\n", lines_);
  }
  std::fill(out, out + n, 0.0f);
  buf_.resize(n);
  int playing = 0;
  for (Slot& s : f_) {
    // A flute silent for a while is not computed (its tail has died away).
    if (s.active && !s.held && !s.v->player().busy() && now_ - s.lastOff > kIdle) s.active = false;
    if (!s.active && f_.size() > 1) continue;
    s.v->render(buf_.data(), n);
    for (int i = 0; i < n; ++i) out[i] += buf_[i];
    playing += s.v->player().busy();
  }
  // Several flutes add up; keep the sum near one flute's level (1 / sqrt(n)).
  const double target = 1 / std::sqrt(std::max(1, playing));
  for (int i = 0; i < n; ++i) {
    mix_ += 0.0005 * (target - mix_);  // ~40 ms
    out[i] *= float(mix_);
  }
  now_ += n / 48000.0;
}

int Ensemble::resets() const {
  int r = 0;
  for (const Slot& s : f_) r += s.v->flute().resets();
  return r;
}

}  // namespace pajupilli
