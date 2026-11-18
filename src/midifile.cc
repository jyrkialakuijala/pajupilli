#include "midifile.h"

#include "organ.h"
#include "organplan.h"
#include "hammond.h"
#include "hammondplan.h"

#include <alsa/asoundlib.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <memory>
#include <thread>
#include <vector>

#include "analysis.h"
#include "calibrate.h"
#include "drumkit.h"
#include "ensemble.h"
#include "instrument.h"
#include "parallel.h"
#include "performer.h"
#include "songclock.h"

namespace pajupilli {

namespace {

constexpr int kRate = 48000, kBlock = 64;

// --- The file.

struct Event {
  double t;      // seconds
  int order;     // (ties: in the file's order)
  uint8_t status, a, b;
  int track = 0;       // (the file's track: an organ part's manuals)
  uint64_t tick = 0;
  float touch = 0;     // (an organ note-on's finger: 0 normal .. 1 the softest, a long high note)
  bool trill = false;  // (an organ note-on in a trill: kept legato, not articulated)
};

struct Song {
  std::vector<Event> ev;
  std::string title;
  int tonic = -1;      // the key's tonic, a pitch class (-1: not known)
  bool minor = false, keySigned = false;
  bool gs = false;  // a Roland GS or General MIDI 2 file (else the drum keys below 35 are Yamaha XG's)
  // The tempo map: from `t` seconds on (at `beat` quarter notes), a quarter lasts `len` s.
  struct Tempo { double t, beat, len; };
  std::vector<Tempo> tempo;
  int div = 480;       // ticks a quarter
  long barTicks = 0;   // a bar (the first time signature; 0: none given, 4/4 taken)
};

uint32_t be(const uint8_t* p, int n) {
  uint32_t v = 0;
  for (int i = 0; i < n; ++i) v = v << 8 | p[i];
  return v;
}

bool readSong(const std::string& path, Song* song, std::string* err) {
  std::ifstream f(path, std::ios::binary);
  if (!f) { *err = "cannot open " + path; return false; }
  std::vector<uint8_t> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  if (d.size() < 14 || std::string(d.begin(), d.begin() + 4) != "MThd") { *err = "not a MIDI file"; return false; }
  const int ntrk = int(be(&d[10], 2)), div = int(be(&d[12], 2));
  if (div & 0x8000) { *err = "SMPTE time division is not supported"; return false; }
  song->div = div;
  // Every track's events in ticks; the tempo map from all of them.
  struct Raw { uint64_t tick; int order; uint8_t s, a, b; int track; };
  std::vector<Raw> raw;
  std::vector<std::pair<uint64_t, double>> tempo;  // tick, seconds per tick from there
  size_t pos = 8 + be(&d[4], 4);
  int order = 0;
  for (int tr = 0; tr < ntrk && pos + 8 <= d.size(); ++tr) {
    const size_t len = be(&d[pos + 4], 4), end = std::min(d.size(), pos + 8 + len);
    const bool isTrk = std::string(d.begin() + pos, d.begin() + pos + 4) == "MTrk";
    size_t p = pos + 8;
    pos = end;
    if (!isTrk) continue;
    uint64_t tick = 0;
    uint8_t run = 0;
    auto var = [&]() {
      uint32_t v = 0;
      while (p < end) {
        const uint8_t c = d[p++];
        v = v << 7 | (c & 0x7f);
        if (!(c & 0x80)) break;
      }
      return v;
    };
    while (p < end) {
      tick += var();
      if (p >= end) break;
      uint8_t s = d[p];
      if (s & 0x80) ++p;
      else s = run;  // running status
      if (s == 0xff) {
        const uint8_t type = d[p++];
        const uint32_t n = var();
        if (type == 0x51 && n == 3 && p + 3 <= end) tempo.push_back({tick, be(&d[p], 3) * 1e-6 / div});
        if (type == 0x59 && n == 2 && p + 2 <= end && song->tonic < 0) {  // the key signature: sharps (flats: negative), minor
          const int sf = int8_t(d[p]);
          song->minor = d[p + 1] == 1;
          song->tonic = ((sf * 7 + (song->minor ? 9 : 0)) % 12 + 12) % 12;
          song->keySigned = true;
        }
        if (type == 0x03 && song->title.empty() && tr <= 1) song->title.assign(d.begin() + p, d.begin() + std::min(end, p + n));
        if (type == 0x58 && n >= 2 && p + 2 <= end && song->barTicks == 0)  // the time signature: a bar's ticks
          song->barTicks = long(d[p]) * div * 4 / (1L << std::min<int>(d[p + 1], 6));
        p += n;
        continue;
      }
      if (s == 0xf0 || s == 0xf7) {
        const uint32_t n = var();
        // A Roland GS reset or a General MIDI 2 system-on: the drum keys below 35 are theirs.
        static const uint8_t kGs[] = {0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f, 0x00, 0x41}, kGm2[] = {0x7e, 0x7f, 0x09, 0x03};
        if (s == 0xf0 && p + n <= end) {
          if (n >= sizeof kGs && d[p] == 0x41 && std::equal(kGs + 2, kGs + 9, d.begin() + p + 2)) song->gs = true;  // (any device number)
          if (n >= sizeof kGm2 && d[p] == 0x7e && d[p + 2] == 0x09 && d[p + 3] == 0x03) song->gs = true;
        }
        p += n;
        continue;
      }
      if (s < 0x80) break;  // corrupt
      run = s;
      const int kind = s & 0xf0;
      const int nb = kind == 0xc0 || kind == 0xd0 ? 1 : 2;
      if (p + nb > end) break;
      raw.push_back({tick, order++, s, d[p], uint8_t(nb == 2 ? d[p + 1] : 0), tr});
      p += nb;
    }
  }
  std::stable_sort(tempo.begin(), tempo.end(), [](auto& x, auto& y) { return x.first < y.first; });
  if (tempo.empty() || tempo[0].first > 0) tempo.insert(tempo.begin(), {0, 0.5 / div});
  std::sort(raw.begin(), raw.end(), [](const Raw& x, const Raw& y) { return x.tick != y.tick ? x.tick < y.tick : x.order < y.order; });
  {
    double t = 0;
    for (size_t i = 0; i < tempo.size(); ++i) {
      if (i > 0) t += (tempo[i].first - tempo[i - 1].first) * tempo[i - 1].second;
      song->tempo.push_back({t, double(tempo[i].first) / div, tempo[i].second * div});
    }
  }
  size_t k = 0;
  double t0 = 0;  // seconds at tempo[k].first
  for (const Raw& r : raw) {
    while (k + 1 < tempo.size() && tempo[k + 1].first <= r.tick) {
      t0 += (tempo[k + 1].first - tempo[k].first) * tempo[k].second;
      ++k;
    }
    song->ev.push_back({t0 + (r.tick - tempo[k].first) * tempo[k].second, r.order, r.s, r.a, r.b, r.track, r.tick});
  }
  return true;
}

// --- General MIDI programs to the instruments built.

// Per program (1..128): the instruments to try, the first built one plays.
const char* const kGm[128][3] = {
    {"grand-piano"}, {"grand-piano"}, {"electric-grand", "grand-piano"}, {"honky-tonk", "upright-piano", "grand-piano"},
    {"electric-piano-1", "electric-grand", "grand-piano"}, {"electric-piano-2", "electric-grand", "grand-piano"},
    {"harpsichord", "upright-piano", "grand-piano"}, {"clavinet", "electric-grand", "grand-piano"},
    // Chromatic percussion.
    {"celesta", "vibraphone", "tinkle-bell"}, {"glockenspiel", "vibraphone", "tinkle-bell"},
    {"music-box", "kalimba", "vibraphone"}, {"vibraphone"}, {"marimba", "vibraphone"}, {"xylophone", "vibraphone"},
    {"tubular-bells", "tinkle-bell", "vibraphone"}, {"dulcimer", "koto"},
    // Organs, accordions, harmonica: the church organ (all its stops), the reed organ, the accordion,
    // the harmonica and the bandoneón (the tango accordion) are their own; the drawbar organs fall
    // back to the calliope's pipes.
    {"drawbar-organ", "lead-calliope"}, {"percussive-organ", "lead-calliope"}, {"rock-organ", "lead-calliope"},
    {"organ-plan", "organ", "lead-calliope"}, {"reed-organ", "lead-square"}, {"accordion", "lead-square"},
    {"harmonica", "lead-square"}, {"bandoneon", "accordion", "lead-square"},
    // Guitars.
    {"nylon-guitar", "koto"}, {"steel-guitar", "banjo", "koto"}, {"jazz-guitar", "koto"}, {"clean-guitar", "koto"},
    {"muted-guitar", "shamisen", "koto"}, {"overdriven-guitar", "lead-charang", "lead-saw"},
    {"distortion-guitar", "lead-charang", "lead-saw"}, {"guitar-harmonics", "kalimba"},
    // Basses.
    {"double-bass"}, {"electric-bass", "double-bass"}, {"pick-bass", "double-bass"}, {"fretless-bass", "double-bass"},
    {"slap-bass-1", "double-bass"}, {"slap-bass-2", "double-bass"}, {"synth-bass-1"}, {"synth-bass-2"},
    // Strings.
    {"violin", "fiddle"}, {"viola", "fiddle"}, {"cello", "contrabass"}, {"contrabass"},
    {"tremolo-strings", "synth-strings-1"}, {"pizzicato-strings", "double-bass"}, {"harp", "koto"}, {"timpani", "taiko"},
    // Ensembles.
    {"strings", "synth-strings-1"}, {"slow-strings", "synth-strings-2"}, {"synth-strings-1"}, {"synth-strings-2"},
    {"choir", "pad-choir"}, {"voice-oohs", "synth-voice"}, {"synth-voice"}, {"orchestra-hit", "synth-brass-1"},
    // Brass.
    {"trumpet"}, {"trombone"}, {"tuba"}, {"muted-trumpet"}, {"horn"}, {"brass-section", "horn", "trumpet"},
    {"synth-brass-1"}, {"synth-brass-2"},
    // Reeds.
    {"soprano-sax"}, {"alto-sax"}, {"tenor-sax"}, {"baritone-sax"}, {"oboe"}, {"english-horn"}, {"bassoon"}, {"clarinet"},
    // Pipes.
    {"piccolo"}, {"flute"}, {"recorder"}, {"panflute"}, {"bottle"}, {"shakuhachi"}, {"whistle"}, {"ocarina"},
    // Synth leads and pads.
    {"lead-square"}, {"lead-saw"}, {"lead-calliope"}, {"lead-chiff"}, {"lead-charang"}, {"lead-voice"}, {"lead-fifths"},
    {"lead-bass+lead"}, {"pad-new-age"}, {"pad-warm"}, {"pad-polysynth"}, {"pad-choir"}, {"pad-bowed"},
    {"pad-metallic"}, {"pad-halo"}, {"pad-sweep"},
    // Synth effects.
    {"fx-rain", "pad-new-age"}, {"fx-soundtrack", "pad-warm"}, {"fx-crystal"}, {"fx-atmosphere", "pad-halo"},
    {"fx-brightness", "pad-metallic"}, {"fx-goblins", "pad-sweep"}, {"fx-echoes", "pad-halo"}, {"fx-sci-fi", "pad-sweep"},
    // Ethnic.
    {"sitar"}, {"banjo"}, {"shamisen"}, {"koto"}, {"kalimba"}, {"bagpipe"}, {"fiddle"}, {"shehnai"},
    // Percussive.
    {"tinkle-bell"}, {"agogo"}, {"steel-drums"}, {"woodblock"}, {"taiko"}, {"melodic-tom"}, {"synth-drum"},
    {"reverse-cymbal"},
    // Sound effects.
    {"fret-noise"}, {"breath-noise"}, {"seashore"}, {"bird-tweet"}, {"telephone"}, {"helicopter"}, {"applause"},
    {"gunshot"},
};

// -reinstrument's rules: (from, to).
std::vector<std::pair<std::string, const Instrument*>> gReinstrument;

const Instrument* forProgram(int prog) {
  const Instrument* in = nullptr;
  for (const char* nm : kGm[prog & 127])
    if (nm && !in) in = findInstrument(nm);
  for (const auto& [from, to] : gReinstrument)
    if (from == "*" || (in && (from == in->name || from == in->family))) return to;
  return in;
}

// (an instrument of the Hammond family: all of a file's join one organ)
bool isHammond(const Instrument* in) { return in && std::strcmp(in->family, "hammond") == 0; }

}  // namespace

static int gPlayTranspose = 0;
void setPlayTranspose(int semitones) { gPlayTranspose = semitones; }

bool setReinstrument(const std::string& spec) {
  gReinstrument.clear();
  for (size_t at = 0; at <= spec.size();) {
    const size_t e = std::min(spec.find(',', at), spec.size());
    const std::string rule = spec.substr(at, e - at);
    at = e + 1;
    if (rule.empty()) continue;
    const size_t eq = rule.find('=');
    if (eq == std::string::npos) {
      std::fprintf(stderr, "-reinstrument: %s: want FROM=TO\n", rule.c_str());
      return false;
    }
    const std::string from = rule.substr(0, eq);
    std::string to = rule.substr(eq + 1);
    if (to == "organ") to = "organ-plan";  // (the church organ by its plan)
    const Instrument* in = findInstrument(to.c_str());
    if (!in) {
      std::fprintf(stderr, "-reinstrument: no instrument %s\n", to.c_str());
      return false;
    }
    gReinstrument.push_back({from, in});
    std::fprintf(stderr, "  reinstrumented: %s on %s\n", from.c_str(), in->name);
  }
  return true;
}

namespace {

// One player of the song: a channel's instrument (or a drum's), as a pool.
struct Part {
  const Instrument* inst;
  std::unique_ptr<Ensemble> ens;
  // An organ played by a plan (organ-plan: src/organplan.h): its tracks to
  // the Pedal and the manuals, its registration by the bar.
  std::unique_ptr<OrganPlan> plan;
  size_t planNext = 0;
  std::map<int, int> manualOf;               // track -> division
  std::map<int, std::pair<int, int>> organKey;  // channel * 128 + key -> (division, key)
  // A Hammond by its plan (src/hammondplan.h): each note's division, the drawbars for the piece.
  std::unique_ptr<HammondPlan> hplan;
  std::map<int, int> hammondDiv;             // channel * 128 + key -> division (held notes)
  size_t leslieNext = 0;                     // the plan's next Leslie change
  size_t percNext = 0;                       // the plan's next percussion switch
  size_t regNext = 0;                        // the plan's next re-registration
  size_t boostNext = 0;                      // the plan's next Leslie nudge
  std::map<int, int> hammondChans;           // channel -> its notes (a Hammond playing several)
  int maxPoly = 1, maxHeld = 1, played = 0;  // (voices: notes held at once, their tails counted)
  double vol = 100 / 127.0, expr = 1;
  std::vector<float> l, r;
};

}  // namespace

int playMidiFile(const std::string& path, const char* outWav, const RoomSpec* room, const std::string& device,
                 bool verbose, bool roomGiven) {
  Song song;
  std::string err;
  if (!readSong(path, &song, &err)) { std::fprintf(stderr, "-play: %s\n", err.c_str()); return 1; }
  if (song.ev.empty()) { std::fprintf(stderr, "-play: no events in %s\n", path.c_str()); return 1; }
  if (gPlayTranspose != 0) {  // (every note but the drums', before anything plans on them)
    int clipped = 0;
    for (Event& e : song.ev) {
      const int kind = e.status & 0xf0;
      if ((e.status & 15) == 9 || (kind != 0x80 && kind != 0x90 && kind != 0xa0)) continue;
      const int k = e.a + gPlayTranspose;
      clipped += k < 0 || k > 127;
      e.a = uint8_t(std::clamp(k, 0, 127));
    }
    std::fprintf(stderr, "  transposed %+d semitones%s\n", gPlayTranspose, clipped ? " (some notes clamped to 0..127)" : "");
  }
  // A file played hot throughout (every note near 127: a sequencer's default,
  // a keyboard with a hard curve) has no loudness to spare: its velocities
  // are scaled together until the median is 96 (PJ_PLAY_VELMEDIAN), keeping
  // their ratios (a constant few dB down on the GM curve, and softer strokes).
  // A file with real dynamics (a median of 40 .. 80) is played as it is.
  {
    static const double kMedian = std::getenv("PJ_PLAY_VELMEDIAN") ? std::atof(std::getenv("PJ_PLAY_VELMEDIAN")) : 96;
    std::vector<int> v;
    for (const Event& e : song.ev)
      if ((e.status & 0xf0) == 0x90 && e.b > 0) v.push_back(e.b);
    if (!v.empty()) {
      std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
      const int med = v[v.size() / 2];
      if (med > kMedian) {
        const double k = kMedian / med;
        for (Event& e : song.ev)
          if ((e.status & 0xf0) == 0x90 && e.b > 0) e.b = uint8_t(std::clamp(int(std::lround(e.b * k)), 1, 127));
        std::fprintf(stderr, "velocities: median %d, scaled by %.2f (%.1f dB)\n", med, k, 33.3 * std::log10(k));
      }
    }
  }
  // First pass: which parts there are, and how many notes each holds at once.
  std::map<int, Part> parts;  // key: channel * 128 + program (channel 9: 9 * 128 + drum instrument index)
  std::vector<std::string> drumNames;
  auto drumIndex = [&](const char* nm) {
    for (size_t i = 0; i < drumNames.size(); ++i)
      if (drumNames[i] == nm) return int(i);
    drumNames.push_back(nm);
    return int(drumNames.size() - 1);
  };
  // The song's key, for what is tuned to it. The file's key signature says
  // it; a file without one (or with the empty C major that many carry
  // whatever their key): the key whose scale fits the notes best, each
  // pitch class by how long it sounds (Krumhansl's profiles).
  {
    double hist[12] = {};
    std::map<int, double> onAt;
    for (const Event& e : song.ev) {
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (ch == 9) continue;
      if (kind == 0x90 && e.b > 0) onAt[ch * 128 + e.a] = e.t;
      else if (kind == 0x80 || kind == 0x90) {
        auto it = onAt.find(ch * 128 + e.a);
        if (it != onAt.end()) { hist[e.a % 12] += std::min(e.t - it->second, 2.0); onAt.erase(it); }
      }
    }
    static const double maj[12] = {6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88};
    static const double min[12] = {6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17};
    double best = -1e9, total = 0;
    int bt = 0;
    bool bm = false;
    for (double h : hist) total += h;
    for (int t = 0; t < 12 && total > 0; ++t)
      for (int m = 0; m < 2; ++m) {
        double sxy = 0, sx = 0, sy = 0, sxx = 0, syy = 0;
        for (int i = 0; i < 12; ++i) {
          const double x = hist[(t + i) % 12], y = m ? min[i] : maj[i];
          sxy += x * y; sx += x; sy += y; sxx += x * x; syy += y * y;
        }
        const double r = (12 * sxy - sx * sy) / std::sqrt(std::max((12 * sxx - sx * sx) * (12 * syy - sy * sy), 1e-30));
        if (r > best) { best = r; bt = t; bm = m; }
      }
    if (!song.keySigned || (song.tonic == 0 && !song.minor)) { song.tonic = bt; song.minor = bm; }
    SongClock::get().tonic = song.tonic;
    SongClock::get().minor = song.minor;
  }
  // Which snare the drummer plays. The usual file: sticks. A file whose band
  // has a piano and an upright bass (General MIDI programs 1 .. 4, and 33 or
  // the fretless 36, with notes on them) is a jazz group: the brushed snare. A file that selects a
  // Brush kit gets the brushes whatever its band.
  bool jazzBand = false;
  {
    int pr[16] = {};
    bool piano = false, upright = false;
    for (const Event& e : song.ev) {
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pr[ch] = e.a;
      else if (kind == 0x90 && e.b > 0 && ch != 9) {
        piano |= pr[ch] <= 3;
        upright |= pr[ch] == 32 || pr[ch] == 35;  // (acoustic; fretless too: what a sequenced upright is often set to)
      }
    }
    jazzBand = piano && upright;
  }
  auto kitOf = [&](int program) {
    if (brushKitWanted() || kitFor(program) == Kit::kBrush) return Kit::kBrush;
    return jazzBand ? Kit::kJazz : Kit::kStandard;
  };
  // With the brushed snare the drummer now and then swirls a brush on a cymbal
  // for a few bars (drumkit.h: planCymbalSwirls): its own held keys on the drum channel.
  {
    std::vector<DrumPlanNote> notes;
    int pr[16] = {};
    for (const Event& e : song.ev) {
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pr[ch] = e.a;
      else if (kind == 0x90 && e.b > 0) notes.push_back({e.t, e.a, e.b, ch == 9, kitOf(pr[9])});
    }
    std::vector<DrumTempo> tm;
    for (const Song::Tempo& x : song.tempo) tm.push_back({x.t, x.beat, x.len});
    const std::vector<CymbalSwirl> sw = planCymbalSwirls(notes, tm, song.gs);
    for (const CymbalSwirl& s : sw) {
      std::fprintf(stderr, "  (the brushes: a swirl on the %s, bars %d-%d, %.1f-%.1f s, velocity %d)\n",
                   s.key == kKeySwirlRide ? "ride" : s.key == kKeySwirlCrash2 ? "crash (18\")" : "crash (16\")", s.bar0 + 1, s.bar0 + s.bars, s.on, s.off, s.velocity);
      uint64_t tick = 0;
      for (const Event& e : song.ev)
        if (e.t <= s.on) tick = e.tick;
      for (int k : {s.key, kKeyRightHandAway}) {
        song.ev.push_back({s.on, 0, uint8_t(0x99), uint8_t(k), uint8_t(s.velocity)});
        song.ev.back().tick = tick;
        song.ev.push_back({s.off, 0, uint8_t(0x89), uint8_t(k), uint8_t(0)});
        song.ev.back().tick = tick;
      }
    }
    if (!sw.empty())
      std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& a, const Event& b) { return a.t < b.t; });
  }
  // An instrument may want its note early: what its player does before the
  // sound falls on the beat (a maraca's seeds sliding before the slap) takes
  // time. Its notes' events are moved that much earlier (noteLead), and the
  // player lands the note that much after it is told.
  {
    int pr[16] = {};
    bool moved = false;
    for (Event& e : song.ev) {
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pr[ch] = e.a;
      if (kind != 0x90 && kind != 0x80) continue;
      const Instrument* in = nullptr;
      if (ch == 9) {
        const Drum dr = drumFor(e.a, kitOf(pr[ch]), song.gs);
        in = dr.inst ? findInstrument(dr.inst) : nullptr;
      } else {
        in = forProgram(pr[ch]);
      }
      const double lead = in ? noteLead(*in) : 0;
      if (lead > 0) { e.t = std::max(0.0, e.t - lead); moved = true; }
    }
    if (moved) std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& a, const Event& b) { return a.t < b.t; });
    noteLeadsApplied() = true;
  }
  int prog[16] = {};
  std::map<int, int> sounding;  // part -> notes down now
  std::map<int, int> noteOwner;  // channel * 128 + key -> part
  // Each note keeps its voice past its key's release while it dies away:
  // the voices a part needs are the notes sounding at once with their tails.
  static const double kTail = std::getenv("PJ_PLAY_TAIL") ? std::atof(std::getenv("PJ_PLAY_TAIL")) : 1.5;  // s
  std::map<int, std::vector<std::pair<double, int>>> busy;  // part -> (time, +1 on / -1 tail's end)
  int skipped = 0, unbuilt = 0;
  // (every channel played on the organ by a plan joins one organ part: its
  // lines are the channels' tracks, the plan gives them their manuals)
  int organPart = -1;
  // (and every channel on one Hammond program joins one Hammond: both hands and the pedals on one
  // organ through one Leslie, the channels its lines)
  int hammondPart = -1;  // (one organ: every channel of the Hammond family, whatever its program or remap)
  auto partOf = [&](int ch, int key, int* playKey) -> int {
    if (ch != 9) {
      const Instrument* in = forProgram(prog[ch]);
      if (in && std::strcmp(in->name, "organ-plan") == 0) {
        *playKey = key;
        if (organPart < 0) organPart = ch * 128 + prog[ch];
        return organPart;
      }
      if (in && std::strcmp(in->family, "hammond") == 0) {
        *playKey = key;
        if (hammondPart < 0) hammondPart = ch * 128 + prog[ch];
        return hammondPart;
      }
    }
    if (ch == 9) {
      const Drum dr = drumFor(key, kitOf(prog[ch]), song.gs);
      if (!dr.inst || !findInstrument(dr.inst)) return -1;
      *playKey = dr.key;
      return 9 * 128 + drumIndex(dr.inst);
    }
    *playKey = key;
    return ch * 128 + prog[ch];
  };
  for (const Event& e : song.ev) {
    const int ch = e.status & 15, kind = e.status & 0xf0;
    if (kind == 0xc0) prog[ch] = e.a;
    if (kind == 0x90 && e.b > 0) {
      int pk;
      const int id = partOf(ch, e.a, &pk);
      if (id < 0) { ++skipped; continue; }
      if (!parts.count(id)) {
        const Instrument* in = ch == 9 ? findInstrument(drumNames[id - 9 * 128]) : forProgram(prog[ch]);
        if (!in) { ++unbuilt; continue; }
        parts[id].inst = in;
      }
      noteOwner[ch * 128 + e.a] = id;
      parts[id].maxHeld = std::max(parts[id].maxHeld, ++sounding[id]);
      busy[id].push_back({e.t, +1});
    } else if (kind == 0x80 || (kind == 0x90 && e.b == 0)) {
      auto it = noteOwner.find(ch * 128 + e.a);
      if (it != noteOwner.end()) {
        --sounding[it->second];
        busy[it->second].push_back({e.t + kTail, -1});
        noteOwner.erase(it);
      }
    }
  }
  for (auto& [k, id] : noteOwner) busy[id].push_back({song.ev.back().t + kTail, -1});  // (never released)
  for (auto& [id, marks] : busy) {
    std::sort(marks.begin(), marks.end(), [](auto& a, auto& b) { return a.first != b.first ? a.first < b.first : a.second < b.second; });
    int now = 0, most = 1;
    for (auto& [t, d] : marks) most = std::max(most, now += d);
    if (parts.count(id)) parts[id].maxPoly = most;
  }
  if (!song.title.empty()) std::fprintf(stderr, "%s\n", song.title.c_str());
  std::fprintf(stderr, "%.1f s, %zu parts%s:\n", song.ev.back().t, parts.size(),
               jazzBand ? " (a piano and an upright bass: the snare with brushes)" : "");
  for (auto& [id, p] : parts)
    if (std::strncmp(p.inst->name, "harpsichord", 11) == 0 && !std::getenv("PJ_HPSI_TUNING")) {
      static const char* const kNames[12] = {"C", "C#", "D", "E-flat", "E", "F", "F#", "G", "A-flat", "A", "B-flat", "B"};
      std::fprintf(stderr, "  (the harpsichord tuned just, on %s%s: %s)\n", kNames[song.tonic], song.minor ? " minor" : "",
                   song.keySigned ? "the file's key signature" : "the key its notes fit best");
      break;
    }
  // An organ plays in a church, unless -room said otherwise.
  if (!roomGiven)
    for (auto& [id, p] : parts)
      if (std::strcmp(p.inst->family, "organ") == 0) {
        room = findRoom("church");
        std::fprintf(stderr, "  (an organ: in the church)\n");
        break;
      }
  std::map<const Instrument*, std::unique_ptr<Calibration>> cals;
  for (auto& [id, p] : parts) {
    if (!cals.count(p.inst)) {
      std::fprintf(stderr, "  calibrating the %s...\n", p.inst->name);
      cals[p.inst].reset(new Calibration(std::getenv("PJ_NOCAL") ? Calibration() : calibrate(*p.inst, 440, nullptr)));
    }
    static const int kMaxVoices = std::getenv("PJ_PLAY_VOICES") ? std::atoi(std::getenv("PJ_PLAY_VOICES")) : 32;
    const int poly = std::min(p.maxPoly, kMaxVoices);
    p.ens.reset(new Ensemble(*p.inst, *cals[p.inst], poly, verbose, room));
    p.ens->setKeepTails(kTail);
    p.ens->setVelocityGain(true);
    if (p.inst->oneVoice())
      std::fprintf(stderr, "  %s %2d: %s (%d at once, one voice)\n", id / 128 == 9 ? "drums  " : "channel", id / 128 + 1,
                   p.inst->name, p.maxHeld);
    else
      std::fprintf(stderr, "  %s %2d: %s (%d held at once; %d voices for their tails)\n", id / 128 == 9 ? "drums  " : "channel",
                   id / 128 + 1, p.inst->name, p.maxHeld, poly);
  }
  // A Hammond: its notes by track, its pedals and manuals and drawbars planned (src/hammondplan.h).
  for (auto& [id, p] : parts) {
    if (std::strcmp(p.inst->family, "hammond") != 0 || id / 128 == 9) continue;
    std::vector<HammondNote> notes;
    std::map<int, std::pair<uint64_t, int>> on;  // channel * 128 + key -> (tick, velocity)
    int progs[16] = {}, cc11[16];
    std::fill(cc11, cc11 + 16, 127);
    std::map<int, int> onExpr;  // channel * 128 + key -> its channel's expression then
    for (const Event& e : song.ev) {
      const int ech = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) progs[ech] = e.a;
      if (kind == 0xb0 && e.a == 11) cc11[ech] = e.b;
      if (ech == 9 || !isHammond(forProgram(progs[ech]))) continue;
      if (kind == 0x90 && e.b > 0) {
        on[ech * 128 + e.a] = {e.tick, e.b};
        onExpr[ech * 128 + e.a] = cc11[ech];
      } else if (kind == 0x80 || kind == 0x90) {
        auto it = on.find(ech * 128 + e.a);
        if (it == on.end()) continue;
        // (an instrument routing to one division: hammond-upper-manual, -lower-manual, -pedal)
        const std::string nm = forProgram(progs[ech])->name;
        const int forced = nm == "hammond-upper-manual" ? HammondModel::kUpper : nm == "hammond-lower-manual" ? HammondModel::kLower
                         : nm == "hammond-pedal" ? HammondModel::kPedal : -1;
        notes.push_back({ech * 1000 + e.track, long(it->second.first), long(e.tick), e.a, it->second.second, ech, onExpr[ech * 128 + e.a], forced});
        on.erase(it);
      }
    }
    // Does the file set the Leslie itself (the mod wheel, the sustain pedal, CC82)?
    bool fileLeslie = false;
    {
      int pg[16] = {};
      for (const Event& e : song.ev) {
        const int ech = e.status & 15, kind = e.status & 0xf0;
        if (kind == 0xc0) pg[ech] = e.a;
        if (kind == 0xb0 && ech != 9 && isHammond(forProgram(pg[ech])) && (e.a == 1 || e.a == 64 || e.a == 82)) fileLeslie = true;
      }
    }
    for (const HammondNote& n : notes) ++p.hammondChans[n.ch];
    const double bpm = song.tempo.empty() ? 120 : 60 / song.tempo[0].len;
    p.hplan.reset(new HammondPlan(planHammond(notes, song.div, bpm, p.inst->name, fileLeslie)));
    std::string sum = p.hplan->summary;
    for (size_t at = 0; (at = sum.find('\n', at)) != std::string::npos && at + 1 < sum.size(); at += 5) sum.insert(at + 1, "    ");
    std::fprintf(stderr, "    %s", sum.c_str());
    if (auto* hp = dynamic_cast<HammondPlayer*>(&p.ens->player(0))) {
      hp->setRegistration(p.hplan->reg);
      hp->setLeslieExternal();  // (the file's controls or the plan: the player does not ride it)
    }
  }
  // An organ played by a plan: its notes by track, planned (src/organplan.h).
  for (auto& [id, p] : parts) {
    if (std::strcmp(p.inst->name, "organ-plan") != 0) continue;
    // (its lines: each channel's tracks on the organ, channel * 1000 + track)
    std::vector<OrganNote> notes;
    std::map<int, std::pair<int, uint64_t>> on;  // channel * 128 + key -> (line, tick)
    int progs[16] = {};
    for (const Event& e : song.ev) {
      const int ech = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) progs[ech] = e.a;
      if (ech == 9) continue;
      const Instrument* in = forProgram(progs[ech]);
      if (!in || std::strcmp(in->name, "organ-plan") != 0) continue;
      if (kind == 0x90 && e.b > 0) {
        on[ech * 128 + e.a] = {ech * 1000 + e.track, e.tick};
      } else if (kind == 0x80 || kind == 0x90) {
        auto it = on.find(ech * 128 + e.a);
        if (it == on.end()) continue;
        notes.push_back({it->second.first, long(it->second.second), long(e.tick), e.a});
        on.erase(it);
      }
    }
    std::vector<long> starts;
    std::vector<std::pair<long, double>> bpm;
    for (size_t i = 0; i < song.tempo.size(); ++i) {
      const long tk = long(std::lround(song.tempo[i].beat * song.div));
      if (i > 0) starts.push_back(tk);
      bpm.push_back({tk, 60 / song.tempo[i].len});
    }
    p.plan.reset(new OrganPlan(planOrgan(notes, song.div, song.barTicks ? song.barTicks : 4L * song.div, starts, bpm)));
    std::string sum = p.plan->summary;
    for (size_t at = 0; (at = sum.find('\n', at)) != std::string::npos && at + 1 < sum.size(); at += 5) sum.insert(at + 1, "    ");
    std::fprintf(stderr, "    %s", sum.c_str());
    bool pedal = false;
    for (const OrganPlanChange& c : p.plan->changes)
      for (auto& [t, d] : c.manuals) pedal |= d == 3;
    if (auto* op = dynamic_cast<OrganPlayer*>(&p.ens->player(0))) op->setAutoPedal(!pedal);
    // The articulation: the organ has no touch dynamics, so the notes'
    // lengths are its expression. In the Baroque styles (toucher ordinaire)
    // every note a little detached (a manual's ~12 %, 25 .. 90 ms; the pedal's
    // a quarter, 60 .. 180 ms: its low notes clearing in the church), more
    // before a strong beat (the silence before it: an agogic accent); in the
    // romantic ones (legato absolu) held. A note struck again at once (the
    // same key: its pipe would only sound on) halved in the Baroque, a short
    // break in the romantic, so that the repeat is heard.
    // Trills played alive: a line alternating two keys a step apart (8 notes
    // or more, each under 180 ms) in a perfectly even rhythm (its onsets within
    // 3 ms of even: a quantized file) has its time warped within its span:
    // relaxed at its start (the repercussions ~25 % slower), quickening to its
    // end (~25 % faster), its start and its end kept (PJ_ORGAN_TRILL, 0.25).
    {
      static const double kTrill = std::getenv("PJ_ORGAN_TRILL") ? std::atof(std::getenv("PJ_ORGAN_TRILL")) : 0.25;
      int progs3[16] = {};
      std::map<int, std::vector<std::pair<size_t, size_t>>> lineNotes;  // line -> (note-on, note-off) indices
      std::map<int, size_t> open3;
      for (size_t i = 0; i < song.ev.size(); ++i) {
        const Event& e = song.ev[i];
        const int ech = e.status & 15, kind = e.status & 0xf0;
        if (kind == 0xc0) progs3[ech] = e.a;
        const Instrument* in = ech == 9 ? nullptr : forProgram(progs3[ech]);
        if (!in || std::strcmp(in->name, "organ-plan") != 0) continue;
        const int k = (ech * 1000 + e.track) * 128 + e.a;
        if (kind == 0x90 && e.b > 0) {
          open3[k] = i;
        } else if (kind == 0x80 || kind == 0x90) {
          auto it = open3.find(k);
          if (it == open3.end()) continue;
          lineNotes[ech * 1000 + e.track].push_back({it->second, i});
          open3.erase(it);
        }
      }
      int trills = 0, trillsFound = 0;
      for (auto& [line, ns] : lineNotes) {
        std::sort(ns.begin(), ns.end(), [&](auto& x, auto& y) { return song.ev[x.first].t < song.ev[y.first].t; });
        size_t a = 0;
        while (a + 8 <= ns.size()) {
          auto key = [&](size_t q) { return int(song.ev[ns[q].first].a); };
          auto at = [&](size_t q) { return song.ev[ns[q].first].t; };
          const int k0 = key(a), k1 = a + 1 < ns.size() ? key(a + 1) : -1;
          const double ioi = a + 1 < ns.size() ? at(a + 1) - at(a) : 1;
          // (a trill: the two keys alternating, each step under 180 ms and
          // within 30 % of the first; even within 3 ms throughout: quantized)
          size_t b = a + 1;
          bool even = true;
          if (std::abs(k1 - k0) >= 1 && std::abs(k1 - k0) <= 2 && ioi > 0.02 && ioi < 0.18)
            while (b < ns.size() && key(b) == ((b - a) % 2 ? k1 : k0) && std::fabs(at(b) - at(b - 1) - ioi) < 0.3 * ioi) {
              even = even && std::fabs(at(b) - at(b - 1) - ioi) < 0.003;
              ++b;
            }
          if (b - a >= 8) {
            for (size_t q = a; q < b; ++q) song.ev[ns[q].first].trill = true;
            ++trillsFound;
          }
          if (kTrill > 0 && b - a >= 8 && even) {
            const double t0 = at(a), t1 = song.ev[ns[b - 1].second].t, D = t1 - t0;
            auto warp = [&](double t) {
              const double u = std::clamp((t - t0) / D, 0.0, 1.0);
              return t0 + D * ((1 + kTrill) * u - kTrill * u * u);  // (its rate from 1 - kTrill .. 1 + kTrill)
            };
            for (size_t q = a; q < b; ++q) {
              song.ev[ns[q].first].t = warp(song.ev[ns[q].first].t);
              song.ev[ns[q].second].t = warp(song.ev[ns[q].second].t);
            }
            ++trills;
            a = b;
          } else if (b - a >= 8) {
            a = b;
          } else {
            ++a;
          }
        }
      }
      if (trills) std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& x, const Event& y) { return x.t < y.t; });
      if (trillsFound)
        std::fprintf(stderr, "    (%d trills, legato; %d of them quantized, played alive: relaxed at their start, quickening)\n",
                     trillsFound, trills);
    }
    std::set<int> pedalLines;
    for (const OrganPlanChange& c : p.plan->changes)
      for (auto& [t, d] : c.manuals)
        if (d == 3) pedalLines.insert(t);
    {
      const long bar = song.barTicks ? song.barTicks : 4L * song.div;
      std::map<int, size_t> onAt;  // channel * 128 + key -> the note-on's index
      std::map<int, std::vector<double>> starts;  // channel * 128 + key -> its note-ons' times
      std::map<int, std::vector<double>> lineStarts;  // line -> its note-ons' times (a phrase's end: a rest after it)
      std::set<int> hwLines;  // (the lines on the Hauptwerk: the melody's)
      for (const OrganPlanChange& c : p.plan->changes)
        for (auto& [t, d] : c.manuals)
          if (d == 0) hwLines.insert(t);
      int progs2[16] = {};
      auto onOrgan = [&](int ech) {
        if (ech == 9) return false;
        const Instrument* in = forProgram(progs2[ech]);
        return in && std::strcmp(in->name, "organ-plan") == 0;
      };
      for (const Event& e : song.ev) {
        const int ech = e.status & 15, kind = e.status & 0xf0;
        if (kind == 0xc0) progs2[ech] = e.a;
        if (kind == 0x90 && e.b > 0 && onOrgan(ech)) {
          starts[ech * 128 + e.a].push_back(e.t);
          lineStarts[ech * 1000 + e.track].push_back(e.t);
        }
      }
      std::fill(progs2, progs2 + 16, 0);
      int shortened = 0, repeats = 0, softened = 0, phraseEnds = 0, led = 0;
      std::map<int, double> lastOff;  // (channel * 128 + key: its last note's release, articulated)
      int held = 0;                   // (organ notes held now, as written)
      double lastAnyOff = -1;         // (the last release, as written)
      std::set<size_t> fromSilence;   // (note-ons into a silent church: half the bass's lead)
      const bool baroque = p.plan->pedalDetached;
      for (size_t i = 0; i < song.ev.size(); ++i) {
        Event& e = song.ev[i];
        const int ech = e.status & 15, kind = e.status & 0xf0;
        if (kind == 0xc0) progs2[ech] = e.a;
        if (!onOrgan(ech)) continue;
        if (kind == 0x90 && e.b > 0) {
          onAt[ech * 128 + e.a] = i;
          // (from silence: nothing held, nothing released within 1.5 s)
          if (held == 0 && (lastAnyOff < 0 || e.t - lastAnyOff > 1.5)) fromSilence.insert(i);
          ++held;
        } else if (kind == 0x80 || kind == 0x90) {
          auto it = onAt.find(ech * 128 + e.a);
          if (it == onAt.end()) continue;
          held = std::max(0, held - 1);
          lastAnyOff = std::max(lastAnyOff, e.t);
          const double on = song.ev[it->second].t, dur = e.t - on;
          const bool pedalNote = pedalLines.count(ech * 1000 + e.track) > 0;
          if (song.ev[it->second].trill) {  // (a trill's notes legato: unshortened)
            lastOff[ech * 128 + e.a] = e.t;
            onAt.erase(it);
            continue;
          }
          // (a long high note played with a softer finger: from G4 to C6, held
          // from 0.5 s to 2 s; its pallet slower, its attack gentler)
          // (the bass played ahead: its big pipes speak slowly, ~75 ms at middle
          // C, ~ (262 / f)^0.6 longer below, to ~200 ms at 16' C; an organist
          // anticipates part of it, the pedal half the difference, a manual's
          // bass less, at most 60 ms: PJ_PLAY_BASS_LEAD scales it)
          {
            static const double kLead = std::getenv("PJ_PLAY_BASS_LEAD") ? std::atof(std::getenv("PJ_PLAY_BASS_LEAD")) : 1.0;
            const double f = 440 * std::exp2((e.a - 69) / 12.0) * (pedalNote ? 0.5 : 1.0);  // (its sounding pitch: the pedal's 16')
            const double speech = 0.075 * std::pow(262 / f, 0.6);
            // (into silence half of it: no pulse to keep ahead of, nothing to sound with)
            const double lead = std::min(0.06, kLead * (pedalNote ? 0.5 : 0.3) * std::max(0.0, speech - 0.075)) *
                                (fromSilence.count(it->second) ? 0.5 : 1.0);
            // (never ahead of the same key's last release: its late note-off
            // would end this note)
            const auto lo = lastOff.find(ech * 128 + e.a);
            const double room = lo == lastOff.end() ? lead : std::max(0.0, on - lo->second - 0.005);
            if (std::min(lead, room) > 0.002) {
              song.ev[it->second].t = std::max(0.0, on - std::min(lead, room));
              ++led;
            }
          }
          if (!pedalNote) {
            float touch = float(std::clamp((e.a - 67) / 17.0, 0.0, 1.0) * std::clamp((dur - 0.5) / 1.5, 0.0, 1.0));
            // (a phrase's last note, a rest of 250 ms or more after it in its
            // line, or the line's last: softer too, the melody's on the
            // Hauptwerk the most)
            const auto& ls = lineStarts[ech * 1000 + e.track];
            const auto nl = std::upper_bound(ls.begin(), ls.end(), on + 1e-6);
            if (nl == ls.end() || *nl - e.t >= 0.25) {
              touch = std::max(touch, hwLines.count(ech * 1000 + e.track) ? 0.6f : 0.35f);
              ++phraseEnds;
            }
            song.ev[it->second].touch = touch;
            softened += touch > 0.2;
          }
          // (the next strong beat: a bar's start, or its middle in an even metre)
          const bool strong = e.tick % bar == 0 || (bar % (2 * song.div) == 0 && e.tick % (bar / 2) == 0);
          double gap = !baroque ? (pedalNote ? 0.03 : 0.0)
                       : (pedalNote ? std::clamp(0.25 * dur, 0.06, 0.18) : std::clamp(0.12 * dur, 0.025, 0.09)) * (strong ? 1.6 : 1.0);
          // (the same key again at once: its pipe would only sound on)
          const auto& st = starts[ech * 128 + e.a];
          const auto nx = std::upper_bound(st.begin(), st.end(), on + 1e-6);
          if (nx != st.end() && *nx - e.t < 0.03) {
            gap = std::max(gap, baroque ? 0.5 * dur : std::min(0.5 * dur, 0.06));
            ++repeats;
          }
          gap = std::min(gap, 0.6 * dur);
          if (gap > 0) {
            e.t -= gap;
            ++shortened;
          }
          lastOff[ech * 128 + e.a] = e.t;
          onAt.erase(it);
        }
      }
      std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& x, const Event& y) { return x.t < y.t; });
      std::fprintf(stderr, "    (%s: %d notes shortened, %d of them repeated notes; %d notes with a softer finger, %d of them phrase ends; %d bass notes played ahead)\n",
                   baroque ? "toucher ordinaire, the Baroque's articulate legato; the pedal detached"
                           : "legato absolu, the romantic's: held", shortened, repeats, softened, phraseEnds, led);
    }
  }
  if (skipped) std::fprintf(stderr, "  (%d drum notes on keys with nothing built: silent)\n", skipped);
  if (unbuilt) std::fprintf(stderr, "  (%d notes on programs with nothing near built: silent)\n", unbuilt);
  // Second pass: play.
  std::fill(prog, prog + 16, 0);
  noteOwner.clear();
  std::map<int, int> heldKey;  // channel * 128 + key -> the key the part plays
  double chVol[16], chExpr[16];
  std::fill(chVol, chVol + 16, 100 / 127.0);
  std::fill(chExpr, chExpr + 16, 1.0);
  const double end = song.ev.back().t + 3.0;  // and the rooms' tails
  const size_t frames = (size_t(end * kRate) / kBlock + 1) * kBlock;
  // The song is rendered on its own thread into this buffer; the player (or
  // the file) takes what is ready: playing starts once 10 s are ahead, and
  // waits for 10 s again should the rendering fall behind.
  std::vector<float> all(2 * frames, 0.0f);
  std::atomic<size_t> ready{0};
  const bool streaming = outWav == nullptr;
  // Rendering on several threads (PJ_PLAY_THREADS, default 16): one job a
  // part, the heaviest first (the most voices); the synth circuits all in one
  // job, in their order (one board couples them, sample by sample).
  const int threads = std::max(1, std::getenv("PJ_PLAY_THREADS") ? std::atoi(std::getenv("PJ_PLAY_THREADS"))
                                                                   : std::min(16, int(std::thread::hardware_concurrency())));
  std::vector<std::vector<Part*>> jobs;
  {
    std::vector<Part*> circuits, others;
    for (auto& [id, p] : parts) {
      p.l.resize(kBlock);
      p.r.resize(kBlock);
      (p.inst->circuit() ? circuits : others).push_back(&p);
    }
    std::stable_sort(others.begin(), others.end(), [](Part* a, Part* b) { return a->ens->size() > b->ens->size(); });
    if (!circuits.empty()) jobs.push_back(circuits);
    for (Part* p : others) jobs.push_back({p});
  }
  WorkPool pool(std::min(threads, int(jobs.size())));
  if (verbose) std::fprintf(stderr, "  rendering on %d threads (%zu jobs)\n", pool.threads(), jobs.size());
  auto renderSong = [&]() {
    std::vector<float> l(kBlock), r(kBlock);
    size_t next = 0, tk = 0, ahead = 0;
    int heldAll = 0;
    double lastOn = -1e9;
    std::map<int, int> heldNote;  // channel * 128 + key -> times down (not the drums)
    for (size_t n = 0; n < frames; n += kBlock) {
      const double now = (n + kBlock) / double(kRate);
      {  // the beat, for those who keep time with it
        while (tk + 1 < song.tempo.size() && song.tempo[tk + 1].t <= now) ++tk;
        SongClock& c = SongClock::get();
        c.valid = !song.tempo.empty();
        c.now = now;
        if (c.valid) {
          c.beatLen = song.tempo[tk].len;
          c.beat = song.tempo[tk].beat + (now - song.tempo[tk].t) / song.tempo[tk].len;
        }
        // The band's silences: what sounds now, what was last begun, what comes next.
        while (ahead < song.ev.size() && (song.ev[ahead].t < now || (song.ev[ahead].status & 0xf0) != 0x90 || song.ev[ahead].b == 0)) ++ahead;
        const double toNext = ahead < song.ev.size() ? song.ev[ahead].t - now : 1e9;
        c.pause = heldAll <= 0 && now - lastOn > 2 * c.beatLen && toNext > c.beatLen;
      }
      // A Hammond's planned Leslie nudges: in the middle of a long note, where no event comes, so
      // by the clock (their ticks turned to seconds by the tempo map).
      for (auto& [id, p] : parts)
        if (p.hplan)
          while (p.boostNext < p.hplan->leslieBoost.size()) {
            const double b = double(p.hplan->leslieBoost[p.boostNext].first) / song.div;
            size_t k = 0;
            while (k + 1 < song.tempo.size() && song.tempo[k + 1].beat <= b) ++k;
            const double at = song.tempo.empty() ? b * 0.5 : song.tempo[k].t + (b - song.tempo[k].beat) * song.tempo[k].len;
            if (at > now) break;
            if (auto* hp = dynamic_cast<HammondPlayer*>(&p.ens->player(0))) hp->setLeslieBoost(p.hplan->leslieBoost[p.boostNext].second);
            ++p.boostNext;
          }
      for (; next < song.ev.size() && song.ev[next].t < now; ++next) {
        const Event& e = song.ev[next];
        const int ch = e.status & 15, kind = e.status & 0xf0;
        // A Hammond's planned Leslie: its speed changes from their tick on.
        for (auto& [id, p] : parts)
          if (p.hplan)
            while (p.leslieNext < p.hplan->leslie.size() && p.hplan->leslie[p.leslieNext].first <= long(e.tick)) {
              if (auto* hp = dynamic_cast<HammondPlayer*>(&p.ens->player(0))) hp->setLeslie(p.hplan->leslie[p.leslieNext].second ? 2 : 1);
              ++p.leslieNext;
            }
        for (auto& [id, p] : parts)
          if (p.hplan)
            while (p.percNext < p.hplan->percussion.size() && p.hplan->percussion[p.percNext].first <= long(e.tick)) {
              if (auto* hp = dynamic_cast<HammondPlayer*>(&p.ens->player(0))) hp->setPercussion(p.hplan->percussion[p.percNext].second);
              ++p.percNext;
            }
        for (auto& [id, p] : parts)
          if (p.hplan)
            while (p.regNext < p.hplan->upperChanges.size() && p.hplan->upperChanges[p.regNext].first <= long(e.tick)) {
              if (auto* hp = dynamic_cast<HammondPlayer*>(&p.ens->player(0)))
                hp->setUpper(p.hplan->upperChanges[p.regNext].second, p.hplan->lowerExtra);
              ++p.regNext;
            }
        // An organ's plan: its registrations and manuals from their bar on (before a note at the same tick).
        for (auto& [id, p] : parts)
          if (p.plan)
            while (p.planNext < p.plan->changes.size() && p.plan->changes[p.planNext].tick <= long(e.tick)) {
              const OrganPlanChange& c = p.plan->changes[p.planNext++];
              if (auto* op = dynamic_cast<OrganPlayer*>(&p.ens->player(0))) {
                op->setRegistration(c.registration);
                op->setSwellOpening(c.swell);
              }
              for (auto& [t, d] : c.manuals) p.manualOf[t] = d;
            }
        if (kind == 0x90 && e.b > 0) {
          lastOn = e.t;
          if (ch != 9) SongClock::get().lastOn[e.a & 127] = e.t;
          if (ch != 9) { ++heldNote[ch * 128 + e.a]; ++heldAll; }
        } else if ((kind == 0x80 || kind == 0x90) && ch != 9) {
          auto h = heldNote.find(ch * 128 + e.a);
          if (h != heldNote.end() && h->second > 0) { --h->second; --heldAll; }
        }
        if (kind == 0xc0) prog[ch] = e.a;
        else if (kind == 0x90 && e.b > 0) {
          int pk;
          const int id = partOf(ch, e.a, &pk);
          auto it = parts.find(id);
          if (id < 0 || it == parts.end()) continue;
          noteOwner[ch * 128 + e.a] = id;
          heldKey[ch * 128 + e.a] = pk + it->second.inst->transpose;
          if (it->second.plan) {  // (an organ by its plan: the note on its track's division)
            Part& op = it->second;
            const int line = ch * 1000 + e.track;
            const int div = op.manualOf.count(line) ? op.manualOf[line] : 0;
            const int key = pk + op.inst->transpose;
            if (auto* pl = dynamic_cast<OrganPlayer*>(&op.ens->player(0))) {
              auto old = op.organKey.find(ch * 128 + e.a);
              if (old != op.organKey.end()) pl->noteOffManual(old->second.first, old->second.second);
              pl->noteOnManual(div, key, e.touch);
              op.organKey[ch * 128 + e.a] = {div, key};
            }
            ++op.played;
            continue;
          }
          if (it->second.hplan) {  // (a Hammond by its plan: the note on its division)
            Part& hp = it->second;
            auto d = hp.hplan->division.find({ch * 128 + e.a, long(e.tick)});
            const int div = d != hp.hplan->division.end() ? d->second : HammondModel::kUpper;
            if (auto* pl = dynamic_cast<HammondPlayer*>(&hp.ens->player(0))) {
              auto old = hp.hammondDiv.find(ch * 128 + e.a);
              if (old != hp.hammondDiv.end()) pl->noteOffManual(old->second, e.a);
              pl->noteOnManual(div, e.a, e.b);
              hp.hammondDiv[ch * 128 + e.a] = div;
            }
            ++hp.played;
            continue;
          }
          it->second.ens->noteOn(pk + it->second.inst->transpose, e.b);
          ++it->second.played;
        } else if (kind == 0x80 || kind == 0x90) {
          auto it = noteOwner.find(ch * 128 + e.a);
          if (it == noteOwner.end()) continue;
          Part& np = parts[it->second];
          if (np.plan) {
            auto k = np.organKey.find(ch * 128 + e.a);
            if (k != np.organKey.end()) {
              if (auto* pl = dynamic_cast<OrganPlayer*>(&np.ens->player(0))) pl->noteOffManual(k->second.first, k->second.second);
              np.organKey.erase(k);
            }
            noteOwner.erase(it);
            continue;
          }
          if (np.hplan) {
            auto k = np.hammondDiv.find(ch * 128 + e.a);
            if (k != np.hammondDiv.end()) {
              if (auto* pl = dynamic_cast<HammondPlayer*>(&np.ens->player(0))) pl->noteOffManual(k->second, e.a);
              np.hammondDiv.erase(k);
            }
            noteOwner.erase(it);
            continue;
          }
          np.ens->noteOff(heldKey[ch * 128 + e.a]);
          noteOwner.erase(it);
        } else if (kind == 0xb0) {
          // (a Hammond's part hears the controllers of every channel it plays)
          auto hears = [&](int id, const Part& p) {
            return id / 128 == ch || (ch != 9 && std::strcmp(p.inst->family, "hammond") == 0 && isHammond(forProgram(prog[ch])));
          };
          if (e.a == 64)  // the sustain pedal
            for (auto& [id, p] : parts)
              if (hears(id, p)) p.ens->setHold(e.b >= 64);
          if (e.a == 7) chVol[ch] = e.b / 127.0;
          if (e.a == 11) {  // expression: the mixer's fader; the church organ's swell pedal, the Hammond's expression pedal instead
            bool swell = false;
            for (auto& [id, p] : parts)
              if (hears(id, p) && (std::strcmp(p.inst->family, "organ") == 0 || std::strcmp(p.inst->family, "hammond") == 0)) {
                // (a plan's swell wins; a Hammond's pedal follows its tune's channels only, the
                // accompaniment's loudness being in its registration)
                if (!p.plan && !(p.hplan && !p.hplan->exprChannels.count(ch))) p.ens->all(&Performer::setBreath, e.b / 127.0 - 0.5);
                swell = true;
              }
            if (!swell) chExpr[ch] = e.b / 127.0;
          }
          if (e.a == 1)  // the mod wheel: the harmonica's hand tremolo; the Hammond's Leslie (fast from 64)
            for (auto& [id, p] : parts)
              if (hears(id, p) && (std::strcmp(p.inst->name, "harmonica") == 0 || std::strcmp(p.inst->family, "hammond") == 0))
                p.ens->all(&Performer::setFlutter, e.b / 127.0);
          if (e.a == 82)  // the Leslie's switch (as the Leslie 2101's default): stop, slow, fast
            for (auto& [id, p] : parts)
              if (hears(id, p) && std::strcmp(p.inst->family, "hammond") == 0)
                if (auto* hp = dynamic_cast<HammondPlayer*>(&p.ens->player(0))) hp->setLeslie(e.b < 43 ? 0 : e.b < 86 ? 1 : 2);
          if (e.a == 123 || e.a == 120)  // all notes off
            for (auto& [k, id] : noteOwner)
              if (k / 128 == ch) parts[id].ens->noteOff(heldKey[k]);
        } else if (kind == 0xd0) {  // channel pressure: the breath (a Clavinet: its keys pressed harder)
          for (auto& [id, p] : parts)
            if (id / 128 == ch) p.ens->all(&Performer::setBreath, 0.5 * e.a / 127.0);
        } else if (kind == 0xe0) {  // pitch bend: the lips (a Clavinet: the wah)
          const double b = ((e.b << 7 | e.a) - 8192) / 8192.0;
          for (auto& [id, p] : parts)
            if (id / 128 == ch) p.ens->all(&Performer::setLipRoll, b);
        }
      }
      // A harmonica hears how much the others play in its register (C3..D7): its hands' drama.
      for (auto& [id, p] : parts)
        if (std::strcmp(p.inst->name, "harmonica") == 0) {
          double busy = 0;
          for (const auto& [k, c] : heldNote)
            if (c > 0 && k / 128 != id / 128 && k % 128 >= 48 && k % 128 <= 98) busy += 0.25 * c;
          p.ens->all(&Performer::setAccompaniment, busy);
        }
      std::fill(l.begin(), l.end(), 0.0f);
      std::fill(r.begin(), r.end(), 0.0f);
      // The parts side by side on the pool's threads (the circuits as one
      // job: they share a board); then mixed in their order, as on one thread.
      pool.run(int(jobs.size()), [&](int i) {
        for (Part* p : jobs[i]) p->ens->render(p->l.data(), p->r.data(), kBlock);
      });
      for (auto& [id, p] : parts) {
        // Volume and expression: as a mixing desk's fader, on the square law
        // (100 the usual: unity).
        const int ch = id / 128;
        float g = float(std::pow(chVol[ch] * 127 / 100.0, 2) * std::pow(chExpr[ch], 2));
        // (one Hammond playing several channels: one fader, the channels' own by how many of its
        // notes each plays)
        if (!p.hammondChans.empty()) {
          double sum = 0, w = 0;
          for (auto& [c, n] : p.hammondChans) {
            sum += n * std::pow(chVol[c] * 127 / 100.0, 2) * std::pow(chExpr[c], 2);
            w += n;
          }
          if (w > 0) g = float(sum / w);
        }
        for (int j = 0; j < kBlock; ++j) { l[j] += g * p.l[j]; r[j] += g * p.r[j]; }
      }
      float* o = &all[2 * n];
      for (int j = 0; j < kBlock; ++j) {
        o[2 * j] = l[j];
        o[2 * j + 1] = r[j];
      }
      if (streaming)  // (no second look at the whole: a soft knee over 0.9 instead of scaling it down)
        for (int j = 0; j < 2 * kBlock; ++j)
          if (std::fabs(o[j]) > 0.9f) o[j] = std::copysign(0.9f + 0.1f * std::tanh((std::fabs(o[j]) - 0.9f) / 0.1f), o[j]);
      ready.store(n + kBlock, std::memory_order_release);
    }
  };
  const auto t0 = std::chrono::steady_clock::now();
  std::thread renderer(renderSong);
  int lastPct = -1;
  auto progress = [&](size_t at, const char* what) {
    const int pct = int(100.0 * at / frames);
    if (pct / 10 != lastPct / 10) { std::fprintf(stderr, "\r  %s %3d %%", what, pct); lastPct = pct; }
  };
  if (!streaming) {
    while (ready.load(std::memory_order_acquire) < frames) {
      progress(ready.load(), "rendering");
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    renderer.join();
    std::fprintf(stderr, "\r  rendered %.1f s\n", frames / double(kRate));
  } else {
    snd_pcm_t* pcm = nullptr;
    if (snd_pcm_open(&pcm, device.c_str(), SND_PCM_STREAM_PLAYBACK, 0) < 0 ||
        snd_pcm_set_params(pcm, SND_PCM_FORMAT_FLOAT_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 2, kRate, 1, 200000) < 0) {
      std::fprintf(stderr, "-play: cannot open the audio device %s (write a file with -o)\n", device.c_str());
      renderer.join();
      return 1;
    }
    // Playing starts once 10 s are ready (PJ_PLAY_LEAD, s; or the whole song,
    // if shorter), and should the rendering fall behind, it waits for 10 s
    // again: never longer.
    static const double kLead = std::getenv("PJ_PLAY_LEAD") ? std::atof(std::getenv("PJ_PLAY_LEAD")) : 10.0;
    const size_t lead = size_t(std::max(0.1, kLead) * kRate);
    int waits = 0;
    for (size_t i = 0; i < frames;) {
      size_t have = ready.load(std::memory_order_acquire);
      if (have <= i) {
        ++waits;
        const size_t need = std::min(frames, i + lead);
        if (i > 0) {
          const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
          const double speed = have / std::max(secs, 1e-3) / kRate;  // x real time
          std::fprintf(stderr, "\r  (rendering at %.2f x real time: waiting %.0f s)\n", speed,
                       (need - have) / std::max(speed, 0.05) / kRate);
        }
        while ((have = ready.load(std::memory_order_acquire)) < need)
          std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (i == 0) std::fprintf(stderr, "playing...\n");
      }
      const snd_pcm_uframes_t want = std::min<size_t>(2048, have - i);
      snd_pcm_sframes_t w = snd_pcm_writei(pcm, all.data() + 2 * i, want);
      if (w < 0) w = snd_pcm_recover(pcm, int(w), 0);
      if (w < 0) break;
      i += size_t(w);
    }
    snd_pcm_drain(pcm);
    snd_pcm_close(pcm);
    renderer.join();
    if (waits > 1) std::fprintf(stderr, "  (the rendering fell behind: %d pauses)\n", waits - 1);
  }
  int resets = 0;
  for (auto& [id, p] : parts) {
    resets += p.ens->resets();
    if (verbose) std::fprintf(stderr, "  %s: %d notes\n", p.inst->name, p.played);
  }
  if (resets) std::fprintf(stderr, "  (state resets: %d)\n", resets);
  if (!streaming) {
    float peak = 0;
    for (float v : all) peak = std::max(peak, std::fabs(v));
    if (peak > 0.99f) {  // the whole song down to fit, as a mastering engineer would
      std::fprintf(stderr, "  (peak %.2f: the whole song %.1f dB down)\n", peak, 20 * std::log10(0.98 / peak));
      for (float& v : all) v *= 0.98f / peak;
    }
    if (!writeWav(outWav, all, kRate, 2)) { std::fprintf(stderr, "cannot write %s\n", outWav); return 1; }
    std::fprintf(stderr, "wrote %s\n", outWav);
  }
  return 0;
}

}  // namespace pajupilli
