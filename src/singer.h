// A singer for the choir pad (pad-choir): a vocal tract as puhuja models it
// (~/puhuja: TRACT_MODEL.md, tract.cc, line.cc), driven by a glottis at the
// note's pitch.
//
//   - The tract: one acoustic transmission line from the glottis to the lips,
//     44 sections, simulated in the time domain (1-D FDTD, staggered leapfrog:
//     pressure at each section's middle, volume flow at its boundaries), 3
//     substeps per 48 kHz sample (Courant 0.6-0.75).
//       momentum:   rho / A du/dt = -dp/dx - (loss) u
//       continuity: A / (rho c^2) dp/dt = -du/dx
//   - Its shape: puhuja's parametric area function in log area (a larynx
//     tube, the pharynx blending into the mouth at the velum, a cos^2 tongue
//     hump, the lip tube), each section the mean of 8 points. The vowels
//     a, e, i, o, u are puhuja's fitted rows (kLineVowels, linefit). Between
//     vowels the sections glide in log area.
//   - The lips radiate through Flanagan's piston load (R parallel L); the
//     sound is the lip flow's time derivative.
//   - The glottis: the folds' opening a(t) follows a smooth pulse at f0 (the
//     open quotient falling and the closing growing faster as the voice gets
//     louder), with a small posterior chink (breath). The flow obeys puhuja's
//     law, Lg du/dt = dp - rho u|u| / (2 Cd^2 a^2), solved implicitly each
//     substep against the tract's first section: the formants load the source.
//   - Turbulence at the glottis: noise ~ the flow's dynamic pressure, above a
//     critical Reynolds number, as a pressure across the glottis.
//   - A singer's own: tract length (a bass's 18 cm .. a soprano's 14.5),
//     pitch scatter, jitter and shimmer, a vibrato at its own rate.
#pragma once

#include <cstdint>

namespace pajupilli {

class Singer {
 public:
  static constexpr int kN = 44, kSub = 3;
  void init(double fs, unsigned seed, int index);
  void reset();
  // Per control block: the pitch (Hz, without the singer's own vibrato), the
  // vowel (0 u, 1 o, 2 a, 3 e, 4 i; fractions between), the lungs' pressure
  // (Pa), breathiness 0..1, the vibrato (depth in semitones, rate Hz), the
  // note (MIDI: the tract's length follows the voice part), the block (s).
  void control(double f0, double vowel, double lung, double breath, double vibDepth, double vibRate, double note,
               double dt);
  // One output sample: the lips' dU/dt (m^3/s^2).
  double sample();
  // A whisper (0..1): the folds held apart, not vibrating; the air through
  // the open glottis turbulent (breath noise, GM 122).
  void setWhisper(double w) { whisper_ = w; }
  // A constriction (consonants, as puhuja's line.cc makes them): at place
  // (a share of the tract from the glottis; 1: the lips), its area (cm^2; 0:
  // none). The tongue tip or the lips close it in ~3 ms and open it in ~6
  // (a release); its jet hisses past a critical Reynolds number, only while
  // it is a slit (puhuja: full below 0.12 cm^2, none above 0.2156).
  void setConstriction(double place, double areaCm2) { cPlace_ = place, cTarget_ = areaCm2; }
  // (Tests: a pressure kick (Pa) into the section above the glottis.)
  void kick(double dp) { p_[0] += float(dp); }

