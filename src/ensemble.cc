#include "ensemble.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

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

// Where each instrument is at ease (sounding MIDI), and how readily it is
// chosen when several would do: the flutes lead.
struct Comfort { const char* name; int lo, hi; double pref; };
constexpr Comfort kComfort[] = {
    {"flute", 60, 91, 0}, {"flute-c", 60, 91, 0}, {"piccolo", 79, 103, 1},
    {"alto", 55, 79, 1}, {"bass", 48, 69, 1.5},
};
constexpr double kCross = 6;    // two voices crossing (a low instrument above a higher one)
constexpr double kOctave = 24;  // a key folded by an octave into an instrument's range

}  // namespace

Ensemble::Ensemble(const std::vector<Member>& members, bool verbose, const RoomSpec* room)
    : verbose_(verbose), ji_(std::getenv("PJ_JI") ? std::atof(std::getenv("PJ_JI")) != 0 : true) {
  init(members, room);
}

Ensemble::Ensemble(const Instrument& inst, const Calibration& cal, int maxFlutes, bool verbose,
                   const RoomSpec* room)
    : verbose_(verbose), ji_(std::getenv("PJ_JI") ? std::atof(std::getenv("PJ_JI")) != 0 : true) {
  init(std::vector<Member>(std::max(1, maxFlutes), Member{&inst, &cal}), room);
}

void Ensemble::init(const std::vector<Member>& members, const RoomSpec* room) {
  std::fill(owner_, owner_ + 128, -1);
  std::fill(pitch_, pitch_ + 128, -1);
  f_.resize(members.size());
  int same = 0;
  for (int i = 0; i < int(f_.size()); ++i) {
    const Instrument& in = *members[i].inst;
    Slot& s = f_[i];
    s.v.reset(new Voice(in));
    s.v->seed(unsigned(i));  // each flute its own breath and wander
    s.v->player().setCalibration(*members[i].cal);
    s.lo = in.lo + in.transpose;
    s.hi = in.hi + in.transpose;
    s.rank = s.lo;
    s.easyLo = s.lo;
    s.easyHi = s.hi - 5;
    s.pref = 1;
    for (const Comfort& c : kComfort)
      if (std::strcmp(c.name, in.name) == 0) { s.easyLo = c.lo; s.easyHi = c.hi; s.pref = c.pref; }
    lo_ = std::min(lo_, s.lo);
    hi_ = std::max(hi_, s.hi);
    mixed_ |= members[i].inst != members[0].inst;
    int nth = 0, of = 0;
    for (int j = 0; j < int(members.size()); ++j)
      if (members[j].inst == members[i].inst) { ++of; if (j <= i) ++nth; }
    s.label = std::string(std::strncmp(in.name, "flute", 5) == 0 ? "flute" : in.name) +
              (of > 1 ? " " + std::to_string(nth) : "");
    same += of > 1;
  }
  (void)same;
  if (room && room->volume > 0) {
    // Seats across the stage, high instruments on the left, 1 m apart.
    const int n = int(f_.size());
    std::vector<int> order(n);
    for (int i = 0; i < n; ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return f_[a].rank > f_[b].rank; });
    std::vector<double> x(n);
    const double gap = n > 6 ? 0.8 : 1.0;
    for (int k = 0; k < n; ++k) x[order[k]] = (k - (n - 1) / 2.0) * gap;
    room_.reset(new Room(*room, x));
    for (Slot& s : f_) s.room.assign(Room::kBlock, 0.0f);
  }
}

std::string Ensemble::describe() const {
  std::string d;
  for (const Slot& s : f_) d += (d.empty() ? "" : ", ") + s.label;
  if (!mixed_ && f_.size() > 1) d = std::to_string(f_.size()) + " x " + f_[0].label.substr(0, f_[0].label.find(' '));
  if (room_) {
    char b[96];
    std::snprintf(b, sizeof b, "; room: %s (%.0f m3, RT60 %.1f s)", room_->spec().name, room_->spec().volume,
                  room_->spec().rt60);
    d += b;
  }
  return d;
}

int Ensemble::holding() const {
  int h = 0;
  for (const Slot& s : f_) h += s.held > 0;
  return h;
}

