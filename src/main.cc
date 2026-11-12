// pajupilli: a physical model of a Boehm concert flute.
//
//   ./flute -note 69 -o a4.wav        play one note (MIDI), print its pitch
//   ./flute -scale                    play C4..C7, print a tuning table
//   ./flute -imp 69                   passive resonances of a fingering
//   ./flute -slur 71 72 -dyn 0.2      slur into a note; print its pitch and level wobble
//   ./flute -inst piccolo ...         piccolo, alto or bass flute, flute-c (default: flute, B foot);
//                                     notes are sounding MIDI pitches
//   ./flute -h                        all options
//   ./flute -live                     play it from the MPK mini mk3
//       [-dev hw:0] [-midi MPK] [-period 64] [-periods 3] [-transpose 12] [-v] [-selftest]
#include <cmath>
#include <memory>
#include <random>
#if defined(__SSE__)
#include <xmmintrin.h>
#endif
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "analysis.h"
#include "calibrate.h"
#include "ensemble.h"
#include "flute.h"
#include "live.h"
#include "player.h"
#include "room.h"
#include "voice.h"

using namespace pajupilli;

namespace {

constexpr int kFs = 48000;

struct Result { double f, clarity, rms; };

Calibration gCal;
const Instrument* gInst = &concertFlute();

Result playNote(int midi, double secs, double dyn, std::vector<float>* keep) {
  Voice v(*gInst);
  Player& pl = v.player();
  pl.setCalibration(gCal);
  pl.setDynamics(dyn);
  const int n = int(secs * kFs);
  std::vector<float> x(n);
  pl.noteOn(midi, 90);
  v.render(x.data(), n);
  const int a = n * 2 / 3, len = n - a;
  const double f0 = noteFrequency(midi);
  Result r;
  r.f = estimatePitch(x.data() + a, len, kFs, f0 / 2.2, f0 * 2.2, &r.clarity);
  r.rms = rms(x.data() + a, len);
  if (keep) keep->insert(keep->end(), x.begin(), x.end());
  return r;
}

double cents(double f, double ref) { return 1200 * std::log2(f / ref); }

const char* noteName(int m) {
  static const char* n[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "Bb", "B"};
  static char buf[16];
  std::snprintf(buf, sizeof buf, "%s%d", n[m % 12], m / 12 - 1);
  return buf;
}

// Passive resonances seen by the jet: a pressure impulse in the window branch,
// the window flow's spectrum; peaks of |Y|.
void impulse(int midi) {
  Flute fl(*gInst);  // at the model's own rate
  const double fs = fl.fs();
  fl.setHoles(noteInfo(*gInst, midi)->holes);
  std::vector<float> tmp(int(fs / 10));
  fl.process(tmp.data(), int(tmp.size()));  // let the pads settle
  const int n = 1 << 16;
  std::vector<double> q(n);
  for (int i = 0; i < n; ++i) {
    fl.testDrive(i == 0 ? 1.0 : 0.0);
    float y;
    fl.process(&y, 1);
    q[i] = fl.windowFlow();
  }
  // DFT magnitude on a fine grid, peaks below 4 kHz.
  std::printf("%s fingering %s, admittance peaks (Hz):", noteName(midi), noteInfo(*gInst, midi)->holes);
  double prev2 = 0, prev1 = 0;
  const double df = 0.5;
  for (double f = 50; f < 4000 / gInst->scale; f += df) {
    double re = 0, im = 0;
    const double w = 2 * M_PI * f / fs;
    // Goertzel
    double s1 = 0, s2 = 0;
    const double c = 2 * std::cos(w);
    for (int i = 0; i < n; ++i) { const double s0 = q[i] + c * s1 - s2; s2 = s1; s1 = s0; }
    re = s1 - s2 * std::cos(w);
    im = s2 * std::sin(w);
    const double mag = re * re + im * im;
    if (prev1 > prev2 && prev1 > mag) std::printf(" %.1f", f - df);
    prev2 = prev1;
    prev1 = mag;
  }
  std::printf("  (target %.1f)\n", noteFrequency(midi));
}

}  // namespace

