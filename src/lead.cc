#include "lead.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>

#include "knob.h"

namespace pajupilli {

namespace {

constexpr double kBoltzmannQ = 8.617333e-5;  // k / q (V/K)
double thermalVoltage(double tempC) { return kBoltzmannQ * (tempC + 273.15); }
const double kVT25 = thermalVoltage(25.0);

// Every note is just a control voltage: no fingerings.
const NoteInfo kLeadNotes[61] = {
#define N {"", 1}
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N
#undef N
};
const Design kLeadSawDesign = {nullptr, 0, nullptr, 0, kLeadNotes, 0, 0, false, nullptr, 0, {}, {}};

// The board the voices share (all leads in this process: one synthesizer).
struct Board {
  struct V {
    bool on = false;
    double level = 0, iout = 0, slope = 0, inbox = 0;
  };
  std::vector<V> v;
  double rail = 15, ground = 0, sumIout = 0, rise = 0, t = 0, hum = 0;
  int join() {
    for (size_t i = 0; i < v.size(); ++i)
      if (!v[i].on) { v[i] = V(); v[i].on = true; return int(i); }
    v.push_back(V());
    v.back().on = true;
    return int(v.size()) - 1;
  }
  void leave(int i) {
    sumIout -= v[i].iout;
    v[i] = V();
  }
  static double cx(int i, int j) { return std::abs(i - j) == 1 ? 0.5e-12 : 0.1e-12; }
  // A reset: voice i's output jumps by dv; every other summing node gets Cx dv.
  void reset(int i, double dv, double scale) {
    for (size_t j = 0; j < v.size(); ++j)
      if (int(j) != i && v[j].on) v[j].inbox += scale * cx(i, int(j)) * dv;
  }
  double take(int i) {
    const double q = v[i].inbox;
    v[i].inbox = 0;
    return q;
  }
  double slopeCurrent(int i, double scale) const {
    double a = 0;
    for (size_t j = 0; j < v.size(); ++j)
      if (int(j) != i && v[j].on) a += cx(i, int(j)) * v[j].slope;
    return scale * a;
  }
  void output(int i, double iout) {
    sumIout += iout - v[i].iout;
    v[i].iout = iout;
  }
  // Once per control block, by the first voice: the rail, heat, hum.
  void update(int i, double dt, double scale) {
    for (size_t j = 0; j < v.size(); ++j)
      if (v[j].on) {
        if (int(j) != i) return;
        break;
      }
    t += dt;
    double load = 0;
    for (const V& x : v)
      if (x.on) load += x.level;
    // The rectifier's ripple (50 Hz mains, full wave), what the regulator leaves of it.
    const double ripple = 0.5e-3 * (std::fabs(std::sin(2 * M_PI * 50 * t)) - 2 / M_PI);
    rail = 15 - scale * (0.3 * 5e-3 * load + ripple);
    rise += (scale * 0.6 * load - rise) * dt / 40;
    const double w = 2 * M_PI * 50 * t;
    hum = scale * 6e-5 * (std::sin(w) + 0.5 * std::sin(3 * w) + 0.25 * std::sin(5 * w));
  }
};
Board& board() {
  static Board b;
  return b;
}

}  // namespace

// The GM synth leads (81-88): one core and one ladder each, and between
// them each its own sound circuit (research/synth_leads.md section 4).
const LeadPatch kPatches[] = {
    // 82 Sawtooth: the ramp alone.
    {"lead-saw"},
    // 81 Square: a comparator on the ramp; its threshold swept slowly by its
    // own triangle (a pulse width 35..65 %): hollow, odd harmonics, moving.
    [] { LeadPatch p{"lead-square"}; p.saw = 0; p.pulse = 1; p.pwm = 0.15; p.cutoff = 6; p.fltEnv = 0.7;
         p.ampD = 0.4; p.ampS = 0.8; return p; }(),
    // 83 Calliope: the ramp rectified into a triangle, a differential pair
    // rounds it into a near-sine; breath from a junction's noise, its band
    // through a delay-line comb at the note's period (pitched breath, as a
    // pipe's); a slow wobble of the pitch (a steam pipe's pressure).
    [] { LeadPatch p{"lead-calliope"}; p.saw = 0; p.sine = 1; p.breath = 0.35; p.comb = 0.9; p.wobble = 4;
         p.ampA = 0.025; p.ampD = 0.5; p.ampS = 0.9; p.cutoff = 0; p.fltEnv = 0.2; p.resScale = 0.3; return p; }(),
    // 84 Chiff: a mellow tone (the sine and a little pulse), and at each
    // attack a resonant filter a few octaves up pinged by a noise burst.
    [] { LeadPatch p{"lead-chiff"}; p.saw = 0.15; p.sine = 0.8; p.pulse = 0.25; p.pw = 0.3; p.chiff = 1;
         p.breath = 0.08; p.comb = 0.6; p.ampA = 0.006; p.ampD = 0.3; p.ampS = 0.8; p.cutoff = -3;
         p.fltEnv = 0.3; p.resScale = 0.5; return p; }(),
    // 85 Charang: two cores 7 c apart (they beat), an asymmetric diode
    // overdrive (compressed, sustaining), a mid scoop after it.
    [] { LeadPatch p{"lead-charang"}; p.second = 1; p.detune = 0.07; p.drive = 1; p.notch = 1;
         p.ampA = 0.003; p.ampD = 0.8; p.ampS = 0.85; p.cutoff = 3; p.fltEnv = 0.6; p.resScale = 0.6; return p; }(),
    // 86 Voice: a narrow (glottal) pulse and the ramp into four formant
    // band-passes, "oo" for a soft key to "ah" for a hard one; breath through
    // them too; a bucket-brigade chorus after the amplifier.
    [] { LeadPatch p{"lead-voice"}; p.saw = 0.6; p.pulse = 0.7; p.pw = 0.18; p.formants = 1; p.breath = 0.12;
         p.chorus = 1; p.ampA = 0.035; p.ampD = 0.5; p.ampS = 0.9; p.cutoff = 18; p.fltEnv = 0.15;
         p.resScale = 0.2; return p; }(),
    // 87 Fifths: a second core pre-set 3 c under a just fifth, pulled onto 3:2
    // by soft sync from the first core's reset (every second reset finds it
    // near its top); a sine wavefolder on the pair.
    [] { LeadPatch p{"lead-fifths"}; p.second = 0.8; p.secondSemis = 12 * std::log2(1.5) - 0.03; p.syncWin = 0.04;
         p.fold = 1; return p; }(),
    // 88 Bass + Lead: flip-flops halve and quarter the resets (exact octaves
    // below); the ramp ring-modulated by the /2 square (it growls).
    [] { LeadPatch p{"lead-bass+lead"}; p.sub = 0.9; p.sub2 = 0.4; p.ring = 0.35; p.ampA = 0.003; p.ampD = 0.3;
         p.ampS = 0.8; p.cutoff = 6; return p; }(),
    // The synth versions of the acoustic families (GM 39, 40, 51, 52, 55, 63, 64).
    // 39 Synth Bass 1: the ramp and its octave below (/2), a low cutoff that a
    // fast filter envelope snaps open (a "pluck"), resonant.
    [] { LeadPatch p{"synth-bass-1"}; p.sub = 0.7; p.ampA = 0.002; p.ampD = 0.6; p.ampS = 0.7; p.cutoff = -12;
         p.fltEnv = 1.6; p.fltA = 0.002; p.fltD = 0.12; p.fltS = 0.05; p.resScale = 1.2; p.relMul = 0.4; return p; }(),
    // 40 Synth Bass 2: a narrow pulse ring-modulated by its octave square
    // (rubbery), the sine under it, overdriven a little.
    [] { LeadPatch p{"synth-bass-2"}; p.saw = 0; p.pulse = 0.8; p.pw = 0.3; p.sine = 0.6; p.ring = 0.5;
         p.drive = 0.25; p.ampA = 0.002; p.ampD = 0.4; p.ampS = 0.6; p.cutoff = -6; p.fltEnv = 1.2; p.fltA = 0.003;
         p.fltD = 0.2; p.fltS = 0.1; p.relMul = 0.4; return p; }(),
    // 51 Synth Strings 1: two cores 12 c apart and a swept pulse, a slow bow
    // (attack), a long release, the bucket-brigade ensemble (chorus).
    [] { LeadPatch p{"synth-strings-1"}; p.second = 1; p.detune = 0.12; p.pulse = 0.5; p.pwm = 0.2; p.chorus = 1;
         p.ampA = 0.28; p.ampD = 1.0; p.ampS = 0.95; p.cutoff = 0; p.fltEnv = 0.3; p.fltA = 0.3; p.fltD = 1;
         p.fltS = 0.6; p.resScale = 0.3; p.relMul = 3; return p; }(),
    // 52 Synth Strings 2: brighter, quicker: the cores 20 c apart, no pulse.
    [] { LeadPatch p{"synth-strings-2"}; p.second = 1; p.detune = 0.2; p.chorus = 1; p.ampA = 0.12; p.ampD = 1.0;
         p.ampS = 0.95; p.cutoff = 8; p.fltEnv = 0.4; p.fltA = 0.12; p.fltD = 1; p.fltS = 0.5; p.resScale = 0.4;
         p.relMul = 2; return p; }(),
    // 55 Synth Voice: a scat singer. The formant bank sings a syllable on each
    // attack (da, du, bi, dei, ba, bu, bei ...: the vowel from the key's speed),
    // breathy, through the ensemble.
    [] { LeadPatch p{"synth-voice"}; p.saw = 0.5; p.pulse = 0.8; p.pw = 0.15; p.formants = 1; p.scat = 1;
         p.breath = 0.5; p.chorus = 0.8; p.ampA = 0.01; p.ampD = 0.7; p.ampS = 0.85; p.cutoff = 18; p.fltEnv = 0.1;
         p.resScale = 0.2; p.relMul = 1.2; return p; }(),
    // 63 Synth Brass 1: two cores 6 c apart; the filter opens slower than the
    // amplifier speaks, overshoots and settles (the "bwah").
    [] { LeadPatch p{"synth-brass-1"}; p.second = 1; p.detune = 0.06; p.ampA = 0.012; p.ampD = 0.5; p.ampS = 0.85;
         p.cutoff = -8; p.fltEnv = 1.6; p.fltA = 0.06; p.fltD = 0.35; p.fltS = 0.45; p.resScale = 0.5; return p; }(),
    // 64 Synth Brass 2: the ramp and a wide pulse, softer and slower.
    [] { LeadPatch p{"synth-brass-2"}; p.pulse = 0.6; p.pw = 0.4; p.ampA = 0.025; p.ampD = 0.6; p.ampS = 0.85;
         p.cutoff = -10; p.fltEnv = 1.3; p.fltA = 0.11; p.fltD = 0.5; p.fltS = 0.5; p.resScale = 0.4; return p; }(),
    // 118 Melodic Tom: the sine (a skin's mode), its pitch falling from a few
    // semitones above as the skin relaxes; the stick's noise; it dies away held.
    [] { LeadPatch p{"melodic-tom"}; p.saw = 0; p.sine = 1; p.chiff = 0.6; p.ampA = 0.001; p.ampD = 0.35;
         p.ampS = 0; p.cutoff = 12; p.fltEnv = 0.5; p.fltA = 0.001; p.fltD = 0.1; p.fltS = 0; p.resScale = 0.2;
         p.pitchEnv = 4; p.pitchTau = 0.12; p.relMul = 0.5; return p; }(),
    // 119 Synth Drum: the analog drum: a triangle-sine swept down an octave in
    // ~50 ms through a resonant filter, a noise burst on top.
    [] { LeadPatch p{"synth-drum"}; p.saw = 0.1; p.sine = 1; p.chiff = 1; p.ampA = 0.0007; p.ampD = 0.22; p.ampS = 0;
         p.cutoff = 6; p.fltEnv = 1.2; p.fltA = 0.001; p.fltD = 0.06; p.fltS = 0; p.resScale = 0.8; p.pitchEnv = 12;
         p.pitchTau = 0.045; p.relMul = 0.4; return p; }(),
};
constexpr int kNumPatches = int(sizeof(kPatches) / sizeof(kPatches[0]));

extern const Instrument kLeads[] = {
    {"lead-saw", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "lead"},
    {"lead-square", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "lead"},
    {"lead-calliope", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "lead"},
    {"lead-chiff", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "lead"},
    {"lead-charang", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "lead"},
    {"lead-voice", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "lead"},
    {"lead-fifths", 0, 36, 89, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "lead"},
    {"lead-bass+lead", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "lead"},
    {"synth-bass-1", 0, 28, 67, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "bass"},
    {"synth-bass-2", 0, 28, 67, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "bass"},
    {"synth-strings-1", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "strings"},
    {"synth-strings-2", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "strings"},
    {"synth-voice", 0, 41, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "voice"},
    {"synth-brass-1", 0, 36, 89, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "brass"},
    {"synth-brass-2", 0, 36, 89, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "brass"},
    {"melodic-tom", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "tom"},
    {"synth-drum", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kCircuit, &kLeadSawDesign, "drum"},
};
extern const int kNumLeads = int(sizeof(kLeads) / sizeof(kLeads[0]));

Lead::Lead(const Instrument& inst) : Tube(inst) {
  patch_ = &kPatches[0];
  for (const LeadPatch& p : kPatches)
    if (std::strcmp(p.name, inst.name) == 0) patch_ = &p;
  simple_ = patch_ == &kPatches[0];
  if (!simple_) {
    comb_.assign(4096, 0.0f);
    bbd_.assign(2048, 0.0f);
    // The mid scoop: a notch at 650 Hz, Q 0.6 (RBJ), half deep.
    const double w = 2 * M_PI * 650 / fs_, al = std::sin(w) / (2 * 0.6), a0 = 1 + al;
    nb_[0] = 1 / a0; nb_[1] = -2 * std::cos(w) / a0; nb_[2] = 1 / a0;
    nb_[3] = -2 * std::cos(w) / a0; nb_[4] = (1 - al) / a0;
  }
  kAnalog_ = knob("PJ_ANALOG", 1);
  kWander_ = knob("PJ_FILTERWANDER", 1);
  id_ = board().join();
  seed(0);
}

Lead::~Lead() { board().leave(id_); }

void Lead::seed(unsigned s) {
  Tube::seed(s);
  rng_ = 99991u + 7919u * s;
  // The parts (seed 0: nominal, at 25 C): C within 5 %, the divider within 1 %,
  // the voice's own temperature.
  const bool nominal = s == 0;
  auto tol = [&](double v, double t) { return nominal ? v : v * (1 + t * urand()); };
  C_ = tol(1e-9, 0.05);
  r3_ = tol(100e3, 0.01);
  r4_ = tol(574.5e3, 0.01);
  vref_ = tol(2.257, 0.005);
  slew_ = tol(13e6, 0.2);
  tempC_ = nominal ? 25.0 : 25.0 + 0.4 * urand();
  re_ = nominal ? 1.5 : 1.0 + 1.0 * (0.5 + 0.5 * urand());  // emitter resistance (ohm)
  leak_ = nominal ? 1e-9 : (0.5 + 1.5 * (0.5 + 0.5 * urand())) * 1e-9;  // summing-node leakage (A)
  tempDrift_ = 0;
  // The imperfections that are not tolerances: the same for every voice at
  // seed 0 (but present), random per voice otherwise.
  auto r = [&](double v) { return nominal ? v : v * urand(); };
  tcErr_ = nominal ? 0.05 : 0.1 * urand();
  for (double& o : vos_) o = r(1e-3);
  if (nominal) { vos_[0] = 0.8e-3; vos_[1] = -0.5e-3; vos_[2] = 0.3e-3; vos_[3] = -0.9e-3; vos_[4] = 0.4e-3; }
  ft_ = nominal ? 0.02 : 0.03 * urand();
  bleed_ = std::pow(10, -75 / 20.0) * (nominal ? 1 : 1 + 0.5 * urand());
  cvNoise_ = 0;
  fcWander_ = resWander_ = Wander();
  // The LFO: where it is at power-on. (Its rate is set per note, by the
  // player: uniform within 15 % of the setting.)
  lfoRateMul_ = 1;
  {
    const double ph = nominal ? 0 : M_PI * urand();
    lfoRe_ = std::cos(ph);
    lfoIm_ = std::sin(ph);
  }
  lfoWander_ = lfoN1_ = lfoD1_ = lfoDepthNow_ = 0;
  lfoDepthMul_ = nominal ? 1 : 1 + 0.25 * urand();
  // Comparator thresholds from its divider: th = (v R4 + out R3) / (R3 + R4) = vref.
  vhi_ = (vref_ * (r3_ + r4_) + vsw_ * r3_) / r4_;
  vlo_ = (vref_ * (r3_ + r4_) - vsw_ * r3_) / r4_;
  vhiNow_ = vhi_;
  vloNow_ = vlo_;
  // The expo converter's reference current: designed for A4 over a 5 V ramp on
  // a nominal 1 nF (as the SPICE netlist).
  iref_ = 1e-9 * 5.0 * 440.0;
  if (!simple_) {  // the second core's capacitor; the pulse's sweep starts anywhere
    C2_ = nominal ? 1e-9 : 1e-9 * (1 + 0.05 * urand());
    pwmPh_ = nominal ? 0 : M_PI * urand();
  }
  resetAir();
  autotune();
}

void Lead::resetAir() {
  Tube::resetAir();
  if (id_ >= 0) {  // this voice's share of the board, cleared
    Board& b = board();
    b.output(id_, 0);
    b.v[id_].inbox = 0;
    b.v[id_].slope = 0;
    if (!std::isfinite(b.sumIout)) {
      b.sumIout = 0;
      for (const Board::V& x : b.v) b.sumIout += x.iout;
    }
  }
  v_ = vlo_;
  dead_ = blepNext_ = prevOut_ = 0;
  std::fill(y_, y_ + 4, 0.0);
  std::fill(w_, w_ + 4, 0.0);
  y4Prev_ = 0;
  if (!simple_) {
    vStart_ = vPre_ = v_;
    resetD_ = -1;
    pulseHi_ = -1; pulseOut_ = pulseNext_ = 0;
    ff1_ = ff2_ = 1; ff1Out_ = ff1Next_ = ff2Out_ = ff2Next_ = 0;
    v2_ = vlo_; dead2_ = prev2_ = out2_ = 0;
    brLp_ = brHp_ = chiffEnv_ = chiffA_ = chiffB_ = 0;
    std::fill(comb_.begin(), comb_.end(), 0.0f);
    std::fill(bbd_.begin(), bbd_.end(), 0.0f);
    bbdLp_ = 0;
    for (int k = 0; k < 4; ++k) fmA_[k] = fmB_[k] = 0;
    nz_[0] = nz_[1] = 0;
  }
}

// The expo converter: a matched pair (its saturation currents cancel), the CV
// scaled for 25 C (VT ln 2 per octave) by a tempco resistor that follows VT
// with the absolute temperature; `tcErr` is the share of VT's drift it leaves
// (0: perfect; -1: none).
static double expoCurrent(double iref, double note, double tempC, double tcErr) {
  const double x = (note - 69) / 12 * std::log(2.0);
  return iref * std::exp(x * (1 - tcErr * (tempC - 25) / 298.15));
}

// The current charging C, with the parasitics that move the pitch
// (tools/circuit.py parasitics; the rest only shift it, which the autotune
// takes out): the expo transistor's emitter resistance takes I RE off its
// base-emitter voltage (the top goes flat: -2.4 c at A7 for 2 ohm), and
// junction leakage at the summing node adds a few nA, doubling every 10 C
// (the bottom goes sharp: +13 c at A1 for 2 nA).
double Lead::coreCurrent(double note, double tempC) const {
  const double i = expoCurrent(iref_, note, tempC, tcErr_);
  const double vt = thermalVoltage(tempC);
  const double ie = i * std::exp(-i * re_ / vt);  // (one fixed-point step: I RE << VT)
  return ie + leak_ * std::exp2((tempC - 25) / 10);
}

double Lead::wanderKick(double rate, double dt) {
  const double hit = 0.5 + 0.5 * urand(), u = urand();
  if (hit >= rate * dt) return 0;
  // Laplace, unit variance (b = 1/sqrt 2), scaled so the kicks sum to unit
  // variance per second whatever their rate.
  const double l = -(u < 0 ? -1 : 1) * std::log(std::max(1e-7, 1 - std::fabs(u))) / std::sqrt(2.0);
  return l / std::sqrt(rate);
}

double Lead::lfoStep(double rate, double dt) {
  // A breathing, not a machine: the rotation's speed and the depth wander,
  // each a noise through two low-passes (smooth, nothing faster than about
  // 0.3 Hz: it changes over several cycles, never within one). The speed
  // ~10 % (the phase strays by about a cycle over a few seconds), the depth
  // ~20 %. PJ_VIBNOISE scales both.
  static const double kNoise = knob("PJ_VIBNOISE", 1);
  constexpr double kTau = 0.6, kRate = 0.10, kDepth = 0.20;
  const double k = std::sqrt(2 * dt / kTau) * std::sqrt(3.0) * std::sqrt(2.0) * kAnalog_ * kNoise;
  lfoN1_ += k * urand() - dt / kTau * lfoN1_;
  lfoWander_ += (lfoN1_ - lfoWander_) * dt / kTau;
  lfoD1_ += k * urand() - dt / kTau * lfoD1_;
  lfoDepthNow_ += (lfoD1_ - lfoDepthNow_) * dt / kTau;
  const double a = 2 * M_PI * rate * lfoRateMul_ * std::max(0.3, 1 + kRate * lfoWander_) * dt;
  const double c = std::cos(a), s = std::sin(a);
  const double re = lfoRe_ * c - lfoIm_ * s;
  lfoIm_ = lfoRe_ * s + lfoIm_ * c;
  lfoRe_ = re;
  // The amplitude loop: |z| back towards 1.
  const double m = 1.5 - 0.5 * (lfoRe_ * lfoRe_ + lfoIm_ * lfoIm_);
  lfoRe_ *= m;
  lfoIm_ *= m;
  return lfoRe_ * std::max(0.0, 1 + kDepth * lfoDepthNow_);
}

double Lead::measureC(double note, double C) {
  // Run the core alone: the ramp from the lower threshold, sub-sample crossings
  // over a few periods (what an autotune counter does with a crystal clock).
  const double dt = 1 / fs_, slope = coreCurrent(note, tempC_) / C, td = (vhi_ - vlo_) / slew_;
  double v = vlo_, dead = 0, t = 0, first = -1, last = -1;
  int n = 0;
  for (long k = 0; k < long(fs_) && n < 6; ++k) {
    double avail = dt;
    if (dead > 0) {
      const double u = std::min(dead, avail);
      dead -= u;
      avail -= u;
    }
    v += slope * avail;
    t += dt;
    if (v >= vhi_) {
      const double over = (v - vhi_) / slope;
      const double te = t - over;
      if (first < 0) first = te;
      last = te;
      ++n;
      const double rem = over - td;
      if (rem >= 0) v = vlo_ + slope * rem;
      else { v = vlo_; dead = -rem; }
    }
  }
  return n > 1 ? (n - 1) / (last - first) : 0;
}

void Lead::autotune() {
  for (int k = 0; k <= 10; ++k) {
    const double note = 12.0 * k;
    const double f = measure(note);
    tune_[k] = f > 0 ? 1200 * std::log2(noteFrequency(int(note)) / f) : 0;
    if (simple_ || patch_->second == 0) continue;
    const double f2 = measureC(note, C2_);  // the second core, its own trim
    tune2_[k] = f2 > 0 ? 1200 * std::log2(noteFrequency(int(note)) / f2) : 0;
  }
}

double Lead::trim(double note) const {
  const double u = std::clamp(note / 12, 0.0, 10.0);
  const int k = std::min(9, int(u));
  return tune_[k] + (tune_[k + 1] - tune_[k]) * (u - k);
}

void Lead::controlStep() {
  const double blockDt = 16 / fs_;
  Board& b = board();
  b.update(id_, blockDt, kAnalog_);
  // The voice's temperature wanders a little (tenths of a degree over minutes),
  // and the board warms with what is played.
  tempDrift_ += std::sqrt(blockDt) * 0.004 * urand() - blockDt / 60 * tempDrift_;
  const double temp = tempC_ + tempDrift_ + b.rise;
  // The expo transistor's 1/f noise: a slow wander of the CV (~0.03 c).
  cvNoise_ += kAnalog_ * std::sqrt(blockDt) * 3e-4 * urand() - blockDt / 2 * cvNoise_;
  const double lfo = lfoStep(ctl_.vibRate, blockDt);  // (runs always, as the board's LFOs do)
  const double note = ctl_.note + trim(ctl_.note) / 100 + cvNoise_ + lfoDepthMul_ * ctl_.vibDepth * lfo;
  double noteW = note;
  if (!simple_ && patch_->wobble > 0) {  // a pipe's slow wobble (~2 Hz, cents)
    wob1_ += std::sqrt(blockDt) * 6 * urand() - blockDt * 12 * wob1_;
    wob2_ += (wob1_ - wob2_) * blockDt * 12;
    noteW += patch_->wobble / 100 * wob2_;
  }
  if (!simple_) noteW -= patch_->detune / 2;
  slope_ = coreCurrent(noteW, temp) / C_;
  if (!simple_) {
    const LeadPatch& P = *patch_;
    if (P.second > 0) {
      const double n2 = noteW + P.secondSemis + P.detune;
      const double u = std::clamp(n2 / 12, 0.0, 10.0);
      const int k = std::min(9, int(u));
      slope2_ = coreCurrent(n2 + (tune2_[k] + (tune2_[k + 1] - tune2_[k]) * (u - k)) / 100 - trim(ctl_.note) / 100,
                            temp) / C2_;
    }
    const double f0 = noteFrequency(69) * std::exp2((ctl_.note - 69) / 12);
    combLen_ = std::clamp(fs_ / f0, 2.0, double(comb_.size() - 2));
    if (P.pwm > 0) {  // the width's own triangle, ~0.6 Hz
      pwmPh_ += 2 * M_PI * 0.6 * blockDt;
      if (pwmPh_ > M_PI) pwmPh_ -= 2 * M_PI;
      pw_ = P.pw + P.pwm * (2 * std::fabs(pwmPh_) / M_PI - 1);
    } else {
      pw_ = P.pw;
    }
    if (ctl_.strike != lastStrike_) {  // an attack: the chiff's burst; a new syllable
      lastStrike_ = ctl_.strike;
      chiffEnv_ = 0.3 + 0.7 * ctl_.vel;
      if (P.scat > 0) {
        // The vowel from the key's speed (soft u, a, i, hard ei), now and
        // then a neighbour; the stop mostly alternating d, b.
        const double v = std::clamp(ctl_.vel + 0.08 * urand(), 0.0, 1.0);
        scV_ = v < 0.32 ? 1 : v < 0.6 ? 0 : v < 0.78 ? 2 : 3;  // u a i e(i)
        scV2_ = scV_ == 3 ? 2 : scV_;                        // ei: e gliding to i
        scC_ = urand() < 0.4 ? scC_ : 1 - scC_;
        scT_ = 0;
        scClose_ = 0.010 + 0.015 * (1 - ctl_.vel);  // a soft syllable lingers in the closure
      }
    }
    {  // the chiff's resonator: ~4 x the note, Q 6
      const double fc = std::clamp(4 * f0, 900.0, 0.4 * fs_);
      chiffG_ = std::tan(M_PI * fc / fs_);
    }
    if (P.formants > 0 && P.scat > 0) {
      double F[4], G[4];
      scatFormants(blockDt, f0, F, G);
      static const double q[4] = {9, 13, 18, 20};
      for (int k = 0; k < 4; ++k) {
        fmG_[k] = std::tan(M_PI * std::min(F[k], 0.45 * fs_) / fs_);
        fmK_[k] = 1 / q[k];
        fmGain_[k] = G[k];
      }
    } else if (P.formants > 0) {
      // "oo" (soft) .. "ah" (hard); the formants follow the note a little.
      static const double oo[4] = {320, 800, 2300, 3300}, ah[4] = {750, 1150, 2600, 3500};
      static const double q[4] = {7, 9, 11, 13};
      const double a = std::clamp(ctl_.vel, 0.0, 1.0), sc = std::pow(f0 / 220, 0.12);
      for (int k = 0; k < 4; ++k) {
        const double f = std::min(sc * (oo[k] + a * (ah[k] - oo[k])), 0.45 * fs_);
        fmG_[k] = std::tan(M_PI * f / fs_);
        fmK_[k] = 1 / q[k];
      }
    }
  }
  // The comparator on the present rail: its swing and its reference (a divider
  // from +15 V) follow it.
  {
    const double vsw = vsw_ + (b.rail - 15), vref = vref_ * b.rail / 15;
    vhiNow_ = (vref * (r3_ + r4_) + vsw * r3_) / r4_;
    vloNow_ = (vref * (r3_ + r4_) - vsw * r3_) / r4_;
  }
  // The others' ramps through the parallel wires: a current into the summing node.
  xtalk_ = -b.slopeCurrent(id_, kAnalog_) / C_;
  b.v[id_].slope = slope_;
  b.v[id_].level = level_;
  // The filter's wander (std ~0.15 semitone in the cutoff, ~0.02 in the
  // resonance; tools: PJ_FILTERWANDER scales it).
  const double kw = kAnalog_ * kWander_;
  fcWander_.step(0.53 * kw * wanderKick(8, blockDt), blockDt);
  resWander_.step(0.07 * kw * wanderKick(5, blockDt), blockDt);
  // The ladder's cutoff: an expo current too.
  const double fc = std::min(
      noteFrequency(69) * std::exp2((ctl_.cutoff + fcWander_.x - 69) / 12 * kVT25 / thermalVoltage(temp)), 0.4 * fs_);
  g_ = 1 - std::exp(-2 * M_PI * fc / (2 * fs_));
  res_ = std::clamp(ctl_.res + resWander_.x, 0.0, 1.1);
  vt2_ = 2 * thermalVoltage(temp);
}

double Lead::coreStep(double& out) {
  const double dt = 1 / fs_;
  Board& b = board();
  double avail = dt;
  if (dead_ > 0) {
    const double u = std::min(dead_, avail);
    dead_ -= u;
    avail -= u;
  }
  // The others' resets through the wires: a charge into the summing node (the
  // integrator's output steps by -Q / C).
  v_ -= b.take(id_) / C_;
  // The ground under the expo transistor's base, lifted by the outputs'
  // currents: its collector current follows exp(dV / VT).
  const double slope = (slope_ * (1 + kAnalog_ * 0.5e-3 * b.sumIout / kVT25) + xtalk_);
  const double vStart = v_;
  v_ += slope * avail;
  const double vPre = v_;
  double resetD = -1;
  double corrPrev = 0, corrCur = 0;
  // The comparator's threshold, with a little noise (its input, ~0.5 mV).
  const double hi = vhiNow_ + 5e-4 * urand();
  if (v_ >= hi) {
    const double over = std::min((v_ - hi) / slope, dt);
    const double d = over / dt;  // the share of this sample after the crossing
    const double td = (vhiNow_ - vloNow_) / slew_;
    const double rem = over - td;
    const double before = v_;
    if (rem >= 0) v_ = vloNow_ + slope * rem;
    else { v_ = vloNow_; dead_ = -rem; }
    b.reset(id_, vloNow_ - hi, kAnalog_);
    // polyBLEP for the drop h (band-limited minus naive, two samples).
    const double h = (v_ - before) + slope * over;  // the step alone (the ramp goes on)
    corrPrev = h * d * d / 2;
    corrCur = -h * (1 - d) * (1 - d) / 2;
    resetD = d;
  }
  out = prevOut_ + corrPrev;
  prevOut_ = v_ + corrCur;
  if (!simple_) events(slope, vStart, vPre, resetD, resetD >= 0);
  return out;
}

// The comparator (a pulse) and the flip-flops on the core's events, band-
// limited like the ramp's reset (polyBLEP, one sample late, as the ramp).
double Lead::events(double slope, double vStart, double vPre, double d, bool reset) {
  const double dt = 1 / fs_;
  const double th = vloNow_ + pw_ * (vhiNow_ - vloNow_);
  double cp = 0, cc = 0;
  auto edge = [&](double h, double dd) {
    dd = std::clamp(dd, 0.0, 1.0);
    cp += h * dd * dd / 2;
    cc -= h * (1 - dd) * (1 - dd) / 2;
  };
  // Rising: the ramp crosses the threshold (before a reset, or after it).
  if (vStart < th && vPre >= th) edge(2, (vPre - th) / (slope * dt));
  if (reset) {
    if (vPre >= th) edge(-2, d);  // the reset: the comparator falls
    if (v_ >= th) edge(2, (v_ - th) / (slope * dt));
  }
  pulseHi_ = v_ >= th ? 1 : -1;
  pulseOut_ = pulseNext_ + cp;
  pulseNext_ = pulseHi_ + cc;
  // The flip-flops: the first toggles on every reset, the second on the first's rising edge.
  double c1p = 0, c1c = 0, c2p = 0, c2c = 0;
  if (reset) {
    const double dd = std::clamp(d, 0.0, 1.0);
    const double h1 = -2 * ff1_;
    c1p = h1 * dd * dd / 2;
    c1c = -h1 * (1 - dd) * (1 - dd) / 2;
    ff1_ = -ff1_;
    if (ff1_ > 0) {
      const double h2 = -2 * ff2_;
      c2p = h2 * dd * dd / 2;
      c2c = -h2 * (1 - dd) * (1 - dd) / 2;
      ff2_ = -ff2_;
    }
  }
  ff1Out_ = ff1Next_ + c1p;
  ff1Next_ = ff1_ + c1c;
  ff2Out_ = ff2Next_ + c2p;
  ff2Next_ = ff2_ + c2c;
  // The second core: its own ramp, reset at its top; soft sync: at the first
  // core's reset, if it is within the window of its top, it fires then.
  if (patch_->second > 0) {
    double avail = dt;
    if (dead2_ > 0) {
      const double u = std::min(dead2_, avail);
      dead2_ -= u;
      avail -= u;
    }
    v2_ += slope2_ * avail;
    double p2 = 0, q2 = 0;
    bool fired = false;
    const double td = (vhiNow_ - vloNow_) / slew_;
    if (v2_ >= vhiNow_) {
      const double over = std::min((v2_ - vhiNow_) / slope2_, dt), dd = over / dt, rem = over - td;
      const double before = v2_;
      if (rem >= 0) v2_ = vloNow_ + slope2_ * rem;
      else { v2_ = vloNow_; dead2_ = -rem; }
      const double h = (v2_ - before) + slope2_ * over;
      p2 = h * dd * dd / 2;
      q2 = -h * (1 - dd) * (1 - dd) / 2;
      fired = true;
    }
    if (!fired && reset && patch_->syncWin > 0) {
      const double dd = std::clamp(d, 0.0, 1.0);
      const double vAt = v2_ - slope2_ * dd * dt;
      if (vhiNow_ - vAt <= patch_->syncWin * (vhiNow_ - vloNow_)) {
        const double before = v2_;
        v2_ = vloNow_ + slope2_ * dd * dt;
        const double h = v2_ - before;
        p2 = h * dd * dd / 2;
        q2 = -h * (1 - dd) * (1 - dd) / 2;
      }
    }
    out2_ = prev2_ + p2;
    prev2_ = v2_ + q2;
  }
  return 0;
}

// Scat: the syllable's formants this block (Hz, F1..F4) and their levels.
//   - The closure (10..25 ms): the lips or the tongue shut; only the voice
//     bar (a murmur under 300 Hz) sounds; then a burst of noise.
//   - The release: the formants glide from the stop's locus to the vowel
//     (F1 from 250 Hz; F2 from ~900 Hz for b, ~1700 Hz for d), ~30 ms.
//   - ei: e gliding to i from 120 ms over ~150 ms.
void Lead::scatFormants(double dt, double f0, double* F, double* G) {
  // a (open), u, i, e: formants (adult, between a man's and a woman's) and levels.
  static const double V[4][4] = {{800, 1250, 2650, 3500}, {330, 780, 2300, 3300},
                                 {290, 2250, 2950, 3600}, {460, 1900, 2600, 3500}};
  static const double Gv[4][4] = {{1.0, 0.9, 0.45, 0.25}, {1.0, 0.35, 0.12, 0.08},
                                  {0.8, 0.5, 0.45, 0.3}, {1.0, 0.65, 0.4, 0.25}};
  static const double locus[2][3] = {{250, 1700, 2700}, {250, 900, 2200}};  // d, b
  scT_ += dt;
  const double t = scT_ - scClose_;
  // The vowel now (a diphthong glides to its second vowel).
  double tgt[4], gain[4];
  const double dg = scV2_ != scV_ ? std::clamp((scT_ - 0.12) / 0.15, 0.0, 1.0) : 0;
  const double sm = dg * dg * (3 - 2 * dg);
  for (int k = 0; k < 4; ++k) {
    tgt[k] = V[scV_][k] + sm * (V[scV2_][k] - V[scV_][k]);
    gain[k] = Gv[scV_][k] + sm * (Gv[scV2_][k] - Gv[scV_][k]);
  }
  const double sc = std::pow(std::max(f0, 80.0) / 220, 0.1);  // a higher voice, higher formants
  if (t < 0) {  // the closure: the voice bar
    srcGain_ = 0.12;
    F[0] = 250; F[1] = locus[scC_][1]; F[2] = locus[scC_][2]; F[3] = 3400;
    G[0] = 1; G[1] = 0.05; G[2] = 0.03; G[3] = 0.02;
    return;
  }
  if (t < dt) burst_ = 1;  // the release
  srcGain_ = std::min(1.0, 0.12 + t / 0.012);
  const double a = 1 - std::exp(-t / 0.030);
  for (int k = 0; k < 4; ++k) {
    const double from = k < 3 ? locus[scC_][k] : tgt[3];
    F[k] = sc * (from + a * (tgt[k] - from));
    G[k] = gain[k];
  }
}

// The patch's sound circuit: from the core's outputs to the ladder's input
// (volts around the ramp's middle).
double Lead::sound(double saw) {
  const LeadPatch& P = *patch_;
  const double mid = 0.5 * (vhi_ + vlo_), half = 0.5 * (vhiNow_ - vloNow_);
  const double r = saw - mid;
  double m = P.saw * r;
  if (P.pulse > 0) m += P.pulse * half * pulseOut_;
  if (P.sine > 0) {
    // Full-wave rectified ramp: a triangle (its corners only bend), then a
    // differential pair (tanh) rounds it.
    const double tri = 2 * std::fabs(v_ - mid) / half - 1;
    m += P.sine * half * std::tanh(1.6 * tri) / std::tanh(1.6);
  }
  if (P.sub > 0) m += P.sub * half * ff1Out_;
  if (P.sub2 > 0) m += P.sub2 * half * ff2Out_;
  if (P.ring > 0) m += P.ring * r * ff1Out_;
  if (P.second > 0) m += P.second * (out2_ - mid);
  if (P.scat > 0) {
    m *= srcGain_;
    if (burst_ > 1e-3) {  // the stop's release: a few ms of noise
      m += half * 0.8 * burst_ * whiteNoise();
      burst_ *= std::exp(-1 / (0.004 * fs_));
    }
  }
  if (P.breath > 0) {
    // A reverse-biased junction's noise, band-passed (~0.5 .. 5 kHz), into a
    // delay-line comb at the note's period: the pipe's resonances.
    const double wn = whiteNoise();
    brLp_ += 0.45 * (wn - brLp_);
    brHp_ += 0.06 * (brLp_ - brHp_);
    double b = brLp_ - brHp_;
    if (P.comb > 0) {
      const int n = int(comb_.size());
      const double rp = combPos_ - combLen_;
      const int i0 = int(std::floor(rp));
      const double fr = rp - i0;
      const double y0 = comb_[(i0 + n) % n], y1 = comb_[(i0 + 1 + n) % n];
      const double fbv = y0 + fr * (y1 - y0);
      const double c = b + P.comb * fbv;
      comb_[combPos_] = float(c);
      combPos_ = (combPos_ + 1) % n;
      b = (1 - P.comb) * c * 2.5;
    }
    m += P.breath * half * 0.5 * b * std::max(level_, 0.0) / std::max(ctl_.level, 0.05);
  }
  if (P.formants > 0) {
    // Four band-passes in parallel (trapezoidal state-variable), levels
    // falling with the formant; a little of the source through.
    const double* gain = fmGain_;
    double f = 0.4 * m;
    for (int k = 0; k < 4; ++k) {
      const double g = fmG_[k], kk = fmK_[k];
      const double a1 = 1 / (1 + g * (g + kk)), a2 = g * a1;
      const double v1 = a1 * fmA_[k] + a2 * (m - fmB_[k]);
      const double v2 = fmB_[k] + g * v1;
      fmA_[k] = 2 * v1 - fmA_[k];
      fmB_[k] = 2 * v2 - fmB_[k];
      f += gain[k] * kk * 2.5 * v1;
    }
    m = f;
  }
  if (P.drive > 0) {
    // The overdrive: a preamp's gain into an asymmetric diode pair (a soft
    // knee one way, harder the other), then the mid scoop.
    const double x = (1 + 7 * P.drive) * m / half;
    const double y = x > 0 ? std::tanh(x + 0.2) - std::tanh(0.2) : x / std::pow(1 + std::pow(-x, 2.5), 0.4);
    m = 0.8 * half * y;
    if (P.notch > 0) {
      const double in = m;
      const double o = nb_[0] * in + nz_[0];
      nz_[0] = nb_[1] * in - nb_[3] * o + nz_[1];
      nz_[1] = nb_[2] * in - nb_[4] * o;
      m = in + P.notch * 0.6 * (o - in);
    }
  }
  if (P.fold > 0) m = half * std::sin(P.fold * 1.4 * m / half);
  return m;
}

// After the ladder, volts: the chiff's pinged resonator, the chorus.
double Lead::post(double y) {
  const LeadPatch& P = *patch_;
  if (P.chiff > 0 && chiffEnv_ > 1e-4) {
    const double g = chiffG_, k = 1 / 6.0;
    const double a1 = 1 / (1 + g * (g + k)), a2 = g * a1;
    const double in = chiffEnv_ * whiteNoise();
    const double v1 = a1 * chiffA_ + a2 * (in - chiffB_);
    const double v2 = chiffB_ + g * v1;
    chiffA_ = 2 * v1 - chiffA_;
    chiffB_ = 2 * v2 - chiffB_;
    y += P.chiff * 1.2 * v1;
    chiffEnv_ *= std::exp(-1 / (0.025 * fs_));
  }
  if (P.chorus > 0) {
    // A bucket-brigade line, two taps swept by their own LFOs (0.6, 0.83 Hz)
    // around 7 and 11 ms, its clock's low-pass; mixed with the dry.
    const int n = int(bbd_.size());
    bbdLp_ += 0.5 * (y - bbdLp_);
    bbd_[bbdPos_] = float(bbdLp_);
    chPh1_ += 0.6 / fs_;
    chPh2_ += 0.83 / fs_;
    if (chPh1_ > 1) chPh1_ -= 1;
    if (chPh2_ > 1) chPh2_ -= 1;
    auto tap = [&](double ms) {
      const double rp = bbdPos_ - ms * 1e-3 * fs_;
      const int i0 = int(std::floor(rp));
      const double fr = rp - i0;
      const double a = bbd_[(i0 + 2 * n) % n], b = bbd_[(i0 + 1 + 2 * n) % n];
      return a + fr * (b - a);
    };
    const double w = tap(7 + 1.5 * std::sin(2 * M_PI * chPh1_)) + tap(11 + 1.8 * std::sin(2 * M_PI * chPh2_));
    bbdPos_ = (bbdPos_ + 1) % n;
    y = 0.7 * y + P.chorus * 0.45 * w;
  }
  return y;
}

float Lead::sample() {
  double saw;
  coreStep(saw);
  // Into the ladder: the ramp around its middle, attenuated to tens of mV (a
  // transistor ladder's input range: its tanh at 2 VT).
  const double mid = 0.5 * (vhi_ + vlo_), kIn = 0.035;
  const double* vo = vos_;
  const double ka = kAnalog_;
  const double x = (simple_ ? (saw - mid) : sound(saw)) * kIn + testDp_;  // (tests: a drive into the ladder)
  testDp_ = 0;
  for (int k = 0; k < 2; ++k) {
    const double fb = 0.5 * (y_[3] + y4Prev_);
    y4Prev_ = y_[3];
    // (Each differential pair with its offset: its tanh not centred: even harmonics.)
    const double in = std::tanh((x - 4 * res_ * fb + ka * vo[0]) / vt2_);
    y_[0] += vt2_ * g_ * (in - w_[0]);
    w_[0] = std::tanh((y_[0] + ka * vo[1]) / vt2_);
    y_[1] += vt2_ * g_ * (w_[0] - w_[1]);
    w_[1] = std::tanh((y_[1] + ka * vo[2]) / vt2_);
    y_[2] += vt2_ * g_ * (w_[1] - w_[2]);
    w_[2] = std::tanh((y_[2] + ka * vo[3]) / vt2_);
    y_[3] += vt2_ * g_ * (w_[2] - w_[3]);
    w_[3] = std::tanh((y_[3] + ka * vo[4]) / vt2_);
  }
  // The amplifier: an OTA, gain from its bias current (the envelope), a soft
  // limit on its input.
  level_ += 0.02 * (ctl_.level - level_);
  // Volts again, with the passband's loss made up in part: the ladder's
  // gain under the cutoff is 1 / (1 + 4r), -14 dB at r = 1; the output stage
  // gives back (1 + 2r) of it (as later ladder designs do).
  double y = y_[3] / kIn * (1 + 2 * res_);
  if (!simple_) y = post(y);
  // The OTA: its gain from the envelope's bias current; when closed it still
  // leaks the always-running oscillator; its control feedthrough steps with
  // the bias (the DC blocker makes it a thump); hum picked up on the way out.
  const double vca = level_ * 2.0 * std::tanh(0.5 * y) + ka * (bleed_ * y + ft_ * level_) + board().hum;
  // Into the mixer's 1 kohm: the ground's current. (A NaN stays in this voice:
  // it resets itself; the board must not keep it.)
  board().output(id_, std::isfinite(vca) ? vca * 1e-3 : 0.0);
  return finish(float(0.3 * vca));
}

// ---------------------------------------------------------------------------

// Each key's own formant offset, -1..1: random per key (splitmix64), fixed,
// centred over the keyboard so the spread does not brighten or darken on
// average.
static double keyFormant(int key) {
  static const auto table = [] {
    std::array<double, 128> t{};
    double mean = 0;
    for (int k = 0; k < 128; ++k) {
      uint64_t z = uint64_t(k) + 0x9E3779B97F4A7C15ull;
      z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
      z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
      z ^= z >> 31;
      t[k] = (z >> 40) * (2.0 / 16777216.0) - 1;
      mean += t[k] / 128;
    }
    for (double& v : t) v = std::clamp(v - mean, -1.0, 1.0);
    return t;
  }();
  return table[std::clamp(key, 0, 127)];
}

LeadPlayer::LeadPlayer(Lead& l) : Performer(l), lead_(l) {}

void LeadPlayer::seed(unsigned s) {
  rng_ = 4242u + 7907u * s;
  if (s == 0) return;
  for (int i = 0; i < 4; ++i) urand();
}

void LeadPlayer::setHold(bool h) {
  hold_ = h;
  if (!h && stackTop_ == 0) gate(false, false);
}

void LeadPlayer::gate(bool on, bool retrigger) {
  if (on && (retrigger || !gate_)) {
    amp_.attack = flt_.attack = true;
    sinceOnClock_ = 0;
    ++strike_;
    // Each attacked note its own vibrato: deeper or shallower, and its own
    // rotation speed, uniform within 15 % of the setting (a slurred note
    // keeps its predecessor's). The LFO's slow phase noise comes on top.
    noteVibDepth_ = 1 + 0.35 * urand();
    static const double kSpread = knob("PJ_VIBSPREAD", 0.15);
    noteVibRate_ = 1 + kSpread * urand();
    // ... and its own onset, a random path: plain at first, the vibrato
    // coming in after ~0.35 s over ~0.6 s, its depth's curve steeper or
    // gentler each time. Most notes (~90 %) swing at their own rate from the
    // start; now and then (~10 %) one starts slower (55-80 % of it) and
    // speeds up.
    noteVibDelay_ = 0.35 * (1 + 0.3 * urand());
    noteVibFade_ = 0.6 * (1 + 0.3 * urand());
    noteVibCurve_ = std::exp2(0.6 * urand());  // the depth curve's power, 0.66..1.5
    noteVibStart_ = urand() < -0.8 ? 0.675 + 0.125 * urand() : 1.0;
  }
  gate_ = on;
  blowing_ = on;
}

void LeadPlayer::noteOn(int midi, int velocity) {
  if (!info(midi)) return;
  held_[midi] = 1;
  if (stackTop_ < 16) stack_[stackTop_++] = midi;
  const bool legato = note_ >= 0 && gate_ && (stackTop_ > 1 || hold_);
  const Articulation art = articulate(midi, velocity, legato);
  takeHoleGlide();
  note_ = midi;
  keyVel_ = velocity;
  // Legato: the envelopes go on (a slur), or start again from where they are
  // (tongued legato: a harder key re-triggers); a glide slides the CV.
  glideTau_ = art == kArtGlide ? std::max(0.02, glideDur_ / 2.5) : 0;
  if (art == kArtSlur || art == kArtGlide) {
    gate(true, false);
  } else {
    velAmt_ = velocity / 127.0;
    gate(true, true);
  }
}

void LeadPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  held_[midi] = 0;
  int w = 0;
  for (int r = 0; r < stackTop_; ++r)
    if (stack_[r] != midi) stack_[w++] = stack_[r];
  stackTop_ = w;
  if (midi != note_) return;
  if (stackTop_ > 0) {  // back to the last key held: legato
    note_ = stack_[stackTop_ - 1];
    glideTau_ = 0;
  } else if (!hold_) {
    gate(false, false);
    released();
  }
}