int Ensemble::fold(int f, int key, int* octaves) const {
  const Slot& s = f_[f];
  int p = key, o = 0;
  while (p < s.lo) { p += 12; ++o; }
  while (p > s.hi) { p -= 12; ++o; }
  if (octaves) *octaves = o;
  return p >= s.lo ? p : -1;
}

double Ensemble::cost(int f, int p, int octaves) const {
  const Slot& s = f_[f];
  if (!mixed_)  // one semitone weighs as much as 0.1 s since the flute's last key went up
    return s.lastNote < 0 ? 1e9 + f : std::abs(s.lastNote - p) + 10 * (now_ - s.lastOff);
  const double strain = std::max(0, s.easyLo - p) + std::max(0, p - s.easyHi);
  const double line = s.lastNote < 0 ? 2 : std::min(2.0, 0.2 * std::abs(s.lastNote - p) + 2 * (now_ - s.lastOff));
  return strain + s.pref + line + kOctave * octaves;
}

double Ensemble::crossing(int f, int p, int g, int q) const {
  if (!mixed_) return 0;
  const int a = f_[f].rank, b = f_[g].rank;
  return (a < b && p > q) || (a > b && p < q) ? kCross : 0;
}

int Ensemble::pickFree(int key, int* pitch) const {
  // Near in pitch and recently played: a line keeps to its flute; in an
  // ensemble also the instrument the note suits, not crossing the others.
  int best = -1;
  double bestCost = 1e300;
  for (int i = 0; i < int(f_.size()); ++i) {
    if (f_[i].held) continue;
    int o;
    const int p = fold(i, key, &o);
    if (p < 0) continue;
    double c = cost(i, p, o);
    for (int g = 0; g < int(f_.size()); ++g)
      if (f_[g].held) c += crossing(i, p, g, f_[g].v->player().sounding());
    if (c < bestCost) { bestCost = c; best = i; *pitch = p; }
  }
  return best;
}

int Ensemble::nearestHeld(int key) const {
  int best = -1, bestD = 1 << 30;
  for (int i = 0; i < int(f_.size()); ++i) {
    const Slot& s = f_[i];
    if (!s.held || (mixed_ && (key < s.lo || key > s.hi))) continue;
    const int d = std::abs(s.v->player().sounding() - key);
    if (d < bestD) { bestD = d; best = i; }
  }
  return best;
}

void Ensemble::take(int flute, int key, int pitch, int velocity) {
  Slot& s = f_[flute];
  owner_[key] = flute;
  pitch_[key] = pitch;
  pressT_[key] = now_;
  vel_[key] = velocity;
  ++s.held;
  s.lastNote = pitch;
  s.active = true;
  s.tunedNote = -1;  // a fresh note: it fits the chord, not its old place
  s.v->player().noteOn(pitch, velocity);
}

void Ensemble::release(int key) {
  Slot& s = f_[owner_[key]];
  owner_[key] = -1;
  --s.held;
  s.lastOff = now_;
  s.v->player().noteOff(pitch_[key]);
}