// Fuzz: random notes, velocities and controls for `secs`; reports NaN resets
// and the peak. Exit 1 on any non-finite sample or reset.
int fuzz(double secs, unsigned seed) {
  Voice v(*gInst);
  Player& pl = v.player();
  Flute& fl = v.flute();
  const int lo = gInst->lo + gInst->transpose, span = gInst->hi - gInst->lo + 1;
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> u(0, 1);
  std::vector<float> x(64);
  double peak = 0;
  bool bad = false;
  int held = -1;
  for (long n = 0; n < long(secs * kFs); n += 64) {
    if (u(rng) < 0.02) {
      const int m = lo + int(u(rng) * span);
      pl.noteOn(m, 1 + int(u(rng) * 126));
      if (held >= 0 && u(rng) < 0.7) pl.noteOff(held);
      held = m;
    }
    if (u(rng) < 0.005 && held >= 0) { pl.noteOff(held); held = -1; }
    if (u(rng) < 0.01) pl.setDynamics(u(rng));
    if (u(rng) < 0.01) pl.setLipRoll(2 * u(rng) - 1);
    if (u(rng) < 0.01) pl.setBreath(u(rng) - 0.5);
    if (u(rng) < 0.005) pl.setFlutter(u(rng) < 0.5 ? 0 : u(rng));
    if (u(rng) < 0.005) pl.setVibrato(0.15 * u(rng));
    if (u(rng) < 0.003) pl.setHold(u(rng) < 0.5);
    v.render(x.data(), 64);
    for (float v : x) { bad |= !std::isfinite(v); peak = std::max(peak, double(std::fabs(v))); }
  }
  std::printf("fuzz %.0f s seed %u: peak %.3f, resets %d, %s\n", secs, seed, peak, fl.resets(),
              bad || fl.resets() ? "FAIL" : "ok");
  return bad || fl.resets() ? 1 : 0;
}

// A NaN injected into the window must not silence the flute for good.
int nanTest() {
  Voice v(*gInst);
  Player& pl = v.player();
  Flute& fl = v.flute();
  pl.noteOn(69 + gInst->transpose, 80);
  std::vector<float> x(kFs / 2);
  v.render(x.data(), int(x.size()));
  fl.testDrive(NAN);
  v.render(x.data(), int(x.size()));
  const double r = rms(x.data() + x.size() / 2, int(x.size() / 2));
  bool finite = true;
  for (float v : x) finite &= std::isfinite(v);
  std::printf("nan recovery: resets %d, finite %d, rms after %.4f: %s\n", fl.resets(), finite, r,
              fl.resets() == 1 && finite && r > 1e-3 ? "ok" : "FAIL");
  return fl.resets() == 1 && finite && r > 1e-3 ? 0 : 1;
}

