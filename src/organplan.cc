#include "organplan.h"

#include "knob.h"
#include "organ.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>

namespace pajupilli {

namespace {

const char* const kLevelName[8] = {"ppp", "pp", "p", "mp", "mf", "f", "ff", "fff"};

// Each division's ladder: its stops at each level (0 ppp .. 7 fff), from its
// own stops only (the user's disposition, research/church_organ.md §6) and no
// couplers between the manuals: each line its own colour and character.
const char* const kMainHW[8] = {
    "rohrflote8", "rohrflote8", "fluteharm8", "principal8,rohrflote8", "principal8,fluteharm8,octave4",
    "principal8,octave4,superoctave2,mixtur5",
    "principal16,principal8,octave4,quinte223,superoctave2,mixtur5,cymbel3,trompete8",
    "principal16,principal8,fluteharm8,rohrflote8,octave4,quinte223,superoctave2,mixtur5,cymbel3,trompete16,trompete8"};
const char* const kMainSW[8] = {
    "gedackt8", "gamba8,celeste8", "gedackt8,flute4", "gedackt8,flute4,nasard223", "gedackt8,gamba8,flute4,waldflote2",
    "gedackt8,flute4,nasard223,waldflote2,terz135",
    "bourdon16,gedackt8,flute4,waldflote2,trompette8,oboe8",
    "bourdon16,gedackt8,flute4,waldflote2,fagott16,trompette8,oboe8,clairon4"};
const double kSwellOf[8] = {0.25, 0.5, 0.8, 1, 1, 1, 1, 1};  // (the swell box's opening at each level)
const char* const kMainPos[8] = {
    "rohrgedackt8", "rohrgedackt8", "rohrgedackt8,gedacktflote4", "rohrgedackt8,posoctave4",
    "rohrgedackt8,posoctave4,gedacktflote4", "rohrgedackt8,gedacktflote4,sesquialtera",
    "rohrgedackt8,posoctave4,cromorne8", "rohrgedackt8,posoctave4,gedacktflote4,sesquialtera,cromorne8"};
const char* const kMainPed[8] = {
    "subbass16", "subbass16,gedacktbass8", "subbass16,gedacktbass8", "subbass16,octavbass8", "subbass16,violonbass16,octavbass8,choralbass4",
    "principalbass16,octavbass8,choralbass4,posaune16",  // (f: the organo pleno's pedal)
    "principalbass16,subbass16,violonbass16,octavbass8,choralbass4,pedmixtur4,posaune16",
    "principalbass16,subbass16,violonbass16,octavbass8,choralbass4,pedmixtur4,posaune16,pedtrompete8,untersatz32"};
// After Janakkala (Organ.md): the same eight levels from its own stops; the Pedal's reeds from f.
const char* const kJanHW[8] = {
    "j_hw_rohrflot8", "j_hw_rohrflot8", "j_hw_rohrflot8,j_hw_flot4", "j_hw_principal8,j_hw_rohrflot8",
    "j_hw_principal8,j_hw_octav4,j_hw_flot4", "j_hw_principal8,j_hw_octav4,j_hw_octav2,j_hw_mixtur5",
    "j_hw_quintadena16,j_hw_principal8,j_hw_octav4,j_hw_nassat3,j_hw_octav2,j_hw_mixtur5,j_hw_cimbel3,j_hw_trommet8",
    "j_hw_quintadena16,j_hw_principal8,j_hw_rohrflot8,j_hw_octav4,j_hw_nassat3,j_hw_octav2,j_hw_mixtur5,j_hw_cimbel3,j_hw_trommet16,j_hw_trommet8"};
const char* const kJanSW[8] = {
    "j_bw_gedackt8", "j_bw_querflot8", "j_bw_gedackt8,j_bw_gedacktflot4", "j_bw_querflot8,j_bw_gedacktflot4",
    "j_bw_gedackt8,j_bw_gedacktflot4,j_bw_octav2", "j_bw_gedackt8,j_bw_gedacktflot4,j_bw_octav2,j_bw_tertia",
    "j_bw_gedackt8,j_bw_gedacktflot4,j_bw_octav2,j_bw_sedetz1,j_bw_scharff3,j_bw_krumphorn8",
    "j_bw_gedackt8,j_bw_gedacktflot4,j_bw_octav2,j_bw_sedetz1,j_bw_scharff3,j_bw_krumphorn8,j_bw_schalmey4"};
const char* const kJanPos[8] = {
    "j_rp_gedackt8", "j_rp_quintadena8", "j_rp_gedackt8,j_rp_rohrflot4", "j_rp_principal8,j_rp_gedackt8",
    "j_rp_principal8,j_rp_octav4,j_rp_rohrflot4", "j_rp_gedackt8,j_rp_rohrflot4,j_rp_sesquialter",
    "j_rp_principal8,j_rp_octav4,j_rp_waltflot2,j_rp_scharff4,j_rp_trechterregal8",
    "j_rp_principal8,j_rp_gedackt8,j_rp_octav4,j_rp_rohrflot4,j_rp_waltflot2,j_rp_sieflot113,j_rp_sesquialter,j_rp_scharff4,j_rp_dulcian16,j_rp_trechterregal8"};
const char* const kJanPed[8] = {
    "j_p_subbass16", "j_p_subbass16", "j_p_subbass16,j_p_octav8", "j_p_subbass16,j_p_octav8", "j_p_subbass16,j_p_octav8,j_p_octav4",
    "j_p_principal16,j_p_octav8,j_p_octav4,j_p_posaun16", "j_p_principal16,j_p_subbass16,j_p_octav8,j_p_octav4,j_p_mixtur4,j_p_posaun16",
    "j_p_principal16,j_p_subbass16,j_p_octav8,j_p_octav4,j_p_nachthorn2,j_p_mixtur4,j_p_posaun16,j_p_dulcian16,j_p_trommet8"};
const char* const* ladderHW() { return organDisposition() == 1 ? kJanHW : kMainHW; }
const char* const* ladderSW() { return organDisposition() == 1 ? kJanSW : kMainSW; }
const char* const* ladderPos() { return organDisposition() == 1 ? kJanPos : kMainPos; }
const char* const* ladderPed() { return organDisposition() == 1 ? kJanPed : kMainPed; }
const char* divName(int d) {
  static const char* const kMain[4] = {"Hauptwerk", "Schwellwerk", "Positiv", "Pedal"}, *const kJan[4] = {"Hauptwerk", "Brustwerk", "Rückpositiv", "Pedal"};
  return (organDisposition() == 1 ? kJan : kMain)[d];
}

struct TrackStats {
  int track = 0, n = 0, lo = 127, hi = 0, poly = 0;
  double mean = 0, overPedal = 0;  // (the share of its notes above the pedal's compass)
  double twoShare = 0;             // (the share of its time with two notes or more held)
};

}  // namespace

std::string organRegistration(int level, int handsOn, bool pedal) {
  level = std::clamp(level, 0, 7);
  std::string r = handsOn == 2 ? ladderPos()[level] : handsOn == 1 ? ladderSW()[level] : ladderHW()[level];
  if (pedal) r += std::string(",") + ladderPed()[level];
  return r;
}

ZoneRegistration organZoneRegistration(int level) {
  level = std::clamp(level, 0, 7);
  ZoneRegistration z;
  z.rightHand = level <= 2 ? 2 : 0;
  z.stops = std::string(z.rightHand == 2 ? ladderPos()[level] : ladderHW()[level]) + "," + ladderSW()[level] + "," + ladderPed()[level];
  z.swell = kSwellOf[level];
  return z;
}

namespace {

// The ticks covered by a set of notes: merged spans.
std::vector<std::pair<long, long>> spansOf(std::vector<std::pair<long, long>> iv) {
  std::sort(iv.begin(), iv.end());
  std::vector<std::pair<long, long>> out;
  for (auto& s : iv) {
    if (s.second <= s.first) continue;
    if (!out.empty() && s.first <= out.back().second) out.back().second = std::max(out.back().second, s.second);
    else out.push_back(s);
  }
  return out;
}

long lengthOf(const std::vector<std::pair<long, long>>& spans, long a = 0, long b = -1) {
  long sum = 0;
  for (auto& s : spans) {
    const long x = std::max(a, s.first), y = b < 0 ? s.second : std::min(b, s.second);
    if (y > x) sum += y - x;
  }
  return sum;
}

// The continuo's plan (src/organplan.h).
OrganPlan planContinuo(const std::vector<OrganNote>& notes, int tpq, long barTicks, const OrganCompany& company) {
  OrganPlan plan;
  plan.pedalDetached = true;
  const bool jan = organDisposition() == 1;
  const bool positive = company.stage != "church" && company.stage != "concert";
  // The organ's pedal line, if it has its own track: the lowest, one voice, within the compass.
  std::map<int, TrackStats> st;
  for (const OrganNote& n : notes) {
    TrackStats& s = st[n.track];
    s.track = n.track;
    ++s.n;
    s.mean += n.pitch;
    s.overPedal += n.pitch > 67;
    s.lo = std::min(s.lo, n.pitch);
  }
  int pedal = -1;
  if (st.size() >= 2) {
    const TrackStats* low = nullptr;
    for (auto& [t, s] : st) {
      s.mean /= s.n;
      s.overPedal /= s.n;
      if (!low || s.mean < low->mean) low = &s;
    }
    std::vector<std::pair<long, long>> iv;
    long sum = 0;
    for (const OrganNote& n : notes)
      if (n.track == low->track) iv.push_back({n.on, n.off}), sum += n.off - n.on;
    const bool oneVoice = sum < 1.15 * lengthOf(spansOf(iv));
    if (low->overPedal < 0.05 && low->lo >= 24 && oneVoice) pedal = low->track;
  }
  // The lines around it, and the general pauses (everyone silent a beat: a movement's end).
  std::map<int, std::vector<std::pair<long, long>>> lines;  // line -> its spans
  std::map<int, int> kindOf;
  std::vector<std::pair<long, long>> all;
  long end = 0;
  for (const OrganCompanyNote& n : company.notes) {
    lines[n.line].push_back({n.on, n.off});
    kindOf[n.line] = n.kind;
    all.push_back({n.on, n.off});
    end = std::max(end, n.off);
  }
  for (const OrganNote& n : notes) all.push_back({n.on, n.off}), end = std::max(end, n.off);
  for (auto& [l, iv] : lines) iv = spansOf(iv);
  all = spansOf(all);
  std::set<long> pauseEnds;
  for (size_t i = 1; i < all.size(); ++i)
    if (all[i].first - all[i - 1].second >= tpq) pauseEnds.insert(all[i].first);
  // Windows of two bars (a movement's first beginning where its pause ends): who plays in each.
  struct Win {
    long at, to;
    bool fresh;  // (after a general pause)
    double sounding = 0;  // the others' lines sounding, on average
    int choir = 0;        // the choir's lines singing
    bool festive = false, organ = false;
    int level = 0;
    bool ped = false;
  };
  std::vector<Win> win;
  {
    std::vector<long> cuts = {0};
    for (long p : pauseEnds) cuts.push_back(p);
    cuts.push_back(end);
    for (size_t c = 0; c + 1 < cuts.size(); ++c)
      for (long t = cuts[c]; t < cuts[c + 1]; t += 2 * barTicks) {
        const long to = std::min(t + 2 * barTicks, cuts[c + 1]);
        if (to - t < barTicks && t > cuts[c]) {  // (a short tail joins the window before)
          win.back().to = to;
          break;
        }
        win.push_back({t, to, t == cuts[c] && c > 0});
      }
  }
  std::vector<std::pair<long, long>> organSpans;
  for (const OrganNote& n : notes) organSpans.push_back({n.on, n.off});
  organSpans = spansOf(organSpans);
  for (Win& w : win) {
    const double len = double(std::max(1L, w.to - w.at));
    double brass = 0;
    bool timp = false;
    for (auto& [l, iv] : lines) {
      const double share = lengthOf(iv, w.at, w.to) / len;
      w.sounding += share;
      if (kindOf[l] == 1) brass += share;
      if (kindOf[l] == 2 && share > 0.02) timp = true;
      if (kindOf[l] == 3 && share > 0.3) ++w.choir;
    }
    w.organ = lengthOf(organSpans, w.at, w.to) > 0;
    const bool full = w.sounding >= 8 || w.choir >= 3;
    w.festive = (timp || brass >= 1.5) && w.sounding >= 6;  // (two trumpets: one "brass" line may be a soloist on a trombone's program)
    w.level = w.festive ? 2 : w.sounding >= 4.5 || full ? 1 : 0;
    w.ped = w.festive || full;
  }
  // A change inside a movement must last 8 bars (four windows): first the middle of each five
  // windows (a bar's rest in the trumpets is not a new registration), then the shortest run left
  // joins its longer neighbour until none is shorter.
  if (std::getenv("PJ_ORGAN_CONTINUO_TRACE")) {
    for (const Win& w : win) std::fprintf(stderr, "%s%d%s", w.fresh ? " | " : "", w.level, w.ped ? "p" : "");
    std::fprintf(stderr, "\n");
  }
  for (size_t a = 0; a < win.size();) {
    size_t b = a + 1;
    while (b < win.size() && !win[b].fresh) ++b;
    for (int pass = 0; pass < 2; ++pass) {
      std::vector<int> v;
      for (size_t k = a; k < b; ++k) v.push_back(pass == 0 ? win[k].level : int(win[k].ped));
      const int n = int(v.size());
      std::vector<int> m = v;
      for (int k = 0; k < n; ++k) {
        int five[5];
        for (int d = -2; d <= 2; ++d) five[d + 2] = v[std::clamp(k + d, 0, n - 1)];
        std::sort(five, five + 5);
        m[k] = five[2];
      }
      for (;;) {
        int best = -1, bestLen = 4, bestEnd = 0;
        for (int k = 0; k < n;) {
          int e = k + 1;
          while (e < n && m[e] == m[k]) ++e;
          if (e - k < bestLen && e - k < n) best = k, bestLen = e - k, bestEnd = e;
          k = e;
        }
        if (best < 0) break;
        auto runLen = [&](int at, int dir) {
          int len = 0;
          for (int k = at; k >= 0 && k < n && m[k] == m[at]; k += dir) ++len;
          return len;
        };
        const int left = best > 0 ? runLen(best - 1, -1) : 0, right = bestEnd < n ? runLen(bestEnd, 1) : 0;
        const int to = left >= right ? m[best - 1] : m[bestEnd];
        for (int k = best; k < bestEnd; ++k) m[k] = to;
      }
      // The movement's own registration: what its fuller passages ask (a chorale's lines come and go
      // with the choir, the trumpets rest: the organist does not follow them); another only where it
      // lasts 16 bars.
      std::vector<int> sorted = v;
      std::sort(sorted.begin(), sorted.end());
      int base = sorted[int(0.7 * (n - 1))];
      if (pass == 0 && base == 1) {  // (a movement with trumpets and timpani in a tenth of it or more: festive)
        int fest = 0;
        for (size_t k = a; k < b; ++k) fest += win[k].festive;
        if (fest >= 0.1 * n) {
          base = 2;
          for (int& x : m) x = x == 1 ? 2 : x;
        }
      }
      for (int k = 0; k < n;) {
        int e = k + 1;
        while (e < n && m[e] == m[k]) ++e;
        if (m[k] != base && e - k < 8)
          for (int q = k; q < e; ++q) m[q] = base;
        k = e;
      }
      for (size_t k = a; k < b; ++k) (pass == 0 ? win[k].level = m[k - a] : win[k].ped = m[k - a] != 0);
    }
    a = b;
  }
  for (Win& w : win) w.ped = w.ped && w.level >= 1;
  // The climax: a festive ending's last bars, a little more.
  if (!positive && !win.empty() && win.back().level == 2) {
    size_t i = win.size();
    while (i > 0 && win[i - 1].level == 2 && (i == win.size() || !win[i].fresh)) --i;
    const size_t run = win.size() - i, take = std::min<size_t>(4, run / 3);
    for (size_t k = win.size() - take; k < win.size(); ++k) win[k].level = 3;
  }
  static const char* const kMainHands[4] = {"rohrgedackt8", "rohrgedackt8,gedacktflote4", "rohrgedackt8,gedacktflote4,waldflote2,sw-pos",
                                            "rohrgedackt8,gedacktflote4,waldflote2,sw-pos,principal8,hw-pos"};
  static const char* const kJanHands[4] = {"j_rp_gedackt8", "j_rp_gedackt8,j_rp_rohrflot4", "j_rp_gedackt8,j_rp_rohrflot4,j_rp_waltflot2",
                                           "j_rp_principal8,j_rp_gedackt8,j_rp_rohrflot4,j_rp_waltflot2"};
  static const char* const kMainPed[4] = {"", "subbass16", "subbass16,gedacktbass8", "subbass16,gedacktbass8"};
  static const char* const kJanPed[4] = {"", "j_p_subbass16", "j_p_subbass16,j_p_octav8", "j_p_subbass16,j_p_octav8"};
  static const char* const kWhat[4] = {"a quiet aria or a recitative: the Gedackt 8' alone", "the 8' and the Flute 4'",
                                       "a festive tutti: the 8', the 4' and a flute 2'", "the climax: a light Principal 8' more"};
  char buf[512];
  std::snprintf(buf, sizeof buf, "organ plan: a continuo under %zu other lines, on %s (the stage: %s)%s\n", lines.size(),
                positive ? "a positive organ: Gedackt 8', Flute 4', no pedal" : "the church organ's Positiv, registered lightly",
                company.stage.c_str(), pedal >= 0 ? "; its bass line on the pedal where the Subbass is drawn" : "");
  plan.summary = buf;
  int lastLevel = -1, lastPed = -1;
  for (size_t wi = 0; wi < win.size(); ++wi) {
    const Win& w = win[wi];
    const int level = positive ? std::min(1, w.level) : w.level;
    const bool ped = !positive && w.ped;
    if (!w.organ && lastLevel >= 0) continue;  // (the organ resting: nothing to change)
    if (level == lastLevel && int(ped) == lastPed) continue;
    lastLevel = level;
    lastPed = ped;
    std::string reg = (jan ? kJanHands : kMainHands)[level];
    if (ped) reg += std::string(",") + (jan ? kJanPed : kMainPed)[level];
    OrganPlanChange c{w.at, reg, level >= 2 ? 0.6 : 1.0, {}, ""};
    for (auto& [t, s] : st) c.manuals[t] = t == pedal && ped ? 3 : 2;
    double around = 0;  // (over the bars this registration stays)
    int nw = 0, choir = 0;
    bool festive = false;
    for (size_t k = wi; k < win.size() && (k == wi || (win[k].level == w.level && win[k].ped == w.ped && !win[k].fresh)); ++k)
      around += win[k].sounding, ++nw, choir = std::max(choir, win[k].choir), festive |= win[k].festive;
    around /= nw;
    std::snprintf(buf, sizeof buf, "bar %ld: %s%s (%.1f lines around it%s%s) | %s", w.at / barTicks + 1, kWhat[level],
                  ped ? (pedal >= 0 ? "; the bass on the pedal's Subbass 16'" : "; the Subbass 16' under the bass") : "",
                  around, choir >= 3 ? ", the choir" : "", festive ? ", trumpets or timpani" : "", reg.c_str());
    c.what = buf;
    plan.summary += c.what + "\n";
    plan.changes.push_back(c);
  }
  if (!plan.changes.empty()) plan.changes[0].tick = 0;
  return plan;
}

}  // namespace

bool organIsContinuo(const std::vector<OrganNote>& organ, const OrganCompany& company) {
  static const double kForce = knob("PJ_ORGAN_CONTINUO", -1);
  if (kForce >= 0) return kForce > 0 && !organ.empty();
  if (organ.empty() || company.notes.empty()) return false;
  std::vector<std::pair<long, long>> o, c;
  for (const OrganNote& n : organ) o.push_back({n.on, n.off});
  // (the others' articulation is not silence: each note reaches a typical note's length past its end)
  std::vector<long> dur;
  for (const OrganCompanyNote& n : company.notes) dur.push_back(n.off - n.on);
  std::nth_element(dur.begin(), dur.begin() + dur.size() / 2, dur.end());
  const long reach = dur[dur.size() / 2];
  for (const OrganCompanyNote& n : company.notes) c.push_back({n.on, n.off + reach});
  o = spansOf(o);
  c = spansOf(c);
  long with = 0;  // (the organ's ticks with another line sounding)
  size_t j = 0;
  for (auto& s : o) {
    while (j < c.size() && c[j].second <= s.first) ++j;
    for (size_t k = j; k < c.size() && c[k].first < s.second; ++k) with += std::min(s.second, c[k].second) - std::max(s.first, c[k].first);
  }
  return with >= 0.85 * lengthOf(o);
}

bool organCompanyIsChamber(const OrganCompany& company) {
  std::set<int> lines;
  for (const OrganCompanyNote& n : company.notes) {
    if (n.kind != 0) return false;
    lines.insert(n.line);
  }
  return !lines.empty() && lines.size() <= 6;
}

OrganPlan planOrgan(const std::vector<OrganNote>& notes, int tpq, long barTicks, const std::vector<long>& partStartsIn,
                    const std::vector<std::pair<long, double>>& tempo, const OrganCompany* company) {
  OrganPlan plan;
  if (notes.empty()) return plan;
  if (company && organIsContinuo(notes, *company)) return planContinuo(notes, tpq, barTicks, *company);
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
  plan.partStarts = partStarts;
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
    long multi = 0, last = ev.empty() ? 0 : ev.front().first;
    for (auto& e : ev) {
      if (cur >= 2) multi += e.first - last;
      last = e.first;
      s.poly = std::max(s.poly, cur += e.second);
    }
    const long span = ev.empty() ? 1 : std::max(1L, ev.back().first - ev.front().first);
    s.twoShare = double(multi) / span;
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
  // A trio: two one-voice upper tracks sharing a range. (One voice: a rare double note, a held
  // note under the next, under 2 % of its time, does not make it two: BWV 530's left hand holds two
  // for one note in 910, and the trio sonata went to the organo pleno, its fast pedal line on the
  // Principal 16' and the Posaune.)
  bool trio = false;
  if (pedal >= 0 && upper.size() == 2) {
    const TrackStats &a = st[upper[0]], &b = st[upper[1]];
    const int overlap = std::min(a.hi, b.hi) - std::max(a.lo, b.lo);
    auto oneVoice = [](const TrackStats& t) { return t.poly == 1 || (t.poly == 2 && t.twoShare < 0.02); };
    trio = oneVoice(a) && oneVoice(b) && overlap >= 0.5 * std::min(a.hi - a.lo, b.hi - b.lo);
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
          if (used[d]) reg += std::string(reg.empty() ? "" : ",") + (d == 0 ? ladderHW() : d == 1 ? ladderSW() : ladderPos())[shown];
      if (pedal >= 0) {
        reg += std::string(",") + ladderPed()[shown];
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
        lines += std::string(u ? ", " : "") + "line " + std::to_string(upper[u]) + " " + divName(man[upper[u]]);
      // (every division's stops: the lines' manuals, then the pedal)
      std::string stops;
      if (grandChoeur) {
        stops = std::string("coupled: ") + reg;
      } else {
        for (int d = 0; d < 3; ++d)
          if (used[d]) stops += std::string(stops.empty() ? "" : "; ") + divName(d) + " " + (d == 0 ? ladderHW() : d == 1 ? ladderSW() : ladderPos())[shown];
        if (pedal >= 0) stops += std::string("; Pedal ") + ladderPed()[shown] + (!f.pedal ? " (resting)" : "");
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
