#include "jazzsinger.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>

#include "knob.h"
#include "puhuja/puhuja.h"

namespace pajupilli {

namespace {

using puhuja::Ph;
using puhuja::Synth;

constexpr double kFs = 48000;
// Where the voice holds its pitch (measured, the male voice with the tightened controller: the
// middle of every vowel within 13 c from D2 to C4; above, the folds leave the pitch asked).
constexpr int kLow = 40, kHigh = 60;  // E2 .. C4
constexpr int kMapLow = 36, kMapHigh = 64;

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
    default: return "?";
  }
}

// The voice as the singer uses it: puhuja's default speaker (string folds, line tract), with the
// singer's pitch control and larynx.
void voice(Synth& s, const std::vector<float>* map) {
  s.setCarrier(puhuja::Carrier::String);
  s.setFormantMotion(puhuja::FormantMotion::Planned);
  s.setTract(puhuja::Tract::Line);
  s.setSpellingRules(false);
  s.setFricationGain(4.5f);
  s.setDarkL(true);
  s.setAspiration(true);
  // (the ear: 0.6 of the error a cycle, learning wherever the folds swing at all; the speaker's
  // 0.30 and 98 cm^3/s never engaged at sung pitches)
  static const float kShare = float(knobD("PJ_SINGER_EAR", 0.6)), kVib = float(knobD("PJ_SINGER_EAR_VIB", 3));
  s.setPitchControl(kShare, kVib);
  // (the larynx low and still: 0.6 cm longer a tract, no rise in s k g r; to be set by ear)
  static const float kLarynxLow = float(knobD("PJ_SINGER_LARYNX_LOW", 0.6)), kLarynxRaise = float(knobD("PJ_SINGER_LARYNX_RAISE", 0));
  s.setLarynx(kLarynxLow, kLarynxRaise);
  if (map && !map->empty()) s.setTensionMap(float(440 * std::exp2((kMapLow - 69) / 12.0)), *map);
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
const std::vector<float>& tensionMap() {
  static std::vector<float> map;
  static std::once_flag once;
  std::call_once(once, [] {
    if (knobD("PJ_SINGER_KNOWN", 1) <= 0) return;
    const int n = kMapHigh - kMapLow + 1;
    std::vector<double> sum(n, 0.0);
    std::vector<int> cnt(n, 0);
    for (const Ph v : {Ph::AA, Ph::U}) {
      auto scale = [&](Synth& s) {
        voice(s, nullptr);
        const int base = (kMapLow + kMapHigh) / 2;
        s.setBaseF0(float(440 * std::exp2((base - 69) / 12.0)));
        for (int k = 0; k < n; ++k) {
          s.setWord(k + 1);
          s.push(Ph::D, true);
          Synth::Shape sh;
          sh.ms = 300;
          s.shapeNext(sh);
          s.setPitch(float(kMapLow + k - base));
          s.setPitchEnd(float(kMapLow + k - base));
          s.push(v);
        }
        s.finish();
      };
      Synth a{float(kFs)};
      scale(a);
      pullAll(a);
      std::vector<double> at(n, -1);
      for (const Synth::Label& l : a.labels())
        if (l.word >= 1 && l.word <= n && !l.wordStart) at[l.word - 1] = l.start + 0.85 * (l.end - l.start);
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
        if (c > 0.6f && c < 1.7f) { sum[k] += c; ++cnt[k]; }
      }
    }
    map.assign(n, 1.0f);
    for (int k = 0; k < n; ++k)
      if (cnt[k]) map[k] = float(sum[k] / cnt[k]);
    // (a pitch never read: its neighbours'; then smoothed over three semitones, the ear's own
    // unrest taken out)
    std::vector<float> sm(map);
    for (int k = 0; k < n; ++k) {
      const float l = map[std::max(k - 1, 0)], r = map[std::min(k + 1, n - 1)];
      sm[k] = 0.25f * l + 0.5f * map[k] + 0.25f * r;
    }
    map = sm;
  });
  return map;
}

uint32_t mix(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15; x *= 0x846ca68bu; x ^= x >> 16;
  return x;
}

}  // namespace

std::string JazzSinger::words() const {
  std::string out;
  for (const Phrase& p : phrase_) {
    for (int i = p.first; i < p.first + p.count; ++i) {
      const Syllable& y = syl_[i];
      if (i > p.first) out += ' ';
      for (char c : y.cons)
        if (c) out += c == 'j' ? 'y' : c;
      out += spell(y.nucleus);
      if (y.coda) out += y.coda;
    }
    out += '\n';
  }
  return out;
}