// Slur from `from` into `to` (as on the keyboard: overlap, then release) and
// measure the settled note in 20 ms frames: mean pitch, pitch wobble (std of
// cents), level wobble (peak-to-peak / mean) and frames in another register.
int slurTest(int from, int to, double dyn, double secs, std::vector<float>* keep) {
  Voice voice(*gInst);
  Player& pl = voice.player();
  pl.setCalibration(gCal);
  pl.setDynamics(dyn);
  pl.setWander(0);  // measure the model, not the player's natural drift
  std::vector<float> a(int(0.4 * kFs)), x(int(secs * kFs));
  pl.noteOn(from, 70);
  voice.render(a.data(), int(a.size()));
  pl.noteOn(to, 70);
  pl.noteOff(from);
  voice.render(x.data(), int(x.size()));
  if (keep) { keep->insert(keep->end(), a.begin(), a.end()); keep->insert(keep->end(), x.begin(), x.end()); }
  const double f0 = noteFrequency(to);
  const int n = int(std::max(0.02, 3 / f0) * kFs);  // 20 ms, >= 3 periods
  std::vector<double> cs, ls;
  int other = 0;
  // Register by the spectrum (a bright note's third harmonic can outweigh its
  // fundamental and fool a pitch tracker): in register if the fundamental is
  // within 25 dB of the strongest of f0 .. 4 f0 and no subharmonic rivals it.
  auto level = [&](const float* y, double f) {  // Hann-windowed Goertzel, power
    const double w = 2 * M_PI * f / kFs, c = 2 * std::cos(w);
    double s1 = 0, s2 = 0;
    for (int k = 0; k < n; ++k) {
      const double s0 = y[k] * (0.5 - 0.5 * std::cos(2 * M_PI * k / n)) + c * s1 - s2;
      s2 = s1;
      s1 = s0;
    }
    return s1 * s1 + s2 * s2 - c * s1 * s2;
  };
  for (int i = int(0.15 * kFs); i + n <= int(x.size()); i += n) {
    const float* y = x.data() + i;
    const double l1 = level(y, f0);
    double lmax = l1;
    for (int h = 2; h <= 4 && h * f0 < 0.45 * kFs; ++h) lmax = std::max(lmax, level(y, h * f0));
    const double sub = std::max(level(y, f0 / 2), level(y, f0 * 1.5));
    if (l1 < lmax * 3e-3 || sub > l1 * 0.03) { ++other; continue; }
    const double f = estimatePitch(y, n, kFs, f0 / 1.3, f0 * 1.3);
    const double c = f > 0 ? cents(f, f0) : 1e4;
    if (std::fabs(c) > 300) { ++other; continue; }
    cs.push_back(c);
    ls.push_back(rms(x.data() + i, n));
  }
  double m = 0, v = 0, lm = 0, lmin = 1e9, lmax = 0;
  for (double c : cs) m += c;
  m /= std::max<size_t>(1, cs.size());
  for (double c : cs) v += (c - m) * (c - m);
  v = std::sqrt(v / std::max<size_t>(1, cs.size()));
  for (double l : ls) { lm += l; lmin = std::min(lmin, l); lmax = std::max(lmax, l); }
  lm /= std::max<size_t>(1, ls.size());
  const std::string fromName = noteName(from), toName = noteName(to);
  std::printf("slur %s -> %s dyn %.2f: mean %+.0f cents, wobble %.1f cents, level %.4f wobble %.0f %%, "
              "other register %d/%d frames\n", fromName.c_str(), toName.c_str(), dyn, m, v, lm,
              ls.empty() ? 0.0 : 100 * (lmax - lmin) / lm, other, other + int(cs.size()));
  return 0;
}

