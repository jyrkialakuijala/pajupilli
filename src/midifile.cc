#include "midifile.h"

#include "organ.h"
#include "organplan.h"
#include "heard.h"
#include "tonewheel.h"
#include "tonewheelplan.h"
#include "harpsichord.h"
#include "pianoinst.h"
#include "pianoplan.h"

#include <alsa/asoundlib.h>

#include <algorithm>
#include <functional>
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
#include <tuple>
#include <vector>

#include "analysis.h"
#include "brasssect.h"
#include "calibrate.h"
#include "bassinst.h"
#include "cowbellplan.h"
#include "longnote.h"
#include "rng.h"
#include "tambplan.h"
#include "director.h"
#include "drumkit.h"
#include "band.h"
#include "ensemble.h"
#include "fretless.h"
#include "instrument.h"
#include "parallel.h"
#include "performer.h"
#include "modalinst.h"
#include "choir.h"
#include "jazzsinger.h"
#include "knob.h"
#include "songclock.h"
#include "vmath.h"

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
  int cue = -1;        // (a note-on: the director's cue for it, an index into Direction::cues)
  bool skip = false;   // (-start_time: a note over before the start)
};

struct Song {
  std::vector<Event> ev;
  std::string title;
  int tonic = -1;      // the key's tonic, a pitch class (-1: not known)
  bool minor = false, keySigned = false;
  bool gs = false;  // a GS or General MIDI 2 file (else the drum keys below 35 are XG's)
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
        // A GS reset or a General MIDI 2 system-on: the drum keys below 35 are theirs.
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
    {"grand-piano"}, {"bright-piano", "grand-piano"}, {"electric-grand", "grand-piano"}, {"honky-tonk", "upright-piano", "grand-piano"},
    {"electric-piano-1", "electric-grand", "grand-piano"}, {"electric-piano-2", "electric-grand", "grand-piano"},
    {"harpsichord", "upright-piano", "grand-piano"}, {"clavi", "electric-grand", "grand-piano"},
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
    {"slap-bass-1"}, {"slap-bass-2"}, {"synth-bass-1"}, {"synth-bass-2"},
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
    {"lead-square"}, {"lead-saw"}, {"lead-calliope"}, {"lead-chiff"}, {"lead-charang"}, {"voice-lead", "lead-voice"}, {"lead-fifths"},
    {"lead-bass+lead"}, {"pad-new-age"}, {"pad-warm"}, {"pad-polysynth"}, {"choir-pad", "pad-choir"}, {"pad-bowed"},
    {"pad-metallic"}, {"pad-halo"}, {"pad-sweep"},
    // Synth effects.
    {"fx-rain", "pad-new-age"}, {"fx-soundtrack", "pad-warm"}, {"fx-crystal"}, {"fx-atmosphere", "pad-halo"},
    {"fx-brightness", "pad-metallic"}, {"fx-goblins", "pad-sweep"}, {"fx-echoes", "pad-halo"}, {"fx-sci-fi"},
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
// (a rule's transposition, TO:N semitones, for the lead singer: "flute=voice-lead:-12"; kNoShift: the singer's own choice)
constexpr int kNoShift = 1000;
std::vector<int> gReinstrumentShift;
std::vector<std::string> gReinstrumentVoice;  // (... and its voice, TO:soprano; empty: the singer's own choice)
// (a jazz trio's file, a piano and an upright bass: its GM 36 is the upright, not a fretless:
// research/fretless_bass.md 5, 8.4; the user's decision of 2026-11-16)
bool gJazzUpright = false;

// (the director's choice for a Baroque piece: the flute is the one-keyed Baroque flute)
const Instrument* gBaroqueFlute = nullptr;

const Instrument* forProgram(int prog) {
  const Instrument* in = nullptr;
  for (const char* nm : kGm[prog & 127])
    if (nm && !in) in = findInstrument(nm);
  if (gBaroqueFlute && in && std::strcmp(in->name, "flute") == 0) in = gBaroqueFlute;
  if (gJazzUpright && (prog & 127) == 35)
    if (const Instrument* db = findInstrument("double-bass")) in = db;
  for (const auto& [from, to] : gReinstrument)
    if (from == "*" || (in && (from == in->name || from == in->family))) return to;
  return in;
}

// The transposition -reinstrument gave the part of program prog (kNoShift: none given), and the voice.
int reinstrumentShift(int prog, std::string* voice) {
  const Instrument* in = nullptr;
  for (const char* nm : kGm[prog & 127])
    if (nm && !in) in = findInstrument(nm);
  for (size_t i = 0; i < gReinstrument.size(); ++i) {
    const std::string& from = gReinstrument[i].first;
    if (from == "*" || (in && (from == in->name || from == in->family))) {
      *voice = gReinstrumentVoice[i];
      return gReinstrumentShift[i];
    }
  }
  return kNoShift;
}

// (an instrument of the Tonewheel family: all of a file's join one organ)
bool isTonewheel(const Instrument* in) { return in && std::strcmp(in->family, "tonewheel") == 0; }

// (the file's own program a Tonewheel, before -reinstrument: its sustain pedal and mod wheel are the
// Rotary's switch; a piano's or a lead's reinstrumented onto the organ are its damper and its
// vibrato, nothing on a Tonewheel)
bool fileTonewheel(int prog) {
  for (const char* nm : kGm[prog & 127])
    if (nm)
      if (const Instrument* in = findInstrument(nm)) return isTonewheel(in);
  return false;
}

}  // namespace

// -inst's list: the instruments in General MIDI's order, in its groups of eight programs, each
// with its program number; then those no program reaches.
std::string instrumentList() {
  static const char* const kGroups[16] = {
      "Piano", "Chromatic percussion", "Organ", "Guitar", "Bass", "Strings", "Ensemble", "Brass",
      "Reed", "Pipe", "Synth lead", "Synth pad", "Synth effects", "Ethnic", "Percussive", "Sound effects"};
  std::string out = "General MIDI programs (-play's; a program not built: who plays it instead)\n";
  std::set<std::string> listed;
  auto wrap = [&](const std::vector<std::string>& items) {
    std::string line = "   ";
    for (size_t i = 0; i < items.size(); ++i) {
      const std::string it = items[i] + (i + 1 < items.size() ? "," : "");
      if (line.size() + 1 + it.size() > 100) { out += line + "\n"; line = "   "; }
      line += " " + it;
    }
    out += line + "\n";
  };
  for (int g = 0; g < 16; ++g) {
    out += std::string("  ") + kGroups[g] + " (" + std::to_string(8 * g + 1) + "-" + std::to_string(8 * g + 8) + ")\n";
    std::vector<std::string> items;
    for (int p = 8 * g; p < 8 * g + 8; ++p) {
      std::string it = std::to_string(p + 1) + " " + kGm[p][0];
      const Instrument* in = nullptr;
      for (const char* nm : kGm[p])
        if (nm && !in) in = findInstrument(nm);
      if (in) listed.insert(in->name);
      if (!in) it += " (not built)";
      else if (std::strcmp(in->name, kGm[p][0]) != 0) it += std::string(" (not built: ") + in->name + ")";
      else if (p > 0 && std::strcmp(kGm[p][0], kGm[p - 1][0]) == 0) it = std::to_string(p + 1) + " (" + kGm[p][0] + " again)";
      items.push_back(it);
    }
    wrap(items);
  }
  // The rest, by family, in the order they were built.
  const std::string all = instrumentNames();
  std::vector<std::pair<std::string, std::vector<std::string>>> fams;
  for (size_t at = 0; at < all.size();) {
    const size_t e = std::min(all.find(", ", at), all.size());
    const std::string nm = all.substr(at, e - at);
    at = e + 2;
    if (listed.count(nm)) continue;
    const Instrument* in = findInstrument(nm);
    const std::string fam = in && in->family && *in->family ? in->family : "other";
    auto it = std::find_if(fams.begin(), fams.end(), [&](const auto& f) { return f.first == fam; });
    if (it == fams.end()) { fams.push_back({fam, {}}); it = fams.end() - 1; }
    it->second.push_back(nm);
  }
  out += "Not a General MIDI program of their own (the drum kit's on channel 10, variants, single stops)\n";
  for (const auto& f : fams) {
    out += "  " + f.first + "\n";
    wrap(f.second);
  }
  return out;
}

static int gPlayTranspose = 0;
void setPlayTranspose(int semitones) { gPlayTranspose = semitones; }

static std::vector<std::string> gOnly;  // (-only: the instruments heard)
static bool gOnlySkip = false;          // (-onlyskip: ... from just before the first of their notes)
void setOnly(const std::string& list, bool skip) {
  gOnlySkip = skip;
  gOnly.clear();
  for (size_t at = 0; at <= list.size();) {
    const size_t e = std::min(list.find(',', at), list.size());
    if (e > at) gOnly.push_back(list.substr(at, e - at));
    at = e + 1;
  }
}
static bool gPianist = false;
void setPianist(bool on) { gPianist = on; }
static double gPlayVolumeDb = 0;
void setPlayVolume(double dB) { gPlayVolumeDb = dB; }
static double gPlayStart = 0;
void setPlayStart(double seconds) { gPlayStart = std::max(seconds, 0.0); }

namespace {

// -pianist: each channel played wholly by a piano with dampers (the grand, the bright, the upright,
// the honky-tonk: not the dulcimer, the harpsichord, the electric pianos) pedalled by a plan from
// its notes (src/pianoplan.h); the file's own sustain pedal on that channel set aside, the plan's
// put among the events, to go the same way.
void planPianists(Song& song, bool baroque) {
  int prog[16] = {}, pianoN[16] = {}, otherN[16] = {};
  const Instrument* inst[16] = {};
  for (const Event& e : song.ev) {
    const int ch = e.status & 15, kind = e.status & 0xf0;
    if (kind == 0xc0) prog[ch] = e.a;
    if (kind != 0x90 || e.b == 0 || ch == 9) continue;
    const Instrument* in = forProgram(prog[ch]);
    if (in && grandPiano(*in) && std::strcmp(in->name, "dulcimer") != 0) { ++pianoN[ch]; inst[ch] = in; }
    else ++otherN[ch];
  }
  const double qpb = song.barTicks > 0 ? double(song.barTicks) / song.div : 4.0;
  std::vector<Event> add;
  bool any = false;
  for (int ch = 0; ch < 16; ++ch) {
    if (!pianoN[ch] || otherN[ch]) continue;
    std::vector<PianoNote> notes;
    std::map<int, std::vector<size_t>> open;  // key -> its notes held (in notes)
    int own = 0;
    for (const Event& e : song.ev) {
      if ((e.status & 15) != ch) continue;
      const int kind = e.status & 0xf0;
      if (kind == 0xb0 && e.a == 64) ++own;
      if (kind == 0x90 && e.b > 0) {
        open[e.a].push_back(notes.size());
        notes.push_back({e.t, e.t, double(e.tick) / song.div, long(e.tick), e.a, e.b});
      } else if ((kind == 0x80 || kind == 0x90) && !open[e.a].empty()) {
        notes[open[e.a].front()].off = e.t;
        open[e.a].erase(open[e.a].begin());
      }
    }
    for (auto& [k, v] : open)
      for (size_t i : v) notes[i].off = song.ev.back().t;
    char who[64];
    std::snprintf(who, sizeof who, "channel %d (%s)", ch + 1, inst[ch]->name);
    const PianoPedalPlan plan = planPedal(notes, qpb, baroque, 1 + ch, who);
    std::fprintf(stderr, "%s", plan.summary.c_str());
    if (own) std::fprintf(stderr, "  (the file's own %d sustain-pedal events on it set aside)\n", own);
    const int logLevel = std::getenv("PJ_PIANIST_LOG") ? std::atoi(std::getenv("PJ_PIANIST_LOG")) : 0;
    if (logLevel > 0)
      for (const std::string& l : plan.log) std::fprintf(stderr, "  pianist ch %d: %s\n", ch + 1, l.c_str());
    if (logLevel > 1)  // (2: every pedal event, for a comparison with a pianist's)
      for (const PedalChange& c : plan.changes) std::fprintf(stderr, "  pianist ch %d: pedal %.4f %s\n", ch + 1, c.t, c.down ? "down" : "up");
    song.ev.erase(std::remove_if(song.ev.begin(), song.ev.end(),
                                 [&](const Event& e) { return (e.status & 15) == ch && (e.status & 0xf0) == 0xb0 && e.a == 64; }),
                  song.ev.end());
    for (const PedalChange& c : plan.changes) {
      Event e{std::max(0.0, c.t), 0, uint8_t(0xb0 | ch), 64, uint8_t(c.down ? 127 : 0)};
      e.tick = uint64_t(c.tick);
      add.push_back(e);
    }
    any = true;
  }
  if (!any) {
    std::fprintf(stderr, "pianist: no part played by a piano with dampers\n");
    return;
  }
  song.ev.insert(song.ev.end(), add.begin(), add.end());
  std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& a, const Event& b) { return a.t < b.t; });
}

}  // namespace

bool setReinstrument(const std::string& spec) {
  gReinstrument.clear();
  gReinstrumentShift.clear();
  gReinstrumentVoice.clear();
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
    // (TO:N: transposed N semitones; TO:VOICE: the lead singer's voice; both: voice-lead:tenor:-12)
    int shift = kNoShift;
    std::string voiceName;
    for (size_t colon = to.rfind(':'); colon != std::string::npos; colon = to.rfind(':')) {
      const std::string part = to.substr(colon + 1);
      char* endp = nullptr;
      const long v = std::strtol(part.c_str(), &endp, 10);
      if (!part.empty() && !*endp) {
        if (v < -60 || v > 60) {
          std::fprintf(stderr, "-reinstrument: %s: a transposition of -60 .. 60 semitones\n", rule.c_str());
          return false;
        }
        shift = int(v);
      } else {
        JazzSinger probe;
        if (!probe.setVoice(part)) {
          std::fprintf(stderr, "-reinstrument: %s: want TO:SEMITONES or TO:VOICE (profondo, bass, baritone, tenor, countertenor, contralto, mezzo, soprano, coloratura)\n",
                       rule.c_str());
          return false;
        }
        voiceName = part;
      }
      to = to.substr(0, colon);
    }
    if (to == "organ") to = "organ-plan";  // (the church organ by its plan)
    const Instrument* in = findInstrument(to.c_str());
    if (!in) {
      std::fprintf(stderr, "-reinstrument: no instrument %s\n", to.c_str());
      return false;
    }
    if ((shift != kNoShift || !voiceName.empty()) && std::strcmp(in->name, "voice-lead") != 0) {
      std::fprintf(stderr, "-reinstrument: %s: only voice-lead takes a transposition or a voice so far\n", rule.c_str());
      return false;
    }
    gReinstrument.push_back({from, in});
    gReinstrumentShift.push_back(shift);
    gReinstrumentVoice.push_back(voiceName);
    if (shift != kNoShift) std::fprintf(stderr, "  reinstrumented: %s on %s, %+d semitones\n", from.c_str(), in->name, shift);
    else std::fprintf(stderr, "  reinstrumented: %s on %s\n", from.c_str(), in->name);
  }
  return true;
}

namespace {

// One player of the song: a channel's instrument (or a drum's), as a pool.
struct Part {
  const Instrument* inst;
  std::unique_ptr<Ensemble> ens;
  float gmGain = 1;  // the program's level at the fader (src/gmlevels_data.h)
  float dirGain = 1;  // (a wind: the director's level as the player's loudness, PJ_DIRECTOR_WINDDB)
  // An organ played by a plan (organ-plan: src/organplan.h): its tracks to
  // the Pedal and the manuals, its registration by the bar.
  std::unique_ptr<OrganPlan> plan;
  size_t planNext = 0;
  std::map<int, int> manualOf;               // track -> division
  std::map<int, std::pair<int, int>> organKey;  // channel * 128 + key -> (division, key)
  // A Tonewheel by its plan (src/tonewheelplan.h): each note's division, the drawbars for the piece.
  std::unique_ptr<TonewheelPlan> hplan;
  // The brass section (src/brasssect.h): the section the part stands for, the song's harmony per onset.
  std::unique_ptr<BrassPlan> bplan;
  // The lead voice's singer, who reads the part ahead and sings it (src/jazzsinger.h): the part's
  // keys do not go to its ensemble, the singer's sound is heard from a place in it.
  std::unique_ptr<JazzSinger> singer;
  float singerSide = 0;  // where it stands, -1 .. 1 of the stage's half width
  std::map<int, int> tonewheelDiv;             // channel * 128 + key -> division (held notes)
  size_t rotaryNext = 0;                     // the plan's next Rotary change
  size_t lvibNext = 0;                       // the plan's next lower vibrato tab switch
  size_t percNext = 0;                       // the plan's next percussion switch
  size_t regNext = 0;                        // the plan's next re-registration
  size_t boostNext = 0;                      // the plan's next Rotary nudge
  size_t thinNext = 0;                       // the plan's next trill thinning
  size_t runNext = 0, swellNext = 0;         // the plan's next run switch, expression shape point
  // The plan's added notes (a glissando's) as events (tick, on, pitch, velocity, division), and
  // the next of them; the notes let go early for the percussion (tick, channel * 128 + key).
  std::vector<std::tuple<long, int, int, int, int>> hx;
  size_t hxNext = 0;
  std::vector<std::pair<long, int>> hrel;
  std::map<int, int> tonewheelChans;           // channel -> its notes (a Tonewheel playing several)
  int maxPoly = 1, maxHeld = 1, played = 0;  // (voices: notes held at once, their tails counted)
  double vol = 100 / 127.0, expr = 1;
  std::vector<float> l, r;
};

}  // namespace