  // --- A sung voice (the choir, src/choir.h): this singer's own, fixed for its seat. ---
  // Set, the singer sings: the vowel axis of control() is then the sung rows
  // (src/singer_vowels_data.h: 0 u, 1 ʊ, 2 ɔ, 3 ɑ; on toward the bright
  // vowels with the spoken rows 4 e, 5 i; fractions between), the
  // tract's length is the voice part's (not the note's), the lungs' range is a
  // singer's (loudness from ~250 Pa (pp) to ~3.5 kPa (ff), log), and its pitch
  // offset, jitter and shimmer are these. Not set (pad-choir, the effects):
  // everything as before.
  struct Voicing {
    double lengthCm = 17.5;         // the tract's length
    bool female = false;            // the female rows (fitted at 15 cm: F2/F1 differ, not only the scale)
    double hump = 0;                // the tongue hump's place moved (share of the length)
    double humpArea = 1, lip = 1, mouth = 1, pharynx = 1;  // areas x (the singer's own vowel)
    double cents = 0;               // its pitch off the note (c)
    double jitter = 1, shimmer = 1; // x the defaults (+-0.3 %, +-3 % per period)
  };
  void setVoicing(const Voicing& v);
  bool sung() const { return sung_; }
  // The jaw (sung): 0 the row's own mouth; 1 opened (the mouth, the lips and
  // the tongue's gap wider, the pharynx narrower, the lip tube shorter, the
  // larynx up: the tract 10 % shorter; R1 of [ɑ] up ~20 %, of [u] ~50 %); up
  // to 1.6; below 0 a little closed. Moves with the tongue (~50 ms).
  void setJaw(double jaw) { jaw_ = jaw; }
  // The pitch's share of the lungs' pressure (sung; x): a high note needs more pressure to sound
  // at all, so the voice's effort (how short the open phase, how quick the closure) is read from
  // the pressure divided by this.
  void setEffort(double pitchMul) { effort_ = pitchMul > 0 ? 1 / pitchMul : 1; }
  // The tract's resonances as shaped (sung; Hz): R1 (k = 1) or R2 (k = 2) of
  // the vowel at this jaw, from the line's chain matrix (the glottis closed,
  // the lips' piston load); and the jaw that puts R1 at f (within -0.3 .. 1).
  double resonance(int k, double vowel, double jaw) const;
  double jawFor(double vowel, double f) const;

 private:
  bool sung_ = false;
  double effort_ = 1, gLp_ = 0, gK_ = 1;  // 1 / the pitch's share; the folds' opening smoothed, its step
  double jaw_ = 0, jitMul_ = 1, shimMul_ = 1, lenSung_ = 0.175;
  static constexpr int kSungRows = 6;  // u ʊ ɔ ɑ (sung), e i (spoken)
  float sungLn_[kSungRows][kN] = {}, sungOpen_[kSungRows][kN] = {};  // the sung rows (log m^2): the jaw at 0 and at 1
  mutable float r1Tab_[kSungRows][7] = {};  // R1 (Hz) per row at the jaws of jawFor's grid (0: not yet)
  void sungShape(double vowel, double jaw, float* ln) const;
  double sungLength(double jaw) const;
  double fs_ = 48000, dts_ = 0;
  float A_[kN] = {}, lnA_[kN] = {}, lnT_[kN] = {};  // areas (m^2): now, in log, the target in log
  float p_[kN] = {}, u_[kN + 1] = {};
  float ku_[kN + 1] = {}, kp_[kN] = {}, damp_[kN + 1] = {};
  double dx_ = 0.004, len_ = 0.175, lenTarget_ = 0.175;
  double iL_ = 0, radR_ = 0, radL_ = 1, uLipPrev_ = 0;
  double phase_ = 0, f0_ = 220, lung_ = 0, breath_ = 0.2, aMax_ = 0, oq_ = 0.6, cq_ = 0.35;
  double jit_ = 0, shim_ = 1, scatter_ = 0, drift_ = 0, lenMul_ = 1, vibPh_ = 0, vibRateMul_ = 1;
  double nLp_ = 0, nHp_ = 0;
  double vowel_ = 2, whisper_ = 0, jawSet_ = 0;
  double cPlace_ = 1, cTarget_ = 0, cArea_ = 0;  // the constriction: where, the area wanted, now (cm^2; 0 none)
  int cBound_ = -1;                              // the boundary its jet leaves through (-1: none)
  double cFric_ = 0, fLp_ = 0, fHp_ = 0;         // its noise's drive this block; its band
  bool shaped_ = false;
  uint32_t rng_ = 1;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
  void shape(double vowel, double len);  // the target areas
  void coefs();                          // per block: the line's coefficients from the areas
};

}  // namespace pajupilli