void JazzSinger::setScore(std::vector<SungNote> notes, std::vector<double> others, Style style) {
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
}

void JazzSinger::plan(Style style) {
  const int n = int(note_.size());
  // Into the voice's range, by octaves: the move that leaves the least of the part outside it (by
  // the notes' lengths), then nearest the range's middle; a note still outside, by its own octave.
  {
    double best = 1e30;
    for (int k = -48; k <= 24; k += 12) {
      double out = 0, far = 0;
      for (const SungNote& s : note_) {
        const int key = s.key + k;
        out += (s.off - s.on) * std::max({0, kLow - key, key - kHigh});
        far += (s.off - s.on) * std::abs(key - (kLow + kHigh) / 2);
      }
      const double cost = 1000 * out + far;
      if (cost < best) { best = cost; shift_ = k; }
    }
    for (SungNote& s : note_) {
      s.key += shift_;
      while (s.key > kHigh) s.key -= 12;
      while (s.key < kLow) s.key += 12;
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
  syl_.assign(n, Syllable());
  std::vector<bool> fixed(n, false);
  for (const Phrase& p : phrase_) {
    const int a = p.first, b = p.first + p.count;
    auto ioi = [&](int i) { return i + 1 < b ? note_[i + 1].on - note_[i].on : note_[i].off - note_[i].on; };
    auto bound = [&](int i) { return i > a && note_[i].on - note_[i - 1].off < 0.03; };  // legato from the note before
    char lastV = 0;
    int sameV = 0;
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
                                       (busy > 8 ? 0.1 : 0.0), 0.0, 1.0);
      const bool urgent = loud > 0.85 && (busy > 6 || s.accent > 0.5);
      auto vowelBy = [&]() -> char {  // the colour by the note's energy
        if (energy < 0.42) return (r >> 3) % 3 == 0 ? 'o' : 'u';
        if (energy < 0.68) return 'a';
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
        // Jazz. First the figures the line itself asks for.
        // "doo-dl-ah": a quick note between two longer ones is the tongue's flick.
        if (!first && !last && d < 0.16 && ioi(i - 1) > 1.6 * d && ioi(i + 1) > 1.6 * d && !fixed[i + 1] && syl_[i - 1].nucleus != 'l' &&
            syl_[i - 1].nucleus != 'n') {
          y.cons[0] = 'd'; y.nucleus = 'l';
          syl_[i + 1].cons[0] = 0; syl_[i + 1].nucleus = lastV == 'a' ? 'u' : 'a'; fixed[i + 1] = true;
          lastV = 'l';
          continue;
        }
        if (ghost) {  // a ghost note: swallowed, a nasal
          y.cons[0] = 'd'; y.nucleus = 'n';
          lastV = 'n';
          continue;
        }
        // The consonant: the articulation.
        const bool fastLine = d < 0.2 || (i > a && ioi(i - 1) < 0.2);
        if (first) y.cons[0] = urgent ? 'b' : 'd';
        else if (style == kBallad && leg) y.cons[0] = (r % 3) ? 0 : "lwj"[(r >> 4) % 3];  // a ballad: mostly the vowels
        else if (leg && !fastLine) y.cons[0] = "lwj"[(r >> 4) % 3];  // bound and unhurried: liquids and glides
        else if (urgent) y.cons[0] = "bkg"[(r >> 5) % 3];
        else if (fastLine) y.cons[0] = style == kBebop ? (s.accent > 0.3 ? "tp"[r % 2] : ((i - a) % 2 ? 'b' : 'd')) : ((i - a) % 2 ? 'b' : 'd');
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
        // (not one colour on and on: after three notes of it, the next)
        if (v == lastV && ++sameV >= 3) { v = v == 'a' ? 'u' : v == 'u' ? 'a' : v == 'i' ? 'a' : 'u'; }
        if (v != lastV) sameV = 0;
        y.nucleus = v;
      }
      lastV = y.nucleus;
    }
  }
}