// A mixed pool: the new key and the keys struck just before it (still alone on
// their instruments) are dealt together over those instruments and the free
// ones, at the least total cost (comfort, lines, no crossings with each other
// or with the voices already sounding).
bool Ensemble::deal(int key, int velocity) {
  int keys[8], nk = 0, cand[16], nc = 0;
  for (int k = 0; k < 128 && nk < 5; ++k)
    if (owner_[k] >= 0 && now_ - pressT_[k] < kChord && f_[owner_[k]].held == 1) keys[nk++] = k;
  keys[nk++] = key;
  for (int i = 0; i < int(f_.size()) && nc < 16; ++i) {
    bool movable = false;
    for (int j = 0; j + 1 < nk; ++j) movable |= owner_[keys[j]] == i;
    if (!f_[i].held || movable) cand[nc++] = i;
  }
  if (nc < nk) return false;
  int fixed[16], fixedP[16], nf = 0;
  for (int i = 0; i < int(f_.size()) && nf < 16; ++i) {
    bool isCand = false;
    for (int j = 0; j < nc; ++j) isCand |= cand[j] == i;
    if (f_[i].held && !isCand) { fixed[nf] = i; fixedP[nf++] = f_[i].v->player().sounding(); }
  }
  // Exhaustive: at most 6 keys over at most 16 instruments, pruned by cost.
  int pick[8], pickP[8], best[8], bestP[8];
  bool used[16] = {};
  double bestCost = 1e300;
  auto search = [&](auto&& self, int j, double c) -> void {
    if (c >= bestCost) return;
    if (j == nk) {
      bestCost = c;
      std::copy(pick, pick + nk, best);
      std::copy(pickP, pickP + nk, bestP);
      return;
    }
    for (int ci = 0; ci < nc; ++ci) {
      if (used[ci]) continue;
      const int f = cand[ci];
      int o;
      const int p = fold(f, keys[j], &o);
      if (p < 0) continue;
      double add = cost(f, p, o);
      for (int m = 0; m < nf; ++m) add += crossing(f, p, fixed[m], fixedP[m]);
      for (int m = 0; m < j; ++m) add += crossing(f, p, pick[m], pickP[m]);
      used[ci] = true;
      pick[j] = f;
      pickP[j] = p;
      self(self, j + 1, c + add);
      used[ci] = false;
    }
  };
  search(search, 0, 0.0);
  if (bestCost >= 1e300) return false;
  // Move the keys that are better elsewhere (struck within kChord: their
  // attack has hardly begun), then add the new one.
  bool moved = false;
  for (int j = 0; j + 1 < nk; ++j) {
    const int k = keys[j];
    if (owner_[k] == best[j] && pitch_[k] == bestP[j]) continue;
    const double t = pressT_[k];
    release(k);
    take(best[j], k, bestP[j], vel_[k]);
    pressT_[k] = t;
    moved = true;
  }
  take(best[nk - 1], key, bestP[nk - 1], velocity);
  if (verbose_ && (nk > 1 || moved)) {
    std::fprintf(stderr, "deal:");
    for (int j = 0; j < nk; ++j)
      std::fprintf(stderr, " %s%d %s%s", name(bestP[j]), bestP[j] / 12 - 1, f_[best[j]].label.c_str(), j + 1 < nk ? "," : "");
    std::fprintf(stderr, "\n");
  }
  return true;
}

void Ensemble::noteOn(int key, int velocity) {
  if (key < 0 || key > 127) return;
  if (owner_[key] >= 0) noteOff(key);
  bool chord = false;
  for (int k = 0; k < 128; ++k)
    if (owner_[k] >= 0 && now_ - pressT_[k] < kChord) chord = true;
  const int h = holding();
  // Silence, a chord, or more lines expected than flutes holding: a flute of
  // its own; otherwise a slur on the nearest held flute.
  const bool own = h == 0 || chord || h < lines_;
  if (mixed_ && own && deal(key, velocity)) return;
  int f = -1, p = key;
  if (own && !mixed_) f = pickFree(key, &p);
  if (f < 0) { f = nearestHeld(key); p = key; }
  if (f < 0) f = pickFree(key, &p);  // no held instrument reaches it
  if (f < 0) {  // all busy, none reaches it: the nearest, an octave off
    int bestD = 1 << 30;
    for (int i = 0; i < int(f_.size()); ++i) {
      const int q = fold(i, key, nullptr), d = q < 0 ? 1 << 29 : std::abs(q - key);
      if (d < bestD) { bestD = d; f = i; p = q; }
    }
  }
  if (f < 0 || p < 0) return;
  take(f, key, p, velocity);
}

void Ensemble::noteOff(int key) {
  if (key < 0 || key > 127 || owner_[key] < 0) return;
  release(key);
}

