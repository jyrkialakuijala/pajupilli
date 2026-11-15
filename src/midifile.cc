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
#include "ensemble.h"
#include "instrument.h"
#include "performer.h"

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
      if (s == 0xf0 || s == 0xf7) { p += var(); continue; }
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

// Channel 10: a drum key (35..81) to an instrument built and the key it plays there (-1: none).
struct Drum { const char* inst; int key; };
Drum drumFor(int k) {
  switch (k) {
    case 35: case 36: return {"taiko", 36};                                   // bass drum
    case 37: case 75: case 76: case 77: return {"woodblock", k == 77 ? 67 : 72};  // rim, claves, wood blocks
    case 38: case 40: case 39: return {"synth-drum", 50};                     // snare, clap
    case 41: case 43: case 45: case 47: case 48: case 50:                     // toms, low to high
      return {"melodic-tom", 43 + (k - 41)};
    case 56: case 67: case 68: return {"agogo", k == 68 ? 72 : 77};           // cowbell, agogos
    case 60: case 61: case 62: case 63: case 64: case 65: case 66:            // bongos, congas, timbales
      return {"melodic-tom", 52 + (k - 60) * 2};
    default: return {nullptr, -1};                                           // cymbals, hi-hats, shakers: not built
  }
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
  // First pass: which parts there are, and how many notes each holds at once.
  std::map<int, Part> parts;  // key: channel * 128 + program (channel 9: 9 * 128 + drum instrument index)
  std::vector<std::string> drumNames;
  auto drumIndex = [&](const char* nm) {
    for (size_t i = 0; i < drumNames.size(); ++i)
      if (drumNames[i] == nm) return int(i);
    drumNames.push_back(nm);
    return int(drumNames.size() - 1);
  };
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
      const Drum dr = drumFor(key);
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
  std::fprintf(stderr, "%.1f s, %zu parts:\n", song.ev.back().t, parts.size());
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
    if (p.inst->oneVoice())
      std::fprintf(stderr, "  %s %2d: %s (%d at once, one voice)\n", id / 128 == 9 ? "drums  " : "channel", id / 128 + 1,
                   p.inst->name, p.maxHeld);
    else
      std::fprintf(stderr, "  %s %2d: %s (%d held at once; %d voices for their tails)\n", id / 128 == 9 ? "drums  " : "channel",
                   id / 128 + 1, p.inst->name, p.maxHeld, poly);
  }
  if (skipped) std::fprintf(stderr, "  (%d drum notes on cymbals, hi-hats and others not built: silent)\n", skipped);
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
  // the file) takes what is ready: playing starts once a second is ahead, and
  // waits should the rendering fall behind.
  std::vector<float> all(2 * frames, 0.0f);
  std::atomic<size_t> ready{0};
  const bool streaming = outWav == nullptr;
  auto renderSong = [&]() {
    std::vector<float> l(kBlock), r(kBlock);
    size_t next = 0;
    for (size_t n = 0; n < frames; n += kBlock) {
      const double now = (n + kBlock) / double(kRate);
      for (; next < song.ev.size() && song.ev[next].t < now; ++next) {
        const Event& e = song.ev[next];
        const int ch = e.status & 15, kind = e.status & 0xf0;
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
      for (auto& [id, p] : parts) {
        p.l.resize(kBlock);
        p.r.resize(kBlock);
        p.ens->render(p.l.data(), p.r.data(), kBlock);
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
    const size_t lead = kRate;  // a second ahead before playing, and after a wait
    int waits = 0;
    for (size_t i = 0; i < frames;) {
      size_t have = ready.load(std::memory_order_acquire);
      if (have <= i) {
        // The rendering is behind (or not yet a second ahead at the start):
        // wait until, at the speed it has kept so far, it will stay ahead to
        // the end (a song heavier than real time: one long wait, not many).
        ++waits;
        size_t need = i + lead;
        if (i > 0) {
          const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
          const double speed = have / std::max(secs, 1e-3) / kRate;  // x real time
          if (speed < 1) need += size_t((frames - i) * (1 - speed));
          std::fprintf(stderr, "\r  (rendering at %.2f x real time: waiting %.0f s)\n", speed,
                       (std::min(frames, need) - have) / std::max(speed, 0.05) / kRate);
        }
        while ((have = ready.load(std::memory_order_acquire)) < std::min(frames, need))
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
