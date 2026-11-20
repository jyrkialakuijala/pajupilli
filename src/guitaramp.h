// The guitar amplifier (research/electric_guitars.md 11): a module of its own, after the jack. It
// does not know what is plugged in; it is told the source's network (a pickup's coil and the
// guitar's pots) and is given the source's EMF, sample by sample.
//
//   - The input: the pickup's coil (L in series with its winding's R, its own C across) with the
//     cable's C and the load (the guitar's pots in parallel with the amp's 1 MOhm), solved
//     together: a resonance at 2-6 kHz, its height set by the load (6.3). That is why it is the
//     amp that solves it: a fuzz's low input would flatten it, a tube's 1 MOhm lets it ring.
//     The noise a high-gain amp makes audible comes in here: the pickup's winding and the grid
//     stopper hiss (a few microvolts), and the coils pick up the mains' hum, a humbucker little
//     of it (research/distortion_guitar.md 4).
//   - A compressor pedal, if the preset has one (an OTA compressor, the MXR Dyna Comp's kind):
//     above a threshold the level grows a quarter as fast, quick to clamp (3 ms) and slow to let
//     go (0.2 s): a muted note's thud and a funk strum's attack brought closer together.
//   - A Tube Screamer in front, if the preset has one (distortion_guitar.md 3.1, 10.3): the clean
//     signal passes, and to it is added its part above 720 Hz, amplified (12x at drive 0) and
//     clipped by two diodes: into a high-gain amp it lifts the pick's attack and the upper
//     partials into the clipping stages, not the low E's bass (a "tighter" chug).
//   - The preamp: up to five triode stages (11.2). Each nearly straight until its grid conducts
//     (+1.4 V from its bias), cut off more softly below (-2.8 V), its curve a little steeper on
//     the way up (an even harmonic); a stage biased cold (the Marshall's second, the Soldano's
//     third: its bias nearer cut-off) clips its lower half early, unevenly. Its coupling
//     capacitor (a high-pass), the Miller capacitance (a low-pass), a partly bypassed cathode (a
//     low shelf: the bass amplified a few dB less than the treble), a divider into the next
//     stage. Driven past its grid's conduction, the grid draws current and charges the coupling
//     capacitor: the bias slides cold for tens of milliseconds after a hard attack (the "bloom"
//     and, overdone, the "blocking" of a cranked amp: PY09). A cathode follower after the last
//     stage (DC coupled: Marshall, Soldano) squashes the stage's cut-off swing when its own grid
//     conducts. A high-gain preset cascades four: what is kept out of them (the bass, by small
//     coupling capacitors and partial bypasses) is what makes it tight (distortion_guitar.md 2).
//   - The tone stack (the passive three-knob network of Fender and Marshall: exact, from its
//     parts, closed form: Yeh and Smith 2006, checked against a nodal solve): after the first
//     stage (Fender), after the second (Marshall: what is clipped is then shaped), or after all
//     of them (Soldano). A volume pot between the first two stages (with the Marshall's bright
//     capacitor across it: the lower the gain, the more of the treble passes it), a master after.
//   - A noise gate (high gain: distortion_guitar.md 4), as the ISP Decimator does it: it listens
//     to the guitar itself, before the amp, and opens and closes the preamp's output: open on the
//     pick's attack, closing 6 dB lower after a hold (no chatter), fading out over 80 ms. Its
//     release adapts as the Decimator's does: a note that falls quickly far under its attack (a
//     palm-muted chug, a string let go) closes it at once, and only a new attack opens it again;
//     a note that decays as a string does keeps it open down to the threshold.
//   - The power stage: push-pull (its halves alike: odd harmonics), its supply sagging under a
//     loud passage (0.1 s); the EL34's harder knee or the 6L6's softer one. Presence and
//     resonance: the negative feedback taken off above 3 kHz and around the closed box's 100 Hz.
//   - Distortion makes harmonics above the audio band, which would fold back as an inharmonic
//     haze: the stages run at 4 x 48 kHz when the preset drives them (11.7: 4x satisfies most
//     listeners), through an elliptic low-pass on the way up and down (tools/oversample.py); the
//     high-gain preset runs them at 2x and averages each stage's curve over the sample
//     (first-order antiderivative antialiasing, PZL16): a held A5's aliases -79 dB, where 2x
//     alone leaves -48 and 4x alone would cost a third more.
//   - The speaker in its cabinet, as a filter: a 2x12 open back (Fender), a closed 4x12 of
//     Greenbacks (Marshall: the low thump of the closed box, the forward upper mids), or a closed
//     4x12 of Vintage 30s with an SM57 just off the dust cap (Celestion's plot: the presence pair
//     at 2.3 and 3.2 kHz, the fall of ~25 dB an octave above 4 kHz).
// Presets: "twin" (a Fender Twin Reverb, clean: GM 28); "twin-comp" (the same with the compressor
// in front: GM 29); "jcm800" (a Marshall JCM800 2203 at crunch, a compressor in front: GM 30);
// "slo" (a Soldano SLO-100's overdrive channel into a V30 4x12, gated: GM 31).
//
// The same module for the electric bass (research/bass_amps.md 9-10), where the record is a DI
// blended with a miked amp and compressed after:
//   - The input's resistance a preset's own: an active DI and the amp on a splitter, 0.9 MOhm.
//   - Bass stages: 6SL7s and 12AU7s with their cathodes unbypassed swing several times as far as
//     a 12AX7 before they clip (a stage's swing); coupling corners of a few Hz and the output's
//     at 8 Hz: the E1's 41 Hz kept whole.
//   - The James (passive Baxandall) tone stack of Ampeg's B-15 and SVT, exact from its parts: an
//     admittance matrix solved at eight points of s, its two fourth-order polynomials read off
//     (a DFT: the solve is exact for degree < 8), their roots taken over by the bilinear transform
//     one by one (two biquads). Flat at noon (no built-in scoop, unlike the Fender stack), 22-23
//     dB lower. After the stages fixed filters (the SVT's mid at its switch's frequencies, Ultra Lo/Hi).
//   - Bass cabinets with their microphones (4.3): an 8x10 with a D112, a 1x15 with an RE20, a 4x10
//     with a horn; or none.
//   - The DI: the pickup's own signal, a filter or two, a SansAmp-like drive if the preset has
//     one, blended with the amp's (level-matched at 200 Hz). Its polarity and its phase below a
//     few hundred Hz turned to the mic's (an allpass: what an engineer's phase-alignment box does;
//     the cabinet's high-pass would otherwise cancel the E1's fundamental against the DI's); the
//     mic's distance as a delay.
//   - After the blend a studio compressor: an RMS detector (two 10 ms poles: it does not follow a
//     41 Hz wave), a 6 dB soft knee; an LA-2A's opto cell (half its gain reduction quick, half a
//     slow charge that lets go over seconds) or an 1176's FET (attack under a millisecond).
// Bass presets: "b15-di" (GM 34), "svt" (GM 35), "slap-di" (GM 37), "slap-squash" (GM 38).
#pragma once