const char kUsage[] =
    "pajupilli: a physical model of a Boehm flute.\n"
    "\n"
    "usage: flute [-inst NAME] MODE [options]\n"
    "\n"
    "Instrument:\n"
    "  -inst NAME          flute (default, B foot), flute-c (C foot), piccolo,\n"
    "                      alto or bass. Notes are sounding MIDI pitches (flute\n"
    "                      59..96, flute-c 60..96, piccolo 74..107, alto 55..83,\n"
    "                      bass 48..74)\n"
    "  -cfoot              the flute with a C foot (no low B key): same as\n"
    "                      -inst flute-c\n"
    "\n"
    "Modes:\n"
    "  -live               play from a MIDI keyboard (Akai MPK mini mk3)\n"
    "  -selftest           -live without a keyboard: plays an arpeggio\n"
    "  -note M             play one note, print its pitch\n"
    "  -scale              play the whole range, print a tuning table\n"
    "  -demo               a slurred scale and a short phrase\n"
    "  -slur A B           slur from A into B, print pitch and level wobble\n"
    "  -imp M              passive resonances of a fingering\n"
    "  -cal                relearn the tuning (cached in ~/.cache/pajupilli)\n"
    "  -fuzz SECS SEED     random playing, check for blow-ups\n"
    "  -nantest            check recovery from NaN states\n"
    "  -scatter            how much a silent instrument gives back of the room's\n"
    "                      sound, per octave band\n"
    "  -roomimp ROOM       the room at the microphones: room vs direct sound,\n"
    "                      left/right balance and correlation over time\n"
    "  -polydemo           a line, a held note under a melody, chords, a rolled\n"
    "                      chord; with -ensemble also a chorale (-o to save,\n"
    "                      stereo)\n"
    "\n"
    "Options:\n"
    "  -o FILE.wav         write the sound (48 kHz)\n"
    "  -secs S             note length (default 1)\n"
    "  -dyn D              dynamics 0 (piano) .. 1 (forte), default 0.5\n"
    "  -dump FILE          with -note: raw float32 window pressure\n"
    "\n"
    "Live options:\n"
    "  -dev PCM            ALSA device (default: default, via PipeWire)\n"
    "  -midi NAME          MIDI client name to connect to (default: MPK)\n"
    "  -period N           frames per period (default 64)\n"
    "  -periods N          periods in the buffer (default 3)\n"
    "  -transpose N        keys to sounding pitch (default: the lowest C plays\n"
    "                      the instrument's lowest octave)\n"
    "  -v                  verbose\n"
    "  -poly N             up to N flutes (default 8; 1 = a single flute): keys\n"
    "                      struck together, or held together over 100 ms, get\n"
    "                      flutes of their own\n"
    "\n"
    "Ensemble and room (live and -polydemo):\n"
    "  -ensemble [LIST]    a flute choir instead of -poly: bass, alto, two flutes\n"
    "                      and a piccolo (default LIST bass,alto,flute,flute,piccolo);\n"
    "                      notes go to the instrument they suit, chords low to high\n"
    "  -room NAME          dry, studio, hall (default) or church: the instruments\n"
    "                      play in it and hear it in their bores; stereo out\n"
    "\n"
    "Environment: PJ_NOCAL (skip calibration), PJ_NOCALCACHE (ignore the cache),\n"
    "PJ_TRACE (with -note: print model state per sample)\n"
    "\n"
    "Examples:\n"
    "  flute -live -inst alto\n"
    "  flute -note 69 -o a4.wav\n"
    "  flute -inst piccolo -scale\n";

// Scattering: a silent flute driven by 1 Pa of room noise in one octave band;
// what it radiates (Pa at 1 m) per Pa of room pressure. A passive instrument
// at 1 m gives back far less than it gets.
int scatter() {
  Flute fl(*gInst);
  const double fs = fl.fs();
  std::vector<float> tmp(int(fs / 5));
  fl.process(tmp.data(), int(tmp.size()));
  std::mt19937 rng(1);
  std::normal_distribution<float> nd(0, 1);
  for (double fc = 125; fc < 0.4 * fs && fc <= 16000; fc *= 2) {
    const int n = int(fs);
    std::vector<float> pr(n), y(n);
    // Band: two one-pole sections each way (rough octave).
    double a = 0, b = 0, c = 0, d = 0;
    const double ah = 1 - std::exp(-2 * M_PI * fc * 1.41 / fs), al = 1 - std::exp(-2 * M_PI * fc / 1.41 / fs);
    double e2 = 0;
    for (int i = 0; i < n; ++i) {
      const double x = nd(rng);
      a += ah * (x - a); b += ah * (a - b);
      c += al * (b - c); d += al * (c - d);
      pr[i] = float(b - d);
      e2 += pr[i] * pr[i];
    }
    const float sc = float(1 / std::sqrt(e2 / n));
    for (float& v : pr) v *= sc;
    fl.setRoom(pr.data(), n);
    fl.process(y.data(), n);
    double s2 = 0;
    for (int i = n / 2; i < n; ++i) s2 += double(y[i]) * y[i];
    std::printf("%6.0f Hz: radiates %.4f Pa at 1 m per Pa\n", fc, std::sqrt(s2 / (n / 2)) / fl.outScale());
  }
  return 0;
}

