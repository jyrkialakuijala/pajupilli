#include "band.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {

namespace {
bool any(const char* s, std::initializer_list<const char*> names) {
  for (const char* n : names)
    if (std::strcmp(s, n) == 0) return true;
  return false;
}
// How each family's note falls while it rings (dB/s; research/fretless_bass.md 7.4: a piano
// about -8, a TinePiano -6, vibes with the motor off -3), and whether it rings on after its key.
double fallOf(int fam) {
  switch (fam) {
    case kBandPiano: return 8;
    case kBandPluck: return 6;
    case kBandMallet: return 3;
    case kBandBell: return 4;
    case kBandBass: return 5;
    default: return 0;
  }
}
bool ringsOn(int fam) { return fam == kBandBell; }
}  // namespace

bool& jazzBandFile() {
  static bool jazz = false;
  return jazz;
}

int bandFamily(const char* family, const char* name) {
  if (!family) return kBandOther;
  if (any(family, {"drum", "cymbal", "tom", "taiko", "block", "agogo", "sfx", "fx"})) return kBandNone;
  if (any(family, {"piano", "harpsichord"})) return kBandPiano;
  if (any(family, {"guitar", "banjo", "koto", "sitar", "shamisen", "dulcimer"})) return kBandPluck;
  if (any(family, {"harp", "kalimba", "music box", "bells", "bell", "celesta", "glockenspiel", "pan"})) return kBandBell;
  if (any(family, {"vibraphone", "marimba", "xylophone"})) return kBandMallet;
  if (any(family, {"organ", "tonewheel", "reed organ", "accordion", "harmonica", "bandoneon", "bagpipe"})) return kBandOrgan;
  if (any(family, {"choir", "choir-singer", "voice"})) return kBandChoir;
  if (any(family, {"flute", "recorder", "panflute", "whistle", "ocarina", "shakuhachi", "bottle"})) return kBandFlute;
  if (any(family, {"strings", "violin", "viola", "cello", "fiddle", "orchestra"})) return kBandStrings;
  if (any(family, {"brass", "trumpet", "trombone", "tuba", "horn", "brass-section"})) return kBandBrass;
  if (std::strcmp(family, "clarinet") == 0) return kBandClarinet;
  if (any(family, {"sax", "oboe", "bassoon", "duduk", "shawm", "shehnai"})) return kBandReed;
  if (std::strcmp(family, "bass") == 0) return name && std::strstr(name, "synth") ? kBandOther : kBandBass;
  return kBandOther;
}

double bandPartial(int fam, double f0, int k) {
  if (k < 1 || k * f0 > 5000) return 0;
  switch (fam) {
    case kBandChoir: {  // (Ensemble::choirTune's)
      const double band = std::log2(k * f0 / 700);
      return (1.0 / k) * (1 + 2 * std::exp(-band * band / 0.8));
    }
    case kBandFlute: return k <= 3 ? (k == 1 ? 1.0 : k == 2 ? 0.5 : 0.3) : 0.1 / k;
    case kBandBrass: return k * f0 < 2000 ? 1 / std::sqrt(double(k)) : 0.5 / k;
    case kBandClarinet: return k % 2 ? 1.0 / k : k * f0 < 1500 ? 0.1 / k : 0.6 / k;
    case kBandMallet: return k == 1 ? 1.0 : k == 4 ? 0.15 : 0.0;
    case kBandBell: return k == 1 ? 1.0 : 0.5 / k;
    case kBandBass: {
      // (FluidR3's recorded fretless, E1 at 30 ms: dB under the fundamental, research/fretless_bass.md 7.3)
      static const double kDb[16] = {0, -13, -11, -11, -12, -13, -15, -17, -19, -22, -24, -27, -31, -35, -38, -44};
      return k <= 16 ? std::pow(10.0, kDb[k - 1] / 20) : 0.0;
    }
    default: return 1.0 / k;
  }
}

void SoundingSet::on(int ch, int key, int pitch, int velocity, int fam, int ear) {
  if (fam == kBandNone) return;
  for (BandNote& n : n_)
    if (n.ch == ch && n.key == key) {  // (struck again: anew)
      n.pitch = pitch;
      n.at = t_;
      n.byEar = ear > 0;
      n.wind = ear == 2;
      n.level = std::pow(std::clamp(velocity, 1, 127) / 127.0, 1.5);
      n.down = true;
      n.kept = false;
      ++version_;
      return;
    }
  BandNote b;
  b.ch = ch;
  b.key = key;
  b.pitch = pitch;
  b.level = std::pow(std::clamp(velocity, 1, 127) / 127.0, 1.5);
  b.fam = fam;
  b.fall = fallOf(fam);
  b.at = t_;
  b.byEar = ear > 0;
  b.wind = ear == 2;
  n_.push_back(b);
  ++version_;
}

