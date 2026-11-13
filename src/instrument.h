// The instruments: the flute family (resized concert flutes, src/flute.cc) and
// instruments described by a Design (src/designs.cc): a bore profile, tone holes,
// fingerings and an excitation (a jet from a duct or the lips, or a reed).
#pragma once

#include <string>

namespace pajupilli {

// How the air column is set into motion.
enum class Kind {
  kFlute,     // transverse flute: the lips form the jet across an embouchure hole
  kDuct,      // a duct (windway) forms the jet: recorder, ocarina
  kEndBlown,  // the lips blow across the rim of the open top: shakuhachi, pan flute
  kReed,      // a single reed on a mouthpiece: clarinet, saxophones
  kDoubleReed, // a double reed: oboe, bassoon, duduk, shawm
  kLip,        // the lips buzzing in a cup mouthpiece: horn (src/brass.cc)
  kCircuit     // an analog circuit: the synth leads (src/lead.cc)
};

struct NoteInfo {
  const char* holes;  // hole 1 .. n, 'o' open, 'x' closed, 'h' half-holed; left out: open
  int harmonic;       // which resonance of the fingering sounds (1, 2, 3, ...)
};

// A point of the bore profile: x from the top of the air column (the cork face,
// the reed tip or the top of a pipe), diameter, both in mm. Linear in between.
struct BorePoint { double x, d; };

enum HoleFlags {
  kFingerHole = 1,      // covered by a finger (no pad); 'h' leaves a quarter of it open
  kNormallyClosed = 2,  // a key that a spring holds closed (opened by a finger)
  kPerforated = 4,      // a French (open-hole) key
};

// A tone hole: x of its centre (mm from the top), diameter, chimney height
// (wall), the pad's opening and its cup diameter (mm; a finger hole: how far
// the finger lifts, and the finger's width).
struct HoleDef { double x, d, t, padH, cup; int flags; };

// A pipe of a pan flute: its sounding note, length (mm, top to the stopper) and bore.
// One pipe per note (a pan flute), or a bottle: a neck (length, diameter; 0:
// none) over the body; len from the top to the closed end (mm).
struct PipeDef { int midi; double len, d, neck = 0, neckD = 0; };

// A reed: Bernoulli flow through the slit, the reed a damped mass-spring.
struct ReedParams {
  double width;   // slit width (m)
  double h0;      // rest opening with the lips at mezzo-forte (m)
  double k;       // stiffness per area (Pa/m): closing pressure = k h0
  double fr, qr;  // resonance (Hz) and damping (1 / Q) with the lips on it
  double sr;      // area the reed sweeps (m^2): flow from its motion
  double psi;     // double reed: head loss in the reed's duct, in units of its
  double sd;      //   area sd (m^2) (0: a single reed)
  double press;   // mezzo-forte mouth pressure at the lowest note (Pa)
  double pressHi; // ... at the top of the range (Pa)
  double vib;     // vibrato: from the breath (oboe, bassoon), or the jaw (sax): 0..1
};

// A duct or the lips at an open end: the jet and the window it crosses.
struct JetParams {
  double h;        // jet thickness at the exit (m): windway height (duct)
  double width;    // jet width (m)
  double w;        // exit to edge (m): window length (duct), nominal (lips)
  double window;   // window (or uncovered rim) width along the edge (m)
  double chimney;  // window's depth (m)
  double channel;  // windway length (jet inertia, m)
  double y0;       // edge offset (m)
  double press;    // mezzo-forte pressure at the lowest note (Pa) ...
  double pressHi;  // ... and at the top (Pa)
};

// A valve (brass): where it sits on the bore (mm from the lips), the length
// its loop adds when pressed (mm), the loop's bore (mm), and how many
// semitones it lowers (for the fingering tables).
struct ValveDef { double x, len, d; int semis; };

// The lips in a cup mouthpiece (Elliott & Bowsher 1982; research/horns.md):
// an outward-striking valve, h'' + (wl / Q) h' + wl^2 (h - H) = (pm - p) / mu,
// the player tuning its resonance fl to each note (a little below the
// partial: the note sounds above it); H = hk / fl; Bernoulli flow through the
// opening w h, plus the swept flow sr dh/dt.
struct LipParams {
  double cup;      // cup volume (m^3), lumped at the lips
  double mu;       // lip mass per area (kg/m^2)
  double q;        // the lip resonance's Q
  double width;    // lip opening width (m)
  double hk;       // rest opening H = hk / fl (m Hz)
  double sr;       // area the lips sweep (m^2)
  double k;        // mezzo-forte mouth pressure per Hz of the note (Pa/Hz) ...
  double pressLo;  // ... but no less than this (Pa) ...
  double pressHi;  // ... nor more (Pa)
  double fl;       // lip resonance / the note's frequency, before calibration
  // The second, inward-striking mode (Cullen et al. 2000, Newton et al. 2008:
  // human lips show both): its resonance / fl, its Q, its share of the opening.
  // It pulls the pitch down towards the bore's resonance (one outward mode
  // alone plays 50..200 cents sharp of it) and lets the upper register speak.
  double r2, q2, a2;
  const ValveDef* valves;
  int nValves;
};

struct Design {
  const BorePoint* bore;
  int nBore;
  const HoleDef* holes;
  int nHoles;
  const NoteInfo* notes;  // written lo..hi
  double excite;          // the window centre or the reed, mm from the top
  double end;             // the open (or stopped) end, mm from the top
  bool closedEnd;         // stopped pipe / vessel
  const PipeDef* pipes;   // a pan flute: one pipe per note (holes unused)
  int nPipes;
  ReedParams reed;
  JetParams jet;
  const LipParams* lip = nullptr;  // brass: the lips and valves (the notes'
                                   // patterns are valves, 'x' pressed; harmonic:
                                   // the partial)
  const NoteInfo* alt = nullptr;   // an alternate fingering per note (holes
                                   // nullptr: none); the calibration chooses
};

// A member of the flute family: the concert flute resized. Along the bore
// everything scales with the pitch (`scale`); bore, holes and pads with the
// real instrument's bore (`bore`); the embouchure hole, the lips and the jet
// with about sqrt(scale) (`emb`). The model runs at 48 kHz / scale so the
// air column keeps its 96 cells; Voice resamples to 48 kHz.
// Other instruments: `design` gives the geometry; scale and emb size the player.
struct Instrument {
  const char* name;
  int transpose;     // sounding minus written pitch (semitones)
  int lo, hi;        // written range (MIDI); the fingerings are the flute's
  double scale;      // lengths along the bore, vs the concert flute
  double bore;       // bore, tone hole and pad diameters
  double emb;        // embouchure hole, lip opening, jet
  double fs;         // the model's sample rate (Hz)
  double foot;       // embouchure centre to the open end, concert-flute mm
  bool dFoot;        // no C and C# keys (holes 15, 16)
  bool bFoot;        // a B foot: a C hole (17) and the tube on to B3
  double head;       // head shortened by (mm, own size): keeps the scale with
                     // an embouchure hole relatively smaller (alto, bass) or
                     // bigger (piccolo) than the flute's
  double gain;       // output level, as if miked: the bass flute is ~15 dB
                     // quieter than the flute, the piccolo louder
  Kind kind = Kind::kFlute;
  const Design* design = nullptr;
  const char* family = "flute";  // the ensemble's label ("flute 2", "sax 1")
  bool reed() const { return kind == Kind::kReed || kind == Kind::kDoubleReed; }
  bool lip() const { return kind == Kind::kLip; }
  bool circuit() const { return kind == Kind::kCircuit; }
};

// By name; nullptr if unknown.
const Instrument* findInstrument(const std::string& name);
const Instrument& concertFlute();
// All names, comma-separated (for messages).
std::string instrumentNames();

// Written MIDI 59 (B3, B foot) .. 96 (C7) on the Boehm flute; nullptr outside.
const NoteInfo* noteInfo(int midi);
// A sounding note on an instrument: its fingering, or nullptr outside its range.
const NoteInfo* noteInfo(const Instrument& inst, int midi);
// Fingering k of a sounding note (0: the standard one, 1: the alternate);
// nullptr if there is none.
const NoteInfo* noteInfo(const Instrument& inst, int midi, int k);
double noteFrequency(int midi, double a4 = 440.0);

}  // namespace pajupilli