// The room's impulse response at the microphones for a player on the far left
// and one in the middle (five seats, 1 m apart): direct vs room energy, and
// over time the left / right balance and correlation (a diffuse field: even
// and uncorrelated, wherever the player sits).
int roomImpulse(const RoomSpec& rs) {
  if (rs.volume <= 0) { std::printf("no room\n"); return 0; }
  const std::vector<double> x = {-2, -1, 0, 1, 2};
  for (int src : {0, 2}) {
    Room room(rs, x);
    const int B = Room::kBlock, N = kFs * 2 / B * B;
    std::vector<float> L(N), R(N), zero(B, 0.0f), in(B, 0.0f), at[5];
    float* atp[5];
    for (int i = 0; i < 5; ++i) { at[i].assign(B, 0.0f); atp[i] = at[i].data(); }
    const float lg[5] = {1, 1, 1, 1, 1};
    for (int b = 0; b < N / B; ++b) {
      in[0] = b == 0 ? 1.0f : 0.0f;
      const float* srcs[5];
      for (int i = 0; i < 5; ++i) srcs[i] = i == src ? in.data() : zero.data();
      room.process(srcs, B, atp, &L[b * B], &R[b * B], lg);
    }
    // The direct sound: the first 2 ms after it arrives.
    int first = 0;
    while (first < N && std::fabs(L[first]) + std::fabs(R[first]) < 1e-6f) ++first;
    double d = 0, r = 0;
    for (int i = 0; i < N; ++i) (i < first + kFs / 500 ? d : r) += double(L[i]) * L[i] + double(R[i]) * R[i];
    std::printf("player at %+.0f m: room / direct %+.1f dB\n", x[src], 10 * std::log10(r / d));
    std::printf("  side walls: %s\n", room.lateral(src).c_str());
    const double w[][2] = {{20, 50}, {50, 100}, {100, 300}, {300, 1000}};
    for (auto& t : w) {
      double el = 0, er = 0, lr = 0;
      for (int i = first + int(t[0] * kFs / 1000); i < std::min(N, first + int(t[1] * kFs / 1000)); ++i) {
        el += double(L[i]) * L[i]; er += double(R[i]) * R[i]; lr += double(L[i]) * R[i];
      }
      std::printf("  %4.0f-%4.0f ms: left/right %+.1f dB, correlation %+.2f\n", t[0], t[1],
                  10 * std::log10(el / er), lr / std::sqrt(el * er));
    }
  }
  return 0;
}

