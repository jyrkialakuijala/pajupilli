#include "jazzsinger.h"

#include "band.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <mutex>
#include <string>

#include <sys/wait.h>
#include <thread>
#include <unistd.h>

#include "knob.h"
#include "singer_data.h"
#include "puhuja/puhuja.h"

namespace pajupilli {

namespace {

using puhuja::Ph;
using puhuja::Synth;

constexpr double kFs = 48000;
constexpr double kLn10 = 2.302585092994046;  // (10^x as exp(kLn10 x): a decibel value to a gain without std::pow)
// Where the voice holds its pitch (measured, the male voice with the tightened controller: the
// middle of every vowel within 13 c from D2 to C4; above, the folds leave the pitch asked).
// The voices (the author, 2026-11-26: the folds' mass chosen by the singer's range, and changed in a
// controlled way to get higher). From bass to soprano the folds are shorter and lighter (a bass's
// about 21 mm, a baritone's 19.5, a tenor's 18.4; a contralto's 16.6, a mezzo's 16.0, a soprano's
// 14.9, and thinner): `mass` x the model's fold mass. Above the voice's first passaggio (kneeHz)
// less and less of the fold swings, in the end its thin edge alone: the mass x (kneeHz / pitch)^
// power from there up (puhuja's setFoldMass; a countertenor is a man's folds swinging at their
// edges almost from the bottom of his range). tractCm: the tract longer (+) or shorter (-) than the
// speaker's, with the low larynx. lo .. hi: the range it is given parts in (MIDI keys).
// woman: where the voice lies between puhuja's male voice (0) and its female one (1) (the author,
// 2026-11-26: a woman's whole head is smaller, the nasal cavities too; and it is a continuum: a
// soprano a little past the usual woman, 1.1 female - 0.1 male, a mezzo the woman herself, an alto
// 0.9; the men likewise, a bass a little past the usual man). puhuja's female voice has the
// resonances 12 to 15 % higher, her glottal chink, nasal passage and output; tractCm then only
// trims the length.
// The registers (the author's table, 2026-11-26): each voice has a chest range, a crossover zone
// between two pivots and a head range above it:
//   bass       chest E2-G3,  crossover G3-C4,  head C4-F4      baritone  G2-B3,  B3-E4,  E4-A4
//   tenor      chest C3-D4,  crossover D4-G4,  head G4-C5      contralto F3-G4,  G4-C5,  C5-F5
//   mezzo      chest A3-B4,  crossover B4-E5,  head E5-A5      soprano   C4-D5,  D5-G5,  G5-C6
// kneeHz is the lower pivot (the primo passaggio: to there the whole fold swings), knee2Hz the
// upper (the secondo); through the zone the mass falls as (kneeHz / pitch)^2, the stiffness no
// longer growing, and above it as (knee2Hz / pitch)^1.5. The whole fold has to carry the pitch up
// to the lower pivot, so a voice whose chest goes higher has lighter folds: the model's folds at
// mass 1 hold to about D4, at 0.5 to G#4, at 0.25 to C5 (measured), hence the women's 0.45 to
// 0.18 here, lighter than their anatomy alone says. (An earlier search of mass, pivot and power,
// -singer-voices, found every setting tried singing every note, the misses a few cents apart.)
struct VoiceType { const char* name; int lo, hi; float mass, kneeHz, power, tractCm; float woman = 0; float knee2Hz = 0; };
const VoiceType kVoices[] = {
    // (a basso profondo: the longest and heaviest folds, the longest tract; down to A1)
    {"profondo", 33, 57, 1.4f, 165, 2.0f, 1.4f, -0.15f, 220},
    {"bass", 40, 65, 1.15f, 196, 2.0f, 0.9f, -0.1f, 262},
    {"baritone", 38, 67, 1.0f, 247, 2.0f, 0.6f, 0, 330},
    {"tenor", 48, 72, 0.8f, 294, 2.0f, 0.2f, 0.1f, 392},
    {"countertenor", 52, 76, 0.9f, 165, 2.0f, 0.2f, 0.1f, 220},
    {"contralto", 53, 77, 0.45f, 392, 2.0f, 0.6f, 0.9f, 523},
    {"mezzo", 57, 81, 0.28f, 494, 2.0f, 0.4f, 1.0f, 659},
    {"soprano", 60, 84, 0.16f, 587, 2.0f, 0.3f, 1.1f, 784},
    {"coloratura", 60, 89, 0.18f, 622, 2.0f, 0.2f, 1.15f, 831},
};
constexpr int kNVoices = int(sizeof(kVoices) / sizeof(kVoices[0]));
constexpr int kMapPad = 4;  // (the tension is learnt this many semitones past the range, both ways)

double knobD(const char* name, double def) { return knob(name, def); }

Ph phOf(char c) {
  switch (c) {
    case 'd': return Ph::D;
    case 'b': return Ph::B;
    case 't': return Ph::T;
    case 'p': return Ph::P;
    case 'k': return Ph::K;
    case 'g': return Ph::G;
    case 'l': return Ph::L;
    case 'w': return Ph::W;
    case 'j': return Ph::J;
    case 'n': return Ph::N;
    case 'm': return Ph::M;
    case 's': return Ph::S;
    case 'z': return Ph::Z;
    case 'S': return Ph::SH;
    case 'r': return Ph::R;
    case 'I': return Ph::IH;
    case 'v': return Ph::VV;
    case 'h': return Ph::H;
    case 'f': return Ph::F;
    case 'R': return Ph::ER;
    case 'a': return Ph::AA;
    case 'u': return Ph::U;
    case 'o': return Ph::O;
    case 'i': return Ph::I;
    case 'e': return Ph::E;
    case '@': return Ph::Schwa;
    default: return Ph::Stop;
  }
}

const char* spell(char nucleus) {
  switch (nucleus) {
    case 'a': return "ah";
    case 'u': return "oo";
    case 'o': return "oh";
    case 'i': return "ee";
    case 'e': return "eh";
    case '@': return "a";
    case 'E': return "ey";
    case 'l': return "l";
    case 'n': return "n";
    case 'm': return "m";
    case 'I': return "ih";
    case 'Y': return "oy";
    case 'W': return "ow";
    case 'R': return "er";
    default: return "?";
  }
}

// The voice as the singer uses it: puhuja's default speaker (string folds, line tract), with the
// singer's pitch control and larynx.
const VoiceType* gVoiceTry = nullptr;  // (the practice: a voice's folds being tried out)
void voice(Synth& s, const VoiceType& vt, const std::vector<float>* map, float larynxCm = 0) {
  static const bool kWoman = knobD("PJ_SINGER_WOMAN", 1) > 0;
  static const float kBlendAdd = float(knobD("PJ_SINGER_BLEND_ADD", 0));  // (a trial knob: the voice moved along the male-female line)
  if (vt.woman != 0 && kWoman) s.setVoiceBlend(vt.woman + kBlendAdd);
  s.setCarrier(puhuja::Carrier::String);
  s.setFormantMotion(puhuja::FormantMotion::Planned);
  s.setTract(puhuja::Tract::Line);
  s.setSpellingRules(false);
  s.setFricationGain(4.5f);
  s.setDarkL(true);
  s.setAspiration(true);
  // (the ear: 0.2 of the error a cycle, learning wherever the folds swing at all; the speaker's
  // 98 cm^3/s never engaged at sung pitches. It was 0.6: in running notes that overshot after
  // every change of note; measured on a soprano line 2026-11-26, the median miss 20 c at 0.6,
  // 12 c at 0.3, 9 c at 0.15, 16 c at 0.05)
  static const float kShare = float(knobD("PJ_SINGER_EAR", 0.2)), kVib = float(knobD("PJ_SINGER_EAR_VIB", 3));
  s.setPitchControl(kShare, kVib);
  // (the larynx low and still: 0.6 cm longer a tract, no rise in s k g r; to be set by ear)
  static const float kLarynxLow = float(knobD("PJ_SINGER_LARYNX_LOW", 0.6)), kLarynxRaise = float(knobD("PJ_SINGER_LARYNX_RAISE", 0));
  s.setLarynx(vt.tractCm + (kLarynxLow - 0.6f) + larynxCm, kLarynxRaise);
  static const float kMassX = float(knobD("PJ_SINGER_FOLD_MASS", 1)), kPower = float(knobD("PJ_SINGER_FOLD_POWER", 0));
  static const float kKneeX = float(knobD("PJ_SINGER_KNEE_X", 1)), kPower2 = float(knobD("PJ_SINGER_FOLD_POWER2", 1.5));
  s.setFoldMass(vt.mass * kMassX, vt.kneeHz * kKneeX, kPower > 0 ? kPower : vt.power, vt.knee2Hz * kKneeX, kPower2);
  // The folds shielded from half of the mouth's sound pressure (2026-11-26): where the sung pitch met the vowel's
  // first resonance ("ah" at G#5 and A5, "ee" at E4, "oo" at F4) the folds broke into a rough swing a semitone
  // flat, which no mass, closure or breath cured. PJ_SINGER_SHIELD=-1: the folds as the speaker has them.
  // (0.5 at 3000 Hz at first; the author: "sounds much better, but still a little screechy ... perhaps go down to
  // 25 %": with a quarter of the pressure left on the folds and the corner at 8000 Hz no held "ah" or "ee" from
  // C4 to D6 is rough any more; what is left, "oo" at E5 to F#5, the practice sings with the mouth opened.)
  static const float kShield = float(knobD("PJ_SINGER_SHIELD", 0.75)), kShieldHz = float(knobD("PJ_SINGER_SHIELD_HZ", 8000));
  s.setTractShield(kShield, kShieldHz);
  // (a held note holds still: the pitch line planned with little smoothing, or it wandered 20 to 30 cents)
  static const float kPitchEps = float(knobD("PJ_SINGER_PITCH_EPS", 0.0002));
  s.setPitchSmoothing(kPitchEps);
  // (the speaker's slow random pitch drift, about 20 cents: a third of it left; with the lighter folds of the
  // soprano, 0.16 for 0.2, the middle of her long notes wanders 4 cents for 8 and her vowels are 3 dB cleaner)
  static const float kDrift = float(knobD("PJ_SINGER_DRIFT", 0.3));
  s.setPitchDrift(kDrift);
  if (map && !map->empty()) s.setTensionMap(float(440 * std::exp2((vt.lo - kMapPad - 69) / 12.0)), *map);
}

// The sound as the ear weighs it (the A curve: 100 Hz 19 dB under 1 kHz, 200 Hz 11 dB, 400 Hz 5 dB):
// a low voice's strong fundamental counts for little of its loudness, so levels are matched on
// this and not on the plain signal (the author, 2026-11-26: the baritone sang the same line less
// loud than the tenor). Four high-passes (20.6 Hz twice, 107.7 Hz, 737.9 Hz) and two low-passes
// (12.2 kHz), each first-order, 0 dB at 1 kHz.
std::vector<float> weighed(const std::vector<float>& x) {
  std::vector<double> y(x.begin(), x.end());
  double gain = 1;
  auto section = [&](double hz, bool high) {
    const double k = std::tan(M_PI * hz / kFs), a = (1 - k) / (1 + k), b = high ? 1 / (1 + k) : k / (1 + k);
    double x1 = 0, y1 = 0;
    for (double& v : y) {
      const double in = v;
      v = high ? b * (in - x1) + a * y1 : b * (in + x1) + a * y1;
      x1 = in;
      y1 = v;
    }
    const double r = 1000 / hz;  // (the analog section's gain at 1 kHz)
    gain *= high ? r / std::sqrt(1 + r * r) : 1 / std::sqrt(1 + r * r);
  };
  section(20.6, true); section(20.6, true); section(107.7, true); section(737.9, true);
  section(12194, false); section(12194, false);
  std::vector<float> out(y.size());
  for (size_t i = 0; i < y.size(); ++i) out[i] = float(y[i] / gain);
  return out;
}

std::vector<float> pullAll(Synth& s) {
  std::vector<float> all, buf(4800);
  for (;;) {
    const size_t n = s.pull(buf.data(), buf.size());
    all.insert(all.end(), buf.begin(), buf.begin() + n);
    if (n < buf.size()) break;
  }
  return all;
}

// What the singer knows: the folds' tension by pitch. Learnt once a run by singing a scale on "dah"
// and on "doo" with the ear alone and keeping, for each pitch, the correction the ear settled on
// (read at 85 % of the vowel). The scale is sung twice: once to learn when each vowel sounds (the
// phonemes' times are known only afterwards), once to read the correction then; puhuja renders
// the same input the same way. (PJ_SINGER_KNOWN=0: the ear alone.)
int gLearnRounds = 1;  // (the practice: 3)
bool practised();
struct Known { std::vector<float> tension, levelDb; };  // by key, from the voice's lo - kMapPad
const Known& knownOf(int vi) {
  static Known known[kNVoices];
  static std::once_flag once[kNVoices];
  std::call_once(once[vi], [vi] {
    const VoiceType& vt = kVoices[vi];
    std::vector<float>& map = known[vi].tension;
    const int kMapLow = vt.lo - kMapPad, kMapHigh = vt.hi + kMapPad;
    const int n = kMapHigh - kMapLow + 1;
    std::vector<double> lsum(n, 0.0);
    std::vector<int> lcnt(n, 0);
    std::vector<double> sum(n, 0.0);
    std::vector<double> cnt(n, 0.0);
    // (practised, -singer-practice: read from the table, nothing to learn now)
    if (gLearnRounds == 1 && practised()) {
      bool any = false;
      known[vi].levelDb.assign(n, -34.0f);
      map.assign(n, 1.0f);
      for (const SingerKnown* r = kSingerKnown; r->voice >= 0; ++r)
        if (r->voice == vi && r->key >= kMapLow && r->key <= kMapHigh) {
          map[r->key - kMapLow] = r->tension;
          known[vi].levelDb[r->key - kMapLow] = r->levelDb;
          any = true;
        }
      if (any) {
        if (knobD("PJ_SINGER_KNOWN", 1) <= 0) map.clear();
        return;
      }
    }
    // (learnt in rounds: the first by ear alone; each next one sung with what the last taught, the
    // ear's leftover correction then added to it. One round leaves the notes starting flat or
    // sharp and crawling to pitch; the practice takes three.)
    std::vector<float> prior;
    for (int round = 0; round < gLearnRounds; ++round) {
    std::fill(sum.begin(), sum.end(), 0.0); std::fill(cnt.begin(), cnt.end(), 0.0);
    std::fill(lsum.begin(), lsum.end(), 0.0); std::fill(lcnt.begin(), lcnt.end(), 0);
    // (practised as it will be sung, by the spectra: every third note 15 c high, every third as
    // much low, the other vowel's the other way round; each reading shared between the two
    // semitones about its pitch)
    auto offGrid = [](int k, Ph v) { return 0.15f * float((k + (v == Ph::U ? 1 : 0)) % 3 - 1); };
    for (const Ph v : {Ph::AA, Ph::U}) {
      auto scale = [&](Synth& s) {
        voice(s, vt, prior.empty() ? nullptr : &prior);
        const int base = (kMapLow + kMapHigh) / 2;
        s.setBaseF0(float(440 * std::exp2((base - 69) / 12.0)));
        for (int k = 0; k < n; ++k) {
          s.setWord(k + 1);
          s.push(Ph::D, true);
          Synth::Shape sh;
          sh.ms = 300;
          s.shapeNext(sh);
          s.setPitch(float(kMapLow + k - base) + offGrid(k, v));
          s.setPitchEnd(float(kMapLow + k - base) + offGrid(k, v));
          s.push(v);
        }
        s.finish();
      };
      Synth a{float(kFs)};
      scale(a);
      const std::vector<float> heard = weighed(pullAll(a));
      std::vector<double> at(n, -1);
      for (const Synth::Label& l : a.labels())
        if (l.word >= 1 && l.word <= n && !l.wordStart) {
          at[l.word - 1] = l.start + 0.85 * (l.end - l.start);
          // (how loud the voice is on this pitch: its level over the vowel's second half)
          const size_t i0 = size_t((l.start + 0.5 * (l.end - l.start)) * kFs), i1 = std::min(heard.size(), size_t((l.start + 0.9 * (l.end - l.start)) * kFs));
          double e = 0;
          for (size_t i = i0; i < i1; ++i) e += double(heard[i]) * heard[i];
          if (i1 > i0) { lsum[l.word - 1] += 10 * std::log10(e / (i1 - i0) + 1e-12); ++lcnt[l.word - 1]; }
        }
      Synth b{float(kFs)};
      scale(b);
      std::vector<float> buf(4800);
      size_t done = 0;
      for (int k = 0; k < n; ++k) {
        if (at[k] < 0) continue;
        const size_t to = size_t(at[k] * kFs);
        while (done < to) {
          const size_t got = b.pull(buf.data(), std::min(buf.size(), to - done));
          done += got;
          if (got == 0) break;
        }
        const float c = b.pitchCorrection();
        if (c > 0.6f && c < 1.7f) {
          const float o = offGrid(k, v);
          const int k2 = std::clamp(k + (o > 0 ? 1 : -1), 0, n - 1);
          sum[k] += (1 - std::fabs(o)) * c; cnt[k] += 1 - std::fabs(o);
          sum[k2] += std::fabs(o) * c; cnt[k2] += std::fabs(o);
        }
      }
    }
    known[vi].levelDb.assign(n, -34.0f);
    for (int k = 0; k < n; ++k)
      if (lcnt[k]) known[vi].levelDb[k] = float(lsum[k] / lcnt[k]);
    map.assign(n, 1.0f);
    for (int k = 0; k < n; ++k)
      if (cnt[k] > 0) map[k] = float(sum[k] / cnt[k]);
    // (a pitch never read: its neighbours'; then smoothed over three semitones, the ear's own
    // unrest taken out)
    std::vector<float> sm(map);
    for (int k = 0; k < n; ++k) {
      const float l = map[std::max(k - 1, 0)], r = map[std::min(k + 1, n - 1)];
      sm[k] = 0.25f * l + 0.5f * map[k] + 0.25f * r;
    }
    map = sm;
    if (!prior.empty())
      for (int k = 0; k < n; ++k) map[k] *= prior[k];
    prior = map;
    }  // (round)
    if (knobD("PJ_SINGER_KNOWN", 1) <= 0) map.clear();  // (the ear alone)
    if (std::getenv("PJ_SINGER_TRACE")) {
      std::fprintf(stderr, "singer %s: weighed level by key from %d:", vt.name, kMapLow);
      for (int k = 0; k < n; k += 3) std::fprintf(stderr, " %.0f", known[vi].levelDb[k]);
      std::fprintf(stderr, " dB\n");
    }
  });
  return known[vi];
}
const std::vector<float>& tensionMap(int vi) { return knownOf(vi).tension; }
// How loud the voice comes out on a key by itself (dB, puhuja's scale), as learnt with the tension.
float levelOf(int vi, int key) {
  const std::vector<float>& l = knownOf(vi).levelDb;
  return l[size_t(std::clamp(key - (kVoices[vi].lo - kMapPad), 0, int(l.size()) - 1))];
}

uint32_t mix(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
  return x;
}


// What was practised (src/singer_data.h): a note's trim by its vowel (the key's own, else the nearest
// key's), and the way into a note by the interval (the nearest practised: its size first, then
// where it starts). kind 0: slurred, 1: direct.
bool practised() {
  static const bool on = knobD("PJ_SINGER_PRACTISED", 1) > 0;
  return on;
}
std::vector<SingerTrim> gTrimsNow;  // (the practice: the notes just learnt, used while the intervals are practised)
// What was practised of a note on a vowel (the key's own row, else the nearest key's within a minor third), or null.
const SingerTrim* noteOf(int vi, int key, char vowel) {
  const SingerTrim* best = nullptr;
  int bestD = 4;
  if (!gTrimsNow.empty()) {
    for (const SingerTrim& t : gTrimsNow)
      if (t.voice == vi && t.vowel == vowel && std::abs(t.key - key) < bestD) { bestD = std::abs(t.key - key); best = &t; }
    return best;
  }
  for (const SingerTrim* t = kSingerTrim; t->vowel; ++t)
    if (t->voice == vi && t->vowel == vowel && std::abs(t->key - key) < bestD) { bestD = std::abs(t->key - key); best = t; }
  return best;
}
bool wayOf(int vi, int kind, int from, int interval, JazzSinger::Way* w) {
  const SingerWay* best = nullptr;
  int bestD = 1 << 30;
  for (const SingerWay* r = kSingerWay; r->kind >= 0; ++r) {
    if (r->voice != vi || r->kind != kind || (r->interval > 0) != (interval > 0)) continue;
    const int d = 4 * std::abs(r->interval - interval) + std::abs(r->from - from);
    if (d < bestD) { bestD = d; best = r; }
  }
  if (!best) return false;
  *w = JazzSinger::Way{best->leaveMs, best->arriveMs, best->over, best->overMs, best->dipDb};
  return true;
}

}  // namespace

std::string JazzSinger::words() const {
  std::string out;
  for (const Phrase& p : phrase_) {
    for (int i = p.first; i < p.first + p.count; ++i) {
      const Syllable& y = syl_[i];
      if (i > p.first) out += ' ';
      for (char c : y.cons)
        if (c) out += c == 'j' ? std::string("y") : c == 'S' ? std::string("sh") : std::string(1, c);
      out += spell(y.nucleus);
      if (y.coda) out += y.coda;
    }
    out += '\n';
  }
  return out;
}

void JazzSinger::setScore(std::vector<SungNote> notes, std::vector<double> others, std::vector<HeardNote> heard, Style style) {
  // One voice: of the notes beginning together the highest; a note ends where the next begins.
  std::stable_sort(notes.begin(), notes.end(), [](const SungNote& a, const SungNote& b) { return a.on < b.on; });
  note_.clear();
  for (const SungNote& n : notes) {
    if (n.off <= n.on) continue;
    if (!note_.empty() && n.on - note_.back().on < 0.02) {
      if (n.key > note_.back().key) note_.back() = n;
      continue;
    }
    if (!note_.empty()) note_.back().off = std::min(note_.back().off, n.on);
    note_.push_back(n);
  }
  others_ = std::move(others);
  std::sort(others_.begin(), others_.end());
  phrase_.clear();
  syl_.clear();
  at_ = 0;
  next_ = 0;
  head_ = 0;
  lastEnd_ = -1e9;
  ahead_.clear();
  if (note_.empty()) return;
  plan(style);
  // (the harmony under each note: the other parts' pitch classes sounding with it; and how busy they are)
  for (SungNote& sn : note_) {
    unsigned classes = 0;
    for (const HeardNote& h : heard)
      if (h.on < sn.off - 0.02 && h.off > sn.on + 0.02) classes |= 1u << (((h.pitch % 12) + 12) % 12);
    int count = 0;
    for (unsigned c = classes; c; c &= c - 1) ++count;
    sn.harm = float(std::clamp((count - 1) / 4.0, 0.0, 1.0));
    sn.busy = float((std::upper_bound(others_.begin(), others_.end(), sn.on + 1) - std::lower_bound(others_.begin(), others_.end(), sn.on - 1)) / 2.0);
  }
  tune(std::move(heard));
  // Buoyed (the author, 2026-11-26): a singer leans to the top side of the pitch, which the ear takes
  // as bright and alive where the same amount flat is tired and out of tune: PJ_SINGER_BUOY_C (5)
  // cents up on every note, except one the spectra have pulled 6 cents or more down (a major third
  // locking under its chord, which pushed sharp would beat).
  {
    static const float kBuoy = float(knobD("PJ_SINGER_BUOY_C", 5));
    for (SungNote& sn : note_)
      if (sn.cents > -6) sn.cents += kBuoy;
  }
  if (std::getenv("PJ_SINGER_TRACE"))
    for (const SungNote& n : note_)
      std::fprintf(stderr, "singer cue: t=%.3f key %d %s level %.2f -> %.2f swell %.2f at %.2f, vibrato x%.2f from %.2f s, rate x%.2f\n", n.on, n.key,
                   n.cued ? "directed" : "own", n.level, n.levelEnd, n.swell, n.swellPeak, n.vibDepth, n.vibDelay, n.vibRate);
}

std::vector<float> JazzSinger::tuning() const {
  std::vector<float> c;
  for (const SungNote& n : note_) c.push_back(n.cents);
  return c;
}

// The tuning by the spectra: the other parts' notes played through a band (src/band.h: their
// levels falling as their families' do), and each sung note heard against it a little after its
// start (60 ms, or its middle: the notes struck with it are then sounding): the pull of its own
// partials (the choir's template) towards theirs, followed by its weight as the winds follow it
// (src/windear.cc). One tuning a note: the pitch is set when the note is rendered.
void JazzSinger::tune(std::vector<HeardNote> heard) {
  const char* name = std::getenv("PJ_SINGER_TUNING");
  if (name && std::strcmp(name, "jyrki-spectral") != 0) return;  // (tet12, or anything else: tempered)
  if (heard.empty()) return;
  static const double kLim = knobD("PJ_SINGER_TUNE_LIM", 15);
  constexpr double kHalf = 0.0005, kFloor = 3e-4;  // (the winds': src/windear.cc)
  struct Ev { double t; bool on; int i; };
  std::vector<Ev> ev;
  for (int i = 0; i < int(heard.size()); ++i) {
    if (heard[i].off <= heard[i].on) continue;
    ev.push_back({heard[i].on, true, i});
    ev.push_back({heard[i].off, false, i});
  }
  std::stable_sort(ev.begin(), ev.end(), [](const Ev& a, const Ev& b) { return a.t < b.t; });
  SoundingSet band;
  size_t e = 0;
  double now = 0;
  auto to = [&](double t) {
    if (t > now) { band.step(t - now); now = t; }
  };
  for (SungNote& n : note_) {
    const double at = n.on + std::min(0.06, 0.5 * (n.off - n.on));
    for (; e < ev.size() && ev[e].t <= at; ++e) {
      to(ev[e].t);
      const HeardNote& h = heard[ev[e].i];
      if (ev[e].on) band.on(h.ch, h.pitch, h.pitch, h.vel, h.fam);
      else band.off(h.ch, h.pitch);
    }
    to(at);
    constexpr int kN = 12;
    double pl[kN], pw[kN], pk[kN];
    const double f0 = 440 * std::exp2((n.key - 69) / 12.0);
    for (int k = 0; k < kN; ++k) {
      pl[k] = std::log2(k + 1.0);
      pw[k] = bandPartial(kBandChoir, f0, k + 1);
      pk[k] = k + 1;
    }
    BandListener me;
    me.f0 = f0;
    me.pl = pl; me.pw = pw; me.pk = pk; me.np = kN;
    me.pitch = n.key;
    double w = 0;
    const double pull = bandPull(band, me, &w);
    n.cents = w >= kFloor ? float(std::clamp(pull * w / (w + kHalf), -kLim, kLim)) : 0.0f;
    if (std::getenv("PJ_SINGER_TRACE"))
      std::fprintf(stderr, "singer tune: t=%.3f key %d pull %+.1f c (w %.5f) -> %+.1f c\n", n.on, n.key, pull, w, n.cents);
  }
}

namespace {
// The scat singers' words (the author's lists, 2026-11-26: the common syllables every singer uses,
// the swing and bebop connectors, the percussive ones, the quick bebop clusters; and Louis
// Armstrong's, hard stops at the front of the mouth on open vowels). cls: how rare, 1 common ..
// 4 rare (the rarer the more the play has to be up; the classes beyond, the drum-kit sounds,
// lip trills, growls and clicks, have no voice here yet). The spelling is the singer's own code,
// a syllable a note: consonants (S: sh, j: y), a nucleus (a ah, u oo, o oh, i ee, e eh, I ih,
// @ uh, E ey, Y oy, W ow, R er; or the last consonant, sung: "dl", "dn", "dm"), a closing consonant.
// who: singing is cultural, and has its code of what a man and a woman sing (the author, 2026-11-26):
// the men's voices favour Louis Armstrong's words ('L': heavy, hard stops at the front of the mouth, "sk",
// "br", open vowels, the horn's "wah", "zoot", "bwee"), the women's Ella Fitzgerald's ('E': d and b on "oo"
// and "ee", "dn", "dm", "dl", the light quick clusters); 'B': everyone's.
struct LexWord { int cls; char who; const char* text; };
const LexWord kLex[] = {
    {1, 'B', "du"}, {1, 'B', "ba"}, {1, 'B', "da"}, {1, 'B', "bi"}, {1, 'B', "di"}, {1, 'B', "bop"}, {1, 'B', "d@p"}, {1, 'B', "dot"}, {1, 'B', "la"}, {1, 'B', "ja"},
    {1, 'B', "bo"}, {1, 'L', "So"}, {1, 'B', "bu"}, {1, 'B', "bap"}, {1, 'B', "dIt"}, {1, 'B', "du bi"}, {1, 'B', "du da"}, {1, 'B', "ba da"}, {1, 'B', "di da"},
    {2, 'B', "Su bi"}, {2, 'B', "du wa"}, {2, 'B', "bIp"}, {2, 'B', "dap"}, {2, 'L', "skit"}, {2, 'L', "skat"}, {2, 'L', "zut"}, {2, 'L', "vut"}, {2, 'B', "bi bop"},
    {2, 'L', "ri bop"}, {2, 'B', "up"}, {2, 'L', "dwi"}, {2, 'L', "bwi"}, {2, 'E', "tu dl"}, {2, 'B', "wa"}, {2, 'L', "zup"}, {2, 'L', "Su"}, {2, 'L', "lo"}, {2, 'L', "li"},
    {2, 'L', "lu"}, {2, 'L', "jo"}, {2, 'L', "ji"}, {2, 'E', "du dl a"}, {2, 'E', "bu bi"}, {2, 'E', "di dI"},
    {3, 'E', "dm"}, {3, 'E', "bdn"}, {3, 'E', "dja"}, {3, 'L', "bja"}, {3, 'L', "bdW"}, {3, 'B', "gdY"}, {3, 'E', "sbop"}, {3, 'L', "kbop"}, {3, 'B', "fdu"}, {3, 'L', "dW"},
    {3, 'L', "if"}, {3, 'L', "gaf"}, {3, 'L', "m bop"}, {3, 'L', "bR"}, {3, 'E', "zYt"},
    {4, 'E', "du dl ot"}, {4, 'E', "skI bi"}, {4, 'E', "zI bi"}, {4, 'E', "bdu dl"}, {4, 'L', "kdut"}, {4, 'L', "tbop"}, {4, 'E', "dl ot"}, {4, 'L', "brIp"},
    {4, 'L', "brip"}, {4, 'L', "skip"}, {4, 'L', "skup"}, {4, 'L', "skId dat di dat"}, {4, 'L', "rIp bop di bop di du"}, {4, 'L', "zup di dup di bop"},
    {4, 'L', "di di di di da ba"}, {4, 'E', "bI bi bI bi du dap"},
};
constexpr int kLexN = int(sizeof(kLex) / sizeof(kLex[0]));
const std::vector<std::vector<JazzSinger::Syllable>>& lexicon() {
  static const std::vector<std::vector<JazzSinger::Syllable>> words = [] {
    std::vector<std::vector<JazzSinger::Syllable>> all;
    for (const LexWord& w : kLex) {
      std::vector<JazzSinger::Syllable> word;
      for (const char* p = w.text; *p;) {
        std::string tok;
        while (*p && *p != ' ') tok += *p++;
        while (*p == ' ') ++p;
        JazzSinger::Syllable y;
        size_t nuc = tok.find_first_of("auoie@IEYWR");
        if (nuc == std::string::npos) nuc = tok.size() - 1;  // (a sung consonant: "dl", "dn", "m")
        for (size_t q = 0; q < nuc && q < 2; ++q) y.cons[q] = tok[q];
        y.nucleus = tok[nuc];
        y.coda = nuc + 1 < tok.size() ? tok[nuc + 1] : 0;
        word.push_back(y);
      }
      all.push_back(word);
    }
    return all;
  }();
  return words;
}
}  // namespace

void JazzSinger::plan(Style style) {
  const int n = int(note_.size());
  // The voice and the octave: the voice asked for (setVoice), else the one whose range holds the
  // most of the part; moved by octaves only if the part lies outside every voice (or the one
  // asked for), and then as little as can be; a note still outside, by its own octave. A
  // transposition asked for (setShift) is kept, and its notes sung where they fall.
  {
    double best = 1e30;
    int bestV = voice_ >= 0 ? voice_ : 2, bestK = shift_;
    for (int vi = 0; vi < kNVoices; ++vi) {
      if (voice_ >= 0 && vi != voice_) continue;
      for (int k = -48; k <= 24; k += 12) {
        const int sh = asked_ ? shift_ : k;
        double out = 0, far = 0;
        for (const SungNote& s : note_) {
          const int key = s.key + sh;
          out += (s.off - s.on) * std::max({0, kVoices[vi].lo - key, key - kVoices[vi].hi});
          far += (s.off - s.on) * std::abs(key - (kVoices[vi].lo + kVoices[vi].hi) / 2);
        }
        double len = 0;
        for (const SungNote& s : note_) len += s.off - s.on;
        const double cost = 1000 * out + 200 * len * std::abs(sh) / 12 + far;
        if (cost < best) { best = cost; bestV = vi; bestK = sh; }
        if (asked_) break;
      }
    }
    voice_ = bestV;
    shift_ = bestK;
    const int kLow = kVoices[voice_].lo, kHigh = kVoices[voice_].hi;
    for (SungNote& s : note_) {
      s.key += shift_;
      // (a transposition asked for: sung where it falls, within what a voice can try at all)
      if (asked_) s.key = std::clamp(s.key, 28, 96);
      while (!asked_ && s.key > kHigh) s.key -= 12;
      while (!asked_ && s.key < kLow) s.key += 12;
      outside_ += s.key < kLow || s.key > kHigh;
    }
  }
  // Phrases: a breath at the director's phrase ends, at a rest over 0.35 s, and in a long line at
  // the first rest over 0.1 s past 6 s (none by 10 s: before the next note).
  for (int i = 0; i < n; ++i) {
    bool fresh = i == 0;
    if (!fresh) {
      const double gap = note_[i].on - note_[i - 1].off, len = note_[i].on - note_[phrase_.back().first].on;
      fresh = note_[i - 1].phraseEnd || gap > 0.35 || (len > 6 && gap > 0.1) || len > 10;
    }
    if (fresh) phrase_.push_back(Phrase{i, 0, 0, 0, {}, false});
    ++phrase_.back().count;
  }
  // How the part moves: its usual time from note to note, its usual loudness, its range.
  std::vector<double> iois;
  std::vector<int> vels;
  int lo = 127, hi = 0;
  for (int i = 0; i < n; ++i) {
    if (i + 1 < n && note_[i + 1].on - note_[i].on < 2) iois.push_back(note_[i + 1].on - note_[i].on);
    vels.push_back(note_[i].vel);
    lo = std::min(lo, note_[i].key);
    hi = std::max(hi, note_[i].key);
  }
  std::sort(iois.begin(), iois.end());
  std::sort(vels.begin(), vels.end());
  const double usual = iois.empty() ? 0.4 : iois[iois.size() / 2];
  const int usualVel = vels[vels.size() / 2];
  if (style == kAuto) {
    const char* e = std::getenv("PJ_SINGER_STYLE");
    const std::string s = e ? e : "";
    style = s == "jazz" ? kJazz : s == "ballad" ? kBallad : s == "bebop" ? kBebop : s == "baroque" ? kBaroque
            : s == "classical" ? kClassical : s == "romantic" ? kRomantic : s == "folk" ? kFolk
            : usual > 0.5 ? kBallad : usual < 0.2 ? kBebop : kJazz;
  }
  style_ = style;
  // The play (the author, 2026-11-26: "the lead-voice can try to work out its playful scatting
  // through a differential equation -- there should be some ideas of contrast, especially if
  // there is the same phrase sung many times, it should be something like mf, p, f, or with
  // different colors and accentuations"). Two things swing through the song, each a damped
  // spring pulled towards what the music has there: the level (x: + louder) and the colour
  // (c: + brighter):  x'' = -w^2 (x - u) - 2 z w x',  u from the notes' loudness and height.
  // Lightly damped (z 0.25), so each overshoots and comes back: after loud, soft. Every phrase
  // gives them a push away from where they are, the harder the more often that very phrase (the
  // same intervals in the same rhythm) has been sung already: its second time comes out the other
  // way from its first, its third back past it. The pushes' size is the mischief (m), which
  // chooses the playful syllables. A phrase's every second repeat also moves its accents off the
  // beat. PJ_SINGER_PLAY (1): how far it all goes; 0: none.
  play_.assign(n, 0.0f);
  bright_.assign(n, 0.0f);
  mischief_.assign(n, 0.0f);
  flip_.assign(n, 0);
  again_.assign(n, 0);
  playV_.assign(n, 0.0f);
  brightV_.assign(n, 0.0f);
  {
    static const double kPlay = knobD("PJ_SINGER_PLAY", 1);
    std::map<std::string, int> sung;
    double x = 0, xv = 0, c = 0, cv = 0, m = 0, t = note_.empty() ? 0 : note_[0].on;
    const double wx = 2 * M_PI / 9.0, wc = 2 * M_PI / 13.0, z = 0.25;
    for (size_t k = 0; k < phrase_.size() && kPlay > 0; ++k) {
      const int a = phrase_[k].first, b = a + phrase_[k].count;
      std::string sig;
      for (int i = a + 1; i < b; ++i) {
        sig += char('m' + std::clamp(note_[i].key - note_[i - 1].key, -12, 12));
        sig += char('m' + std::clamp(int(std::lround(2 * std::log2(std::max(0.03, note_[i].on - note_[i - 1].on) / usual))), -6, 6));
      }
      const int again = phrase_[k].count >= 3 ? sung[sig]++ : 0;
      // (the push: away from where it is; a phrase heard before, harder, and its colour the other way round each time)
      const double push = 0.35 + 0.45 * std::min(again, 3);
      xv += -(x >= 0 ? 1 : -1) * push * wx * (again > 0 || std::fabs(x) > 0.15 ? 1.0 : 0.3);
      cv += (again % 2 ? -1 : 1) * (c >= 0 ? -1 : 1) * 0.8 * push * wc;
      m = std::min(1.0, 0.5 * m + 0.35 * again + 0.25 * std::fabs(x));
      for (int i = a; i < b; ++i) {
        const double high = hi > lo ? double(note_[i].key - lo) / (hi - lo) : 0.5;
        const double u = 0.8 * (note_[i].vel / 127.0 - usualVel / 127.0) + 0.3 * (high - 0.5);
        for (double left = std::max(0.0, note_[i].on - t); left > 0;) {  // (stepped in 20 ms: semi-implicit Euler)
          const double h = std::min(left, 0.02);
          xv += h * (-wx * wx * (x - u) - 2 * z * wx * xv); x += h * xv;
          cv += h * (-wc * wc * (c - 0.6 * u) - 2 * z * wc * cv); c += h * cv;
          left -= h;
        }
        t = std::max(t, note_[i].on);
        play_[i] = float(kPlay * std::clamp(x, -1.0, 1.0));
        bright_[i] = float(kPlay * std::clamp(c, -1.0, 1.0));
        mischief_[i] = float(kPlay * m);
        flip_[i] = again % 2;
        again_[i] = char(std::min(again, 100));
        playV_[i] = float(xv);
        brightV_[i] = float(cv);
      }
    }
    // (the level it plays at: on top of the director's, about 4 dB either way at the most)
    for (int i = 0; i < n; ++i) {
      note_[i].level = std::clamp(note_[i].level + 0.3f * play_[i], 0.0f, 1.0f);
      note_[i].levelEnd = std::clamp(note_[i].levelEnd + 0.3f * play_[i], 0.0f, 1.0f);
    }
  }
  syl_.assign(n, Syllable());
  std::vector<bool> fixed(n, false);
  for (const Phrase& p : phrase_) {
    const int a = p.first, b = p.first + p.count;
    auto ioi = [&](int i) { return i + 1 < b ? note_[i + 1].on - note_[i].on : note_[i].off - note_[i].on; };
    auto bound = [&](int i) { return i > a && note_[i].on - note_[i - 1].off < 0.03; };  // legato from the note before
    char lastV = 0;
    int sameV = 0;
    int lastWord = -1;
    int quickAt = 0, quickPat = 0;
    static const double kPlainUnder = knobD("PJ_SINGER_PLAIN_UNDER", 0.2);  // (s: notes shorter than this are sung plainly; 0: none)
    const bool womanSings = kVoices[voice_].woman >= 0.5f;
    for (int i = a; i < b; ++i) {
      if (fixed[i]) { lastV = syl_[i].nucleus; continue; }
      Syllable& y = syl_[i];
      const SungNote& s = note_[i];
      const double d = ioi(i), loud = s.vel / 127.0;
      const double high = hi > lo ? double(s.key - lo) / (hi - lo) : 0.5;
      // (the accompaniment around the note: the other parts' notes a second, over two seconds)
      const double busy = (std::upper_bound(others_.begin(), others_.end(), s.on + 1) -
                           std::lower_bound(others_.begin(), others_.end(), s.on - 1)) / 2.0;
      const uint32_t r = mix(uint32_t(i) * 2654435761u + 17);
      const bool first = i == a, last = i == b - 1, leg = bound(i);
      const bool ghost = !first && s.vel < 0.6 * usualVel && d < 0.3;
      const double energy = std::clamp(0.5 * loud + 0.3 * high + 0.2 * std::clamp(double(s.accent), -1.0, 1.0) +
                                       (busy > 8 ? 0.1 : 0.0) + 0.25 * bright_[i], 0.0, 1.0);
      const double mis = mischief_[i];
      const bool urgent = loud > 0.85 && (busy > 6 || s.accent > 0.5);
      auto vowelBy = [&]() -> char {  // the colour by the note's energy
        // (the scat singers' own: "doo", "dee", "dih", "boo", "bee" on d and b; quick notes go
        // "doo-dee" in swing and "dee-dih" in bebop; "ah" for the open, loud ones)
        if (d < 0.2 && style != kBallad && !womanSings) return (i - a) % 4 == 3 ? 'a' : energy < 0.5 ? ((i - a) % 2 ? 'i' : 'u') : 'i';  // (his: "de-de-de-dah")
        if (d < 0.2 && style != kBallad) return style == kBebop ? ((i - a) % 2 ? 'I' : 'i') : energy < 0.5 ? ((i - a) % 2 ? 'i' : 'u') : ((i - a) % 2 ? 'I' : 'i');
        if (energy < 0.42) return (r >> 3) % 3 == 0 ? 'o' : 'u';
        if (energy < 0.68) return (r >> 3) % 3 == 0 ? 'u' : 'a';
        return d < 0.25 ? 'i' : urgent && d > 0.4 ? 'E' : 'a';
      };
      if (style == kBaroque) {  // la, ti, di: pure a and e
        y.cons[0] = d < 0.22 ? ((i - a) % 2 ? 't' : 'd') : 'l';
        y.nucleus = d < 0.22 ? 'i' : (r % 4 == 0 ? 'e' : 'a');
      } else if (style == kClassical) {  // one pure vowel, begun with an l, a note not bound taking it again
        y.cons[0] = leg ? 0 : 'l';
        y.nucleus = hi - lo > 0 && high > 0.8 ? 'a' : 'a';
        if (energy < 0.35) y.nucleus = 'o';
      } else if (style == kRomantic) {  // open a, o on liquids and nasals
        y.cons[0] = leg ? 0 : first ? 'l' : "lmln"[r % 4];
        y.nucleus = energy < 0.5 ? 'o' : 'a';
      } else if (style == kFolk) {  // diddle-um
        if (i + 2 < b && d < 0.2 && ioi(i + 1) < 0.2 && !fixed[i + 1]) {
          y.cons[0] = 'd'; y.nucleus = 'i';
          syl_[i + 1].cons[0] = 'd'; syl_[i + 1].nucleus = 'l'; fixed[i + 1] = true;
          if (!fixed[i + 2]) { syl_[i + 2].cons[0] = 0; syl_[i + 2].nucleus = '@'; syl_[i + 2].coda = 'm'; fixed[i + 2] = true; }
        } else {
          y.cons[0] = 'd';
          y.nucleus = d < 0.25 ? 'i' : 'a';
          if (last && d > 0.3) { y.nucleus = '@'; y.coda = 'm'; }
        }
      } else {
        // The shortest notes plainly (the author's ear, 2026-11-26: with every rare word and bright
        // vowel on them they "feel a bit nervous and there is a plenty of them"): the commonest
        // scat, "doo-bah-dah" and its turns, one consonant and one open vowel a note, no stops, no
        // clusters. A run keeps one pattern; the next run, or the phrase sung again, another.
        if (d < kPlainUnder) {
          static const char* const kRunC[] = {"dbd", "dbd", "ddb", "bdd"};
          static const char* const kRunV[] = {"uaa", "aau", "uaa", "aua"};
          if (!(i > a && ioi(i - 1) < kPlainUnder)) { quickAt = 0; quickPat = int(mix(uint32_t(i) * 7919u + 13u * uint32_t(again_[i])) >> 5) % 4; }
          y.cons[0] = first ? 'd' : kRunC[quickPat][quickAt % 3];
          // (most of them with the mouth left open, the air let out: no closing consonant; three in
          // four after a run's first, the vowel going straight on, with an h where the pitch repeats)
          if (quickAt > 0 && !first && (mix(uint32_t(i) * 2654435761u + 5u) >> 6) % 4 != 0) y.cons[0] = s.key == note_[i - 1].key ? 'h' : 0;
          y.nucleus = kRunV[quickPat][quickAt % 3];
          ++quickAt;
          lastV = y.nucleus;
          continue;
        }
        // Jazz. The scat singers' own words first (the lexicon above the function), when one fits.
        {
          double total = 0;
          int pick = -1;
          const uint32_t rr = mix(uint32_t(i) * 40503u + 977u * uint32_t(again_[i]) + 31u);  // (a phrase sung again draws anew)
          std::vector<std::pair<int, double>> cand;
          for (int e = 0; e < kLexN; ++e) {
            const std::vector<Syllable>& w = lexicon()[e];
            const int k = int(w.size()), cls = kLex[e].cls;
            if (i + k > b) continue;
            // (how often: the common words always, the rarer the more the play is up; a ballad keeps to the common, bebop likes the quick clusters)
            double wt = cls == 1 ? 1.0 : cls == 2 ? 0.15 + 0.6 * mis : cls == 3 ? 0.35 * mis : 0.45 * mis * mis + (style == kBebop ? 0.12 : 0.0);
            if (style == kBallad && cls > 2) continue;
            if (style == kBallad) wt *= cls == 1 ? 1.0 : 0.3;
            bool ok = true;
            double col = 0;
            for (int q = 0; q < k && ok; ++q) {
              const Syllable& y2 = w[q];
              const int at = i + q;
              const double dq = ioi(at);
              if (fixed[at]) ok = false;
              const bool sungCons = y2.nucleus == 'l' || y2.nucleus == 'n' || y2.nucleus == 'm';
              const bool stop = y2.coda == 'p' || y2.coda == 't' || y2.coda == 'f';
              if (sungCons && dq > 0.2) ok = false;                      // a "dl", a "dn": a quick note
              if (stop && dq > 0.4) ok = false;                           // a "bop": short
              if (stop && at + 1 < b && bound(at + 1) && q == k - 1 && dq > 0.22) ok = false;  // ... and not bound on
              if (y2.cons[1] && at > a && ioi(at - 1) < 0.13) ok = false; // two consonants need the time
              if (y2.cons[1] == 'w' && !(at > a && note_[at].key > note_[at - 1].key)) ok = false;  // "dwee", "bwee": up into it
              if (!y2.cons[0] && y2.nucleus == 'u' && y2.coda == 'p' && !(at > a && note_[at].key > note_[at - 1].key)) ok = false;  // "oop"
              if (y2.nucleus == 'W' && !(at + 1 >= b || note_[at + 1].key < note_[at].key || dq > 0.4)) ok = false;  // "dow": falling, or held
              if (!y2.cons[0] && q == 0 && !bound(at) && at != a) ok = false;  // no consonant: bound to the note before
              if (y2.cons[0] == 'l' || y2.cons[0] == 'j' || y2.cons[0] == 'w') { if (!bound(at) && dq < 0.25) ok = false; }  // "lah", "yah", "wah": unhurried
              if (k >= 3 && q + 1 < k && dq > 0.3) ok = false;            // the long words run
              const char v = y2.nucleus;
              const double vb = v == 'i' || v == 'I' || v == 'e' ? 0.85 : v == 'a' || v == 'E' || v == 'Y' ? 0.6 : v == 'u' || v == 'o' || v == 'W' ? 0.25 : 0.4;
              const double hq = hi > lo ? double(note_[at].key - lo) / (hi - lo) : 0.5;
              col += std::fabs(std::clamp(0.5 * note_[at].vel / 127.0 + 0.3 * hq + 0.2 + 0.25 * bright_[at], 0.0, 1.0) - vb);
            }
            if (!ok) continue;
            wt *= std::max(0.1, 1 - 1.3 * col / k) * (e == lastWord ? 0.25 : 1.0) * (k > 1 ? 1.6 : 1.0);
            if (kLex[e].who != 'B') wt *= (kLex[e].who == 'E') == womanSings ? 2.0 : 0.2;
            cand.push_back({e, wt});
            total += wt;
          }
          // (a word is taken three times in four when any fits; else the figures and the rules below)
          if (total > 0 && (rr >> 3) % 4 != 0) {
            double u = total * ((rr >> 8) % 4096) / 4096.0;
            for (const auto& cnd : cand) {
              u -= cnd.second;
              if (u <= 0) { pick = cnd.first; break; }
            }
            if (pick < 0) pick = cand.back().first;
          }
          if (pick >= 0) {
            const std::vector<Syllable>& w = lexicon()[pick];
            for (size_t q = 0; q < w.size(); ++q) { syl_[i + q] = w[q]; fixed[i + q] = true; }
            if (first && !syl_[i].cons[0]) syl_[i].cons[0] = 'd';
            lastWord = pick;
            lastV = syl_[i].nucleus;
            continue;
          }
        }
        // The figures the line itself asks for.
        // "doo-dl-ah": a quick note between two longer ones is the tongue's flick.
        if (!first && !last && d < 0.16 && ioi(i - 1) > 1.6 * d && ioi(i + 1) > 1.6 * d && !fixed[i + 1] && syl_[i - 1].nucleus != 'l' &&
            syl_[i - 1].nucleus != 'n') {
          y.cons[0] = 'd'; y.nucleus = 'l';
          syl_[i + 1].cons[0] = 0; syl_[i + 1].nucleus = lastV == 'a' ? 'u' : 'a'; fixed[i + 1] = true;
          lastV = 'l';
          continue;
        }
        if (ghost) {  // a ghost note: swallowed, a nasal
          y.cons[0] = 'd'; y.nucleus = (r >> 9) % 3 == 0 ? 'm' : 'n';  // ("dn", sometimes "dm")
          lastV = 'n';
          continue;
        }
        // Playing (the mischief up): "bibby bibby doo dap", b, p and d taking turns over a run of
        // four or more quick notes; "zoyt" on a short accented note before a rest.
        if (womanSings && mis > 0.45 && d < 0.2 && i + 3 < b && ioi(i + 1) < 0.2 && ioi(i + 2) < 0.2 && !fixed[i + 1] && !fixed[i + 2] && !fixed[i + 3]) {
          static const char kC[] = "bbbbdd", kV[] = "IiIiua";
          int q = 0;
          for (; q < 6 && i + q < b && (q < 4 || ioi(i + q) < 0.25) && !fixed[i + q]; ++q) {
            syl_[i + q].cons[0] = kC[q]; syl_[i + q].nucleus = kV[q];
            fixed[i + q] = true;
          }
          if (q == 6 && i + 6 < b ? note_[i + 6].on - note_[i + 5].off > 0.1 : q == 6) syl_[i + 5].coda = 'p';
          lastV = syl_[i].nucleus;
          continue;
        }
        if (womanSings && mis > 0.6 && d < 0.3 && (s.accent > 0.2 || loud > 0.7) && i + 1 < b && note_[i + 1].on - s.off > 0.15 && (r >> 11) % 3 == 0) {
          y.cons[0] = 'z'; y.nucleus = 'Y'; y.coda = 't';
          lastV = 'o';
          continue;
        }
        // The consonant: the articulation.
        const bool fastLine = d < 0.2 || (i > a && ioi(i - 1) < 0.2);
        if (first) y.cons[0] = urgent ? 'b' : 'd';
        else if (style == kBallad && leg) y.cons[0] = (r % 3) ? 0 : "lwj"[(r >> 4) % 3];  // a ballad: mostly the vowels
        else if (leg && !fastLine) y.cons[0] = "lwj"[(r >> 4) % 3];  // bound and unhurried: liquids and glides
        else if (urgent) y.cons[0] = "bkg"[(r >> 5) % 3];
        else if (fastLine) y.cons[0] = style == kBebop ? (s.accent > 0.3 ? "tp"[r % 2] : (i - a) % 4 == 3 ? "blr"[(r >> 8) % 3] : 'd') : ((i - a) % 2 ? 'b' : 'd');
        else if (mis > 0.5 && loud > 0.7 && d > 0.4 && (r >> 12) % 4 == 0) y.cons[0] = 'S';  // ("shah": a horn's breath)
        else if (s.accent > 0.3 || loud > 0.8) y.cons[0] = 'b';
        else y.cons[0] = busy < 2 && energy < 0.45 ? "dlm"[(r >> 6) % 3] : 'd';  // (a thin accompaniment, softly: softer consonants)
        // The vowel: the colour.
        char v = vowelBy();
        if (style == kBebop && d < 0.25 && v == 'u') v = (r >> 7) % 2 ? 'i' : 'a';  // bebop: bright vowels
        if (style == kBallad && v == 'i') v = 'a';
        // "doo-wee": up a fourth or more, bound.
        if (!first && s.key - note_[i - 1].key >= 5 && (leg || y.cons[0] == 'w') && d > 0.15) { y.cons[0] = 'w'; v = 'i'; }
        // "doo-bah": a long note answered by a stronger one.
        else if (!first && lastV == 'u' && d > 0.25 && (s.vel > note_[i - 1].vel + 6 || s.key > note_[i - 1].key) && y.cons[0] == 'd') {
          y.cons[0] = 'b'; v = 'a';
        }
        // "dah-ba": a short note after a long one, thrown away.
        else if (!first && d < 0.22 && ioi(i - 1) > 1.8 * d && lastV == 'a') { y.cons[0] = 'b'; v = '@'; }
        // "bop", "dat": an accented short note before a rest, closed by a stop (jazz, bebop).
        if ((style == kJazz || style == kBebop) && d < 0.3 && (s.accent > 0.3 || s.beat >= 0.75f || loud > 0.75) && i + 1 < b &&
            note_[i + 1].on - s.off > 0.12 && (y.cons[0] == 'b' || y.cons[0] == 'd')) {
          y.coda = y.cons[0] == 'b' ? 'p' : 't';
          v = y.cons[0] == 'b' ? 'o' : 'a';
        }
        // (not one colour on and on: after three notes of it, the next)
        if (v == lastV && ++sameV >= 3) { v = v == 'a' ? 'u' : v == 'u' ? 'a' : v == 'i' ? 'a' : 'u'; }
        if (v != lastV) sameV = 0;
        y.nucleus = v;
      }
      lastV = y.nucleus;
    }
  }
  weigh(style);
}

// The syllables into words, and each word's weighted syllable (src/jazzsinger.h).
void JazzSinger::weigh(Style style) {
  const int n = int(note_.size());
  weight_.assign(n, 0.0f);
  wordStart_.assign(n, 0);
  const bool latin = style == kBaroque || style == kClassical || style == kRomantic;
  std::vector<double> lens;
  for (const SungNote& s : note_) lens.push_back(s.off - s.on);
  std::sort(lens.begin(), lens.end());
  const double usual = lens.empty() ? 0.3 : lens[lens.size() / 2];
  auto reduced = [&](int i) { const char v = syl_[i].nucleus; return v == '@' || v == 'l' || v == 'n' || v == 'm'; };
  for (const Phrase& p : phrase_) {
    const int a = p.first, b = p.first + p.count;
    // Words: a new one after a rest, at a stop consonant on a beat, and after three syllables; a
    // reduced syllable, or one bound to the note before by a liquid, a glide or no consonant,
    // belongs to the word before it.
    int inWord = 0;
    for (int i = a; i < b; ++i) {
      const char c = syl_[i].cons[0];
      const bool stop = c == 'd' || c == 'b' || c == 't' || c == 'p' || c == 'k' || c == 'g';
      const bool rest = i > a && note_[i].on - note_[i - 1].off > 0.06;
      const bool onBeat = note_[i].beat < 0 ? (i - a) % 2 == 0 : note_[i].beat >= 0.5f;
      const bool fresh = i == a || rest || inWord >= (style == kFolk ? 4 : 3) || (!reduced(i) && stop && onBeat && inWord >= 1);
      if (fresh) { wordStart_[i] = 1; inWord = 0; }
      ++inWord;
    }
    for (int w0 = a; w0 < b;) {
      int w1 = w0 + 1;
      while (w1 < b && !wordStart_[w1]) ++w1;
      const int len = w1 - w0;
      int stress = w0;
      if (len == 1) {
        stress = reduced(w0) ? -1 : w0;
      } else if (latin) {
        // (two syllables: the first; more: the one before the last if long (its note at least the
        // part's usual length, and no shorter than the one before it), else the one before that)
        if (len >= 3) {
          const int pen = w1 - 2;
          const double lp = note_[pen].off - note_[pen].on, la = note_[pen - 1].off - note_[pen - 1].on;
          stress = lp >= usual && lp >= la ? pen : pen - 1;
        }
      } else {
        // (as speech: a full-vowel syllable, the one on the strongest beat; then the melody's peak,
        // then the longer; never the last of the word if another can carry it)
        double best = -1e9;
        stress = -1;
        for (int i = w0; i < w1; ++i) {
          if (reduced(i)) continue;
          double beat = note_[i].beat < 0 ? ((i - w0) % 2 == 0 ? 0.5 : 0.2) : note_[i].beat;
          if (flip_[i]) beat = 1.2 - beat;  // (the phrase again: its accents off the beat this time)
          const bool peak = (i == w0 || note_[i].key > note_[i - 1].key) && (i + 1 >= w1 || note_[i].key >= note_[i + 1].key);
          const double sc = 4 * beat + (peak ? 1.0 : 0.0) + std::min(1.0, (note_[i].off - note_[i].on) / (2 * usual)) + 0.8 * note_[i].accent -
                            (i == w1 - 1 ? 1.5 : 0.0);
          if (sc > best) { best = sc; stress = i; }
        }
      }
      for (int i = w0; i < w1; ++i) {
        const double beat = note_[i].beat < 0 ? 0.5 : note_[i].beat;
        float w;
        if (i == stress) w = float(0.7 + 0.3 * beat);            // the word's weight, the more on a strong beat
        else if (reduced(i)) w = 0;                               // light and reduced
        else if (i == w1 - 1 && len > 1) w = 0;                   // a word's last syllable: no new push
        else w = note_[i].beat >= 1 ? 0.6f : 0.25f;               // (the bar's first beat carries some weight anyway)
        weight_[i] = w;
      }
      // (a phrase's last syllable above all, unless it is all there is of its word)
      if (w1 == b && len > 1 && b - 1 != stress) weight_[b - 1] = 0;
      w0 = w1;
    }
  }
  // (the shortest notes carry no stress of their own: no scoop, no pulse, no pressed folds on a
  // note that is over in a fifth of a second; off the bar's first beat they are plain)
  for (int i = 0; i < n; ++i)
    if (note_[i].off - note_[i].on < 0.2 && note_[i].beat < 1 && weight_[i] > 0.5f) weight_[i] = 0.5f;
  // The ends (the author, 2026-11-26): the last syllable of a phrase inside the piece is sung on a
  // little less breath than a plain one (0.3 against 0.4), whatever its word gave it; the piece's
  // very last note has a little emphasis.
  for (size_t k = 0; k < phrase_.size(); ++k) {
    const int last = phrase_[k].first + phrase_[k].count - 1;
    if (k + 1 == phrase_.size()) weight_[last] = std::max(weight_[last], 0.75f);
    else weight_[last] = 0.3f;
  }
  // Swing lays its heavy syllables a little behind the beat (the rolling feel of the New Orleans
  // singers): a weighted syllable of a jazz line PJ_SINGER_LAYBACK_MS (18) late, where its note
  // has the room.
  static const double kLaybackMs = knobD("PJ_SINGER_LAYBACK_MS", 18) * 1e-3;
  if (style == kJazz && kLaybackMs > 0)
    for (int i = 0; i < n; ++i)
      if (weight_[i] >= 0.7f && note_[i].off - note_[i].on > 0.2) {
        const double kLayback = (kVoices[voice_].woman >= 0.5f ? 0.6 : 1.3) * kLaybackMs;  // (his heavier, further back)
        note_[i].on += kLayback;
        if (i > 0 && note_[i - 1].off > note_[i].on - 0.03) note_[i - 1].off = std::max(note_[i - 1].off, note_[i].on);
      }
  // The vowel on the beat (the author, 2026-11-26: "the singer should not try to start the consonant at the note
  // begin, but earlier -- note begin is when the vowel should already ring", and: "only start early if there is a
  // consonant to sing first, and try to be that much early as needed to reach the first vowel"). The consonant
  // already lies before the written start and the vowel begins on it; but after a consonant the voice is not
  // there when its vowel begins: in a song it reached half its level 14 ms (soprano) to 28 ms (baritone) after
  // the written start. So a note that begins with a consonant is sung that much sooner (PJ_SINGER_EARLY_MS: 15
  // for a woman's voice, 28 for a man's, x PJ_SINGER_EARLY); a note with none, a slur or a bare vowel, is not moved.
  static const double kEarlyX = knobD("PJ_SINGER_EARLY", 1), kEarlyMs = knobD("PJ_SINGER_EARLY_MS", 0);
  if (kEarlyX > 0 && int(syl_.size()) == n) {
    const double e = kEarlyX * (kEarlyMs > 0 ? kEarlyMs : kVoices[voice_].woman >= 0.5f ? 15.0 : 28.0) * 1e-3;
    for (int i = 0; i < n; ++i) {
      if (!syl_[i].cons[0]) continue;
      const double on = std::max(note_[i].on - e, i > 0 ? note_[i - 1].on + 0.03 : 0.0);
      if (on >= note_[i].on) continue;
      note_[i].on = on;
      if (i > 0 && note_[i - 1].off > on) note_[i - 1].off = on;
    }
  }
}

bool JazzSinger::setVoice(const std::string& name) {
  for (int vi = 0; vi < kNVoices; ++vi)
    if (name == kVoices[vi].name || (name == "alto" && std::strcmp(kVoices[vi].name, "contralto") == 0) ||
        (name == "basso-profondo" && std::strcmp(kVoices[vi].name, "profondo") == 0)) { voice_ = vi; return true; }
  return false;
}
const char* JazzSinger::voiceName() const { return voice_ >= 0 ? kVoices[voice_].name : "?"; }
std::string JazzSinger::range() const {
  static const char* kN[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
  if (voice_ < 0) return "";
  auto nm = [](int k) { return std::string(kN[k % 12]) + std::to_string(k / 12 - 1); };
  return nm(kVoices[voice_].lo) + "-" + nm(kVoices[voice_].hi);
}

std::string JazzSinger::weighted() const {
  std::string out;
  for (const Phrase& p : phrase_) {
    for (int i = p.first; i < p.first + p.count; ++i) {
      std::string y;
      for (char c : syl_[i].cons)
        if (c) y += c == 'j' ? std::string("y") : c == 'S' ? std::string("sh") : std::string(1, c);
      y += spell(syl_[i].nucleus);
      if (syl_[i].coda) y += syl_[i].coda;
      if (weight_[i] >= 0.7f)
        for (char& c : y) c = char(std::toupper(static_cast<unsigned char>(c)));
      if (i > p.first) out += wordStart_[i] ? " " : "-";
      out += y;
    }
    out += "\n";
  }
  return out;
}

// One rendering of a phrase with the given lengths (ms): each note's sung part, and the rest after
// it. Returns the sound; vowelAt: when each note's sung part began in it (s); lead: the first's.
std::vector<float> JazzSinger::take(const Phrase& p, const std::vector<double>& vowelMs, const std::vector<double>& restMs,
                                    const std::vector<double>& sungAt, std::vector<double>* vowelAt, double* lead) const {
  Synth s{float(kFs)};
  // The school (the author, 2026-11-26: the same apparatus, shaped by the tradition). The classical
  // European way: the larynx low, the tract long, dark and round, a vibrato all through the note.
  // The way jazz has from the African traditions: the larynx at speech level (the tract
  // PJ_SINGER_JAZZ_LARYNX cm, -0.5, shorter: brighter, forward, the words' own vowels), the tone
  // straight and locked, the vibrato an ornament late in a note. (Not here yet: the twang of a
  // narrowed epilarynx, and nasal colour.)
  static const float kJazzLarynx = float(knobD("PJ_SINGER_JAZZ_LARYNX", -0.5));
  const bool jazzSchool = style_ == kJazz || style_ == kBebop || style_ == kBallad || style_ == kFolk;
  if (gVoiceTry) voice(s, *gVoiceTry, nullptr);
  else voice(s, kVoices[voice_], &tensionMap(voice_), jazzSchool && !noLevel_ ? kJazzLarynx : 0.0f);
  std::vector<int> keys;
  for (int i = p.first; i < p.first + p.count; ++i) keys.push_back(note_[i].key);
  std::sort(keys.begin(), keys.end());
  const int base = keys[keys.size() / 2];
  s.setBaseF0(float(440 * std::exp2((base - 69) / 12.0)));
  static const double kLevelLo = knobD("PJ_SINGER_LEVEL_LO", 0.6), kLevelHi = knobD("PJ_SINGER_LEVEL_HI", 1.4);
  std::vector<int> ncons(p.count, 0);
  // The way into each note from the one before (practised; none after a rest, where a note starts anew).
  std::vector<Way> way(p.count);
  // (trial knobs: how much of a practised note's breath, as a power, and of its mouth opening is used)
  static const double kRowPress = knobD("PJ_SINGER_ROW_PRESS", 1), kRowOpen = knobD("PJ_SINGER_ROW_OPEN", 1);
  std::vector<char> hasWay(p.count, 0);
  std::vector<const SingerTrim*> noteRow(p.count, nullptr), levelRow(p.count, nullptr);
  // What of the practice is used in a song (PJ_SINGER_TABLE). 1, the default: only how loud each note comes out
  // on each vowel sung plainly, which evens the vowels after the voice. 2: also each note's pitch trim, breath
  // and mouth opening and the intervals' ways. Those are practised on half-second notes; on a song's short ones
  // every one of them put the soprano out of tune (median miss 3.5 c without, 13 c with; 2026-11-26).
  static const int kTable = int(knobD("PJ_SINGER_TABLE", 1));
  for (int j = 1; j < p.count; ++j) {
    if (restMs[j - 1] > 1 || note_[p.first + j].key == note_[p.first + j - 1].key) continue;
    const char c = syl_[p.first + j].cons[0];
    const int kind = c == 0 || c == 'l' || c == 'w' || c == 'j' ? 0 : 1;
    if (tryWay_) { way[j] = *tryWay_; hasWay[j] = 1; }
    else if (!plain_ && practised() && kTable >= 2)
      hasWay[j] = wayOf(voice_, kind, note_[p.first + j - 1].key, note_[p.first + j].key - note_[p.first + j - 1].key, &way[j]);
  }
  for (int j = 0; j < p.count; ++j) {
    const SungNote& n = note_[p.first + j];
    const Syllable& y = syl_[p.first + j];
    s.setWord(j + 1);
    bool first = true;
    // Stress is more than breath (the author, 2026-11-26): a weighted syllable has a slightly
    // crisper consonant, a small scoop up into its pitch, its vowel pure and whole; an unweighted
    // one a softer consonant and a vowel relaxed towards the neutral one (PJ_SINGER_REDUCE, 0.35
    // of the way), and it gives a little of its time away at a word's end.
    const double wt = weight_[p.first + j];
    for (char c : y.cons) {
      if (!c) continue;
      Synth::Shape cs;
      cs.level = float(1 + 0.25 * (wt - 0.4));
      // (the closure held a little longer before a weighted syllable: the pressure builds behind it)
      if (style_ == kJazz || style_ == kBebop || style_ == kBallad || style_ == kFolk) cs.length = float(1 + 0.3 * std::clamp(wt - 0.4, -0.3, 0.6));
      s.shapeNext(cs);
      s.push(phOf(c), first);
      first = false;
      ++ncons[j];
    }
    Synth::Shape sh;
    sh.level = float((kLevelLo + (kLevelHi - kLevelLo) * std::pow(n.vel / 127.0, 1.5)) * (1 + 0.15 * std::clamp(double(n.accent), -1.0, 1.0)));
    // (the note as practised on this vowel: how far off it lands, the breath it takes, the vowel opened)
    const SingerTrim* row = (!plain_ && practised()) ? noteOf(voice_, n.key, y.nucleus) : nullptr;
    const SingerTrim* nt = tryNoteSet_ ? tryNote_ : kTable >= 2 ? row : nullptr;
    noteRow[j] = nt;
    levelRow[j] = tryNoteSet_ || kTable < 1 ? nullptr : row;
    static const float kTrimX = float(knobD("PJ_SINGER_TRIM_X", 1));
    const float trim = nt ? nt->cents * (tryNoteSet_ ? 1.0f : kTrimX) : 0.0f;
    const float st = float(n.key - base) + (n.cents + trim) / 100;
    static const double kReduce = knobD("PJ_SINGER_REDUCE", 0.35), kStressDip = knobD("PJ_SINGER_STRESS_SCOOP", 0.3);
    if (wt >= 0.7) sh.pitchDip = float(kStressDip * wt);  // (semitones under the note at its onset)
    auto colour = [&](Ph v) {  // an unweighted syllable's vowel, towards the neutral one
      if (wt >= 0.2 || kReduce <= 0 || v == phOf('@')) { sh.f1 = sh.f2 = sh.f3 = 0; return; }
      const float x = float(kReduce * (1 - wt / 0.2));
      sh.f1 = Synth::tableFormant(v, 0) + x * (Synth::tableFormant(phOf('@'), 0) - Synth::tableFormant(v, 0));
      sh.f2 = Synth::tableFormant(v, 1) + x * (Synth::tableFormant(phOf('@'), 1) - Synth::tableFormant(v, 1));
      sh.f3 = Synth::tableFormant(v, 2) + x * (Synth::tableFormant(phOf('@'), 2) - Synth::tableFormant(v, 2));
    };
    // (the pitch reached 35 ms into the sung part and held to 35 ms before its end: the way to the
    // next note is in the consonant. PJ_SINGER_SCOOP ms: later, a scoop.)
    // (8 ms asked, 2026-11-26: the pitch line rounds what is asked to three or four times its length, so the
    // 35 ms asked before were 120 ms heard: a held note began 80 cents off and slid 100 to 150 cents towards the
    // next one at full voice over its last fifth of a second, a swoop that on a soprano is a screech.)
    static const double kScoop = knobD("PJ_SINGER_SCOOP", 8), kLeave = knobD("PJ_SINGER_LEAVE", 8);
    const double arriveMs = hasWay[j] ? way[j].arriveMs : kScoop, leaveMs = j + 1 < p.count && hasWay[j + 1] ? way[j + 1].leaveMs : kLeave;
    auto hold = [&](double ms) {
      sh.sungAt = float(std::min(0.3, arriveMs / std::max(ms, 1.0)));
      sh.sungEndAt = float(std::max(0.6, 1 - leaveMs / std::max(ms, 1.0)));
    };
    const double codaMs = y.coda ? std::min(90.0, 0.3 * vowelMs[j]) : 0;
    if (y.nucleus == 'W') {  // "ow": ah closing to oo (a brass fall-off, a mute closing)
      sh.ms = float(0.55 * (vowelMs[j] - codaMs));
      hold(sh.ms);
      s.shapeNext(sh); s.setPitch(st); s.setPitchEnd(st);
      s.push(Ph::AA, first);
      sh.ms = float(0.45 * (vowelMs[j] - codaMs));
      hold(sh.ms);
      s.shapeNext(sh); s.setPitch(st); s.setPitchEnd(st);
      s.push(Ph::U, false);
    } else if (y.nucleus == 'Y') {  // "oy": o, then ee
      sh.ms = float(0.55 * (vowelMs[j] - codaMs));
      hold(sh.ms);
      s.shapeNext(sh); s.setPitch(st); s.setPitchEnd(st);
      s.push(Ph::AO, first);
      sh.ms = float(0.45 * (vowelMs[j] - codaMs));
      hold(sh.ms);
      s.shapeNext(sh); s.setPitch(st); s.setPitchEnd(st);
      s.push(Ph::I, false);
    } else if (y.nucleus == 'E') {  // the diphthong: its first part 60 %
      sh.ms = float(0.6 * (vowelMs[j] - codaMs));
      hold(sh.ms);
      colour(Ph::E);
      s.shapeNext(sh); s.setPitch(st); s.setPitchEnd(st);
      s.push(Ph::E, first);
      sh.ms = float(0.4 * (vowelMs[j] - codaMs));
      hold(sh.ms);
      colour(Ph::I);
      s.shapeNext(sh); s.setPitch(st); s.setPitchEnd(st);
      s.push(Ph::I, false);
    } else {
      sh.ms = float(vowelMs[j] - codaMs);
      hold(sh.ms);
      if (y.nucleus != 'l' && y.nucleus != 'n' && y.nucleus != 'm') colour(phOf(y.nucleus));
      s.shapeNext(sh); s.setPitch(st); s.setPitchEnd(st);
      s.push(phOf(y.nucleus), first);
    }
    if (y.coda) {
      Synth::Shape c;
      c.ms = float(codaMs);
      c.level = sh.level;
      s.shapeNext(c);
      s.push(phOf(y.coda), false);
    }
    if (restMs[j] > 1) {  // (a rest inside the phrase: 15 ms pauses)
      s.setWord(0);
      for (int q = int(std::lround(restMs[j] / 15.0)); q > 0; --q) s.push(Ph::Stop);
    }
  }
  s.finish();
  // The director's drama, hairpins and vibrato (the notes' cues), as the voice's expression line:
  // the breath's pressure and the vibrato over the phrase's time. sungAt: when each note's sung
  // part begins in the sound (known from the rendering before; none: no expression yet).
  if (!sungAt.empty()) {
    static const double kDramaDb = knobD("PJ_SINGER_DRAMA_DB", 14), kVib = knobD("PJ_SINGER_VIB", 30), kVibHz = knobD("PJ_SINGER_VIB_HZ", 5.4);
    std::vector<Synth::Expr> ex;
    // (the level's dB as pressure: the voice's amplitude goes about as pressure^1.5)
    static const double kWeightDb = knobD("PJ_SINGER_WEIGHT_DB", 3);
    static const double kPulseDb = knobD("PJ_SINGER_PULSE_DB", 1.5);
    static const double kClassicalOpen = knobD("PJ_SINGER_CLASSICAL_OPEN", 0.3);
    static const double kReduceX = knobD("PJ_SINGER_REDUCE", 0.35);
    const bool scat = style_ == kJazz || style_ == kBebop || style_ == kBallad || style_ == kFolk;
    // (level: the director's, 0 .. 1; w: the syllable's weight, 0.4 the plain middle)
    auto press = [](double level, double w) { return float(std::exp(kLn10 * ((kDramaDb * (level - 0.5) + kWeightDb * (w - 0.4)) / 20 / 1.5))); };
    double carried = 0.4;  // the weight the breath is at, at the end of the syllable before
    for (int j = 0; j < p.count; ++j) {
      const SungNote& n = note_[p.first + j];
      if (sungAt[j] < 0) continue;
      const double len = vowelMs[j] / 1000, t0 = sungAt[j];
      // (without a director: the singer's own habit, a long note's vibrato coming in late)
      double depth = kVib * (n.cued ? n.vibDepth : len > 0.45 ? 1.0 : 0.0), delay = n.cued ? n.vibDelay : 0.25;
      // (a man scatting: when the vibrato comes in a long note, and how deep, src/jazzsinger.h)
      if (scat && kVoices[voice_].woman < 0.5f && len > 0.45) {
        const uint32_t r = mix(uint32_t(p.first + j) * 2246822519u + 101u);
        const double u = (r >> 8) % 1000 / 1000.0;
        if (!n.cued) depth = kVib;
        if (u < 0.80) delay = (0.42 + 0.16 * ((r >> 20) % 100) / 100.0) * len;  // about half-way
        else if (u < 0.95) delay = 0.03;                                       // from the beginning
        else { delay = 0.8 * len; depth *= 0.3; }                              // a trace at the end
        depth *= (0.75 + 0.6 * n.harm) * std::clamp(1.3 - 0.6 * n.busy / 8.0, 0.7, 1.3);
      }
      // (the classical school: a vibrato all through every note that has the time for one, from its
      // beginning, a little quicker)
      if (!scat && len > 0.3) {
        if (!n.cued) depth = kVib;
        delay = std::min(delay, 0.06);
      }
      if (noLevel_) depth = 0;  // (the practice hears the notes plain: no vibrato)
      const double hz = kVibHz * (n.cued ? std::clamp(double(n.vibRate), 0.5, 1.6) : 1.0) * (scat ? 1.0 : 1.08);
      // How a vibrato comes in (the author, 2026-11-27: "sometimes vibrato could start with a lower frequency and
      // ramp up to the normal frequency during the vibrato, other times the vibrato is roughly constant frequency
      // and only the amplitude of the vibrato ramps up when it is introduced"). PJ_SINGER_VIB_QUICKEN (0.4) of
      // the notes with the time for it begin at PJ_SINGER_VIB_QUICKEN_FROM (0.7) of the rate and reach it over
      // PJ_SINGER_VIB_QUICKEN_S (0.6 s); the rest keep the rate and only grow in depth.
      static const double kQuick = knobD("PJ_SINGER_VIB_QUICKEN", 0.4), kQuickFrom = knobD("PJ_SINGER_VIB_QUICKEN_FROM", 0.7), kQuickS = std::max(0.05, knobD("PJ_SINGER_VIB_QUICKEN_S", 0.6));
      const bool quicken = len - delay > 0.5 && (mix(uint32_t(p.first + j) * 2654435761u + 977u) >> 9) % 1000 < 1000 * kQuick;
      const int steps = std::max(2, int(len / 0.02));
      // How a long note develops (src/jazzsinger.h): which way, from where the play is going.
      static const double kDevelop = knobD("PJ_SINGER_DEVELOP", 1), kTurn = knobD("PJ_SINGER_TURN", 1);
      const uint32_t rd = mix(uint32_t(p.first + j) * 3266489917u + 7u);
      const double devAmp = kDevelop * std::clamp((len - 0.6) / 0.6 + 0.5, 0.0, 1.0) * (len >= 0.6 ? 1.0 : 0.0);
      const double trend = playV_[p.first + j];
      const int devKind = std::fabs(trend) > 0.08 && (rd >> 4) % 10 < 7 ? (trend > 0 ? 0 : 1) : (rd >> 8) % 8 == 0 ? 3 : 2;  // up, down, swell, dip
      const double devColour = brightV_[p.first + j] >= 0 ? 1.0 : -1.0;  // (brightening: the mouth opens with it; darkening: it closes)
      const bool directed = n.cued && (std::fabs(n.swell) > 0.03f || std::fabs(n.levelEnd - n.level) > 0.03f);  // (the director shapes this note's level already)
      // (the turn at a very long scat note's beginning)
      const bool turn = scat && kTurn > 0 && len >= 1.3 && (rd >> 12) % 100 < 18 + 25 * mischief_[p.first + j] * kTurn;
      const double turnCents = j > 0 && std::abs(n.key - note_[p.first + j - 1].key) == 1 ? 100 : 200;
      // A weighted syllable: the breath pressed for it from its consonant on, at its most at its
      // beginning and easing through it. An unweighted one: no new push; it begins on what the
      // syllable before left and eases on, however long its note.
      const double w = weight_[p.first + j];
      const bool push = w >= 0.2 || ex.empty();
      // (no punch: a weighted syllable's pressure is steady and supported, a little eased by its end)
      const double w0 = push ? w : std::min(carried, 0.4), w1 = push ? w - 0.1 * std::max(0.0, w - 0.4) : std::min(w0, 0.15);
      if (push && (ex.empty() || t0 - 0.06 > ex.back().t)) ex.push_back({std::max(0.0, t0 - 0.06), press(n.level, w0) * (noteRow[j] ? float(std::pow(noteRow[j]->press, kRowPress)) : 1.0f), 0, float(hz)});
      carried = w1;
      for (int q = 0; q <= steps; ++q) {
        const double x = double(q) / steps, t = t0 + x * len;
        const double pk = std::clamp(double(n.swellPeak), 0.1, 0.9);
        const double bump = x < pk ? std::sin(0.5 * M_PI * x / pk) : std::cos(0.5 * M_PI * (x - pk) / (1 - pk));
        const double level = n.level + (n.levelEnd - n.level) * x + n.swell * bump * bump;
        // (the vibrato grows over 0.2 s from its beginning and eases over the note's last 60 ms; one that
        // quickens is at its depth sooner, and slower at first)
        const double into = x * len - delay;
        const double grow = std::clamp(into / (quicken ? 0.1 : 0.2), 0.0, 1.0) * std::clamp((len - x * len) / 0.06, 0.0, 1.0);
        const double hzNow = quicken ? hz * (kQuickFrom + (1 - kQuickFrom) * std::clamp(into / kQuickS, 0.0, 1.0)) : hz;
        if (!ex.empty() && t <= ex.back().t) continue;
        // (the weight eases over the syllable's first 0.3 s, a long note no longer pressed)
        const double ease = std::min(1.0, x * len / 0.3);
        // (the long note's development: g from -1 to 1 over it)
        double g = 0;
        if (devAmp > 0) {
          const double sw = std::sin(M_PI * x);
          g = devAmp * (devKind == 0 ? 2 * x - 1 : devKind == 1 ? 1 - 2 * x : devKind == 2 ? 2 * sw - 1 : 1 - 2 * sw);
        }
        // (the practised arrival: aimed past the note at first, the breath eased or pressed, both fading)
        float aim = 0, breath = 1;
        if (hasWay[j]) {
          const double tin = x * len;
          const int dir = n.key > note_[p.first + j - 1].key ? 1 : -1;
          aim = float(dir * way[j].over * std::max(0.0, 1 - tin / std::max(1e-3, way[j].overMs * 1e-3)));
          breath = float(std::exp(kLn10 * (way[j].dipDb * std::max(0.0, 1 - tin / 0.08) / 20 / 1.5)));
        } else {
          // (a note with no practised way: the voice comes on under its pitch, 70 to 95 cents low and
          // 100 to 150 ms in getting there, on almost every note of a song; so it is aimed above the
          // note at first, by PJ_SINGER_START_C cents fading over PJ_SINGER_START_MS)
          static const double kStartC = knobD("PJ_SINGER_START_C", 0), kStartMs = knobD("PJ_SINGER_START_MS", 120);
          aim = float(kStartC * std::max(0.0, 1 - x * len / (kStartMs * 1e-3)));
        }
        // (the mouth opened for a high note, as practised: the jaw dropped and the lips flared, the
        // vowel towards "ah", which keeps the tract's first resonance at the pitch or above it; opening
        // over the note's first 40 ms)
        float open = noteRow[j] ? float(kRowOpen) * noteRow[j]->f1x * float(std::min(1.0, x * len / 0.04)) : 0.0f;
        // (a classical singer keeps the jaw open throughout: the colours, the overtones and the tuning are easier)
        if (!scat) open = std::max(open, float(kClassicalOpen));
        // (a high voice's vowels migrate towards an open "ah" as the pitch rises. The author, 2026-11-26: "perhaps for
        // the soprano in highest notes they need to move towards a single vowel space to have enough resonance
        // because the carrier wave becomes less dense", and: "should be gradual with the increasing pitch, the idea
        // is to keep the sound more or less without 'breaks' within pitch changes". So no step anywhere: nothing at
        // PJ_SINGER_MIGRATE_LO (C5), growing evenly with the pitch, slowly at first, to PJ_SINGER_MIGRATE (0.85)
        // at PJ_SINGER_MIGRATE_HI (C6) and held above; within a note it follows the note's own pitch.)
        {
          static const double kMigLo = knobD("PJ_SINGER_MIGRATE_LO", 523.25), kMigHi = knobD("PJ_SINGER_MIGRATE_HI", 1046.5), kMig = knobD("PJ_SINGER_MIGRATE", 0.85);
          const double f0 = 440 * std::exp2((n.key - 69) / 12.0);
          const double u = std::clamp(std::log2(f0 / kMigLo) / std::log2(kMigHi / kMigLo), 0.0, 1.0);
          open = std::max(open, float(kMig * std::pow(u, 1.5)));
        }
        // (the second resonance: the tongue's body brought forward as the pitch rises, puhuja's Expr::front, which
        // raises the tract's second resonance towards the note's octave partial. The author, 2026-11-26: "let's build
        // a second resonance for the octave partial". Measured on the soprano's held notes: on "ah" from F#5 up the
        // overtones 2 to 4 are 6 to 14 dB stronger with the tongue fully forward, on "oo" from G#4 up 5 to 8 dB, on
        // "ee" from G#5 up 2 to 5 dB. Gradual with the pitch like the migration: nothing at PJ_SINGER_FRONT_LO (C5),
        // PJ_SINGER_FRONT (1) from PJ_SINGER_FRONT_HI (G5) up.)
        float front = 0;
        {
          static const double kLo = knobD("PJ_SINGER_FRONT_LO", 523.25), kHi = knobD("PJ_SINGER_FRONT_HI", 784.0), kMax = knobD("PJ_SINGER_FRONT", 1);
          const double f0 = 440 * std::exp2((n.key - 69) / 12.0);
          const double u = std::clamp(std::log2(f0 / kLo) / std::log2(kHi / kLo), 0.0, 1.0);
          front = float(kMax * u * u * (3 - 2 * u));
        }
        // (the vowel towards the neutral one: an unweighted syllable's, PJ_SINGER_REDUCE of the way;
        // a quick note's more, the tongue and lips busy with the consonants and not reaching the vowel)
        float neutral = 0;
        if (scat && syl_[p.first + j].nucleus != '@') neutral = float(std::max(w < 0.2 ? kReduceX : 0.0, len < 0.16 ? 0.5 * (0.16 - len) / 0.08 : 0.0));
        neutral = std::min(neutral, 0.6f);
        // (stress from the larynx and the mouth, in the jazz styles: src/jazzsinger.h)
        float adduct = 1, pulse = 1;
        if (scat) {
          const bool ghost = syl_[p.first + j].nucleus == 'n' || syl_[p.first + j].nucleus == 'm';
          adduct = ghost ? 1.4f : w >= 0.7 ? float(1 - 0.15 * (w - 0.7) / 0.3) * 0.925f + 0.075f : w < 0.2 ? 1.25f : 1.0f;
          if (w >= 0.7) {
            open = std::min(1.0f, open + 0.15f * float(std::min(1.0, x * len / 0.04)));
            pulse = float(std::exp(kLn10 * (kPulseDb * std::max(0.0, 1 - x * len / 0.06) / 20 / 1.5)));
          }
        }
        if (g != 0) {
          if (!directed) breath *= float(std::exp(kLn10 * (2.2 * g / 20 / 1.5)));           // the breath, +-2.2 dB
          open = std::clamp(open + float(0.12 * (g + devAmp) * (devColour > 0 ? 1.0 : 0.4)), 0.0f, 1.0f);  // the mouth
          adduct *= float(1 - 0.07 * g);                                                 // the folds: more together as it grows
          neutral = std::clamp(neutral + float(0.12 * (devAmp - g) * (devColour > 0 ? 0.5 : 1.0)), 0.0f, 0.6f);  // the vowel clearer as it grows
        }
        if (turn) {  // up, up, the note, up, home: 85 ms a step, gliding 25 ms
          const double u = x * len / 0.085;
          auto lvl = [](double q) { return q < 2 ? 1.0 : q < 3 ? 0.0 : q < 4 ? 1.0 : 0.0; };
          const double fr = u - std::floor(u), now = lvl(u), before = u >= 1 ? lvl(u - 1) : now;
          aim += float(turnCents * (before + (now - before) * std::min(1.0, fr / 0.3)));
        }
        ex.push_back({t, press(level, w0 + (w1 - w0) * ease) * breath * pulse * (noteRow[j] ? float(std::pow(noteRow[j]->press, kRowPress)) : 1.0f), float(depth * grow * (push ? 1.0 : 0.5)), float(hzNow), aim, open, adduct, neutral, front});
      }
    }
    s.setExpression(std::move(ex));
  }
  std::vector<float> pcm = pullAll(s);
  vowelAt->assign(p.count, -1.0);
  std::vector<int> seen(p.count, 0);
  struct Phone { double start, end; bool nucleus; };
  std::vector<Phone> phone;
  for (const Synth::Label& l : s.labels()) {
    if (l.word < 1 || l.word > p.count) continue;
    const int j = l.word - 1;
    const bool nucleus = seen[j]++ == ncons[j];
    if (nucleus) (*vowelAt)[j] = l.start;
    if (l.symbol != "_") phone.push_back({l.start, l.end, nucleus});
  }
  *lead = (*vowelAt)[0] >= 0 ? (*vowelAt)[0] : 0.5;
  // Lighter folds swing wider and sound much louder (a soprano's 10 dB over the baritone's on the
  // same breath, 20 dB more at her top): each note brought to the level a voice has there, the
  // speaker's own at the bottom of the range and PJ_SINGER_RISE_DB (4) more an octave up, by what
  // was learnt of the voice's loudness on each pitch (practised: on that vowel, with the breath it
  // was given there; else from the scale the tension was learnt on). The gain moves over 40 ms.
  if (!noLevel_) {
    static const double kRise = knobD("PJ_SINGER_RISE_DB", 4);
    std::vector<std::pair<double, float>> pt;
    for (int j = 0; j < p.count; ++j) {
      if ((*vowelAt)[j] < 0) continue;
      const int key = note_[p.first + j].key;
      const double want = -34 + kRise * (key - kVoices[voice_].lo) / 12.0;
      pt.push_back({(*vowelAt)[j] - 0.05, float(std::exp(kLn10 * (std::clamp(want - (noteRow[j] && !sungAt.empty() ? noteRow[j]->levelDb : levelRow[j] && !sungAt.empty() ? levelRow[j]->plainDb : levelOf(voice_, key)), -40.0, 12.0) / 20)))});
    }
    if (!pt.empty()) {
      size_t q = 0;
      float g = pt[0].second;
      const float step = float(1 / (0.04 * kFs));
      for (size_t i = 0; i < pcm.size(); ++i) {
        while (q + 1 < pt.size() && i / kFs >= pt[q + 1].first) ++q;
        const float to = pt[q].second;
        g += std::clamp(to - g, -step * std::max(g, to), step * std::max(g, to));
        pcm[i] *= g;
      }
    }
  }
  // Resonances that follow the note (the author, 2026-11-26: "let's get the 3rd overtone as well as 5th overtone
  // covered with respective resonances", "the resonances follow the pitch lazily (like 10 ms delayed ...)", then
  // 2026-11-27: "they don't need to trail 10 ms but be guided through PI-control in a way that they have half
  // corrected at 10 ms",
  // "coloratura could have lower Q values with the overtone resonances ... possibly also soprano"). A singer
  // tunes resonances to the partials of the note being sung; the tract model has controls for the first two (the
  // mouth's opening, the tongue's place), so the next two are resonances after the voice: one at three times and
  // one at five times the note's pitch, each led to the pitch by a PI control (on the pitch's logarithm: half of
  // a step made good after PJ_SINGER_PARTIAL_HALF_MS, 10; the integral leaves no lag behind a glide; its damping
  // PJ_SINGER_PARTIAL_DAMP, 1: critical, a step overshot by 13 %; higher overshoots less), added to the
  // voice at PJ_SINGER_PARTIAL_DB (0: a partial on the resonance comes out 6 dB stronger; -99: none), their
  // width PJ_SINGER_PARTIAL_CENTS (100 cents; the soprano's 150, the coloratura's 200: a lower Q). Fixed
  // resonances on the scale's pitches cannot do this: all partials of a note in tune lie on them alike.
  if (!noLevel_ && vowelAt) {
    static const double kDb = knobD("PJ_SINGER_PARTIAL_DB", 0), kHalfMs = knobD("PJ_SINGER_PARTIAL_HALF_MS", 10), kDamp = knobD("PJ_SINGER_PARTIAL_DAMP", 1), kCentsAll = knobD("PJ_SINGER_PARTIAL_CENTS", 0);
    static const double kTop = knobD("PJ_SINGER_PARTIAL_TOP_HZ", 9000);
    if (kDb > -90) {
      const double cents = kCentsAll > 0 ? kCentsAll : !std::strcmp(kVoices[voice_].name, "coloratura") ? 200.0 : !std::strcmp(kVoices[voice_].name, "soprano") ? 150.0 : 100.0;
      const double k = std::exp2(cents / 1200) - 1, gain = std::exp(kLn10 * (kDb / 20));  // (k = 1 / Q)
      // (the PI control: the resonance's pitch moves at kp * miss + ki * the miss's integral; its natural
      // rate w is found by trying w = 1 and timing the half way of a step)
      static const double kW = [] {
        const double z = std::max(kDamp, 0.3), dt = 1e-4;
        double y = 0, in = 0, t = 0;
        for (; y < 0.5 && t < 100; t += dt) { const double e = 1 - y; in += e * dt; y += (2 * z * e + in) * dt; }
        return t / (std::max(kHalfMs, 0.1) * 1e-3);
      }();
      const double dt = 1 / kFs, kp = 2 * std::max(kDamp, 0.3) * kW * dt, ki = kW * kW * dt * dt;
      std::vector<std::pair<double, double>> hz;  // (from when, the note's pitch)
      for (int j = 0; j < p.count; ++j)
        if ((*vowelAt)[j] >= 0) hz.push_back({(*vowelAt)[j], 440 * std::exp2((note_[p.first + j].key - 69 + note_[p.first + j].cents / 100) / 12.0)});
      if (!hz.empty()) {
        size_t q = 0;
        double lf = std::log(hz[0].second), integ = 0, s1[2] = {0, 0}, s2[2] = {0, 0};
        const int part[2] = {3, 5};
        for (size_t i = 0; i < pcm.size(); ++i) {
          while (q + 1 < hz.size() && i / kFs >= hz[q + 1].first) ++q;
          const double miss = std::log(hz[q].second) - lf;
          integ += ki * miss;
          lf += kp * miss + integ;
          const double f = std::exp(lf);
          const double x = pcm[i];
          double add = 0;
          for (int h = 0; h < 2; ++h) {
            const double fc = part[h] * f;
            if (fc > kTop) continue;
            // (a state-variable resonance, steady while its pitch moves; its peak gain 1)
            const double g = std::tan(M_PI * fc / kFs), a1 = 1 / (1 + g * (g + k)), a2 = g * a1, a3 = g * a2;
            const double v3 = x - s2[h], v1 = a1 * s1[h] + a2 * v3, v2 = s2[h] + a2 * s1[h] + a3 * v3;
            s1[h] = 2 * v1 - s1[h]; s2[h] = 2 * v2 - s2[h];
            add += k * v1;
          }
          pcm[i] = float(x + gain * add);
        }
      }
    }
  }
  // Consonants against vowels (the author, 2026-11-27: "now the vowels are a bit too powerful in comparison to
  // the consonants with all these resonances likely, perhaps just make consonants a little stronger (+2 db) and
  // vowels a little less strong (like -3 db)", "it is possible that the inbalance only existed for soprano and
  // coloratura"). For those two voices a syllable's nucleus is brought down 3 dB and the consonants around it
  // up 2 dB, the gain moving between them over about 8 ms; PJ_SINGER_VOWEL_DB and PJ_SINGER_CONS_DB set them
  // for every voice.
  if (!noLevel_ && !phone.empty()) {
    static const double kVDb = knobD("PJ_SINGER_VOWEL_DB", 99), kCDb = knobD("PJ_SINGER_CONS_DB", 99);
    const bool high = !std::strcmp(kVoices[voice_].name, "coloratura") || !std::strcmp(kVoices[voice_].name, "soprano");
    const float kV = float(std::exp(kLn10 * ((kVDb < 90 ? kVDb : high ? -3.0 : 0.0) / 20))), kC = float(std::exp(kLn10 * ((kCDb < 90 ? kCDb : high ? 2.0 : 0.0) / 20)));
    if (kV != 1 || kC != 1) {
      const float a = float(1 - std::exp(-1 / (0.003 * kFs)));
      size_t q = 0;
      float g = phone[0].nucleus ? kV : kC;
      for (size_t i = 0; i < pcm.size(); ++i) {
        const double t = i / kFs;
        while (q + 1 < phone.size() && t >= phone[q + 1].start) ++q;
        g += a * ((phone[q].nucleus ? kV : kC) - g);
        pcm[i] *= g;
      }
    }
  }
  return pcm;
}

void JazzSinger::sing(Phrase& p) {
  // Each note's sung part runs from its beat to its end: where the key is let go, or (the next
  // note following at once) where the next syllable's consonants begin, which stand before the
  // beat. The consonants' lengths are puhuja's; they are learnt from a first rendering, and the
  // phrase is sung again with the lengths corrected until every beat is within 4 ms.
  const int n = p.count;
  std::vector<double> cons(n, 0.0), vowelMs(n), restMs(n), at;
  for (int j = 0; j < n; ++j)
    for (char c : syl_[p.first + j].cons)
      if (c) cons[j] += 0.07;
  double lead = 0, bestLead = 0, best = 1e9;
  std::vector<double> bestAt, bestMs;
  std::vector<float> pcm, bestPcm;
  const double t0 = note_[p.first].on;
  for (int pass = 0; pass < 4; ++pass) {
    for (int j = 0; j < n; ++j) {
      const SungNote& s = note_[p.first + j];
      if (j + 1 < n) {
        const double nextCons = note_[p.first + j + 1].on - cons[j + 1];
        const double end = std::max(s.on + 0.04, std::min(s.off, nextCons));
        vowelMs[j] = 1000 * (end - s.on);
        restMs[j] = std::max(0.0, 1000 * (nextCons - end));
        // (an unweighted syllable at its word's end gives a tenth of its time away, to the word
        // that follows: the weighted ones have theirs whole)
        if (weight_[p.first + j] < 0.2 && wordStart_[p.first + j + 1] && vowelMs[j] > 150) {
          restMs[j] += 0.1 * vowelMs[j];
          vowelMs[j] *= 0.9;
        }
        if (restMs[j] < 20) { vowelMs[j] += restMs[j]; restMs[j] = 0; }  // (too short for a rest: sung through)
      } else {
        vowelMs[j] = 1000 * std::max(0.08, s.off - s.on);
        restMs[j] = 0;
      }
    }
    // (the expression needs the notes' places in the sound: from the second rendering on, each
    // note where the first had its beat's lead plus its own time)
    std::vector<double> sungAt;
    if (pass > 0)
      for (int j = 0; j < n; ++j) sungAt.push_back(at[j] < 0 ? -1.0 : lead + (note_[p.first + j].on - t0));
    pcm = take(p, vowelMs, restMs, sungAt, &at, &lead);
    // (a consonant longer than taken makes its note and every later one late: each note is
    // corrected by its lateness beyond the note before it)
    double worst = 0, before = 0;
    for (int j = 1; j < n; ++j) {
      if (at[j] < 0) continue;
      const double err = (at[j] - lead) - (note_[p.first + j].on - t0);  // + late
      worst = std::max(worst, std::fabs(err));
      cons[j] = std::clamp(cons[j] + (err - before), 0.0, 0.4);
      before = err;
    }
    if (pass > 0 && worst < best) { best = worst; bestPcm = pcm; bestLead = lead; bestAt = at; bestMs = vowelMs; }  // (the first: without the expression)
    static const bool kTrace = knobD("PJ_SINGER_TRACE", 0) > 0;
    if (kTrace) {
      std::fprintf(stderr, "singer: phrase at %.2f s, pass %d, worst %.0f ms, lead %.0f ms:", t0, pass, 1000 * worst, 1000 * lead);
      for (int j = 0; j < n; ++j)
        std::fprintf(stderr, " %s%s%+.0f(v%.0f r%.0f)", syl_[p.first + j].cons, spell(syl_[p.first + j].nucleus),
                     at[j] < 0 ? -999.0 : 1000 * ((at[j] - lead) - (note_[p.first + j].on - t0)), vowelMs[j], restMs[j]);
      std::fprintf(stderr, "\n");
    }
    if (pass > 0 && worst < 0.008) break;  // (puhuja's lengths come in steps of about 10 ms)
  }
  pcm = std::move(bestPcm);
  lead = bestLead;
  // (the voice at the level the level hold is set for; the singer's fader, PJ_SINGER_GAIN, comes
  // after the hold: render())
  for (float& v : pcm) v *= 6.0f;
  p.pcm = std::move(pcm);
  p.start = t0 - lead;
  // (PJ_SINGER_LABELS=FILE: where each note's sung part lies in the rendering, for measuring:
  // start s, length s, key, cents asked, the syllable's nucleus, its weight)
  static std::FILE* lab = std::getenv("PJ_SINGER_LABELS") ? std::fopen(std::getenv("PJ_SINGER_LABELS"), "w") : nullptr;
  if (lab)
    for (int j = 0; j < n && j < int(bestAt.size()); ++j)
      if (bestAt[j] >= 0) {
        std::fprintf(lab, "%.4f %.4f %d %.1f %c %.2f\n", p.start + bestAt[j], bestMs[j] / 1000, note_[p.first + j].key, note_[p.first + j].cents,
                     syl_[p.first + j].nucleus, weight_[p.first + j]);
        std::fflush(lab);
      }
  p.lead = lead;
  p.done = true;
}

void JazzSinger::render(float* out, int n) {
  // A phrase is sung once its first note is within 1.5 s (its breath and consonants come before
  // the beat), and laid over what is already there (the breath over the last note's end).
  const double until = (at_ + n) / kFs + 1.5;
  while (next_ < phrase_.size() && note_[phrase_[next_].first].on <= until) {
    Phrase& p = phrase_[next_++];
    sing(p);
    const long s0 = std::lround(p.start * kFs);
    // (the breath before a phrase is its first 0.4 s: where the last phrase still sounds or the
    // rendering has passed, it is cut, the rest faded in over 20 ms)
    long from = std::max(0L, at_ - s0);
    // (no breath while the last phrase's last note is still sung: the breath is cut to the rest
    // there is, down to the 0.15 s before the first beat, where the first consonant is)
    const long sung = std::lround((lastEnd_ - p.start) * kFs), keep = std::lround((p.lead - 0.15) * kFs);
    from = std::max(from, std::min(sung, std::max(0L, keep)));
    lastEnd_ = note_[p.first + p.count - 1].off;
    const long len = long(p.pcm.size());
    if (from >= len) { p.pcm.clear(); continue; }
    if (head_ + size_t(s0 + len - at_) > ahead_.size()) ahead_.resize(head_ + size_t(s0 + len - at_), 0.0f);
    const long fade = from > 0 ? 1440 : 0;
    for (long i = from; i < len; ++i) {
      const float g = fade && i - from < fade ? float(i - from) / fade : 1.0f;
      ahead_[head_ + size_t(s0 + i - at_)] += g * p.pcm[size_t(i)];
    }
    p.pcm.clear();
    p.pcm.shrink_to_fit();
  }
  const int have = int(std::min<long>(n, long(ahead_.size() - head_)));
  std::copy(ahead_.begin() + head_, ahead_.begin() + head_ + have, out);
  std::fill(out + have, out + n, 0.0f);
  head_ += size_t(have);
  if (head_ >= ahead_.size()) { ahead_.clear(); head_ = 0; }
  else if (head_ > (1u << 18)) { ahead_.erase(ahead_.begin(), ahead_.begin() + head_); head_ = 0; }
  // Resonators at the pitches of the scale (a trial; the author, 2026-11-26: "let's try extra nasal resonances
  // perhaps each at each 12-tet frequences"): a narrow resonance on every equal-tempered semitone from C4 to C8,
  // each PJ_SINGER_NASAL12_CENTS (25) wide, their sum added to the voice at PJ_SINGER_NASAL12_DB (-6; -99: none).
  // In a song the soprano's vowels came out 1 dB cleaner at -6 dB and 1.4 dB at 0 dB, her overtones 0.2 to 0.4 dB
  // stronger: a small effect, left on low for the ear to judge.
  // Of a note in tune its fundamental and the partials an octave apart (2, 4, 8) fall on one; 3 and 6 lie
  // 2 cents off one, 5 lies 14 off, 7 lies 31 off. After the voice, not inside the tract: it colours the sound
  // and does not act back on the folds.
  {
    static const double kDb = knobD("PJ_SINGER_NASAL12_DB", -6);
    // (the author: "coloratura could have lower Q values with the overtone resonances ... possibly also soprano":
    // their resonances twice and one and a half times as wide, 50 and 38 cents for 25; PJ_SINGER_NASAL12_CENTS: all)
    static const double kCentsAll = knobD("PJ_SINGER_NASAL12_CENTS", 0);
    static const double kCents = kCentsAll > 0 ? kCentsAll : !std::strcmp(kVoices[voice_].name, "coloratura") ? 50.0 : !std::strcmp(kVoices[voice_].name, "soprano") ? 38.0 : 25.0;
    if (kDb > -90) {
      constexpr int kN = 3 * 49;
      struct Bank { double a1[kN], a2[kN], b[kN], y1[kN] = {}, y2[kN] = {}, x1 = 0, x2 = 0; };
      static Bank* bank = [] {
        Bank* q = new Bank;
        // (the author: "let's get the 3rd overtone as well as 5th overtone covered with respective resonances". A
        // note's third partial lies 2.0 cents above a pitch of the scale and its fifth 13.7 cents below one (and so
        // do the sixth and the tenth), which the 25-cent resonance on the scale's pitch covers poorly for the fifth.
        // So two more rows: every semitone + 1.955 cents and every semitone - 13.686 cents. PJ_SINGER_NASAL12_JUST=0:
        // the scale's own pitches alone.)
        static const bool kJust = knobD("PJ_SINGER_NASAL12_JUST", 1) > 0;
        const double off[3] = {0, 1.955, -13.686};
        for (int k = 0; k < kN; ++k) {
          const double f = 440 * std::exp2((60 + k % 49 - 69 + off[k / 49] / 100) / 12.0), bw = f * (std::exp2(kCents / 1200) - 1);
          const double r = std::exp(-M_PI * bw / kFs);
          q->a1[k] = 2 * r * std::cos(2 * M_PI * f / kFs); q->a2[k] = -r * r; q->b[k] = k >= 49 && !kJust ? 0.0 : 0.5 * (1 - r * r);
        }
        return q;
      }();
      static const double g = std::exp(kLn10 * (kDb / 20));  // (once: the knob does not change)
      for (int i = 0; i < n; ++i) {
        const double x = out[i], d = x - bank->x2;
        double sum = 0;
        for (int k = 0; k < kN; ++k) {
          const double y = bank->b[k] * d + bank->a1[k] * bank->y1[k] + bank->a2[k] * bank->y2[k];
          bank->y2[k] = bank->y1[k]; bank->y1[k] = y; sum += y;
        }
        bank->x2 = bank->x1; bank->x1 = x;
        out[i] = float(x + g * sum);
      }
    }
  }
  // The level held gently, as a hand on the fader: above the threshold 2 : 1 on the voice's
  // loudness over ~50 ms, coming in over 20 ms and letting go over 0.3 s, and made up by what
  // a usual note loses.
  {
    static const double kRatio = std::max(1.0, knobD("PJ_SINGER_COMP", 2)), kThr = std::exp(kLn10 * (knobD("PJ_SINGER_COMP_DB", -18) / 20));
    static const double kUp = 1 - std::exp(-1 / (0.02 * kFs)), kDown = 1 - std::exp(-1 / (0.3 * kFs));
    // (a usual sung note is about -13 dB here: what it loses is given back, so the knob changes
    // the evenness and not the level)
    static const double kMakeup = std::exp(kLn10 * (std::max(0.0, -13 - knobD("PJ_SINGER_COMP_DB", -18)) * (1 - 1 / kRatio) / 20));
    if (kRatio > 1)
      for (int i = 0; i < n; ++i) {
        const double p = double(out[i]) * out[i];
        compEnv_ += (p - compEnv_) * (p > compEnv_ ? kUp : kDown);
        const double rms = std::sqrt(compEnv_);
        const double g = rms > kThr ? std::pow(rms / kThr, 1 / kRatio - 1) : 1.0;
        out[i] = float(out[i] * g * kMakeup);
      }
  }
  // The singer's fader (PJ_SINGER_GAIN, 9.4 by the author's ear, 2026-11-26, after 24, then 7.5: twice it is 6 dB more), after the hold, so that it is
  // a plain gain (before it, the hold took half of every change back).
  static const float kFader = float(knobD("PJ_SINGER_GAIN", 9.4) / 6.0);
  if (kFader != 1)
    for (int i = 0; i < n; ++i) out[i] *= kFader;
  sent_.insert(sent_.end(), out, out + n);
  // (PJ_SINGER_DUMP=FILE: the singer alone, before the hall, as raw 32-bit floats; for measuring)
  static std::FILE* dump = std::getenv("PJ_SINGER_DUMP") ? std::fopen(std::getenv("PJ_SINGER_DUMP"), "wb") : nullptr;
  if (dump) { std::fwrite(out, sizeof(float), size_t(n), dump); std::fflush(dump); }
  at_ += n;
}

}  // namespace pajupilli

namespace pajupilli {

// The singer's plate: a thin steel sheet's bending modes (its edges held: sin x sin shapes),
// driven at one point by the voice and heard at two. A plate is stiff, not stretched: its modes
// lie evenly over the frequency, thousands of them, which is its smooth, dense ring.
struct JazzSinger::Plate {
  std::vector<float> a1, a2, b, cl, cr, y1, y2;
  std::vector<float> delay;
  size_t at = 0;
  float send = 0;
  Plate() {
    static const double kT60 = knobD("PJ_SINGER_PLATE_T60", 1.6), kPre = knobD("PJ_SINGER_PLATE_PREDELAY", 50);
    const double A = 2.0, B = 1.0, h = 0.5e-3, E = 2.0e11, rho = 7850, nu = 0.3;
    const double K = 0.5 * M_PI * h * std::sqrt(E / (12 * rho * (1 - nu * nu)));  // f = K ((m / A)^2 + (n / B)^2)
    const double dx = 0.47, dy = 0.42, lx = 0.21, ly = 0.71, rx = 0.81, ry = 0.29;  // shares of the sheet
    double el = 0, er = 0;
    for (int m = 1; m < 400; ++m)
      for (int n = 1; n < 200; ++n) {
        const double f = K * (double(m) * m / (A * A) + double(n) * n / (B * B));
        if (f < 120 || f > 7500) continue;
        // (the return's colour: little under 200 Hz, less and less over 6 kHz)
        const double w8 = (f * f / (f * f + 200.0 * 200.0)) / std::sqrt(1 + std::pow(f / 6000, 6));
        const double t60 = kT60 / (1 + std::pow(f / 3500, 1.2));
        const double r = std::exp(-6.91 / (t60 * kFs)), w = 2 * M_PI * f / kFs;
        const double pd = std::sin(m * M_PI * dx) * std::sin(n * M_PI * dy);
        const double pl = std::sin(m * M_PI * lx) * std::sin(n * M_PI * ly), pr = std::sin(m * M_PI * rx) * std::sin(n * M_PI * ry);
        if (std::fabs(pd) < 0.05) continue;
        a1.push_back(float(2 * r * std::cos(w)));
        a2.push_back(float(-r * r));
        b.push_back(float(std::sin(w) * pd * w8));
        cl.push_back(float(pl));
        cr.push_back(float(pr));
        const double e = (pd * w8) * (pd * w8) / (2 * (1 - r * r));  // (the mode's impulse energy, about)
        el += e * pl * pl;
        er += e * pr * pr;
      }
    // (each channel's impulse response of unit energy: the send is then the plate's level against the voice)
    const float nl = float(1 / std::sqrt(std::max(el, 1e-20))), nr = float(1 / std::sqrt(std::max(er, 1e-20)));
    for (float& v : cl) v *= nl;
    for (float& v : cr) v *= nr;
    y1.assign(a1.size(), 0.0f);
    y2.assign(a1.size(), 0.0f);
    delay.assign(std::max<size_t>(1, size_t(kPre * 1e-3 * kFs)), 0.0f);
    send = float(std::exp(kLn10 * (knobD("PJ_SINGER_PLATE_DB", -99) / 20)));
  }
  void run(const float* in, float* l, float* r, int n, float side) {
    const int nm = int(a1.size());
    const float gl = send * (1 - 0.3f * side), gr = send * (1 + 0.3f * side);
    for (int i = 0; i < n; ++i) {
      const float x = delay[at];
      delay[at] = in[i];
      if (++at >= delay.size()) at = 0;
      float sl = 0, sr = 0;
      float *pa1 = a1.data(), *pa2 = a2.data(), *pb = b.data(), *pcl = cl.data(), *pcr = cr.data(), *p1 = y1.data(), *p2 = y2.data();
      for (int k = 0; k < nm; ++k) {
        const float y = pa1[k] * p1[k] + pa2[k] * p2[k] + pb[k] * x;
        p2[k] = p1[k];
        p1[k] = y;
        sl += pcl[k] * y;
        sr += pcr[k] * y;
      }
      l[i] += gl * sl;
      r[i] += gr * sr;
    }
  }
};

void JazzSinger::halo(float* l, float* r, int n, float side) {
  static const bool kOn = knobD("PJ_SINGER_PLATE_DB", -99) > -90;
  const int have = std::min<int>(n, int(sent_.size()));
  if (kOn && have > 0) {
    if (!plate_) plate_ = std::make_shared<Plate>();
    plate_->run(sent_.data(), l, r, have, side);
  }
  sent_.erase(sent_.begin(), sent_.begin() + have);
}

}  // namespace pajupilli

namespace pajupilli {

namespace {
// The pitch (cents from `ft` Hz) and how clearly pitched (0 .. 1) the sound is around sample `at`,
// looked for within a fourth either side of ft: the best lag of the normalized autocorrelation
// over 40 ms. *rms: the level there.
double pitchAt(const std::vector<float>& x, long at, double ft, double* clarity, double* rms) {
  const int w = int(0.04 * kFs), lo = int(kFs / (1.33 * ft)), hi = int(kFs / (0.75 * ft));
  const long a = at - w / 2;
  *clarity = 0;
  *rms = 0;
  if (a < 0 || a + w + hi + 1 >= long(x.size())) return 0;
  double e0 = 0;
  for (int i = 0; i < w; ++i) e0 += double(x[a + i]) * x[a + i];
  *rms = std::sqrt(e0 / w);
  if (e0 < 1e-12) return 0;
  std::vector<double> r(hi - lo + 3);
  for (int L = lo - 1; L <= hi + 1; ++L) {
    double c = 0, e1 = 0;
    for (int i = 0; i < w; ++i) { c += double(x[a + i]) * x[a + i + L]; e1 += double(x[a + i + L]) * x[a + i + L]; }
    r[L - lo + 1] = c / std::sqrt(e0 * e1 + 1e-30);
  }
  int best = 1;
  for (int k = 1; k + 1 < int(r.size()); ++k)
    if (r[k] > r[best]) best = k;
  const double y0 = r[best - 1], y1 = r[best], y2 = r[best + 1], den = y0 - 2 * y1 + y2;
  const double lag = lo - 1 + best + (std::fabs(den) > 1e-12 ? 0.5 * (y0 - y2) / den : 0.0);
  *clarity = y1;
  return 1200 * std::log2(kFs / lag / ft);
}
}  // namespace

int JazzSinger::practise(const char* outPath, int jobs) {
  if (jobs <= 0) jobs = std::max(1u, std::thread::hardware_concurrency());
  std::fprintf(stderr, "the singers learn their folds' tension (a scale each)...\n");
  static const char kVowels[] = "auoie@";
  struct Case { int voice, kind, from, interval; };  // kind 2: a note's trim (from: the key, interval: the vowel's index)
  std::vector<Case> cases;
  const char* only = std::getenv("PJ_SINGER_PRACTICE_VOICE");  // (one voice: the others' rows are lost)
  for (int vi = 0; vi < kNVoices; ++vi) {
    if (only && std::strcmp(only, kVoices[vi].name) != 0) continue;
    gLearnRounds = std::getenv("PJ_SINGER_PRACTICE_VOICES_MODE") ? 1 : 3;
    knownOf(vi);
    const int kLow = kVoices[vi].lo, kHigh = kVoices[vi].hi;
    for (int key = kLow; key <= kHigh; ++key)
      for (int v = 0; kVowels[v]; ++v) cases.push_back({vi, 2, key, v});
    for (int kind = 0; kind < 2; ++kind)
      for (int from = kLow; from <= kHigh; from += 6)
        for (int iv = -12; iv <= 12; ++iv)
          if (iv != 0 && from + iv >= kLow && from + iv <= kHigh) cases.push_back({vi, kind, from, iv});
  }
  // Sings one or two notes (0.45 s each; the second direct, "dah dah", or slurred, "dah-ah") and
  // returns the sound, with where each note's sung part begins in it (s).
  auto sing2 = [](int vi, int key1, int key2, int kind, char vowel1, char vowel2, const Way* w, bool noteSet, const SingerTrim* note, bool plain,
                  std::vector<double>* at) {
    JazzSinger js;
    js.voice_ = vi;
    const int n = key2 >= 0 ? 2 : 1;
    js.note_.resize(n);
    js.syl_.assign(n, Syllable());
    js.note_[0].on = 0; js.note_[0].off = n == 2 ? 0.45 : 0.5; js.note_[0].key = key1;
    js.syl_[0].cons[0] = 'd'; js.syl_[0].nucleus = vowel1;
    std::vector<double> vowelMs(n, 450.0), restMs(n, 0.0);
    if (n == 2) {
      js.note_[1].on = 0.45; js.note_[1].off = 0.9; js.note_[1].key = key2;
      js.syl_[1].cons[0] = kind == 1 ? 'd' : 0; js.syl_[1].nucleus = vowel2;
      if (kind == 1) vowelMs[0] = 380;
    } else vowelMs[0] = 500;
    js.weight_.assign(n, 0.5f);
    js.wordStart_.assign(n, 0);
    js.play_.assign(n, 0.0f); js.bright_.assign(n, 0.0f); js.mischief_.assign(n, 0.0f);
    js.playV_.assign(n, 0.0f); js.brightV_.assign(n, 0.0f);
    js.flip_.assign(n, 0); js.again_.assign(n, 0);
    js.style_ = kJazz;
    js.wordStart_[0] = 1;
    js.phrase_.push_back(Phrase{0, n, 0, 0, {}, false});
    js.tryWay_ = w;
    js.tryNote_ = note;
    js.tryNoteSet_ = noteSet;
    js.noLevel_ = true;  // (heard as the voice gives it)
    js.plain_ = plain && !w;  // (an interval is practised with the notes' own ways in)
    double lead = 0;
    std::vector<double> first;
    js.take(js.phrase_[0], vowelMs, restMs, {}, &first, &lead);
    return js.take(js.phrase_[0], vowelMs, restMs, first, at, &lead);
  };
  // A held note as heard: how far off it sits (the median of its middle, cents; 1e9: no pitch),
  // how much it sways (cents), how loud (dB).
  // rough: the share of it where every second cycle differs (the folds fallen into half the pitch,
  // a rattle); early: where the pitch is 50 to 120 ms in (cents).
  // rich: the 2nd to 5th partials' mean level against the fundamental's (dB).
  // clean: the harmonics' energy over all the rest between 100 Hz and 12 kHz (dB): low where the folds swing
  // unevenly on the right pitch (a screech), as they do where the pitch meets one of the vowel's resonances.
  struct Heard { double off = 1e9, sway = 0, level = -99, rough = 0, early = 0, rich = -40, clean = 40; };
  auto hear = [&](int vi, int key, char vowel, const SingerTrim* row) {
    Heard h;
    std::vector<double> at;
    const std::vector<float> x = sing2(vi, key, -1, 1, vowel, vowel, nullptr, true, row, false, &at);
    if (at.empty() || at[0] < 0) return h;
    const double ft = 440 * std::exp2((key - 69) / 12.0);
    std::vector<double> c;
    auto like = [&](long a, double lag) {  // how alike the sound is to itself `lag` samples on (40 ms)
      const int w = int(0.04 * kFs), L = int(std::lround(lag));
      if (a < 0 || a + w + L >= long(x.size())) return 0.0;
      double cc = 0, e0 = 0, e1 = 0;
      for (int i = 0; i < w; ++i) { cc += double(x[a + i]) * x[a + i + L]; e0 += double(x[a + i]) * x[a + i]; e1 += double(x[a + i + L]) * x[a + i + L]; }
      return cc / std::sqrt(e0 * e1 + 1e-30);
    };
    int rough = 0, heardN = 0;
    std::vector<double> early;
    for (double t = at[0] + 0.05; t < at[0] + 0.45; t += 0.01) {
      double cl, rms;
      const double v = pitchAt(x, long(t * kFs), ft, &cl, &rms);
      if (cl <= 0.6) continue;
      if (t < at[0] + 0.12) { early.push_back(v); continue; }
      if (t < at[0] + 0.2) continue;
      c.push_back(v);
      const double T = kFs / (ft * std::exp2(v / 1200));
      ++heardN;
      rough += like(long(t * kFs) - int(0.02 * kFs), 2 * T) > like(long(t * kFs) - int(0.02 * kFs), T) + 0.03;
    }
    if (heardN) h.rough = double(rough) / heardN;
    {  // (its level as the ear weighs it)
      const std::vector<float> xw = weighed(x);
      double e = 0;
      const size_t i0 = size_t((at[0] + 0.2) * kFs), i1 = std::min(xw.size(), size_t((at[0] + 0.45) * kFs));
      for (size_t i = i0; i < i1; ++i) e += double(xw[i]) * xw[i];
      if (i1 > i0) h.level = 10 * std::log10(e / (i1 - i0) + 1e-12);
    }
    if (c.size() < 8) return h;
    std::sort(c.begin(), c.end());
    h.off = c[c.size() / 2];
    h.sway = 0.5 * (c[c.size() * 9 / 10] - c[c.size() / 10]);
    {  // (the overtones: each partial's strength over 0.2 .. 0.45 s, at the pitch really sung)
      const double f0 = ft * std::exp2(h.off / 1200);
      const size_t i0 = size_t((at[0] + 0.2) * kFs), i1 = std::min(x.size(), size_t((at[0] + 0.45) * kFs));
      double p[5] = {0, 0, 0, 0, 0};
      if (i1 > i0 + 1000)
        for (int q = 0; q < 5; ++q) {
          const double w = 2 * M_PI * (q + 1) * f0 / kFs;
          double re = 0, im = 0;
          for (size_t i = i0; i < i1; ++i) {
            const double win = 0.5 - 0.5 * std::cos(2 * M_PI * double(i - i0) / double(i1 - i0));
            re += win * x[i] * std::cos(w * double(i - i0));
            im += win * x[i] * std::sin(w * double(i - i0));
          }
          p[q] = 10 * std::log10(re * re + im * im + 1e-20);
        }
      h.rich = std::clamp(0.25 * (p[1] + p[2] + p[3] + p[4]) - p[0], -40.0, 10.0);
      // (the spectrum of 8192 samples from 0.2 s on: what lies within 1.2 % or 12 Hz of a harmonic, and the rest)
      const int N = 8192;
      if (i0 + N <= x.size()) {
        std::vector<std::complex<double>> a(N);
        for (int i = 0; i < N; ++i) a[i] = x[i0 + i] * (0.5 - 0.5 * std::cos(2 * M_PI * i / N));
        for (int i = 1, j = 0; i < N; ++i) {
          int bit = N >> 1;
          for (; j & bit; bit >>= 1) j ^= bit;
          j ^= bit;
          if (i < j) std::swap(a[i], a[j]);
        }
        for (int len = 2; len <= N; len <<= 1) {
          const std::complex<double> wl = std::polar(1.0, -2 * M_PI / len);
          for (int i = 0; i < N; i += len) {
            std::complex<double> w = 1;
            for (int k = 0; k < len / 2; ++k, w *= wl) {
              const std::complex<double> u = a[i + k], v = a[i + k + len / 2] * w;
              a[i + k] = u + v; a[i + k + len / 2] = u - v;
            }
          }
        }
        double har = 0, rest = 0;
        for (int k = 1; k < N / 2; ++k) {
          const double f = k * kFs / N;
          if (f < 100 || f > 12000) continue;
          const double hn = std::max(1.0, std::round(f / f0));
          (std::fabs(f - hn * f0) < std::max(12.0, 0.012 * hn * f0) ? har : rest) += std::norm(a[k]);
        }
        h.clean = std::clamp(10 * std::log10((har + 1e-30) / (rest + 1e-30)), -20.0, 40.0);
      }
    }
    std::sort(early.begin(), early.end());
    h.early = early.empty() ? h.off : early[early.size() / 2];
    return h;
  };
  // An interval's cost (ms): how long after the second note's beginning until it is there (within
  // 20 c, pitched, not 8 dB under its own level) for good; what it overshoots past 15 c (2 ms a
  // cent); the first note's time given away (half of leaveMs); 600 if it is never there.
  auto costOne = [&](int vi, int kind, int from, int iv, const Way* w, bool plain, char v1, char v2) {
    std::vector<double> at;
    const std::vector<float> x = sing2(vi, from, from + iv, kind, v1, v2, w, false, nullptr, plain, &at);
    if (at.size() < 2 || at[1] < 0) return 600.0;
    const double ft = 440 * std::exp2((from + iv - 69) / 12.0);
    std::vector<double> cs, cls, lv;
    // (the phrase's last note sounds shorter than written: heard as far as it lasts, 60 ms short of the end)
    const double avail = std::min(0.40, double(x.size()) / kFs - at[1] - 0.09);
    if (avail < 0.15) return 600.0;
    for (double t = 0; t < avail; t += 0.005) {
      double cl, rms;
      cs.push_back(pitchAt(x, long((at[1] + t) * kFs), ft, &cl, &rms));
      cls.push_back(cl);
      lv.push_back(20 * std::log10(rms + 1e-9));
    }
    if (std::getenv("PJ_SINGER_PRACTICE_DEBUG")) {
      std::fprintf(stderr, "vowels at %.3f %.3f of %.3f s\n", at[0], at[1], x.size() / kFs);
      for (size_t i = 0; i < cs.size(); i += 2) std::fprintf(stderr, "  %3.0f ms: %+7.1f c clarity %.2f level %.1f dB\n", 5.0 * i, cs[i], cls[i], lv[i]);
    }
    std::vector<double> late(lv.begin() + int(lv.size() * 0.6), lv.end());
    std::sort(late.begin(), late.end());
    const double level = late[late.size() / 2];
    int lastBad = -1, firstGood = -1;
    double over = 0;
    for (int i = 0; i < int(cs.size()); ++i) {
      const bool there = cls[i] > 0.6 && std::fabs(cs[i]) < 20 && lv[i] > level - 8;
      if (!there) lastBad = i;
      else if (firstGood < 0) firstGood = i;
      if (firstGood >= 0 && cls[i] > 0.6) over = std::max(over, std::fabs(cs[i]));
    }
    if (firstGood < 0 || lastBad >= int(cs.size()) - 12) return 600.0;
    return 5.0 * (lastBad + 1) + 2 * std::max(0.0, over - 15) + (w ? 0.5 * w->leaveMs + 1.0 * std::fabs(w->dipDb) + 0.05 * std::fabs(w->over) : 17.5);
  };
  // (on one vowel held, and on a vowel changing with the note: a way has to serve both)
  auto costOf = [&](int vi, int kind, int from, int iv, const Way* w, bool plain) {
    return 0.5 * (costOne(vi, kind, from, iv, w, plain, 'a', 'a') + costOne(vi, kind, from, iv, w, plain, 'u', 'i'));
  };
  if (const char* dbg = std::getenv("PJ_SINGER_PRACTICE_DEBUG")) {  // "voice kind from interval": one interval's track
    int vi = 2, kind = 0, from = 48, iv = 4;
    std::sscanf(dbg, "%d %d %d %d", &vi, &kind, &from, &iv);
    Way w;
    std::fprintf(stderr, "cost %.0f\n", costOf(vi, kind, from, iv, &w, true));
    return 0;
  }
  // The folds themselves (-singer-voices): for each voice, which mass, which passaggio and how fast
  // the swinging part thins above it sing its whole range best: every second note on three vowels
  // (opened where the pitch passes them), by ear alone; the misses, the unsteadiness, the notes
  // with no pitch and the unevenness of the level from note to note counted. Prints the best.
  if (std::getenv("PJ_SINGER_PRACTICE_VOICES_MODE")) {
    struct Try { int vi; float mass, knee, power; };
    std::vector<Try> tries;
    for (int vi = 0; vi < kNVoices; ++vi) {
      if (only && std::strcmp(only, kVoices[vi].name) != 0) continue;
      for (float m : {0.35f, 0.5f, 0.7f, 1.0f, 1.4f})
        for (float k : {0.5f, 0.7f, 1.0f, 1.3f})
          for (float pw : {1.0f, 1.5f, 2.0f, 2.5f}) tries.push_back({vi, kVoices[vi].mass * m, kVoices[vi].kneeHz * k, pw});
    }
    const std::string part = std::string(outPath) + ".voices";
    std::vector<pid_t> kids;
    for (int k = 0; k < jobs; ++k) {
      const pid_t pid = fork();
      if (pid == 0) {
        std::FILE* f = std::fopen((part + std::to_string(k)).c_str(), "w");
        if (!f) _exit(1);
        for (size_t i = k; i < tries.size(); i += jobs) {
          const Try& t = tries[i];
          VoiceType vt = kVoices[t.vi];
          vt.mass = t.mass; vt.kneeHz = t.knee; vt.power = t.power;
          gVoiceTry = &vt;
          double score = 0, miss = 0, sway = 0;
          int mute = 0, n = 0;
          for (char v : {'a', 'u', 'i'}) {
            std::vector<double> lv;
            for (int key = vt.lo; key <= vt.hi; key += 2) {
              const SingerTrim row{t.vi, key, v, 0, 1, 1.0f, 0};
              const Heard h = hear(t.vi, key, v, &row);
              ++n;
              if (h.off > 1e8) { ++mute; score += 150; lv.push_back(-60); continue; }
              score += std::min(std::fabs(h.off), 100.0) + 2 * std::min(h.sway, 50.0);
              miss += std::min(std::fabs(h.off), 100.0); sway += std::min(h.sway, 50.0);
              lv.push_back(h.level);
            }
            for (size_t q = 1; q + 1 < lv.size(); ++q) score += 2 * std::min(20.0, std::fabs(lv[q] - 0.5 * (lv[q - 1] + lv[q + 1])));
          }
          std::fprintf(f, "%d %.3f %.0f %.1f %.1f %d %.1f %.1f\n", t.vi, t.mass, t.knee, t.power, score / n, mute, miss / std::max(1, n - mute), sway / std::max(1, n - mute));
          std::fflush(f);
        }
        std::fclose(f);
        _exit(0);
      }
      kids.push_back(pid);
    }
    for (pid_t pid : kids) { int st = 0; waitpid(pid, &st, 0); }
    struct Row { int vi; float mass, knee, power, score; int mute; float miss, sway; };
    std::vector<Row> rows;
    for (int k = 0; k < jobs; ++k) {
      std::FILE* f = std::fopen((part + std::to_string(k)).c_str(), "r");
      if (!f) continue;
      Row r;
      while (std::fscanf(f, "%d %f %f %f %f %d %f %f", &r.vi, &r.mass, &r.knee, &r.power, &r.score, &r.mute, &r.miss, &r.sway) == 8) rows.push_back(r);
      std::fclose(f);
      std::remove((part + std::to_string(k)).c_str());
    }
    std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.vi != b.vi ? a.vi < b.vi : a.score < b.score; });
    int last = -1, shown = 0;
    for (const Row& r : rows) {
      if (r.vi != last) {
        last = r.vi; shown = 0;
        for (const Row& q : rows)
          if (q.vi == r.vi && std::fabs(q.mass - kVoices[r.vi].mass) < 1e-3 && std::fabs(q.knee - kVoices[r.vi].kneeHz) < 0.5 && q.power == kVoices[r.vi].power)
            std::printf("%-12s as it is: mass %.2f knee %.0f Hz power %.1f: score %.1f, %d notes without pitch, miss %.1f c, sway %.1f c\n", kVoices[r.vi].name,
                        q.mass, q.knee, q.power, q.score, q.mute, q.miss, q.sway);
      }
      if (shown++ < 3)
        std::printf("%-12s   best %d: mass %.2f knee %.0f Hz power %.1f: score %.1f, %d notes without pitch, miss %.1f c, sway %.1f c\n", kVoices[r.vi].name, shown,
                    r.mass, r.knee, r.power, r.score, r.mute, r.miss, r.sway);
    }
    return 0;
  }
  const std::string out = outPath;
  std::vector<std::string> trims, ways;
  struct Sum { double tb = 0, ta = 0, lb = 0, la = 0, rb = 0, ra = 0, eb = 0, ea = 0, hb = 0, ha = 0, press = 0, wb[2] = {0, 0}, wa[2] = {0, 0}; int nt = 0, ntb = 0, opened = 0, nw[2] = {0, 0}, failB[2] = {0, 0}, failA[2] = {0, 0}, mute = 0; };
  Sum sum[kNVoices];
  // Two rounds: the notes' trims first; then the intervals, sung with the trims in.
  for (int phase = 0; phase < 2; ++phase) {
  std::fprintf(stderr, phase == 0 ? "every note on every vowel...\n" : "every interval, slurred and direct...\n");
  std::vector<pid_t> kids;
  for (int k = 0; k < jobs; ++k) {
    const pid_t pid = fork();
    if (pid < 0) { std::perror("fork"); return 1; }
    if (pid == 0) {
      std::FILE* f = std::fopen((out + ".part" + std::to_string(k)).c_str(), "w");
      if (!f) _exit(1);
      size_t turn = 0;
      for (size_t i = 0; i < cases.size(); ++i) {
        const Case& c = cases[i];
        if ((c.kind == 2) != (phase == 0) || turn++ % size_t(jobs) != size_t(k)) continue;
        if (c.kind == 2) {
          // The note on this vowel, with the least effort: sung and heard, then the pitch corrected
          // by most of its miss and the breath set for the level every vowel is to have on this
          // pitch (a vowel that comes out loud is sung on less breath), four times; where the pitch
          // is near or above the vowel's first resonance also with the vowel opened to it. The best
          // kept: on pitch, steady, at the level, on little breath.
          const char v = kVowels[c.interval];
          static const double kRise = knobD("PJ_SINGER_RISE_DB", 4);
          const double want = -34 + kRise * (c.from - kVoices[c.voice].lo) / 12.0;
          const Heard before = hear(c.voice, c.from, v, nullptr);
          SingerTrim best{c.voice, c.from, v, 0, 1, 0, float(before.level)};
          double bestScore = 1e18;
          Heard bestHeard = before;
          const double f0 = 440 * std::exp2((c.from - 69) / 12.0), f1 = Synth::tableFormant(phOf(v), 0);
          for (float f1x : {0.0f, 0.35f, 0.7f, 1.0f}) {  // (the mouth opened: Expr::open)
            if (f1x > 0 && f0 < 0.6 * f1) break;
            SingerTrim row{c.voice, c.from, v, 0, 1, f1x, 0};
            for (int it = 0; it < 4; ++it) {
              const Heard h = hear(c.voice, c.from, v, &row);
              if (h.off > 1e8) break;
              row.levelDb = float(h.level);
              const double score = std::fabs(h.off) + 2 * h.sway + 3 * std::fabs(h.level - want) + 4 * std::max(0.0, 20 * std::log10(double(row.press))) + (f1x > 0 ? 2 + 3 * f1x : 0) +
                                   100 * h.rough + 0.5 * std::min(100.0, std::fabs(h.early - h.off)) +
                                   0.4 * std::max(0.0, -8 - h.rich) +  // (overtones weaker than 8 dB under the fundamental cost)
                                   2.0 * std::max(0.0, 20 - h.clean);  // (a screech: the harmonics less than 20 dB over the rest)
              if (score < bestScore) { bestScore = score; best = row; bestHeard = h; }
              row.cents = float(std::clamp(row.cents - 0.8 * h.off, -90.0, 90.0));
              row.press = float(std::clamp(row.press * std::exp(kLn10 * ((want - h.level) / 20 / 1.5)), 0.45, 1.5));
            }
          }
          std::fprintf(f, "T %d %d %c %.1f %.3f %.2f %.1f %.1f %.1f %.1f %.1f %.1f %.2f %.2f %.0f %.0f %.1f %.1f\n", c.voice, c.from, v, best.cents, best.press, best.f1x, best.levelDb,
                       before.off, before.level - want, bestHeard.off, bestHeard.level - want, bestScore > 1e17 ? 1.0 : 0.0, before.rough, bestHeard.rough,
                       std::fabs(before.early - before.off), std::fabs(bestHeard.early - bestHeard.off), before.rich, bestHeard.rich);
        } else {
          // The way: one thing changed at a time, twice round, from the plain way.
          static const float kLeave[] = {10, 20, 35, 60, 90}, kArrive[] = {10, 20, 35, 60, 90}, kOver[] = {-40, -20, 0, 20, 40, 80},
                             kOverMs[] = {40, 80, 150}, kDip[] = {-4, -2, 0, 2};
          Way w;
          const double before = costOf(c.voice, c.kind, c.from, c.interval, &w, true);
          double best = before;
          auto tryAll = [&](float Way::*m, const float* vals, int nv) {
            for (int q = 0; q < nv; ++q) {
              Way t = w;
              if (t.*m == vals[q]) continue;
              t.*m = vals[q];
              const double cst = costOf(c.voice, c.kind, c.from, c.interval, &t, true);
              if (cst < best - 1) { best = cst; w = t; }
            }
          };
          for (int round = 0; round < 2; ++round) {
            tryAll(&Way::arriveMs, kArrive, 5);
            tryAll(&Way::over, kOver, 6);
            if (w.over != 0) tryAll(&Way::overMs, kOverMs, 3);
            tryAll(&Way::leaveMs, kLeave, 5);
            tryAll(&Way::dipDb, kDip, 4);
          }
          std::fprintf(f, "W %d %d %d %d %.0f %.0f %.0f %.0f %.0f %.0f %.0f\n", c.voice, c.kind, c.from, c.interval, w.leaveMs, w.arriveMs, w.over, w.overMs,
                       w.dipDb, before, best);
        }
        std::fflush(f);
      }
      std::fclose(f);
      _exit(0);
    }
    kids.push_back(pid);
  }
  bool ok = true;
  for (pid_t pid : kids) {
    int st = 0;
    waitpid(pid, &st, 0);
    ok = ok && WIFEXITED(st) && WEXITSTATUS(st) == 0;
    if (WIFSIGNALED(st)) std::fprintf(stderr, "a worker ended on signal %d\n", WTERMSIG(st));
    else if (WIFEXITED(st) && WEXITSTATUS(st)) std::fprintf(stderr, "a worker ended with status %d\n", WEXITSTATUS(st));
  }
  if (!ok) { std::fprintf(stderr, "the singer's practice: a worker failed\n"); return 1; }
  for (int k = 0; k < jobs; ++k) {
    const std::string part = out + ".part" + std::to_string(k);
    std::FILE* f = std::fopen(part.c_str(), "r");
    if (!f) continue;
    char line[256];
    while (std::fgets(line, sizeof line, f)) {
      int vi, a, b, c2;
      char v;
      float x1, x2, x3, x4, x5, before, after;
      float pr, f1x, lv, lb, la, mute, rb, ra, eb, ea, hb, ha;
      if (std::sscanf(line, "T %d %d %c %f %f %f %f %f %f %f %f %f %f %f %f %f %f %f", &vi, &a, &v, &x1, &pr, &f1x, &lv, &before, &lb, &after, &la, &mute, &rb, &ra, &eb, &ea, &hb, &ha) == 18) {
        if (mute > 0.5f || before > 1e8f) {
          ++sum[vi].mute;
          if (mute > 0.5f) continue;
        }
        char row[160];
        const float plain = float(-34 + knobD("PJ_SINGER_RISE_DB", 4) * (a - kVoices[vi].lo) / 12.0) + lb;  // (sung plainly: the level wanted + how far over it)
        std::snprintf(row, sizeof row, "    {%d, %3d, '%c', %.1ff, %.3ff, %.2ff, %.1ff, %.1ff},  // %s: %+.0f c %+.1f dB -> %+.0f c %+.1f dB", vi, a, v, x1, pr, f1x, lv, plain,
                      kVoices[vi].name, before > 1e8f ? 999.0f : before, lb, after, la);
        trims.push_back(row);
        gTrimsNow.push_back(SingerTrim{vi, a, v, x1, pr, f1x, lv, plain});
        if (before < 1e8f) { sum[vi].tb += std::fabs(before); sum[vi].lb += std::fabs(lb); sum[vi].rb += rb; sum[vi].eb += eb; ++sum[vi].ntb; }
        sum[vi].ra += ra; sum[vi].ea += ea; sum[vi].ha += ha;
        if (before < 1e8f) sum[vi].hb += hb;
        sum[vi].ta += std::fabs(after); sum[vi].la += std::fabs(la); sum[vi].press += pr; sum[vi].opened += f1x > 0; ++sum[vi].nt;
      } else if (std::sscanf(line, "W %d %d %d %d %f %f %f %f %f %f %f", &vi, &a, &b, &c2, &x1, &x2, &x3, &x4, &x5, &before, &after) == 11) {
        char row[180];
        std::snprintf(row, sizeof row, "    {%d, %d, %3d, %3d, %.1ff, %.1ff, %.1ff, %.1ff, %.1ff},  // %s: %.0f -> %.0f ms", vi, a, b, c2, x1, x2, x3, x4, x5,
                      kVoices[vi].name, before, after);
        if (after < 590) ways.push_back(row);  // (a way that never arrives is no way: the plain one stays)
        sum[vi].wb[a] += before; sum[vi].wa[a] += after; ++sum[vi].nw[a];
        sum[vi].failB[a] += before >= 590; sum[vi].failA[a] += after >= 590;
      }
    }
    std::fclose(f);
    std::remove(part.c_str());
  }
  }  // (phase)
  std::sort(trims.begin(), trims.end());
  std::sort(ways.begin(), ways.end());
  std::FILE* f = std::fopen(outPath, "w");
  if (!f) { std::perror(outPath); return 1; }
  std::fprintf(f, "// What the lead singer has practised (./pajupilli -singer-practice writes this file; src/jazzsinger.h).\n");
  std::fprintf(f, "// voice:");
  for (int vi = 0; vi < kNVoices; ++vi) std::fprintf(f, " %d %s%s", vi, kVoices[vi].name, vi + 1 < kNVoices ? "," : ".\n");
  std::fprintf(f, "// Notes: {voice, key, vowel, cents added to the pitch asked, breath x, the mouth opened (0 .. 1: towards ah, jaw dropped, lips flared), its level then (dB), its level sung plainly (dB)}.\n");
  std::fprintf(f, "// Ways: {voice, kind (0 slurred, 1 direct), from key, interval, leaveMs, arriveMs, over, overMs, dipDb}.\n");
  std::fprintf(f, "struct SingerTrim { int voice, key; char vowel; float cents, press, f1x, levelDb, plainDb = 0; };\n");
  std::fprintf(f, "struct SingerWay { int voice, kind, from, interval; float leaveMs, arriveMs, over, overMs, dipDb; };\n");
  std::fprintf(f, "static const SingerTrim kSingerTrim[] = {\n");
  for (const std::string& r : trims) std::fprintf(f, "%s\n", r.c_str());
  std::fprintf(f, "    {0, 0, 0, 0, 1, 0, 0},\n};\nstatic const SingerWay kSingerWay[] = {\n");
  for (const std::string& r : ways) std::fprintf(f, "%s\n", r.c_str());
  std::fprintf(f, "    {0, -1, 0, 0, 0, 0, 0, 0, 0},\n};\n");
  std::fprintf(f, "// What each voice knows of its folds: {voice, key, the tension the pitch takes (x), how loud it comes out (dB, as the ear weighs it)}.\n");
  std::fprintf(f, "struct SingerKnown { int voice, key; float tension, levelDb; };\nstatic const SingerKnown kSingerKnown[] = {\n");
  for (int vi = 0; vi < kNVoices; ++vi) {
    if (only && std::strcmp(only, kVoices[vi].name) != 0) continue;
    const Known& kn = knownOf(vi);
    for (size_t k = 0; k < kn.tension.size(); ++k)
      std::fprintf(f, "    {%d, %d, %.4ff, %.1ff},\n", vi, kVoices[vi].lo - kMapPad + int(k), kn.tension[k], kn.levelDb[k]);
  }
  std::fprintf(f, "    {-1, 0, 1, 0},\n};\n");
  std::fclose(f);
  for (int vi = 0; vi < kNVoices; ++vi) {
    const Sum& q = sum[vi];
    if (!q.nt && !q.mute) continue;
    std::printf("%-12s notes x vowels: %3d sing, %2d had no pitch at first; pitch miss %.1f -> %.1f c; level off the vowels' common one %.1f -> %.1f dB; breath x%.2f; %d vowels opened\n",
                kVoices[vi].name, q.nt, q.mute, q.ntb ? q.tb / q.ntb : 0, q.nt ? q.ta / q.nt : 0, q.ntb ? q.lb / q.ntb : 0, q.nt ? q.la / q.nt : 0,
                q.nt ? q.press / q.nt : 0, q.opened);
    std::printf("%-12s   rough (half-pitch rattle) %.0f %% -> %.0f %% of the sound; the first 0.1 s off the note's own pitch by %.0f -> %.0f c\n", kVoices[vi].name,
                q.ntb ? 100 * q.rb / q.ntb : 0, q.nt ? 100 * q.ra / q.nt : 0, q.ntb ? q.eb / q.ntb : 0, q.nt ? q.ea / q.nt : 0);
    std::printf("%-12s   overtones 2 to 5 against the fundamental %.1f -> %.1f dB\n", kVoices[vi].name, q.ntb ? q.hb / q.ntb : 0, q.nt ? q.ha / q.nt : 0);
    for (int k = 0; k < 2; ++k)
      std::printf("%-12s %s intervals: %3d, mean cost %.0f -> %.0f ms, never arriving %d -> %d\n", kVoices[vi].name, k ? "direct " : "slurred", q.nw[k],
                  q.nw[k] ? q.wb[k] / q.nw[k] : 0, q.nw[k] ? q.wa[k] / q.nw[k] : 0, q.failB[k], q.failA[k]);
  }
  std::printf("wrote %s; build again to use it\n", outPath);
  return 0;
}

}  // namespace pajupilli
