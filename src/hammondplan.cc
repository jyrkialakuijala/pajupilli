#include "hammondplan.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace pajupilli {

HammondPlan planHammond(const std::vector<HammondNote>& notes, int tpq, double bpm, const std::string& instrument,
                        bool fileLeslie) {
  HammondPlan plan;
  plan.reg = hammondPreset(instrument);
  if (notes.empty()) {
    plan.summary = "hammond: no notes\n";
    return plan;
  }
  // The lines and their character.
  struct Line { int n = 0; double pitch = 0; int onsets = 0; long last = -1; };
  std::map<int, Line> lines;
  for (const HammondNote& n : notes) {
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
  std::map<int, std::vector<const HammondNote*>> byLine;
  for (const HammondNote& n : notes) byLine[n.line].push_back(&n);
  int counts[3] = {};
  std::string lineText;
  for (auto& [id, v] : byLine) {
    std::sort(v.begin(), v.end(), [](const HammondNote* a, const HammondNote* b) { return a->on != b->on ? a->on < b->on : a->pitch < b->pitch; });
    // The line's part: the pedals; the upper manual (the tune's own track); the lower (an
    // accompaniment's track: its low notes under it the pedals if no line has them); or both hands
    // in one track, split moment by moment.
    const size_t handLines = lines.size() - (pedalLine >= 0 ? 1 : 0);
    const char mode = id == pedalLine ? 'P' : handLines < 2 ? 'S' : id == upperLine ? 'U' : 'L';
    if (mode == 'P' || mode == 'U') {
      const int div = mode == 'P' ? HammondModel::kPedal : HammondModel::kUpper;
      for (const HammondNote* n : v) { plan.division[{n->ch * 128 + n->pitch, n->on}] = div; ++counts[div]; }
      char buf[96];
      std::snprintf(buf, sizeof buf, "track %d: %s; ", id % 1000, mode == 'P' ? "pedals" : "upper");
      lineText += buf;
      continue;
    }
    // (with a separate lower line and this the upper, still split off what lies far below)
    const bool mayPedal = pedalLine < 0;
    int lastSingle = -1, lastDiv = HammondModel::kUpper;
    for (size_t i = 0; i < v.size();) {
      size_t j = i;
      while (j < v.size() && v[j]->on - v[i]->on <= together) ++j;
      std::vector<const HammondNote*> g(v.begin() + i, v.begin() + j);
      std::sort(g.begin(), g.end(), [](const HammondNote* a, const HammondNote* b) { return a->pitch < b->pitch; });
      size_t lo = 0;
      if (mayPedal && g.size() >= 2 && g[0]->pitch < 48 && g[1]->pitch - g[0]->pitch >= 7) {
        plan.division[{g[0]->ch * 128 + g[0]->pitch, g[0]->on}] = HammondModel::kPedal;
        ++counts[HammondModel::kPedal];
        lo = 1;
      }
      if (mode == 'L') {
        for (size_t k = lo; k < g.size(); ++k) {
          plan.division[{g[k]->ch * 128 + g[k]->pitch, g[k]->on}] = HammondModel::kLower;
          ++counts[HammondModel::kLower];
        }
      } else if (g.size() - lo == 1) {
        // A single note: the tune keeps its manual while it moves by steps; a leap down into
        // the low register goes to the lower manual (or, alone and low, the pedals).
        const HammondNote* n = g[lo];
        int div;
        if (lastSingle >= 0 && std::abs(n->pitch - lastSingle) <= 5) div = lastDiv;
        else if (mayPedal && lo == 0 && n->pitch < 43) div = HammondModel::kPedal;
        else div = n->pitch < 55 ? HammondModel::kLower : HammondModel::kUpper;
        if (div != HammondModel::kPedal) { lastSingle = n->pitch; lastDiv = div; }
        plan.division[{n->ch * 128 + n->pitch, n->on}] = div;
        ++counts[div];
      } else {
        const int top = g.back()->pitch;
        for (size_t k = lo; k < g.size(); ++k) {
          // (a note close under the top that moves with it, as long: the right hand's block
          // chord; held on while the tune moves: the left hand's)
          const double len = double(g[k]->off - g[k]->on), topLen = double(g.back()->off - g.back()->on);
          const bool together = std::fabs(len - topLen) <= 0.3 * std::max(topLen, 1.0);
          const int div = (k + 1 == g.size() || (top - g[k]->pitch <= 9 && g[k]->pitch >= 60 && together)) ? HammondModel::kUpper : HammondModel::kLower;
          plan.division[{g[k]->ch * 128 + g[k]->pitch, g[k]->on}] = div;
          ++counts[div];
        }
        lastSingle = top;
        lastDiv = HammondModel::kUpper;
      }
      i = j;
    }
    char buf[64];
    std::snprintf(buf, sizeof buf, "track %d: %s; ", id % 1000, mode == 'L' ? "lower" : "split by hand");
    lineText += buf;
  }
  // The piece's character: the upper manual's notes a second, how many of its moments are chords.
  long first = notes.front().on, last = 0;
  int upperOnsets = 0, chordOnsets = 0, allOnsets = 0;
  {
    std::map<long, int> perOnset;
    for (const HammondNote& n : notes) {
      first = std::min(first, n.on);
      last = std::max(last, n.off);
      ++perOnset[n.on];
      if (plan.division[{n.ch * 128 + n.pitch, n.on}] == HammondModel::kUpper) ++upperOnsets;
    }
    for (auto& [t, c] : perOnset) { ++allOnsets; if (c >= 3) ++chordOnsets; }
  }
  const double secs = std::max(1.0, (last - first) / double(tpq) * 60 / std::max(bpm, 20.0));
  const double busy = upperOnsets / secs, chordal = double(chordOnsets) / std::max(allOnsets, 1);
  HammondRegistration& r = plan.reg;
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
    r.vibrato = 6;
  } else {
    style = "jazz";
    r.upper = {8, 8, 8, 0, 0, 0, 0, 0, 0};
    r.perc = true;
    r.percThird = true;
    r.percFast = true;
    r.vibrato = 6;
  }
  if (counts[HammondModel::kPedal] == 0) r.pedal = {8, 5};
  hammondOverrides(r);  // (PJ_HAMMOND_*: by hand, over the plan)
  // The Leslie: the file's own (CC1, CC64, CC82) if it has any; else planned. Most pieces stay on
  // the slow rotors (chorale) throughout; about one in four (one in two on the rock organ) goes
  // fast where it builds most: the phrase (between the band's rests) with the most, loudest notes
  // a second, and perhaps a second one nearly as strong; fast from its start, slow again at its
  // end. The choice is drawn from the notes themselves: the same file, the same plan.
  std::string leslieText;
  if (fileLeslie || std::getenv("PJ_HAMMOND_LESLIE")) {
    leslieText = fileLeslie ? "the file's" : "set by hand";
  } else {
    uint32_t h = 2166136261u;
    for (const HammondNote& n : notes) h = (h ^ uint32_t(n.pitch * 131 + n.on)) * 16777619u;
    auto draw = [&h] { h = h * 1664525u + 1013904223u; return (h >> 8) * (1.0 / 16777216.0); };
    const bool rock = instrument == "rock-organ";
    r.leslieFast = false;
    plan.leslie.push_back({first, false});
    if (draw() < (rock ? 0.5 : 0.25)) {
      // Phrases: spans of notes with no silence of a beat or more in all the lines.
      std::vector<std::pair<long, long>> spans;
      std::vector<const HammondNote*> byOn;
      for (const HammondNote& n : notes) byOn.push_back(&n);
      std::sort(byOn.begin(), byOn.end(), [](const HammondNote* a, const HammondNote* b) { return a->on < b->on; });
      long s0 = byOn[0]->on, reach = byOn[0]->off;
      for (const HammondNote* n : byOn) {
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
        for (const HammondNote* n : byOn)
          if (n->on >= spans[i].first && n->on < spans[i].second) { loud += n->vel; ++k; }
        score.push_back({k / beats * (loud / std::max(k, 1)) / 100, i});
      }
      std::sort(score.rbegin(), score.rend());
      std::vector<size_t> pick;
      if (!score.empty()) pick.push_back(score[0].second);
      if (score.size() > 2 && score[1].first > 0.85 * score[0].first && draw() < 0.5) pick.push_back(score[1].second);
      std::sort(pick.begin(), pick.end());
      for (size_t i : pick) {
        plan.leslie.push_back({spans[i].first, true});
        plan.leslie.push_back({spans[i].second, false});
      }
      char b2[64];
      std::snprintf(b2, sizeof b2, "planned: fast in %zu phrase%s", pick.size(), pick.size() == 1 ? "" : "s");
      leslieText = b2;
    } else {
      leslieText = "planned: slow throughout";
    }
  }
  char buf[640];
  std::snprintf(buf, sizeof buf,
                "hammond plan: %s (%.0f bpm, %.1f tune notes a second, %.0f %% chords): %s\n"
                "%snotes: %d upper, %d lower, %d pedals; Leslie %s\n",
                style.c_str(), bpm, busy, 100 * chordal, r.text().c_str(), lineText.c_str(), counts[HammondModel::kUpper],
                counts[HammondModel::kLower], counts[HammondModel::kPedal], leslieText.c_str());
  plan.summary = buf;
  return plan;
}

}  // namespace pajupilli
