#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstdint>
#include <cstring>

#include "gmlevels_data.h"
#include "levels_data.h"
#include "velgain_data.h"
#include "instrument.h"
#include "knob.h"

#include <cmath>
#include <cstring>

namespace pajupilli {
namespace {

// Standard Boehm fingerings, C foot (research/keywork.md section 5).
// Holes 1..16: D#tr Dtr C# C B Bb A G# G#' G F# F E D# D C#; a 17th character
// is the B foot's C hole (left out: open; a C foot has no such hole).
constexpr NoteInfo kNotes[38] = {
    {"xxxxxxxxxxxxxxxxx", 1},  // 59 B3 (B foot)
    {"xxxxxxxxxxxxxxxx", 1},  // 60 C4
    {"xxxxxxxxxxxxxxxo", 1},  // 61 C#4
    {"xxxxxxxxxxxxxxoo", 1},  // 62 D4
    {"xxxxxxxxxxxxxooo", 1},  // 63 D#4
    {"xxxxxxxxxxxxoooo", 1},  // 64 E4
    {"xxxxxxxxxxxooooo", 1},  // 65 F4
    {"xxxxxxxxxxooxooo", 1},  // 66 F#4
    {"xxxxxxxxxooooooo", 1},  // 67 G4
    {"xxxxxxxoxooooooo", 1},  // 68 G#4
    {"xxxxxxoxoooooooo", 1},  // 69 A4
    {"xxxxxooxoooooooo", 1},  // 70 Bb4 (thumb)
    {"xxxxoooxoooooooo", 1},  // 71 B4
    {"xxxooooxoooooooo", 1},  // 72 C5
    {"xxoooooxoooooooo", 1},  // 73 C#5
    {"xxoxxxxxxxxxxxoo", 2},  // 74 D5 (C# hole vents)
    {"xxoxxxxxxxxxxooo", 2},  // 75 D#5
    {"xxxxxxxxxxxxoooo", 2},  // 76 E5
    {"xxxxxxxxxxxooooo", 2},  // 77 F5
    {"xxxxxxxxxxoxoooo", 2},  // 78 F#5 (middle finger: the ring-finger F# barely holds its register)
    {"xxxxxxxxxooooooo", 2},  // 79 G5
    {"xxxxxxxoxooooooo", 2},  // 80 G#5
    {"xxxxxxoxoooooooo", 2},  // 81 A5
    {"xxxxxooxoooooooo", 2},  // 82 Bb5
    {"xxxxoooxoooooooo", 2},  // 83 B5
    {"xxxooooxoooooooo", 2},  // 84 C6
    {"xxoooooxoooooooo", 2},  // 85 C#6
    {"xxoxxxxxxooooooo", 3},  // 86 D6 (G4 fingering, vented)
    {"xxxxxxxoxxxxxooo", 3},  // 87 D#6
    {"xxxxxxoxoxxxoooo", 3},  // 88 E6
    {"xxxxxoxxxxxooooo", 3},  // 89 F6
    {"xxxxooxxxxooxooo", 3},  // 90 F#6
    {"xxxoxxxxxooooooo", 3},  // 91 G6
    {"xxooxxxoxooooooo", 3},  // 92 G#6
    {"xxoxxxoxoxxooooo", 3},  // 93 A6
    {"xooxxooxoxxooxoo", 3},  // 94 Bb6
    {"xoxxooxxxooooxoo", 3},  // 95 B6 (D trill key as the vent: +1 c; with the D# trill key +21)
    {"xxxoxxxoxxxooxoo", 4},  // 96 C7 (no gizmo: with it the B foot's C7 is +50 c, without +29)
};

// Piccolo (C, D foot), alto (G) and bass (C) flutes: Boehm system, the same
// fingerings. Bores: piccolo ~10.5 mm (here a cylindrical-bore piccolo with the
// flute's head taper), alto 26 mm, bass 35 mm. A D foot ends where an open
// end sounds the flute's D4 fingering (fitted with -imp).
const Instrument kFlutes[] = {
    {"flute", 0, 59, 96, 1.0, 1.0, 1.0, 48000, 633.3, false, true, 0, 1.0},
    {"flute-c", 0, 60, 96, 1.0, 1.0, 1.0, 48000, 600.4, false, false, 0, 1.0},
    {"piccolo", 12, 62, 95, 0.5, 0.55, 0.7071, 96000, 528.0, true, false, -5, 0.7},
    {"alto", -5, 60, 88, 1.3348, 1.37, 1.1553, 36000, 600.4, false, false, 15, 1.4},
    {"bass", -12, 60, 86, 2.0, 1.84, 1.4142, 24000, 600.4, false, false, 30, 3.0},
};

}  // namespace

// The designed instruments (src/designs.cc).
extern const Instrument kDesigned[];
extern const int kNumDesigned;
// The brass (src/brass.cc).
extern const Instrument kBrass[];
extern const int kNumBrass;
extern const Instrument kLeads[];
extern const int kNumLeads;
extern const Instrument kPlucked[];
extern const int kNumPlucked;
extern const Instrument kModals[];
extern const int kNumModals;
extern const Instrument kOrchestraHit;  // (src/orchhit.cc)
extern const Instrument kStringEnsemble1, kStringEnsemble2, kStringTremolo, kStringPizzicato;  // (src/orchhit.cc, src/stringsect.cc)
extern const Instrument kChoirInstruments[];  // (src/choir.cc)
extern const Instrument kBrassSection;  // (src/brasssect.cc)
extern const int kNumChoirInstruments;

// The instruments at a common loudness: each one's trim (tools/levels.py),
// as a factor. PJ_NOLEVELS: none (the tool measures the untrimmed levels).
double drumTrimDb(const char* inst);  // (drumkit.cc: a drum at the SoundFonts' level, what its gain leaves over)
// Where the SoundFonts put the instrument's General MIDI program against their piano, over the
// common loudness (tools/gmcompare.py levels --emit): -play's channel fader (src/midifile.cc), so the
// room's sound follows it as the direct sound does. PJ_NOGMLEVELS: none.
double gmLevelDb(const char* inst) {
  static const bool off = std::getenv("PJ_NOGMLEVELS") != nullptr;
  if (off) return 0;
  for (const GmLevel& t : kGmLevels)
    if (std::strcmp(t.name, inst) == 0) return t.db;
  return 0;
}
namespace {
double levelTrimByName(const char* name) {
  const double drum = drumTrimDb(name);
  for (const LevelTrim& t : kLevelTrims)
    if (std::strcmp(t.name, name) == 0) return std::pow(10.0, (t.db + drum) / 20);
  return std::pow(10.0, drum / 20);
}
}  // namespace
// (Asked for every voice every block: the last answers kept, by the name's address, each
// thread its own.)
double levelTrim(const Instrument& inst) {
  static const bool off = std::getenv("PJ_NOLEVELS") != nullptr;
  if (off) return 1;
  struct Kept { const char* name = nullptr; double trim = 1; };
  thread_local Kept kept[64];
  Kept& k = kept[(reinterpret_cast<std::uintptr_t>(inst.name) >> 3) & 63];
  if (k.name != inst.name) k = {inst.name, levelTrimByName(inst.name)};
  return k.trim;
}

// The sample players' curve (TiMidity++ with FluidR3, its median over every
// GM program and drum: 33 log10(v / 127) dB, v^(5/3) in amplitude) less this
// instrument's own (its physics: the hammer, the breath), both from velocity
// 90; measured by tools/velcurve.py -emit. PJ_NOVELGAIN: none.
namespace {
const VelGain* velGainOf(const Instrument& inst) {
  static const bool off = std::getenv("PJ_NOVELGAIN") != nullptr;
  if (off) return nullptr;
  for (const VelGain& t : kVelGains)
    if (std::strcmp(t.name, inst.name) == 0) return &t;
  return nullptr;
}
double tableDb(const VelGain& t, double v) {
  v = std::clamp(v, double(kVelGainVels[0]), 127.0);
  int i = 0;
  while (i + 2 < kVelGainN && kVelGainVels[i + 1] < v) ++i;
  const double a = (v - kVelGainVels[i]) / double(kVelGainVels[i + 1] - kVelGainVels[i]);
  return t.db[i] + a * (t.db[i + 1] - t.db[i]);
}
double gmDb(double v) { return 33.3 * std::log10(v / 90); }
}  // namespace

double velocityGainDb(const Instrument& inst, int velocity) {
  const VelGain* t = velGainOf(inst);
  return t ? std::min(6.0, tableDb(*t, velocity)) : 0;
}

double noteLead(const Instrument& inst) {
  static const double kMaracas = knob("PJ_MARACA_LEAD", 0.12);
  static const double kHit = knob("PJ_HIT_LEAD", 0.12);  // (the orchestra hit's run-ups: src/orchhit.cc)
  if (std::strcmp(inst.name, "orchestra-hit") == 0) return kHit;
  // (the cello's keys sound 100 ms late, for its pizzicato: src/bassinst.cc)
  static const double kPizz = knob("PJ_PIZZ_MS", 100) / 1000;
  if (std::strcmp(inst.name, "cello") == 0 || std::strcmp(inst.name, "viola") == 0 || std::strcmp(inst.name, "violin") == 0) return kPizz;
  // (the string ensembles' players are those: src/stringsect.cc)
  if (std::strcmp(inst.name, "strings") == 0 || std::strcmp(inst.name, "slow-strings") == 0 ||
      std::strcmp(inst.name, "tremolo-strings") == 0)
    return kPizz;  // (pizzicato-strings: no wait, every key plucked at once)
  // (the slap basses: a pop's hook before its snap, and a look at the short notes: src/ebass.cc)
  // (the fretless: a look at whether the last key is held on over the next, a slide: src/fretless.cc)
  static const double kFretless = knob("PJ_FRETLESS_LEAD", 0.06);
  if (std::strcmp(inst.name, "fretless-bass") == 0) return kFretless;
  static const double kSlap = knob("PJ_SLAP_LEAD", 0.09);
  if (std::strcmp(inst.name, "slap-bass-1") == 0 || std::strcmp(inst.name, "slap-bass-2") == 0) return kSlap;
  // (the brass section's players start each by its instrument's speaking time ahead of the beat,
  // as section players do: the horns' long bores up to 85 ms: src/brasssect.cc)
  static const double kBrass = knob("PJ_BRASS_LEAD", 0.09);
  if (std::strcmp(inst.name, "brass-section") == 0) return kBrass;
  return std::strcmp(inst.name, "maracas") == 0 ? kMaracas : 0.0;
}

bool& noteLeadsApplied() {
  static bool applied = false;
  return applied;
}

bool velocityAsGain(const Instrument& inst) {
  return !inst.oneVoice() || std::strcmp(inst.name, "fiddle") == 0 || std::strcmp(inst.name, "contrabass") == 0 ||
         std::strcmp(inst.name, "cello") == 0 || std::strcmp(inst.name, "viola") == 0 ||
         std::strcmp(inst.name, "violin") == 0 ||
         // (the slap basses: one line, its velocity the player's (thumb or pop, ghost, how hard);
         // the frets cap how loud a slap rings, so the GM curve's level is a gain: src/ebass.cc)
         std::strcmp(inst.name, "slap-bass-1") == 0 || std::strcmp(inst.name, "slap-bass-2") == 0 ||
         std::strcmp(inst.name, "brass-section") == 0;  // (its players' gains: src/brasssect.cc)
}

// The instrument's own loudness at velocity u is the GM curve's less the
// table's gain: the softest u whose loudness reaches the GM curve at v.
int velocityRemap(const Instrument& inst, int velocity) {
  const VelGain* t = velGainOf(inst);
  if (!t) return velocity;
  const double want = gmDb(velocity);
  for (int u = 1; u < 127; ++u)
    if (gmDb(u) - tableDb(*t, u) >= want) return u;
  return 127;
}

const Instrument* findInstrument(const std::string& name) {
  for (const Instrument& i : kFlutes)
    if (name == i.name) return &i;
  for (int k = 0; k < kNumDesigned; ++k)
    if (name == kDesigned[k].name) return &kDesigned[k];
  for (int k = 0; k < kNumBrass; ++k)
    if (name == kBrass[k].name) return &kBrass[k];
  for (int k = 0; k < kNumLeads; ++k)
    if (name == kLeads[k].name) return &kLeads[k];
  for (int k = 0; k < kNumPlucked; ++k)
    if (name == kPlucked[k].name) return &kPlucked[k];
  for (int k = 0; k < kNumModals; ++k)
    if (name == kModals[k].name) return &kModals[k];
  if (name == kOrchestraHit.name) return &kOrchestraHit;
  if (name == kStringEnsemble1.name) return &kStringEnsemble1;
  if (name == kStringEnsemble2.name) return &kStringEnsemble2;
  if (name == kStringTremolo.name) return &kStringTremolo;
  if (name == kStringPizzicato.name) return &kStringPizzicato;
  for (int k = 0; k < kNumChoirInstruments; ++k)
    if (name == kChoirInstruments[k].name) return &kChoirInstruments[k];
  if (name == kBrassSection.name) return &kBrassSection;
  return nullptr;
}

const Instrument& concertFlute() { return kFlutes[0]; }

std::string instrumentNames() {
  std::string s;
  for (const Instrument& i : kFlutes) s += (s.empty() ? "" : ", ") + std::string(i.name);
  for (int k = 0; k < kNumDesigned; ++k) s += ", " + std::string(kDesigned[k].name);
  for (int k = 0; k < kNumBrass; ++k) s += ", " + std::string(kBrass[k].name);
  for (int k = 0; k < kNumLeads; ++k) s += ", " + std::string(kLeads[k].name);
  for (int k = 0; k < kNumPlucked; ++k) s += ", " + std::string(kPlucked[k].name);
  for (int k = 0; k < kNumModals; ++k) s += ", " + std::string(kModals[k].name);
  s += ", " + std::string(kOrchestraHit.name);
  s += ", " + std::string(kStringEnsemble1.name) + ", " + std::string(kStringEnsemble2.name) + ", " + std::string(kStringTremolo.name) +
       ", " + std::string(kStringPizzicato.name);
  for (int k = 0; k < kNumChoirInstruments; ++k) s += ", " + std::string(kChoirInstruments[k].name);
  s += ", " + std::string(kBrassSection.name);
  return s;
}

const NoteInfo* noteInfo(int midi) {
  return midi >= 59 && midi <= 96 ? &kNotes[midi - 59] : nullptr;
}

const NoteInfo* noteInfo(const Instrument& inst, int midi) {
  const int w = midi - inst.transpose;
  if (w < inst.lo || w > inst.hi) return nullptr;
  return inst.design ? &inst.design->notes[w - inst.lo] : noteInfo(w);
}

const NoteInfo* noteInfo(const Instrument& inst, int midi, int k) {
  const NoteInfo* n = noteInfo(inst, midi);
  if (!n || k == 0) return n;
  if (k != 1 || !inst.design || !inst.design->alt) return nullptr;
  const NoteInfo* a = &inst.design->alt[midi - inst.transpose - inst.lo];
  return a->holes ? a : nullptr;
}

double noteFrequency(int midi, double a4) { return a4 * std::pow(2.0, (midi - 69) / 12.0); }

}  // namespace pajupilli