// One rendering of a phrase with the given lengths (ms): each note's sung part, and the rest after
// it. Returns the sound; vowelAt: when each note's sung part began in it (s); lead: the first's.
std::vector<float> JazzSinger::take(const Phrase& p, const std::vector<double>& vowelMs, const std::vector<double>& restMs,
                                    std::vector<double>* vowelAt, double* lead) const {
  Synth s{float(kFs)};
  voice(s, &tensionMap());
  std::vector<int> keys;
  for (int i = p.first; i < p.first + p.count; ++i) keys.push_back(note_[i].key);
  std::sort(keys.begin(), keys.end());
  const int base = keys[keys.size() / 2];
  s.setBaseF0(float(440 * std::exp2((base - 69) / 12.0)));
  static const double kLevelLo = knobD("PJ_SINGER_LEVEL_LO", 0.6), kLevelHi = knobD("PJ_SINGER_LEVEL_HI", 1.4);
  std::vector<int> ncons(p.count, 0);
  for (int j = 0; j < p.count; ++j) {
    const SungNote& n = note_[p.first + j];
    const Syllable& y = syl_[p.first + j];
    s.setWord(j + 1);
    bool first = true;
    for (char c : y.cons) {
      if (!c) continue;
      s.push(phOf(c), first);
      first = false;
      ++ncons[j];
    }
    Synth::Shape sh;
    sh.level = float((kLevelLo + (kLevelHi - kLevelLo) * std::pow(n.vel / 127.0, 1.5)) * (1 + 0.15 * std::clamp(double(n.accent), -1.0, 1.0)));
    const float st = float(n.key - base);
    // (the pitch reached 35 ms into the sung part and held to 35 ms before its end: the way to the
    // next note is in the consonant. PJ_SINGER_SCOOP ms: later, a scoop.)
    static const double kScoop = knobD("PJ_SINGER_SCOOP", 35);
    auto hold = [&](double ms) {
      sh.sungAt = float(std::min(0.3, kScoop / std::max(ms, 1.0)));
      sh.sungEndAt = float(std::max(0.6, 1 - 35 / std::max(ms, 1.0)));
    };
    const double codaMs = y.coda ? std::min(90.0, 0.3 * vowelMs[j]) : 0;
    if (y.nucleus == 'E') {  // the diphthong: its first part 60 %
      sh.ms = float(0.6 * (vowelMs[j] - codaMs));
      hold(sh.ms);
      s.shapeNext(sh); s.setPitch(st); s.setPitchEnd(st);
      s.push(Ph::E, first);
      sh.ms = float(0.4 * (vowelMs[j] - codaMs));
      hold(sh.ms);
      s.shapeNext(sh); s.setPitch(st); s.setPitchEnd(st);
      s.push(Ph::I, false);
    } else {
      sh.ms = float(vowelMs[j] - codaMs);
      hold(sh.ms);
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
  std::vector<float> pcm = pullAll(s);
  vowelAt->assign(p.count, -1.0);
  std::vector<int> seen(p.count, 0);
  for (const Synth::Label& l : s.labels()) {
    if (l.word < 1 || l.word > p.count) continue;
    const int j = l.word - 1;
    if (seen[j]++ == ncons[j]) (*vowelAt)[j] = l.start;
  }
  *lead = (*vowelAt)[0] >= 0 ? (*vowelAt)[0] : 0.5;
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
        if (restMs[j] < 20) { vowelMs[j] += restMs[j]; restMs[j] = 0; }  // (too short for a rest: sung through)
      } else {
        vowelMs[j] = 1000 * std::max(0.08, s.off - s.on);
        restMs[j] = 0;
      }
    }
    pcm = take(p, vowelMs, restMs, &at, &lead);
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
    if (worst < best) { best = worst; bestPcm = pcm; bestLead = lead; }
    static const bool kTrace = knobD("PJ_SINGER_TRACE", 0) > 0;
    if (kTrace) {
      std::fprintf(stderr, "singer: phrase at %.2f s, pass %d, worst %.0f ms, lead %.0f ms:", t0, pass, 1000 * worst, 1000 * lead);
      for (int j = 0; j < n; ++j)
        std::fprintf(stderr, " %s%s%+.0f(v%.0f r%.0f)", syl_[p.first + j].cons, spell(syl_[p.first + j].nucleus),
                     at[j] < 0 ? -999.0 : 1000 * ((at[j] - lead) - (note_[p.first + j].on - t0)), vowelMs[j], restMs[j]);
      std::fprintf(stderr, "\n");
    }
    if (worst < 0.008) break;  // (puhuja's lengths come in steps of about 10 ms)
  }
  pcm = std::move(bestPcm);
  lead = bestLead;
  static const float kGain = float(knobD("PJ_SINGER_GAIN", 6.0));
  for (float& v : pcm) v *= kGain;
  p.pcm = std::move(pcm);
  p.start = t0 - lead;
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
  at_ += n;
}

}  // namespace pajupilli