#include <complex>
#include <cstdint>
#include <vector>

namespace pajupilli {

class GuitarAmp {
 public:
  // What is plugged in: its coil, and the guitar's own load across it (its pots: Ohm).
  // Rt, Ct: a tone control across it (the tone pot as a resistor in series with its capacitor:
  // research/jazz_guitar.md 4.3; Ct 0: none, the pots' resistance alone).
  struct Source { double L = 2.2, R = 6.5e3, C = 110e-12, pots = 125e3, Rt = 0, Ct = 0; };
  // A triode stage: its gain, its bias from the usual (V; negative: colder), its coupling
  // capacitor's corner and its Miller roll-off (Hz; 0: none). Optionally: its cathode partly
  // bypassed (the bass shelfDb under the treble below about shelfHz: a first-order shelf, its
  // corners shelfHz -+ half the shelf), a divider's shelf in front of it (the same form: the
  // Soldano's 470 k with 2000 pF across it, before the second stage), a divider after it (into
  // the next stage), a cathode follower after it (its grid conducting: the stage's cut-off swing
  // squashed above this many volts at the stage's grid; 0: none); how fast its grid's current
  // charges the coupling capacitor (a grid stopper or a large capacitor: slower).
  struct Stage {
    double gain, bias, hp, lp;
    double shelfHz = 0, shelfDb = 0, inHz = 0, inDb = 0;
    double divider = 1, follower = 0;
    double grid = 1;  // how quickly grid current charges its coupling capacitor (x the usual 0.9 ms)
    double swing = 1;  // its grid's range against a 12AX7's (6SL7, 12AU7 with an unbypassed cathode: 3-5)
  };
  // A fixed filter (RBJ): 0 low-pass, 1 high-pass, 2 peak, 3 high shelf, 4 low shelf.
  struct Eq { int type; double f, q, db; };
  // The James stack's parts (research/bass_amps.md 3.1): the source's resistance, R1 above the bass
  // pot (Rb) with C1 across its upper half and C2 across its lower, R2 under it; C3, the treble
  // pot (Rt), C4 to ground; R3 from the bass wiper to the treble wiper (the output); Rw from the
  // bass wiper to ground (0: none); the load.
  struct James { double rs, r1, rb, c1, c2, r2, c3, rt, c4, r3, rw, rl; };
  static constexpr int kMaxStages = 5;
  struct Preset {
    const char* name;
    Stage stage[kMaxStages];
    int stages;                           // how many
    int stackAfter;                       // the tone stack after this stage (0: before the first)
    double c1, c2, c3, r1, r2, r3, r4;    // the tone stack's parts
    double treble, mid, bass;             // its knobs (0 .. 1)
    double volume, bright;                // the pot after the first stage (its share), the capacitor across it (F; 0: none)
    double master;                        // before the power stage
    double clip;                          // the power stage's limit, at its input (V)
    int cabinet;                          // 0: 2x12 open back, 1: closed 4x12 Greenbacks, 2: closed 4x12 V30s, SM57
    int oversample;                       // 1, 2 or 4
    double compRatio = 0, compAt = 0.05;  // the compressor pedal (0: none) and its threshold (V)
    double compRef = 0.2;                 // a peak it leaves where it was (V: its make-up)
    // High gain (research/distortion_guitar.md 10). All off by default.
    bool adaa = false;                    // the stages' curves antialiased (first order)
    double knee = 3;                      // the power stage's knee: 3 (EL34), 2 (6L6: softer)
    double resonance = 0, presence = 0;   // dB: the power stage's bump at 100 Hz, its shelf from 3 kHz
    double hiss = 0, hum = 0, humHz = 50; // the noise at the input: V rms; the hum's peak (V); its mains
    double gateAt = 0, gateRelease = 0.08, gateHold = 0.015;  // the gate's threshold (V at the input; 0: none), its fade, its hold (s)
    double gateDrop = 0;                  // dB: it also closes on a quick fall this far under the last peak (0: not)
    double tsLevel = 0, tsDrive = 0, tsTone = 0.55;  // the Tube Screamer (level 0: none)
    // Bass (research/bass_amps.md 9). As the guitars have it by default.
    double inputR = 1e6;                  // the input's resistance (Ohm)
    double hpOut = 30;                    // the output's high-pass (Hz)
    double sagDepth = 0.15;               // how far the supply gives way under a loud passage (of the swing)
    int stackType = 0;                    // 0: Fender/Marshall (Yeh-Smith); 1: James
    James james{};
    Eq voice[6]{};                        // fixed filters after the last stage (a mid, Ultra Lo/Hi, a solid-state head's EQ)
    int voices = 0;
    double blend = 0;                     // the DI's share (0: the amp alone, as the guitars; 1: the DI alone)
    Eq di[4]{};                           // the DI's own filters
    int dis = 0;
    int diPolarity = 0;                   // the DI against the mic: +1, -1, 0: the one that keeps more low end
    bool diAlign = false;                 // the DI's phase turned to the mic's at the low end (an allpass)
    double micDelay = 0;                  // the mic's distance (samples at the base rate; 0: aligned)
    double driveAt = 0, driveMix = 0;     // a SansAmp-like drive on the DI: its knee (V at the pickup; 0: none), its share
    int studio = 0;                       // the studio compressor after the blend: 0 none, 1 opto (LA-2A), 2 FET (1176)
    double stRatio = 3, stAt = 0.1;       // its ratio, its threshold (V rms at the pickup, as the DI has it)
    double stAttack = 0.01, stRelease = 0.06, stTail = 1.0, stMakeup = 0;  // s, s (opto: to half), s (opto's slow part), dB
    // The jazz guitar's "speaking" strip (research/jazz_guitar.md 12.3), after the speaker (an
    // engineer's channel on the miked amp). A presence band (a peak at presHz, Q presQ) lifted
    // presDb, and more or less as the band itself is louder or softer than presRef (dB, of the
    // output): speak dB of lift per dB (an upward expander on the band alone: a hard attack opens
    // it, a soft one closes it; as the note's highs die it falls back), within presLo .. presHi
    // dB. Then a gentle compressor whose ear is the signal before that lift (its body): above
    // scAt (dB rms, of the output) at scRatio, a soft knee of scKnee dB, scAttack and scRelease
    // (s): loud and soft brought a little together in level while the presence keeps the
    // difference in colour. All off by default (presDb 0, speak 0, scRatio 1).
    double presDb = 0, presHz = 3400, presQ = 0.8, speak = 0, presRef = -40, presLo = -4, presHi = 7;
    double presAttack = 0.001, presRelease = 0.08;
    double scRatio = 1, scAt = -30, scKnee = 10, scAttack = 0.01, scRelease = 0.25;
  };
  static Preset twin();
  static Preset twinComp();
  static Preset jcm800();
  static Preset slo();
  static Preset b15Di();       // GM 34: a Jazz Bass's DI and an Ampeg B-15N, LA-2A
  static Preset svt();         // GM 35: a Precision with a pick, an SVT and an 8x10, the DI, 1176
  static Preset slapDi();      // GM 37: the DI first, a clean solid-state head and a 4x10, 1176
  static Preset slapSquash();  // GM 38: as 37, the DI 80 %, the 1176 4:1 with a slower attack (no squash)
  // GM 27: a clean solid-state jazz amp (Polytone-like: research/jazz_guitar.md 5, 9.2): no tube
  // stage driven, an active EQ instead of the Fender stack, a closed 1x15, and the speaking strip.
  static Preset jazz();

