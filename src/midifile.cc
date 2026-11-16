#include "midifile.h"

#include <alsa/asoundlib.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
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
};

struct Song {
  std::vector<Event> ev;
  std::string title;
  bool gs = false;  // a Roland GS or General MIDI 2 file (else the drum keys below 35 are Yamaha XG's)
  // The tempo map: from `t` seconds on (at `beat` quarter notes), a quarter lasts `len` s.
  struct Tempo { double t, beat, len; };
  std::vector<Tempo> tempo;
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
  // Every track's events in ticks; the tempo map from all of them.
  struct Raw { uint64_t tick; int order; uint8_t s, a, b; };
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
        if (type == 0x03 && song->title.empty() && tr <= 1) song->title.assign(d.begin() + p, d.begin() + std::min(end, p + n));
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
      raw.push_back({tick, order++, s, d[p], uint8_t(nb == 2 ? d[p + 1] : 0)});
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
    song->ev.push_back({t0 + (r.tick - tempo[k].first) * tempo[k].second, r.order, r.s, r.a, r.b});
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
    // Organs, accordions, harmonica: the calliope's pipes, a reedy square.
    {"drawbar-organ", "lead-calliope"}, {"percussive-organ", "lead-calliope"}, {"rock-organ", "lead-calliope"},
    {"church-organ", "lead-calliope"}, {"reed-organ", "lead-square"}, {"accordion", "lead-square"},
    {"harmonica", "lead-square"}, {"tango-accordion", "lead-square"},
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

const Instrument* forProgram(int prog) {
  for (const char* nm : kGm[prog & 127])
    if (nm)
      if (const Instrument* in = findInstrument(nm)) return in;
  return nullptr;
}

// One player of the song: a channel's instrument (or a drum's), as a pool.
struct Part {
  const Instrument* inst;
  std::unique_ptr<Ensemble> ens;
  int maxPoly = 1, maxHeld = 1, played = 0;  // (voices: notes held at once, their tails counted)
  double vol = 100 / 127.0, expr = 1;
  std::vector<float> l, r;
};

}  // namespace

int playMidiFile(const std::string& path, const char* outWav, const RoomSpec* room, const std::string& device,
                 bool verbose) {
  Song song;
  std::string err;
  if (!readSong(path, &song, &err)) { std::fprintf(stderr, "-play: %s\n", err.c_str()); return 1; }
  if (song.ev.empty()) { std::fprintf(stderr, "-play: no events in %s\n", path.c_str()); return 1; }
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
  auto partOf = [&](int ch, int key, int* playKey) -> int {
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
        if (c.valid) {
          c.beatLen = song.tempo[tk].len;
          c.beat = song.tempo[tk].beat + (now - song.tempo[tk].t) / song.tempo[tk].len;
        }
        // The band's silences: what sounds now, what was last begun, what comes next.
        while (ahead < song.ev.size() && (song.ev[ahead].t < now || (song.ev[ahead].status & 0xf0) != 0x90 || song.ev[ahead].b == 0)) ++ahead;
        const double toNext = ahead < song.ev.size() ? song.ev[ahead].t - now : 1e9;
        c.pause = heldAll <= 0 && now - lastOn > 2 * c.beatLen && toNext > c.beatLen;
      }
      for (; next < song.ev.size() && song.ev[next].t < now; ++next) {
        const Event& e = song.ev[next];
        const int ch = e.status & 15, kind = e.status & 0xf0;
        if (kind == 0x90 && e.b > 0) {
          lastOn = e.t;
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
          it->second.ens->noteOn(pk + it->second.inst->transpose, e.b);
          ++it->second.played;
        } else if (kind == 0x80 || kind == 0x90) {
          auto it = noteOwner.find(ch * 128 + e.a);
          if (it == noteOwner.end()) continue;
          parts[it->second].ens->noteOff(heldKey[ch * 128 + e.a]);
          noteOwner.erase(it);
        } else if (kind == 0xb0) {
          if (e.a == 64)  // the sustain pedal
            for (auto& [id, p] : parts)
              if (id / 128 == ch) p.ens->setHold(e.b >= 64);
          if (e.a == 7) chVol[ch] = e.b / 127.0;
          if (e.a == 11) chExpr[ch] = e.b / 127.0;
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
        const float g = float(std::pow(chVol[ch] * 127 / 100.0, 2) * std::pow(chExpr[ch], 2));
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