void SoundingSet::off(int ch, int key) {
  for (size_t i = 0; i < n_.size(); ++i) {
    BandNote& n = n_[i];
    if (n.ch != ch || n.key != key || !n.down) continue;
    n.down = false;
    if ((pedalDown(ch) && n.fall > 0) || ringsOn(n.fam)) {
      n.kept = true;
      ++version_;
    } else {
      n_.erase(n_.begin() + i);
      ++version_;
    }
    return;
  }
}

void SoundingSet::pedal(int ch, bool down) {
  if (ch < 0 || ch >= 16) return;
  pedal_[ch] = down;
  if (down) return;
  // (The pedal lifts: what it kept is damped.)
  const size_t before = n_.size();
  n_.erase(std::remove_if(n_.begin(), n_.end(), [&](const BandNote& n) { return n.ch == ch && n.kept && !ringsOn(n.fam); }), n_.end());
  if (n_.size() != before) ++version_;
}

void SoundingSet::step(double dt) {
  t_ += dt;
  const size_t before = n_.size();
  for (BandNote& n : n_)
    if (n.fall > 0) n.level *= std::pow(10.0, -n.fall * dt / 20);
  // (-40 dB under a velocity-127 note: gone)
  n_.erase(std::remove_if(n_.begin(), n_.end(), [](const BandNote& n) { return n.level < 0.01; }), n_.end());
  if (n_.size() != before) ++version_;
  // The winds' common healing (drift()).
  double w = 0, wc = 0;
  bool winds = false;
  for (const BandNote& n : n_) {
    const double lv = std::min(1.0, 4 * n.level);
    if (n.wind) { winds = true; wc += lv * n.earShare * n.cents; }
    if (n.wind || !n.byEar) w += lv;
  }
  static const double kTau = knob("PJ_WIND_EAR_HEAL", 1.0);  // (s)
  drift_ = winds && w > 0 && kTau > 0 ? -wc / w / kTau : 0.0;
}

void SoundingSet::setCents(int ch, int key, double cents) {
  for (BandNote& n : n_)
    if (n.ch == ch && n.key == key) n.cents = cents;
}

const BandNote* SoundingSet::find(int ch, int pitch) const {
  const BandNote* oct = nullptr;
  for (const BandNote& n : n_) {
    if (n.ch != ch) continue;
    if (n.pitch == pitch) return &n;
    if (!oct && (n.pitch - pitch) % 12 == 0) oct = &n;
  }
  return oct;
}

void SoundingSet::setEarShare(int ch, int key, double share) {
  for (BandNote& n : n_)
    if (n.ch == ch && n.key == key) n.earShare = share;
}

double bandPull(const SoundingSet& band, const BandListener& me, double* weight, double* earWeight) {
  *weight = 0;
  double swe = 0;
  const int np = std::min(me.np, 32);
  double pw[32];
  double pow2 = 0;
  for (int i = 0; i < np; ++i) {
    pw[i] = me.pw[i];
    pow2 += pw[i] * pw[i];
  }
  for (int i = 0; i < np; ++i) pw[i] /= std::sqrt(pow2);
  const double f0 = me.f0;
  double sw = 0, sd = 0;
  for (const BandNote& o : band.notes()) {
    if (o.ch == me.self) continue;
    // (A part tuning by ear is heard only by the notes after it: the order above.)
    if (o.byEar && me.at >= 0) {
      const bool before = o.at < me.at - 0.035 ||
                          (o.at <= me.at + 0.035 && (o.pitch < me.pitch || (o.pitch == me.pitch && o.ch < me.self)));
      if (!before) continue;
    }
    const double g0 = 440 * std::exp2((o.pitch - 69) / 12.0 + o.cents / 1200);
    double qw[24], qp = 0;
    int nq = 0;
    for (int k = 1; k <= 24; ++k) {
      qw[k - 1] = bandPartial(o.fam, g0, k);
      qp += qw[k - 1] * qw[k - 1];
      if (qw[k - 1] > 0) nq = k;
    }
    if (nq == 0) continue;
    // (A faded note pulls less: the beating's depth.)
    const double lv = std::min(1.0, 4 * o.level) / std::sqrt(qp);
    const double rel = 1200 * std::log2(g0 / f0);  // where its fundamental lies against ours (c)
    for (int i = 0; i < np; ++i)
      for (int k = 1; k <= nq; ++k) {
        if (qw[k - 1] <= 0) continue;
        const double delta = rel + 1200 * (std::log2(double(k)) - me.pl[i]);
        if (std::fabs(delta) > 36) continue;
        const double kq = std::max(me.pk[i], double(k));
        const double w = std::min(pw[i], qw[k - 1] * lv) * std::exp(-(delta / 20) * (delta / 20)) / (kq * kq);
        sw += w;
        sd += w * delta;
        if (o.byEar) swe += w;
      }
  }
  *weight = sw;
  if (earWeight) *earWeight = swe;
  return sw > 0 ? sd / sw : 0;
}

}  // namespace pajupilli
