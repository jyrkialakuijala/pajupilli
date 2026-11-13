#include "instrument.h"

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

const Instrument* findInstrument(const std::string& name) {
  for (const Instrument& i : kFlutes)
    if (name == i.name) return &i;
  for (int k = 0; k < kNumDesigned; ++k)
    if (name == kDesigned[k].name) return &kDesigned[k];
  for (int k = 0; k < kNumBrass; ++k)
    if (name == kBrass[k].name) return &kBrass[k];
  for (int k = 0; k < kNumLeads; ++k)
    if (name == kLeads[k].name) return &kLeads[k];
  return nullptr;
}

const Instrument& concertFlute() { return kFlutes[0]; }

std::string instrumentNames() {
  std::string s;
  for (const Instrument& i : kFlutes) s += (s.empty() ? "" : ", ") + std::string(i.name);
  for (int k = 0; k < kNumDesigned; ++k) s += ", " + std::string(kDesigned[k].name);
  for (int k = 0; k < kNumBrass; ++k) s += ", " + std::string(kBrass[k].name);
  for (int k = 0; k < kNumLeads; ++k) s += ", " + std::string(kLeads[k].name);
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
