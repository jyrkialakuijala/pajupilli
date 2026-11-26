#include "tonewheelplan.h"

#include "knob.h"
#include "vmath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

namespace pajupilli {

TonewheelPlan planTonewheel(const std::vector<TonewheelNote>& notes, int tpq, double bpm, const std::string& instrument,
                        bool fileRotary, bool fileExpr, std::function<double(long)> secondsAt) {
  TonewheelPlan plan;
  plan.reg = tonewheelPreset(instrument);
  if (notes.empty()) {
    plan.summary = "tonewheel: no notes\n";
    return plan;
  }
  // The lines and their character.
  struct Line { int n = 0; double pitch = 0; int onsets = 0; long last = -1; };
  std::map<int, Line> lines;
  for (const TonewheelNote& n : notes) {
    Line& l = lines[n.line];
    ++l.n;
    l.pitch += n.pitch;
    if (n.on != l.last) { ++l.onsets; l.last = n.on; }
  }
  for (auto& [id, l] : lines) l.pitch /= l.n;
  int pedalLine = -1, upperLine = -1;
  if (lines.size() >= 2) {
    double lowest = 1e9, highest = -1e9;
    for (auto& [id, l] : lines) {
      const double poly = double(l.n) / std::max(l.onsets, 1);
      if (l.pitch < 50 && poly < 1.3 && l.n >= 8 && l.pitch < lowest) { lowest = l.pitch; pedalLine = id; }
    }
    // Three (or more) lines: an organist's three staves, the lowest the pedals (a bass line
    // within the pedals' reach, mostly one note at a time), the others the two manuals.
    if (pedalLine < 0 && lines.size() >= 3) {
      int low = -1;
      for (auto& [id, l] : lines)
        if (low < 0 || l.pitch < lines[low].pitch) low = id;
      const Line& l = lines[low];
      if (l.pitch < 57 && double(l.n) / std::max(l.onsets, 1) < 1.6) pedalLine = low;
    }
    for (auto& [id, l] : lines)
      if (id != pedalLine && l.pitch > highest) { highest = l.pitch; upperLine = id; }
  }
  // Per line: a whole-line division, or the hands split moment by moment.
  const long together = std::max(1, tpq / 24);  // (begun "together": within a 96th)
  std::map<int, std::vector<const TonewheelNote*>> byLine;
  for (const TonewheelNote& n : notes) byLine[n.line].push_back(&n);
  int counts[3] = {};
  std::string lineText;
  for (auto& [id, v] : byLine) {
    std::sort(v.begin(), v.end(), [](const TonewheelNote* a, const TonewheelNote* b) { return a->on != b->on ? a->on < b->on : a->pitch < b->pitch; });
    // The line's part: the pedals; the upper manual (the tune's own track); the lower (an
    // accompaniment's track: its low notes under it the pedals if no line has them); or both hands
    // in one track, split moment by moment.
    const size_t handLines = lines.size() - (pedalLine >= 0 ? 1 : 0);
    const char mode = id == pedalLine ? 'P' : handLines < 2 ? 'S' : id == upperLine ? 'U' : 'L';
    if (mode == 'P' || mode == 'U') {
      const int div = mode == 'P' ? TonewheelModel::kPedal : TonewheelModel::kUpper;
      for (const TonewheelNote* n : v) { plan.division[{n->ch * 128 + n->pitch, n->on}] = div; ++counts[div]; }
      char buf[96];
      std::snprintf(buf, sizeof buf, "channel %d track %d: %s; ", id / 1000 + 1, id % 1000, mode == 'P' ? "pedals" : "upper");
      lineText += buf;
      continue;
    }
    // (with a separate lower line and this the upper, still split off what lies far below)
    // (a line that never sounds two notes together is a tune, whatever its register: all of it on
    // the upper manual)
    bool chords = false;
    for (size_t i = 1; i < v.size() && !chords; ++i) chords = v[i]->on - v[i - 1]->on <= together;
    const bool mayPedal = pedalLine < 0 && chords;
    int lastSingle = -1, lastDiv = TonewheelModel::kUpper;
    for (size_t i = 0; i < v.size();) {
      size_t j = i;
      while (j < v.size() && v[j]->on - v[i]->on <= together) ++j;
      std::vector<const TonewheelNote*> g(v.begin() + i, v.begin() + j);
      std::sort(g.begin(), g.end(), [](const TonewheelNote* a, const TonewheelNote* b) { return a->pitch < b->pitch; });
      size_t lo = 0;
      if (mayPedal && g.size() >= 2 && g[0]->pitch < 48 && g[1]->pitch - g[0]->pitch >= 7) {
        plan.division[{g[0]->ch * 128 + g[0]->pitch, g[0]->on}] = TonewheelModel::kPedal;
        ++counts[TonewheelModel::kPedal];
        lo = 1;
      }
      if (mode == 'L') {
        for (size_t k = lo; k < g.size(); ++k) {
          plan.division[{g[k]->ch * 128 + g[k]->pitch, g[k]->on}] = TonewheelModel::kLower;
          ++counts[TonewheelModel::kLower];
        }
      } else if (g.size() - lo == 1) {
        // A single note: the tune keeps its manual while it moves by steps; a leap down into
        // the low register goes to the lower manual (or, alone and low, the pedals).
        const TonewheelNote* n = g[lo];
        int div;
        if (!chords && mode == 'S') div = TonewheelModel::kUpper;
        else if (lastSingle >= 0 && std::abs(n->pitch - lastSingle) <= 5) div = lastDiv;
        else if (mayPedal && lo == 0 && n->pitch < 43) div = TonewheelModel::kPedal;
        else div = n->pitch < 55 ? TonewheelModel::kLower : TonewheelModel::kUpper;
        if (div != TonewheelModel::kPedal) { lastSingle = n->pitch; lastDiv = div; }
        plan.division[{n->ch * 128 + n->pitch, n->on}] = div;
        ++counts[div];
      } else {
        const int top = g.back()->pitch;
        for (size_t k = lo; k < g.size(); ++k) {
          // (a note close under the top that moves with it, as long: the right hand's block
          // chord; held on while the tune moves: the left hand's)
          const double len = double(g[k]->off - g[k]->on), topLen = double(g.back()->off - g.back()->on);
          const bool together = std::fabs(len - topLen) <= 0.3 * std::max(topLen, 1.0);
          const int div = (k + 1 == g.size() || (top - g[k]->pitch <= 9 && g[k]->pitch >= 60 && together)) ? TonewheelModel::kUpper : TonewheelModel::kLower;
          plan.division[{g[k]->ch * 128 + g[k]->pitch, g[k]->on}] = div;
          ++counts[div];
        }
        lastSingle = top;
        lastDiv = TonewheelModel::kUpper;
      }
      i = j;
    }
    char buf[64];
    std::snprintf(buf, sizeof buf, "channel %d track %d: %s; ", id / 1000 + 1, id % 1000, mode == 'L' ? "lower" : "split by hand");
    lineText += buf;
  }
  // Notes routed to a division by their instrument (tonewheel-upper-manual, -lower-manual, -pedal):
  // there, whatever the plan said.
  for (const TonewheelNote& n : notes)
    if (n.forced >= 0) {
      int& d = plan.division[{n.ch * 128 + n.pitch, n.on}];
      --counts[d];
      d = n.forced;
      ++counts[d];
    }
  // The piece's character: the upper manual's notes a second, how many of its moments are chords.
  long first = notes.front().on, last = 0;
  int upperOnsets = 0, chordOnsets = 0, allOnsets = 0;
  {
    std::map<long, int> perOnset;
    for (const TonewheelNote& n : notes) {
      first = std::min(first, n.on);
      last = std::max(last, n.off);
      ++perOnset[n.on];
      if (plan.division[{n.ch * 128 + n.pitch, n.on}] == TonewheelModel::kUpper) ++upperOnsets;
    }
    for (auto& [t, c] : perOnset) { ++allOnsets; if (c >= 3) ++chordOnsets; }
  }
  const double secs = std::max(1.0, (last - first) / double(tpq) * 60 / std::max(bpm, 20.0));
  const double busy = upperOnsets / secs, chordal = double(chordOnsets) / std::max(allOnsets, 1);
  TonewheelRegistration& r = plan.reg;
  std::string style;
  if (instrument == "rock-organ") {
    style = "rock";
  } else if (instrument == "percussive-organ") {
    style = "percussive";
  } else if (chordal > 0.5 && busy < 4) {
    style = "full (chordal)";
    r.upper = {8, 8, 8, 8, 0, 0, 0, 0, 0};
    r.vibrato = 6;
  } else if (bpm < 90 && busy < 3) {
    style = "ballad";
    r.upper = {8, 3, 8, 0, 0, 0, 0, 0, 0};
    r.lower = {8, 0, 8, 0, 0, 0, 0, 0, 0};
    r.vibrato = 4;  // (C1: the ballad clean, a slight movement that blurs neither line nor attack)
  } else {
    style = "jazz";
    r.upper = {8, 8, 8, 0, 0, 0, 0, 0, 0};
    r.vibrato = 6;
  }
  // Harmonically complex (dense chords, semitones and tritones among them, close intervals low): the
  // 888's sub-octave and fifth (16', 5 1/3') turn its harmony to mud, so organists thin to a single
  // foundation, the 8' (008000000, with a little 4'), or nearly. Measured at every onset over what
  // sounds then: the pitch classes beyond a triad, a semitone or tritone among them, a second or
  // third under C4; 0..1 (Bach's chorales ~0.2, jazz standards 0.3-0.45). From 0.25 the 16' and
// 5 1/3' fade, from 0.4 a single drawbar
  // (PJ_TONEWHEEL_COMPLEX scales the measure). (Not the rock organ: its grit is the point.)
  std::string complexText;
  double harmonyCx = 0;
  if (instrument != "rock-organ" && !std::getenv("PJ_TONEWHEEL_UPPER")) {
    std::vector<long> ons;
    for (const TonewheelNote& n : notes) ons.push_back(n.on);
    std::sort(ons.begin(), ons.end());
    ons.erase(std::unique(ons.begin(), ons.end()), ons.end());
    double sum = 0;
    int m = 0;
    for (long t : ons) {
      int pcMask = 0;
      std::vector<int> low;
      for (const TonewheelNote& n : notes)
        if (n.on <= t && n.off > t) {
          pcMask |= 1 << (n.pitch % 12);
          if (n.pitch < 60) low.push_back(n.pitch);
        }
      int pcs = 0;
      for (int b = 0; b < 12; ++b) pcs += (pcMask >> b) & 1;
      if (pcs < 2) continue;
      bool harsh = false;
      for (int a = 0; a < 12; ++a)
        if ((pcMask >> a) & 1) harsh |= ((pcMask >> ((a + 1) % 12)) & 1) || ((pcMask >> ((a + 6) % 12)) & 1);
      bool lowClose = false;
      std::sort(low.begin(), low.end());
      for (size_t i = 1; i < low.size(); ++i) lowClose |= low[i] - low[i - 1] > 0 && low[i] - low[i - 1] <= 4;
      sum += 0.4 * std::clamp((pcs - 3) / 3.0, 0.0, 1.0) + 0.3 * harsh + 0.3 * lowClose;
      ++m;
    }
    static const double kComplex = knob("PJ_TONEWHEEL_COMPLEX", 1.0);
    const double cx = m ? kComplex * sum / m : 0;
    harmonyCx = cx;
    if (cx >= 0.4) {
      r.upper = {0, 0, 8, std::min(r.upper[3], 3), 0, 0, 0, 0, 0};
      char b7[96];
      std::snprintf(b7, sizeof b7, "; a single foundation (harmonically complex, %.2f)", cx);
      complexText = b7;
    } else if (cx >= 0.25) {
      const double f = (0.4 - cx) / 0.15;  // (1 at 0.25 .. 0 at 0.4)
      r.upper[0] = int(std::lround(r.upper[0] * (0.3 + 0.7 * f)));
      r.upper[1] = int(std::lround(r.upper[1] * f));
      char b7[96];
      std::snprintf(b7, sizeof b7, "; its subs thinned (harmonically rich, %.2f)", cx);
      complexText = b7;
    } else {
      char b7[64];
      std::snprintf(b7, sizeof b7, "; harmony %.2f", cx);  // (clarity follows below)
      complexText = b7;
    }
  }
  // Played low, the registration moves up: a low line on the 888 is mud, so organists take the
  // drawbars an octave (or two) higher, the 8' alone becoming the 4' alone (000800000): each
  // drawbar to the one an octave above (16' > 8', 5 1/3' > 2 2/3', 8' > 4', 4' > 2', 2 2/3' >
  // 1 1/3', 2' > 1'; the 1 3/5', 1 1/3', 1' having none, gone). By the upper manual's mean pitch:
  // under E3 (52) an octave, under F#2 (42) two; a little of the sub-octave kept.
  std::string shiftText, balanceText_;
  {
    double sum = 0;
    int n = 0;
    for (const TonewheelNote& nt : notes)
      if (plan.division[{nt.ch * 128 + nt.pitch, nt.on}] == TonewheelModel::kUpper) { sum += nt.pitch; ++n; }
    const double mean = n ? sum / n : 72;
    double lsum = 0;
    int ln = 0;
    for (const TonewheelNote& nt : notes)
      if (plan.division[{nt.ch * 128 + nt.pitch, nt.on}] == TonewheelModel::kLower) { lsum += nt.pitch; ++ln; }
    // (a melody low in itself, or under its accompaniment: a 4' melody, the 4' leading an octave up
    // with a little 16' and 8' under it for weight, 302800000; very low, the whole registration
    // two octaves up instead)
    const bool under = ln > 0 && mean <= lsum / ln + 5;  // (within a fourth above it: in its range)
    int shift = mean < 42 ? 2 : mean < 52 ? 1 : 0;
    if (shift < 2 && (under || shift == 1)) {
      r.upper = {3, 0, 2, 8, 0, 0, 0, 0, 0};
      shift = 0;
      char b6[96];
      std::snprintf(b6, sizeof b6, "; a 4' melody (its tune %s, mean MIDI %.0f)", under ? "in its accompaniment's range" : "low", mean);
      shiftText = b6;
    }
    static const int kUp[9] = {2, 4, 3, 5, 7, 8, -1, -1, -1};  // (each drawbar's octave above; -1 none)
    const std::array<int, 9> orig = r.upper;
    for (int s_ = 0; s_ < shift; ++s_) {
      std::array<int, 9> up{};
      for (int d = 0; d < 9; ++d)
        if (kUp[d] >= 0) up[kUp[d]] = std::max(up[kUp[d]], r.upper[d]);
      r.upper = up;
    }
    // (a low line may be low for its weight: a little of the sub-octave stays, a third of the 16'
    // an octave up; two octaves up, a third of the 8' and a fifth of the 16')
    if (shift == 1) r.upper[0] = std::max(r.upper[0], int(std::lround(orig[0] / 3.0)));
    if (shift == 2) {
      r.upper[2] = std::max(r.upper[2], int(std::lround(orig[2] / 3.0)));
      r.upper[0] = std::max(r.upper[0], int(std::lround(orig[0] / 5.0)));
    }
    if (shift) {
      char b4[96];
      std::snprintf(b4, sizeof b4, "; registered %s up (its tune low, mean MIDI %.0f)", shift == 1 ? "an octave" : "two octaves", mean);
      shiftText = b4;
    }
  }
  // The lower manual: the upper's accompaniment, softer and darker (tonewheelLowerFor).
  bool lowerDry = false;  // (its vibrato off: set apart from the tune)
  {
    std::array<double, 9> up{};
    for (int d = 0; d < 9; ++d) up[d] = r.upper[d];
    // (the file's own balance: an accompaniment written quieter than the tune, by velocity and its
    // channels' expression, played on a softer lower manual: 3 dB a step, General MIDI's square law)
    auto med = [&](int div, bool cc) {
      std::vector<double> v;
      for (const TonewheelNote& nt : notes)
        if (plan.division[{nt.ch * 128 + nt.pitch, nt.on}] == div) v.push_back(cc ? nt.expr : nt.vel);
      if (v.empty()) return -1.0;
      std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
      return v[v.size() / 2];
    };
    // (clarity: a pedal line, the busier the more (a walking or contrapuntal bass), and the harmony's
    // richness: the lower's 16' and 5 1/3' eased back, the pedals on less 16' and more 8')
    {
      static const double kClarity = knob("PJ_TONEWHEEL_CLARITY", 1.0);
      const double pedalRate = counts[TonewheelModel::kPedal] / secs;
      double c = counts[TonewheelModel::kPedal] > 8 ? 0.4 + 0.4 * std::clamp((pedalRate - 0.5) / 1.5, 0.0, 1.0) : 0.0;
      c += std::clamp((harmonyCx - 0.15) * 2, 0.0, 0.5);
      plan.clarity = std::clamp(kClarity * c, 0.0, 1.0);
    }
    const double clar = plan.clarity;
    double extra = 0;
    bool contrast = false;
    const double vu = med(TonewheelModel::kUpper, false), vl = med(TonewheelModel::kLower, false);
    const double eu = med(TonewheelModel::kUpper, true), el = med(TonewheelModel::kLower, true);
    if (vu > 0 && vl > 0) {
      const double db = 40 * std::log10(vu / vl) + 40 * std::log10(std::max(eu, 1.0) / std::max(el, 1.0));
      // (the softening that puts the lower's whole level that far under the upper's (3 dB at
      // least): searched, as the keep shares and the steps stack differently on each registration)
      auto level = [](const std::array<double, 9>& b) {
        double g = 0;
        for (double x : b) g += x >= 1 ? fastPow(10.0, -3.0 * (8 - std::min(x, 8.0)) / 20) : std::max(x, 0.0) * fastPow(10.0, -21.0 / 20);
        return 20 * std::log10(std::max(g, 1e-9));
      };
      // (but no further than ~6 dB: a soft accompaniment left a whisper is lost under the tune;
      // past that it is set apart by colour instead, below)
      static const double kGap = knob("PJ_TONEWHEEL_BALANCEGAP", 6.0);  // (dB)
      const double want = level(up) - std::clamp(db, 3.0, std::max(kGap, 3.0));
      contrast = db > kGap + 1;
      double bestErr = 1e9;
      for (double e = -1; e <= 6; e += 0.1) {
        const double err = std::fabs(level(tonewheelLowerFor(up, e, clar)) - want);
        if (err < bestErr) { bestErr = err; extra = e; }
      }
      if (db >= 4) {
        char b5[160];
        if (contrast)
          std::snprintf(b5, sizeof b5, "; the lower %.0f dB under the upper in the file: %.0f dB under, the hollow 8', its vibrato off (set apart by colour)", db, kGap);
        else
          std::snprintf(b5, sizeof b5, "; the lower %.0f dB under the upper (the file's balance)", db);
        balanceText_ = b5;
      }
    }
    plan.lowerExtra = extra;
    const std::array<double, 9> low = tonewheelLowerFor(up, extra, clar);
    std::array<double, 9> lowS = low;
    // (set apart by colour: the hollow 8' (the clarinet's odd harmonics) at that level, a line of
    // its own beside the tune's foundation and flutes, as a trio organist sets two lines apart)
    if (contrast) {
      lowS = tonewheelLowerShaped(low, 2);
      plan.lowerShape = 2;
      lowerDry = true;
    }
    for (int d = 0; d < 9; ++d) r.lower[d] = int(std::lround(lowS[d]));
    const std::array<double, 2> ped = tonewheelPedalFor(up, clar);  // (and the pedals by its loudness and brightness)
    r.pedal = {int(std::lround(ped[0])), int(std::lround(ped[1]))};
  }
  // The vibrato tabs (the scanner's chorus, C3, on both manuals by default): counterpoint (a moving
  // pedal line under a moving lower manual, not chordal) played straight, both tabs off, the lines
  // clear as on a pipe organ; the lower manual carrying the bass itself (no pedal line, its single
  // notes low, under F3) without it, the bass tight, as trio organists play the left hand's line. (Not the presets
  // without it: the percussive and rock organs.)
  std::string vibText;
  if (r.vibrato > 0 && lowerDry) {
    r.vibLower = false;  // (the tune moving, the accompaniment still: one more difference)
  } else if (r.vibrato > 0) {
    const double pedalRate = counts[TonewheelModel::kPedal] / secs, lowerRate = counts[TonewheelModel::kLower] / secs;
    double lsum = 0;
    int ln = 0;
    std::map<long, std::pair<int, int>> lowerAt;  // (the lower manual's moments, notes begun together: count, pitch)
    for (const TonewheelNote& nt : notes)
      if (plan.division[{nt.ch * 128 + nt.pitch, nt.on}] == TonewheelModel::kLower) {
        auto& m = lowerAt[nt.on / together];
        ++m.first;
        m.second = nt.pitch;
      }
    int lowerSingle = 0;
    for (auto& [t, c] : lowerAt)
      if (c.first == 1) { ++lowerSingle; lsum += c.second; ++ln; }
    // (counterpoint: the lower manual a line of its own, mostly single notes, not a comping hand's
    // chords, over a moving pedal line; in plain harmony, not a jazz line's)
    const bool lowerLine = !lowerAt.empty() && lowerSingle >= 0.7 * lowerAt.size();
    if (counts[TonewheelModel::kPedal] > 8 && pedalRate >= 0.8 && lowerRate >= 1 && lowerLine && chordal < 0.35 && harmonyCx < 0.3) {
      r.vibUpper = r.vibLower = false;
      vibText = "; the vibrato off (counterpoint, the lines straight)";
    } else if (ln >= 8 && lsum / ln < 53 && counts[TonewheelModel::kPedal] < std::max(8, ln / 10)) {
      r.vibLower = false;
      vibText = "; the lower's vibrato off (it carries the bass)";
    }
  }
  // The harmonic percussion: where the upper manual's lines are mostly single notes, detached
  // and moving (the jazz organ's line: the 3rd, fast, the ping on every fresh note); not in
  // legato, chordal or slow playing (single-trigger: a legato line would not strike it, a chord
  // or a ballad would only be thinned by the 1' it takes). One choice for the piece; a single long
  // stretch that clearly asks the other way (a quarter of the piece and at least 8 bars) may
  // switch it there and back: at most two changes, mostly none. The percussive organ (GM 18)
  // keeps it throughout.
  std::string percText, detachText;
  {
    // The upper manual's notes in time order.
    std::vector<const TonewheelNote*> up;
    for (const TonewheelNote& n : notes)
      if (plan.division[{n.ch * 128 + n.pitch, n.on}] == TonewheelModel::kUpper) up.push_back(&n);
    std::sort(up.begin(), up.end(), [](const TonewheelNote* a, const TonewheelNote* b) { return a->on != b->on ? a->on < b->on : a->pitch < b->pitch; });
    const double secsPerTick = 60 / std::max(bpm, 20.0) / tpq;
    // Its character over [t0, t1): its moments (notes begun together), the share of them single
    // notes, the share of moves legato (the last still held at the next), moments a second.
    struct Feel { int groups = 0; double single = 0, legato = 0, rate = 0; };
    auto feel = [&](long t0, long t1) {
      Feel f;
      int singles = 0, legato = 0, moves = 0, inGroup = 0;
      long lastOn = -1, lastOff = -1;
      for (const TonewheelNote* n : up) {
        if (n->on < t0 || n->on >= t1) continue;
        if (n->on - lastOn > together) {
          if (lastOn >= 0) { ++f.groups; singles += inGroup == 1; }
          if (lastOff >= 0) { ++moves; legato += lastOff > n->on; }
          inGroup = 0;
          lastOff = -1;
        }
        ++inGroup;
        lastOn = n->on;
        lastOff = std::max(lastOff, n->off);
      }
      if (lastOn >= 0) { ++f.groups; singles += inGroup == 1; }
      f.rate = f.groups / std::max((t1 - t0) * secsPerTick, 0.5);
      f.single = f.groups ? double(singles) / f.groups : 0;
      f.legato = moves ? double(legato) / moves : 0;
      return f;
    };
    // -1 no say (too few notes), else whether it is the jazz line (single, detached, moving).
    auto suits = [&](long t0, long t1) -> int {
      const Feel f = feel(t0, t1);
      if (f.groups < 6) return -1;
      return f.single >= 0.6 && f.legato < 0.45 && f.rate >= 1.5 ? 1 : 0;
    };
    const bool gm18 = instrument == "percussive-organ";
    // (the rock organ too: its staccato riffs punched by the percussion, at the normal volume)
    const bool riffs = style == "jazz" || style == "rock";
    bool global = gm18 || (riffs && suits(first, last + 1) == 1);
    r.perc = global;
    r.percThird = true;
    r.percFast = true;
    // (the jazz organ's setting soft: Jimmy Smith's 888, 3rd, fast, soft; the ping over the line
    // without the normal setting's drop of the whole manual. The percussive organ's preset normal.)
    r.percSoft = !gm18 && style != "rock";
    std::string onText = gm18 || style == "rock" ? "on (3rd, fast)" : "on (3rd, fast, soft)";
    // A ballad's tune of single, spaced notes, high enough to ring: the 2nd, slow and soft, a bell
    // under each note (the 8''s octave), its slow decay through the note.
    if (!gm18 && style == "ballad") {
      const Feel f = feel(first, last + 1);
      double mean = 0;
      for (const TonewheelNote* n : up) mean += n->pitch;
      mean /= std::max<size_t>(up.size(), 1);
      if (f.groups >= 8 && f.single >= 0.8 && f.legato < 0.3 && f.rate >= 0.4 && f.rate <= 2.5 && mean >= 67) {
        global = r.perc = true;
        r.percThird = r.percFast = false;
        onText = "on (2nd, slow, soft: a bell on each note)";
      }
    }
    percText = global ? onText : "off";
    if (!gm18 && riffs) {
      // Eight-bar stretches (4/4 assumed when the file says nothing finer).
      const long window = 32L * tpq;
      std::vector<int> w;
      for (long t = first; t <= last; t += window) w.push_back(suits(t, t + window));
      for (size_t i = 0; i < w.size(); ++i)
        if (w[i] < 0) w[i] = i ? w[i - 1] : int(global);
      // The longest run against the piece's choice.
      size_t bestA = 0, bestLen = 0;
      for (size_t i = 0; i < w.size();) {
        size_t j = i;
        while (j < w.size() && w[j] == w[i]) ++j;
        if (w[i] != int(global) && j - i > bestLen) { bestLen = j - i; bestA = i; }
        i = j;
      }
      if (bestLen >= 1 && bestLen * 4 >= w.size() && w.size() >= 4) {
        // At the first upper note of its first bar, and back at the first after its end.
        auto at = [&](long t) {
          for (const TonewheelNote* n : up)
            if (n->on >= t) return n->on;
          return t;
        };
        const long t0 = at(first + long(bestA) * window), t1 = first + long(bestA + bestLen) * window;
        plan.percussion.push_back({t0, !global});
        if (t1 <= last) plan.percussion.push_back({at(t1), global});
        char b3[128];
        std::snprintf(b3, sizeof b3, "%s, %s for bars %ld-%ld", global ? onText.c_str() : "off", global ? "off" : "on",
                      long(bestA) * 8 + 1, long(bestA + bestLen) * 8);
        percText = b3;
      }
    }
    // (the jazz organist's upper with the percussion: 888000008. The 1' is silent while the
    // percussion is on, its busbar taken for the ping; where the percussion is switched off it
    // comes in, the line's top brightened instead)
    if (!gm18 && style == "jazz" && (global || !plan.percussion.empty())) r.upper[8] = 8;
    // Where the percussion is on, the player chooses which notes ping by the touch: single-trigger,
    // it strikes only after every upper key has been up (~35 ms to charge again). So in a legato
    // stretch the accented notes (a bar's first, 4/4 assumed, or played clearly louder than the
    // line; 1.5 s apart at least) are detached: the note before let go ~45 ms early. The rest stay
    // legato, without the ping, as written.
    auto percAt = [&](long t) {
      bool on = global;
      for (auto& [tk, v] : plan.percussion)
        if (tk <= t) on = v;
      return on;
    };
    static const double kDetach = knob("PJ_TONEWHEEL_DETACH", 1.0);  // (0: the file's legato as written)
    if ((global || !plan.percussion.empty()) && kDetach > 0) {
      std::vector<int> vel;
      for (const TonewheelNote* n : up) vel.push_back(n->vel);
      std::nth_element(vel.begin(), vel.begin() + vel.size() / 2, vel.end());
      const int medVel = vel.empty() ? 64 : vel[vel.size() / 2];
      const long gap = std::max<long>(1, long(std::lround(0.045 / secsPerTick)));
      int detached = 0;
      long lastDetach = -1000000000;
      for (size_t i = 0; i + 1 < up.size(); ++i) {
        const TonewheelNote* a = up[i];
        const TonewheelNote* b = up[i + 1];
        if (b->on - a->on <= together) continue;  // (a chord: not a line's move)
        if (i > 0 && a->on - up[i - 1]->on <= together) continue;
        if (i + 2 < up.size() && up[i + 2]->on - b->on <= together) continue;
        if (a->off <= b->on - gap || !percAt(b->on)) continue;  // (already detached; or no percussion there)
        const bool accent = b->on % (4L * tpq) == 0 || b->vel >= medVel + 12;
        if (!accent || (b->on - a->on) * secsPerTick < 0.12 || (b->on - lastDetach) * secsPerTick < 1.5) continue;
        // (no other upper key held through the lift: it would keep the percussion from charging)
        bool other = false;
        for (const TonewheelNote* n : up)
          if (n != a && n->on < b->on && n->off > b->on - gap) { other = true; break; }
        if (other) continue;
        plan.release[{a->ch * 128 + a->pitch, a->on}] = b->on - gap;
        lastDetach = b->on;
        ++detached;
      }
      if (detached) detachText = "; " + std::to_string(detached) + " accent" + (detached > 1 ? "s" : "") + " detached to strike the percussion";
    }
  }
  // The lower manual as the percussion's stand-in (an organist's trick: "if you are not playing
  // left hand bass and need an extra octave of percussion-like sound, set your lower manual
  // drawbars to 008880080"): the percussion (3rd, fast) on for the piece, the left hand comping,
  // its moments mostly chords, above the bass (the pedals carry it, or the lower's notes lie above
  // G3 on the average). At the lower's planned level (tonewheelLowerShaped).
  std::string lowerPercText;
  {
    static const double kLowerPerc = knob("PJ_TONEWHEEL_LOWERPERC", 1.0);  // (0: never)
    std::map<long, int> at;
    double sum = 0;
    int n = 0;
    for (const TonewheelNote& nt : notes)
      if (plan.division[{nt.ch * 128 + nt.pitch, nt.on}] == TonewheelModel::kLower) {
        ++at[nt.on / together];
        sum += nt.pitch;
        ++n;
      }
    int chords = 0;
    for (auto& [t, c] : at) chords += c >= 2;
    const bool comping = at.size() >= 8 && chords >= 0.5 * at.size();
    const bool aboveBass = counts[TonewheelModel::kPedal] > 8 || (n && sum / n >= 55);
    if (kLowerPerc > 0 && r.perc && r.percThird && r.percFast && plan.percussion.empty() && comping && aboveBass) {
      std::array<double, 9> low{};
      for (int d = 0; d < 9; ++d) low[d] = r.lower[d];
      low = tonewheelLowerShaped(low, 1);
      for (int d = 0; d < 9; ++d) r.lower[d] = int(std::lround(low[d]));
      plan.lowerShape = 1;
      lowerPercText = "; the lower on 008880080 (the percussion's stand-in: the left hand comps over the bass)";
    }
  }
  tonewheelOverrides(r);  // (PJ_TONEWHEEL_*: by hand, over the plan)
  // At the general pauses (every line silent for half a beat and 0.25 s or more), the organist
  // re-registers: one more drawbar of the upper pulled out each time, in the tradition's order
  // (the 4', the 2', the 2 2/3', the 1', the 1 1/3', the 1 3/5'), to 8 (the top ones to 6). A
  // dramatic arc: at most four, and with more pauses than that the later ones (the piece growing
  // toward its end: the pauses nearest 35, 55, 75 and 90 % of it). The lower manual and pedals
  // follow each.
  std::string regText;
  std::array<int, 9> finalUpper = r.upper;  // (the upper's registration at the end, after the pauses)
  std::vector<long> resumes;                // (the re-registering pauses: the music resuming after each)
  {
    std::vector<const TonewheelNote*> byOn;
    for (const TonewheelNote& nt : notes) byOn.push_back(&nt);
    std::sort(byOn.begin(), byOn.end(), [](const TonewheelNote* a, const TonewheelNote* b) { return a->on < b->on; });
    const long minGap = std::max<long>(tpq / 2, long(0.25 / (60 / std::max(bpm, 20.0) / tpq)));  // (half a beat and a quarter second: time to pull a drawbar)
    std::vector<long> pauses;  // (the pause's middle)
    std::map<long, long> pauseLen;  // (its length, ticks)
    long reach = byOn.empty() ? 0 : byOn[0]->off;
    for (size_t i = 1; i < byOn.size(); ++i) {
      if (byOn[i]->on - reach >= minGap) {
        pauses.push_back((reach + byOn[i]->on) / 2);
        pauseLen[pauses.back()] = byOn[i]->on - reach;
      }
      reach = std::max(reach, byOn[i]->off);
    }
    std::vector<long> use;
    if (pauses.size() <= 4) use = pauses;
    else
      for (double at : {0.35, 0.55, 0.75, 0.9}) {
        const long t = first + long(at * (last - first));
        long best = -1;
        for (long pz : pauses)
          if (std::find(use.begin(), use.end(), pz) == use.end() && (best < 0 || std::labs(pz - t) < std::labs(best - t))) best = pz;
        if (best >= 0) use.push_back(best);
      }
    std::sort(use.begin(), use.end());
    static const int kOrder[6] = {3, 5, 4, 8, 7, 6};
    std::array<int, 9> cur = r.upper;
    for (long t : use) {
      bool pulled = false;
      for (int d : kOrder) {
        const int top = d >= 6 ? 6 : 8;
        if (cur[d] < top) {
          cur[d] = std::min(top, cur[d] + (cur[d] == 0 ? top : 8));  // (out to its full mark)
          pulled = true;
          break;
        }
      }
      if (!pulled) break;
      // (a short pause, under 0.7 s, leaves no time to pull a drawbar out to its mark: the player
      // has set the new registration on the B drawbars beforehand and presses the B preset key,
      // the change at once; it starts half way through the pause, as the hand moves there)
      plan.upperChanges.push_back({t, cur});
      resumes.push_back(t);
      static const double kPreset = knob("PJ_TONEWHEEL_PRESETKEY", 0.7);  // (s: pauses shorter take the preset key; 0 never)
      plan.upperInstant.push_back(pauseLen[t] * 60 / std::max(bpm, 20.0) / tpq < kPreset);
    }
    if (!plan.upperChanges.empty()) {
      regText = "; re-registered at " + std::to_string(plan.upperChanges.size()) + " pause" + (plan.upperChanges.size() > 1 ? "s" : "");
      const int keys = int(std::count(plan.upperInstant.begin(), plan.upperInstant.end(), true));
      if (keys) regText += " (" + std::to_string(keys) + " by the preset key)";
      regText += ", to ";
      for (int v : plan.upperChanges.back().second) regText += char('0' + v);
    }
    finalUpper = cur;
  }
  // Trills: a line alternating between two neighbouring notes (a semitone or a tone apart), six
  // notes or more each within 0.2 s of the last (four, 0.1 s apart: a figure of four slower ones is
  // a turn, not a trill). Played with a rising touch: from about half the
  // trill's own velocity to 1.3 x it (127 at most), so the key clicks sharpen as it goes, the
  // contacts struck ever harder (PJ_TONEWHEEL_TRILL scales the rise; 0 off).
  {
    static const double kTrill = knob("PJ_TONEWHEEL_TRILL", 1.0);
    const double spt = 60 / std::max(bpm, 20.0) / tpq;
    std::map<int, std::vector<const TonewheelNote*>> byLine;
    for (const TonewheelNote& n : notes) byLine[n.line].push_back(&n);
    for (auto& [line, v] : byLine) {
      std::sort(v.begin(), v.end(), [](const TonewheelNote* a, const TonewheelNote* b) { return a->on < b->on || (a->on == b->on && a->pitch < b->pitch); });
      // (single notes only: a moment with a chord breaks a trill)
      size_t i = 0;
      while (i < v.size()) {
        size_t j = i + 1;
        const int a = v[i]->pitch;
        int b = -1;
        while (j < v.size()) {
          const int p = v[j]->pitch, iv = std::abs(p - a);
          const bool next = (v[j]->on - v[j - 1]->on) * spt <= 0.2 && v[j]->on > v[j - 1]->on;
          if (!next) break;
          if (b < 0) {
            if (iv < 1 || iv > 2) break;
            b = p;
          }
          if (p != (((j - i) & 1) ? b : a)) break;
          ++j;
        }
        const double meanGap = j - i > 1 ? (v[j - 1]->on - v[i]->on) * spt / (j - i - 1) : 1;
        if (kTrill > 0 && b >= 0 && (j - i >= 6 || (j - i >= 4 && meanGap <= 0.1))) {
          std::vector<int> vel;
          for (size_t k = i; k < j; ++k) vel.push_back(v[k]->vel);
          std::nth_element(vel.begin(), vel.begin() + vel.size() / 2, vel.end());
          const double base = vel[vel.size() / 2];
          for (size_t k = i; k < j; ++k) {
            const double x = double(k - i) / (j - i - 1);
            const double f = 1 + kTrill * ((0.5 + 0.8 * x) - 1);
            plan.velocity[{v[k]->ch * 128 + v[k]->pitch, v[k]->on}] = std::clamp(int(std::lround(base * f)), 1, 127);
          }
          ++plan.trills;
          {
            static const double kThin = knob("PJ_TONEWHEEL_TRILLTHIN", 1.0);
            if (kThin > 0) {
              plan.thin.push_back({v[i]->on, kThin});
              plan.thin.push_back({v[j - 1]->off, 0.0});
            }
          }
          if (plan.trills == 1) {
            plan.trillFrom = plan.velocity[{v[i]->ch * 128 + v[i]->pitch, v[i]->on}];
            plan.trillTo = plan.velocity[{v[j - 1]->ch * 128 + v[j - 1]->pitch, v[j - 1]->on}];
          }
          i = j;
        } else {
          ++i;
        }
      }
    }
  }
  std::sort(plan.thin.begin(), plan.thin.end());
  // The expression pedal follows the tune: the channels whose notes are mostly on the upper manual.
  {
    std::map<int, std::pair<int, int>> byCh;  // channel -> (upper notes, all)
    for (const TonewheelNote& nt : notes) {
      auto& c = byCh[nt.ch];
      ++c.second;
      if (plan.division[{nt.ch * 128 + nt.pitch, nt.on}] == TonewheelModel::kUpper) ++c.first;
    }
    for (auto& [ch, c] : byCh)
      if (2 * c.first >= c.second) plan.exprChannels.insert(ch);
  }
  // The Rotary: the file's own (CC1, CC64, CC82) if it has any; else planned. Most pieces stay on
  // the slow rotors (chorale) throughout; about one in four (one in two on the rock organ) goes
  // fast where it builds most: the phrase (between the band's rests) with the most, loudest notes
  // a second, and perhaps a second one nearly as strong; fast from its start, slow again at its
  // end. The choice is drawn from the notes themselves: the same file, the same plan.
  // The final chord: the notes sounding at the end (let go within half a beat of the last), from
  // the moment the last of them began; long enough to shape (by the tempo map, its ritardando
  // included): 1.5 s, two notes or more (a single note 2.5 s).
  const double spt = 60 / std::max(bpm, 20.0) / tpq;
  auto secOf = [&](long t) { return secondsAt ? secondsAt(t) : t * spt; };
  long endAt = -1;
  double endDur = 0;
  {
    long lastOn = first;
    int held = 0;
    for (const TonewheelNote& n : notes)
      if (n.off >= last - tpq / 2) { lastOn = std::max(lastOn, n.on); ++held; }
    endDur = secOf(last) - secOf(lastOn);
    if ((held >= 2 && endDur >= 1.5) || (held == 1 && endDur >= 2.5)) endAt = lastOn;
  }
  // (the ending's and the glissandos' choices drawn from the notes too, their own draws)
  uint32_t h2 = 0x9e3779b9u;
  for (const TonewheelNote& n : notes) h2 = (h2 ^ uint32_t(n.pitch * 7919 + n.on * 31 + n.vel)) * 16777619u;
  auto draw2 = [&h2] { h2 = h2 * 1664525u + 1013904223u; return (h2 >> 8) * (1.0 / 16777216.0); };
  // The rock organ's ending, about one in two: the run switch off on the final chord, the generator
  // coasting down under the held keys, the chord sinking and fading (the Rotary left as it is).
  static const double kDive = knob("PJ_TONEWHEEL_DIVE", 0.5);      // (the share of rock endings with it)
  static const double kEnding = knob("PJ_TONEWHEEL_ENDING", 1.0);  // (0: no shaped ending)
  if (kEnding <= 0) endAt = -1;
  const bool dive = instrument == "rock-organ" && endAt >= 0 && draw2() < kDive;
  std::vector<std::string> endParts;
  std::vector<long> climaxes;  // (the planned fast phrases' starts)
  long rotaryEnd = -1;         // (the ending's Rotary from here: no flicks or nudges after it)
  std::string rotaryText;
  if (fileRotary || std::getenv("PJ_TONEWHEEL_ROTARY")) {
    rotaryText = fileRotary ? "the file's" : "set by hand";
  } else {
    uint32_t h = 2166136261u;
    for (const TonewheelNote& n : notes) h = (h ^ uint32_t(n.pitch * 131 + n.on)) * 16777619u;
    auto draw = [&h] { h = h * 1664525u + 1013904223u; return (h >> 8) * (1.0 / 16777216.0); };
    const bool rock = instrument == "rock-organ";
    r.rotaryFast = false;
    plan.rotary.push_back({first, false});
    static const double kFastShare = knob("PJ_TONEWHEEL_FASTSHARE", 0.25);  // (the share of pieces with a fast phrase; the rock organ twice it)
    if (draw() < std::min(1.0, kFastShare * (rock ? 2 : 1))) {
      // Phrases: spans of notes with no silence of a beat or more in all the lines.
      std::vector<std::pair<long, long>> spans;
      std::vector<const TonewheelNote*> byOn;
      for (const TonewheelNote& n : notes) byOn.push_back(&n);
      std::sort(byOn.begin(), byOn.end(), [](const TonewheelNote* a, const TonewheelNote* b) { return a->on < b->on; });
      long s0 = byOn[0]->on, reach = byOn[0]->off;
      for (const TonewheelNote* n : byOn) {
        if (n->on > reach + tpq) { spans.push_back({s0, reach}); s0 = n->on; }
        reach = std::max(reach, n->off);
      }
      spans.push_back({s0, reach});
      std::vector<std::pair<double, size_t>> score;
      for (size_t i = 0; i < spans.size(); ++i) {
        const double beats = std::max(2.0, double(spans[i].second - spans[i].first) / tpq);
        if (beats < 4) continue;  // (too short to turn up for)
        double loud = 0;
        int k = 0;
        for (const TonewheelNote* n : byOn)
          if (n->on >= spans[i].first && n->on < spans[i].second) { loud += n->vel; ++k; }
        score.push_back({k / beats * (loud / std::max(k, 1)) / 100, i});
      }
      std::sort(score.rbegin(), score.rend());
      std::vector<size_t> pick;
      if (!score.empty()) pick.push_back(score[0].second);
      if (score.size() > 2 && score[1].first > 0.85 * score[0].first && draw() < 0.5) pick.push_back(score[1].second);
      std::sort(pick.begin(), pick.end());
      for (size_t i : pick) {
        plan.rotary.push_back({spans[i].first, true});
        plan.rotary.push_back({spans[i].second, false});
        climaxes.push_back(spans[i].first);
        // (a fast Rotary over the lower's chorus-vibrato smears the accompaniment: its tab off for
        // the phrase, a dry, tight comping under the upper's C3; the tab flipped as the switch is)
        if (r.vibrato > 0 && r.vibLower) {
          plan.lowerVib.push_back({spans[i].first, false});
          plan.lowerVib.push_back({spans[i].second, true});
        }
      }
      char b2[64];
      std::snprintf(b2, sizeof b2, "planned: fast in %zu phrase%s", pick.size(), pick.size() == 1 ? "" : "s");
      rotaryText = b2;
      if (!plan.lowerVib.empty()) rotaryText += " (the lower's vibrato off in it)";
    } else {
      rotaryText = "planned: slow throughout";
    }
    // The ending, on most pieces (four in five, PJ_TONEWHEEL_ENDROTARY) with a final chord of 2 s or more: the Rotary
    // switched up as the chord is struck and back to slow at four fifths of it, so the rotors are
    // still winding down when it is let go; already fast, it is switched to slow a third of the
    // way in, the swirl easing to the end. (Not under the rock organ's run switch.)
    static const double kEndRotary = knob("PJ_TONEWHEEL_ENDROTARY", 0.8);  // (the share of endings with it)
    if (endAt >= 0 && endDur >= 2.0 && !dive && draw2() < kEndRotary) {
      const long d = last - endAt;
      bool fastEnd = false;
      for (auto& [tk, f] : plan.rotary)
        if (tk <= endAt) fastEnd = f;
      plan.rotary.erase(std::remove_if(plan.rotary.begin(), plan.rotary.end(), [&](auto& e) { return e.first > endAt; }),
                        plan.rotary.end());
      if (fastEnd) {
        plan.rotary.push_back({endAt + long(0.35 * d), false});
        endParts.push_back("the Rotary winding down");
      } else {
        plan.rotary.push_back({endAt + long(0.12 * d), true});
        plan.rotary.push_back({endAt + long(0.8 * d), false});
        endParts.push_back("the Rotary up and winding down");
      }
      rotaryEnd = endAt;
    }
    // A nudge on a long held note: somewhere round its middle the slow rotors are asked a little
    // faster (1.6-2.4 x), then slow again: the horn swells and settles, the drum barely stirs. On
    // a note held 1.6 s (and two beats) or more with no other note beginning in its middle 60 %,
    // where the plan has the Rotary slow; 8 s apart at least.
    {
      const double secsPerTick = 60 / std::max(bpm, 20.0) / tpq;
      std::vector<long> ons;
      for (const TonewheelNote& n : notes) ons.push_back(n.on);
      std::sort(ons.begin(), ons.end());
      auto fastAt = [&](long t) {
        bool f = false;
        for (auto& [tk, fast] : plan.rotary)
          if (tk <= t) f = fast;
        return f;
      };
      // A flick on a long held chord: three notes or more held unchanged 2.5 s or more (nothing
      // begins or ends in it), the plan slow there: somewhere in its first half the switch goes to
      // fast for 0.35-0.75 s and back, before the rotors get there (the horn ~1 s to its fast
      // speed): the horn swirls up and settles, the drum just stirs. About two in three such
      // chords, 10 s apart at least. (Not on the rock organ's fast pieces' fast phrases.)
      std::vector<long> flicks;
      {
        static const double kFlick = knob("PJ_TONEWHEEL_FLICK", 0.65);  // (the share of held chords flicked)
        std::vector<long> ev;
        for (const TonewheelNote& n : notes) { ev.push_back(n.on); ev.push_back(n.off); }
        std::sort(ev.begin(), ev.end());
        ev.erase(std::unique(ev.begin(), ev.end()), ev.end());
        long lastFlick = -1000000000;
        std::vector<std::pair<long, bool>> add;
        for (size_t i = 0; i + 1 < ev.size(); ++i) {
          const long t0 = ev[i], t1 = ev[i + 1];
          if ((t1 - t0) * secsPerTick < 2.5) continue;
          int held = 0;
          for (const TonewheelNote& n : notes) held += n.on <= t0 && n.off >= t1;
          if (held < 3) continue;
          // (struck there: the swell, an organist's way: "I switch to fast just before I hit the
          // note and switch back to slow in the middle of the note": 0.1-0.3 s before, slow again
          // at 40-60 % of it, the horn about up to speed, the drum half way; else, the chord's
          // notes struck at different times, the flick inside it)
          int struck = 0;
          for (const TonewheelNote& n : notes) struck += n.on == t0 && n.off >= t1;
          const bool swell = struck >= 1;
          long at, len;
          if (swell) {
            at = t0 - long((0.1 + 0.2 * draw()) / secsPerTick);
            len = t0 + long((0.4 + 0.2 * draw()) * (t1 - t0)) - at;
          } else {
            at = t0 + long((0.15 + 0.3 * draw()) * (t1 - t0));
            len = long((0.35 + 0.4 * draw()) / secsPerTick);
          }
          // ("sometimes I leave it on slow over a slow part or a long note just to have that clear
          // sound": in a slow part (under a moment a second over the 10 s round it) half as often)
          int moments = 0;
          long lastOn = -1;
          for (long o : ons)
            if (std::labs(o - t0) * secsPerTick < 5 && o - lastOn > together) { ++moments; lastOn = o; }
          const double share = kFlick * (moments < 10 ? 0.5 : 1.0);
          if ((at - lastFlick) * secsPerTick < 10 || fastAt(at) || fastAt(at + len) || draw() >= share) continue;
          if (rotaryEnd >= 0 && at + len >= rotaryEnd) continue;  // (the ending has its own)
          add.push_back({at, true});
          add.push_back({at + len, false});
          flicks.push_back(at);
          lastFlick = at;
        }
        plan.rotary.insert(plan.rotary.end(), add.begin(), add.end());
        std::stable_sort(plan.rotary.begin(), plan.rotary.end(), [](auto& a, auto& b) { return a.first < b.first; });
      }
      std::vector<const TonewheelNote*> longs;
      for (const TonewheelNote& n : notes)
        if ((n.off - n.on) * secsPerTick >= 1.6 && n.off - n.on >= 2 * tpq) longs.push_back(&n);
      std::sort(longs.begin(), longs.end(), [](const TonewheelNote* a, const TonewheelNote* b) { return a->on < b->on; });
      long lastNudge = -1000000000;
      for (const TonewheelNote* n : longs) {
        const long d = n->off - n->on, a = n->on + long(0.2 * d), b = n->on + long(0.8 * d);
        const auto it = std::upper_bound(ons.begin(), ons.end(), a);
        if (it != ons.end() && *it < b) continue;  // (something begins in its middle: not held alone)
        const long t0 = n->on + long(0.45 * d), t1 = n->on + long(0.7 * d);
        if ((t0 - lastNudge) * secsPerTick < 8 || fastAt(t0) || fastAt(t1)) continue;
        if (rotaryEnd >= 0 && t1 >= rotaryEnd) continue;
        bool nearFlick = false;  // (a flicked chord is not nudged too)
        for (long f : flicks) nearFlick |= std::labs(f - t0) * secsPerTick < 8;
        if (nearFlick) continue;
        const double boost = 1.6 + 0.8 * draw();
        plan.rotaryBoost.push_back({t0, boost});
        plan.rotaryBoost.push_back({t1, 1.0});
        lastNudge = t0;
      }
      if (!flicks.empty()) rotaryText += ", flicked fast on " + std::to_string(flicks.size()) + " held chord" + (flicks.size() > 1 ? "s" : "");
      if (!plan.rotaryBoost.empty()) rotaryText += ", nudged on " + std::to_string(plan.rotaryBoost.size() / 2) + " long note" + (plan.rotaryBoost.size() > 2 ? "s" : "");
    }
  }
  if (endAt >= 0) {
    const long d = last - endAt;
    if (dive) {
      plan.run.push_back({endAt + long(0.5 * d), false});
      endParts.push_back("the run switch off, the chord sinking");
    }
    // The expression pedal on the final chord (when the file has none of its own): a ballad's
    // closing from a third of the way in, the chord fading to ~-12 dB; otherwise the organist's
    // swell, the chord struck, the pedal eased back and opened past where it was.
    if (!fileExpr) {
      if (style == "ballad") {
        plan.swell = {{endAt + long(0.3 * d), 1.0}, {last, 0.6}};
        endParts.push_back("the pedal closing");
      } else {
        plan.swell = {{endAt, 1.0}, {endAt + long(0.1 * d), 0.85}, {endAt + long(0.75 * d), 1.2}};
        endParts.push_back("a swell");
      }
    }
    // Two more drawbars pulled out through a final chord of 2 s or more (the next in the order of
    // the pauses'), the chord opening up: not a ballad's, nor on harmony thinned to one foundation.
    if (style != "ballad" && endDur >= 2.0 && harmonyCx < 0.4) {
      static const int kOrder[6] = {3, 5, 4, 8, 7, 6};
      std::array<int, 9> cur = finalUpper;
      int pulled = 0;
      for (double at : {0.25, 0.5}) {
        for (int dd : kOrder) {
          const int top = dd >= 6 ? 6 : 8;
          if (cur[dd] < top) {
            cur[dd] = top;
            plan.upperChanges.push_back({endAt + long(at * d), cur});
            plan.upperInstant.push_back(false);
            ++pulled;
            break;
          }
        }
      }
      if (pulled) {
        std::string bars;
        for (int v : cur) bars += char('0' + v);
        endParts.push_back(std::to_string(pulled) + " drawbar" + (pulled > 1 ? "s" : "") + " out, to " + bars);
      }
    }
  }
  // Glissandos: the palm swept up the white keys into the first note of a climax (where the Rotary
  // goes fast), and on the rock organ or a full chordal piece into the final chord: over ~0.16-0.26
  // s, from an octave and a bit under the note, each key held into the next, the last let go just
  // before the note so the percussion strikes it. Only where the upper manual is free for the sweep;
  // not on complex harmony (a smear of white keys through it is wrong), not in a ballad. At most two.
  std::string glissText;
  if ((style == "jazz" || style == "rock" || style == "full (chordal)" || style == "percussive") && harmonyCx < 0.3) {
    auto gliss = [&](long t) {
      // (the target: the upper manual's first moment from t on, within two beats; its top note)
      long at = -1;
      for (const TonewheelNote& n : notes)
        if (n.on >= t && n.on <= t + 2L * tpq && plan.division[{n.ch * 128 + n.pitch, n.on}] == TonewheelModel::kUpper && (at < 0 || n.on < at)) at = n.on;
      const TonewheelNote* tg = nullptr;
      for (const TonewheelNote& n : notes)
        if (n.on >= at && n.on <= at + together && plan.division[{n.ch * 128 + n.pitch, n.on}] == TonewheelModel::kUpper && (!tg || n.pitch > tg->pitch)) tg = &n;
      if (at < 0 || !tg) return false;
      const double lspt = std::max(1e-5, (secOf(tg->on) - secOf(tg->on - tpq)) / tpq);  // (the tempo there)
      const long L = std::max<long>(4, long((0.16 + 0.1 * draw2()) / lspt)), t0 = tg->on - L;
      if (t0 < first) return false;
      for (const TonewheelPlan::Extra& x : plan.extra)
        if (x.on >= t0 - L && x.on <= tg->on) return false;  // (swept into already)
      for (const TonewheelNote& n : notes)
        if (plan.division[{n.ch * 128 + n.pitch, n.on}] == TonewheelModel::kUpper && n.on < tg->on && n.off > t0 - long(0.05 / lspt)) return false;
      const int span = 12 + int(5 * draw2());
      std::vector<int> keys;
      for (int k = tg->pitch - span; k < tg->pitch; ++k) {
        const int pc = k % 12;
        if (k >= 36 && (pc == 0 || pc == 2 || pc == 4 || pc == 5 || pc == 7 || pc == 9 || pc == 11)) keys.push_back(k);
      }
      if (keys.size() < 4) return false;
      const size_t m = keys.size();
      const long lift = long(0.035 / lspt) + 1;
      for (size_t k = 0; k < m; ++k) {
        const long on = t0 + long(L * k / m);
        long off = k + 1 < m ? t0 + long(L * (k + 1) / m) + long(0.012 / lspt) : tg->on - lift;
        off = std::min(off, tg->on - lift);
        if (off <= on) off = on + 1;
        bool busy = false;  // (that key sounding on another division)
        for (const TonewheelNote& n : notes) busy |= n.pitch == keys[k] && n.on <= off && n.off >= on;
        if (busy) continue;
        const int vel = std::clamp(int(std::lround(tg->vel * (0.5 + 0.35 * k / double(m - 1)))), 1, 127);
        plan.extra.push_back({on, off, keys[k], vel, TonewheelModel::kUpper});
      }
      return true;
    };
    static const int kGliss = int(knob("PJ_TONEWHEEL_GLISS", 2));  // (at most this many; 0 none)
    int n = 0;
    for (long c : climaxes)
      if (n < kGliss && gliss(c)) ++n;
    // (the rock organ and the full, gospel-like organ also sweep into the music resuming after a
    // re-registering pause, the later ones first: the piece growing)
    if (style == "rock" || style == "full (chordal)")
      for (auto it = resumes.rbegin(); it != resumes.rend() && n < kGliss; ++it)
        if (draw2() < 0.7 && gliss(*it)) ++n;
    if (n < kGliss && endAt >= 0 && (style == "rock" || style == "full (chordal)") && draw2() < 0.6 && gliss(endAt)) ++n;
    if (n) glissText = "; " + std::string(n > 1 ? "two glissandos" : "a glissando") + " up the white keys";
    std::sort(plan.extra.begin(), plan.extra.end(), [](const TonewheelPlan::Extra& a, const TonewheelPlan::Extra& b) { return a.on < b.on; });
  }
  std::string endText;
  for (size_t i = 0; i < endParts.size(); ++i) endText += (i ? ", " : "; the ending: ") + endParts[i];
  char buf[1600];
  std::snprintf(buf, sizeof buf,
                "tonewheel plan: %s (%.0f bpm, %.1f tune notes a second, %.0f %% chords): %s\n"
                "%snotes: %d upper, %d lower, %d pedals; percussion %s; Rotary %s\n",
                style.c_str(), bpm, busy, 100 * chordal, r.text().c_str(), lineText.c_str(), counts[TonewheelModel::kUpper],
                counts[TonewheelModel::kLower], counts[TonewheelModel::kPedal], percText.c_str(), (rotaryText + (plan.trills ? "; " + std::to_string(plan.trills) + " trill" + (plan.trills > 1 ? "s" : "") + " with a rising touch (the first " + std::to_string(plan.trillFrom) + " -> " + std::to_string(plan.trillTo) + ")" : std::string()) + complexText + (plan.clarity > 0.05 ? "; clarity " + std::to_string(int(std::lround(100 * plan.clarity))) + " %" : std::string()) + shiftText + balanceText_ + regText + vibText + detachText + lowerPercText + endText + glissText).c_str());
  plan.summary = buf;
  return plan;
}

}  // namespace pajupilli
