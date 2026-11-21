#include "drumkit.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "drumcal_data.h"
#include "knob.h"

namespace pajupilli {

bool brushKitWanted() {
  const char* k = std::getenv("PJ_KIT");
  return k && std::strcmp(k, "brush") == 0;
}

Drum drumFor(int k, Kit kit, bool gs) {
  const bool brush = kit == Kit::kBrush;
  // (The brush player's own swirl on a cymbal: -play's keys, not General MIDI's.)
  if (kit != Kit::kStandard) {
    if (k == kKeyRightHandAway) return {"brush-snare", 42};
    if (k == kKeySwirlRide) return {"ride-cymbal", 84};
    if (k == kKeySwirlCrash2) return {"crash-cymbal-2", 84};
    if (k == kKeySwirlCrash) return {"crash-cymbal", 84};
  }
  // The usual snare is played with sticks (src/sticksnare.cc: the same drum
  // under sticks, with its rim shot and roll). A jazz group's (Kit::kJazz: a
  // file with a piano and an upright bass) and a Brush kit's is the brushed
  // snare.
  const bool sticks = kit == Kit::kStandard;
  if (sticks) {
    if (k == 37) return {"stick-snare", 37};                  // the cross-stick
    if (k == 38 || k == 40) return {"stick-snare", k};
    if (!gs && k == 34) return {"stick-snare", 41};           // XG open rim shot
    if (!gs && k == 29) return {"stick-snare", 36};           // XG snare roll (held)
    if (gs && k == 25) return {"stick-snare", 36};            // GS / GM2 snare roll (held)
    if (!gs && k == 31) return {"stick-snare", 38};           // XG low snare
  }
  // Otherwise the snare is the brushed snare. The Brush kit: 38 a tap, 39 a
  // slap, 40 the swirl, held. The Jazz kit, where 40 is a second snare and
  // not a held sound: 38 a tap, 40 a tap nearer the edge, 39 (the clap) a
  // slap. 37: the cross-stick, on the same drum.
  if (k == 39 && kit != Kit::kBrush) return {"hand-clap", 39};  // (the Brush kit's 39 is the slap)
  if (k == 37) return {"brush-snare", 37};
  if (k >= 38 && k <= 40) return {"brush-snare", brush || k != 40 ? k : 41};
  // (PJ_HAT_KITBRUSH=1: the Brush kit strikes it with the brush, the hi-hat's keys 43 and 47: 15-25 dB
  // under the stick as the model stands, so not the default; the Brush kit's stick is a light one: drumVelocity)
  static const bool kHatBrush = knob("PJ_HAT_KITBRUSH", 0) > 0;
  if (k == 42 || k == 44 || k == 46) return {"hi-hat", brush && kHatBrush && k != 44 ? k + 1 : k};  // closed, the foot, open
  // Below 35 General MIDI 1 defines nothing. Files that play there are
  // Yamaha XG's (its brush strokes and extra snares live there) unless they
  // say they are Roland GS or General MIDI 2 (clicks, scratches, sticks).
  if (k < 35) {
    if (gs) {
      if (k == 25) return {"brush-snare", 38};                 // snare roll (until one is built: a tap)
      if (k >= 26 && k <= 34) return {"perc-extras", k};
      return {nullptr, -1};
    }
    switch (k) {
      case 15: return {"perc-extras", 27};
      case 16: return {"perc-extras", 28};
      case 17: return {"perc-extras", 29};
      case 18: return {"perc-extras", 30};
      case 19: return {"perc-extras", 26};
      case 20: return {"perc-extras", 20};
      case 21: return {"perc-extras", 33};
      case 22: return {"perc-extras", 34};
      case 23: return {"perc-extras", 23};
      case 24: return {"perc-extras", 24};
      case 30: return {"perc-extras", 21};                     // castanets
      case 32: return {"perc-extras", 31};                     // sticks
      case 25: return {"brush-snare", 38};                     // brush tap
      case 26: return {"brush-snare", 40};                     // brush swirl
      case 27: return {"brush-snare", 39};                     // brush slap
      case 28: return {"brush-snare", 40};                     // brush swirl, the lighter one
      case 29: return {"brush-snare", 38};                     // snare roll (until one is built)
      case 31: return {"brush-snare", 38};                     // low snare
      case 34: return {"brush-snare", 37};                     // open rim shot (until one is built: the cross-stick)
      case 33: return {"jazz-kick-20", 36};                    // low bass drum
      case 13: return {"surdo", 86};                           // surdo, mute
      case 14: return {"surdo", 87};                           // surdo, open
      default: return {nullptr, -1};
    }
  }
  if (k == 85) return {"perc-extras", 21};                     // castanets (General MIDI 2)
  switch (k) {
    case 35: case 36: return {"jazz-kick", k};                                // bass drum (35: lower on the head, 36: nearer its middle)
    case 75: return {"claves", 75};
    case 76: case 77: return {"wood-blocks", k};                             // high, low
    case 84: return {"bell-tree", 84};                                       // General MIDI 2
    case 86: case 87: return {"surdo", k};                                   // General MIDI 2: mute, open
    // The toms (src/tom.cc): six drums; a key of 60 up there is a brush stroke, below a stick.
    case 41: return {"tom-16-low", brush ? 65 : 41};  // low floor tom
    case 43: return {"tom-16", brush ? 67 : 43};      // high floor tom
    case 45: return {"tom-14", brush ? 69 : 45};      // low tom
    case 47: return {"tom-13", brush ? 71 : 47};      // low-mid tom
    case 48: return {"tom-12", brush ? 72 : 48};      // hi-mid tom
    case 50: return {"tom-10", brush ? 74 : 50};      // high tom
    case 56: return {"cowbell", 56};                                          // cowbell
    case 67: case 68: return {"agogo", k == 68 ? 83 : 90};                    // agogo, low / high
    case 60: case 61: return {"bongo", k};                                   // bongos: macho, hembra
    case 62: case 63: case 64: return {"conga", k};                          // mute hi, open hi, low conga
    case 65: case 66: return {"timbale", k};                                 // high, low timbal
    case 58: return {"vibraslap", 58};
    case 71: case 72: return {"pea-whistle", k};                             // 71 short, 72 long
    case 73: case 74: return {"guiro", k};                                   // 73 short, 74 long
    case 78: case 79: return {"cuica", k};                                   // 78 mute, 79 open
    case 80: case 81: return {"triangle", k};                                // mute, open triangle
    // The cymbals (src/cymbal.cc): a brushed stroke is the same key + 24.
    case 51: return {"ride-cymbal", brush ? 75 : 51};                        // ride: the bow
    case 53: return {"ride-cymbal", brush ? 77 : 53};                        // the same ride's bell
    case 59: return {"ride-cymbal-2", brush ? 83 : 59};                      // the second ride
    case 49: return {"crash-cymbal", brush ? 73 : 49};                       // 16" crash
    case 57: return {"crash-cymbal-2", brush ? 81 : 57};                     // 18" crash
    case 55: return {"splash-cymbal", brush ? 79 : 55};                      // 10" splash
    case 52: return {"china-cymbal", brush ? 76 : 52};                       // 18" china
    case 54: return {"tambourine", 54};
    case 69: return {"cabasa", 69};                                          // (twists while held)
    case 70: return {"maracas", 70};
    case 82: return {"shaker", 82};                                          // General MIDI 2
    case 83: return {"jingle-bell", 83};                                     // General MIDI 2
    default: return {nullptr, -1};                                           // not built
  }
}

const char* drumName(int k) {
  static const char* const kNames[47] = {
      "bass drum 2", "bass drum 1", "side stick", "snare 1", "hand clap", "snare 2", "low tom 2", "closed hi-hat",
      "low tom 1", "pedal hi-hat", "mid tom 2", "open hi-hat", "mid tom 1", "high tom 2", "crash 1", "high tom 1",
      "ride 1", "china", "ride bell", "tambourine", "splash", "cowbell", "crash 2", "vibraslap", "ride 2",
      "high bongo", "low bongo", "mute high conga", "open high conga", "low conga", "high timbale", "low timbale",
      "high agogo", "low agogo", "cabasa", "maracas", "short whistle", "long whistle", "short guiro", "long guiro",
      "claves", "high wood block", "low wood block", "mute cuica", "open cuica", "mute triangle", "open triangle"};
  if (k == 82) return "shaker";
  if (k == 83) return "jingle bell";
  if (k == 84) return "bell tree";
  if (k == 85) return "castanets";
  if (k == 86) return "mute surdo";
  if (k == 87) return "open surdo";
  return k >= 35 && k <= 81 ? kNames[k - 35] : "";
}

std::vector<CymbalSwirl> planCymbalSwirls(const std::vector<DrumPlanNote>& notes, const std::vector<DrumTempo>& tempo, bool gs) {
  std::vector<CymbalSwirl> out;
  if (knob("PJ_SWIRL", 1) <= 0 || notes.empty()) return out;
  // The tempo map both ways (none: 120 to the minute).
  auto beatAt = [&](double t) {
    const DrumTempo* m = nullptr;
    for (const DrumTempo& x : tempo)
      if (x.t <= t) m = &x;
    return m ? m->beat + (t - m->t) / m->len : t / 0.5;
  };
  auto timeAt = [&](double b) {
    const DrumTempo* m = nullptr;
    for (const DrumTempo& x : tempo)
      if (x.beat <= b) m = &x;
    return m ? m->t + (b - m->beat) * m->len : b * 0.5;
  };
  // Bar by bar (four beats, from the song's start): how many notes, how
  // loud, the brushed snare's strokes; and when each cymbal is struck.
  const int nBars = int(beatAt(notes.back().t) / 4) + 1;
  if (nBars < 8) return out;
  static const char* const kCym[3] = {"ride-cymbal", "crash-cymbal-2", "crash-cymbal"};
  static const int kKey[3] = {kKeySwirlRide, kKeySwirlCrash2, kKeySwirlCrash};
  std::vector<int> count(nBars, 0), drumN(nBars, 0);
  std::vector<double> vel(nBars, 0), drumVel(nBars, 0);
  std::vector<double> struck[3];
  int first = -1;
  uint32_t seed = 2166136261u;  // (FNV-1a over the notes: the same file, the same swirls)
  for (const DrumPlanNote& n : notes) {
    const int bar = std::clamp(int(beatAt(n.t) / 4), 0, nBars - 1);
    ++count[bar];
    vel[bar] += n.velocity;
    for (uint32_t v : {uint32_t(std::lround(n.t * 1000)), uint32_t(n.key), uint32_t(n.velocity), uint32_t(n.drum)}) {
      seed ^= v;
      seed *= 16777619u;
    }
    if (!n.drum) continue;
    ++drumN[bar];
    drumVel[bar] += n.velocity;
    const Drum d = drumFor(n.key, n.kit, gs);
    if (!d.inst) continue;
    if (n.kit != Kit::kStandard && std::strcmp(d.inst, "brush-snare") == 0 && first < 0) first = bar;
    for (int c = 0; c < 3; ++c)
      if (std::strcmp(d.inst, kCym[c]) == 0) struck[c].push_back(n.t);
  }
  if (first < 0) return out;  // (no brushed snare)
  uint32_t rng = seed;
  auto ur = [&]() {
    rng = rng * 1664525u + 1013904223u;
    return (rng >> 8) * (1.0 / 16777216.0);
  };
  // A ballad (a slow beat: 75 to the minute or slower): longer swirls.
  std::vector<double> lens;
  for (const DrumTempo& x : tempo) lens.push_back(x.len);
  std::sort(lens.begin(), lens.end());
  const double beatLen = lens.empty() ? 0.5 : lens[lens.size() / 2];
  const bool ballad = beatLen >= 0.8;
  // How quiet and how sparse each bar is (0 .. 1): the band's mean
  // velocity, and its notes a beat.
  std::vector<double> calm(nBars, 0);
  for (int b = 0; b < nBars; ++b) {
    if (count[b] == 0) continue;
    const double quiet = std::clamp((105 - vel[b] / count[b]) / 60, 0.0, 1.0);
    const double sparse = 1 / (1 + count[b] / 4.0 / 3);
    calm[b] = std::sqrt(quiet * sparse);
  }
  auto busy = [&](int b0, int b1) {  // notes a beat over bars b0 .. b1 - 1, and their mean velocity
    int n = 0;
    double v = 0;
    for (int b = std::max(b0, 0); b < std::min(b1, nBars); ++b) { n += count[b]; v += vel[b]; }
    return std::make_pair(n / (4.0 * std::max(1, b1 - b0)), n ? v / n : 0.0);
  };
  const int last = nBars - 1;
  struct Cand { double score; int s, e, c; };
  std::vector<Cand> cand;
  for (int e = first + 3; e <= last + 1; ++e) {
    // Where it ends: a phrase's end (every fourth bar), a section's (the
    // eighth, or where the band changes: busier or sparser, louder or
    // softer after it), the song's end.
    double end = e % 8 == 0 ? 0.3 : e % 4 == 0 ? 0.15 : e % 2 == 0 ? 0.0 : -0.2;
    if (e <= last) {
      const auto a = busy(e - 4, e), b = busy(e, e + 4);
      if (std::fabs(std::log((b.first + 0.1) / (a.first + 0.1))) > 0.45 || std::fabs(b.second - a.second) > 15) end += 0.35;
    } else {
      end += 0.2;
    }
    for (int L : {2, 3, 4, 6, 8}) {
      const int s0 = e - L;
      if (s0 <= first) continue;  // (the brushing has begun, a bar before)
      bool playing = true;
      double c = 0;
      for (int b = s0; b < e; ++b) { playing &= count[b] > 0; c += calm[b]; }
      if (!playing) continue;  // (a general pause: the brushes rest too)
      c /= L;
      // Which cymbal: the ride, else a crash, the first the file does not
      // strike from a beat before the swirl to its end. A stroke on it just
      // after: the swirl leads into it (and lifts first).
      const double on = timeAt(4.0 * s0), off = timeAt(4.0 * e);
      int pick = -1;
      double into = 0;
      for (int k = 0; k < 3 && pick < 0; ++k) {
        bool free = true;
        for (double t : struck[k]) free &= t < on - beatLen || t >= off - 0.06;
        if (!free) continue;
        pick = k;
        for (double t : struck[k]) into = std::max(into, t >= off - 0.06 && t < off + beatLen ? 0.2 : 0.0);
      }
      if (pick < 0) continue;
      const double fit = ballad ? (L >= 4 ? 0.1 : 0.0) : (L <= 4 ? 0.1 : -0.05);
      cand.push_back({1.5 * c + end + into + fit + (ballad ? 0.15 : 0) + 0.15 * ur(), s0, e, pick});
      if (c < 0.45) cand.back().score = -1;  // (not in a loud, busy passage)
    }
  }
  std::stable_sort(cand.begin(), cand.end(), [](const Cand& a, const Cand& b) { return a.score > b.score; });
  const int most = int(knob("PJ_SWIRL_MAX", std::clamp(1 + nBars / 40, 1, 3)));
  for (const Cand& k : cand) {
    if (int(out.size()) >= most || k.score < 0.9) break;
    bool apart = true;  // (eight bars at least between two)
    for (const CymbalSwirl& o : out) apart &= k.e + 8 <= o.bar0 || k.s >= o.bar0 + o.bars + 8;
    if (!apart) continue;
    double v = 0;
    int n = 0;
    for (int b = k.s; b < k.e; ++b) { v += drumN[b] ? drumVel[b] : vel[b]; n += drumN[b] ? drumN[b] : count[b]; }
    const int velocity = int(std::clamp(v / std::max(n, 1), 45.0, 90.0));
    out.push_back({timeAt(4.0 * k.s), timeAt(4.0 * k.e) - 0.06, kKey[k.c], velocity, k.s, k.e - k.s});
  }
  std::sort(out.begin(), out.end(), [](const CymbalSwirl& a, const CymbalSwirl& b) { return a.on < b.on; });
  return out;
}

// Each drum at the SoundFonts' level (tools/gmcompare.py kits --emit: drumcal_data.h). The table
// holds, for a kit's key, how loud the fonts' average stroke is at four velocities and how loud
// pajupilli's own is at eight (both against a piano's note in the same file, dB). An instrument
// rings all its keys in one voice, so its level is one gain (drumGainDb, drumTrimDb: enough for its key that
// needs the most at velocity 127) and each key's the stroke's strength (drumVelocity: the velocity
// at which its own curve reaches the fonts' level). PJ_NODRUMCAL: neither.
namespace {
struct DrumCalState {
  bool on = false;
  std::vector<std::pair<std::string, double>> gain;
  unsigned char map[3][128][128];
  bool has[3][128] = {};
  double own(const DrumCal& c, double u) const {  // the key's own level at velocity u, never falling as u rises
    double m = c.own[0];
    if (u <= kDrumCalGrid[0]) return m + 33.3 * std::log10(std::max(u, 1.0) / kDrumCalGrid[0]);
    for (int i = 1; i < kDrumCalGridN; ++i) {
      const double at = m;
      m = std::max(m, double(c.own[i]));
      if (u <= kDrumCalGrid[i]) return at + (u - kDrumCalGrid[i - 1]) / double(kDrumCalGrid[i] - kDrumCalGrid[i - 1]) * (m - at);
    }
    return m;
  }
  static double target(const DrumCal& c, double v) {
    if (v <= kDrumCalVels[0]) return c.target[0] + 33.3 * std::log10(std::max(v, 1.0) / kDrumCalVels[0]);
    int i = 0;
    while (i + 2 < 4 && kDrumCalVels[i + 1] < v) ++i;
    const double a = (v - kDrumCalVels[i]) / double(kDrumCalVels[i + 1] - kDrumCalVels[i]);
    return c.target[i] + a * (c.target[i + 1] - c.target[i]);
  }
  double gainOf(const char* inst) const {
    for (const auto& g : gain)
      if (g.first == inst) return g.second;
    return 0;
  }
  DrumCalState() {
    if (std::getenv("PJ_NODRUMCAL") || kDrumCal[0].kit < 0) return;
    on = true;
    static const Kit kKits[3] = {Kit::kStandard, Kit::kJazz, Kit::kBrush};
    for (const DrumCal& c : kDrumCal) {
      const Drum d = drumFor(c.key, kKits[c.kit], false);
      if (!d.inst) continue;
      double need = std::clamp(double(c.target[3]) - own(c, 127), -15.0, 15.0);
      // (an instrument that is a General MIDI program too keeps its level there: its drum keys by the stroke alone)
      if (std::strcmp(d.inst, "agogo") == 0) need = 0;  // (GM 114)
      bool found = false;
      for (auto& g : gain)
        if (g.first == d.inst) { g.second = std::max(g.second, need); found = true; }
      if (!found) gain.push_back({d.inst, need});
    }
    for (const DrumCal& c : kDrumCal) {
      const Drum d = drumFor(c.key, kKits[c.kit], false);
      if (!d.inst) continue;
      const double g = gainOf(d.inst);
      has[c.kit][c.key] = true;
      int u = 1;
      for (int v = 1; v < 128; ++v) {
        const double want = target(c, v) - g;
        while (u < 127 && own(c, u) < want) ++u;
        map[c.kit][c.key][v] = (unsigned char)u;
      }
    }
  }
};
const DrumCalState& drumCal() {
  static const DrumCalState s;
  static const bool dbg = [] {
    if (std::getenv("PJ_DRUMCALDBG"))
      for (const auto& g : s.gain) std::fprintf(stderr, "drumcal: %s %+.1f dB\n", g.first.c_str(), g.second);
    return true;
  }();
  (void)dbg;
  return s;
}
}  // namespace

// (The gain is the player's, as velocity's is: the room hears it too (ensemble.cc: vg). The level
// trim is not: it moves the direct sound alone, and a kick trimmed 5 dB down in a hall came out
// 1-2 dB softer and further away. As velocity's, at most +6 dB into the room; what is over goes to the trim.)
double drumGainDb(const char* inst) { return drumCal().on ? std::min(drumCal().gainOf(inst), 6.0) : 0; }
double drumTrimDb(const char* inst) { return drumCal().on ? std::max(drumCal().gainOf(inst) - 6.0, 0.0) : 0; }

int drumVelocity(int gmKey, Kit kit, int velocity) {
  const DrumCalState& s = drumCal();
  const int k = kit == Kit::kStandard ? 0 : kit == Kit::kJazz ? 1 : 2;
  if (!s.on || gmKey < 0 || gmKey > 127 || velocity < 1 || !s.has[k][gmKey]) return velocity;
  return s.map[k][gmKey][std::min(velocity, 127)];
}

DrumKit::DrumKit(const RoomSpec* room, bool verbose, int voices, Kit kit) {
  kit_ = kit;
  for (int k = 0; k < 128; ++k) {
    part_[k] = key_[k] = -1;
    const Drum d = drumFor(k, kit, false);
    const Instrument* in = d.inst ? findInstrument(d.inst) : nullptr;
    if (!in) continue;
    int p = 0;
    while (p < int(parts_.size()) && parts_[p].inst != in) ++p;
    if (p == int(parts_.size())) {
      Part np;
      np.inst = in;
      np.cal.reset(new Calibration(std::getenv("PJ_NOCAL") ? Calibration() : calibrate(*in, 440, nullptr)));
      np.ens.reset(new Ensemble(*in, *np.cal, voices, verbose, room));
      np.ens->setKeepTails(1.5);  // (a stroke rings on past its key's release)
      parts_.push_back(std::move(np));
    }
    part_[k] = p;
    key_[k] = d.key + in->transpose;
  }
}

bool DrumKit::noteOn(int k, int velocity) {
  if (k < 0 || k > 127 || part_[k] < 0) return false;
  parts_[part_[k]].ens->noteOn(key_[k], drumVelocity(k, kit_, velocity));
  held_[k] = true;
  return true;
}

void DrumKit::noteOff(int k) {
  if (k < 0 || k > 127 || !held_[k]) return;
  parts_[part_[k]].ens->noteOff(key_[k]);
  held_[k] = false;
}

void DrumKit::allOff() {
  for (int k = 0; k < 128; ++k) noteOff(k);
}

void DrumKit::renderAdd(float* left, float* right, int n) {
  l_.resize(n);
  r_.resize(n);
  for (Part& p : parts_) {
    p.ens->render(l_.data(), r_.data(), n);
    for (int i = 0; i < n; ++i) { left[i] += l_[i]; right[i] += r_[i]; }
  }
}

int DrumKit::resets() const {
  int n = 0;
  for (const Part& p : parts_) n += p.ens->resets();
  return n;
}

std::string DrumKit::describe() const {
  std::string s;
  for (int p = 0; p < int(parts_.size()); ++p) {
    s += std::string("  ") + parts_[p].inst->name + ":";
    for (int k = 35; k <= 87; ++k)
      if (part_[k] == p) s += std::string(" ") + std::to_string(k) + " " + drumName(k) + ",";
    s.back() = '\n';
  }
  s += "  silent (not built):";
  for (int k = 35; k <= 87; ++k)
    if (part_[k] < 0 && *drumName(k)) s += std::string(" ") + std::to_string(k) + " " + drumName(k) + ",";
  s.back() = '\n';
  return s;
}

}  // namespace pajupilli