void LeadPlayer::Env::step(bool g, double a, double d, double s, double r, double dt) {
  if (!g) {
    attack = false;
    v += (1 - std::exp(-dt / r)) * (0 - v);
    return;
  }
  if (attack) {
    v += (1 - std::exp(-dt / a)) * (1.3 - v);
    if (v >= 1) { v = 1; attack = false; }
  } else {
    v += (1 - std::exp(-dt / d)) * (s - v);
  }
}

void LeadPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    tick(bdt);
    sinceOnClock_ += bdt;
    if (note_ >= 0) {
      // The ensemble's tuning (just intonation): taken at once on a note's
      // first 5 ms (the ensemble sets it a block after the key), then slewed
      // over ~10 ms (a chord re-placed as notes join it), never a jump.
      const double tune = ear_ ? earCents_ : 0;
      if (sinceOnClock_ < 0.005) tuneNow_ = tune;
      else tuneNow_ += (1 - std::exp(-bdt / 0.010)) * (tune - tuneNow_);
      const double target = note_ + tuneNow_ / 100 + 2 * roll_;
      if (glideTau_ > 0) cv_ += (1 - std::exp(-bdt / glideTau_)) * (target - cv_);
      else cv_ = target;
    }
    // Envelopes: hardness shortens the attack; the release knob sets the release.
    const double hard = std::max(0.3, hardness_);
    const LeadPatch& P = lead_.patch();
    amp_.step(gate_, P.ampA / hard, P.ampD, P.ampS, std::max(0.01, 5 * release_ * P.relMul), bdt);
    flt_.step(gate_, P.fltA / hard, P.fltD, P.fltS, std::max(0.01, 5 * release_ * P.relMul), bdt);
    // Vibrato: the voice's own LFO on the CV (in the circuit). None at the
    // attack; after the note's delay it comes in along the note's own path:
    // the depth rising (a smoothstep to a random power), the rate usually
    // steady, sometimes rising from slower (over 1.5x the depth's fade).
    const double dyn = std::clamp(dyn_ + breathCtl_, 0.0, 1.0);
    Lead::Controls c;
    c.note = cv_;
    if (P.pitchEnv != 0)  // a drum: the skin's pitch falls from the strike
      c.note += P.pitchEnv * (0.4 + 0.6 * velAmt_) * std::exp(-sinceOnClock_ / P.pitchTau);
    {
      auto smooth = [](double x) { x = std::clamp(x, 0.0, 1.0); return x * x * (3 - 2 * x); };
      const double t = sinceOnClock_ - noteVibDelay_;
      c.vibDepth = 2 * vibDepth_ * noteVibDepth_ * std::pow(smooth(t / noteVibFade_), noteVibCurve_);
      c.vibRate = vibRate_ * noteVibRate_ *
                  (noteVibStart_ + (1 - noteVibStart_) * smooth(t / (1.5 * noteVibFade_)));
    }
    // The filter, on two knobs of its own (the dynamics knob is the level
    // only):
    //   - cutoff (K2, CC71): from 2 octaves under the note to 5 over it (to
    //     ~19 kHz), an octave over at the knob's rest (brightness 1); half
    //     keyboard tracking; the envelope opens it up to two octaves more
    //     at a full-velocity key (~ velocity^2, rising over 6 ms: PJ_FLTENV);
    //   - resonance (K3, CC72): from none to the edge of self-oscillation
    //     (1.0; a little past it at the end: it sings), the knob's travel
    //     weighted towards the narrow end.
    {
      const double x = std::clamp((brightness_ - 0.3) / 2.2, 0.0, 1.0), x0 = (1 - 0.3) / 2.2;
      const double knobSemis = x < x0 ? -24 + 42 * x / x0 : 18 + 42 * (x - x0) / (1 - x0);
      // Each key its own formant: a fixed offset of the cutoff per key (a
      // hash of the key: the same key always the same, neighbours unlike),
      // within +-12 semitones x the spread knob; taken at the attack, slid
      // (~15 ms) on a slur or glide.
      if (note_ >= 0) {
        const double target = 12 * formantSpread_ * keyFormant(note_);
        if (sinceOnClock_ < 0.005) formantNow_ = target;
        else formantNow_ += (1 - std::exp(-bdt / 0.015)) * (target - formantNow_);
      }
      c.cutoff = 69 + 0.5 * (cv_ - 69) + knobSemis + formantNow_ + P.cutoff +
                 kFltEnv_ * P.fltEnv * flt_.v * velAmt_ * velAmt_;
      // The resonance: the knob, and the key's speed: a fast key rings more
      // (the knob's reading moved by up to +-0.2 around a mezzo key).
      c.res = resonance(air_ + 0.4 * (velAmt_ - 0.5)) * P.resScale;
    }
    c.level = amp_.v * (0.4 + 0.6 * dyn) * (0.6 + 0.4 * velAmt_);
    c.strike = strike_;
    c.vel = velAmt_;
    lead_.setControls(c);
    lead_.process(out + i, m);
  }
}

}  // namespace pajupilli