  GuitarAmp(double fs, const Preset& p);
  ~GuitarAmp();
  void connect(const Source& s, double cable = 470e-12);
  void setTone(double treble, double mid, double bass);
  float process(float emf);  // V in; the speaker's sound out (arbitrary units)
  void reset();
  bool gateOpen() const { return gateOpen_; }
  // The small-signal response at f (Hz), for checking: 0 the pickup's network, 1 the tone stack,
  // 2 the cabinet, 3 the amp's path (bass: as blended, the DI's level at 200 Hz), 4 the DI's path
  // (as blended), 5 the blend (the DI's share of 4 and the rest of 3).
  std::complex<double> response(int what, double f) const;

 private:
  struct Biquad {
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double run(double x) {
      const double y = b0 * x + z1;
      z1 = b1 * x - a1 * y + z2;
      z2 = b2 * x - a2 * y;
      return y;
    }
  };
  struct OnePole {  // first order: (b0 + b1 z^-1) / (1 + a1 z^-1)
    double b0 = 1, b1 = 0, a1 = 0, z = 0;
    double run(double x) {
      const double y = b0 * x + z;
      z = b1 * x - a1 * y;
      return y;
    }
  };
  struct Tube {  // a stage's running state
    OnePole hp, lp, shelf, in;
    double shift = 0;  // its bias slid cold by grid current (V)
    double rest = 0;   // its curve at its bias
    double u1 = 0, F1 = 0;  // (antialiasing: the last grid voltage and the curve's integral there)
    bool lpOn = false, shelfOn = false, inOn = false;
  };
  static Biquad rbj(int type, double fs, double f, double q, double db = 0);  // 0 LP, 1 HP, 2 peak, 3 high shelf
  static Biquad eq(const Eq& e, double fs);                                   // those and 4, a low shelf
  static std::complex<double> at(const Biquad& b, double f, double fs);
  static std::complex<double> at(const OnePole& o, double f, double fs);
  void james(double treble, double bass);
  std::complex<double> ampPath(double f) const;  // stage 1 .. the cabinet, small signal
  std::complex<double> diPath(double f) const;   // the DI's filters, its allpass, its polarity
  void align();
  double di(double x);
  double compress(double y);
  static OnePole onePole(int type, double fs, double f);                       // 0 LP, 1 HP
  static OnePole lowShelf(double fs, double f, double db);
  static double triode(double x);
  static double triodeF(double x);  // its integral from 0
  static double triodeLin(double x);  // the curve as the integral's table has it
  template <bool kPeaks>
  double stage(const Stage& s, Tube& t, double x);
  double stack(double x);
  double screamer(double x);
  template <bool kPeaks>
  double chain(double x);  // stage 1 .. the power stage, at the oversampled rate
  void gate(double x);     // the gate's detector, at the base rate
  double fs_, fsOs_;
  Preset p_;
  Biquad input_, cab_[10], up_[4], down_[4], post_[2];
  int nCab_ = 0, nPost_ = 0;
  Tube t_[kMaxStages];
  OnePole pot_, hpOut_;
  // The tone stack: third order, direct form (transposed).
  double tb_[4] = {}, ta_[4] = {}, tz_[3] = {};
  // The James stack: two biquads (at the stages' rate); the fixed filters after the stages.
  Biquad jb_[2], voice_[6];
  // Bass: the DI's filters, its allpass, its drive's high-pass and low-pass; the mic's delay.
  Biquad di_[4], diAp_, drHp_, drLp_;
  bool bass_ = false, diApOn_ = false;
  double diSign_ = 1, ampNorm_ = 1;
  std::vector<double> delay_;
  size_t delayAt_ = 0;
  // The studio compressor: its detector (two poles), its gain reduction (dB: the opto's quick and
  // slow parts, the FET's one), the poles' rates.
  double ms1_ = 0, ms2_ = 0, grFast_ = 0, grSlow_ = 0, msA_ = 0, stAtk_ = 0, stRel_ = 0, stCharge_ = 0, stTailA_ = 0, stThrDb_ = 0;
  // (PJ_AMP_LINEAR: 1 the stages and the power stage straight, 2 the power stage only: to measure
  // what the curves add.) Each stage's slope at its bias.
  int linear_ = 0;
  double slope_[kMaxStages] = {};
  // (PJ_AMP_PEAKS: each stage's farthest grid swing against its knee, the power stage's against
  // its limit, on stderr when done.)
  bool peaks_ = false;
  double peak_[kMaxStages + 1] = {};
  double sag_ = 0, sagA_ = 0, gridA_ = 0, gridRel_ = 0;
  double compEnv_ = 0, compAtk_ = 0, compRel_ = 0, compMakeup_ = 1;
  // The noise: a generator's state, the hum's one period, where in it.
  uint32_t noise_ = 0x9e3779b9u;
  std::vector<float> humWave_;
  size_t humAt_ = 0;
  // The gate: its detector, open or not, its hold left (s), its gain now (dB) and as a factor.
  double gateEnv_ = 0, gateAtk_ = 0, gateRel_ = 0, gateHold_ = 0, gateDb_ = -80, gateGain_ = 1, gateFall_ = 0, gateRise_ = 0;
  double gatePeak_ = 0, gatePeakFall_ = 1, gateReopen_ = 0, gateReopenFall_ = 1, gateDropTo_ = 0;  // (the quick close)
  bool gateOpen_ = false, gateQuick_ = false, gateDebug_ = false;  // (PJ_AMP_GATEDEBUG: each open and close on stderr)
  long gateT_ = 0;
  // The Tube Screamer: its input's high-pass, the 720 Hz leg, the 51 pF, its tone.
  OnePole tsIn_, tsLeg_, tsCap_, tsTone_;
  double tsGain_ = 0;
  // The input with a tone control: third order, direct form (transposed).
  bool tone_ = false;
  double ib_[4] = {}, ia_[4] = {}, iz_[3] = {};
  double input3(double x);
  // The speaking strip: its band-pass, the band's envelope, the lift now (dB), the compressor's
  // detector and gain reduction, their rates.
  double strip(double x);
  bool strip_ = false;
  Biquad pres_;
  double presEnv_ = 0, presGain_ = 0, presAtk_ = 0, presRel_ = 0, presSmooth_ = 0;
  double scMs1_ = 0, scMs2_ = 0, scGr_ = 0, scAtk_ = 0, scRel_ = 0;
 public:
  // (For the checks: the strip's last lift (dB) and gain reduction (dB).)
  double presence() const { return presGain_; }
  double reduction() const { return scGr_; }
};

}  // namespace pajupilli
