#include "harpsichord.h"
#include "director.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "songclock.h"
#include "vmath.h"

namespace pajupilli {

namespace {
constexpr int kChoirs = HarpsichordPlayer::kChoirs, kKeys = HarpsichordPlayer::kKeys, kLow = HarpsichordPlayer::kLow;

// A value given at some keys (MIDI numbers), between them along the logarithm.
double along(const int* at, const double* v, int n, int midi) {
  if (midi <= at[0]) return v[0];
  for (int i = 0; i + 1 < n; ++i)
    if (midi <= at[i + 1]) return v[i] * fastPow(v[i + 1] / v[i], double(midi - at[i]) / (at[i + 1] - at[i]));
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
  // (the brilliance, by ear, 2026-11-24: plucked a fifth nearer the nut, PJ_HPSI_PLUCKAT 0.8, and the
  // wire's loss x0.75, PJ_HPSI_LOSS: 2-8 kHz +1.5 .. 4.5 dB at the pluck, +3.5 .. 6.5 dB as it rings;
  // PJ_HPSI_PLECTRUM scales the tongue's stiffness)
  const double lossK = knob("PJ_HPSI_LOSS", 0.75), pluckAt = knob("PJ_HPSI_PLUCKAT", 0.8), plectrumK = knob("PJ_HPSI_PLECTRUM", 1.0);
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
      // bass to a third in the treble (x PJ_HPSI_PLUCKAT); the front 8' 15 mm further on; the
      // 4' and the 16' their own.
      double beta = c == 2 ? 0.15 + 0.20 * t : c == 3 ? 0.10 + 0.15 * t : 0.125 + 0.20 * t;
      if (c == 1) beta += 0.015 / L;
      s.strike = std::min(beta * pluckAt, 0.5);
      // The plectrum: a Delrin tongue of 1200 N/m that lets go at 1.2 N in
      // the treble to 2.5 N in the bass (more for the 16', less for the 4').
      s.pluckK = 1200 * plectrumK;
      s.pluckLimit = voice * (c == 2 ? 0.7 : c == 3 ? 1.4 : 1.0) * (2.5 - 1.3 * t);
      s.pluckSide = 0.33;  // (the string leaves 1 mm sideways to 3 mm up)
      // (the quill's tip: soft for its first PJ_HPSI_TIP mm, 0.4; the force begun with a corner
      // rang the string's low modes for the 5-12 ms the jack pushes, a thump before the snap: a
      // felt hammer's attack, not a quill's)
      s.pluckSoft = knob("PJ_HPSI_TIP", 0.4) * 1e-3;
      // (the quill's snap off the string, by ear 2026-11-25: the low strings' short-lived high
      // transients; PJ_HPSI_SNAP samples of the release force into their high modes at the bass's
      // lowest key, less up to middle C, none above)
      // (... over the low and middle strings: in full to PJ_HPSI_SNAP_FULLKEY (G2), none from
      // PJ_HPSI_SNAP_TOPKEY (F#4); and from lower down each string's own spectrum: from its
      // PJ_HPSI_SNAP_FROM-th partial (6), in full from its PJ_HPSI_SNAP_TO-th (16), no higher than
      // 1.5 and 5 kHz: a low string's many overtones, there at the pluck and soon gone)
      static const double kSnapFull = knob("PJ_HPSI_SNAP_FULLKEY", 43), kSnapTop = knob("PJ_HPSI_SNAP_TOPKEY", 66);
      static const double kSnapFrom = knob("PJ_HPSI_SNAP_FROM", 6), kSnapTo = knob("PJ_HPSI_SNAP_TO", 16);
      // (PJ_HPSI_SNAP 14: it was 24 on the modes above 1.5 kHz alone; under C2 a little less, as
      // the string's pitch^0.7: the lowest strings have the most modes in the band)
      s.pluckSnap = knob("PJ_HPSI_SNAP", 14) * std::clamp((kSnapTop - midi) / std::max(1.0, kSnapTop - kSnapFull), 0.0, 1.0) *
                    std::clamp(std::pow(pitch / 65.4, 0.7), 0.5, 1.0);
      s.pluckSnapLo = std::min(1500.0, kSnapFrom * pitch);
      s.pluckSnapHi = std::max(s.pluckSnapLo + 100, std::min(5000.0, kSnapTo * pitch));
      // (the lowest two octaves, to F3: more yet of the harmonics far up, past the 60th: the
      // snap x (1 + PJ_HPSI_SNAP_TILT) from five times its full partial on (the 80th), rising from twice it; none
      // of that from C4. A long bass string's pluck has a hundred partials at first.)
      s.pluckSnapTilt = knob("PJ_HPSI_SNAP_TILT", 1.5) * std::clamp((60.0 - midi) / 7.0, 0.0, 1.0);
      s.mh = 1e-3; s.K = 1e9; s.p = 2.5;  // (no hammer)
      // The wire's own loss: thin wire in air, more by frequency than a piano's.
      s.loss = lossK * 0.34 * std::exp2(2.0 * t) * (c == 2 ? 1.4 : c == 3 ? 0.9 : 1.0);
      // The partials' decay by their number (by ear, 2026-11-25: the low ones, to the 7th, ring on;
      // above, they die faster): less loss in proportion to frequency (PJ_HPSI_LOSS2, x0.4 of the
      // earlier), more as its square, turning at the PJ_HPSI_TURN-th partial (7.5) of the string's
      // own pitch: there PJ_HPSI_HIDAMP more (0.3 1/s; at twice that partial four times as much).
      static const double kLin = knob("PJ_HPSI_LOSS2", 0.4), kHi = knob("PJ_HPSI_HIDAMP", 0.3), kTurn = knob("PJ_HPSI_TURN", 7.5);
      s.b2 = lossK * 3.0e-4 * kLin;
      s.b3 = std::max(lossK * 3e-8, kHi / ((kTurn * pitch) * (kTurn * pitch)));
      // The damper: a flag of cloth in the jack's top, just beyond the plectrum.
      s.hasDamper = true;
      s.damper = std::min(s.strike + 0.02, 0.6);
      s.damperLen = 0.012;
      // (the cloth is held by the jack, which hangs on it by its own weight: PJ_HPSI_JACK g, 8.
      // Left at the model's least, a 1 g head, a bass string threw it off again and again: a
      // released note under C3 rang on for half a second, its level wobbling.)
      s.damperMass = knob("PJ_HPSI_JACK", 8) * 1e-3;
      s.damperRate = 250 * (1 + 1.5 * (1 - t));  // (the bass strings' cloth is thicker)
      s.top = c == 3 ? 8000 : 12000;
      // Its place on the soundboard: the bridge point of the piano key of its pitch.
      const int pk = std::clamp(int(std::lround(69 + 12 * std::log2(pitch / 440))) - 21, 0, 87);
      for (int m = 0; m < nb; ++m) phi[size_t(c * kKeys + n) * nb + m] = float(boardK * grand.board.phi[size_t(pk) * nb + m]);
    }
  }
  des.board.phi = phi.data();
  des.hammerWidth = 0.001;   // (a plectrum's width)
  // (a key plucked again while its string still rings, by the owner's ear, 2026-11-26: the new
  // pluck is not only added to the old swing. The quill's tip rests against the string for a
  // moment before it bends, and stops part of what swings into it there: PJ_HPSI_REPLUCK, 0.6, of
  // the energy of a partial that moves fully at the pluck point and is on the quill's side of the
  // string during the touch (-4 dB), less by the partial's share of motion at that point, none of
  // one with a node there or swinging away; over PJ_HPSI_REPLUCK_MS, 1.5, as a hammer on the
  // grand's ringing string. 0: off, the pluck only added, as before.)
  des.repluck = knob("PJ_HPSI_REPLUCK", 0.6);
  des.repluckTime = knob("PJ_HPSI_REPLUCK_MS", 1.5) * 1e-3;
  des.sideTuneCents = knob("PJ_HPSI_SIDETUNE", 14);  // the bridge pin: the two directions 0.8 % apart
  des.top = 12000;
  // (the strings nobody plays, ringing in sympathy: their overtones to 5 kHz only. All 244 strings
  // are computed while anything sounds, and at their full 12 kHz that was four fifths of the
  // harpsichord's cost. PJ_HPSI_SYMTOP 0: all of them, as before.)
  des.symTop = knob("PJ_HPSI_SYMTOP", 5000);
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
  // (the rows switched off: their dampers taken as holding, their strings left out of the
  // sympathetic ringing, not computed. PJ_HPSI_OFFROWS 1: ringing along, as before.)
  static const bool kOffRows = knob("PJ_HPSI_OFFROWS", 0) != 0;
  if (!kOffRows)
    for (int c = 0; c < kChoirs; ++c)
      for (int n = 0; n < kKeys; ++n) m_.piano().sympathetic(c * kKeys + n, on_[c]);
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
  // The manuals, by the director's terraces (PJ_HPSI_MANUALS, 1): a two-manual harpsichord's
  // forte ritornellos on the lower manual, the two 8' coupled (and the 4', the 16' where drawn);
  // its echoes and quiet episodes on the upper, the front 8' alone, brighter and thinner. The
  // director's level holds through a phrase, so the hands change keyboards only between them.
  static const bool kManuals = knob("PJ_HPSI_MANUALS", 1) > 0;
  bool use[kChoirs];
  for (int c = 0; c < kChoirs; ++c) use[c] = on_[c] || (hold_ && c >= 2);
  if (kManuals && cued_ && on_[0] && on_[1] && cue_.v[kCueLevel] < 0.46f) {
    std::fill(use, use + kChoirs, false);
    use[1] = true;
  }
  int row = 0;
  for (int i = 0; i < kChoirs; ++i) {
    const int c = order[i];
    if (!use[c]) continue;
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
    m_.piano().setVoicing(std::exp2(brightness_ - 1) * hardness_);  // (the plectra's voicing)
    m_.process(out + i, m);
  }
  blowing_ = !m_.piano().asleep() || !due_.empty();
}

}  // namespace pajupilli