int main(int argc, char** argv) {
#if defined(__SSE__)
  // Flush denormals to zero: decaying states must not slow the real-time path
  // (fast-math links this in too, the portable build does not).
  _mm_setcsr(_mm_getcsr() | 0x8040);
#endif
  int note = -1, imp = -1, slurFrom = -1, slurTo = -1;
  bool scale = false, demo = false, live = false, cal = false, cfoot = false, polyDemo = false;
  LiveOptions lopt;
  std::string ensemble;  // instruments, comma-separated
  const RoomSpec* room = findRoom("hall");
  double secs = 1.0, dyn = 0.5;
  const char* out = nullptr;
  const char* dump = nullptr;  // window pressure, raw float32
  if (argc < 2) { std::fputs(kUsage, stderr); return 1; }
  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    if (a == "-h" || a == "-help" || a == "--help") { std::fputs(kUsage, stdout); return 0; }
    auto next = [&]() { return i + 1 < argc ? argv[++i] : (char*)""; };
    if (a == "-inst") {
      gInst = findInstrument(next());
      if (!gInst) { std::fprintf(stderr, "-inst: flute, flute-c, piccolo, alto or bass\n"); return 1; }
    }
    else if (a == "-cfoot") cfoot = true;
    else if (a == "-note") note = std::atoi(next());
    else if (a == "-scale") scale = true;
    else if (a == "-cal") cal = true;
    else if (a == "-imp") imp = std::atoi(next());
    else if (a == "-slur") { slurFrom = std::atoi(next()); slurTo = std::atoi(next()); }
    else if (a == "-secs") secs = std::atof(next());
    else if (a == "-dyn") dyn = std::atof(next());
    else if (a == "-demo") demo = true;
    else if (a == "-live") live = true;
    else if (a == "-selftest") { live = true; lopt.selfTest = true; }
    else if (a == "-dev") lopt.device = next();
    else if (a == "-midi") lopt.midiName = next();
    else if (a == "-period") lopt.period = std::atoi(next());
    else if (a == "-periods") lopt.periods = std::atoi(next());
    else if (a == "-transpose") lopt.transpose = std::atoi(next());
    else if (a == "-v") lopt.verbose = true;
    else if (a == "-poly") lopt.poly = std::max(1, std::min(16, std::atoi(next())));
    else if (a == "-polydemo") polyDemo = true;
    else if (a == "-ensemble") {
      ensemble = "bass,alto,flute,flute,piccolo";
      if (i + 1 < argc && argv[i + 1][0] != '-') ensemble = argv[++i];
    }
    else if (a == "-room") {
      room = findRoom(next());
      if (!room) { std::fprintf(stderr, "-room: dry, studio, hall or church\n"); return 1; }
    }
    else if (a == "-fuzz") { const double fs = std::atof(next()); return fuzz(fs, unsigned(std::atoi(next()))); }
    else if (a == "-nantest") return nanTest();
    else if (a == "-scatter") return scatter();
    else if (a == "-roomimp") {
      const RoomSpec* rs = findRoom(next());
      if (!rs) { std::fprintf(stderr, "-roomimp: dry, studio, hall or church\n"); return 1; }
      return roomImpulse(*rs);
    }
    else if (a == "-o") out = next();
    else if (a == "-dump") dump = next();
    else {
      std::fprintf(stderr, "unknown option %s (try -h)\n", a.c_str());
      return 1;
    }
  }
  if (cfoot && gInst == findInstrument("flute")) gInst = findInstrument("flute-c");
  const int lo = gInst->lo + gInst->transpose, hi = gInst->hi + gInst->transpose;
  if (cal) { calibrate(*gInst, 440, stdout); return 0; }
  auto inRange = [&](int m, const char* what) {
    if (noteInfo(*gInst, m)) return true;
    std::fprintf(stderr, "%s: the %s plays MIDI %d..%d\n", what, gInst->name, lo, hi);
    return false;
  };
  if (imp >= 0) {
    if (!inRange(imp, "-imp")) return 1;
    impulse(imp);
    return 0;
  }
  if (note >= 0 && !inRange(note, "-note")) return 1;
  if (slurFrom >= 0 && (!inRange(slurFrom, "-slur") || !inRange(slurTo, "-slur"))) return 1;
  if (!std::getenv("PJ_NOCAL")) {
    // One note: learn only that one (fast); otherwise the whole range.
    if (note >= 0 && !scale && !demo && !live) gCal = calibrate(*gInst, 440, nullptr, note, note);
    else gCal = calibrate(*gInst, 440, nullptr);
  }
  // Who plays: one instrument (-poly N of it) or an ensemble, each calibrated.
  std::vector<const Instrument*> members;
  std::vector<std::unique_ptr<Calibration>> cals;
  std::vector<Ensemble::Member> roster;
  if (!ensemble.empty()) {
    for (size_t b = 0; b <= ensemble.size();) {
      size_t e = ensemble.find(',', b);
      if (e == std::string::npos) e = ensemble.size();
      const std::string nm = ensemble.substr(b, e - b);
      const Instrument* in = findInstrument(nm);
      if (!in) { std::fprintf(stderr, "-ensemble: unknown instrument '%s'\n", nm.c_str()); return 1; }
      members.push_back(in);
      b = e + 1;
    }
    for (const Instrument* in : members) {
      const Calibration* c = nullptr;
      for (size_t j = 0; j < roster.size(); ++j)
        if (members[j] == in) c = roster[j].cal;
      if (!c) {
        cals.emplace_back(new Calibration(std::getenv("PJ_NOCAL") ? Calibration() : calibrate(*in, 440, nullptr)));
        c = cals.back().get();
      }
      roster.push_back({in, c});
    }
  }
  auto makeEnsemble = [&](int poly) {
    return roster.empty() ? std::unique_ptr<Ensemble>(new Ensemble(*gInst, gCal, poly, lopt.verbose || polyDemo, room))
                          : std::unique_ptr<Ensemble>(new Ensemble(roster, lopt.verbose || polyDemo, room));
  };
  if (live) {
    auto ens = makeEnsemble(lopt.poly);
    return runLive(roster.empty() ? *gInst : concertFlute(), lopt, *ens);
  }
  if (polyDemo) {
    // One line, then a held note under a melody, then chords, then a rolled
    // chord: the pool should use 1, 2, 3, then 4 flutes (it prints "playing: 2 x flute" ...).
    auto ensp = makeEnsemble(std::max(2, lopt.poly));
    Ensemble& ens = *ensp;
    std::fprintf(stderr, "ensemble: %s\n", ens.describe().c_str());
    std::vector<float> all;
    auto play = [&](double s) {  // in 64-frame periods, as live
      std::vector<float> l(64), r(64);
      for (int k = 0; k < int(s * kFs) / 64; ++k) {
        ens.render(l.data(), r.data(), 64);
        for (int j = 0; j < 64; ++j) { all.push_back(l[j]); all.push_back(r[j]); }
      }
    };
    const int t = roster.empty() ? gInst->transpose : 0;
    auto say = [&](const char* what) { std::fprintf(stderr, "%5.2f s  %s\n", all.size() / 2.0 / kFs, what); };
    say("a slurred line");
    const int line[] = {72, 74, 76, 77, 79, 77, 76, 74, 72};
    for (int i = 0; i < 9; ++i) { ens.noteOn(line[i] + t, 80); play(0.02); if (i) ens.noteOff(line[i - 1] + t); play(0.23); }
    ens.noteOff(72 + t); play(0.4);
    say("a held C4 under a slurred melody");
    ens.noteOn(60 + t, 70);
    play(0.3);
    const int mel[] = {76, 77, 79, 81, 79, 77, 76};
    for (int i = 0; i < 7; ++i) { ens.noteOn(mel[i] + t, 80); play(0.02); if (i) ens.noteOff(mel[i - 1] + t); play(0.28); }
    ens.noteOff(76 + t); ens.noteOff(60 + t); play(0.5);
    say("chords");
    const int chords[][3] = {{60, 64, 67}, {62, 65, 69}, {64, 67, 72}, {60, 65, 69}, {60, 64, 67}};
    for (auto& c : chords) {
      for (int k = 0; k < 3; ++k) { ens.noteOn(c[k] + t, 75); play(0.01); }
      play(0.6);
      for (int k = 0; k < 3; ++k) ens.noteOff(c[k] + t);
      play(0.1);
    }
    say("a rolled chord: four keys 60 ms apart, held");
    const int roll[] = {60, 64, 67, 72};
    for (int k = 0; k < 4; ++k) { ens.noteOn(roll[k] + t, 75); play(0.06); }
    play(0.8);
    for (int k = 0; k < 4; ++k) ens.noteOff(roll[k] + t);
    play(1.0);
    if (!roster.empty()) {
      say("a chorale, five voices");
      const int ch[][5] = {{48, 55, 64, 72, 79}, {53, 57, 65, 72, 81}, {55, 59, 67, 74, 83},
                           {48, 55, 64, 72, 84}};
      for (auto& c : ch) {
        for (int k : {2, 4, 0, 3, 1}) { ens.noteOn(c[k], 72); play(0.006); }
        play(1.2);
        for (int k = 0; k < 5; ++k) ens.noteOff(c[k]);
        play(0.08);
      }
      say("two low notes together: only the bass reaches E3");
      ens.noteOn(48, 72); play(0.006); ens.noteOn(52, 72);
      play(0.8);
      ens.noteOff(52); ens.noteOff(48);
      play(2.5);  // the room rings on
    }
    say("end");
    std::fprintf(stderr, "state resets: %d\n", ens.resets());
    if (out && !writeWav(out, all, kFs, 2)) { std::fprintf(stderr, "cannot write %s\n", out); return 1; }
    return 0;
  }
  std::vector<float> slurOut;
  if (slurFrom >= 0) {
    slurTest(slurFrom, slurTo, dyn, secs, out ? &slurOut : nullptr);
    if (out && !writeWav(out, slurOut, kFs)) { std::fprintf(stderr, "cannot write %s\n", out); return 1; }
    return 0;
  }
  if (std::getenv("PJ_TRACE") && note >= 0) {  // at the model's own rate
    Flute fl(*gInst);
    Player pl(fl);
    pl.noteOn(note, 90);
    for (int i = 0; i < int(secs * fl.fs()); ++i) {
      float y;
      pl.render(&y, 1);
      std::printf("%d %g %g %g %g %g %g\n", i, y, fl.windowFlow(), fl.jetSplit(),
                  fl.windowPressure(), fl.mouthPressure(), fl.jetVelocity());
    }
    return 0;
  }
  std::vector<float> all;
  if (note >= 0 && dump) {  // at the model's own rate
    Flute fl(*gInst);
    Player pl(fl);
    pl.setDynamics(dyn);
    pl.noteOn(note, 90);
    std::vector<float> pw(int(secs * fl.fs()));
    for (float& v : pw) { float y; pl.render(&y, 1); v = float(fl.windowPressure()); }
    FILE* f = std::fopen(dump, "wb");
    if (!f) { std::fprintf(stderr, "cannot write %s\n", dump); return 1; }
    std::fwrite(pw.data(), 4, pw.size(), f);
    std::fclose(f);
    return 0;
  }
  if (note >= 0) {
    const Result r = playNote(note, secs, dyn, &all);
    std::printf("%s target %.1f Hz: played %.1f Hz (%+.0f cents) clarity %.2f rms %.4f\n",
                noteName(note), noteFrequency(note), r.f, cents(r.f, noteFrequency(note)),
                r.clarity, r.rms);
  }
  if (scale) {
    for (int m = lo; m <= hi; ++m) {
      const Result r = playNote(m, secs, dyn, &all);
      std::printf("%-4s %7.1f %7.1f %+6.0f  clarity %.2f rms %.4f\n", noteName(m),
                  noteFrequency(m), r.f, cents(r.f, noteFrequency(m)), r.clarity, r.rms);
    }
  }
  if (demo) {
    // A scale up and down (slurred), then a tongued phrase with vibrato.
    Voice v(*gInst);
    Player& pl = v.player();
    pl.setCalibration(gCal);
    pl.setDynamics(dyn);
    auto play = [&](double s) { std::vector<float> x(int(s * kFs)); v.render(x.data(), int(x.size())); all.insert(all.end(), x.begin(), x.end()); };
    // In the instrument's own octave (the piccolo from D: it has no low C).
    const int t = gInst->transpose, s0 = gInst->lo > 60 ? 62 : 60;
    int up[] = {s0, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, 81, 83, 84};
    for (int& m : up) m += t;
    int prev = -1;
    for (int m : up) { pl.noteOn(m, 70); if (prev >= 0) pl.noteOff(prev); prev = m; play(0.22); }
    for (int i = 13; i >= 0; --i) { pl.noteOn(up[i], 70); pl.noteOff(prev); prev = up[i]; play(0.22); }
    pl.noteOff(prev); play(0.5);
    pl.setVibrato(0.08);
    const int tune[][2] = {{72, 4}, {76, 2}, {79, 2}, {84, 6}, {83, 2}, {81, 2}, {79, 2}, {77, 4}, {76, 4}, {74, 2}, {72, 6}};
    for (auto& n : tune) { pl.noteOn(n[0] + t, 90); play(n[1] * 0.16); pl.noteOff(n[0] + t); play(0.03); }
    play(0.8);
  }
  if (out && !all.empty() && !writeWav(out, all, kFs)) {
    std::fprintf(stderr, "cannot write %s\n", out);
    return 1;
  }
  return 0;
}
