// A Boehm concert flute (C foot) on the bore waveguide: geometry, tone holes
// with moving pads, the embouchure window driven by the air jet, and the
// player's mouth and tongue upstream of the lips.
//
// Geometry: Pearl PF-661 measured by Dickens (UNSW), see research/keywork.md.
// Jet: Verge / Fabre / de la Cuadra jet-drive model, see research/jet_and_losses.md.
// Player: lungs as a pressure set-point, the mouth one compliance, the tongue a
// valve, the lip slit a Bernoulli orifice with jet inertia
// (research/player_and_controller.md).
#pragma once

#include <cstdint>
#include <string>

#include "bore.h"

namespace pajupilli {

constexpr int kHoles = 16;

// A member of the flute family: the concert flute resized. Along the bore
// everything scales with the pitch (`scale`); bore, holes and pads with the
// real instrument's bore (`bore`); the embouchure hole, the lips and the jet
// with about sqrt(scale) (`emb`). The model runs at 48 kHz / scale so the
// air column keeps its 128 cells; Voice resamples to 48 kHz.
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
  double head;       // head shortened by (mm, own size): keeps the scale with
                     // an embouchure hole relatively smaller (alto, bass) or
                     // bigger (piccolo) than the flute's
  double gain;       // output level, as if miked: the bass flute is ~15 dB
                     // quieter than the flute, the piccolo louder
};
// "flute", "piccolo", "alto", "bass"; nullptr if unknown.
const Instrument* findInstrument(const std::string& name);
const Instrument& concertFlute();

// What the player does with the lips and breath; set any time, smoothed inside.
struct Embouchure {
  double lung = 0;          // lung (blowing) pressure set-point (Pa)
  double tongue = 0;        // tongue valve: 0 closed .. 1 open
  double lipH = 0.8e-3;     // lip opening height = jet thickness h (m)
  double lipWidth = 9e-3;   // lip opening width = jet width H (m)
  double jetLen = 8e-3;     // lip to edge W (m)
  double uncovered = 6e-3;  // embouchure hole width left open by the lower lip (m)
  double y0 = 0.1e-3;       // jet offset from the labium edge (m)
  double noise = 0.01;      // turbulence, in units of the jet velocity
  double air = 0.35;        // 0 focused .. 1 airy: unfocused jet, edge hiss
  double glottis = 1.5e-4;  // glottal opening (m^2): 1.5 cm^2 open .. ~3 mm^2 a slit
};

class Flute {
 public:
  explicit Flute(const Instrument& inst = concertFlute());
  const Instrument& instrument() const { return inst_; }

  // Fingering as 16 chars, hole 1 (D# trill) .. hole 16 (C#4), 'o' open, 'x' closed.
  // Fingering; fingerSpeed scales how fast the fingers move the keys (key velocity:
  // 1 = 10 ms, 2 = 5 ms). Springs always take their own time.
  void setHoles(const char* pattern, double fingerSpeed = 1);
  void setEmbouchure(const Embouchure& e) { target_ = e; }
  const Embouchure& embouchure() const { return target_; }

  // Render n mono samples (Pa at 1 m, roughly).
  void process(float* out, int n);

  // Tests: a pressure impulse (Pa) in the window branch on the next sample.
  void testDrive(double dp) { testDp_ = dp; }
  // Clear all air motion (keys and settings stay); done automatically on NaN.
  void resetAir();
  int resets() const { return resets_; }
  // Introspection for tests.
  double fs() const { return fs_; }
  const Bore& bore() const { return bore_; }
  double mouthPressure() const { return pm_; }
  double jetVelocity() const { return uj_; }
  double windowFlow() const { return eq_; }
  double jetSplit() const { return ef_; }
  double windowPressure() const { return bore_.s.p()[ec_]; }