// The lines playing with the church organ (src/organplan.h: is it their continuo, and how large are they).
static OrganCompany organCompany(const Song& song) {
  OrganCompany c;
  std::map<int, std::pair<int, uint64_t>> on;  // channel * 128 + key -> (kind, tick)
  int progs[16] = {};
  for (const Event& e : song.ev) {
    const int ech = e.status & 15, kind = e.status & 0xf0;
    if (kind == 0xc0) progs[ech] = e.a;
    if (kind != 0x90 && kind != 0x80) continue;
    if (ech != 9) {
      const Instrument* in = forProgram(progs[ech]);
      if (in && std::strcmp(in->name, "organ-plan") == 0) continue;
    }
    if (kind == 0x90 && e.b > 0) {
      const int pg = progs[ech];
      on[ech * 128 + e.a] = {ech == 9 || pg == 47 ? 2 : pg >= 56 && pg <= 63 ? 1 : pg >= 52 && pg <= 54 ? 3 : 0, e.tick};
    } else {
      auto it = on.find(ech * 128 + e.a);
      if (it == on.end()) continue;
      c.notes.push_back({long(it->second.second), long(e.tick), ech * 1000 + e.track, it->second.first});
      on.erase(it);
    }
  }
  return c;
}

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
  // Two channels (or more) on the grand piano: most likely two pianos, and no two grands are
  // alike: the first of those channels plays the bright piano (General MIDI 2, a grand too), the
  // others the grand. (The user's, 2026-11-24. PJ_PLAY_TWOGRANDS=0: all on the one grand's model.)
  if (knob("PJ_PLAY_TWOGRANDS", 1) != 0 && findInstrument("bright-piano")) {
    int prog[16] = {}, grandN[16] = {};
    for (const Event& e : song.ev) {
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) prog[ch] = e.a;
      if (kind == 0x90 && e.b > 0 && ch != 9 && prog[ch] == 0) ++grandN[ch];
    }
    int first = -1, n = 0;
    for (int ch = 0; ch < 16; ++ch)
      if (grandN[ch]) { if (first < 0) first = ch; ++n; }
    if (n >= 2) {
      for (Event& e : song.ev)
        if (e.status == (0xc0 | first) && e.a == 0) e.a = 1;
      // (and from the start: a channel is on program 1 until told otherwise)
      Event pc{0.0, -1, uint8_t(0xc0 | first), uint8_t(1), uint8_t(0)};
      song.ev.insert(song.ev.begin(), pc);
      std::fprintf(stderr, "  (%d channels on the grand piano: channel %d plays the bright piano)\n", n, first + 1);
    }
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
    gJazzUpright = jazzBandFile() = jazzBand;
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
    std::vector<CymbalSwirl> sw = planCymbalSwirls(notes, tm, song.gs);
    // The drummer listens (the author, 2026-11-26: "how loud is the dynamic at any given place and the
    // velocity may need to be interpreted within that context -- especially ... swooshing on the cymbal
    // can easily be too loud"): how hard the pitched parts play around a moment (their keys' mean
    // velocity from a second before to a second after; none there: the song's).
    auto bandVel = [&](double t0, double t1) {
      double sum = 0, all = 0;
      int n = 0, na = 0;
      for (const DrumPlanNote& nn : notes) {
        if (nn.drum) continue;
        all += nn.velocity; ++na;
        if (nn.t >= t0 - 1 && nn.t <= t1 + 1) { sum += nn.velocity; ++n; }
      }
      return n >= 3 ? sum / n : na ? all / na : 64.0;
    };
    // (a swirl is a decoration under the band: no louder than PJ_SWIRL_UNDER, 0.7, of what the band plays there)
    {
      static const double kUnder = knob("PJ_SWIRL_UNDER", 0.7);
      for (CymbalSwirl& q : sw) {
        const int cap = std::max(12, int(kUnder * bandVel(q.on, q.off)));
        if (q.velocity > cap) q.velocity = cap;
      }
    }
    // The ending (the author, 2026-11-26: "the swoosh has to stop before the final note, it cannot
    // continue over it, perhaps there can be a snare, kick and crash to indicate the end instead").
    // The final note: the last notes of the pitched parts, from the first of those begun within a
    // second of the very last. A swirl is over two bars before it (a swirl left under a second
    // long is not played at all), and with brushes the drummer marks the final note with the snare,
    // the bass drum and a crash, unless the file's own drums already strike there. PJ_SWIRL_END=0: as before.
    SongClock::get().brushEnd = 1e18;
    if (knob("PJ_SWIRL_END", 1) > 0) {
      // (the final note: the last chord, three or more pitched notes begun together (each within 60 ms of
      // the one before) in the song's last three seconds, a pickup into it or a late grace note after it
      // not counted; no such chord: the first note begun within a second of the very last)
      double last = -1, final = -1;
      std::vector<double> ons;
      for (const DrumPlanNote& nn : notes)
        if (!nn.drum) ons.push_back(nn.t);
      std::sort(ons.begin(), ons.end());
      if (!ons.empty()) last = ons.back();
      for (size_t e = ons.size(); e > 0 && final < 0 && ons[e - 1] >= last - 3.0;) {
        size_t b = e - 1;
        while (b > 0 && ons[b] - ons[b - 1] <= 0.06) --b;
        if (e - b >= 3) final = ons[b];
        e = b;
      }
      for (const DrumPlanNote& nn : notes)
        if (final < 0 && !nn.drum && nn.t >= last - 1.0) final = nn.t;
      const bool drummer = jazzBand || brushKitWanted() || !sw.empty();
      if (final > 0) {
        const double bar = 4 * (song.tempo.empty() ? 0.5 : song.tempo.back().len);
        // (the snare's own circling brush, and a swirl the file holds, stop there too: any band, any kit)
        SongClock::get().brushEnd = final - 2 * bar;
        std::fprintf(stderr, "  (the brushes: the final note at %.1f s; no brush swirls from %.1f s, two bars before it)\n", final, final - 2 * bar);
      }
      if (final > 0 && drummer) {
        const double bar = 4 * (song.tempo.empty() ? 0.5 : song.tempo.back().len);
        for (size_t q = 0; q < sw.size();) {
          if (sw[q].off > final - 2 * bar) sw[q].off = final - 2 * bar;
          if (sw[q].off - sw[q].on < 1.0) {
            std::fprintf(stderr, "  (the brushes: no swirl into the ending, from %.1f s)\n", sw[q].on);
            sw.erase(sw.begin() + q);
          } else ++q;
        }
        // A proper ending for the drummer of a jazz band (JazzDrumming.md): the file's time-keeping
        // does not run on over the final note (with brushes every such stroke is a sweep, a swoosh
        // under the last chord). Nothing of the drums after the final note; the beat before it left
        // open (the breath); before that a setup figure, three snare strokes in a triplet growing
        // into it, so that the final hit is no surprise.
        if (jazzBand || brushKitWanted()) {
          const double beat = bar / 4;
          int after = 0, open = 0;
          for (Event& e : song.ev) {
            if ((e.status & 15) != 9 || (e.status & 0xf0) != 0x90 || e.b == 0) continue;
            if (e.t > final + 0.03) { e.skip = true; ++after; }
            else if (e.t > final - 0.95 * beat && e.t < final - 0.03) { e.skip = true; ++open; }
          }
          uint64_t tick = 0;
          for (const Event& e : song.ev)
            if (e.t <= final - 2 * beat) tick = e.tick;
          for (int q = 0; q < 3; ++q) {
            const double t = final - 2 * beat + q * beat / 3;
            song.ev.push_back({t, 0, uint8_t(0x99), uint8_t(38), uint8_t(std::clamp(int(bandVel(final - 1, final)) - 14 + 9 * q, 24, 100))});
            song.ev.back().tick = tick;
            song.ev.push_back({t + 0.08, 0, uint8_t(0x89), uint8_t(38), uint8_t(0)});
            song.ev.back().tick = tick;
          }
          std::fprintf(stderr, "  (the ending: a triplet on the snare into the last beat, the beat before the final note left open (%d strokes), nothing after it (%d strokes))\n",
                       open, after);
        }
        bool struck = false;
        for (const DrumPlanNote& nn : notes)
          if (nn.drum && std::fabs(nn.t - final) < 0.05 && (nn.key == 35 || nn.key == 36 || nn.key == 38 || nn.key == 40 || nn.key == 49 || nn.key == 57)) struck = true;
        if (!struck && (jazzBand || brushKitWanted())) {
          uint64_t tick = 0;
          for (const Event& e : song.ev)
            if (e.t <= final) tick = e.tick;
          // (as loud as the band's last notes ask: a little over them, the crash under them)
          const int lv = int(bandVel(final - 1, final));
          const int hit[3][2] = {{36, std::clamp(lv + 14, 40, 110)}, {38, std::clamp(lv + 8, 36, 104)}, {49, std::clamp(lv - 6, 28, 92)}};
          for (const auto& h : hit) {
            song.ev.push_back({final, 0, uint8_t(0x99), uint8_t(h[0]), uint8_t(h[1])});
            song.ev.back().tick = tick;
            song.ev.push_back({final + 0.25, 0, uint8_t(0x89), uint8_t(h[0]), uint8_t(0)});
            song.ev.back().tick = tick;
          }
          std::fprintf(stderr, "  (the ending: the snare, the bass drum and a crash on the final note, %.1f s)\n", final);
          std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& a, const Event& b) { return a.t < b.t; });
        }
      }
    }
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
  // The cowbell player (cowbellplan.h): each key 56 a stroke it chooses, on the mouth or toward
  // the crown, open or muted by the hand, from the meter and the band; sent as its own keys.
  {
    std::vector<DrumPlanNote> notes;
    std::vector<size_t> bell;
    int pr[16] = {};
    for (size_t i = 0; i < song.ev.size(); ++i) {
      const Event& e = song.ev[i];
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pr[ch] = e.a;
      else if (kind == 0x90 && e.b > 0 && !e.skip) {
        if (ch == 9 && e.a == 56) bell.push_back(i);
        notes.push_back({e.t, e.a, e.b, ch == 9, kitOf(pr[9])});
      }
    }
    if (!bell.empty()) {
      std::vector<DrumTempo> tm;
      for (const Song::Tempo& x : song.tempo) tm.push_back({x.t, x.beat, x.len});
      std::string said;
      const std::vector<CowbellStroke> st = planCowbell(notes, tm, &said);
      int sent = 56;
      for (size_t i = 0, j = 0; i < song.ev.size(); ++i) {
        Event& e = song.ev[i];
        if ((e.status & 15) != 9 || e.a != 56) continue;
        const int kind = e.status & 0xf0;
        if (j < bell.size() && i == bell[j]) {
          if (st[j].skip) e.skip = true;
          sent = e.a = uint8_t(st[j].key);
          e.b = uint8_t(st[j].velocity);
          ++j;
        } else if (kind == 0x80 || (kind == 0x90 && e.b == 0)) {
          e.a = uint8_t(sent);
        }
      }
      if (!said.empty()) std::fprintf(stderr, "%s\n", said.c_str());
    }
  }
  // The director (director.h, Director.md): one reading of the piece for every part. It works
  // on the raw song, before the leads and the plans: the tempo map is replaced by its own (the
  // ritardando, the fermatas) and every event's time found again from its tick; the notes'
  // releases are moved for the articulation; each note-on is given its cue; the instruments
  // whose loudness is their key velocity (a piano, a pluck) get the level and the emphasis there.
  Direction dir;
  int ownCueFrom = 1 << 30;  // (the cues the file player adds of its own, from here: applied whatever the director does)
  std::vector<std::pair<double, float>> dirLevel[16];  // each channel's level curve, in seconds
  std::vector<std::pair<double, float>> ownLevel[16];  // (a wind's or a string's long notes shaped by its player: added to it)
  {
    DirInput di;
    di.div = song.div;
    di.barTicks = song.barTicks;
    di.tonic = song.tonic;
    di.minor = song.minor;
    for (const Song::Tempo& x : song.tempo) di.tempo.push_back({x.t, x.beat, x.len});
    struct Span { size_t on, off; const Instrument* inst; };
    std::vector<Span> spans;
    std::map<int, size_t> open;  // channel * 128 + key -> its span
    int pr[16] = {};
    for (size_t i = 0; i < song.ev.size(); ++i) {
      const Event& e = song.ev[i];
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pr[ch] = e.a;
      if ((kind == 0xb0 && (e.a == 11 || e.a == 1 || e.a == 2)) || kind == 0xd0) di.ridden[ch] = true;
      if (kind == 0x90 && e.b > 0) {
        if (ch == 9) di.drums.push_back({long(e.tick), e.a});
        else di.program[pr[ch] & 127] = true;
      }
      if (ch == 9 || (kind != 0x90 && kind != 0x80)) continue;
      auto it = open.find(ch * 128 + e.a);
      if (it != open.end()) { spans[it->second].off = i; open.erase(it); }
      if (kind == 0x90 && e.b > 0) {
        open[ch * 128 + e.a] = spans.size();
        spans.push_back({i, i, forProgram(pr[ch])});
        // (a wind player's line: a flute, a recorder, a reed, a brass; not a bagpipe's drone-bound chanter)
        if (const Instrument* w = spans.back().inst)
          di.wind[ch] |= (w->kind == Kind::kFlute || w->kind == Kind::kDuct || w->kind == Kind::kEndBlown || w->reed() ||
                          w->lip()) && std::strcmp(w->family, "bagpipe") != 0;
        if (const Instrument* w = spans.back().inst) di.pluck[ch] |= harpsichord(*w);
        if (const Instrument* w = spans.back().inst)
          di.flute[ch] |= w->kind == Kind::kFlute || w->kind == Kind::kDuct || w->kind == Kind::kEndBlown;
      }
    }
    std::vector<size_t> order;
    for (size_t k = 0; k < spans.size(); ++k)
      if (spans[k].off != spans[k].on && song.ev[spans[k].off].tick > song.ev[spans[k].on].tick) order.push_back(k);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return song.ev[spans[a].on].tick < song.ev[spans[b].on].tick; });
    for (size_t k : order) {
      const Event& on = song.ev[spans[k].on];
      di.notes.push_back({long(on.tick), long(song.ev[spans[k].off].tick), on.status & 15, on.a, on.b});
    }
    di.jazzBand = jazzBand;
    dir = direct(di);
    std::fprintf(stderr, "%s\n", dir.summary.c_str());
    jazzPiece() = dir.jazz;
    ampedPiece() = dir.amped;
    rotaryRoom() = dir.sparse || dir.ballad;
    std::fprintf(stderr, "  (the director: %s)\n", dir.jazzWhy.c_str());
    // The flutes of a Baroque piece played on the Baroque flute (PJ_BAROQUE_FLUTE, 1; 0: never,
    // 2: always where it can): the director read the piece as Baroque, or the file is named by a
    // Baroque catalogue (BWV, HWV, TWV, BuxWV, RV: Bach, Handel, Telemann, Buxtehude, Vivaldi). Only
    // if every flute note is in its compass (D4 .. D6: it has no third octave, and a note folded
    // down is a wrong note), and not when -reinstrument says what the flute is.
    {
      static const double kBf = knob("PJ_BAROQUE_FLUTE", 1.0);
      std::string base = path.substr(path.find_last_of('/') + 1);
      for (char& ch : base) ch = char(std::tolower((unsigned char)ch));
      bool catalogue = false;
      for (const char* cat : {"bwv", "hwv", "twv", "buxwv", "rv"})
        for (size_t at = base.find(cat); at != std::string::npos && !catalogue; at = base.find(cat, at + 1)) {
          size_t k = at + std::strlen(cat);
          while (k < base.size() && (base[k] == ' ' || base[k] == '_' || base[k] == '-' || base[k] == '.')) ++k;
          catalogue = k < base.size() && std::isdigit((unsigned char)base[k]) && (at == 0 || !std::isalpha((unsigned char)base[at - 1]));
        }
      bool ruled = false;
      for (const auto& [from, to] : gReinstrument) ruled |= from == "*" || from == "flute";
      const Instrument* bf = findInstrument("baroque-flute");
      if (kBf > 0 && bf && !ruled && (kBf >= 2 || dir.style == Direction::kBaroque || catalogue)) {
        int flutes = 0, lo = 127, hi = 0;
        for (const Span& sp : spans)
          if (sp.inst && std::strcmp(sp.inst->name, "flute") == 0) {
            ++flutes;
            lo = std::min(lo, int(song.ev[sp.on].a));
            hi = std::max(hi, int(song.ev[sp.on].a));
          }
        if (flutes > 0 && lo >= bf->lo && hi <= bf->hi) {
          gBaroqueFlute = bf;
          std::fprintf(stderr, "  the flutes on the Baroque flute (%s; their compass %d .. %d)\n",
                       dir.style == Direction::kBaroque ? "a Baroque reading" : catalogue ? "a Baroque catalogue's number" : "PJ_BAROQUE_FLUTE 2", lo, hi);
        } else if (flutes > 0) {
          std::fprintf(stderr, "  (the flutes stay modern: their notes %d .. %d beyond the Baroque flute's %d .. %d)\n", lo, hi, bf->lo, bf->hi);
        }
      }
    }
    // (the times must be the ticks' through the tempo map: so they are in a file as read)
    auto secAt = [](const std::vector<DirTempo>& tm, int div, uint64_t tick) {
      const double b = double(tick) / div;
      size_t k = 0;
      while (k + 1 < tm.size() && tm[k + 1].beat <= b) ++k;
      return tm.empty() ? b * 0.5 : tm[k].t + (b - tm[k].beat) * tm[k].len;
    };
    bool byTicks = true;
    for (const Event& e : song.ev) byTicks &= std::fabs(secAt(di.tempo, song.div, e.tick) - e.t) < 2e-3;
    if (dir.self && !byTicks) {
      std::fprintf(stderr, "  (the winds' own reading: the events' times are not their ticks': left out)\n");
      dir.self = false;
    }
    if (dir.acts && !byTicks) {
      std::fprintf(stderr, "  (director: the events' times are not their ticks': it stays out)\n");
      dir.acts = false;
    }
    // The vibrato for every note (Direction::vib): a cue of its own holding only the director's
    // vibrato, everything else as written; where the reading below gives a note its whole cue,
    // that replaces it. (Not the organs, with their own plans; not the drums.)
    if (dir.vib) {
      const size_t nq = std::min(order.size(), dir.cues.size());
      int planned = 0;
      for (size_t q = 0; q < nq; ++q) {
        const Span& sp = spans[order[q]];
        Event& on = song.ev[sp.on];
        if ((on.status & 15) == 9 || (sp.inst && (std::strcmp(sp.inst->family, "organ") == 0 || isTonewheel(sp.inst)))) continue;
        Cue v{};
        v.v[kCueLevel] = v.v[kCueLevelEnd] = 0.5f;
        v.v[kCueSwellPeak] = 0.5f;
        v.v[kCueLength] = 1;
        for (int k : {int(kCueVibDepth), int(kCueVibDelay), int(kCueVibRate), int(kCuePhraseEnd), int(kCueTongue)}) v.v[k] = dir.cues[q].v[k];
        v.v[kCueOnly] = 1;
        on.cue = int(dir.cues.size());
        dir.cues.push_back(v);
        ++planned;
      }
      std::fprintf(stderr, "  the director's vibrato and phrase ends: planned for every part's notes (%d)%s\n", planned,
                   dir.acts ? "" : dir.self ? ", the winds' read lines their whole reading" : ", only that");
    }
    if (dir.acts) {
      for (size_t q = 0; q < order.size(); ++q) {
        const Span& sp = spans[order[q]];
        const Cue& c = dir.cues[q];
        Event& on = song.ev[sp.on];
        Event& off = song.ev[sp.off];
        if (q < dir.onShift.size()) {  // (a trill played alive, the notes inégales: ticks moved; the organs too)
          on.tick = uint64_t(std::max(0L, long(on.tick) + dir.onShift[q]));
          off.tick = std::max(on.tick + 1, uint64_t(std::max(0L, long(off.tick) + dir.offShift[q])));
        }
        // (the organs articulate and balance by their own plans: they take the shared time only)
        if (sp.inst && (std::strcmp(sp.inst->family, "organ") == 0 || isTonewheel(sp.inst))) continue;
        on.cue = int(q);
        if (c.v[kCueLength] != 1) off.tick = on.tick + uint64_t(std::max(1.0, double(off.tick - on.tick) * c.v[kCueLength]));  // (over 1: a harpsichord's over-legato, the director's)
        // Key velocity: the emphasis for all; the level too where velocity is the loudness.
        double g = 1 + 0.22 * c.v[kCueAccent];
        if (sp.inst && !velocityAsGain(*sp.inst)) g *= 1 + 1.1 * (c.v[kCueLevel] - 0.5);
        on.b = uint8_t(std::clamp(int(std::lround(on.b * g)), 1, 127));
      }
      // Mutes: a muted trumpet is another instrument (General MIDI 60): the trumpets' channels
      // change program where the mutes go on and come off. (The strings take theirs from the cue.)
      {
        std::set<int> trumpets;
        for (const Span& sp : spans)
          if (sp.inst && std::strcmp(sp.inst->name, "trumpet") == 0) trumpets.insert(song.ev[sp.on].status & 15);
        std::vector<Event> pc;
        for (int ch : trumpets)
          for (auto& [tick, on] : dir.mutes) {
            Event e{0, 0, uint8_t(0xc0 | ch), uint8_t(on ? 59 : 56), 0};
            e.tick = uint64_t(tick);
            pc.push_back(e);
          }
        song.ev.insert(song.ev.begin(), pc.begin(), pc.end());
      }
      song.tempo.clear();
      for (const DirTempo& x : dir.tempo) song.tempo.push_back({x.t, x.beat, x.len});
      for (Event& e : song.ev) e.t = secAt(dir.tempo, song.div, e.tick);
      std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& a, const Event& b) { return a.t < b.t; });
      for (int ch = 0; ch < 16; ++ch)
        for (auto& [tick, lv] : dir.level[ch]) dirLevel[ch].push_back({secAt(dir.tempo, song.div, uint64_t(tick)), lv});
    } else if (dir.self) {
      // The winds' own reading: their notes' cues (the emphasis, the vibrato; a quantized file's
      // lengths, its breaths and sighs: a played one keeps its own but for the breaths) and their
      // levels, at the file's own times.
      bool moved = false;
      for (size_t q = 0; q < order.size(); ++q) {
        const Span& sp = spans[order[q]];
        const Event& on0 = song.ev[sp.on];
        const int ch = on0.status & 15;
        if (dir.level[ch].empty()) continue;  // (not a wind line of its own)
        const Cue& c = dir.cues[q];
        Event& on = song.ev[sp.on];
        Event& off = song.ev[sp.off];
        on.cue = int(q);
        if (dir.gridded && q < dir.onShift.size() && (dir.onShift[q] || dir.offShift[q])) {
          on.tick = uint64_t(std::max(0L, long(on.tick) + dir.onShift[q]));
          off.tick = std::max(on.tick + 1, uint64_t(std::max(0L, long(off.tick) + dir.offShift[q])));
          on.t = secAt(di.tempo, song.div, on.tick);
          off.t = secAt(di.tempo, song.div, off.tick);
          moved = true;
        }
        const float len = dir.gridded ? c.v[kCueLength] : std::min(1.0f, std::max(c.v[kCueLength], c.v[kCueLegato] > 0 ? 1.0f : 0.83f));
        if (len < 1 && off.t > on.t) {
          off.t = on.t + (off.t - on.t) * len;
          off.tick = on.tick + uint64_t(std::max(1.0, double(off.tick - on.tick) * len));
          moved = true;
        }
        if (q < dir.keyVel.size() && dir.keyVel[q] > 0) on.b = uint8_t(dir.keyVel[q]);  // (its line is in the level: src/velline.h)
        on.b = uint8_t(std::clamp(int(std::lround(on.b * (1 + 0.22 * c.v[kCueAccent]))), 1, 127));
      }
      if (moved) std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& a, const Event& b) { return a.t < b.t; });
      for (int ch = 0; ch < 16; ++ch)
        for (auto& [tick, lv] : dir.level[ch]) dirLevel[ch].push_back({secAt(di.tempo, song.div, uint64_t(tick)), lv});
    }
  }
  // What each cued note's player hears around it (director.h, kCueOwn*), for its own shaping
  // where the cue gives none: the note's length, the band's activity while it sounds (the other
  // parts' note-ons per second, a drum stroke counting half), and whether it leads up into the
  // part's next note (higher, harder, the band busier after it).
  // (A pan flute's notes, the bottles', get a cue of that alone where the director gives none, even
  // with -director off: the shaping is the musician's own; no vibrato or level in it.)
  {
    ownCueFrom = int(dir.cues.size());
    struct N { double on, off; int ch, key, vel, cue; const Instrument* inst; };
    std::vector<N> ns;
    std::vector<std::pair<double, int>> ons;  // (time, channel)
    std::map<int, size_t> open;
    int pr[16] = {};
    for (Event& e : song.ev) {
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pr[ch] = e.a;
      if (kind != 0x90 && kind != 0x80) continue;
      const bool on = kind == 0x90 && e.b > 0;
      if (on && e.skip) continue;
      if (on && ch != 9 && e.cue < 0) {
        const Instrument* in = forProgram(pr[ch]);
        if (in && in->design && in->design->pipes) {
          Cue v{};
          v.v[kCueLevel] = v.v[kCueLevelEnd] = v.v[kCueSwellPeak] = 0.5f;
          v.v[kCueLength] = v.v[kCueVibRate] = 1;
          v.v[kCueOnly] = 1;
          e.cue = int(dir.cues.size());
          dir.cues.push_back(v);
        }
      }
      if (on) {
        ons.push_back({e.t, ch});
        if (ch == 9) continue;
        open[ch * 128 + e.a] = ns.size();
        ns.push_back({e.t, e.t + 1, ch, e.a, e.b, e.cue, forProgram(pr[ch])});
      } else if (ch != 9) {
        auto it = open.find(ch * 128 + e.a);
        if (it != open.end()) { ns[it->second].off = std::max(ns[it->second].on, e.t); open.erase(it); }
      }
    }
    std::sort(ons.begin(), ons.end());
    auto busy = [&](double t0, double t1, int ch) {
      double c = 0;
      for (auto it = std::lower_bound(ons.begin(), ons.end(), std::make_pair(t0, -1)); it != ons.end() && it->first < t1; ++it)
        if (it->second != ch) c += it->second == 9 ? 0.5 : 1;
      return c / std::max(0.25, t1 - t0);
    };
    std::vector<int> nextOf(ns.size(), -1), last(16, -1);
    int held = 0, shaped = 0;
    // (a part that plays chords: no note of it shaped; one that plays a line, its notes overlapping
    // under 15 % of the time by more than 50 ms)
    int chN[16] = {}, chOver[16] = {};
    {
      double end[16];
      std::fill(end, end + 16, -1e9);
      for (const N& x : ns) {
        ++chN[x.ch];
        chOver[x.ch] += x.on < end[x.ch] - 0.05;
        end[x.ch] = std::max(end[x.ch], x.off);
      }
    }
    int run[16] = {};
    double runEnd[16];
    std::fill(runEnd, runEnd + 16, -1e9);
    static const double kOwnExp = knob("PJ_OWN_EXPAND", 0.08), kOwnCresc = knob("PJ_OWN_CRESC", 0.12), kOwnDecr = knob("PJ_OWN_DECR", 0.06);
    Rng ownRng(4242);
    for (int k = int(ns.size()) - 1; k >= 0; --k) {
      nextOf[k] = last[ns[k].ch];
      last[ns[k].ch] = k;
    }
    for (size_t k = 0; k < ns.size(); ++k) {
      const N& x = ns[k];
      // (a wind's or a string's note the director left uncued is still shaped by its player: a
      // cue of the context alone, for that, not given to the player)
      const Instrument* xi = x.inst;
      const bool singing = xi && (((xi->kind == Kind::kFlute || xi->kind == Kind::kDuct || xi->kind == Kind::kEndBlown || xi->reed() || xi->lip()) &&
                                   std::strcmp(xi->family, "bagpipe") != 0) || bowedBass(*xi));
      Cue scratch{};
      scratch.v[kCueLevel] = scratch.v[kCueLevelEnd] = scratch.v[kCueSwellPeak] = 0.5f;
      scratch.v[kCueLength] = scratch.v[kCueVibRate] = scratch.v[kCueOnly] = 1;
      const bool cued = x.cue >= 0 && x.cue < int(dir.cues.size());
      if (!cued && !singing) continue;
      Cue& c = cued ? dir.cues[x.cue] : scratch;
      const double len = x.off - x.on, b = busy(x.on + 0.02, x.off, x.ch);
      double ramp = 0;
      // (the part's next note: the first begun after this one, within 0.6 s of its end)
      int nx = nextOf[k];
      while (nx >= 0 && ns[nx].on <= x.on + 0.02) nx = nextOf[nx];
      if (nx >= 0 && ns[nx].on - x.off < 0.6) {
        const N& y = ns[nx];
        const bool up = y.key - x.key >= 2, harder = y.vel - x.vel >= 6;
        const double after = busy(y.on, y.on + std::max(1.0, std::min(len, 3.0)), x.ch);
        if (up || harder) ramp = 0.4 * up + 0.3 * harder + 0.3 * (after > 1.3 * b + 0.5);
      }
      // Considerate, so that a long note never turns comical: only a line's notes (none of a part
      // that plays chords); no growing for a pad's note (two parts or more beginning long notes with
      // it: a chord held), nor expanding under a busy melody above it (it accompanies), and a crescendo
      // only into a clear rise; in a run of long notes close together each next one shaped less.
      const bool line = chN[x.ch] > 0 && chOver[x.ch] < 0.15 * chN[x.ch];
      int pad = 0;
      bool padCh[16] = {};
      double above = 0;
      for (size_t j = 0; j < ns.size(); ++j) {
        const N& y = ns[j];
        if (y.ch == x.ch) continue;
        if (!padCh[y.ch] && std::fabs(y.on - x.on) < 0.15 && y.off - y.on >= 0.6 * len) { padCh[y.ch] = true; ++pad; }
        if (y.on > x.on + 0.02 && y.on < x.off && y.key > x.key + 2) above += 1;
      }
      above /= std::max(0.25, len);
      if (above >= 1.5) ramp *= 0.5;
      if (ramp < 0.6) ramp = 0;
      run[x.ch] = len >= 1.2 && x.on - runEnd[x.ch] < 1.0 ? run[x.ch] + 1 : 0;
      if (len >= 1.2) runEnd[x.ch] = x.off;
      const double grow = pad >= 2 ? 0.0 : run[x.ch] == 0 ? 1.0 : run[x.ch] == 1 ? 0.7 : 0.5;
      c.v[kCueOwnLen] = float(line ? len : 0.0);
      c.v[kCueOwnBusy] = float(above >= 1.5 ? 0.0 : b);
      c.v[kCueOwnRamp] = float(ramp);
      c.v[kCueOwnSize] = float(grow * (run[x.ch] >= 2 ? 0.7 : 1.0));
      // A long last note of a solo line (a phrase's, or the part's last) while the band plays on
      // (the others begin notes under it, or sound through its end) does not fade away: no taper,
      // its level kept to the end, and a wind's or a string's vibrato comes in over its later part.
      const Instrument* in = x.inst;
      const bool sings = in && (((in->kind == Kind::kFlute || in->kind == Kind::kDuct || in->kind == Kind::kEndBlown || in->reed() || in->lip()) &&
                                 std::strcmp(in->family, "bagpipe") != 0) || bowedBass(*in));
      static const bool kHoldLast = knob("PJ_HOLD_LAST", 1) > 0;
      bool hold = kHoldLast && sings && len >= 1.2 && (c.v[kCuePhraseEnd] > 0.5f || nx < 0 || ns[nx].on - x.off > 1.0);
      if (hold) {
        bool solo = true, under = b >= 1;
        for (size_t j = 0; j < ns.size() && (solo || !under); ++j) {
          if (j == k || ns[j].on >= x.off || ns[j].off <= x.on + 0.05) continue;
          if (ns[j].ch == x.ch) solo = false;
          else if (ns[j].on < x.off - 0.1 && ns[j].off > x.off - 0.05) under = true;
        }
        hold = solo && under;
      }
      if (hold) {
        c.v[kCuePhraseEnd] = 0;
        c.v[kCueLevelEnd] = std::max(c.v[kCueLevelEnd], c.v[kCueLevel]);
        if (c.v[kCueVibDepth] < 0.8f) {
          c.v[kCueVibDepth] = 1;
          c.v[kCueVibDelay] = float(std::max(double(c.v[kCueVibDelay]), std::min(0.45 * len, 1.5)));
          if (c.v[kCueVibRate] <= 0) c.v[kCueVibRate] = 1;
        } else {
          c.v[kCueVibDelay] = float(std::min(double(c.v[kCueVibDelay]), 0.5 * len));
        }
        // (the director's level curve kept from sinking under the note's start while it sounds)
        auto& lc = dirLevel[x.ch];
        if (!lc.empty()) {
          size_t q = 0;
          while (q + 1 < lc.size() && lc[q + 1].first <= x.on) ++q;
          float at = lc[q].second;
          if (q + 1 < lc.size() && x.on > lc[q].first)
            at += float((lc[q + 1].second - at) * (x.on - lc[q].first) / std::max(1e-9, lc[q + 1].first - lc[q].first));
          for (auto& pt : lc)
            if (pt.first > x.on && pt.first <= x.off + 0.05) pt.second = std::max(pt.second, at);
        }
        ++held;
      }
      // The other winds' and the strings' long notes (not the pan flute's, the bottles': their player
      // shapes its own), through the part's level, as the director's swells come (src/longnote.h).
      {
        const Instrument* wi = x.inst;
        const bool wind = wi && (wi->kind == Kind::kFlute || wi->kind == Kind::kDuct || wi->kind == Kind::kEndBlown || wi->reed() || wi->lip()) &&
                          std::strcmp(wi->family, "bagpipe") != 0 && !(wi->design && wi->design->pipes);
        if (wind || (wi && bowedBass(*wi))) {
          const LongShape sh = longShape(c, kOwnExp, kOwnCresc, kOwnDecr, 0.9, c.v[kCuePhraseEnd] > 0.5f && c.v[kCuePhraseEnd] < 1.5f, ownRng.uniform());
          if (sh.on && (sh.exp > 0.005 || sh.cresc > 0.005 || sh.decr > 0.005)) {
            auto& oc = ownLevel[x.ch];
            const double vd = c.v[kCueVibDepth] > 0 ? c.v[kCueVibDelay] : 1e9;  // (the vibrato's coming in: eases the sinking)
            double sink = 0;
            oc.push_back({x.on, 0.0f});
            for (double t = 0.05; t < len; t += 0.05) oc.push_back({x.on + t, float(longShapeAt(sh, t, 0.05, longSmooth(vd, vd + 0.4, t), sink))});
            oc.push_back({x.off + 0.06, 0.0f});
            ++shaped;
          }
        }
      }
    }
    if (held) std::fprintf(stderr, "  (long last notes of a solo line, the band playing on: %d held to their end, with vibrato)\n", held);
    if (shaped) std::fprintf(stderr, "  (long notes of the winds' and strings' lines shaped by their players: %d)\n", shaped);
    for (auto& oc : ownLevel) std::stable_sort(oc.begin(), oc.end());
  }
  // The tambourine player (tambplan.h, research/tambourine.md): each key 54 a hint that the
  // tambourine does something there; the player chooses what from the meter, the band and the
  // director's reading (after it: its tempo map and times), and may add a lead-in shake.
  {
    std::vector<DrumPlanNote> notes;
    std::vector<size_t> tamb;
    int pr[16] = {};
    for (size_t i = 0; i < song.ev.size(); ++i) {
      const Event& e = song.ev[i];
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pr[ch] = e.a;
      else if (kind == 0x90 && e.b > 0 && !e.skip) {
        if (ch == 9 && e.a == 54) tamb.push_back(i);
        notes.push_back({e.t, e.a, e.b, ch == 9, kitOf(pr[9])});
      }
    }
    tambourinePlan().clear();
    if (!tamb.empty()) {
      std::vector<DrumTempo> tm;
      for (const Song::Tempo& x : song.tempo) tm.push_back({x.t, x.beat, x.len});
      TambDirection td;
      td.jazz = dir.jazz;
      td.amped = dir.amped;
      bool kit = false;  // (no kit: the tambourine of an orchestra, a concert player's)
      for (const DrumPlanNote& nn : notes)
        kit |= nn.drum && (nn.key == 35 || nn.key == 36 || nn.key == 38 || nn.key == 40 || nn.key == 42 || nn.key == 44 || nn.key == 46);
      td.classical = !kit && !dir.jazz && !dir.amped;
      std::string said;
      std::vector<TambPlanned> pl = planTambourine(notes, tm, td, &said);
      for (size_t j = 0; j < tamb.size(); ++j) {
        Event& e = song.ev[tamb[j]];
        if (pl[j].skip) e.skip = true;
        else e.b = uint8_t(std::clamp(pl[j].velocity, 1, 127));
      }
      bool add = false;
      for (size_t j = tamb.size(); j < pl.size(); ++j) {
        uint64_t tick = 0;
        for (const Event& e : song.ev)
          if (e.t <= pl[j].t) tick = e.tick;
        song.ev.push_back({pl[j].t, 0, uint8_t(0x99), uint8_t(54), uint8_t(std::clamp(pl[j].velocity, 1, 127))});
        song.ev.back().tick = tick;
        song.ev.push_back({pl[j].t + 0.05, 0, uint8_t(0x89), uint8_t(54), uint8_t(0)});
        song.ev.back().tick = tick;
        add = true;
      }
      if (add) std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& a, const Event& b) { return a.t < b.t; });
      std::sort(pl.begin(), pl.end(), [](const TambPlanned& a, const TambPlanned& b) { return a.t < b.t; });
      tambourinePlan() = pl;
      std::fprintf(stderr, "%s\n", said.c_str());
    }
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
  if (gPianist) planPianists(song, dir.acts && dir.style == Direction::kBaroque);  // (-pianist: the pedalling planned)
  // -only: the notes of the other instruments left out, after the director has read the whole
  // piece (its tempo, phrasing and dynamics as with every part). A note-on decides; its note-off
  // and aftertouch follow it.
  if (!gOnly.empty()) {
    int pg[16] = {}, kept = 0, dropped = 0;
    double first = -1;  // (the first heard note's time)
    bool keep[16 * 128] = {};
    std::set<std::string> heard, left;
    auto wanted = [&](int ch, const Instrument* in) {
      for (const std::string& w : gOnly) {
        if (w == std::to_string(ch + 1) || w == "ch" + std::to_string(ch + 1)) return true;
        if (ch == 9 ? (w == "drums" || w == "drum") : in && (w == in->name || w == in->family)) return true;
      }
      return false;
    };
    std::vector<Event> out;
    out.reserve(song.ev.size());
    for (const Event& e : song.ev) {
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pg[ch] = e.a;
      if (kind == 0x90 && e.b > 0) {
        const Instrument* in = ch == 9 ? nullptr : forProgram(pg[ch]);
        const std::string nm = ch == 9 ? "drums" : in ? in->name : "?";
        keep[ch * 128 + e.a] = wanted(ch, in);
        (keep[ch * 128 + e.a] ? heard : left).insert(nm);
        if (!keep[ch * 128 + e.a]) { ++dropped; continue; }
        if (!kept++) first = e.t;
      } else if ((kind == 0x80 || kind == 0x90 || kind == 0xa0) && !keep[ch * 128 + e.a]) {
        continue;
      }
      out.push_back(e);
    }
    song.ev.swap(out);
    auto join = [](const std::set<std::string>& v) {
      std::string r;
      for (const std::string& x : v) r += (r.empty() ? "" : ", ") + x;
      return r.empty() ? std::string("nothing") : r;
    };
    std::fprintf(stderr, "  -only: %s heard (%d notes); %s left out (%d notes)\n", join(heard).c_str(), kept,
                 join(left).c_str(), dropped);
    if (!kept) { std::fprintf(stderr, "-only: no notes left to play\n"); return 1; }
    // -onlyskip: the song from PJ_ONLYSKIP_LEAD (0.5 s) before their first note (or -start_time, if later).
    if (gOnlySkip) {
      const double at = std::max(0.0, first - knob("PJ_ONLYSKIP_LEAD", 0.5));
      if (at > gPlayStart) gPlayStart = at;
      std::fprintf(stderr, "  -onlyskip: their first note at %d:%04.1f\n", int(first) / 60, std::fmod(first, 60.0));
    }
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
  // (and every channel on one Tonewheel program joins one Tonewheel: both hands and the pedals on one
  // organ through one Rotary, the channels its lines)
  int tonewheelPart = -1;  // (one organ: every channel of the Tonewheel family, whatever its program or remap)
  auto partOf = [&](int ch, int key, int* playKey) -> int {
    if (ch != 9) {
      const Instrument* in = forProgram(prog[ch]);
      if (in && std::strcmp(in->name, "organ-plan") == 0) {
        *playKey = key;
        if (organPart < 0) organPart = ch * 128 + prog[ch];
        return organPart;
      }
      if (in && std::strcmp(in->family, "tonewheel") == 0) {
        *playKey = key;
        if (tonewheelPart < 0) tonewheelPart = ch * 128 + prog[ch];
        return tonewheelPart;
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
  // An organ plays in a church, unless -room said otherwise. The timpani belong to a symphony hall
  // (research/timpani.md 12.8b): a piece with timpani plays there, and its organ is then a concert
  // hall's organ, not the timpani brought into a church.
  if (!roomGiven) {
    bool organ = false, timpani = false;
    for (auto& [id, p] : parts) {
      if (std::strcmp(p.inst->family, "organ") == 0) organ = true;
      if (id / 128 != 9 && id % 128 == 47) timpani = true;  // (General MIDI program 48, counted from 0)
    }
    if (timpani) {
      room = findRoom("concert");
      std::fprintf(stderr, organ ? "  (timpani and an organ: in the concert hall)\n" : "  (timpani: in the concert hall)\n");
    } else if (organ) {
      // (chamber music with an organ under it, a handful of lines: a room, and a positive organ)
      std::vector<OrganNote> on;
      int pg[16] = {};
      for (const Event& e : song.ev) {
        if ((e.status & 0xf0) == 0xc0) pg[e.status & 15] = e.a;
        const Instrument* in = (e.status & 15) == 9 ? nullptr : forProgram(pg[e.status & 15]);
        if ((e.status & 0xf0) == 0x90 && e.b > 0 && in && std::strcmp(in->name, "organ-plan") == 0)
          on.push_back({e.track, long(e.tick), long(e.tick) + song.div, e.a});
      }
      const OrganCompany company = organCompany(song);
      if (!on.empty() && organIsContinuo(on, company) && organCompanyIsChamber(company)) {
        room = findRoom("hall");
        std::fprintf(stderr, "  (chamber music over an organ's continuo: in the hall, on a positive organ)\n");
      } else {
        room = findRoom("church");
        std::fprintf(stderr, "  (an organ: in the church)\n");
      }
    }
  }
  std::map<const Instrument*, std::unique_ptr<Calibration>> cals;
  for (auto& [id, p] : parts) {
    if (!cals.count(p.inst)) {
      std::fprintf(stderr, "  calibrating the %s...\n", p.inst->name);
      cals[p.inst].reset(new Calibration(std::getenv("PJ_NOCAL") ? Calibration() : calibrate(*p.inst, 440, nullptr)));
    }
    static const int kMaxVoices = std::getenv("PJ_PLAY_VOICES") ? std::atoi(std::getenv("PJ_PLAY_VOICES")) : 32;
    const int poly = std::min(p.maxPoly, kMaxVoices);
    Ensemble::shareStrings(true);  // (a file's string parts: one string orchestra, src/stringsect.cc)
    p.ens.reset(new Ensemble(*p.inst, *cals[p.inst], poly, verbose, room));
    p.ens->setKeepTails(kTail);
    p.ens->setVelocityGain(true);
    p.gmGain = float(fastPow(10.0, gmLevelDb(p.inst->name) / 20));
    if (p.inst->oneVoice())
      std::fprintf(stderr, "  %s %2d: %s (%d at once, one voice)\n", id / 128 == 9 ? "drums  " : "channel", id / 128 + 1,
                   p.inst->name, p.maxHeld);
    else
      std::fprintf(stderr, "  %s %2d: %s (%d held at once; %d voices for their tails)\n", id / 128 == 9 ? "drums  " : "channel",
                   id / 128 + 1, p.inst->name, p.maxHeld, poly);
  }
  // A part-song's choir (PJ_CHOIR_SECTIONS, on): the choir channels that each sing one line (at
  // most two notes at once) are its voice parts. Each is named by its median pitch, the nearest of
  // the soprano's (A4), the alto's (D4), the tenor's (G3) and the bass's (C3): two lines on one
  // name a divided part (SSAATTBB: the second sopranos the mezzos, the first basses the
  // baritones). Each sings with its own section alone, standing in its own block, from the left
  // the highest line to the right the lowest: the harmony laid out across the stage.
  if (knob("PJ_CHOIR_SECTIONS", 1) > 0) {
    std::vector<std::pair<double, int>> lines;  // (median key, part id)
    std::map<int, std::vector<int>> keysOf;
    int pg[16] = {};
    for (const Event& e : song.ev) {
      const int ech = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pg[ech] = e.a;
      if (kind == 0x90 && e.b > 0) keysOf[ech * 128 + pg[ech]].push_back(e.a);
    }
    for (auto& [id, p] : parts) {
      // (not the lead voice: a few singers of the choir, not a section of it)
      if (id / 128 == 9 || std::strcmp(p.inst->family, "choir") != 0 || p.maxHeld > 2 || choirVariant(*p.inst).singers > 0) continue;
      std::vector<int>& k = keysOf[id];
      if (k.empty()) continue;
      std::nth_element(k.begin(), k.begin() + k.size() / 2, k.end());
      lines.push_back({double(k[k.size() / 2]), id});
    }
    if (lines.size() >= 2) {
      std::stable_sort(lines.begin(), lines.end(), [](auto& a, auto& b) { return a.first > b.first; });
      static const int kCentre[4] = {69, 62, 55, 48};  // S A T B
      const int n = int(lines.size());
      std::vector<int> group(n);
      for (int i = 0; i < n; ++i) {
        int g = 0;
        for (int j = 1; j < 4; ++j)
          if (std::fabs(lines[i].first - kCentre[j]) < std::fabs(lines[i].first - kCentre[g])) g = j;
        group[i] = i ? std::max(g, group[i - 1]) : g;  // (never above the line over it)
      }
      // (four lines are SATB, eight SSAATTBB, however they are voiced)
      if (n == 4 || n == 8)
        for (int i = 0; i < n; ++i) group[i] = i * 4 / n;
      std::string named = "  (the choir in sections, from the left:";
      for (int i = 0; i < n; ++i) {
        const bool upper = i + 1 < n && group[i + 1] == group[i], lower = i > 0 && group[i - 1] == group[i];
        int part = group[i] == 0 ? (lower ? kChoirMezzo : kChoirSoprano)
                 : group[i] == 1 ? kChoirAlto
                 : group[i] == 2 ? kChoirTenor
                 : (upper ? kChoirBaritone : kChoirBass);
        parts[lines[i].second].ens->setChoirRole(part, i, n);
        named += strf(" channel %d %s%s", lines[i].second / 128 + 1, kChoirPart[part].name, i + 1 < n ? "," : ")\n");
      }
      std::fprintf(stderr, "%s", named.c_str());
    }
  }
  // The choir parts that do not sing in sections are one choir (PJ_CHOIR_SHARED, 1; src/choir.cc
  // joinChoir): the aahs, the oohs, the choir pad and the lead voice's few, the same singers.
  if (knob("PJ_CHOIR_SHARED", 1) > 0) {
    Ensemble* host = nullptr;
    for (auto& [id, p] : parts) {
      if (id / 128 == 9 || std::strcmp(p.inst->family, "choir") != 0 || !p.ens->wholeChoir()) continue;
      // (the lead voice sung by its own singer, PJ_SINGER 1: not the choir's few)
      if (std::strcmp(p.inst->name, "voice-lead") == 0 && knob("PJ_SINGER", 1) > 0) continue;
      if (!host) host = p.ens.get();
      else p.ens->joinChoir(*host);
    }
  }
  // The brass section: the part read once with the rest of the song (src/brasssect.h): which
  // section it stands for, and under each onset the song's chord, to voice a sparse part by.
  for (auto& [id, p] : parts) {
    if (id / 128 == 9 || !brassSectionInstrument(*p.inst)) continue;
    std::vector<BrassNoteIn> notes;
    std::map<int, std::pair<size_t, int>> on;  // channel * 128 + key -> (its event, the program then)
    int pg[16] = {};
    for (size_t i = 0; i < song.ev.size(); ++i) {
      const Event& e = song.ev[i];
      const int ech = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) pg[ech] = e.a;
      if (kind == 0x90 && e.b > 0) {
        on[ech * 128 + e.a] = {i, pg[ech]};
      } else if (kind == 0x80 || kind == 0x90) {
        auto it = on.find(ech * 128 + e.a);
        if (it == on.end()) continue;
        const Event& o = song.ev[it->second.first];
        notes.push_back({o.t, e.t, long(o.tick), ech, o.a, o.b, it->second.second,
                         ech == id / 128 && it->second.second == id % 128});
        on.erase(it);
      }
    }
    p.bplan.reset(new BrassPlan(planBrassSection(notes, song.div, dir.style)));
    p.ens->setBrassMode(p.bplan->mode);
    std::fprintf(stderr, "    (%s)\n", p.bplan->summary.c_str());
  }
  // A Tonewheel: its notes by track, its pedals and manuals and drawbars planned (src/tonewheelplan.h).
  for (auto& [id, p] : parts) {
    if (std::strcmp(p.inst->family, "tonewheel") != 0 || id / 128 == 9) continue;
    std::vector<TonewheelNote> notes;
    std::map<int, std::pair<uint64_t, int>> on;  // channel * 128 + key -> (tick, velocity)
    int progs[16] = {}, cc11[16];
    std::fill(cc11, cc11 + 16, 127);
    std::map<int, int> onExpr;  // channel * 128 + key -> its channel's expression then
    for (const Event& e : song.ev) {
      const int ech = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) progs[ech] = e.a;
      if (kind == 0xb0 && e.a == 11) cc11[ech] = e.b;
      if (ech == 9 || !isTonewheel(forProgram(progs[ech]))) continue;
      if (kind == 0x90 && e.b > 0) {
        on[ech * 128 + e.a] = {e.tick, e.b};
        onExpr[ech * 128 + e.a] = cc11[ech];
      } else if (kind == 0x80 || kind == 0x90) {
        auto it = on.find(ech * 128 + e.a);
        if (it == on.end()) continue;
        // (an instrument routing to one division: tonewheel-upper-manual, -lower-manual, -pedal)
        const std::string nm = forProgram(progs[ech])->name;
        const int forced = nm == "tonewheel-upper-manual" ? TonewheelModel::kUpper : nm == "tonewheel-lower-manual" ? TonewheelModel::kLower
                         : nm == "tonewheel-pedal" ? TonewheelModel::kPedal : -1;
        notes.push_back({ech * 1000 + e.track, long(it->second.first), long(e.tick), e.a, it->second.second, ech, onExpr[ech * 128 + e.a], forced});
        on.erase(it);
      }
    }
    // Does the file set the Rotary itself (the mod wheel, the sustain pedal, CC82)?
    bool fileRotary = false, fileExpr = false;
    {
      int pg[16] = {};
      for (const Event& e : song.ev) {
        const int ech = e.status & 15, kind = e.status & 0xf0;
        if (kind == 0xc0) pg[ech] = e.a;
        if (kind == 0xb0 && ech != 9 && isTonewheel(forProgram(pg[ech])) && (e.a == 82 || ((e.a == 1 || e.a == 64) && fileTonewheel(pg[ech]))))
          fileRotary = true;
        if (kind == 0xb0 && ech != 9 && isTonewheel(forProgram(pg[ech])) && e.a == 11) fileExpr = true;
      }
    }
    for (const TonewheelNote& n : notes) ++p.tonewheelChans[n.ch];
    const double bpm = song.tempo.empty() ? 120 : 60 / song.tempo[0].len;
    auto secondsAt = [&song](long tick) {
      const double b = double(tick) / song.div;
      size_t k = 0;
      while (k + 1 < song.tempo.size() && song.tempo[k + 1].beat <= b) ++k;
      return song.tempo.empty() ? b * 0.5 : song.tempo[k].t + (b - song.tempo[k].beat) * song.tempo[k].len;
    };
    p.hplan.reset(new TonewheelPlan(planTonewheel(notes, song.div, bpm, p.inst->name, fileRotary, fileExpr, secondsAt)));
    for (const TonewheelPlan::Extra& x : p.hplan->extra) {
      p.hx.push_back({x.on, 1, x.pitch, x.vel, x.div});
      p.hx.push_back({x.off, 0, x.pitch, 0, x.div});
    }
    std::sort(p.hx.begin(), p.hx.end());  // (at one tick, a release before a press)
    std::string sum = p.hplan->summary;
    for (size_t at = 0; (at = sum.find('\n', at)) != std::string::npos && at + 1 < sum.size(); at += 5) sum.insert(at + 1, "    ");
    std::fprintf(stderr, "    %s", sum.c_str());
    if (auto* hp = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0))) {
      hp->setRegistration(p.hplan->reg);
      hp->setRotaryExternal();  // (the file's controls or the plan: the player does not ride it)
    }
  }
  // A piano among an orchestra is not miked: it is heard through the hall, as the players around
  // it are. An orchestra here: bowed strings with at least two kinds of wind or brass, and next to
  // no drum kit (a band with a string pad keeps its close-miked piano). PJ_PIANO_HALL: 0 never,
  // 1 always.
  {
    const double hall = knob("PJ_PIANO_HALL", -1);
    int prog[16] = {};
    size_t notes = 0, kit = 0, bowed = 0;
    std::set<int> blown;
    for (const Event& e : song.ev) {
      const int ch = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0xc0) prog[ch] = e.a;
      if (kind != 0x90 || e.b == 0) continue;
      ++notes;
      if (ch == 9) { ++kit; continue; }
      const int g = prog[ch];
      if ((g >= 40 && g <= 45) || (g >= 48 && g <= 49)) ++bowed;
      if ((g >= 56 && g <= 61) || (g >= 64 && g <= 75)) blown.insert(g);
    }
    const bool orchestra = bowed > 0 && blown.size() >= 2 && kit * 10 < notes;
    if (hall > 0 || (hall < 0 && orchestra))
      for (auto& [id, p] : parts)
        for (int q = 0; q < p.ens->size(); ++q)
          if (auto* pm = dynamic_cast<PianoModel*>(&p.ens->model(q))) pm->concertHall(true);
    // (the plan says how each piano is heard: what feeds the room, and what the audience's
    // microphones then take of it, as energy)
    const bool wanted = hall > 0 || (hall < 0 && orchestra);
    for (auto& [id, p] : parts) {
      if (p.ens->size() < 1) continue;
      const auto* pm = dynamic_cast<PianoModel*>(&p.ens->model(0));
      if (!pm) continue;
      const int board = int(std::lround(100 * pm->boardShare()));
      const char* why = !pm->inConcertHall() && board ? "-piano_close"
                        : board ? (hall > 0 ? "PJ_PIANO_HALL=1" : "with an orchestra")
                        : wanted ? "this piano has no board regions to send"
                        : hall == 0 ? "PJ_PIANO_HALL=0" : "no orchestra";
      if (std::strcmp(p.inst->name, "grand-piano") == 0) std::fprintf(stderr, "  piano   %2d: tuning: %s\n", id / 128 + 1, pianoTuningName());
      std::fprintf(stderr, "  piano   %2d: %s: close mics %d %%, board into the room %d %%, hammers' direct sound %d %% (%s)", id / 128 + 1, p.inst->name, 100 - board, board, int(std::lround(100 * pm->actionShare())), why);
      if (const Room* rm = p.ens->mainRoom()) {
        const int direct = int(std::lround(100 * rm->directShare()));
        std::fprintf(stderr, "; audience mics in the %s: direct %d %%, reverberant %d %%", rm->spec().name, direct, 100 - direct);
      } else {
        std::fprintf(stderr, "; no room");
      }
      std::fprintf(stderr, "\n");
    }
  }
  // The drums spread over their rims (Tube::spreadOn): each a place in the room, not a point,
  // every stroke thrown a little differently. (Not those with their own: the cymbals' plate
  // groups, the snare's directional parts.) PJ_DRUM_SPREAD=0: points.
  if (knob("PJ_DRUM_SPREAD", 1) != 0)
    for (auto& [id, p] : parts) {
      if (id / 128 != 9) continue;
      const std::string nm = p.inst->name;
      auto has = [&](const char* w) { return nm.find(w) != std::string::npos; };
      const double radius = has("kick") ? 0.25 : has("bongo") ? 0.09
                          : (has("tom") || has("conga") || has("timbale") || has("surdo") || has("taiko") || has("tabla")) ? 0.15
                          : 0.06;
      for (int q = 0; q < p.ens->size(); ++q) {
        Tube& t = p.ens->model(q);
        if (t.parts() > 0 || t.dipoleAxes()) continue;
        t.spreadOn(radius, unsigned(std::hash<std::string>()(nm)) + 977u * unsigned(q));
      }
      p.ens->placeParts();
    }
  // The shakuhachi held in the hands (Tube::aimOn): each opening its own place in the room, as
  // the player aims the instrument (src/player.cc, src/reedplayer.cc). PJ_SHAKU_AIM=0: a point, as before.
  if (knob("PJ_SHAKU_AIM", 1) != 0)
    for (auto& [id, p] : parts) {
      // (and, the owner 2026-11-28, the winds whose players move them as they play: the clarinet,
      // the oboe, the cor anglais and the recorder circling a little, the flutes swayed; heldaim.h;
      // PJ_HELD_AIM=0: the shakuhachi alone)
      static const bool kHeldAim = knob("PJ_HELD_AIM", 1) > 0;
      static const char* const kHeld[] = {"clarinet", "oboe", "english-horn", "recorder", "flute", "flute-c", "piccolo", "baroque-flute"};
      bool held = std::strcmp(p.inst->name, "shakuhachi") == 0;
      for (const char* nm : kHeld) held = held || (kHeldAim && std::strcmp(p.inst->name, nm) == 0);
      if (!held) continue;
      const int back = std::strcmp(p.inst->name, "shakuhachi") == 0 || std::strcmp(p.inst->name, "recorder") == 0 ? 0 : -1;  // (hole 1, the thumb's: behind)
      for (int q = 0; q < p.ens->size(); ++q) p.ens->model(q).aimOn(back);
      p.ens->placeParts();
    }
  // The koto's ji, set for the piece as a player sets them (src/kotoplan.h): its part's notes read
  // first (its channel's, while the channel's program is the koto's).
  for (auto& [id, p] : parts) {
    if (std::strcmp(p.inst->name, "koto") != 0 || p.ens->size() < 1) continue;
    auto* mp = dynamic_cast<ModalPlayer*>(&p.ens->player(0));
    if (!mp) continue;
    const int ch = id / 128, prog = id % 128;
    int pr = 0;
    std::vector<KotoNote> notes;
    std::map<int, size_t> on;
    for (const Event& e : song.ev) {
      if ((e.status & 15) != ch) continue;
      const int kind = e.status & 0xf0;
      if (kind == 0xc0) pr = e.a;
      if (kind == 0x90 && e.b > 0) {
        if (pr != prog) continue;
        on[e.a] = notes.size();
        notes.push_back({e.t, e.t + 0.3, e.a, e.b});
      } else if (kind == 0x80 || kind == 0x90) {
        auto it = on.find(e.a);
        if (it != on.end()) {
          notes[it->second].off = e.t;
          on.erase(it);
        }
      }
    }
    if (notes.empty()) continue;
    const KotoPlan plan = planKoto(notes);
    if (plan.steps.empty()) continue;
    mp->setKotoPlan(plan);
    std::fprintf(stderr, "  koto    %2d: %s\n", ch + 1, plan.summary.c_str());
  }
  // A vibraphone's motor, set as a vibist sets it (between sections, not ridden), bar by bar by
  // how exposed the part is: among many instruments the openings on the eighth notes (by the
  // song's clock: the vibraphone stands out); alone (a solo piece, or a solo passage in a slow
  // song) or very gentle, a slow shimmer of 0.7 - 1.3 Hz (the gentler, the slower). A change
  // lasts four bars at least. A file riding the mod wheel on the channel keeps the motor.
  for (auto& [id, p] : parts) {
    if (std::strncmp(p.inst->name, "vibraphone", 10) != 0) continue;
    auto* mp = dynamic_cast<ModalPlayer*>(&p.ens->player(0));
    if (!mp) continue;
    const int ch = id / 128;
    auto tempoAt = [&](double t) {
      size_t k = 0;
      while (k + 1 < song.tempo.size() && song.tempo[k + 1].t <= t) ++k;
      return k;
    };
    auto beatAt = [&](double t) {
      if (song.tempo.empty()) return t / 0.5;
      const size_t k = tempoAt(t);
      return song.tempo[k].beat + (t - song.tempo[k].t) / song.tempo[k].len;
    };
    auto timeAt = [&](double beat) {
      size_t k = 0;
      while (k + 1 < song.tempo.size() && song.tempo[k + 1].beat <= beat) ++k;
      return song.tempo.empty() ? beat * 0.5 : song.tempo[k].t + (beat - song.tempo[k].beat) * song.tempo[k].len;
    };
    struct On { double t; int ch, vel; };
    std::vector<On> mine, others;
    bool wheel = false;
    for (const Event& e : song.ev) {
      const int ech = e.status & 15;
      if (ech == ch && (e.status & 0xf0) == 0xb0 && e.a == 1 && e.b > 0) wheel = true;
      if ((e.status & 0xf0) != 0x90 || e.b == 0 || ech == 9) continue;
      (ech == ch ? mine : others).push_back({e.t, ech, e.b});
    }
    if (wheel || mine.empty()) continue;
    // The part's mood (the tempo, how busy, how loud).
    std::vector<double> q;
    double vel = 0, playing = 0;
    for (size_t i = 0; i < mine.size(); ++i) {
      q.push_back(song.tempo.empty() ? 0.5 : song.tempo[tempoAt(mine[i].t)].len);
      vel += mine[i].vel;
      if (i) playing += std::min(2.0, mine[i].t - mine[i - 1].t);
    }
    std::nth_element(q.begin(), q.begin() + long(q.size() / 2), q.end());
    const double quarter = q[q.size() / 2], bpm = 60 / quarter;
    vel /= double(mine.size());
    const double busy = mine.size() > 1 ? double(mine.size() - 1) / std::max(0.5, playing) : 0;
    const double mood = std::clamp(0.45 * (bpm - 60) / 100 + 0.35 * std::min(1.0, busy / 6) + 0.2 * (vel - 40) / 70, 0.0, 1.0);
    const bool slow = bpm < 100;
    // How many others play around each of its onsets (within two beats), bar by bar (4 quarters).
    std::vector<double> crowdSum, crowdN;
    double crowdAll = 0;
    size_t lo = 0;
    for (const On& n : mine) {
      const double w = 2 * (song.tempo.empty() ? 0.5 : song.tempo[tempoAt(n.t)].len);
      while (lo < others.size() && others[lo].t < n.t - w) ++lo;
      bool seen[16] = {};
      int c = 0;
      for (size_t j = lo; j < others.size() && others[j].t <= n.t + w; ++j)
        if (!seen[others[j].ch]) seen[others[j].ch] = true, ++c;
      const int bar = int(beatAt(n.t) / 4);
      if (bar >= int(crowdSum.size())) crowdSum.resize(bar + 1, 0), crowdN.resize(bar + 1, 0);
      crowdSum[bar] += c;
      crowdN[bar] += 1;
      crowdAll += c;
    }
    crowdAll /= double(mine.size());
    // The harmony and the rhythm, bar by bar: how many pitch classes sound together at each onset
    // (all parts but the drums: a triad 3, sevenths 4, the extended chords 5 and more), and the
    // drums' hits a beat.
    const size_t nBars = crowdSum.size();
    std::vector<double> pcSum(nBars, 0), pcN(nBars, 0), drumHits(nBars, 0);
    {
      int held[16][128] = {};
      int pcOn[12] = {};
      // (counted once a moment's notes are all down: a struck chord at its full size)
      double pendT = -1;
      int pendBar = -1;
      auto settle = [&]() {
        if (pendBar < 0) return;
        int n = 0;
        for (int k = 0; k < 12; ++k) n += pcOn[k] > 0;
        pcSum[pendBar] += n;
        pcN[pendBar] += 1;
        pendBar = -1;
      };
      for (const Event& e : song.ev) {
        const int ech = e.status & 15, kind = e.status & 0xf0;
        const bool noteOn = kind == 0x90 && e.b > 0, noteOff = kind == 0x80 || (kind == 0x90 && e.b == 0);
        if (!noteOn && !noteOff) continue;
        if (e.t > pendT + 0.03) settle();  // (within 30 ms: one chord, rolled or not)
        const int bar = int(beatAt(e.t) / 4);
        if (ech == 9) {
          if (noteOn && bar < int(nBars)) drumHits[bar] += 1;
          continue;
        }
        if (noteOff) {
          if (held[ech][e.a] > 0) --held[ech][e.a], --pcOn[e.a % 12];
          continue;
        }
        ++held[ech][e.a];
        ++pcOn[e.a % 12];
        if (bar < int(nBars) && pendBar < 0) pendT = e.t, pendBar = bar;
      }
      settle();
    }
    // Each bar's choice (a bar without the vibraphone keeps the last), then runs under four bars
    // merged into the one before. A shimmer: a solo piece, a solo passage in a slow song, a very
    // gentle part, or rich harmony with little rhythm under it; else the eighth notes.
    const bool soloPiece = crowdAll < 0.5;
    std::vector<int> shimmer(nBars, -1);
    int nRich = 0, nDriven = 0;
    for (size_t b = 0; b < nBars; ++b) {
      if (crowdN[b] == 0) continue;
      const double crowd = crowdSum[b] / crowdN[b];
      const bool rich = pcN[b] > 0 && pcSum[b] / pcN[b] >= 4.5, driven = drumHits[b] / 4 >= 1.0;
      nRich += rich;
      nDriven += driven;
      shimmer[b] = mood < 0.2 || soloPiece || (slow && crowd < 0.75) || (rich && !driven);
    }
    int last = -1;
    for (int& v : shimmer) last = v = v >= 0 ? v : last;
    for (int& v : shimmer) if (v < 0) v = shimmer.empty() ? 0 : shimmer.back();
    for (size_t b = 0; b < shimmer.size();) {
      size_t e = b;
      while (e < shimmer.size() && shimmer[e] == shimmer[b]) ++e;
      if (e - b < 4 && b > 0) std::fill(shimmer.begin() + long(b), shimmer.begin() + long(e), shimmer[b - 1]);
      b = e;
    }
    // The rates: the shimmer by the mood; the eighth notes by the tempo there (quarters past 5 Hz;
    // triplets or sixteenths under 0.7 Hz).
    const double shimmerHz = 0.7 + 0.6 * std::min(1.0, mood / 0.5);
    auto lockHz = [](double qlen, const char** name) {
      static const struct { double perQuarter; const char* name; } kValues[] = {
          {2, "the eighth notes"}, {1, "the quarter notes"}, {3, "the eighth triplets"}, {4, "the sixteenth notes"}};
      for (const auto& v : kValues)
        if (v.perQuarter / qlen >= 0.7 && v.perQuarter / qlen <= 5) {
          *name = v.name;
          return v.perQuarter / qlen;
        }
      *name = "near the eighth notes";
      return std::clamp(2 / qlen, 0.7, 5.0);
    };
    std::vector<ModalPlayer::MotorStep> plan;
    std::string said;
    int nShimmer = 0, nLock = 0;
    for (size_t b = 0; b < shimmer.size(); ++b) {
      const double t = b == 0 ? 0.0 : timeAt(4.0 * b);
      const char* name = "a slow shimmer";
      const double hz = shimmer[b] ? shimmerHz : lockHz(song.tempo.empty() ? 0.5 : song.tempo[tempoAt(t)].len, &name);
      (shimmer[b] ? nShimmer : nLock)++;
      if (!plan.empty() && std::fabs(plan.back().hz - hz) < 1e-6) continue;
      plan.push_back({t, hz, !shimmer[b]});
      if (said.size() < 300) {
        char buf[96];
        std::snprintf(buf, sizeof buf, "%s%s %.2f Hz from bar %zu", said.empty() ? "" : "; ", shimmer[b] ? "shimmer" : name, hz, b + 1);
        said += buf;
      }
    }
    mp->setMotorPlan(plan);
    int withVib = 0;
    for (double n : crowdN) withVib += n > 0;
    std::fprintf(stderr, "    %s: the motor %s (%.0f bpm, %s, %.1f others around it%s; of its %d bars %d rich in harmony, %d driven by drums)\n",
                 p.inst->name, said.c_str(), bpm, mood < 0.4 ? "gentle" : mood < 0.7 ? "moderate" : "lively", crowdAll,
                 soloPiece ? ": a solo" : "", withVib, nRich, nDriven);
    (void)nShimmer, (void)nLock;
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
    OrganCompany company = organCompany(song);
    company.stage = room ? room->name : "dry";
    p.plan.reset(new OrganPlan(planOrgan(notes, song.div, song.barTicks ? song.barTicks : 4L * song.div, starts, bpm, &company)));
    {
      int stops, ranks, pipes;
      organSize(stops, ranks, pipes);
      std::fprintf(stderr, "  the organ (%s; tuning: %s): %d stops, %d ranks, %d pipes\n", organDispositionName(), organTuningName(), stops, ranks, pipes);
    }
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
      std::map<int, std::vector<long>> lineTicks;     // ... and their ticks
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
          lineTicks[ech * 1000 + e.track].push_back(e.tick);
        }
      }
      std::fill(progs2, progs2 + 16, 0);
      int shortened = 0, repeats = 0, softened = 0, phraseEnds = 0, led = 0, breaths = 0;
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
          double speech = 0, ledBy = 0;  // (its pipes' speech, s; how far it was played ahead)
          {
            static const double kLead = std::getenv("PJ_PLAY_BASS_LEAD") ? std::atof(std::getenv("PJ_PLAY_BASS_LEAD")) : 1.0;
            const double f = 440 * std::exp2((e.a - 69) / 12.0) * (pedalNote ? 0.5 : 1.0);  // (its sounding pitch: the pedal's 16')
            speech = 0.075 * fastPow(262 / f, 0.6);
            // (into silence half of it: no pulse to keep ahead of, nothing to sound with)
            const double lead = std::min(0.06, kLead * (pedalNote ? 0.5 : 0.3) * std::max(0.0, speech - 0.075)) *
                                (fromSilence.count(it->second) ? 0.5 : 1.0);
            // (never ahead of the same key's last release: its late note-off
            // would end this note)
            const auto lo = lastOff.find(ech * 128 + e.a);
            const double room = lo == lastOff.end() ? lead : std::max(0.0, on - lo->second - 0.005);
            if (std::min(lead, room) > 0.002) {
              song.ev[it->second].t = std::max(0.0, on - std::min(lead, room));
              ledBy = on - song.ev[it->second].t;
              ++led;
            }
          }
          // (a phrase's end: a rest of 250 ms or more after it in its line. Not the line's last
          // note, the piece's end, nor one before a new part (a section of the plan begins between
          // it and its line's next note): those are held to their length)
          bool phraseEnd = false, lineEnd = false, restAfter = false;
          {
            const int ln = ech * 1000 + e.track;
            const auto& ls = lineStarts[ln];
            const auto nl = std::upper_bound(ls.begin(), ls.end(), on + 1e-6);
            lineEnd = nl == ls.end();
            restAfter = !lineEnd && *nl - e.t >= 0.25;
            if (restAfter) {
              const long onTick = song.ev[it->second].tick, nextTick = lineTicks[ln][nl - ls.begin()];
              const auto& ps = p.plan->partStarts;
              const auto pb = std::upper_bound(ps.begin(), ps.end(), onTick);
              phraseEnd = pb == ps.end() || *pb > nextTick;
            }
          }
          if (!pedalNote) {
            float touch = float(std::clamp((e.a - 67) / 17.0, 0.0, 1.0) * std::clamp((dur - 0.5) / 1.5, 0.0, 1.0));
            // (a phrase's last note, a rest of 250 ms or more after it in its
            // line, or the line's last: softer too, the melody's on the
            // Hauptwerk the most)
            if (lineEnd || restAfter) {
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
          // (a phrase's end lifted early, the organist's breath before the next phrase: 80 .. 250 ms
          // (the romantic's legato a little less), so the church hears the line close, and a
          // phrase's last note never loses over 17 % of its length; PJ_ORGAN_BREATH scales it)
          if (phraseEnd) {
            static const double kBreath = std::getenv("PJ_ORGAN_BREATH") ? std::atof(std::getenv("PJ_ORGAN_BREATH")) : 1.0;
            const double b = kBreath * std::clamp(0.2 * dur, 0.08, 0.25) * (baroque ? 1.0 : 0.7);
            if (b > gap) {
              gap = b;
              ++breaths;
            }
          }
          gap = std::min(gap, (phraseEnd || lineEnd || restAfter ? 0.17 : 0.6) * dur);
          // (a pedal note sounds at least its big pipes' speech and 0.1 s more, or, written shorter,
          // all but 25 ms: a 16' Principal's fundamental takes ~0.2 s to reach -6 dB, so a fast line
          // detached by a quarter would speak or not by a few ms, its notes struck cold nearly
          // without their fundamental, those struck again while their pipe still rang with it: the
          // organist detaches the pedal less in quick passages. Not for a repeated note.)
          if (pedalNote && baroque && !(nx != st.end() && *nx - e.t < 0.03)) {
            const double need = std::min(dur + ledBy - 0.025, speech + 0.1);
            gap = std::max(0.0, std::min(gap, dur + ledBy - need));
          }
          if (gap > 0) {
            e.t -= gap;
            ++shortened;
          }
          lastOff[ech * 128 + e.a] = e.t;
          onAt.erase(it);
        }
      }
      std::stable_sort(song.ev.begin(), song.ev.end(), [](const Event& x, const Event& y) { return x.t < y.t; });
      std::fprintf(stderr, "    (%s: %d notes shortened, %d of them repeated notes, %d phrase ends lifted early; %d notes with a softer finger, %d of them phrase ends; %d bass notes played ahead)\n",
                   baroque ? "toucher ordinaire, the Baroque's articulate legato; the pedal detached"
                           : "legato absolu, the romantic's: held", shortened, repeats, breaths, softened, phraseEnds, led);
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
  // (and the rooms' tails; PJ_PLAY_UNTIL > 0: only the first so many seconds, as for a timing)
  const double until = std::getenv("PJ_PLAY_UNTIL") ? std::atof(std::getenv("PJ_PLAY_UNTIL")) : 0.0;
  const double end = until > 0 ? std::min(until, song.ev.back().t + 3.0) : song.ev.back().t + 3.0;
  // -start_time: the song from there. Everything before it still happens (programs, controllers, the
  // plans' registrations), at once in the first block, but only the notes still sounding then are
  // played; they begin at the start. (The whole piece is planned as usual, so nothing else moves.)
  const double startT = gPlayStart;
  if (startT >= song.ev.back().t) {
    std::fprintf(stderr, "-start_time: the song ends at %d:%04.1f\n", int(song.ev.back().t) / 60, std::fmod(song.ev.back().t, 60.0));
    return 1;
  }
  if (startT > 0) {
    // (of the sustain pedal's moves before the start only the last counts: no flurry of dampers)
    std::map<int, size_t> lastPedal;
    for (size_t i = 0; i < song.ev.size() && song.ev[i].t <= startT; ++i)
      if ((song.ev[i].status & 0xf0) == 0xb0 && song.ev[i].a == 64) {
        const auto it = lastPedal.find(song.ev[i].status & 15);
        if (it != lastPedal.end()) song.ev[it->second].skip = true;
        lastPedal[song.ev[i].status & 15] = i;
      }
    std::map<int, std::vector<size_t>> open;
    for (size_t i = 0; i < song.ev.size() && song.ev[i].t <= startT; ++i) {
      Event& ev = song.ev[i];
      const int kind = ev.status & 0xf0, c = ev.status & 15;
      if (kind != 0x80 && kind != 0x90) continue;
      if (c == 9) {  // (a drum's stroke is over at once)
        ev.skip = true;
        continue;
      }
      auto& o = open[c * 128 + ev.a];
      if (kind == 0x90 && ev.b > 0) {
        o.push_back(i);
      } else if (!o.empty()) {
        song.ev[o.front()].skip = true;
        ev.skip = true;
        o.erase(o.begin());
      }
    }
    std::fprintf(stderr, "  starting at %d:%04.1f\n", int(startT) / 60, std::fmod(startT, 60.0));
  }
  SongClock::get().start = startT;  // (what is printed tells the song's time)
  const size_t frames = (size_t(std::max(end - startT, 0.1) * kRate) / kBlock + 1) * kBlock;
  // The song is rendered on its own thread into this buffer; the player (or
  // the file) takes what is ready: playing starts once 10 s are ahead, and
  // waits for 10 s again should the rendering fall behind.
  std::vector<float> all(2 * frames, 0.0f);
  std::atomic<size_t> ready{0};
  const bool streaming = outWav == nullptr;
  // Rendering on the shared pool (PJ_THREADS, default the cores up to 32): one job a part, the
  // heaviest first (the most voices); the synth circuits all in one job, in their order (one
  // board couples them, sample by sample). Each part's voices go on the same pool in turn
  // (Ensemble::block), so one big part no longer holds back the rest on one thread.
  // The band's sounding notes (src/band.h), kept by the loop below between blocks; every part
  // may listen (the fretless bass does; the brass and the reeds, Ensemble::windEar), each told its
  // own channel. A part that tunes by ear marks its notes so (the order of hearing, src/band.h).
  SoundingSet band;
  static const bool kBandLog = std::getenv("PJ_BAND_LOG") != nullptr;
  unsigned bandSeen = ~0u;
  for (auto& [id, p] : parts)
    if (id / 128 != 9) {
      for (int i = 0; i < p.ens->size(); ++i) p.ens->player(i).setBand(&band, id / 128);
      const int ear = Ensemble::windEar(*p.inst);
      if (ear >= 0) p.ens->setBand(&band, id / 128, ear);
    }
  auto byEar = [&](const Part& p) {
    return p.ens->listens() ? 2 : fretlessBassInstrument(*p.inst) || std::strcmp(p.inst->name, "koto") == 0 ? 1 : 0;
  };
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
  TaskPool& pool = TaskPool::shared();
  if (verbose) std::fprintf(stderr, "  rendering on %d threads (%zu parts)\n", pool.threads(), jobs.size());
  // A piano's on/off pedal let up (the file's own, or the planned pianist's): how hard the foot
  // lets go, chosen for the music. In a rhythmic piece (half a drum hit a beat or more) a release
  // on the beat goes at full force (0.15 m/s at the dampers: their landing and the rail's knock as
  // loud as they come), off it gentler (0.10); in a calm one gentler still (0.10 on the beat,
  // 0.08 off). Into a quiet passage (no note begun within two beats, 1.5 s at least) or at the
  // end, it is let up carefully (0.035: ~13 dB softer, the landings smeared). Each its own
  // (x 0.85 .. 1.15). A pedal sending the values between keeps the speed its controller shows.
  std::map<std::pair<int, long long>, double> releaseSpeed;  // (channel, t in 0.1 ms) -> m/s
  // (The gentlest releases of the pedal: each channel's last one, whatever the music, and any after
  // which the pianist's own part holds nothing and begins nothing for PJ_PIANO_SILENCE seconds,
  // 1.5, whatever the others play: free to let the foot up calmly, no rhythmic thump. Channel, t
  // in 0.1 ms.)
  std::set<std::pair<int, long long>> gentleUp;
  {
    static const double kSilence = knob("PJ_PIANO_SILENCE", 1.5);
    std::map<int, std::vector<double>> onsOf, offsOf;
    std::map<int, int> open;
    for (const Event& e : song.ev) {
      const int kind = e.status & 0xf0, id = (e.status & 15) * 128 + e.a;
      if (kind == 0x90 && e.b > 0) {
        onsOf[e.status & 15].push_back(e.t);
        ++open[id];
      } else if ((kind == 0x80 || kind == 0x90) && open[id] > 0) {
        --open[id];
        offsOf[e.status & 15].push_back(e.t);
      }
    }
    for (auto& [c, v] : onsOf) std::sort(v.begin(), v.end());
    for (auto& [c, v] : offsOf) std::sort(v.begin(), v.end());
    std::map<int, bool> pedalIsDown;
    std::map<int, double> lastUp;
    for (const Event& e : song.ev) {
      if ((e.status & 0xf0) != 0xb0 || e.a != 64) continue;
      const int c = e.status & 15;
      if (pedalIsDown[c] && e.b < 64) {
        lastUp[c] = e.t;
        const std::vector<double>& ons = onsOf[c], &offs = offsOf[c];
        const auto next = std::upper_bound(ons.begin(), ons.end(), e.t);
        // (a note let go with the foot, within 20 ms, is not held on)
        const long held = long(next - ons.begin()) - long(std::upper_bound(offs.begin(), offs.end(), e.t + 0.02) - offs.begin());
        if (held <= 0 && (next == ons.end() || *next >= e.t + kSilence)) gentleUp.insert({c, std::llround(e.t * 1e4)});
      }
      pedalIsDown[c] = e.b >= 64;
    }
    for (const auto& [c, t] : lastUp) gentleUp.insert({c, std::llround(t * 1e4)});
  }
  {
    std::vector<double> onsets;
    double drumHits = 0;
    bool between[16] = {};
    for (const Event& e : song.ev) {
      const int ech = e.status & 15, kind = e.status & 0xf0;
      if (kind == 0x90 && e.b > 0) {
        if (ech == 9) drumHits += 1;
        else onsets.push_back(e.t);
      }
      if (kind == 0xb0 && e.a == 64 && e.b > 15 && e.b < 112) between[ech] = true;
    }
    std::sort(onsets.begin(), onsets.end());
    const double beats = song.ev.empty() ? 0 : (song.tempo.empty() ? song.ev.back().t / 0.5 : [&] {
      const auto& k = song.tempo.back();
      return k.beat + (song.ev.back().t - k.t) / k.len;
    }());
    const bool rhythmic = beats > 0 && drumHits / beats >= 0.5;
    uint32_t rng = 777;
    int nFull = 0, nGentle = 0, nCareful = 0;
    for (const Event& e : song.ev) {
      const int ech = e.status & 15;
      if ((e.status & 0xf0) != 0xb0 || e.a != 64 || e.b >= 64 || ech == 9 || between[ech]) continue;
      bool piano = false;
      for (auto& [id, p] : parts)
        if (id / 128 == ech && p.ens->size() > 0 && dynamic_cast<PianoPlayer*>(&p.ens->player(0))) piano = true;
      if (!piano) continue;
      size_t k = 0;
      while (k + 1 < song.tempo.size() && song.tempo[k + 1].t <= e.t) ++k;
      const double qlen = song.tempo.empty() ? 0.5 : song.tempo[k].len;
      const double beat = song.tempo.empty() ? e.t / 0.5 : song.tempo[k].beat + (e.t - song.tempo[k].t) / qlen;
      const bool onBeat = std::fabs(beat - std::round(beat)) < 0.08;
      const auto next = std::upper_bound(onsets.begin(), onsets.end(), e.t + 0.02);
      const bool quiet = next == onsets.end() || *next - e.t > std::max(1.5, 2 * qlen);
      double v;
      if (quiet) v = 0.035, ++nCareful;
      else if (rhythmic && onBeat) v = 0.15, ++nFull;
      else v = (rhythmic || onBeat) ? 0.10 : 0.08, ++nGentle;
      rng = rng * 1664525u + 1013904223u;
      v *= 0.85 + 0.3 * ((rng >> 8) / 16777216.0);
      releaseSpeed[{ech, std::llround(e.t * 1e4)}] = v;
    }
    if (nFull + nGentle + nCareful)
      std::fprintf(stderr, "  the piano's pedal let up: %d at full force, %d gently, %d carefully (%s)\n", nFull, nGentle, nCareful,
                   rhythmic ? "a rhythmic piece" : "a calm piece");
  }
  const float masterGain = float(fastPow(10.0, gPlayVolumeDb / 20));
  float mixPeak = 0;  // (the mix's highest sample, and how many the output's limit had to bend)
  size_t kneeSamples = 0;
  // The lead voice (General MIDI "Lead 6 (voice)"): a jazz singer on puhuja's voice, who reads the
  // whole part now, with the director's accents and phrase ends and when the other parts' notes
  // begin, and sings it in syllables (src/jazzsinger.h). PJ_SINGER=0: the choir's few singers.
  if (knob("PJ_SINGER", 1) > 0)
    for (auto& [id, p] : parts) {
      if (id / 128 == 9 || std::strcmp(p.inst->name, "voice-lead") != 0) continue;
      std::vector<SungNote> notes;
      std::vector<double> others;
      std::vector<HeardNote> heard;         // the other parts' notes, for the singer's tuning
      std::map<int, size_t> heardOpen;      // channel x 128 + key -> its note
      std::map<int, size_t> open;  // key -> its note
      double singerKeySum = 0;  // the other lead singers' notes
      int singerKeyN = 0;
      int pg[16] = {};
      for (const Event& e : song.ev) {
        const int ech = e.status & 15, kind = e.status & 0xf0;
        if (kind == 0xc0) pg[ech] = e.a;
        if (kind != 0x80 && kind != 0x90) continue;
        const bool on = kind == 0x90 && e.b > 0;
        if (ech * 128 + pg[ech] != id) {
          if (on && ech != 9 && !e.skip) others.push_back(e.t - startT);
          if (ech != 9) {
            auto ho = heardOpen.find(ech * 128 + e.a);
            if (ho != heardOpen.end()) {
              heard[ho->second].off = e.t - startT;
              heardOpen.erase(ho);
            }
            auto hp = parts.find(ech * 128 + pg[ech]);
            const int fam = hp == parts.end() ? kBandNone : bandFamily(hp->second.inst->family, hp->second.inst->name);
            if (on && !e.skip && hp != parts.end() && std::strcmp(hp->second.inst->name, "voice-lead") == 0) { singerKeySum += e.a; ++singerKeyN; }
            if (on && !e.skip && fam != kBandNone && !hp->second.singer && std::strcmp(hp->second.inst->name, "voice-lead") != 0) {
              HeardNote h;
              h.on = e.t - startT;
              h.off = h.on + 0.25;
              h.ch = ech; h.pitch = e.a; h.vel = e.b; h.fam = fam;
              heardOpen[ech * 128 + e.a] = heard.size();
              heard.push_back(h);
            }
          }
          continue;
        }
        auto o = open.find(e.a);
        if (o != open.end()) {
          notes[o->second].off = e.t - startT;
          open.erase(o);
        }
        if (!on || e.skip) continue;
        SungNote s;
        s.on = e.t - startT;
        s.off = s.on + 0.25;
        s.key = e.a;
        s.vel = e.b;
        {  // (where in the bar: the weighted syllables)
          const long bar = song.barTicks ? song.barTicks : 4L * song.div;
          const long tk = long(e.tick);
          s.beat = tk % bar == 0 ? 1.0f : (bar % (2 * song.div) == 0 && tk % (bar / 2) == 0) ? 0.75f : tk % song.div == 0 ? 0.5f : 0.2f;
          s.inBar = float(tk % bar) / song.div;
          for (const auto& tp : song.tempo)  // (the tempo there: where the singer breathes in, a beat before the bar)
            if (tp.t <= e.t || s.beatLen == 0) s.beatLen = float(tp.len);
        }
        if ((dir.acts || dir.self || dir.vib) && e.cue >= 0) {
          s.accent = dir.cues[e.cue].v[kCueAccent];
          s.phraseEnd = dir.cues[e.cue].v[kCuePhraseEnd] > 0.5f && dir.cues[e.cue].v[kCueOnly] < 0.5f;  // (the singer's ends: the reading's)
          // (the drama, the hairpins and the vibrato are the director's for the singer too)
          const float* cv = dir.cues[e.cue].v;
          s.cued = true;
          s.level = cv[kCueLevel]; s.levelEnd = cv[kCueLevelEnd];
          s.swell = cv[kCueSwell]; s.swellPeak = cv[kCueSwellPeak];
          s.vibDepth = cv[kCueVibDepth]; s.vibDelay = cv[kCueVibDelay]; s.vibRate = cv[kCueVibRate];
        }
        open[e.a] = notes.size();
        notes.push_back(s);
      }
      if (notes.empty()) continue;
      p.singer.reset(new JazzSinger());
      // (its transposition: -reinstrument's TO:N, else PJ_SINGER_SHIFT, else the singer's own choice)
      {
        std::string voice;
        int shift = reinstrumentShift(id % 128, &voice);
        if (shift == kNoShift && std::getenv("PJ_SINGER_SHIFT")) shift = std::atoi(std::getenv("PJ_SINGER_SHIFT"));
        if (shift != kNoShift) p.singer->setShift(shift);
        if (voice.empty() && std::getenv("PJ_SINGER_VOICE")) voice = std::getenv("PJ_SINGER_VOICE");
        if (!voice.empty() && !p.singer->setVoice(voice)) std::fprintf(stderr, "  (no voice %s: the singer chooses)\n", voice.c_str());
      }
      // (the singer's place in the band: a part written for a bass instrument; a solo line above it, which is
      // another singer, or an instrument that plays a note at a time, an octave or more higher on average)
      {
        bool bassPart = false, soloAbove = false;
        for (const char* nm : kGm[id % 128]) bassPart = bassPart || (nm && std::strstr(nm, "bass"));
        double mine = 0;
        for (const SungNote& s : notes) mine += s.key;
        mine /= notes.size();
        if (singerKeyN > 0 && singerKeySum / singerKeyN >= mine + 7) soloAbove = true;
        struct Line { int n = 0, chord = 0; double sum = 0, lastOn = -1e9, lastOff = -1e9; };
        std::map<int, Line> line;
        for (const HeardNote& h : heard) {
          Line& l = line[h.ch];
          if (h.on < l.lastOn + 0.03 || h.on < l.lastOff - 0.08) ++l.chord;
          ++l.n; l.sum += h.pitch; l.lastOn = h.on; l.lastOff = std::max(l.lastOff, h.off);
        }
        for (const auto& [ch, l] : line)
          if (l.n >= 12 && l.chord < 0.2 * l.n && l.sum / l.n >= mine + 12) soloAbove = true;
        p.singer->setBassPlace(bassPart, soloAbove);
      }
      {
        int k = 0;
        for (auto& [id2, p2] : parts) {
          if (id2 == id) break;
          if (id2 / 128 != 9 && std::strcmp(p2.inst->name, "voice-lead") == 0) ++k;
        }
        p.singer->setSinger(k);
      }
      p.singer->setScore(notes, others, heard);
      JazzSinger* js = p.singer.get();
      // (one singer at the centre; two a tenth of the stage either side; more spread a little wider)
      int nLead = 0, kLead = 0;
      for (auto& [id2, p2] : parts)
        if (id2 / 128 != 9 && std::strcmp(p2.inst->name, "voice-lead") == 0) {
          if (id2 == id) kLead = nLead;
          ++nLead;
        }
      p.singerSide = nLead < 2 ? 0.0f : float(-0.1 * (nLead - 1) + 0.2 * kLead);
      // (intimacy by our own means: the singer in the hall everyone shares, with a microphone close
      // to the voice blended in, PJ_SINGER_NEAR_DB: its direct and early sound that much over what
      // the hall's main microphones have of it, the reverberation as it was)
      p.ens->setLeadFeed([js](float* out, int n) { js->render(out, n); }, p.singerSide, float(std::pow(10.0, knob("PJ_SINGER_NEAR_DB", 6) / 20)));
      say(stderr, "  (channel %d: the lead voice sung by a %s%s, moved %+d semitones%s; %d of its %d notes outside the voice's range %s; it begins: %s)\n",
          id / 128 + 1, js->voiceName(), js->singsBass() ? " as a plucked bass" : "", js->transposed(), js->shiftAsked() ? " as asked" : "", js->outside(), int(notes.size()), js->range().c_str(),
          js->weighted().substr(0, std::min<size_t>(js->weighted().find('\n'), 90)).c_str());
    }
  auto renderSong = [&]() {
    std::vector<float> l(kBlock), r(kBlock);
    size_t next = 0, tk = 0, ahead = 0;
    int heldAll = 0;
    double lastOn = -1e9;
    std::map<int, int> heldNote;  // channel * 128 + key -> times down (not the drums)
    size_t dirAt[16] = {};       // where each channel's level curve is
    size_t ownAt[16] = {};       // ... and its players' own long-note shapes
    for (size_t n = 0; n < frames; n += kBlock) {
      const double now = (n + kBlock) / double(kRate) + startT;
      {  // the beat, for those who keep time with it
        while (tk + 1 < song.tempo.size() && song.tempo[tk + 1].t <= now) ++tk;
        SongClock& c = SongClock::get();
        c.valid = !song.tempo.empty();
        c.now = now;
        c.playing = true;
        if (c.valid) {
          c.beatLen = song.tempo[tk].len;
          c.beat = song.tempo[tk].beat + (now - song.tempo[tk].t) / song.tempo[tk].len;
        }
        // The band's silences: what sounds now, what was last begun, what comes next.
        while (ahead < song.ev.size() && (song.ev[ahead].t < now || (song.ev[ahead].status & 0xf0) != 0x90 || song.ev[ahead].b == 0)) ++ahead;
        const double toNext = ahead < song.ev.size() ? song.ev[ahead].t - now : 1e9;
        c.pause = heldAll <= 0 && now - lastOn > 2 * c.beatLen && toNext > c.beatLen;
      }
      // The director's level for each part, block by block (as a file's own expression would
      // come: the breath, the bow's speed); the organs keep their plans' own.
      // (and the winds' and strings' players' own long-note shapes added: ownLevel)
      for (auto& [id, p] : parts) {
          const int ch = id / 128;
          const bool lvOn = (dir.acts || dir.self) && !dirLevel[ch].empty(), ownOn = !ownLevel[ch].empty();
          if (ch == 9 || p.plan || p.hplan || (!lvOn && !ownOn)) continue;
          double lv = 0.5;
          if (lvOn) {
            const auto& c = dirLevel[ch];
            size_t& k = dirAt[ch];
            while (k + 1 < c.size() && c[k + 1].first <= now) ++k;
            lv = c[k].second;
            if (k + 1 < c.size() && now > c[k].first)
              lv += (c[k + 1].second - lv) * (now - c[k].first) / std::max(1e-9, c[k + 1].first - c[k].first);
          }
          double own = 0;
          if (ownOn) {
            const auto& c = ownLevel[ch];
            size_t& k = ownAt[ch];
            while (k + 1 < c.size() && c[k + 1].first <= now) ++k;
            double d = now < c[0].first ? 0.0 : c[k].second;
            if (now >= c[0].first && k + 1 < c.size() && now > c[k].first)
              d += (c[k + 1].second - d) * (now - c[k].first) / std::max(1e-9, c[k + 1].first - c[k].first);
            own = d;
          }
          // (the player's own shape moves the breath a little, PJ_OWN_BREATH 0.4 of it: a reed's or a
          // brass player's level follows the breath strongly; the rest is heard as its loudness)
          static const double kOwnBreath = knob("PJ_OWN_BREATH", 0.4);
          p.ens->all(&Performer::setBreath, lv - 0.5 + kOwnBreath * own);
          lv += own;
          // (A wind's breath moves its level only a little (the flute: ~4 dB over its whole
          // range, the tone changing more); the director's dynamics and its note-by-note speech
          // are heard as the player's loudness too: PJ_DIRECTOR_WINDDB (16) dB per unit of level.)
          static const double kWindDb = knob("PJ_DIRECTOR_WINDDB", 16);
          const Instrument& wi = *p.inst;
          if ((wi.kind == Kind::kFlute || wi.kind == Kind::kDuct || wi.kind == Kind::kEndBlown || wi.reed() || wi.lip()) &&
              std::strcmp(wi.family, "bagpipe") != 0)
            // (the horn turns its bell with the dynamics, src/hornplayer.cc: ~20 dB per unit of
            // level by itself, measured pp .. ff 16 dB; no gain on top, or its line doubles)
            p.dirGain = std::strcmp(wi.name, "horn") == 0 && knob("PJ_BELLTURN", 1) > 0 ? 1.0f : float(fastPow(10.0, kWindDb * (lv - 0.5) / 20));
        }
      // A Tonewheel's planned Rotary nudges: in the middle of a long note, where no event comes, so
      // by the clock (their ticks turned to seconds by the tempo map).
      for (auto& [id, p] : parts) {
        if (!p.hplan) continue;
          while (p.boostNext < p.hplan->rotaryBoost.size()) {
            const double b = double(p.hplan->rotaryBoost[p.boostNext].first) / song.div;
            size_t k = 0;
            while (k + 1 < song.tempo.size() && song.tempo[k + 1].beat <= b) ++k;
            const double at = song.tempo.empty() ? b * 0.5 : song.tempo[k].t + (b - song.tempo[k].beat) * song.tempo[k].len;
            if (at > now) break;
            if (auto* hp = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0))) hp->setRotaryBoost(p.hplan->rotaryBoost[p.boostNext].second);
            ++p.boostNext;
          }
          while (p.thinNext < p.hplan->thin.size()) {  // (a trill's thinned registration, by the clock)
            const double b = double(p.hplan->thin[p.thinNext].first) / song.div;
            size_t k = 0;
            while (k + 1 < song.tempo.size() && song.tempo[k + 1].beat <= b) ++k;
            const double at = song.tempo.empty() ? b * 0.5 : song.tempo[k].t + (b - song.tempo[k].beat) * song.tempo[k].len;
            if (at > now) break;
            if (auto* hp = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0))) hp->setThin(p.hplan->thin[p.thinNext].second);
            ++p.thinNext;
          }
          // (the tick's time by the tempo map)
          auto when = [&](long tick) {
            const double b = double(tick) / song.div;
            size_t k = 0;
            while (k + 1 < song.tempo.size() && song.tempo[k + 1].beat <= b) ++k;
            return song.tempo.empty() ? b * 0.5 : song.tempo[k].t + (b - song.tempo[k].beat) * song.tempo[k].len;
          };
          auto* hpl = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0));
          // The re-registrations (inside a held final chord too, no event to carry them).
          while (p.regNext < p.hplan->upperChanges.size() && when(p.hplan->upperChanges[p.regNext].first) <= now) {
            if (hpl)
              hpl->setUpper(p.hplan->upperChanges[p.regNext].second, p.hplan->lowerExtra, p.hplan->clarity,
                            p.regNext < p.hplan->upperInstant.size() && p.hplan->upperInstant[p.regNext], p.hplan->lowerShape);
            ++p.regNext;
          }
          // The run switch.
          while (p.runNext < p.hplan->run.size() && when(p.hplan->run[p.runNext].first) <= now) {
            if (hpl) hpl->setRun(p.hplan->run[p.runNext].second);
            ++p.runNext;
          }
          // The expression pedal's planned shape: linear between its points (1 before the first).
          if (hpl && !p.hplan->swell.empty()) {
            const auto& sw = p.hplan->swell;
            while (p.swellNext < sw.size() && when(sw[p.swellNext].first) <= now) ++p.swellNext;
            double f = 1;
            if (p.swellNext >= sw.size()) f = sw.back().second;
            else if (p.swellNext > 0) {
              const double a = when(sw[p.swellNext - 1].first), b = when(sw[p.swellNext].first);
              const double x = b > a ? (now - a) / (b - a) : 1;
              f = sw[p.swellNext - 1].second + (sw[p.swellNext].second - sw[p.swellNext - 1].second) * std::clamp(x, 0.0, 1.0);
            }
            hpl->setExprShape(f);
          }
          // Notes let go early (the percussion re-armed for the next).
          for (size_t r = 0; r < p.hrel.size();) {
            if (when(p.hrel[r].first) > now) { ++r; continue; }
            auto k = p.tonewheelDiv.find(p.hrel[r].second);
            if (k != p.tonewheelDiv.end()) {
              if (hpl) hpl->noteOffManual(k->second, p.hrel[r].second & 127);
              p.tonewheelDiv.erase(k);
            }
            p.hrel.erase(p.hrel.begin() + r);
          }
          // The glissandos' keys.
          while (p.hxNext < p.hx.size() && when(std::get<0>(p.hx[p.hxNext])) <= now) {
            const auto& [tk, on, key, vel, div] = p.hx[p.hxNext];
            if (hpl) {
              if (on) hpl->noteOnManual(div, key, vel);
              else hpl->noteOffManual(div, key);
            }
            ++p.hxNext;
          }
          // (... and its speed changes by the clock too: a flick ends inside a held chord, with no
          // event to carry it)
          while (p.rotaryNext < p.hplan->rotary.size()) {
            const double b = double(p.hplan->rotary[p.rotaryNext].first) / song.div;
            size_t k = 0;
            while (k + 1 < song.tempo.size() && song.tempo[k + 1].beat <= b) ++k;
            const double at = song.tempo.empty() ? b * 0.5 : song.tempo[k].t + (b - song.tempo[k].beat) * song.tempo[k].len;
            if (at > now) break;
            if (auto* hp = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0))) hp->setRotary(p.hplan->rotary[p.rotaryNext].second ? 2 : 1);
            ++p.rotaryNext;
          }
      }
      for (; next < song.ev.size() && song.ev[next].t < now; ++next) {
        const Event& e = song.ev[next];
        const int ch = e.status & 15, kind = e.status & 0xf0;
        // A Tonewheel's planned Rotary: its speed changes from their tick on.
        for (auto& [id, p] : parts)
          if (p.hplan)
            while (p.rotaryNext < p.hplan->rotary.size() && p.hplan->rotary[p.rotaryNext].first <= long(e.tick)) {
              if (auto* hp = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0))) hp->setRotary(p.hplan->rotary[p.rotaryNext].second ? 2 : 1);
              ++p.rotaryNext;
            }
        for (auto& [id, p] : parts)
          if (p.hplan)
            while (p.percNext < p.hplan->percussion.size() && p.hplan->percussion[p.percNext].first <= long(e.tick)) {
              if (auto* hp = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0))) hp->setPercussion(p.hplan->percussion[p.percNext].second);
              ++p.percNext;
            }
        for (auto& [id, p] : parts)
          if (p.hplan)
            while (p.lvibNext < p.hplan->lowerVib.size() && p.hplan->lowerVib[p.lvibNext].first <= long(e.tick)) {
              if (auto* hp = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0))) hp->setLowerVibrato(p.hplan->lowerVib[p.lvibNext].second);
              ++p.lvibNext;
            }
        for (auto& [id, p] : parts)
          if (p.hplan)
            while (p.regNext < p.hplan->upperChanges.size() && p.hplan->upperChanges[p.regNext].first <= long(e.tick)) {
              if (auto* hp = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0)))
                hp->setUpper(p.hplan->upperChanges[p.regNext].second, p.hplan->lowerExtra, p.hplan->clarity,
                             p.regNext < p.hplan->upperInstant.size() && p.hplan->upperInstant[p.regNext], p.hplan->lowerShape);
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
        if (e.skip) continue;
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
          // (the file's keys are sounding pitches, and so are the ensemble's: a transposing
          // instrument's `transpose` is between its written notes and these, not added here)
          heldKey[ch * 128 + e.a] = pk;
          if (ch != 9)
            band.on(ch, e.a, heldKey[ch * 128 + e.a], e.b, bandFamily(it->second.inst->family, it->second.inst->name), byEar(it->second),
                    std::strcmp(it->second.inst->name, "koto") == 0);
          if (it->second.plan) {  // (an organ by its plan: the note on its track's division)
            Part& op = it->second;
            const int line = ch * 1000 + e.track;
            const int div = op.manualOf.count(line) ? op.manualOf[line] : 0;
            const int key = pk;
            if (auto* pl = dynamic_cast<OrganPlayer*>(&op.ens->player(0))) {
              auto old = op.organKey.find(ch * 128 + e.a);
              if (old != op.organKey.end()) pl->noteOffManual(old->second.first, old->second.second);
              // (jyrki-heuristic: what the score has sounding a moment after this key)
              if (const double ahead = organTuningLookAhead(); ahead > 0) {
                bool on[128] = {};
                for (const auto& [k, dk] : op.organKey) on[dk.second & 127] = true;
                on[key & 127] = true;
                for (size_t q = next + 1; q < song.ev.size() && song.ev[q].t <= e.t + ahead; ++q) {
                  const Event& f = song.ev[q];
                  const int fk = f.status & 0xf0, fc = f.status & 15;
                  if (f.skip || (fk != 0x80 && fk != 0x90)) continue;
                  int fp = f.a;
                  if (fc == 9 || partOf(fc, f.a, &fp) != id) continue;
                  on[fp & 127] = fk == 0x90 && f.b > 0;
                }
                int keys[64], nk = 0;
                for (int m = 0; m < 128 && nk < 64; ++m)
                  if (on[m]) keys[nk++] = m;
                pl->lookAhead(keys, nk);
              }
              pl->noteOnManual(div, key, e.touch);
              op.organKey[ch * 128 + e.a] = {div, key};
            }
            ++op.played;
            continue;
          }
          if (it->second.hplan) {  // (a Tonewheel by its plan: the note on its division)
            Part& hp = it->second;
            auto d = hp.hplan->division.find({ch * 128 + e.a, long(e.tick)});
            const int div = d != hp.hplan->division.end() ? d->second : TonewheelModel::kUpper;
            if (auto* pl = dynamic_cast<TonewheelPlayer*>(&hp.ens->player(0))) {
              auto old = hp.tonewheelDiv.find(ch * 128 + e.a);
              if (old != hp.tonewheelDiv.end()) pl->noteOffManual(old->second, e.a);
              auto tv = hp.hplan->velocity.find({ch * 128 + e.a, long(e.tick)});  // (a trill's rising touch)
              pl->noteOnManual(div, e.a, tv != hp.hplan->velocity.end() ? tv->second : e.b);
              hp.tonewheelDiv[ch * 128 + e.a] = div;
              auto rl = hp.hplan->release.find({ch * 128 + e.a, long(e.tick)});  // (to be let go early)
              if (rl != hp.hplan->release.end()) hp.hrel.push_back({rl->second, ch * 128 + e.a});
            }
            ++hp.played;
            continue;
          }
          if (it->second.singer) { ++it->second.played; continue; }  // (sung from the part read ahead)
          if (e.cue >= 0 && e.cue < int(dir.cues.size()) && (dir.acts || dir.self || dir.vib || e.cue >= ownCueFrom))
            it->second.ens->direct(dir.cues[e.cue]);
          if (it->second.bplan)  // (the brass section: what the song says about this key)
            if (const BrassHint* bh = it->second.bplan->find(e.a, e.t)) it->second.ens->brassHint(*bh);
          // (a drum: the stroke that sounds it at the SoundFonts' level: drumkit.h)
          it->second.ens->noteOn(pk, ch == 9 ? drumVelocity(e.a, kitOf(prog[ch]), e.b) : e.b);
          ++it->second.played;
        } else if (kind == 0x80 || kind == 0x90) {
          auto it = noteOwner.find(ch * 128 + e.a);
          if (it == noteOwner.end()) continue;
          if (ch != 9) band.off(ch, e.a);
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
            auto k = np.tonewheelDiv.find(ch * 128 + e.a);
            if (k != np.tonewheelDiv.end()) {
              if (auto* pl = dynamic_cast<TonewheelPlayer*>(&np.ens->player(0))) pl->noteOffManual(k->second, e.a);
              np.tonewheelDiv.erase(k);
            }
            noteOwner.erase(it);
            continue;
          }
          if (!np.singer) np.ens->noteOff(heldKey[ch * 128 + e.a]);
          noteOwner.erase(it);
        } else if (kind == 0xb0) {
          // (a Tonewheel's part hears the controllers of every channel it plays)
          auto hears = [&](int id, const Part& p) {
            return id / 128 == ch || (ch != 9 && std::strcmp(p.inst->family, "tonewheel") == 0 && isTonewheel(forProgram(prog[ch])));
          };
          // (a Tonewheel takes the sustain pedal and the mod wheel as its Rotary's switch only from a
          // channel the file wrote for a Tonewheel: a reinstrumented piano's pedalling is not the
          // Rotary's, and would throw it, its relay clicking, at every pedal change)
          auto rotaryFrom = [&](const Part& p) { return std::strcmp(p.inst->family, "tonewheel") != 0 || fileTonewheel(prog[ch]); };
          if (e.a == 64) band.pedal(ch, e.b >= 64);
          if (e.a == 64)  // the sustain pedal
            for (auto& [id, p] : parts)
              if (hears(id, p) && rotaryFrom(p)) {
                if (e.b < 64) {
                  const auto it = releaseSpeed.find({ch, std::llround(e.t * 1e4)});
                  if (it != releaseSpeed.end())
                    for (int q = 0; q < p.ens->size(); ++q)
                      if (auto* pp = dynamic_cast<PianoPlayer*>(&p.ens->player(q))) pp->planRelease(it->second);
                  if (gentleUp.count({ch, std::llround(e.t * 1e4)}))
                    for (int q = 0; q < p.ens->size(); ++q)
                      if (auto* pp = dynamic_cast<PianoPlayer*>(&p.ens->player(q))) pp->planGentleRelease();
                }
                p.ens->sustainPedal(e.b);
              }
          if (e.a == 7) chVol[ch] = e.b / 127.0;
          if (e.a == 11) {  // expression: the mixer's fader; the church organ's swell pedal, the Tonewheel's expression pedal instead
            bool swell = false;
            for (auto& [id, p] : parts)
              if (hears(id, p) && (std::strcmp(p.inst->family, "organ") == 0 || std::strcmp(p.inst->family, "tonewheel") == 0)) {
                // (a plan's swell wins; a Tonewheel's pedal follows its tune's channels only, the
                // accompaniment's loudness being in its registration)
                if (!p.plan && !(p.hplan && !p.hplan->exprChannels.count(ch))) p.ens->all(&Performer::setBreath, e.b / 127.0 - 0.5);
                swell = true;
              }
            if (!swell) chExpr[ch] = e.b / 127.0;
            for (auto& [id, p] : parts)  // (the brass section: a swell moves its players' breath too)
              if (id / 128 == ch && brassSectionInstrument(*p.inst)) p.ens->brassExpression(e.b / 127.0);
          }
          if (e.a == 1)  // the mod wheel: the harmonica's hand tremolo; the Tonewheel's Rotary (fast from 64); the timpani's roll;
                         // the electric piano's Leslies (fast from 64)
            for (auto& [id, p] : parts)
              if (hears(id, p) && (std::strcmp(p.inst->name, "harmonica") == 0 || std::strcmp(p.inst->name, "timpani") == 0 ||
                                   std::strcmp(p.inst->name, "electric-piano-1") == 0 ||
                                   std::strcmp(p.inst->name, "fretless-bass") == 0 || std::strncmp(p.inst->name, "vibraphone", 10) == 0 ||
                                   (std::strcmp(p.inst->family, "tonewheel") == 0 && rotaryFrom(p))))
                p.ens->all(&Performer::setFlutter, e.b / 127.0);
          if (e.a == 82)  // the Rotary's switch (as the digital rotary speaker's default): stop, slow, fast
            for (auto& [id, p] : parts)
              if (hears(id, p) && std::strcmp(p.inst->family, "tonewheel") == 0)
                if (auto* hp = dynamic_cast<TonewheelPlayer*>(&p.ens->player(0))) hp->setRotary(e.b < 43 ? 0 : e.b < 86 ? 1 : 2);
          if (e.a == 123 || e.a == 120)  // all notes off
            for (auto& [k, id] : noteOwner)
              if (k / 128 == ch) { parts[id].ens->noteOff(heldKey[k]); band.off(ch, k % 128); }
        } else if (kind == 0xd0) {  // channel pressure: the breath (a Clavi: its keys pressed harder)
          for (auto& [id, p] : parts)
            if (id / 128 == ch) p.ens->all(&Performer::setBreath, 0.5 * e.a / 127.0);
        } else if (kind == 0xe0) {  // pitch bend: the lips (a Clavi: the wah)
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
      // What the band sounds (src/band.h): the ringing notes fall; a part that tunes its keys by ear
      // (the fretless, the winds) writes back where it has them.
      band.step(double(kBlock) / kRate);
      if (kBandLog && band.version() != bandSeen) {  // (PJ_BAND_LOG: the set whenever it changes)
        bandSeen = band.version();
        std::string d = strf("band %.3f:", now);
        for (const BandNote& bn : band.notes()) d += strf(" %d:%d%s(%.2f)", bn.ch, bn.pitch, bn.down ? "" : "~", bn.level);
        say(stderr, "%s\n", d.c_str());
      }
      for (const BandNote& bn : band.notes()) {
        auto o = noteOwner.find(bn.ch * 128 + bn.key);
        if (o == noteOwner.end()) continue;
        auto pt = parts.find(o->second);
        if (pt == parts.end()) continue;
        if (pt->second.ens->listens()) {  // (a wind by ear)
          band.setCents(bn.ch, bn.key, pt->second.ens->keyCents(bn.pitch));
          band.setEarShare(bn.ch, bn.key, pt->second.ens->keyShare(bn.pitch));
        }
        else if (pt->second.ens->size() == 1) band.setCents(bn.ch, bn.key, pt->second.ens->player(0).tuning(bn.pitch));
      }
      std::fill(l.begin(), l.end(), 0.0f);
      std::fill(r.begin(), r.end(), 0.0f);
      // The parts side by side on the pool's threads (the circuits as one
      // job: they share a board); then mixed in their order, as on one thread.
      // (a part of the shared string orchestra: its fader is its players' own level there, set
      // before they play the block; its output, the orchestra's or silence, is mixed as it is)
      for (auto& [id, p] : parts) {
        if (!p.ens->sharedPart()) continue;
        const int ch = id / 128;
        p.ens->setShareGain(float(std::pow(chVol[ch] * 127 / 100.0, 2) * std::pow(chExpr[ch], 2)) * (ch != 9 ? p.gmGain * p.dirGain : 1.0f));
      }
      pool.run(int(jobs.size()), [&](int i) {
        for (Part* p : jobs[i]) {
          p->ens->render(p->l.data(), p->r.data(), kBlock);
          // (the lead singer's own plate, behind the voice: src/jazzsinger.h)
          if (p->singer) p->singer->halo(p->l.data(), p->r.data(), kBlock, p->singerSide);
        }
      });
      for (auto& [id, p] : parts) {
        // Volume and expression: as a mixing desk's fader, on the square law
        // (100 the usual: unity).
        const int ch = id / 128;
        float g = float(std::pow(chVol[ch] * 127 / 100.0, 2) * std::pow(chExpr[ch], 2));
        // (and the program's level: where the SoundFonts put it against their piano, src/gmlevels_data.h;
        // the drums have theirs key by key, src/drumcal_data.h)
        if (ch != 9) g *= p.gmGain * p.dirGain;
        if (p.ens->sharedPart()) g = 1;
        // (one Tonewheel playing several channels: one fader, the channels' own by how many of its
        // notes each plays)
        if (!p.tonewheelChans.empty()) {
          double sum = 0, w = 0;
          for (auto& [c, n] : p.tonewheelChans) {
            sum += n * std::pow(chVol[c] * 127 / 100.0, 2) * std::pow(chExpr[c], 2);
            w += n;
          }
          if (w > 0) g = float(sum / w);
        }
        for (int j = 0; j < kBlock; ++j) { l[j] += g * p.l[j]; r[j] += g * p.r[j]; }
      }
      float* o = &all[2 * n];
      for (int j = 0; j < kBlock; ++j) {
        o[2 * j] = masterGain * l[j];
        o[2 * j + 1] = masterGain * r[j];
      }
      for (int j = 0; j < 2 * kBlock; ++j) {
        const float m = std::fabs(o[j]);
        if (m > mixPeak) mixPeak = m;
        if (streaming && m > 0.9f) {  // (no second look at the whole: a soft knee over 0.9 instead of scaling it down)
          ++kneeSamples;
          o[j] = std::copysign(0.9f + 0.1f * std::tanh((m - 0.9f) / 0.1f), o[j]);
        }
      }
      ready.store(n + kBlock, std::memory_order_release);
    }
  };
  const auto t0 = std::chrono::steady_clock::now();
  if (streaming) heardLive(true);  // (to the device: the players' timed messages wait for their moment, src/heard.h)
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
      heardLive(false);
      return 1;
    }
    // Playing starts once 10 s are ready (PJ_PLAY_LEAD, s; or the whole song,
    // if shorter), and should the rendering fall behind, it waits for 10 s
    // again: never longer.
    static const double kLead = std::getenv("PJ_PLAY_LEAD") ? std::atof(std::getenv("PJ_PLAY_LEAD")) : 10.0;
    const size_t lead = size_t(std::max(0.1, kLead) * kRate);
    int waits = 0;
    static const double kTick = std::getenv("PJ_PLAY_TICK") ? std::atof(std::getenv("PJ_PLAY_TICK")) : 5.0;
    double nextTick = kTick > 0 ? (std::floor(startT / kTick + 1e-9) + 1) * kTick : 0;  // (on the song's clock)
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
      // The time every 5 s, as it is heard (what the device still holds taken off), so a muddy or
      // otherwise bad moment can be named (PJ_PLAY_TICK, s; 0: none).
      snd_pcm_sframes_t queued = 0;
      if (snd_pcm_delay(pcm, &queued) < 0) queued = 0;
      const double heard = (double(i) - std::max<snd_pcm_sframes_t>(queued, 0)) / kRate + startT;
      while (kTick > 0 && heard >= nextTick) {
        const int s = int(nextTick + 0.5);
        std::fprintf(stderr, "  %d:%02d\n", s / 60, s % 60);
        nextTick += kTick;
      }
      heardUntil(heard);
    }
    snd_pcm_drain(pcm);
    heardLive(false);
    snd_pcm_close(pcm);
    renderer.join();
    if (waits > 1) std::fprintf(stderr, "  (the rendering fell behind: %d pauses)\n", waits - 1);
    if (kneeSamples)
      std::fprintf(stderr, "  (the mix peaked at %.1f dBFS: %zu samples limited; -volume %.0f would clear them)\n",
                   20 * std::log10(mixPeak), kneeSamples, std::floor(gPlayVolumeDb + 20 * std::log10(0.9f / mixPeak)));
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
