// The koto's ji, set for the piece (kotoplan.h; research/koto_expressive.md 2, 6.1).
#include "kotoplan.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>

#include "knob.h"

namespace pajupilli {
namespace {

// What a note costs on a set of open strings: open 0; pressed a semitone 1, a whole tone 2.5 (the
// harder press, the slower hand); out of reach 20 (a ji moved within the phrase).
constexpr double kOpenCost = 0, kPress1 = 1, kPress2 = 2.5, kAway = 20;
// A string kept as it was is worth this much of a note's cost (the player moves no ji it need not).
constexpr double kKeep = 0.6;
// A breath: the next onset this long after the last (s), or every key up this long before it.
constexpr double kGap = 0.25, kFree = 0.05;
// A move only for so much (cost over the window) per string moved; never sooner than kSettle after
// the last move, unless a note would be out of reach.
constexpr double kMoveCost = 3.0, kSettle = 6.0;
// The window a tuning is chosen for: the breaths from here this long (s). (Not only while the pitches
// fit on 13 strings: a line over three octaves of a major key, tuned phrase by phrase on open
// strings alone, moved five ji at every turn of the line; over the whole window the strings take a
// pentatonic through its range, the scale's other two notes pressed, as players tune for such a part.)
constexpr double kWindow = 30;

const int kHira[13] = {50, 55, 57, 58, 62, 63, 67, 69, 70, 74, 75, 79, 81};

double cost(int p, const std::vector<int>& set) {
  double c = kAway;
  for (int o : set) {
    if (o == p) return kOpenCost;
    if (o == p - 1) c = std::min(c, kPress1);
    else if (o == p - 2) c = std::min(c, kPress2);
  }
  return c;
}

using Hist = std::map<int, double>;  // a folded pitch's weight in a window

double histCost(const Hist& h, const std::vector<int>& set) {
  double c = 0;
  for (const auto& [p, w] : h) c += w * cost(p, set);
  return c;
}

// Thirteen open pitches for the window's notes: greedily the string that saves the most, a string
// the koto already has a little preferred; when every note is open, the rest kept where they were
// (or, fresh, the window's pitch classes in other octaves, then the hirajoshi's).
std::vector<int> solve(const Hist& h, const std::vector<int>* prev) {
  std::vector<int> set;
  std::vector<int> left = prev ? *prev : std::vector<int>();
  while (set.size() < 13) {
    int best = -1;
    double gain = 1e-9;
    const double before = histCost(h, set);
    for (int c = kKotoOpenLo; c <= kKotoOpenHi; ++c) {
      if (std::find(set.begin(), set.end(), c) != set.end()) continue;
      std::vector<int> s2 = set;
      s2.push_back(c);
      double g = before - histCost(h, s2);
      if (g <= 1e-9) continue;
      if (std::find(left.begin(), left.end(), c) != left.end()) g += kKeep;
      if (g > gain) { gain = g; best = c; }
    }
    if (best < 0) break;
    set.push_back(best);
    auto it = std::find(left.begin(), left.end(), best);
    if (it != left.end()) left.erase(it);
  }
  // (every note open: the rest of the strings as they were)
  for (int o : left) {
    if (set.size() >= 13) break;
    set.push_back(o);
  }
  // (fresh: the window's pitch classes an octave away, nearest its middle; then the hirajoshi)
  if (set.size() < 13 && !h.empty()) {
    double mid = 0, wsum = 0;
    for (const auto& [p, w] : h) mid += p * w, wsum += w;
    mid /= wsum;
    std::vector<std::pair<double, int>> cand;
    for (const auto& [p, w] : h)
      for (int o : {-24, -12, 12, 24}) {
        const int q = p + o;
        if (q < kKotoOpenLo || q > kKotoOpenHi || h.count(q)) continue;
        cand.push_back({std::fabs(q - mid) - 0.01 * w, q});
      }
    std::sort(cand.begin(), cand.end());
    for (const auto& [d, q] : cand) {
      if (set.size() >= 13) break;
      if (std::find(set.begin(), set.end(), q) == set.end()) set.push_back(q);
    }
  }
  for (int o : kHira) {
    if (set.size() >= 13) break;
    if (std::find(set.begin(), set.end(), o) == set.end()) set.push_back(o);
  }
  while (set.size() < 13) set.push_back(set.empty() ? 62 : set.back());  // (a unison pair, as strings 1 and 5)
  std::sort(set.begin(), set.end());
  return set;
}

int moved(const std::vector<int>& a, const std::vector<int>& b) {
  std::vector<int> x = a, y = b, both;
  std::sort(x.begin(), x.end());
  std::sort(y.begin(), y.end());
  std::set_intersection(x.begin(), x.end(), y.begin(), y.end(), std::back_inserter(both));
  return 13 - int(both.size());
}

std::string names(const std::vector<int>& s) {
  static const char* kN[12] = {"C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B"};
  std::string r;
  for (int p : s) r += std::string(r.empty() ? "" : " ") + kN[p % 12] + std::to_string(p / 12 - 1);
  return r;
}

// The fingering (kotoplan.h). press[i]: note i is pressed (the left hand busy with it).
std::vector<KotoStroke> finger(const std::vector<KotoNote>& notes, const std::vector<bool>& press) {
  constexpr double kChord = 0.03, kFast = 0.12, kRepeat = 0.3, kBroken = 0.25, kLeftFree = 0.15;
  constexpr int kBassTop = kKotoOpenLo + 9;  // (A3: the lowest strings, the left hand's reach)
  const size_t n = notes.size();
  std::vector<KotoStroke> st(n);
  for (size_t i = 0; i < n; ++i) st[i] = {notes[i].t, notes[i].key, kKotoThumb};
  // (the left hand free at t: no press within kLeftFree of it)
  auto leftFree = [&](double t) {
    for (size_t i = 0; i < n; ++i)
      if (press[i] && notes[i].t > t - kLeftFree && notes[i].t < t + kLeftFree) return false;
    return true;
  };
  // The groups: onsets within kChord.
  std::vector<std::pair<size_t, size_t>> g;
  for (size_t i = 0; i < n;) {
    size_t j = i + 1;
    while (j < n && notes[j].t - notes[i].t < kChord) ++j;
    g.push_back({i, j});
    i = j;
  }
  int run = 0, runDir = 0, lastFinger = -1;
  for (size_t gi = 0; gi < g.size(); ++gi) {
    const auto [a, b] = g[gi];
    if (b - a >= 2) {
      std::vector<size_t> idx;
      for (size_t i = a; i < b; ++i) idx.push_back(i);
      std::sort(idx.begin(), idx.end(), [&](size_t x, size_t y) { return notes[x].key > notes[y].key; });
      const size_t lo = idx.back();
      if (idx.size() >= 4 && kotoFold(notes[lo].key) <= kBassTop &&
          notes[idx[idx.size() - 2]].key - notes[lo].key >= 7 && leftFree(notes[lo].t)) {
        st[lo].finger = kKotoLeft;
        idx.pop_back();
      }
      for (size_t r = 0; r < idx.size(); ++r)
        st[idx[r]].finger = r == 0 ? kKotoThumb : r + 1 == idx.size() ? kKotoMiddle : kKotoIndex;
      run = 0;
      lastFinger = -1;
      continue;
    }
    // A single note.
    const KotoNote& me = notes[a];
    const KotoNote* pv = gi > 0 && g[gi - 1].second - g[gi - 1].first == 1 ? &notes[g[gi - 1].first] : nullptr;
    const KotoNote* nx = gi + 1 < g.size() && g[gi + 1].second - g[gi + 1].first == 1 ? &notes[g[gi + 1].first] : nullptr;
    const double ioiP = pv ? me.t - pv->t : 1e9, ioiN = nx ? nx->t - me.t : 1e9;
    // (a low note under the right hand's line: the left hand's, if free)
    if (kotoFold(me.key) <= kBassTop && leftFree(me.t)) {
      bool under = false;
      for (size_t i = 0; i < n && !under; ++i)
        under = i != a && std::fabs(notes[i].t - me.t) < kFast && notes[i].key - me.key >= 7;
      if (under) {
        st[a].finger = kKotoLeft;
        continue;
      }
    }
    int f = kKotoThumb;
    const int dir = nx && ioiN < kBroken ? (nx->key > me.key) - (nx->key < me.key)
                                         : pv && ioiP < kBroken ? (me.key > pv->key) - (me.key < pv->key) : 0;
    if (pv && pv->key == me.key && ioiP < kRepeat) {
      f = lastFinger == kKotoThumb ? kKotoIndex : kKotoThumb;  // (two fingers in turn)
      run = 0;
    } else if ((ioiP < kFast || ioiN < kFast) && dir != 0) {
      f = dir > 0 ? kKotoIndex : kKotoThumb;  // (one finger through the strings)
      run = 0;
    } else if (dir != 0 && ((pv && ioiP < kBroken && std::abs(me.key - pv->key) >= 3) ||
                            (nx && ioiN < kBroken && std::abs(nx->key - me.key) >= 3))) {
      run = dir == runDir ? run + 1 : 0;
      runDir = dir;
      static const int kUp[3] = {kKotoMiddle, kKotoIndex, kKotoThumb}, kDown[3] = {kKotoThumb, kKotoIndex, kKotoMiddle};
      f = (dir > 0 ? kUp : kDown)[run % 3];
    } else {
      run = 0;
    }
    st[a].finger = f;
    lastFinger = f;
  }
  return st;
}

}  // namespace

int kotoFold(int key) {
  while (key < kKotoOpenLo) key += 12;
  while (key > kKotoOpenHi + 2) key -= 12;
  return key;
}

KotoPlan planKoto(const std::vector<KotoNote>& notes) {
  KotoPlan plan;
  if (notes.empty() || knob("PJ_KOTO_PLAN", 1) == 0) return plan;
  static const bool kLog = std::getenv("PJ_KOTO_LOG") != nullptr;  // (each retuning, and why)
  // The breaths: where the hands are free a moment (a ji can be moved there).
  std::vector<size_t> start{0};
  double lastOff = notes[0].off;
  for (size_t i = 1; i < notes.size(); ++i) {
    if (notes[i].t - notes[i - 1].t >= kGap || notes[i].t - lastOff >= kFree) start.push_back(i);
    lastOff = std::max(lastOff, notes[i].off);
  }
  start.push_back(notes.size());
  const size_t nc = start.size() - 1;
  auto addChunk = [&](Hist& h, size_t c) {
    for (size_t i = start[c]; i < start[c + 1]; ++i) h[kotoFold(notes[i].key)] += 1 + 0.5 * std::min(notes[i].off - notes[i].t, 2.0);
  };
  // The window from chunk c.
  auto window = [&](size_t c) {
    Hist h;
    addChunk(h, c);
    for (size_t d = c + 1; d < nc && notes[start[d]].t - notes[start[c]].t < kWindow; ++d) addChunk(h, d);
    return h;
  };
  std::vector<int> cur = solve(window(0), nullptr);
  plan.steps.push_back({0.0, {}});
  std::copy(cur.begin(), cur.end(), plan.steps.back().open);
  double lastMove = notes[0].t;
  int moves = 0;
  for (size_t c = 1; c < nc; ++c) {
    Hist hc;
    addChunk(hc, c);
    double cc = 0;
    bool away = false;
    for (const auto& [p, w] : hc) {
      const double k = cost(p, cur);
      cc += w * k;
      away = away || k >= kAway;
    }
    if (cc <= 0) continue;
    const double t = notes[start[c]].t;
    if (!away && t - lastMove < kSettle) continue;
    const Hist hw = window(c);
    const std::vector<int> nw = solve(hw, &cur);
    const int m = moved(cur, nw);
    if (m == 0 || histCost(hw, cur) - histCost(hw, nw) < kMoveCost * m) continue;
    // (between the chunk's first onset and the one before it: the hands free)
    const double at = 0.5 * (notes[start[c] - 1].t + t);
    if (kLog)
      std::fprintf(stderr, "koto plan %.2f s: %s -> %s (%d moved; the window %zu pitches, cost %.1f -> %.1f%s)\n", at,
                   names(cur).c_str(), names(nw).c_str(), m, hw.size(), histCost(hw, cur), histCost(hw, nw),
                   away ? ", a note out of reach" : "");
    plan.steps.push_back({at, {}});
    std::copy(nw.begin(), nw.end(), plan.steps.back().open);
    cur = nw;
    lastMove = t;
    moves += m;
  }
  // What it came to: the share of the notes pressed, and of those out of reach.
  size_t k = 0, pressed = 0, away = 0;
  std::vector<bool> press(notes.size());
  for (size_t i = 0; i < notes.size(); ++i) {
    const KotoNote& n = notes[i];
    while (k + 1 < plan.steps.size() && plan.steps[k + 1].t <= n.t) ++k;
    const std::vector<int> s(plan.steps[k].open, plan.steps[k].open + 13);
    const double c = cost(kotoFold(n.key), s);
    pressed += c > 0 && c < kAway;
    away += c >= kAway;
    press[i] = c > 0;
  }
  plan.strokes = finger(notes, press);
  size_t by[4] = {};
  for (const KotoStroke& s : plan.strokes) ++by[s.finger];
  const double N = 100.0 / notes.size();
  char buf[640];
  std::snprintf(buf, sizeof buf, "the ji set for the part: %s; %zu retunings (%d ji moved at breaths); %.0f %% of the notes pressed, %zu out of reach; "
                "thumb %.0f %%, index %.0f %%, middle %.0f %%, the left hand %.0f %%",
                names(std::vector<int>(plan.steps[0].open, plan.steps[0].open + 13)).c_str(), plan.steps.size() - 1, moves,
                100.0 * pressed / notes.size(), away, by[0] * N, by[1] * N, by[2] * N, by[3] * N);
  plan.summary = buf;
  return plan;
}

}  // namespace pajupilli