 private:
  struct Hole {
    double x;         // centre from the cork face (m)
    double b;         // radius (m)
    double padH;      // key opening (m)
    double padR;      // key cup radius (m)
    bool perforated;  // French (open-hole) key
    int cell;
    double tSeries;   // chimney + inner + matching length (m), incl. grid offset
    double tRad;      // radiation (outer) end correction (m)
    double open = 0, target = 0;  // pad openness 0..1 (time along the stroke)
    bool closing = false;         // closing stroke: accelerates into the seat
    double stroke = 0.010;        // duration of the current stroke (s)
    double wait = 0;              // the stroke starts after this delay (s)
    float thRe = 0, thIm = 0;     // landing thud: the pad on its felt (flow, m^3/s)
    bool held = false;            // pressed into its felt by a closing stroke
    double padIn = 0;             // air the pad pushes into the bore (m^3/s)
    double a = 0;                 // bore radius at the hole (m)
    bool present = true;          // a piccolo has no C# and D holes (D foot)
  };
  void updateHoles(double dtBlock);
  double keyDelay();  // a random 0..PJ_KEYSPREAD start delay (s)
  Branch holeBranch(const Hole& h) const;
  void embouchureGeometry();
  void controlStep();
  double horn_r() const;
  float sample();
  double boreDiameter(double x) const;  // at x (m) from the cork face

  const Instrument& inst_;
  double fs_;
  // The instrument's dimensions (m).
  double cork_, head_, foot_, embLong_, embRin_, embChimney_, holeT_, embA_;
  double noiseA_, noiseHpA_, glotA_, noiseNorm_;  // noise filters at this rate
  Bore bore_;
  Hole hole_[kHoles];
  int ec_;  // embouchure cell
  // Hole cells, for the radiated sound.
  int nRad_ = 0;
  int radCell_[kHoles + 2];
  int radDelay_[kHoles + 2];
  float radGain_[kHoles + 2];
  float radPrev_[kHoles + 2] = {}, radPrev2_[kHoles + 2] = {};
  static constexpr int kOutRing = 1024;
  float outRing_[kOutRing] = {};
  int outPos_ = 0;

  Embouchure target_, cur_;
  // Embouchure window branch (scalar): flow out of the bore Q, radiation flow l.
  double eq_ = 0, el_ = 0, ef_ = 0;
  double eg_ = 0, ers_ = 0, err_ = 0, ehl_ = 0, esw_ = 1e-4, ecnl_ = 0;
  // Mouth and jet.
  double pm_ = 0, uj_ = 0, tongue_ = 0;
  static constexpr int kHist = 2048;
  float vHist_[kHist] = {};
  int hPos_ = 0;
  uint32_t rng_ = 22222;
  double noiseLp_ = 0, noiseHp_ = 0, glotLp_ = 0, kGlotNoise_;
  double whiteNoise() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return int32_t(rng_) * (1.0 / 2147483648.0) * 1.732 * noiseNorm_;
  }
  float dcX_ = 0, dcY_ = 0, dcPole_ = 0.99935f;
  int ctl_ = 0;
  double testDp_ = 0;
  int resets_ = 0;
  // Knobs.
  int kJetModel_, kRadOnly_;
  double kRel_, kSpread_, kJetLp_, kEtaMax_, kEmbR_, xi_ = 0, jl1_ = 0, jl2_ = 0;
  double kXiHp_, xiLow_ = 0, kVortex_ = 1;
  double kTurbIn_, kEdge_, edgeLow_ = 0, edgeBand_ = 0;
  double fingerSpeed_ = 1, kPadPump_, kKeyClick_, kKeySpread_, kThud_, kRelease_;
  float thudCos_ = 1, thudSin_ = 0, thudDecay_ = 0;
  int thudLeft_ = 0;  // samples until every thud has died out
  int nPumping_ = 0;
  uint32_t keyRng_ = 777;
  // Key impact: a few damped modes of the key and the tube wall.
  static constexpr int kTickModes = 3;
  double tickRe_[kTickModes] = {}, tickIm_[kTickModes] = {}, tickCos_[kTickModes] = {},
         tickSin_[kTickModes] = {}, tickDecay_[kTickModes] = {};
  double kJetSign_, kEtaSign_, kEmbLen_, kAlpha_, kConv_, kNoiseGain_, kOut_;
};

}  // namespace pajupilli
