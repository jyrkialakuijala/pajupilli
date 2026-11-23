#include "harpsichord.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "songclock.h"

namespace pajupilli {

namespace {
constexpr int kChoirs = HarpsichordPlayer::kChoirs, kKeys = HarpsichordPlayer::kKeys, kLow = HarpsichordPlayer::kLow;

// A value given at some keys (MIDI numbers), between them along the logarithm.
double along(const int* at, const double* v, int n, int midi) {
  if (midi <= at[0]) return v[0];
  for (int i = 0; i + 1 < n; ++i)
    if (midi <= at[i + 1]) return v[i] * std::pow(v[i + 1] / v[i], double(midi - at[i]) / (at[i + 1] - at[i]));
  return v[n - 1];
}
}  // namespace

bool harpsichord(const Instrument& inst) { return std::strncmp(inst.name, "harpsichord", 11) == 0; }

PianoDesign harpsichordDesign() {
  static std::vector<float> phi;  // [string][resonance]: the board's share at each string's bridge point
  const PianoDesign grand = grandDesign();
  PianoDesign des = grand;
  des.name = "harpsichord";
  des.pedalNoise = 0;  // (a jack's cloth is no piano damper; no pedal)
  des.keys = kChoirs * kKeys;
  const double a4 = knob("PJ_HPSI_A4", 440), voice = knob("PJ_HPSI_VOICE", 1.0), boardK = knob("PJ_HPSI_BOARD", 2.0);
  const double lossK = knob("PJ_HPSI_LOSS", 1.0);
  // The tuning: just on the song's key (a MIDI file's; alone: C), PJ_HPSI_TONIC (0 = C .. 11 = B)
  // says otherwise; PJ_HPSI_TUNING=equal: equal temperament.
  static const double kJust[12] = {1, 16.0 / 15, 9.0 / 8, 6.0 / 5, 5.0 / 4, 4.0 / 3, 45.0 / 32, 3.0 / 2, 8.0 / 5, 5.0 / 3, 9.0 / 5, 15.0 / 8};
  const char* tuning = std::getenv("PJ_HPSI_TUNING");
  const bool just = !(tuning && std::strcmp(tuning, "equal") == 0);
  int tonic = SongClock::get().tonic >= 0 ? SongClock::get().tonic : 0;
  if (std::getenv("PJ_HPSI_TONIC")) tonic = ((std::atoi(std::getenv("PJ_HPSI_TONIC")) % 12) + 12) % 12;
  // The scales (research/harpsichord_clavi.md 2.2, harpsichord.md 3): Taskin's 8' and 4' shortened
  // to Hass's c'' = 340 mm; the 16' a guess for a 2.74 m case. Length and wire by key.
  static const int k8[] = {29, 36, 48, 60, 72, 84, 89};
  static const double l8[] = {1.764, 1.617, 1.178, 0.711, 0.357, 0.179, 0.135}, d8[] = {0.55e-3, 0.49e-3, 0.40e-3, 0.30e-3, 0.25e-3, 0.21e-3, 0.20e-3};
  static const int k4[] = {29, 48, 72, 89};
  static const double l4[] = {1.000, 0.605, 0.163, 0.058}, d4[] = {0.35e-3, 0.28e-3, 0.22e-3, 0.18e-3};
  static const int k16[] = {29, 48, 60, 72, 89};
  static const double l16[] = {2.300, 1.900, 1.150, 0.620, 0.250}, d16[] = {1.0e-3, 0.60e-3, 0.45e-3, 0.35e-3, 0.28e-3};
  const int nb = grand.board.n;
  phi.assign(size_t(des.keys) * nb, 0.0f);
  for (int c = 0; c < kChoirs; ++c) {
    for (int n = 0; n < kKeys; ++n) {
      PianoKey& s = des.key[c * kKeys + n];
      s = PianoKey();
      const int midi = kLow + n;
      const double t = n / (kKeys - 1.0);
      // Tuned before playing, to a key: just intonation on its tonic (every
      // interval from the tonic a ratio of small numbers, every octave
      // alike, the tonic itself where equal temperament has it), or equal.
      const int step = ((midi - tonic) % 12 + 12) % 12;
      const double tuned = just ? a4 * std::exp2((midi - step - 69) / 12.0) * kJust[step] : a4 * std::exp2((midi - 69) / 12.0);
      const double pitch = tuned * (c == 2 ? 2.0 : c == 3 ? 0.5 : 1.0);
      double L, d;
      bool brass;
      if (c < 2) { L = 0.952 * along(k8, l8, 7, midi); d = along(k8, d8, 7, midi); brass = midi < 48; }
      else if (c == 2) { L = 0.952 * along(k4, l4, 4, midi); d = along(k4, d4, 4, midi); brass = midi < 48; }
      else { L = along(k16, l16, 5, midi); d = along(k16, d16, 5, midi); brass = midi < 72; }
      const double rho = brass ? 8500 : 7850, E = brass ? 100e9 : 190e9;
      s.strings = 1;
      s.f1 = pitch;
      s.len = L;
      s.mu = rho * M_PI * d * d / 4;
      s.tension = s.mu * (2 * L * pitch) * (2 * L * pitch);
      s.B = M_PI * M_PI * M_PI * E * d * d * d * d / (64 * s.tension * L * L);
      if (c == 3 && midi < 48) s.B *= 0.15;  // (the 16' bass overspun: a thin core under a wrap of the same mass)
      // Where its row plucks: an eighth of the string from the nut in the
      // bass to a third in the treble; the front 8' 15 mm further on; the
      // 4' and the 16' their own.
      double beta = c == 2 ? 0.15 + 0.20 * t : c == 3 ? 0.10 + 0.15 * t : 0.125 + 0.20 * t;
      if (c == 1) beta += 0.015 / L;
      s.strike = std::min(beta, 0.5);
      // The plectrum: a Delrin tongue of 1200 N/m that lets go at 1.2 N in
      // the treble to 2.5 N in the bass (more for the 16', less for the 4').
      s.pluckK = 1200;
      s.pluckLimit = voice * (c == 2 ? 0.7 : c == 3 ? 1.4 : 1.0) * (2.5 - 1.3 * t);
      s.pluckSide = 0.33;  // (the string leaves 1 mm sideways to 3 mm up)
      s.mh = 1e-3; s.K = 1e9; s.p = 2.5;  // (no hammer)
      // The wire's own loss: thin wire in air, more by frequency than a piano's.
      s.loss = lossK * 0.34 * std::exp2(2.0 * t) * (c == 2 ? 1.4 : c == 3 ? 0.9 : 1.0);
      s.b2 = lossK * 3.0e-4;
      s.b3 = lossK * 3e-8;
      // The damper: a flag of cloth in the jack's top, just beyond the plectrum.
      s.hasDamper = true;
      s.damper = std::min(s.strike + 0.02, 0.6);
      s.damperLen = 0.012;
      s.damperRate = 250 * (1 + 1.5 * (1 - t));  // (the bass strings' cloth is thicker)
      s.top = c == 3 ? 8000 : 12000;
      // Its place on the soundboard: the bridge point of the piano key of its pitch.
      const int pk = std::clamp(int(std::lround(69 + 12 * std::log2(pitch / 440))) - 21, 0, 87);
      for (int m = 0; m < nb; ++m) phi[size_t(c * kKeys + n) * nb + m] = float(boardK * grand.board.phi[size_t(pk) * nb + m]);
    }
  }
  des.board.phi = phi.data();
  des.hammerWidth = 0.001;   // (a plectrum's width)
  des.sideTuneCents = knob("PJ_HPSI_SIDETUNE", 14);  // the bridge pin: the two directions 0.8 % apart
  des.top = 12000;
  des.topTreble = 12000;
  return des;
}

HarpsichordPlayer::HarpsichordPlayer(PianoModel& m) : Performer(m), m_(m) {
  const char* name = m.instrument().name;
  if (!std::strcmp(name, "harpsichord-8")) on_[1] = false;
  if (!std::strcmp(name, "harpsichord-full")) on_[2] = on_[3] = true;
  if (const char* st = std::getenv("PJ_HPSI_STOPS")) {  // e.g. "16,8,8,4": the first 8 the back row, the second the front
    std::fill(on_, on_ + kChoirs, false);
    int eights = 0;
    for (const char* p = st; *p;) {
      const int v = std::atoi(p);
      if (v == 8) on_[std::min(eights++, 1)] = true;
      else if (v == 4) on_[2] = true;
      else if (v == 16) on_[3] = true;
      while (*p && *p != ',') ++p;
      if (*p) ++p;
    }
  }
}

int HarpsichordPlayer::keyOf(int midi) const {
  while (midi < kLow) midi += 12;
  while (midi > kLow + kKeys - 1) midi -= 12;
  return midi - kLow;
}

void HarpsichordPlayer::noteOn(int midi, int velocity) {
  const int n = keyOf(midi);
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  // The key's speed is the jacks': 0.15 .. 0.5 m/s. It hardly changes the
  // sound; it sets how soon each row lets go. The rows are staggered: the
  // back 8' first, then the 4', the front 8', the 16', each a little later
  // (a slow key spreads them further).
  const double v = std::clamp(velocity, 1, 127) / 127.0, speed = 0.15 + 0.35 * v;
  static const int order[kChoirs] = {0, 2, 1, 3};
  int row = 0;
  for (int i = 0; i < kChoirs; ++i) {
    const int c = order[i];
    if (!(on_[c] || (hold_ && c >= 2))) continue;
    due_.push_back({row * 0.0006 / speed, c * kKeys + n, speed});  // (0.6 mm of key travel between rows)
    ++row;
  }
}

void HarpsichordPlayer::noteOff(int midi) {
  const int n = keyOf(midi);
  // The jacks fall back: every choir's damper for this key lands (those of
  // the rows that are off never left).
  for (int c = 0; c < kChoirs; ++c) m_.piano().damper(c * kKeys + n, true);
  for (size_t i = 0; i < due_.size();)
    if (due_[i].key % kKeys == n) due_.erase(due_.begin() + i); else ++i;
}

void HarpsichordPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    for (size_t j = 0; j < due_.size();) {
      if ((due_[j].in -= m / fs_) <= 0) {
        m_.piano().damper(due_[j].key, false);
        m_.piano().strike(due_[j].key, due_[j].speed);
        due_.erase(due_.begin() + j);
      } else {
        ++j;
      }
    }
    m_.piano().setVoicing(std::pow(2.0, brightness_ - 1) * hardness_);  // (the plectra's voicing)
    m_.process(out + i, m);
  }
  blowing_ = !m_.piano().asleep() || !due_.empty();
}

}  // namespace pajupilli
