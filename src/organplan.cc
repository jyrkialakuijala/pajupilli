#include "organplan.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <set>

namespace pajupilli {

namespace {

const char* const kLevelName[8] = {"ppp", "pp", "p", "mp", "mf", "f", "ff", "fff"};

// Each division's ladder: its stops at each level (0 ppp .. 7 fff), from its
// own stops only (the user's disposition, research/church_organ.md §6) and no
// couplers between the manuals: each line its own colour and character.
const char* const kHW[8] = {
    "rohrflote8", "rohrflote8", "fluteharm8", "principal8,rohrflote8", "principal8,fluteharm8,octave4",
    "principal8,octave4,superoctave2,mixtur5",
    "principal16,principal8,octave4,quinte223,superoctave2,mixtur5,cymbel3,trompete8",
    "principal16,principal8,fluteharm8,rohrflote8,octave4,quinte223,superoctave2,mixtur5,cymbel3,trompete16,trompete8"};
const char* const kSW[8] = {
    "gedackt8", "gamba8,celeste8", "gedackt8,flute4", "gedackt8,flute4,nasard223", "gedackt8,gamba8,flute4,waldflote2",
    "gedackt8,flute4,nasard223,waldflote2,terz135",
    "bourdon16,gedackt8,flute4,waldflote2,trompette8,oboe8",
    "bourdon16,gedackt8,flute4,waldflote2,fagott16,trompette8,oboe8,clairon4"};
const double kSwellOf[8] = {0.25, 0.5, 0.8, 1, 1, 1, 1, 1};  // (the swell box's opening at each level)
const char* const kPos[8] = {
    "rohrgedackt8", "rohrgedackt8", "rohrgedackt8,gedacktflote4", "rohrgedackt8,posoctave4",
    "rohrgedackt8,posoctave4,gedacktflote4", "rohrgedackt8,gedacktflote4,sesquialtera",
    "rohrgedackt8,posoctave4,cromorne8", "rohrgedackt8,posoctave4,gedacktflote4,sesquialtera,cromorne8"};
const char* const kPed[8] = {
    "subbass16", "subbass16,gedacktbass8", "subbass16,gedacktbass8", "subbass16,octavbass8", "subbass16,violonbass16,octavbass8,choralbass4",
    "principalbass16,octavbass8,choralbass4,posaune16",  // (f: the organo pleno's pedal)
    "principalbass16,subbass16,violonbass16,octavbass8,choralbass4,pedmixtur4,posaune16",
    "principalbass16,subbass16,violonbass16,octavbass8,choralbass4,pedmixtur4,posaune16,pedtrompete8,untersatz32"};
const char* const kDivName[4] = {"Hauptwerk", "Schwellwerk", "Positiv", "Pedal"};

struct TrackStats {
  int track = 0, n = 0, lo = 127, hi = 0, poly = 0;
  double mean = 0, overPedal = 0;  // (the share of its notes above the pedal's compass)
};

}  // namespace

std::string organRegistration(int level, int handsOn, bool pedal) {
  level = std::clamp(level, 0, 7);
  std::string r = handsOn == 2 ? kPos[level] : handsOn == 1 ? kSW[level] : kHW[level];
  if (pedal) r += std::string(",") + kPed[level];
  return r;
}

ZoneRegistration organZoneRegistration(int level) {
  level = std::clamp(level, 0, 7);
  ZoneRegistration z;
  z.rightHand = level <= 2 ? 2 : 0;
  z.stops = std::string(z.rightHand == 2 ? kPos[level] : kHW[level]) + "," + kSW[level] + "," + kPed[level];
  z.swell = kSwellOf[level];
  return z;
}

OrganPlan planOrgan(const std::vector<OrganNote>& notes, int tpq, long barTicks, const std::vector<long>& partStartsIn,
                    const std::vector<std::pair<long, double>>& tempo) {
  OrganPlan plan;
  if (notes.empty()) return plan;
  // The sections: the tempo's changes, and the general pauses (every line
  // silent a beat or more, inside the piece).
  std::vector<long> partStarts = partStartsIn;
  {
    std::vector<std::pair<long, long>> iv;
    for (const OrganNote& n : notes) iv.push_back({n.on, n.off});
    std::sort(iv.begin(), iv.end());
    long reach = iv[0].second;
    for (size_t i = 1; i < iv.size(); ++i) {
      if (iv[i].first - reach >= tpq) partStarts.push_back(iv[i].first);  // (a general pause ends here)
      reach = std::max(reach, iv[i].second);
    }
    std::sort(partStarts.begin(), partStarts.end());
  }
  auto bpmAt = [&](long tick) {
    double b = tempo.empty() ? 100 : tempo[0].second;
    for (auto& [t, v] : tempo)
      if (t <= tick) b = v;
    return b;
  };
  // The tracks: their range, their centre, how many voices at once.
  std::map<int, TrackStats> st;
  for (const OrganNote& n : notes) {
    TrackStats& s = st[n.track];
    s.track = n.track;
    ++s.n;
    s.lo = std::min(s.lo, n.pitch);
    s.hi = std::max(s.hi, n.pitch);
    s.mean += n.pitch;
    s.overPedal += n.pitch > 67;
  }
  for (auto& [t, s] : st) {
    s.mean /= s.n;
    s.overPedal /= s.n;
    std::vector<std::pair<long, int>> ev;
    for (const OrganNote& n : notes)
      if (n.track == t) {
        ev.push_back({n.on, 1});
        ev.push_back({n.off, -1});
      }
    std::sort(ev.begin(), ev.end(), [](auto& a, auto& b) { return a.first != b.first ? a.first < b.first : a.second < b.second; });
    int cur = 0;
    for (auto& e : ev) s.poly = std::max(s.poly, cur += e.second);
  }
  // The pedal: the lowest track, within the compass, one voice (or two),
  // clearly under the rest.
  std::vector<TrackStats> byMean;
  for (auto& [t, s] : st) byMean.push_back(s);
  std::sort(byMean.begin(), byMean.end(), [](auto& a, auto& b) { return a.mean < b.mean; });
  int pedal = -1;
  if (byMean.size() >= 2) {
    const TrackStats& low = byMean[0];
    double others = 0;
    int on = 0;
    for (size_t i = 1; i < byMean.size(); ++i) {
      others += byMean[i].mean * byMean[i].n;
      on += byMean[i].n;
    }
    others /= std::max(1, on);
    if (low.overPedal < 0.05 && low.poly <= 2 && low.lo >= 24 && others - low.mean >= 7) pedal = low.track;
  }
  std::vector<int> upper;
  for (auto it = byMean.rbegin(); it != byMean.rend(); ++it)
    if (it->track != pedal) upper.push_back(it->track);  // (highest first)
  // A trio: two one-voice upper tracks sharing a range.
  bool trio = false;
  if (pedal >= 0 && upper.size() == 2) {
    const TrackStats &a = st[upper[0]], &b = st[upper[1]];
    const int overlap = std::min(a.hi, b.hi) - std::max(a.lo, b.lo);
    trio = a.poly == 1 && b.poly == 1 && overlap >= 0.5 * std::min(a.hi - a.lo, b.hi - b.lo);
  }
  char buf[1024];
  std::snprintf(buf, sizeof buf, "organ plan: %zu tracks; pedal: %s%s\n", st.size(),
                pedal >= 0 ? ("track " + std::to_string(pedal)).c_str() : "none (the lowest key below G3)",
                trio ? "; a trio: the right hand on the Hauptwerk, the left on the Positiv, one registration" : "");
  plan.summary = buf;
  // The kind of piece, from its notes: a cantus firmus (an upper line of long,
  // sparse notes over busier ones), a meditative piece (slow, long notes,
  // sparse: the céleste), a chordal one (the romantic grand choeur), a
  // contrapuntal one (the organo pleno; a long one built up as variations).
  long endAll = 0;
  for (const OrganNote& n : notes) endAll = std::max(endAll, n.off);
  auto medianDur = [&](int t) {
    std::vector<long> d;
    for (const OrganNote& n : notes)
      if (n.track == t) d.push_back(n.off - n.on);
    if (d.empty()) return 0L;
    std::nth_element(d.begin(), d.begin() + d.size() / 2, d.end());
    return d[d.size() / 2];
  };
  int cantus = -1;
  if (!trio && upper.size() >= 2) {
    for (int t : upper) {
      long others = 0;
      int k = 0;
      for (int u : upper)
        if (u != t) others += medianDur(u), ++k;
      if (st[t].poly == 1 && k && medianDur(t) >= 2 * others / k && st[t].n * 3 < int(notes.size())) {
        cantus = t;
        break;
      }
    }
  }
  double chordal = 0;  // (the share of note onsets with three more at the same moment)
  {
    std::map<long, int> at;
    for (const OrganNote& n : notes) ++at[n.on];
    int in = 0;
    for (auto& [t, c] : at) in += c >= 4 ? c : 0;
    chordal = double(in) / notes.size();
  }
  std::vector<long> allDur;
  for (const OrganNote& n : notes) allDur.push_back(n.off - n.on);
  std::nth_element(allDur.begin(), allDur.begin() + allDur.size() / 2, allDur.end());
  const double beatsAll = double(endAll) / tpq, perBeat = notes.size() / std::max(1.0, beatsAll);
  const double bpm0 = bpmAt(0);
  const bool meditative = !trio && cantus < 0 && bpm0 <= 66 && allDur[allDur.size() / 2] >= tpq && perBeat < 3;
  const bool grandChoeur = !trio && cantus < 0 && !meditative && chordal >= 0.5;
  const double minutes = [&] {
    double sec = 0, b = bpmAt(0);
    for (long t = 0; t < endAll; t += tpq) b = bpmAt(t), sec += 60 / b;
    return sec / 60;
  }();
  const bool pleno = !trio && cantus < 0 && !meditative && !grandChoeur && minutes < 6;
  auto fixedPlan = [&](const char* style, const std::string& reg, double swell, std::map<int, int> man) {
    plan.summary += std::string("  ") + style + "\n";
    OrganPlanChange c{0, reg, swell, man, style};
    plan.changes.push_back(c);
  };
  plan.pedalDetached = !meditative && !grandChoeur;
  if (cantus >= 0) {
    std::map<int, int> man;
    for (int t : upper) man[t] = t == cantus ? 1 : 2;
    if (pedal >= 0) man[pedal] = 3;
    std::snprintf(buf, sizeof buf, "a cantus firmus (line %d): the melody on the swell's cornet, the accompaniment on the "
                  "Positiv's flutes; Pedal Subbass 16', Gedacktbass 8'", cantus);
    fixedPlan(buf, "gedackt8,flute4,nasard223,waldflote2,terz135,rohrgedackt8,gedacktflote4,subbass16,gedacktbass8", 1, man);
    return plan;
  }
  if (meditative) {
    std::map<int, int> man;
    for (int t : upper) man[t] = 1;
    if (pedal >= 0) man[pedal] = 3;
    fixedPlan("meditative (slow, long notes): the swell's strings, the Gamba and the Voix celeste, the box half open; "
              "Pedal Subbass 16'", "gamba8,celeste8,subbass16", 0.55, man);
    return plan;
  }
  if (trio) {
    // One registration a section: the right hand on the Hauptwerk, the left
    // on the Positiv; at each structural change (a general pause, a tempo's
    // change: a new section) the left hand to the other of the Positiv and the
    // swell, a little more (a clearly slower section a little less).
    const char* const kLeft[2][3] = {{"rohrgedackt8,gedacktflote4", "rohrgedackt8,posoctave4", "rohrgedackt8,gedacktflote4,sesquialtera"},
                                     {"gedackt8,flute4", "gedackt8,flute4,nasard223", "gedackt8,flute4,waldflote2"}};
    const char* const kRight[3] = {"rohrflote8,octave4", "principal8,rohrflote8", "principal8,octave4"};
    const char* const kBass[3] = {"subbass16,octavbass8", "subbass16,octavbass8,choralbass4", "subbass16,violonbass16,octavbass8,choralbass4"};
    long endT = 0;
    for (const OrganNote& n : notes) endT = std::max(endT, n.off);
    std::vector<long> secs = {0};
    for (long p : partStarts)
      if (p > secs.back() + 8 * barTicks && endT - p >= 8 * barTicks) secs.push_back(p);
    int step = 0;
    for (size_t i = 0; i < secs.size(); ++i) {
      if (i > 0) step = bpmAt(secs[i] + barTicks) < 0.75 * bpmAt(secs[i - 1] + barTicks) ? std::max(0, step - 1) : std::min(2, step + 1);
      const int left = i % 2;  // (0 the Positiv, 1 the swell)
      OrganPlanChange c{secs[i], std::string(kRight[step]) + "," + kLeft[left][step] + "," + kBass[step], 1, {}, ""};
      c.manuals[upper[0]] = 0;
      c.manuals[upper[1]] = left ? 1 : 2;
      c.manuals[pedal] = 3;
      std::snprintf(buf, sizeof buf, "bar %ld: the right hand Hauptwerk %s, the left %s %s; Pedal %s", secs[i] / barTicks + 1,
                    kRight[step], left ? "Schwellwerk" : "Positiv", kLeft[left][step], kBass[step]);
      c.what = buf;
      plan.summary += c.what + "\n";
      plan.changes.push_back(c);
    }
    return plan;
  }
  plan.summary += grandChoeur ? "  the grand choeur (chordal): the fonds coupled, the swell opening, the reeds at the climaxes\n"
                 : pleno ? "  the organo pleno (contrapuntal): the principals and the Mixtur, the pedal's principals and "
                           "Posaune, a step more at each new section\n"
                         : "  building up (a long contrapuntal piece): from mp to fff, phrase by phrase\n";
  // A plenum's texture: an upper line playing chords or several voices at once.
  bool polyphonic = false;
  for (int t : upper) polyphonic |= st[t].poly >= 3;
  // The phrases (8 bars) within the large parts.
  long end = 0;
  for (const OrganNote& n : notes) end = std::max(end, n.off);
  std::vector<long> parts = {0};
  for (long p : partStarts)  // (sections of 8 bars or more on each side, not an ending's ritardando)
    if (p > 0 && end - p >= 8 * barTicks && p - parts.back() >= 8 * barTicks) parts.push_back(p);
  const long phrase = 8 * barTicks;
  struct Phrase {
    long at;
    int part;
    double density;  // upper notes a bar
    int voices;      // tracks sounding
    bool pedal;
  };
  std::vector<Phrase> ph;
  for (size_t p = 0; p < parts.size(); ++p) {
    const long a = parts[p], b = p + 1 < parts.size() ? parts[p + 1] : end;
    for (long t = a; t < b; t += phrase)
      if (b - t >= 4 * barTicks || t == a) ph.push_back({t, int(p), 0, 0, false});  // (a short tail joins the phrase before)
  }
  for (Phrase& f : ph) {
    const long b = f.at + phrase;
    std::set<int> sounding;
    int up = 0, ped = 0;
    for (const OrganNote& n : notes)
      if (n.on >= f.at && n.on < b) {
        sounding.insert(n.track);
        (n.track == pedal ? ped : up)++;
      }
    f.density = up / 8.0;
    f.voices = int(sounding.size());
    f.pedal = ped >= 2;
  }
  // The level: each part building from its start to its end, the texture
  // nudging it; a step at a time upwards; the manualiter phrases a contrast.
  const long total = end;
  int lastLevel = -1;
  int secStart = 3, secPeak = 5, prevPeak = 3;
  std::string lastReg;
  std::map<int, int> lastMan;
  for (size_t p = 0; p < parts.size(); ++p) {
    std::vector<size_t> idx;
    for (size_t i = 0; i < ph.size(); ++i)
      if (ph[i].part == int(p)) idx.push_back(i);
    if (idx.empty()) continue;
    const long len = (p + 1 < parts.size() ? parts[p + 1] : total) - parts[p];
    const bool longPart = len >= 40 * barTicks;
    // Each section a little more than the last; a clearly slower one (an
    // adagio after an allegro) less; the last reaching fff if it is long.
    if (p == 0) {
      secStart = longPart ? 3 : 4;
      secPeak = longPart ? (parts.size() > 1 ? 6 : 7) : 5;
    } else {
      const double ratio = bpmAt(parts[p] + barTicks) / bpmAt(parts[p - 1] + barTicks);
      if (ratio < 0.75) {
        secStart = std::max(1, secStart - 2);
        secPeak = std::min(secStart + 1, 6);
      } else {
        secStart = std::min(5, std::max(secStart, prevPeak - 1) + 1);
        secPeak = std::min(7, std::max(secPeak, secStart + (longPart ? 2 : 1)));
      }
      if (p + 1 == parts.size() && longPart && ratio >= 0.75) secPeak = 7;
    }
    if (pleno) {  // (the organo pleno: f throughout a section, a step more at each new one; a slower one less)
      secStart = p == 0 ? 5 : secStart;
      secPeak = secStart;
    }
    prevPeak = secPeak;
    const int start = secStart, peak = secPeak;
    std::vector<double> dens;
    for (size_t i : idx) dens.push_back(ph[i].density);
    std::vector<double> sorted = dens;
    std::sort(sorted.begin(), sorted.end());
    const double median = sorted[sorted.size() / 2];
    int level = start;
    for (size_t k = 0; k < idx.size(); ++k) {
      const Phrase& f = ph[idx[k]];
      const double pos = idx.size() > 1 ? double(k) / (idx.size() - 1) : 1.0;
      double want = start + (peak - start) * std::pow(pos, 1.2);
      if (f.density > 1.3 * median && f.voices >= 3 && !pleno) want += 0.7;
      if (f.voices <= 1 && pedal >= 0 && !f.pedal) want -= 1;
      int target = int(std::lround(want));
      if (k + 1 == idx.size()) target = peak;
      level = std::min(7, std::clamp(target, level, level + 1));  // (building: a step at a time, never back within a part)
      if (k + 1 == idx.size()) level = std::min(7, std::max(level, peak - 1));
      // The lines on their own manuals (each its colour): the highest on the
      // Hauptwerk, the next on the swell, a third on the Positiv; where the
      // pedal rests (in a piece with one), on the Positiv and the swell, two
      // steps softer (the thinnest phrases a step more).
      // (lines of chords and several voices, a plenum's texture: all on one
      // manual; single lines, a solo's or a trio's: each on its own)
      int shown = level;
      std::vector<int> divs = polyphonic || grandChoeur || pleno ? std::vector<int>{0} : std::vector<int>{0, 1, 2};
      if (pedal >= 0 && !f.pedal && k > 0) {
        shown = std::max(0, level - (f.density < 0.6 * median ? 3 : 2));
        divs = polyphonic || pleno ? std::vector<int>{f.density < 0.6 * median ? 1 : 2} : std::vector<int>{2, 1, 2};
      }
      std::string reg;
      bool used[3] = {false, false, false};
      if (grandChoeur) {  // (the fonds coupled; the swell opening; the reeds at the climaxes)
        static const char* const kGC[8] = {
            "gedackt8,sw-hw", "gedackt8,gamba8,sw-hw", "rohrflote8,gedackt8,gamba8,flute4,sw-hw",
            "principal8,fluteharm8,gedackt8,gamba8,celeste8,flute4,sw-hw",
            "principal8,fluteharm8,octave4,gedackt8,gamba8,flute4,rohrgedackt8,posoctave4,sw-hw,pos-hw",
            "principal16,principal8,fluteharm8,octave4,bourdon16,gedackt8,gamba8,flute4,rohrgedackt8,posoctave4,sw-hw,pos-hw",
            "principal16,principal8,fluteharm8,octave4,bourdon16,gedackt8,gamba8,flute4,fagott16,trompette8,oboe8,clairon4,"
            "rohrgedackt8,posoctave4,sw-hw,pos-hw",
            "principal16,principal8,fluteharm8,octave4,superoctave2,mixtur5,trompete16,trompete8,bourdon16,gedackt8,gamba8,"
            "flute4,fagott16,trompette8,oboe8,clairon4,rohrgedackt8,posoctave4,cromorne8,sw-hw,pos-hw"};
        reg = kGC[shown];
        for (int t : upper) used[0] = true, (void)t;
      }
      std::map<int, int> man;
      for (size_t u = 0; u < upper.size(); ++u) {
        const int d = divs[std::min<size_t>(u, divs.size() - 1)];
        man[upper[u]] = d;
        used[d] = true;
      }
      if (!grandChoeur)
        for (int d = 0; d < 3; ++d)
          if (used[d]) reg += std::string(reg.empty() ? "" : ",") + (d == 0 ? kHW : d == 1 ? kSW : kPos)[shown];
      if (pedal >= 0) {
        reg += std::string(",") + kPed[shown];
        man[pedal] = 3;
      }
      const int hands = man.empty() ? 0 : man.begin()->second;
      if (reg == lastReg && man.size() == 0) continue;
      bool same = reg == lastReg;
      for (auto& [t, d] : man) same &= lastMan.count(t) && lastMan[t] == d;
      if (same) continue;
      lastMan = man;
      OrganPlanChange c{f.at, reg, grandChoeur ? 0.3 + 0.1 * shown : used[1] ? kSwellOf[shown] : 1.0, man, ""};
      std::string lines;
      for (size_t u = 0; u < upper.size(); ++u)
        lines += std::string(u ? ", " : "") + "line " + std::to_string(upper[u]) + " " + kDivName[man[upper[u]]];
      // (every division's stops: the lines' manuals, then the pedal)
      std::string stops;
      if (grandChoeur) {
        stops = std::string("coupled: ") + reg;
      } else {
        for (int d = 0; d < 3; ++d)
          if (used[d]) stops += std::string(stops.empty() ? "" : "; ") + kDivName[d] + " " + (d == 0 ? kHW : d == 1 ? kSW : kPos)[shown];
        if (pedal >= 0) stops += std::string("; Pedal ") + kPed[shown] + (!f.pedal ? " (resting)" : "");
      }
      std::snprintf(buf, sizeof buf, "bar %ld: %s: %s | %s", f.at / barTicks + 1, kLevelName[shown], lines.c_str(), stops.c_str());
      c.what = buf;
      plan.summary += c.what + "\n";
      plan.changes.push_back(c);
      lastReg = reg;
      lastLevel = level;
      (void)hands;
    }
  }
  (void)lastLevel;
  return plan;
}

}  // namespace pajupilli