// Two keys held together on one flute for longer than a slur's overlap: the
// older note is another line; give it a flute of its own.
void Ensemble::split() {
  for (int i = 0; i < int(f_.size()); ++i) {
    Slot& s = f_[i];
    if (s.held < 2) continue;
    const int cur = s.v->player().sounding();
    int curKey = -1;
    for (int k = 0; k < 128; ++k)
      if (owner_[k] == i && pitch_[k] == cur) curKey = k;
    if (curKey < 0 || now_ - pressT_[curKey] < kSplit) continue;
    for (int k = 0; k < 128; ++k) {
      if (owner_[k] != i || k == curKey) continue;
      int p;
      const int nf = pickFree(k, &p);
      if (nf < 0) return;
      --s.held;
      s.v->player().noteOff(pitch_[k]);  // not sounding there: only leaves its key stack
      const double t = pressT_[k];
      take(nf, k, p, vel_[k]);
      pressT_[k] = t;
      if (verbose_) std::fprintf(stderr, "poly: note %d moves to %s\n", p, mixed_ ? f_[nf].label.c_str() : ("flute " + std::to_string(nf + 1)).c_str());
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

void Ensemble::render(float* left, float* right, int n) {
  // In blocks of Room::kBlock (the room's coupling step), queued.
  for (int done = 0; done < n;) {
    if (qPos_ >= qL_.size()) {
      qL_.assign(Room::kBlock, 0.0f);
      qR_.assign(Room::kBlock, 0.0f);
      qPos_ = 0;
      block(qL_.data(), qR_.data(), Room::kBlock);
    }
    const int m = std::min(n - done, int(qL_.size() - qPos_));
    std::copy(qL_.begin() + qPos_, qL_.begin() + qPos_ + m, left + done);
    std::copy(qR_.begin() + qPos_, qR_.begin() + qPos_ + m, right + done);
    qPos_ += m;
    done += m;
  }
}

void Ensemble::render(float* out, int n) {
  std::vector<float> r(n);
  render(out, r.data(), n);
  for (int i = 0; i < n; ++i) out[i] = 0.5f * (out[i] + r[i]);
}

void Ensemble::block(float* left, float* right, int n) {
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
  const int nf = int(f_.size());
  buf_.resize(nf);
  int playing = 0;
  for (int i = 0; i < nf; ++i) {
    Slot& s = f_[i];
    buf_[i].assign(n, 0.0f);
    // Without a room, a flute silent for a while is not computed (its tail
    // has died away); in a room every flute listens.
    if (s.active && !s.held && !s.v->player().busy() && now_ - s.lastOff > kIdle) s.active = false;
    if (!room_ && !s.active && nf > 1) continue;
    s.v->render(buf_[i].data(), n, room_ ? s.room.data() : nullptr);
    playing += s.v->player().busy();
  }
  if (room_) {
    // To pressure at 1 m (Pa) for the room; the listener's microphones keep
    // each instrument's balance (its gain).
    const float k = float(f_[0].v->flute().outScale());
    srcBuf_.resize(size_t(nf) * n);
    std::vector<const float*> src(nf);
    std::vector<float*> at(nf);
    std::vector<float> lg(nf);
    for (int i = 0; i < nf; ++i) {
      const float g = float(f_[i].v->instrument().gain);
      for (int j = 0; j < n; ++j) srcBuf_[size_t(i) * n + j] = buf_[i][j] / (g * k);
      src[i] = &srcBuf_[size_t(i) * n];
      at[i] = f_[i].room.data();
      lg[i] = g;
    }
    room_->process(src.data(), n, at.data(), left, right, lg.data());
    for (int i = 0; i < nf; ++i)
      for (int j = 0; j < n; ++j) at[i][j] *= roomFb_;
    for (int j = 0; j < n; ++j) { left[j] *= k; right[j] *= k; }
  } else {
    for (int i = 0; i < nf; ++i)
      for (int j = 0; j < n; ++j) left[j] += buf_[i][j];
    std::copy(left, left + n, right);
  }
  // Several flutes add up; keep the sum near one flute's level (1 / sqrt(n)).
  const double target = 1 / std::sqrt(std::max(1, playing));
  for (int i = 0; i < n; ++i) {
    mix_ += 0.0005 * (target - mix_);  // ~40 ms
    left[i] *= float(mix_);
    right[i] *= float(mix_);
  }
  now_ += n / 48000.0;
}

int Ensemble::resets() const {
  int r = 0;
  for (const Slot& s : f_) r += s.v->flute().resets();
  return r;
}

}  // namespace pajupilli
