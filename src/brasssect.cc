// The brass section (General MIDI 62): the voicer, the plan from the song, and the section's
// playing. See src/brasssect.h.
#include "brasssect.h"
#include "heard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <sstream>

#include "director.h"
#include "ensemble.h"
#include "hornplayer.h"
#include "knob.h"

namespace pajupilli {

namespace {

// ---- The seats (research 11.1), top to bottom: a voicing never crosses them ----
//   name        label kind         range    at ease  place (m)
const BrassSeatDef kPopSeats[] = {
    {"trumpet", "tp1", kBrTrumpet, 58, 82, 65, 81, -0.5},
    {"trumpet", "tp2", kBrTrumpet, 55, 81, 60, 78, -1.2},
    {"trumpet", "tp3", kBrTrumpet, 54, 79, 58, 76, -1.9},
    {"trombone", "tb1", kBrTrombone, 40, 74, 46, 70, 0.7},
    {"trombone", "tb2", kBrTrombone, 38, 72, 43, 67, 1.5},
};
// (the trumpets in the back row, the trombones in front of them: two rows across the stage, the
// leads in the middle; the bass trombone at the end)
const BrassSeatDef kBigBandSeats[] = {
    {"trumpet", "tp1", kBrTrumpet, 58, 82, 67, 81, -0.3},
    {"trumpet", "tp2", kBrTrumpet, 55, 81, 62, 79, 0.6},
    {"trumpet", "tp3", kBrTrumpet, 54, 79, 60, 77, -1.2},
    {"trumpet", "tp4", kBrTrumpet, 54, 77, 58, 74, 1.5},
    {"trombone", "tb1", kBrTrombone, 41, 74, 48, 70, 0.3},
    {"trombone", "tb2", kBrTrombone, 40, 72, 45, 67, -0.6},
    {"trombone", "tb3", kBrTrombone, 40, 70, 43, 65, 1.2},
    {"trombone", "tb4", kBrTrombone, 36, 65, 38, 60, -1.5},
};
// (US seating: the horns left of centre, the trumpets in the middle, the trombones and tuba right)
const BrassSeatDef kOrchestralSeats[] = {
    {"trumpet", "tp1", kBrTrumpet, 58, 82, 62, 79, 0.2},
    {"trumpet", "tp2", kBrTrumpet, 55, 79, 60, 77, 0.9},
    {"horn", "hn1", kBrHorn, 45, 77, 53, 74, -0.9},
    {"horn", "hn3", kBrHorn, 45, 77, 52, 72, -1.6},
    {"horn", "hn2", kBrHorn, 43, 72, 48, 67, -2.3},
    {"horn", "hn4", kBrHorn, 41, 70, 45, 65, -3.0},
    {"trombone", "tb1", kBrTrombone, 40, 72, 46, 67, 1.7},
    {"trombone", "tb2", kBrTrombone, 40, 70, 43, 65, 2.4},
    {"trombone", "tb3", kBrTrombone, 36, 65, 38, 60, 3.1},
    {"tuba", "tuba", kBrTuba, 28, 58, 31, 53, 3.8},
};

// ---- Chords ----
struct ChordQ { const char* sym; int pcs[4]; int n; };
const ChordQ kBrassChords[] = {
    {"", {0, 4, 7}, 3},       {"m", {0, 3, 7}, 3},       {"7", {0, 4, 7, 10}, 4},  {"maj7", {0, 4, 7, 11}, 4},
    {"m7", {0, 3, 7, 10}, 4}, {"m7b5", {0, 3, 6, 10}, 4}, {"dim7", {0, 3, 6, 9}, 4}, {"dim", {0, 3, 6}, 3},
    {"6", {0, 4, 7, 9}, 4},   {"m6", {0, 3, 7, 9}, 4},   {"sus4", {0, 5, 7}, 3},    {"7sus4", {0, 5, 7, 10}, 4},
};
constexpr int kNumChords = int(sizeof kBrassChords / sizeof kBrassChords[0]);
enum { kQMaj, kQMin, kQDom7, kQMaj7, kQMin7, kQHalfDim, kQDim7, kQDim, kQ6, kQMin6, kQSus4, kQ7Sus4 };

unsigned maskOf(int q) {
  unsigned m = 0;
  for (int i = 0; i < kBrassChords[q].n; ++i) m |= 1u << kBrassChords[q].pcs[i];
  return m;
}
int thirdOf(int q) {  // (semitones over the root; -1: none)
  const unsigned m = maskOf(q);
  return m >> 4 & 1 ? 4 : m >> 3 & 1 ? 3 : -1;
}

const char* const kFlat[12] = {"C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B"};
std::string noteStr(int k) { return k < 0 ? "-" : std::string(kFlat[k % 12]) + std::to_string(k / 12 - 1); }
int pcOf(int k) { return ((k % 12) + 12) % 12; }

// How long each instrument takes to speak (s): the key to its sound within 10 dB of its steady
// level, measured on the players here (velocity 100, -seq): the trumpet ~20-33 ms, the trombone
// 27 ms low to ~50 ms high, the horn's long bore up to 85 ms, the tuba ~40 ms.
double speakTime(BrassKind kind, int note) {
  struct P { int k; double ms; };
  static const P kTp[] = {{58, 19}, {64, 30}, {70, 33}, {76, 28}, {82, 26}};
  static const P kTb[] = {{40, 32}, {46, 30}, {52, 30}, {58, 46}, {64, 52}, {70, 54}};  // (+4 ms: as heard in the section)
  static const P kHn[] = {{45, 34}, {52, 80}, {58, 76}, {64, 76}, {70, 84}, {76, 51}};
  static const P kTu[] = {{31, 44}, {36, 39}, {41, 42}, {46, 41}, {52, 40}};
  const P* t = kTp;
  int n = 5;
  if (kind == kBrTrombone) { t = kTb; n = 6; }
  if (kind == kBrHorn) { t = kHn; n = 6; }
  if (kind == kBrTuba) { t = kTu; n = 5; }
  if (note <= t[0].k) return t[0].ms / 1000;
  for (int i = 1; i < n; ++i)
    if (note <= t[i].k) return (t[i - 1].ms + (t[i].ms - t[i - 1].ms) * (note - t[i - 1].k) / double(t[i].k - t[i - 1].k)) / 1000;
  return t[n - 1].ms / 1000;
}

// The low interval limits (research 5.1, the user's "no close thirds below about E3/F3"): an
// interval of d semitones (1..11) whose lower note is under this is mud.
const int kLowLimit[12] = {0, 55, 53, 53, 52, 47, 48, 34, 43, 41, 41, 41};

bool lowLimitsOk(std::vector<int> v) {
  std::sort(v.begin(), v.end());
  v.erase(std::unique(v.begin(), v.end()), v.end());
  for (size_t i = 0; i + 1 < v.size(); ++i) {
    const int d = v[i + 1] - v[i];
    if (d < 12 && v[i] < kLowLimit[d]) return false;
  }
  return true;
}

// The chord heard in pitch-class weights w (a bass pitch class, -1: none): the best template;
// false if none is clear (at least three of its notes present, little outside it).
bool recognise(const double w[12], int bass, int* root, int* quality) {
  double total = 0, top = 0;
  for (int i = 0; i < 12; ++i) { total += w[i]; top = std::max(top, w[i]); }
  if (total <= 0) return false;
  double best = -1e9;
  for (int r = 0; r < 12; ++r)
    for (int q = 0; q < kNumChords; ++q) {
      const ChordQ& c = kBrassChords[q];
      double in = 0;
      int present = 0;
      unsigned m = 0;
      for (int i = 0; i < c.n; ++i) {
        const int pc = (r + c.pcs[i]) % 12;
        m |= 1u << pc;
        in += w[pc];
        present += w[pc] > 0.12 * top;
      }
      const double out = total - in;
      const bool third = thirdOf(q) >= 0 && w[(r + thirdOf(q)) % 12] > 0.12 * top;
      if (present < 3 && !(present >= 2 && third && bass == r)) continue;
      if (present < c.n - 1) continue;  // (a seventh chord with two of its notes missing is not one)
      if (out > 0.2 * total) continue;
      const double s = in - 1.5 * out + (bass == r ? 0.3 * total : 0) - 0.04 * total * c.n +
                       (present == c.n ? 0.05 * total : -0.1 * total);
      if (s > best) { best = s; *root = r; *quality = q; }
    }
  return best > -1e9;
}

// The parts' tones for voicing under a lead at li semitones over the root: the chord's own, a
// tension in the lead replacing what it stands for (9 for the root, 13 for the fifth, 11 on minor
// for the fifth); n parts. Empty: the lead is not one of them (a passing note).
std::vector<int> voicingTones(int q, int li, int n) {
  std::vector<int> t;
  switch (q) {
    case kQMaj: case kQ6:
      if (n >= 4) t = li == 11 ? std::vector<int>{0, 4, 7, 11} : std::vector<int>{0, 4, 7, 9};
      else t = li == 9 ? std::vector<int>{4, 7, 9} : li == 11 ? std::vector<int>{4, 7, 11} : std::vector<int>{0, 4, 7};
      break;
    case kQMaj7:
      // (the 6 for the 7 unless the lead is the 7 itself: no 7 against the root a second apart)
      t = li == 11 ? std::vector<int>{0, 4, 7, 11} : std::vector<int>{0, 4, 7, 9};
      if (n < 4) t = li == 11 ? std::vector<int>{4, 7, 11} : std::vector<int>{0, 4, 7};
      break;
    case kQDom7:
      t = n >= 4 ? std::vector<int>{0, 4, 7, 10} : std::vector<int>{4, 7, 10};
      if (n < 4 && li == 0) t = {0, 4, 10};
      break;
    case kQMin:
      if (n >= 4) t = li == 9 ? std::vector<int>{0, 3, 7, 9} : li == 11 ? std::vector<int>{0, 3, 7, 11} : std::vector<int>{0, 3, 7, 10};
      else t = {0, 3, 7};
      break;
    case kQMin7:
      t = n >= 4 ? std::vector<int>{0, 3, 7, 10} : std::vector<int>{3, 7, 10};
      if (n < 4 && li == 0) t = {0, 3, 10};
      break;
    case kQMin6: t = n >= 4 ? std::vector<int>{0, 3, 7, 9} : std::vector<int>{0, 3, 9}; break;
    case kQHalfDim: t = n >= 4 ? std::vector<int>{0, 3, 6, 10} : std::vector<int>{3, 6, 10}; break;
    case kQDim7: case kQDim: t = n >= 4 ? std::vector<int>{0, 3, 6, 9} : std::vector<int>{0, 3, 6}; break;
    case kQSus4: case kQ7Sus4: t = n >= 4 ? std::vector<int>{0, 5, 7, 10} : std::vector<int>{0, 5, 7}; break;
    default: return {};
  }
  auto has = [&](int pc) { return std::find(t.begin(), t.end(), pc) != t.end(); };
  if (has(li)) return t;
  // Tensions: the 9 for the root (any chord but the diminished), the 13 for the fifth (major,
  // dominant), the 11 for the fifth on minor.
  auto swap = [&](int from, int to) {
    for (int& x : t)
      if (x == from) { x = to; return true; }
    return false;
  };
  const bool minor = q == kQMin || q == kQMin7 || q == kQMin6;
  if (li == 2 && q != kQDim7 && q != kQDim && q != kQHalfDim && (swap(0, 2) || (n < 4 && (swap(7, 2) || true)))) {
    if (!has(2)) t.push_back(2);
    return t;
  }
  if (li == 9 && (q == kQDom7 || q == kQMaj7) && swap(7, 9)) return t;
  if (li == 5 && minor && swap(7, 5)) return t;
  return {};
}

}  // namespace

int brassLowLimit(int d) { return d >= 1 && d <= 11 ? kLowLimit[d] : 0; }

const BrassSeatDef* brassSeats(int mode, int* n) {
  if (mode == kBrassBigBand) { *n = int(sizeof kBigBandSeats / sizeof kBigBandSeats[0]); return kBigBandSeats; }
  if (mode == kBrassOrchestral) { *n = int(sizeof kOrchestralSeats / sizeof kOrchestralSeats[0]); return kOrchestralSeats; }
  *n = int(sizeof kPopSeats / sizeof kPopSeats[0]);
  return kPopSeats;
}

const char* brassModeName(int mode) {
  return mode == kBrassBigBand ? "big band" : mode == kBrassOrchestral ? "orchestral" : "pop";
}

bool parseBrassChord(const std::string& s, int* root, int* quality) {
  if (s.empty()) return false;
  static const int kL[7] = {9, 11, 0, 2, 4, 5, 7};  // A..G
  const char c = char(std::toupper(s[0]));
  if (c < 'A' || c > 'G') return false;
  int r = kL[c - 'A'];
  size_t i = 1;
  if (i < s.size() && (s[i] == '#' || s[i] == 'b')) { r += s[i] == '#' ? 1 : -1; ++i; }
  const std::string q = s.substr(i);
  for (int k = 0; k < kNumChords; ++k)
    if (q == kBrassChords[k].sym) { *root = (r + 12) % 12; *quality = k; return true; }
  return false;
}

std::string brassChordName(int root, int quality) {
  if (root < 0 || quality < 0) return "?";
  return std::string(kFlat[root]) + kBrassChords[quality].sym;
}

// ---- The voicer ----

BrassVoicer::BrassVoicer(int mode) : mode_(mode) { s_ = brassSeats(mode, &n_); }

// Monotone dealing of the keys (top down) over the seats (top down): every key at least one
// player, the spare ones doubling the lead, then the bottom (never an added octave, nor a low
// third), or resting; the lead seat on the top key when it reaches it. Least total cost: the
// comfort of each seat on its note, the motion from where it was, the trumpets on top and the low
// brass at the bottom. unison: per seat, may it play key 0 (a lone key: those at ease on it).
double BrassVoicer::deal(const std::vector<Key>& keys, const std::vector<int>& prev, const std::vector<bool>* unison,
                         std::vector<int>* out) const {
  const int K = int(keys.size()), P = n_;
  constexpr double kInf = 1e18;
  out->assign(P, -1);
  if (K == 0) return 0;
  // dp[c][m]: c keys covered, m players on key c - 1 so far. At most `cap` players a key (the
  // lead one more): a chord's notes are shared out, not all spares on the lead.
  const int cap = (P + K - 1) / K;
  auto capOf = [&](int j) { return cap + (j == 0 ? 1 : 0); };
  const int M = P + 1;
  std::vector<double> dp((K + 1) * M, kInf), nd((K + 1) * M);
  std::vector<std::vector<int>> from(P, std::vector<int>((K + 1) * M, -1));  // the state it came from
  std::vector<std::vector<int>> choice(P, std::vector<int>((K + 1) * M, -9));  // -1 rest, 0 double, 1 next
  dp[0] = 0;
  const int bottom = keys[K - 1].k;
  auto playCost = [&](int s, int j) {
    const BrassSeatDef& d = s_[s];
    const int k = keys[j].k;
    if (k < d.lo || k > d.hi) return kInf;
    if (unison && j == 0 && !(*unison)[s]) return kInf;
    if (d.kind == kBrTuba && (j != K - 1 || k > 53)) return kInf;     // (the tuba: the bottom, low down)
    if (keys[j].octave && d.kind == kBrTrumpet) return kInf;          // (an octave under the lead: the low brass)
    double c = 1.5 * (std::max(0, d.easyLo - k) + std::max(0, k - d.easyHi));
    // (the motion: from where it was, beyond the nearest key it could have taken; whether it
    // plays at all is not the motion's to decide)
    if (s < int(prev.size()) && prev[s] >= 0) {
      int near = 1000;
      for (const Key& q : keys) near = std::min(near, std::abs(q.k - prev[s]));
      c += 0.15 * (std::abs(k - prev[s]) - near);
    }
    if (K >= 2 && !(K == 2 && keys[1].octave)) {  // (the trumpets on top, the horns in the middle, the low brass under)
      const double rel = double(j) / (K - 1);
      const double target = d.kind == kBrTrumpet ? 0 : d.kind == kBrHorn ? 0.45 : d.kind == kBrTrombone ? 0.85 : 1.0;
      c += (mode_ == kBrassOrchestral ? 0.8 : 0.3) * std::fabs(rel - target);
    }
    return c;
  };
  auto restCost = [&](int s) {
    const BrassSeatDef& d = s_[s];
    if (d.kind == kBrTuba) return bottom <= 50 ? 1.5 : 0.0;
    if (std::strcmp(d.label, "tb4") == 0) return bottom <= 55 ? 1.0 : 0.3;  // (the bass trombone)
    return mode_ == kBrassOrchestral ? 0.8 : 1.0;
  };
  // (doubling: the lead first (and its octave), then the bottom; an inner note in pairs in the
  // orchestra's horns)
  auto dupCost = [&](int j) {
    double c = j == 0 || keys[j].octave ? 0.2 : j == K - 1 ? 0.5 : mode_ == kBrassOrchestral ? 0.35 : 1.0;
    if (keys[j].lowThird) c += 3;
    if (keys[j].added && !keys[j].octave) c += 0.3;
    return c;
  };
  for (int s = 0; s < P; ++s) {
    std::fill(nd.begin(), nd.end(), kInf);
    // (the lead seat takes the top key if it reaches it)
    const bool leadSeat = s == 0 && playCost(0, 0) < kInf;
    for (int c = 0; c <= K; ++c)
      for (int m = 0; m < M; ++m) {
        const double v = dp[c * M + m];
        if (v >= kInf) continue;
        auto put = [&](int c2, int m2, double x, int ch) {
          if (x < nd[c2 * M + m2]) { nd[c2 * M + m2] = x; choice[s][c2 * M + m2] = ch; from[s][c2 * M + m2] = c * M + m; }
        };
        if (!leadSeat) {
          put(c, m, v + restCost(s), -1);
          if (c >= 1 && m < capOf(c - 1)) {
            const double pc = playCost(s, c - 1);
            if (pc < kInf) put(c, m + 1, v + pc + dupCost(c - 1), 0);
          }
        }
        if (c < K && (!leadSeat || c == 0)) {
          const double pc = playCost(s, c);
          if (pc < kInf) put(c + 1, 1, v + pc, 1);
        }
      }
    dp.swap(nd);
  }
  int end = -1;
  for (int m = 0; m < M; ++m)
    if (dp[K * M + m] < kInf && (end < 0 || dp[K * M + m] < dp[end])) end = K * M + m;
  if (end < 0) return kInf;
  const double total = dp[end];
  for (int s = P - 1, st = end; s >= 0; --s) {
    const int ch = choice[s][st];
    if (ch != -1) (*out)[s] = keys[st / M - 1].k;
    st = from[s][st];
  }
  return total;
}

// A sparse onset voiced under its lead (and the other given key, if a dyad): the candidates of
// the mode, each dealt; the cheapest that keeps every practical rule.
bool BrassVoicer::harmonise(int lead, int other, const BrassHint& h, const std::vector<int>& prev, std::vector<int>* out,
                            std::string* how) const {
  if (h.root < 0 || h.quality < 0 || mode_ == kBrassOrchestral) return false;
  const int n = mode_ == kBrassBigBand ? 4 : 3;
  const int li = pcOf(lead - h.root);
  const std::vector<int> tones = voicingTones(h.quality, li, n);
  if (tones.empty()) return false;
  // Close position: the chord's tones under the lead, each once, within the octave.
  std::vector<int> close{lead};
  for (int p = lead - 1; p > lead - 12 && int(close.size()) < n; --p) {
    const int pc = pcOf(p - h.root);
    if (pc == li) continue;
    if (std::find(tones.begin(), tones.end(), pc) == tones.end()) continue;
    bool dup = false;
    for (int q : close) dup |= pcOf(q - h.root) == pc;
    if (!dup) close.push_back(p);
  }
  if (int(close.size()) < n) return false;
  std::vector<int> drop2 = close;
  drop2[1] -= 12;
  std::vector<int> drop24 = drop2;
  if (n >= 4) drop24[3] -= 12;
  const int third = h.quality >= 0 && thirdOf(h.quality) >= 0 ? (h.root + thirdOf(h.quality)) % 12 : -1;
  struct Cand { std::vector<int> v; std::string name; double cost; };
  std::vector<Cand> cands;
  auto down = [](std::vector<int> v) { for (int& x : v) x -= 12; return v; };
  auto join = [](std::vector<int> a, const std::vector<int>& b) { a.insert(a.end(), b.begin(), b.end()); return a; };
  if (mode_ == kBrassBigBand) {
    const double pc = lead >= 74 ? 0 : 0.8, pd = lead >= 74 ? 0.6 : 0;
    cands.push_back({join(close, down(close)), "4-way close, the trombones 8vb", pc});
    cands.push_back({join(drop2, down(drop2)), "drop-2, the trombones 8vb", pd});
    cands.push_back({join(close, down(drop2)), "4-way close, the trombones drop-2 8vb", pc + 0.3});
    cands.push_back({join(drop24, down(drop24)), "drop-2-4, the trombones 8vb", pd + 0.5});
    cands.push_back({join(close, {lead - 12}), "4-way close, the lead 8vb", pc + 1.0});
    cands.push_back({join(drop2, {lead - 12}), "drop-2, the lead 8vb", pd + 1.0});
    // (the trombones on the inner parts 8vb, the lead not doubled: where its octave would rub)
    const std::vector<int> inner(close.begin() + 1, close.end()), innerD(drop2.begin() + 1, drop2.end());
    cands.push_back({join(close, down(inner)), "4-way close, the trombones on the inner parts 8vb", pc + 0.6});
    cands.push_back({join(drop2, down(innerD)), "drop-2, the trombones on the inner parts 8vb", pd + 0.6});
    cands.push_back({close, "4-way close, no doubling", pc + 1.4});
    cands.push_back({drop2, "drop-2, no doubling", pd + 1.4});
  } else {
    const double pc = 0, pd = lead >= 76 ? 0.2 : 0.6;
    cands.push_back({join(close, {lead - 12}), "3-part close, the lead 8vb", pc});
    cands.push_back({join(drop2, {lead - 12}), "drop-2, the lead 8vb", pd});
    cands.push_back({close, "3-part close", pc + 0.4});
    cands.push_back({drop2, "drop-2", pd + 0.4});
  }
  double best = 1e17;
  for (Cand& c : cands) {
    std::vector<int> v = c.v;
    std::sort(v.begin(), v.end(), std::greater<int>());
    v.erase(std::unique(v.begin(), v.end()), v.end());
    if (v[0] != lead) continue;
    if (other >= 0 && std::find(v.begin(), v.end(), other) == v.end()) continue;
    if (!lowLimitsOk(v)) continue;
    // (no minor second or ninth between any two voices: a voicing, not a cluster)
    bool bad = false;
    for (size_t i = 0; i < v.size(); ++i)
      for (size_t j = i + 1; j < v.size(); ++j) bad |= v[i] - v[j] == 1 || v[i] - v[j] == 13;
    if (bad) continue;
    std::vector<Key> keys;
    int thirds = 0, lowThird = 0;
    for (int k : v) {
      const bool isThird = third >= 0 && pcOf(k) == third;
      thirds += isThird;
      lowThird += isThird && k < 60;
      keys.push_back({k, k != lead && k != other, false, k == lead - 12});
    }
    double cost = c.cost + (thirds > 1 && lowThird ? 3 : 0);
    std::vector<int> o;
    const double d = deal(keys, prev, nullptr, &o);
    if (d >= 1e17) continue;
    // (every part of the voicing played: a candidate whose notes the seats cannot all take loses)
    int played = 0;
    for (int k : v) played += std::find(o.begin(), o.end(), k) != o.end();
    if (played < int(v.size())) continue;
    cost += d;
    if (cost < best) {
      best = cost;
      *out = o;
      if (how) *how = c.name;
    }
  }
  return best < 1e17;
}

std::vector<int> BrassVoicer::voice(const std::vector<int>& keysIn, const BrassHint& h, const std::vector<int>& prev,
                                    std::string* how) const {
  std::vector<int> ks = keysIn;
  std::sort(ks.begin(), ks.end(), std::greater<int>());
  ks.erase(std::unique(ks.begin(), ks.end()), ks.end());
  std::vector<int> out(n_, -1);
  if (ks.empty()) {
    if (how) *how = "rest";
    return out;
  }
  int lo = 127, hi = 0;
  for (int i = 0; i < n_; ++i) { lo = std::min(lo, s_[i].lo); hi = std::max(hi, s_[i].hi); }
  // (a key beyond every seat: into the section's range by octaves, the only octave ever moved)
  for (int& k : ks) {
    while (k > hi) k -= 12;
    while (k < lo) k += 12;
  }
  std::sort(ks.begin(), ks.end(), std::greater<int>());
  ks.erase(std::unique(ks.begin(), ks.end()), ks.end());
  const int lead = ks[0];
  // The third of the harmony (the song's, or the keys' own): not doubled low down.
  int root = h.root, quality = h.quality;
  if (root < 0 && ks.size() >= 3) {
    double w[12] = {};
    for (int k : ks) w[pcOf(k)] += 1;
    if (!recognise(w, pcOf(ks.back()), &root, &quality)) root = quality = -1;
  }
  const int third = root >= 0 && quality >= 0 && thirdOf(quality) >= 0 ? (root + thirdOf(quality)) % 12 : -1;
  const bool octaveDyad = ks.size() == 2 && (ks[0] - ks[1]) % 12 == 0;
  // A sparse onset the song lets us voice.
  if (h.harmonise && (ks.size() == 1 || (ks.size() == 2 && !octaveDyad))) {
    std::string hw;
    if (harmonise(lead, ks.size() == 2 ? ks[1] : -1, h, prev, &out, &hw)) {
      if (how) *how = hw;
      return compact(out);
    }
    // A passing note between voiced ones (big band): the whole voicing moves with the lead
    // (planing), if it was voiced a step away.
    int pl = -1, distinct = 0;
    std::vector<int> pv;
    for (int p : prev)
      if (p >= 0) { pl = std::max(pl, p); if (std::find(pv.begin(), pv.end(), p) == pv.end()) pv.push_back(p); }
    distinct = int(pv.size());
    if (mode_ == kBrassBigBand && ks.size() == 1 && distinct >= 4 && pl >= 0 && std::abs(lead - pl) <= 2 && lead != pl &&
        h.len >= 0 && h.len < 0.4) {
      std::vector<Key> keys;
      for (int p : pv) keys.push_back({p + lead - pl, p != pl, false});
      std::sort(keys.begin(), keys.end(), [](const Key& a, const Key& b) { return a.k > b.k; });
      std::vector<int> v;
      for (const Key& k : keys) v.push_back(k.k);
      std::vector<int> o;
      if (lowLimitsOk(v) && deal(keys, prev, nullptr, &o) < 1e17) {
        int played = 0;
        for (int k : v) played += std::find(o.begin(), o.end(), k) != o.end();
        if (played == int(v.size())) {
          if (how) *how = "planed (a passing note)";
          return compact(o);
        }
      }
    }
  }
  std::vector<Key> keys;
  for (int k : ks) keys.push_back({k, false, third >= 0 && pcOf(k) == third && k < 60});
  static const bool kOctave = knob("PJ_BRASS_OCTAVE", 1) > 0;
  std::string what = keys.size() == 1 ? "unison" : octaveDyad ? "octaves" : "dealt";
  // Big band: a close chord of two to four keys in the trumpets, the trombones on it an octave
  // down (the section's own doubling), where that is no mud.
  if (mode_ == kBrassBigBand && kOctave && h.octaves && !octaveDyad && keys.size() >= 2 && keys.size() <= 4 &&
      ks.front() - ks.back() <= 12 && ks.back() - 12 >= 45) {
    std::vector<int> all = ks;
    for (int k : ks) all.push_back(k - 12);
    if (lowLimitsOk(all)) {
      for (int k : ks) keys.push_back({k - 12, true, third >= 0 && pcOf(k) == third && k - 12 < 60, true});
      what = "dealt, the trombones 8vb";
    }
  }
  // Too many keys for the seats: the inner ones go, an octave's duplicate first, then the one
  // closest to the key above it.
  while (int(keys.size()) > n_) {
    int drop = -1;
    for (int j = int(keys.size()) - 2; j >= 1 && drop < 0; --j)
      for (int i = 0; i < int(keys.size()); ++i)
        if (i != j && pcOf(keys[i].k) == pcOf(keys[j].k)) { drop = j; break; }
    if (drop < 0) {
      int gap = 1000;
      for (int j = 1; j + 1 < int(keys.size()); ++j)
        if (keys[j - 1].k - keys[j].k < gap) { gap = keys[j - 1].k - keys[j].k; drop = j; }
    }
    keys.erase(keys.begin() + drop);
  }
  // A lone key: a unison of those at ease on it; in a line, the trombones (or horns) an octave
  // under a lead at G4 or above (research 5.1).
  std::vector<bool> uni(n_, true);
  if (keys.size() == 1) {
    const int k = lead;
    bool any = false;
    for (int s = 0; s < n_; ++s) {
      const BrassSeatDef& d = s_[s];
      bool on;
      if (mode_ == kBrassOrchestral)
        on = d.kind == kBrTrumpet ? k >= 62 : d.kind == kBrHorn ? (k >= 48 && k <= 74) : d.kind == kBrTrombone ? k <= 62 : k <= 50;
      else
        on = d.kind == kBrTrumpet ? k >= 60 : std::strcmp(d.label, "tb4") == 0 ? k <= 62 : k <= 70;
      uni[s] = on && k >= d.lo && k <= d.hi;
      any |= uni[s];
    }
    if (!any) std::fill(uni.begin(), uni.end(), true);
    if (kOctave && h.line && h.octaves && lead >= 67 && lead - 12 >= 52) {
      keys.push_back({lead - 12, true, false, true});
      for (int s = 0; s < n_; ++s)  // (the low brass all on the octave)
        if (s_[s].kind == kBrTrombone || (s_[s].kind == kBrHorn && lead > 72)) uni[s] = false;
      what = "unison, the low brass 8vb";
    }
  }
  std::vector<int> o;
  double c = deal(keys, prev, keys.size() == 1 || (keys.size() == 2 && keys[1].octave) ? &uni : nullptr, &o);
  if (c >= 1e17 && keys.size() == 2 && keys[1].octave) {  // (no one for the octave: the unison alone)
    keys.pop_back();
    what = "unison";
    c = deal(keys, prev, &uni, &o);
  }
  if (c >= 1e17) c = deal(keys, prev, nullptr, &o);
  // (more keys in a register than seats reach there: inner keys go, the top and the bottom stay)
  while (c >= 1e17 && keys.size() > 2) {
    int drop = 1, gap = 1000;
    for (int j = 1; j + 1 < int(keys.size()); ++j)
      if (keys[j - 1].k - keys[j].k < gap) { gap = keys[j - 1].k - keys[j].k; drop = j; }
    keys.erase(keys.begin() + drop);
    c = deal(keys, prev, nullptr, &o);
  }
  if (c >= 1e17) {
    // (nobody reaches a key: each to the nearest seat that does, by octaves)
    for (Key& k : keys) {
      int best = k.k, bd = 1000;
      for (int s = 0; s < n_; ++s)
        for (int o2 = -24; o2 <= 24; o2 += 12)
          if (k.k + o2 >= s_[s].lo && k.k + o2 <= s_[s].hi && std::abs(o2) < bd) { bd = std::abs(o2); best = k.k + o2; }
      k.k = best;
    }
    std::sort(keys.begin(), keys.end(), [](const Key& a, const Key& b) { return a.k > b.k; });
    deal(keys, prev, nullptr, &o);
  }
  if (how) *how = what;
  return compact(o);
}

// Within each instrument's row the notes from its first seat down, the resting seats last (two
// seats on one note are the same to the listener; the row reads as an arranger writes it).
std::vector<int> BrassVoicer::compact(std::vector<int> o) const {
  for (int a = 0; a < n_;) {
    int b = a;
    while (b < n_ && s_[b].kind == s_[a].kind) ++b;
    std::vector<int> v;
    for (int i = a; i < b; ++i)
      if (o[i] >= 0) v.push_back(o[i]);
    std::sort(v.begin(), v.end(), std::greater<int>());
    // (not if a note would leave its seat's range: the bass trombone's low notes stay its own)
    bool fits = true;
    for (int i = a; i < b && i - a < int(v.size()); ++i) fits &= v[i - a] >= s_[i].lo && v[i - a] <= s_[i].hi;
    if (fits)
      for (int i = a; i < b; ++i) o[i] = i - a < int(v.size()) ? v[i - a] : -1;
    a = b;
  }
  return o;
}

const BrassHint* BrassPlan::find(int key, double on) const {
  const std::pair<int, long long> q{key, std::llround(on * 1e6)};
  auto it = std::lower_bound(hints.begin(), hints.end(), q, [](const auto& a, const std::pair<int, long long>& b) { return a.first < b; });
  // (within a millisecond: the times are the same events')
  for (auto j = it; j != hints.end() && j->first.first == key && j->first.second <= q.second + 1000; ++j)
    if (std::llabs(j->first.second - q.second) <= 1000) return &j->second;
  if (it != hints.begin()) {
    auto j = std::prev(it);
    if (j->first.first == key && std::llabs(j->first.second - q.second) <= 1000) return &j->second;
  }
  return nullptr;
}

// ---- The plan: the part and the song read once (research 4.3) ----

BrassPlan planBrassSection(const std::vector<BrassNoteIn>& notes, int div, int style) {
  BrassPlan P;
  std::vector<const BrassNoteIn*> own, others;
  for (const BrassNoteIn& n : notes) (n.self ? own : others).push_back(&n);
  if (own.empty()) return P;
  std::sort(own.begin(), own.end(), [](const BrassNoteIn* a, const BrassNoteIn* b) { return a->on < b->on; });
  // The onsets: notes within 30 ms of the first.
  std::vector<std::vector<const BrassNoteIn*>> groups;
  for (const BrassNoteIn* n : own) {
    if (groups.empty() || n->on - groups.back()[0]->on > 0.030) groups.emplace_back();
    groups.back().push_back(n);
  }
  std::vector<double> durs, lows;
  for (const BrassNoteIn* n : own) { durs.push_back(n->off - n->on); lows.push_back(n->key); }
  std::sort(durs.begin(), durs.end());
  std::sort(lows.begin(), lows.end());
  const double med = durs[durs.size() / 2], low2 = lows[size_t(0.02 * (lows.size() - 1))];
  int g1 = 0, g3 = 0, g45 = 0, oct = 0, wide = 0, multi = 0, sparse = 0;
  for (auto& g : groups) {
    std::vector<int> k;
    for (auto* n : g) k.push_back(n->key);
    std::sort(k.begin(), k.end());
    k.erase(std::unique(k.begin(), k.end()), k.end());
    g1 += k.size() == 1;
    g3 += k.size() >= 3;
    g45 += k.size() >= 4 && k.size() <= 5;
    if (k.size() == 2 && (k[1] - k[0]) % 12 == 0) ++oct;
    if (k.size() >= 2) { ++multi; wide += k.back() - k.front() > 12; }
    sparse += k.size() == 1 || (k.size() == 2 && (k[1] - k[0]) % 12 != 0);
  }
  const double G = double(groups.size());
  // The band around it.
  bool sax = false, piano = false, abass = false, ebass = false, strings = false, timpani = false, drums = false, guitar = false;
  for (const BrassNoteIn* n : others) {
    const int p = n->prog;
    if (n->ch == 9) { drums = true; continue; }
    sax |= p >= 64 && p <= 67;
    piano |= p <= 7;
    abass |= p == 32;
    ebass |= p >= 33 && p <= 39;
    strings |= (p >= 40 && p <= 45) || (p >= 48 && p <= 51);
    timpani |= p == 47;
    guitar |= p >= 24 && p <= 31;
  }
  // Swing: the off-beat eighths on the triplet's third, not half-way.
  int at50 = 0, at67 = 0;
  for (const BrassNoteIn& n : notes) {
    if (div <= 0) break;
    const double ph = double(n.tick % div) / div;
    at50 += std::fabs(ph - 0.5) < 0.04;
    at67 += std::fabs(ph - 0.667) < 0.04;
  }
  const bool swing = at67 > 12 && at67 > 1.5 * at50;
  // Which section (research 4.3, its table).
  double orch = 0, big = 0, pop = 1.5;
  if (style == Direction::kBaroque || style == Direction::kClassical || style == Direction::kRomantic) orch += 3;
  if (med > 0.4) orch += 1;
  if (multi > 0 && wide > 0.3 * multi) orch += 1;
  if (low2 < 45) orch += 1;
  if (strings || timpani) orch += 1;
  if (drums) orch -= 1.5;
  if (ebass || guitar) orch -= 1;
  if (g45 > 0.4 * G) big += 3;
  else if (g3 > 0.4 * G) big += 1;
  if (sax) big += 1;
  if (piano && abass) big += 1;
  if (swing) big += 1.5;
  if (ebass || guitar) big -= 0.5;
  if (drums) pop += 0.5;
  if (ebass || guitar) pop += 0.5;
  if (med < 0.25) pop += 1;
  P.mode = orch > pop && orch >= big ? kBrassOrchestral : big > pop ? kBrassBigBand : kBrassPop;
  if (const char* m = std::getenv("PJ_BRASS_MODE")) {
    const std::string s = m;
    P.mode = s == "bigband" || s == "1" ? kBrassBigBand : s == "orchestral" || s == "2" ? kBrassOrchestral : kBrassPop;
  }
  // Its own octaves written: then none are added.
  P.octaves = oct < 0.15 * G;
  // The harmony under each onset: the other channels' notes sounding over it (weighted by how
  // long, the lowest note as the bass), as a chord if one is clear.
  std::vector<const BrassNoteIn*> oth;
  for (const BrassNoteIn* n : others)
    if (n->ch != 9) oth.push_back(n);
  std::sort(oth.begin(), oth.end(), [](const BrassNoteIn* a, const BrassNoteIn* b) { return a->on < b->on; });
  double maxLen = 0;
  for (const BrassNoteIn* n : oth) maxLen = std::max(maxLen, n->off - n->on);
  struct G2 { int root = -1, q = -1; bool known = false; };
  std::vector<G2> harm(groups.size());
  double known = 0, fit = 0, sparseLen = 0;
  for (size_t gi = 0; gi < groups.size(); ++gi) {
    const BrassNoteIn* lead = groups[gi][0];
    double len = 0;
    for (auto* n : groups[gi]) {
      if (n->key > lead->key) lead = n;
      len = std::max(len, n->off - n->on);
    }
    const double t0 = lead->on, t1 = lead->on + std::max(0.12, std::min(len, 0.6));
    // (what sounded over the onset; failing a clear chord there, also at half weight what sounded
    // in the half second before it: a comping piano's chord still holds after its keys are up)
    auto first = std::lower_bound(oth.begin(), oth.end(), t0 - 0.5 - maxLen, [](const BrassNoteIn* a, double t) { return a->on < t; });
    G2& h = harm[gi];
    for (int pass = 0; pass < 2 && !h.known; ++pass) {
      double w[12] = {};
      int bass = -1, bassKey = 999;
      for (auto it = first; it != oth.end() && (*it)->on < t1; ++it) {
        const BrassNoteIn* n = *it;
        double ov = std::max(0.0, std::min(t1, n->off) - std::max(t0 - 0.02, n->on));
        if (pass) ov += 0.5 * std::max(0.0, std::min(t0 - 0.02, n->off) - std::max(t0 - 0.5, n->on));
        if (ov <= 0) continue;
        w[pcOf(n->key)] += ov * n->vel / 100.0;
        if (n->key < bassKey) { bassKey = n->key; bass = pcOf(n->key); }
      }
      if (bass >= 0) w[bass] *= 1.5;
      h.known = recognise(w, bass, &h.root, &h.q);
    }
    std::vector<int> k;
    for (auto* n : groups[gi]) k.push_back(n->key);
    std::sort(k.begin(), k.end());
    k.erase(std::unique(k.begin(), k.end()), k.end());
    const bool sp = k.size() == 1 || (k.size() == 2 && (k[1] - k[0]) % 12 != 0);
    if (!sp) continue;
    sparseLen += len;
    if (!h.known) continue;
    known += len;
    if (!voicingTones(h.q, pcOf(lead->key - h.root), 4).empty()) fit += len;
  }
  // Voiced only where the part supports it: mostly sparse, the song's chords clear under most of
  // it, and the line on their notes (a blues line's flat third over a major chord is not).
  static const double kHarm = knob("PJ_BRASS_HARMONISE", 1);
  P.harmonise = kHarm > 0 && P.mode != kBrassOrchestral && G >= 8 && sparse >= 0.7 * G && sparseLen > 0 &&
                known >= 0.6 * sparseLen && fit >= 0.75 * known;
  if (kHarm >= 2 && P.mode != kBrassOrchestral) P.harmonise = true;
  const bool lines = g1 >= 0.5 * G;
  for (size_t gi = 0; gi < groups.size(); ++gi)
    for (const BrassNoteIn* n : groups[gi]) {
      BrassHint h;
      h.root = harm[gi].known ? harm[gi].root : -1;
      h.quality = harm[gi].known ? harm[gi].q : -1;
      h.len = n->off - n->on;
      h.line = lines && groups[gi].size() == 1;
      h.octaves = P.octaves;
      h.harmonise = P.harmonise && harm[gi].known && (P.mode == kBrassBigBand ? h.len >= 0.08 : h.len >= 0.3);
      P.hints.push_back({{n->key, std::llround(n->on * 1e6)}, h});
    }
  std::sort(P.hints.begin(), P.hints.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
  char b[512];
  std::snprintf(b, sizeof b,
                "the brass section: %s (%d onsets: %.0f %% single, %.0f %% of 3+ keys; median note %.2f s; %s%s%s%s); %s%s",
                brassModeName(P.mode), int(G), 100 * g1 / G, 100 * g3 / G, med, swing ? "swung, " : "",
                sax ? "saxes, " : "", strings || timpani ? "strings, " : "", drums ? "drums" : "no drums",
                P.harmonise ? "sparse onsets voiced under the song's chords" : "the keys as written",
                P.octaves ? "" : " (its own octaves: none added)");
  P.summary = b;
  return P;
}

// ---- -brassvoice ----

int brassVoiceDemo(const std::string& spec) {
  int mode = kBrassPop;
  std::vector<int> prev;
  std::stringstream all(spec);
  std::string step;
  while (std::getline(all, step, ';')) {
    std::istringstream is(step);
    std::string w;
    BrassHint h;
    std::vector<int> keys;
    bool any = false;
    while (is >> w) {
      if (w == "mode") {
        is >> w;
        const int m = w == "bigband" ? kBrassBigBand : w == "orchestral" ? kBrassOrchestral : kBrassPop;
        if (m != mode) prev.clear();
        mode = m;
      } else if (w == "chord") {
        is >> w;
        if (!parseBrassChord(w, &h.root, &h.quality)) { std::fprintf(stderr, "-brassvoice: not a chord: %s\n", w.c_str()); return 1; }
        h.harmonise = true;
      } else if (w == "line") h.line = true;
      else if (w == "noline") h.line = false;
      else if (w == "len") is >> h.len;
      else { keys.push_back(std::atoi(w.c_str())); any = true; }
    }
    if (!any) continue;
    BrassVoicer v(mode);
    if (prev.size() != size_t(v.seats())) prev.assign(v.seats(), -1);
    std::string how;
    const std::vector<int> o = v.voice(keys, h, prev, &how);
    int motion = 0;
    for (int s = 0; s < v.seats(); ++s)
      if (o[s] >= 0 && prev[s] >= 0) motion += std::abs(o[s] - prev[s]);
    std::printf("%s | %s | keys", brassModeName(mode), brassChordName(h.root, h.quality).c_str());
    for (int k : keys) std::printf(" %d", k);
    std::printf(" -> %s:", how.c_str());
    for (int s = 0; s < v.seats(); ++s) std::printf(" %s %s%s", v.seat(s).label, noteStr(o[s]).c_str(), s + 1 < v.seats() ? "," : "");
    std::printf(" | motion %d | notes", motion);
    for (int s = 0; s < v.seats(); ++s) std::printf(" %d", o[s]);
    std::printf("\n");
    prev = o;
  }
  return 0;
}

// ---- The instrument ----

namespace {
const NoteInfo kBrassNotes[128] = {
#define N {"", 1}
#define N8 N, N, N, N, N, N, N, N
    N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8
#undef N8
#undef N
};
const Design kBrassDesign = {nullptr, 0, nullptr, 0, kBrassNotes, 0, 0, false, nullptr, 0, {}, {}};
}  // namespace

// (sounding E1 .. C7: what no seat reaches is played by octaves where one does)
extern const Instrument kBrassSection = {"brass-section", 0, 28, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0,
                                         Kind::kString, &kBrassDesign, "brass-section"};

bool brassSectionInstrument(const Instrument& inst) { return std::strcmp(inst.family, "brass-section") == 0; }

BrassSectionModel::BrassSectionModel(const Instrument& inst) : Tube(inst) {
  static const Calibration cal;
  ens_.reset(new Ensemble(inst, cal, 1, std::getenv("PJ_BRASS_SHOW") != nullptr, nullptr));
}
BrassSectionModel::~BrassSectionModel() = default;

void BrassSectionPlayer::noteOn(int midi, int velocity) {
  m_.ensemble().noteOn(midi, velocity);
  blowing_ = true;
}
void BrassSectionPlayer::noteOff(int midi) { m_.ensemble().noteOff(midi); }
void BrassSectionPlayer::setHold(bool h) { m_.ensemble().setHold(h); }

void BrassSectionPlayer::render(float* out, int n) {
  Ensemble& e = m_.ensemble();
  e.all(&Performer::setDynamics, dyn_);
  e.all(&Performer::setBreath, breathCtl_);
  e.all(&Performer::setLipRoll, roll_);
  e.all(&Performer::setAir, air_);
  e.all(&Performer::setFlutter, flutter_);
  e.render(out, n);
  // (the Voice applies the section's level trim: the section's ensemble does too)
  const float g = float(1 / levelTrim(inst_));
  for (int i = 0; i < n; ++i) out[i] *= g;
}

// ---- The section's playing (Ensemble) ----

struct BrassState {
  const Instrument* inst = nullptr;
  const RoomSpec* room = nullptr;
  int mode = kBrassPop;
  std::unique_ptr<BrassVoicer> voicer;
  struct Seat {
    int slot = 0;
    int cur = -1, want = -1;  // the note it plays; the note it is to play (-1: none)
    double at = 0;            // ... from when
    int vel = 64;
    double habit = 0;         // its own lateness (s)
    double noteAt = -1e9;     // when its note began
    double bendDelay = 0, bendSpeed = 1, roll = 0;
    int shift = 0;            // semitones it has glided away from its note (a fall, a doit)
    double vg = 1, vgTo = 1;  // its gain (the lead on top; velocity's, -play), now and its goal
    double vib = 0;           // its vibrato now (setVibrato)
    double dynAt = 0;         // the expression at its attack
    bool lead = false, added = false;
  };
  std::vector<Seat> seats;
  double pressT[128] = {};
  bool ghost[128] = {};
  int ghostOf[128];           // (the key that took over its line)
  double groupT = -1e9;
  int groupN = 0, lastGroupN = 1, prevGroupN = 1;
  BrassHint hint;
  bool hinted = false;
  std::vector<int> committed;
  double commitT = -1;
  double cueVib = -1;         // the director's vibrato depth (x the player's; -1: none given)
  std::vector<std::pair<double, double>> bends;  // (s, bend) as it came
  double bendX = 0;
  double gain = 1, keysW = 1, g = 0;
  double dyn = 0.5, expr = 1;
  double lead = 0;  // (-play: the keys come this early, noteLead)
  bool verbose = false, built = false;
  unsigned rng = 2062;
  std::string lastShow, pendingShow;
  std::map<std::string, std::unique_ptr<Calibration>> cals;
  double rnd() { rng = rng * 1664525u + 1013904223u; return (rng >> 8) / 16777216.0; }
  double gauss() { return (rnd() + rnd() + rnd() + rnd() - 2) * 1.73; }
};

void Ensemble::initBrass(const Instrument& inst, const RoomSpec* room) {
  lo_ = inst.lo;
  hi_ = inst.hi;
  brass_ = std::make_shared<BrassState>();
  BrassState& B = *brass_;
  B.inst = &inst;
  B.room = room;
  B.verbose = verbose_ || std::getenv("PJ_BRASS_SHOW");
  B.lead = noteLeadsApplied() ? noteLead(inst) : 0;
  std::fill(B.ghostOf, B.ghostOf + 128, -1);
  int mode = kBrassPop;
  if (const char* m = std::getenv("PJ_BRASS_MODE")) {
    const std::string s = m;
    mode = s == "bigband" || s == "1" ? kBrassBigBand : s == "orchestral" || s == "2" ? kBrassOrchestral : kBrassPop;
  }
  brassBuild(mode);
}

void Ensemble::setBrassMode(int mode) {
  if (!brass_ || (brass_->built && brass_->mode == mode)) return;
  brassBuild(mode);
}

// The section's players (one ensemble, kids_[0]), each its own (vary) on its own instrument.
void Ensemble::brassBuild(int mode) {
  BrassState& B = *brass_;
  B.mode = mode;
  B.voicer.reset(new BrassVoicer(mode));
  kids_.clear();
  std::vector<Member> members;
  const int n = B.voicer->seats();
  B.seats.assign(n, BrassState::Seat());
  B.committed.assign(n, -1);
  for (int i = 0; i < n; ++i) {
    const BrassSeatDef& d = B.voicer->seat(i);
    const Instrument* in = findInstrument(d.inst);
    auto& c = B.cals[d.inst];
    if (!c) c.reset(new Calibration(std::getenv("PJ_NOCAL") ? Calibration() : calibrate(*in, 440, nullptr)));
    Member m{in, c.get()};
    m.x = d.x;
    members.push_back(m);
    BrassState::Seat& s = B.seats[i];
    s.slot = i;
    // (its habit: up to 3 ms early or late, the lead on time; how it follows a bend: later, its own speed)
    s.habit = i == 0 ? 0 : 0.003 * (2 * B.rnd() - 1);
    s.bendDelay = i == 0 ? 0 : 0.003 + 0.017 * B.rnd();
    s.bendSpeed = 1 + 0.15 * (2 * B.rnd() - 1);
  }
  Section s;
  s.ens.reset(new Ensemble(members, false, B.room));
  s.ens->quiet_ = true;
  s.ens->mixFixed_ = true;  // (the section's own balance: brassRender)
  // (the held chord tuned by its partials, as the organ's keys: the overtones lock. At full
  // strength: the spectral pull is itself mild, a close major third about 5 c narrow, an open one
  // 9 c, a seventh left near tempered. PJ_BRASS_JI_SPECTRAL=0, PJ_BRASS_JI=0.5: the interval
  // table at half strength, as before)
  s.ens->spectral_ = knob("PJ_BRASS_JI_SPECTRAL", 1) != 0;
  s.ens->jiScale_ = knob("PJ_BRASS_JI", s.ens->spectral_ ? 1.0 : 0.5);
  s.lo = lo_;
  s.hi = hi_;
  s.gain = 1;
  s.stops = true;
  kids_.push_back(std::move(s));
  // A key at about one trumpet's level, +2 dB (research 6.4; measured on a unison key).
  B.gain = knob("PJ_BRASS_GAIN", 1.08) * levelTrim(*B.inst);  // (-play: a unison G4 2 dB over the trumpet at velocity 100; 0.72 while -play sounded the trumpet a tone low)
  unsigned seat = 0;
  for (Slot& f : kids_[0].ens->f_) {
    ++seat;
    f.v->player().vary(seat);
    static const bool kBuild = knob("PJ_BRASS_BUILD", 1) > 0;  // (0: every instrument the same)
    if (kBuild) f.v->model().vary(seat);
  }
  B.built = true;
  if (std::getenv("PJ_BRASS_DEBUG"))
    for (Slot& f : kids_[0].ens->f_)
      if (auto* hp = dynamic_cast<HornPlayer*>(&f.v->player())) say(stderr, "brass: seat cents %+.1f\n", hp->seatCents());
  if (B.verbose) {
    std::string who;
    for (int i = 0; i < n; ++i) who += std::string(i ? ", " : "") + B.voicer->seat(i).label;
    say(stderr, "brass: the %s section: %s\n", brassModeName(mode), who.c_str());
  }
}

void Ensemble::brassHint(const BrassHint& h) {
  if (!brass_) return;
  brass_->hint = h;
  brass_->hinted = true;
}

void Ensemble::brassCue(const Cue& c) {
  if (brass_) brass_->cueVib = c.v[kCueVibDepth];
}

void Ensemble::brassBend(double x) {
  BrassState& B = *brass_;
  if (x == B.bendX) return;
  B.bendX = x;
  B.bends.push_back({now_, x});
  if (B.bends.size() > 512) B.bends.erase(B.bends.begin(), B.bends.begin() + 256);
}

void Ensemble::brassExpression(double x) {
  if (brass_) brass_->expr = x;
}

void Ensemble::brassOn(int key, int velocity) {
  BrassState& B = *brass_;
  if (key < 0 || key > 127) return;
  // (a key struck again: its old note goes first)
  if (sheld_[key] > 0) brassOff(key);
  // An onset: keys within 35 ms of the first are one chord.
  if (now_ - B.groupT > 0.035) {
    B.prevGroupN = B.lastGroupN;
    B.lastGroupN = B.groupN > 0 ? B.groupN : B.lastGroupN;
    B.groupT = now_;
    B.groupN = 0;
  }
  ++B.groupN;
  sheld_[key] = std::clamp(velocity, 1, 127);
  B.pressT[key] = now_;
  B.ghost[key] = false;
  // A key while others are held from before, more than the lines lately played: the nearest of
  // them (a step or two away) is the line moving on (a legato's overlap), not another line.
  {
    int held = 0, near = -1, nd = 6;
    const int lines = std::max(B.lastGroupN, B.groupN);
    for (int k = 0; k < 128; ++k)
      if (sheld_[k] > 0 && !B.ghost[k]) {
        ++held;
        if (k != key && now_ - B.pressT[k] > 0.035 && std::abs(k - key) < nd) { nd = std::abs(k - key); near = k; }
      }
    if (held > lines && near >= 0) {
      B.ghost[near] = true;
      B.ghostOf[near] = key;
    }
  }
  brassAssign(true);
  B.hinted = false;
}

void Ensemble::brassOff(int key) {
  BrassState& B = *brass_;
  if (key < 0 || key > 127 || sheld_[key] == 0) return;
  sheld_[key] = 0;
  if (B.ghost[key]) { B.ghost[key] = false; return; }  // (its line moved on already)
  // The players on it stop; the others play on (no one moves to another key on a release).
  bool any = false;
  for (int k = 0; k < 128; ++k) any |= sheld_[k] > 0 && !B.ghost[k];
  if (!any) { brassAssign(false); return; }
  // (the voicing's added notes go with the lead: they stop if it was the lead's key)
  bool wasTop = true;
  for (int k = key + 1; k < 128; ++k) wasTop &= !(sheld_[k] > 0 && !B.ghost[k]);
  for (BrassState::Seat& s : B.seats) {
    if (s.want < 0) continue;
    const bool keep = s.added ? !wasTop : s.want != key;
    if (keep) continue;
    const bool stab = now_ - s.noteAt < 0.3;
    s.want = -1;
    s.added = false;
    s.at = now_ + B.lead + (stab ? 0.002 + 0.005 * std::fabs(B.gauss()) : 0.010 + 0.025 * B.rnd());
  }
}

// The held keys voiced and dealt to the seats (brasssect.h); onset: a key went down.
void Ensemble::brassAssign(bool /*onset*/) {
  BrassState& B = *brass_;
  const int n = int(B.seats.size());
  // What the seats were doing before this moment: the voice leading's starting point.
  if (now_ > B.commitT) {
    for (int i = 0; i < n; ++i) B.committed[i] = B.seats[i].want >= 0 ? B.seats[i].want : B.seats[i].cur;
    B.commitT = now_;
  }
  std::vector<int> keys;
  int topVel = 64, top = -1;
  for (int k = 0; k < 128; ++k)
    if (sheld_[k] > 0 && !B.ghost[k]) {
      keys.push_back(k);
      if (k > top) { top = k; topVel = sheld_[k]; }
    }
  BrassHint h;
  if (B.hinted) h = B.hint;
  else h.line = B.groupN <= 1 && B.lastGroupN <= 1;  // (live, -seq: a run of single keys is a line)
  std::string how;
  const std::vector<int> v = keys.empty() ? std::vector<int>(n, -1) : B.voicer->voice(keys, h, B.committed, &how);
  // Its weight in the mix: the keys, and a third of a key for each note the voicing added.
  int added = 0;
  {
    std::vector<int> d;
    for (int x : v)
      if (x >= 0 && std::find(d.begin(), d.end(), x) == d.end()) d.push_back(x);
    for (int x : d) added += std::find(keys.begin(), keys.end(), x) == keys.end();
    const int given = int(d.size()) - added;
    B.keysW = std::max(1.0, std::pow(std::max(1, given), 0.7) + 0.3 * added);  // (four keys ~ +8 dB over one, as the GM section)
  }
  const bool longish = h.len > 0 ? h.len >= 0.5 : topVel < 64;
  const bool unison = [&] {
    int a = -1;
    for (int x : v) {
      if (x < 0) continue;
      if (a < 0) a = x;
      else if (pcOf(x) != pcOf(a)) return false;
    }
    return true;
  }();
  (void)unison;
  // Each section's moment for this onset (trumpets, horns, trombones, tuba).
  double moment[4];
  for (double& m : moment) m = 0.004 * (2 * B.rnd() - 1);
  bool leadSet = false;
  for (int i = 0; i < n; ++i) {
    BrassState::Seat& s = B.seats[i];
    const int w = v[i];
    const bool isLead = w >= 0 && w == top && !leadSet;
    if (isLead) leadSet = true;
    const bool isAdded = w >= 0 && std::find(keys.begin(), keys.end(), w) == keys.end();
    if (w == s.want) {
      s.lead = isLead;
      s.added = isAdded;
      continue;
    }
    const bool restarting = w >= 0 && s.cur == w && B.pressT[w] == now_;  // (its key struck again)
    s.lead = isLead;
    s.added = isAdded;
    if (w < 0) {
      if (s.want < 0) continue;
      const bool stab = now_ - s.noteAt < 0.3;
      s.want = -1;
      s.at = now_ + B.lead + (stab ? 0.002 + 0.005 * std::fabs(B.gauss()) : 0.010 + 0.025 * B.rnd());
      continue;
    }
    const BrassKind kind = B.voicer->seat(i).kind;
    const double mom = moment[kind];
    const int keyVel = sheld_[w] > 0 ? sheld_[w] : topVel;
    // Its velocity: the key's, its own a little (+-4); the voicing's added notes softer, under the lead.
    s.vel = std::clamp(keyVel + int(std::lround(4 * (2 * B.rnd() - 1))) - (isAdded ? 8 : 0), 1, 127);
    const bool fresh = s.cur < 0 || s.want < 0 || restarting;
    // Played from a file the keys come B.lead early: each player starts its instrument's speaking
    // time before the beat (the trumpet's 30 ms, the horn's 80), so the section lands together;
    // live, as they come.
    const double ahead = fresh ? B.lead - speakTime(kind, w) + speakTime(kBrTrumpet, w) : B.lead;
    double d;
    if (!fresh) d = isLead ? 0 : 0.003 * B.rnd();  // (a change of note on the same breath)
    else if (isLead) d = longish ? std::max(0.0, 0.004 + 0.004 * B.gauss()) : 0;
    else if (longish) d = std::max(0.0005, 0.010 + 0.015 * B.gauss() + s.habit + mom);
    else d = std::max(0.002, 0.004 + 0.006 * B.gauss() + s.habit + mom);  // (the others after the lead)
    d = std::max(0.0, d + ahead);
    if (restarting) {  // (the old note let go first: the key struck again is a new attack)
      kids_[0].ens->drive(s.slot, s.cur + s.shift, 0, false);
      s.cur = -1;
      s.shift = 0;
    }
    s.want = w;
    s.at = now_ + d;
  }
  if (B.verbose && !keys.empty()) {
    std::string line = "brass: ";
    if (B.hint.root >= 0 && B.hinted) line += brassChordName(B.hint.root, B.hint.quality) + " ";
    line += "keys";
    for (int k : keys) line += " " + noteStr(k);
    line += " -> " + how + ":";
    for (int i = 0; i < n; ++i)
      if (v[i] >= 0) line += std::string(" ") + B.voicer->seat(i).label + " " + noteStr(v[i]);
    B.pendingShow = line;  // (printed as it is played: a chord's keys arrive one by one)
  }
}

void Ensemble::brassRender(float* left, float* right, int n) {
  BrassState& B = *brass_;
  Ensemble& kid = *kids_[0].ens;
  hitL_.resize(n);
  hitR_.resize(n);
  static const double kLeadDb = knob("PJ_BRASS_LEAD_DB", 1.5);
  static const double kVib = knob("PJ_BRASS_VIB", 0.6);  // (the lead's depth without a director's cue)
  if (!B.pendingShow.empty()) {
    if (B.pendingShow != B.lastShow) say(stderr, "%s\n", B.pendingShow.c_str());
    B.lastShow = B.pendingShow;
    B.pendingShow.clear();
  }
  for (int done = 0; done < n;) {
    const int m = std::min(n - done, 128);
    const double dt = m / 48000.0;
    int playing = 0;
    for (BrassState::Seat& s : B.seats) {
      if (s.want != s.cur && now_ >= s.at) {
        // To its new note before it leaves the old one: a slur on the same breath.
        if (s.want >= 0) {
          const bool fresh = s.cur < 0;
          kid.drive(s.slot, s.want, s.vel, true);
          s.noteAt = now_;
          if (fresh) s.dynAt = B.expr;
          Slot& f = kid.f_[s.slot];
          double g = s.lead ? std::pow(10.0, kLeadDb / 20) : 1.0;
          if (velGain_) g *= std::pow(10.0, velocityGainDb(*B.inst, s.vel) / 20);
          s.vgTo = g;
          if (fresh || !f.v->player().busy()) s.vg = g;
        }
        if (s.cur >= 0) kid.drive(s.slot, s.cur + s.shift, 0, false);
        s.cur = s.want;
        s.shift = 0;
      }
      Slot& f = kid.f_[s.slot];
      Performer& p = f.v->player();
      playing += s.cur >= 0 || p.busy();
      s.vg += (1 - std::exp(-dt / 0.06)) * (s.vgTo - s.vg);
      static const int kOnly = int(knob("PJ_BRASS_ONLY", 0));  // (tests: one seat heard, 1 .. n; the others play on)
      f.vg = kOnly > 0 && s.slot + 1 != kOnly ? 0.0 : s.vg;
      // Vibrato (research 6.3): none under 0.6 s; then the lead a little, the other trumpets less
      // and later; the horns, trombones and tuba none; none in a unison.
      const BrassKind kind = B.voicer->seat(s.slot).kind;
      double want = 0;
      if (s.cur >= 0) {
        const double age = now_ - s.noteAt;
        const double base = 0.05 * (B.cueVib >= 0 ? B.cueVib : kVib);
        bool uni = true;
        for (const BrassState::Seat& o : B.seats)
          if (o.cur >= 0 && pcOf(o.cur) != pcOf(s.cur)) uni = false;
        if (s.lead && kind != kBrTuba) want = 0.5 * base * std::clamp((age - 0.6) / 0.3, 0.0, 1.0);
        else if (kind == kBrTrumpet && !uni) want = 0.2 * base * std::clamp((age - 0.9) / 0.4, 0.0, 1.0);
        if (kind == kBrTrombone || kind == kBrTuba || (kind == kBrHorn && !s.lead)) want = 0;
      }
      s.vib += (1 - std::exp(-dt / 0.05)) * (want - s.vib);
      p.setVibrato(s.vib);
      // The bend (a fall, a doit): each player follows it a moment later, at its own speed.
      double bx = 0;
      for (auto it = B.bends.rbegin(); it != B.bends.rend(); ++it)
        if (it->first <= now_ - s.bendDelay) { bx = it->second; break; }
      s.roll += (1 - std::exp(-dt / (0.025 * s.bendSpeed))) * (bx - s.roll);
      // Past a semitone the player glides down (or up) the valves or the slide a semitone at a
      // time, the lips bending the rest (their roll reaches ~30 c): a fall through the notes.
      static const double kRange = knob("PJ_BRASS_BEND_RANGE", 2);  // (General MIDI's bend: +-2 semitones)
      const double semis = s.roll * kRange;
      if (s.cur >= 0 && std::fabs(semis - s.shift) > 0.65) {
        const int to = int(std::lround(semis));
        const BrassSeatDef& d = B.voicer->seat(s.slot);
        if (to != s.shift && s.cur + to >= d.lo && s.cur + to <= d.hi) {
          kid.drive(s.slot, s.cur + to, 30, true);  // (a soft key over the held one: a glide)
          kid.drive(s.slot, s.cur + s.shift, 0, false);
          s.shift = to;
        }
      }
      p.setLipRoll(std::clamp((semis - s.shift) / 0.3, -1.0, 1.0));  // (the rest, by the lips: 30 c a unit)
      // A swell inside a held note (CC 11) moves the breath too: darker and brighter with it.
      p.setDynamics(std::clamp(B.dyn + 0.6 * (B.expr - s.dynAt), 0.0, 1.0));
    }
    // The section's level: a key about one trumpet's (+2 dB: B.gain), a voiced chord adding up.
    const double target = B.gain * std::sqrt(B.keysW / std::max(1, playing));
    B.g += (1 - std::exp(-dt / 0.03)) * (target - B.g);
    if (B.g <= 0) B.g = target;
    kid.render(hitL_.data(), hitR_.data(), m);
    const float g = float(B.g);
    for (int i = 0; i < m; ++i) {
      left[done + i] = g * hitL_[i];
      right[done + i] = g * hitR_[i];
    }
    now_ += dt;
    done += m;
  }
}

void Ensemble::brassSetDynamics(double d) {
  if (brass_) brass_->dyn = d;
}

}  // namespace pajupilli
