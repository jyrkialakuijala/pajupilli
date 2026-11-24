// The choir (General MIDI 53 "Choir Aahs"): the parts, the singers and the dealing of the keys.
// See src/choir.h.
#include "choir.h"
#include "heard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "band.h"
#include "ensemble.h"
#include "knob.h"

namespace pajupilli {

const ChoirPart kChoirPart[kChoirParts] = {
    // name, range, at ease, divisi, tract (cm), female, breath, vibrato x
    {"bass", 31, 64, 40, 57, 3, 17.8, false, 0.15, 0.7},  // (G1 .. D2: the oktavists, initChoir)
    {"baritone", 45, 69, 45, 62, 3, 17.3, false, 0.17, 0.8},
    {"tenor", 48, 72, 50, 67, 3, 16.8, false, 0.20, 0.9},
    {"alto", 53, 77, 55, 72, 3, 15.5, true, 0.30, 1.0},
    {"mezzo-soprano", 57, 81, 59, 76, 3, 15.0, true, 0.32, 1.1},
    {"soprano", 60, 84, 64, 81, 3, 14.5, true, 0.35, 1.2},
};

namespace {

// The variants (see choir.h). The first is the default.
const ChoirVariant kChoirVariants[] = {
    // 53 Choir Aahs: every part on a dark choral [ɑ], a moderate attack (the section full in ~0.2 s).
    {"choir", {3, 3, 3, 3, 3, 3}, -1, 0.07, 0.12, 0, 1, 1, 1, 0x3f, 128, 0, 0, 0},
    // 54 Voice Oohs: the women lead (the men only below C4) on a rounded [u] (sopranos, mezzos) ..
    // [ʊ] (basses), the women's top opening toward [ɔ] (D5 .. G5); softer (lower pressures),
    // breathier, less vibrato, a soft "d" on firmer keys (one maker's "dooh"). The lungs rise with
    // tau ~75 ms at mf (0.055 x 0.92 x the softness's 1.5): a soft, coordinated onset builds its
    // pressure in 50-150 ms (research/choir.md 5.1); with the entries' scatter and the room the
    // section is at half level in ~0.1 s, within 2-3 dB of full in ~0.2 s. (At 0.14, tau 0.19 s,
    // it was full only after ~0.5 s: an attack of 774 ms against the SoundFonts' 40-80,
    // research/gm_compare.md p053; rhythmic "ooh" parts dragged.)
    {"voice-oohs", {0.7, 0.6, 0.5, 0.3, 0.2, 0.2}, 2, 0.055, 0.2, 0.5, 1.3, 0.8, 1.1, 0x3f, 60, 0.4, 0, 0},
};

const NoteInfo kChoirNotes[128] = {
#define N {"", 1}
#define N8 N, N, N, N, N, N, N, N
    N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8
#undef N8
#undef N
};
const Design kChoirDesign = {nullptr, 0, nullptr, 0, kChoirNotes, 0, 0, false, nullptr, 0, {}, {}};

// The parts' singers as instruments (one per part: the seats' Voices).
const Instrument kChoirSeat[kChoirParts] = {
    {"choir-bass", 0, 31, 64, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kChoirDesign, "choir-singer"},
    {"choir-baritone", 0, 45, 69, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kChoirDesign, "choir-singer"},
    {"choir-tenor", 0, 48, 72, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kChoirDesign, "choir-singer"},
    {"choir-alto", 0, 53, 77, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kChoirDesign, "choir-singer"},
    {"choir-mezzo", 0, 57, 81, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kChoirDesign, "choir-singer"},
    {"choir-soprano", 0, 60, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kChoirDesign, "choir-singer"},
};

// The keys evened (dB, by the note sung, E2 .. C6 every 4 semitones): a lone key's choir measured
// (K-weighted, -note) rose 35 dB from E2 to C6 (the lungs' 1.4 x an octave, the radiation's
// 6 dB an octave, the sopranos' tuned R1, fewer singers low); a sampled GM choir is level within
// a few dB. Each singer's level is trimmed by its note to leave a gentle rise (0.15 dB a
// semitone). PJ_CHOIR_EVEN: the share of it (0: the physics' own levels).
double keyTrimDb(double note) {
  static const double kTrim[12] = {15.9, 10.3, 7.1, 3.9, 1.7, -1.1, -1.6, -0.8, -2.3, -2.2, -7.8, -12.0};
  static const double kEven = knob("PJ_CHOIR_EVEN", 1.0);
  const double x = std::clamp((note - 40) / 4, 0.0, 11.0);
  const int i = std::min(10, int(x));
  return kEven * (kTrim[i] + (x - i) * (kTrim[i + 1] - kTrim[i]));
}

double smooth(double x, double x0, double x1) {
  const double t = std::clamp((x - x0) / (x1 - x0), 0.0, 1.0);
  return t * t * (3 - 2 * t);
}

}  // namespace

// The choirs as instruments: C2 .. C7 (sounding; folded by octaves into E2 .. C6).
extern const Instrument kChoirInstruments[] = {
    {"choir", 0, 31, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kChoirDesign, "choir"},
    {"voice-oohs", 0, 31, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kChoirDesign, "choir"},
};
extern const int kNumChoirInstruments = int(sizeof kChoirInstruments / sizeof kChoirInstruments[0]);

const ChoirVariant& choirVariant(const Instrument& inst) {
  for (const ChoirVariant& v : kChoirVariants)
    if (std::strcmp(v.name, inst.name) == 0) return v;
  return kChoirVariants[0];
}
bool choirInstrument(const Instrument& inst) { return std::strcmp(inst.family, "choir") == 0; }
bool choirSinger(const Instrument& inst) { return std::strcmp(inst.family, "choir-singer") == 0; }

// ---- One singer ----

SingerModel::SingerModel(const Instrument& inst) : Tube(inst) {
  for (int p = 0; p < kChoirParts; ++p)
    if (&inst == &kChoirSeat[p]) part_ = p;
  g_.init(fs_, 0, 0);
  hum_.init(fs_);
}

void SingerModel::wake() {
  if (!asleep_) return;
  g_.reset();
  hum_.reset();
  asleep_ = false;
  quiet_ = 0;
}

void SingerModel::resetAir() {
  Tube::resetAir();
  g_.reset();
  hum_.reset();
}

void SingerModel::controlStep() {
  if (asleep_) return;
  const double dt = 16 / fs_;
  g_.control(c_.f0, c_.vowel, c_.lung, c_.breath, c_.vibDepth, c_.vibRate, 60, dt);
  if (c_.hum > 0 || hum_.amount() > 0) hum_.control(c_.hum, dt);
}

float SingerModel::sample() {
  if (asleep_) return 0;
  static const double kGain = knob("PJ_CHOIR_SINGER", 0.2);
  static const int kOnly = int(knob("PJ_CHOIR_ONLY", -1));
  double y = g_.sample();
  if (c_.hum > 0 || hum_.amount() > 0) y = hum_.process(float(y));
  // (asleep once its lungs are empty and its sound has died away for 0.1 s)
  if (c_.lung <= 0 && std::fabs(y) * c_.gain * kGain < 1e-6) {
    if (++quiet_ > 4800) asleep_ = true;
  } else {
    quiet_ = 0;
  }
  last_ = float(y);
  if (kOnly >= 0 && seat != kOnly) y = 0;
  return finish(float(kGain * c_.gain * y));
}

SingerPlayer::SingerPlayer(SingerModel& m)
    : Performer(m), m_(m), var_(&kChoirVariants[0]), part_(kChoirPart[m.part()]) {
  vary(0);
}

// This singer's own (research/choir.md 11.2), fixed for its seat.
void SingerPlayer::vary(unsigned seat) {
  rng_ = 2654435761u * (seat + 1) + 40503u * unsigned(m_.part()) + 17;
  for (int i = 0; i < 4; ++i) rnd();
  auto trunc = [&](double sd, double lim) { return std::clamp(sd * gauss(), -lim, lim); };
  Singer::Voicing v;
  v.lengthCm = part_.lengthCm * (1 + trunc(0.04, 0.08));
  v.female = part_.female;
  // (its own vowel: R1 +-8 %, R2 +-6 % (sd) on the Singer's line)
  v.hump = trunc(0.015, 0.04);
  v.humpArea = std::exp(trunc(0.10, 0.25));
  v.lip = std::exp(trunc(0.10, 0.25));
  v.mouth = std::exp(trunc(0.08, 0.2));
  v.pharynx = std::exp(trunc(0.08, 0.2));
  v.cents = trunc(6, 15);
  v.jitter = 1;
  v.shimmer = 0.5;
  m_.seat = int(seat);
  m_.singer().init(fs_, seat, int(seat));  // (its own noise)
  m_.singer().setVoicing(v);
  vibRate_ = 5.3 + trunc(0.35, 0.9);
  vibExt_ = std::clamp(35 + 12 * gauss(), 10.0, 70.0) * part_.vibScale;
  vibDelay_ = std::clamp(0.3 + 0.1 * gauss(), 0.1, 0.6);
  levelDb_ = trunc(2.5, 6);
  strong_ = rnd() < 1.0 / 6;
  breath_ = std::clamp(part_.breath + 0.1 * (2 * rnd() - 1), 0.05, 0.6);
  vowelSeat_ = trunc(0.08, 0.2);  // (its own colour of the vowel, on top of its tract's)
}

bool SingerPlayer::setKnob(int k, double x) {
  if (k == 1) { knobVowel_ = 5 * std::clamp(x, 0.0, 1.0); return true; }
  if (k == 2) { knobOpen_ = std::clamp(x, 0.0, 1.0); return true; }
  return false;
}

void SingerPlayer::noteOn(int midi, int velocity) {
  const bool slur = cur_ >= 0 && !releasing_ && lung_ > 1;
  cur_ = midi;
  vel_ = velocity;
  releasing_ = false;
  noteCents_ = std::clamp(4 * gauss(), -10.0, 10.0);
  jawJit_ = 0.04 * gauss();
  blowing_ = true;
  if (slur) return;  // (the pitch glides there: render)
  m_.wake();
  noteNow_ = midi;
  startOff_ = std::clamp(-15 + 15 * gauss(), -50.0, 10.0);
  t_ = 0;
  breathing_ = false;
  nextBreath_ = 8 + 7 * rnd();
  onsetAir_ = 0.25 + 0.3 * var_->softness;
  // (a soft "d": the tongue on the ridge, released ~20-30 ms in)
  if (var_->consonant > 0 && var_->consonant * (0.4 + 0.6 * velocity / 127.0) > 0.25) {
    m_.singer().setConstriction(0.85, 0.01);
    dClock_ = 0;
  }
}

void SingerPlayer::noteOff(int midi) {
  if (midi == cur_ && !hold_) releasing_ = true;
}

void SingerPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  const ChoirVariant& V = *var_;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double dt = m / fs_;
    tick(dt);
    t_ += dt;
    if (dClock_ < 1e8 && (dClock_ += dt) > 0.022) {
      m_.singer().setConstriction(0.85, 0);
      dClock_ = 1e9;
    }
    // Staggered breathing in a long note: out for half a second or so, every 8-15 s.
    if (cur_ >= 0 && !releasing_) {
      if (!breathing_ && t_ > nextBreath_) {
        breathing_ = true;
        breathEnd_ = t_ + 0.45 + 0.4 * rnd();
      } else if (breathing_ && t_ > breathEnd_) {
        breathing_ = false;
        nextBreath_ = t_ + 8 + 7 * rnd();
      }
    }
    // The lungs: velocity, dynamics and expression on a log scale (research/choir.md 5.3), up
    // 1.4 x an octave above the part's middle.
    const double note = cur_ >= 0 ? cur_ : noteNow_;
    static const double kVelExp = knob("PJ_CHOIR_VELEXP", 1.7);
    const double L = std::clamp(std::pow(vel_ / 127.0, kVelExp) + 0.6 * (dyn_ - 0.5) + breathCtl_, 0.03, 1.15);
    const double mid = 0.5 * (std::max(part_.lo, 40) + part_.hi);  // (the bass as from E2: the oktavists' depths aside)
    double press = 400 * std::pow(3500.0 / 400, L) * (1 - 0.3 * V.softness);
    press *= std::clamp(std::pow(1.4, (note - mid) / 12), 0.6, 2.0);
    press = std::min(press, 5000.0);
    const bool on = cur_ >= 0 && !releasing_ && !breathing_;
    const double target = on ? press : 0;
    double tau;
    if (target > lung_) tau = V.attack * (1.6 - 0.9 * vel_ / 127.0) * (1 + V.softness);
    else tau = breathing_ ? 0.05 : V.release;
    lung_ += (1 - std::exp(-dt / tau)) * (target - lung_);
    if (!on && lung_ < 2) {
      lung_ = target > 0 ? lung_ : 0;
      if (releasing_ && lung_ == 0) { cur_ = -1; releasing_ = false; }
    }
    blowing_ = cur_ >= 0 || lung_ > 0;
    // The knobs, glided (~40 ms: no click as they turn).
    const double kg = 1 - std::exp(-dt / 0.04);
    const double vowelTo = knobVowel_ >= 0 ? knobVowel_ : V.vowel[m_.part()];
    vowelNow_ = vowelNow_ < 0 ? vowelTo : vowelNow_ + kg * (vowelTo - vowelNow_);
    openNow_ += kg * (knobOpen_ - openNow_);
    const double openJaw = openNow_ > 0.5 ? 1.2 * (openNow_ - 0.5) : -0.6 * (0.5 - openNow_);
    const double openAir = openNow_ < 0.5 ? 0.6 * (0.5 - openNow_) : -0.2 * (openNow_ - 0.5);
    // The breath: the part's and the seat's chink, more at the onset (it fades in ~60 ms) and as
    // the voice lets go.
    onsetAir_ *= std::exp(-dt / 0.06);
    const double breath = std::clamp(breath_ * V.breath + openAir + onsetAir_ + (releasing_ || breathing_ ? 0.25 : 0) +
                                         0.3 * (air_ - 0.35), 0.0, 1.0);
    // The pitch: a slur glides (~80 ms), a fresh start comes in a little flat (60 ms).
    if (cur_ >= 0) noteNow_ += (1 - std::exp(-dt / 0.035)) * (cur_ - noteNow_);
    startOff_ *= std::exp(-dt / 0.06);
    // The chord's tuning (Ensemble::choirTune): heard and followed in ~80 ms.
    tuneNow_ += (1 - std::exp(-dt / 0.08)) * ((ear_ ? earCents_ : 0) - tuneNow_);
    const double cents = noteCents_ + startOff_ + tuneNow_ + 200 * roll_;
    const double f0 = 440 * std::exp2((noteNow_ - 69) / 12 + cents / 1200);
    // The vibrato: after its delay, in over ~0.35 s.
    const double vib = vibExt_ / 100 * V.vibrato * smooth(t_, vibDelay_, vibDelay_ + 0.35);
    // The vowel: the part's; the women's top opening toward the variant's high vowel.
    double vowel = std::clamp(vowelNow_ + vowelSeat_, 0.0, 5.0);
    if (part_.female && V.highVowel >= 0) vowel += (V.highVowel - vowel) * smooth(noteNow_, 74, 79);
    // Resonance tuning: the women keep R1 just above f0 once f0 nears it; the men nudge R1 onto
    // 2 f0 when it is near.
    const double f0n = 440 * std::exp2((noteNow_ - 69) / 12);
    double jaw = 0;
    if (part_.female) {
      jaw = std::max(0.0, m_.singer().jawFor(vowel, f0n + 20));
    } else {
      const double j2 = m_.singer().jawFor(vowel, 2 * f0n + 10);
      if (std::fabs(j2) < 0.2) jaw = j2 * (1 - std::fabs(j2) / 0.2);
    }
    m_.singer().setJaw(jaw + jawJit_ + openJaw);
    // The level: the seat's; a strong voice +4 dB at f and ff.
    const double gain = std::pow(10.0, (levelDb_ + (strong_ ? 4 * smooth(L, 0.7, 0.9) : 0) + V.gainDb + keyTrimDb(noteNow_)) / 20);
    SingerModel::Controls c;
    c.f0 = f0;
    c.vowel = vowel;
    c.lung = lung_;
    c.breath = breath;
    c.vibDepth = vib;
    c.vibRate = vibRate_;
    c.gain = gain;
    c.hum = V.hum;
    m_.setControls(c);
    m_.process(out + i, m);
  }
}

// ---- The choir as one voice ----

ChoirModel::ChoirModel(const Instrument& inst) : Tube(inst) {
  static const Calibration cal;
  ens_.reset(new Ensemble(inst, cal, 1, false, nullptr));
}
ChoirModel::~ChoirModel() = default;

void ChoirPlayer::noteOn(int midi, int velocity) {
  m_.ensemble().noteOn(midi, velocity);
  blowing_ = true;
}
void ChoirPlayer::noteOff(int midi) { m_.ensemble().noteOff(midi); }
void ChoirPlayer::setHold(bool h) { m_.ensemble().setHold(h); }
bool ChoirPlayer::setKnob(int k, double x) { return m_.ensemble().instrumentKnob(k, x); }

void ChoirPlayer::render(float* out, int n) {
  Ensemble& e = m_.ensemble();
  e.all(&Performer::setDynamics, dyn_);
  e.all(&Performer::setBreath, breathCtl_);
  e.all(&Performer::setLipRoll, roll_);
  e.all(&Performer::setAir, air_);
  e.render(out, n);
  // (the Voice applies the choir's level trim: the singers' ensemble does too)
  const float g = float(1 / levelTrim(inst_));
  for (int i = 0; i < n; ++i) out[i] *= g;
}

// ---- The choir's dealing (Ensemble) ----

struct ChoirState {
  const ChoirVariant* var;
  struct Seat {
    int slot, part;
    int cur = -1, want = -1;  // the key it sings; the key it is to sing (-1: none)
    double at = 0;            // ... from when
    int vel = 64;
    double habit;             // its own lateness (s)
    int lo, hi;               // its own reach
    double x = 0;             // where it stands across the stage (m, from the audience: - left)
  };
  std::vector<Seat> seats;
  int held[128] = {};
  double tune[128] = {};      // each sung note's tuning (c; choirTune)
  bool sounding[128] = {};    // ... the notes it tuned last
  double since[128] = {};     // ... since when (s)
  // The band's pull on each sung note (choirEar): where it is, where it is going, the weight it was
  // heard with, when it was last heard, the band's version then, its share that is other ears',
  // what it was at the last solve; whether the note is being heard.
  double band[128] = {}, bandTo[128] = {}, bandMix[128] = {}, bandAt[128] = {}, share[128] = {}, solved[128] = {};
  unsigned bandVer[128] = {};
  int role = -1;  // (one voice part only: setChoirRole)
  bool heard[128] = {};
  unsigned rng = 2027;
  float gain = 1;
  double rnd() { rng = rng * 1664525u + 1013904223u; return (rng >> 8) / 16777216.0; }
  double gauss() { return (rnd() + rnd() + rnd() + rnd() - 2) * 1.73; }
};

void Ensemble::initChoir(const Instrument& inst, const RoomSpec* room, int role, int index, int count) {
  // (a section alone: 24 singers shared over the sections, 4 .. 8 each)
  const int kPer = role < 0 ? std::max(1, int(knob("PJ_CHOIR_N", 4)))
                            : std::clamp(int(knob("PJ_CHOIR_SECTION_N", std::clamp(24 / std::max(1, count), 4, 8))), 1, 16);
  lo_ = inst.lo;
  hi_ = inst.hi;
  choirInst_ = &inst;
  choirRoom_ = room;
  choir_ = std::make_shared<ChoirState>();
  ChoirState& C = *choir_;
  C.var = &choirVariant(inst);
  C.role = role;
  kids_.clear();
  kidCals_.clear();
  kidCals_.emplace_back(new Calibration());
  std::vector<Member> members;
  // Each seat's reach: the part's range less a few semitones at either end (one singer of the part
  // reaches the whole range, the others fall short by 1 .. 4 semitones at the top, 0 .. 3 at the bottom).
  static const int kTopShort[8] = {0, 3, 1, 4, 2, 3, 1, 2}, kLowShort[8] = {0, 2, 1, 3, 0, 2, 1, 3};
  // (the basses: two in four oktavists, down to G1; the others from D2 .. E2)
  static const int kBassLow[8] = {31, 38, 31, 40, 33, 39, 34, 41};
  // Where they stand (from the audience): the usual four blocks on the risers behind the
  // orchestra, two rows, each a continuous gradient of pitch from left to right (SSAA in front,
  // TTBB behind): the first sopranos on the far left (with the first violins: the treble projects
  // most there), the second sopranos (mezzos), then the altos, the second altos anchoring the far
  // right; behind, the tenors left (behind the sopranos), the baritones (first basses), the
  // basses far right (with the cellos and double basses). A row spans the string section's width
  // (24 players in 10.35 m: src/ensemble.cc), its singers evenly apart; within a part the higher
  // reaching ones on the left (the oktavists far right), and a divided part's upper line to its
  // left half (where the held keys are dealt, below). PJ_CHOIR_WIDTH scales it (0: all in the middle). (The rows' depth is
  // not heard: the room places its sources across the stage only.)
  static const int kRowOrder[kChoirParts] = {2, 1, 0, 2, 1, 0};  // bass .. soprano: its place in its row
  static const double width = knob("PJ_CHOIR_WIDTH", 1.0);
  const double gap = 10.35 / std::max(1, 3 * kPer - 1);
  for (int p = 0; p < kChoirParts; ++p) {
    if (role >= 0 ? p != role : !(C.var->parts >> p & 1)) continue;
    // (its singers by reach, the highest first: the left of the block)
    std::vector<int> byReach(kPer);
    for (int i = 0; i < kPer; ++i) byReach[i] = i;
    auto reach = [&](int i) {
      return p == kChoirBass ? kBassLow[i % 8] + kChoirPart[p].hi - kTopShort[i % 8]
                             : 2 * kChoirPart[p].lo + kLowShort[i % 8] - kTopShort[i % 8];
    };
    std::stable_sort(byReach.begin(), byReach.end(), [&](int a, int b) { return reach(a) > reach(b); });
    for (int i = 0; i < kPer; ++i) {
      ChoirState::Seat s;
      s.slot = int(members.size());
      s.part = p;
      s.habit = std::clamp(0.010 * C.gauss(), -0.02, 0.02);
      s.lo = p == kChoirBass ? kBassLow[i % 8] : kChoirPart[p].lo + kLowShort[i % 8];
      s.hi = kChoirPart[p].hi - kTopShort[i % 8];
      const int at = int(std::find(byReach.begin(), byReach.end(), i) - byReach.begin());
      // (a section alone: its block, the index-th of count across the risers' 10.35 m)
      s.x = role >= 0 ? width * 10.35 * ((index * kPer + at + 0.5) / (count * kPer) - 0.5)
                      : width * gap * (kRowOrder[p] * kPer + at - (3 * kPer - 1) / 2.0);
      C.seats.push_back(s);
      members.push_back(Member{&kChoirSeat[p], kidCals_.back().get(), s.x});
    }
  }
  Section s;
  s.ens.reset(new Ensemble(members, false, room));
  s.ens->quiet_ = true;
  s.lo = lo_;
  s.hi = hi_;
  s.gain = float(knob("PJ_CHOIR_GAIN", 0.25) * levelTrim(inst));
  s.stops = true;
  kids_.push_back(std::move(s));
  unsigned seat = 0;
  for (Slot& f : kids_[0].ens->f_) {
    if (auto* sp = dynamic_cast<SingerPlayer*>(&f.v->player())) {
      sp->setVariant(*C.var);
      sp->vary(++seat + (role >= 0 ? 64 * (index + 1) : 0));  // (each section its own singers)
    }
  }
}

// The held keys dealt to the parts and their singers (see choir.h).
void Ensemble::choirAssign() {
  ChoirState& C = *choir_;
  const ChoirVariant& V = *C.var;
  static const double kSpread = knob("PJ_CHOIR_SPREAD", 1.0);
  const double spread = kSpread * V.spread;
  // The keys, folded by octaves into the choir's range (E2 .. C6; a section alone: its own).
  int lo = 127, hi = 0;
  for (int p = 0; p < kChoirParts; ++p)
    if (C.role >= 0 ? p == C.role : V.parts >> p & 1) lo = std::min(lo, kChoirPart[p].lo), hi = std::max(hi, kChoirPart[p].hi);
  int vel[128] = {};
  for (int k = 0; k < 128; ++k)
    if (C.held[k] > 0) {
      int f = k;
      while (f < lo) f += 12;
      while (f > hi) f -= 12;
      vel[f] = std::max(vel[f], C.held[k]);
    }
  std::vector<int> keys;
  for (int k = lo; k <= hi; ++k)
    if (vel[k] > 0) keys.push_back(k);
  const int n = int(keys.size());
  auto allowed = [&](int p, int k) {
    const ChoirPart& d = kChoirPart[p];
    return (V.parts >> p & 1) && k >= d.lo && k <= d.hi && (d.female || k < V.menBelow);
  };
  auto easy = [&](int p, int k) { return allowed(p, k) && k >= kChoirPart[p].easyLo && k <= kChoirPart[p].easyHi; };
  int seatsOf[kChoirParts] = {};
  for (const ChoirState::Seat& s : C.seats) ++seatsOf[s.part];
  // Each part's keys (indices into keys).
  std::vector<int> pk[kChoirParts];
  if (C.role >= 0) {  // (a section alone sings what it is given, divided if it must be)
    for (int i = 0; i < n && i < std::min(kChoirPart[C.role].maxDiv, seatsOf[C.role]); ++i) pk[C.role].push_back(n - 1 - i);
    std::sort(pk[C.role].begin(), pk[C.role].end());
  } else if (n == 1) {
    bool any = false;
    for (int p = 0; p < kChoirParts; ++p)
      if (seatsOf[p] && easy(p, keys[0])) pk[p].push_back(0), any = true;
    if (!any)
      for (int p = 0; p < kChoirParts; ++p)
        if (seatsOf[p] && allowed(p, keys[0])) pk[p].push_back(0), any = true;
  } else if (n > 1) {
    std::vector<int> cand;
    for (int p = 0; p < kChoirParts; ++p) {
      if (!seatsOf[p]) continue;
      for (int k : keys)
        if (allowed(p, k)) { cand.push_back(p); break; }
    }
    const int nc = int(cand.size());
    auto cost = [&](int p, int ci, int i) {
      if (!allowed(p, keys[i])) return 1e9;
      const double tp = nc > 1 ? double(ci) / (nc - 1) : 0.5, si = double(i) / (n - 1);
      return std::fabs(si - tp) + (easy(p, keys[i]) ? 0 : 0.3);
    };
    std::vector<int> cover(n, 0);
    for (int ci = 0; ci < nc; ++ci) {
      int best = -1;
      double bc = 1e8;
      for (int i = 0; i < n; ++i) {
        const double c = cost(cand[ci], ci, i);
        if (c < bc) bc = c, best = i;
      }
      if (best >= 0) pk[cand[ci]].push_back(best), ++cover[best];
    }
    // The keys no part chose: the nearest part that can still divide takes each.
    for (int i = 0; i < n; ++i) {
      if (cover[i]) continue;
      int bp = -1;
      double bc = 1e8;
      for (int ci = 0; ci < nc; ++ci) {
        const int p = cand[ci], have = int(pk[p].size());
        if (have >= kChoirPart[p].maxDiv || have >= seatsOf[p]) continue;
        const double c = cost(p, ci, i) + 0.15 * have;
        if (c < bc) bc = c, bp = p;
      }
      if (bp >= 0) pk[bp].push_back(i), ++cover[i];
    }
    for (auto& v : pk) std::sort(v.begin(), v.end());
  }
  // Within each part: its singers shared evenly over its keys, as few moving as will do; a singer
  // does not take a key beyond its own reach unless no one else of the part can.
  auto enter = [&]() { return std::clamp(0.025 + 0.020 * C.gauss(), 0.0, 0.08); };
  for (int p = 0; p < kChoirParts; ++p) {
    if (!seatsOf[p]) continue;
    const double partAt = 0.008 * C.gauss();
    std::vector<int> who;
    for (int i = 0; i < int(C.seats.size()); ++i)
      if (C.seats[i].part == p) who.push_back(i);
    // (a divided part sings its lines left to right from the highest: its singers taken from the
    // left, its keys from the top)
    std::stable_sort(who.begin(), who.end(), [&](int a, int b) { return C.seats[a].x < C.seats[b].x; });
    std::vector<int> ks(pk[p].rbegin(), pk[p].rend());
    const int nk = int(ks.size());
    std::vector<int> want(nk, nk ? int(who.size()) / nk : 0), have(nk, 0);
    for (int r = 0; nk && r < int(who.size()) % nk; ++r) want[nk - 1 - r] += 1;  // (the spare singers to the lowest lines)
    auto reaches = [&](int i, int k) { return keys[k] >= C.seats[i].lo && keys[k] <= C.seats[i].hi; };
    std::vector<int> spare;
    for (int i : who) {
      int at = -1;
      for (int j = 0; j < nk; ++j)
        if (C.seats[i].want == keys[ks[j]]) at = j;
      if (at < 0 || have[at] >= want[at] || !reaches(i, ks[at])) spare.push_back(i);
      else ++have[at];
    }
    std::vector<bool> used(spare.size(), false);
    for (int j = 0; j < nk; ++j)
      for (int pass = 0; pass < 2 && have[j] < want[j]; ++pass)
        for (size_t q = 0; q < spare.size() && have[j] < want[j]; ++q) {
          // (first those who reach it; if none of the part does, the nearest anyway: pass 1, only
          // while the key still has no one)
          // A bass who does not reach a deep key sings it an octave up (doubling the oktavists).
          const ChoirState::Seat& sq = C.seats[spare[q]];
          const bool up = pass == 1 && !reaches(spare[q], ks[j]) && keys[ks[j]] + 12 >= sq.lo && keys[ks[j]] + 12 <= sq.hi &&
                          keys[ks[j]] < 40;
          if (used[q] || (pass == 0 && !reaches(spare[q], ks[j])) || (pass == 1 && have[j] > 0 && !up)) continue;
          used[q] = true;
          ++have[j];
          ChoirState::Seat& s = C.seats[spare[q]];
          const bool fresh = s.want < 0;
          s.want = keys[ks[j]] + (up ? 12 : 0);
          s.vel = std::clamp(vel[keys[ks[j]]] + int(13 * C.rnd()) - 6 - (up ? 25 : 0), 1, 127);  // (the doubling under them)
          s.at = now_ + std::max(0.0, spread * ((fresh ? enter() : 0.06 * C.rnd()) + s.habit + partAt));
        }
    for (size_t q = 0; q < spare.size(); ++q) {  // nothing for it: it stops (by breath: 1.5 x as scattered)
      if (used[q]) continue;
      ChoirState::Seat& s = C.seats[spare[q]];
      if (s.want < 0) continue;
      s.want = -1;
      s.at = now_ + spread * std::clamp(0.03 + 0.03 * C.gauss(), 0.0, 0.12);
    }
  }
}

// The pajupilli intonation for the choir (PJ_JI, PJ_CHOIR_JI; on), tuned by the spectra as the
// organ's (src/organ.cc justTune): each sung note's spectrum its voice's harmonics (falling ~6 dB an
// octave, the band of the first two resonances, ~300 .. 1500 Hz, stronger), every two notes'
// partials within 36 c of each other pulling their interval to where they meet, by the weaker
// one's level (the beating's depth) over the higher harmonic number squared. A note already sounding stays near where it is (at most 3 c
// a change); the new ones are placed together, their mean on equal temperament. At most +-25 c.
void Ensemble::choirTune() {
  ChoirState& C = *choir_;
  static const bool kOn = ji_ && knob("PJ_CHOIR_JI", 1) != 0;
  const bool ear = listens();
  if (!kOn) {  // (no intonation of its own: each note where the band pulls it, or tempered)
    if (ear)
      for (int k = 0; k < 128; ++k) C.tune[k] = C.heard[k] ? C.band[k] : 0.0;
    return;
  }
  // (By ear, each note's "equal temperament" is its band pull b: a new note anchored on it, a
  // sounding one where it was, moved as its pull has moved since the last solve.)
  auto pull = [&](int k) { return ear && C.heard[k] ? C.band[k] : 0.0; };
  int note[64], n = 0;
  bool was[64];
  for (const ChoirState::Seat& s : C.seats) {
    if (s.want < 0) continue;
    bool dup = false;
    for (int i = 0; i < n; ++i) dup |= note[i] == s.want;
    if (!dup && n < 64) note[n++] = s.want;
  }
  std::sort(note, note + n);
  // (a note is placed while young, its first 150 ms: a chord struck together placed together)
  bool now[128] = {};
  for (int i = 0; i < n; ++i) {
    now[note[i]] = true;
    if (!C.sounding[note[i]]) C.since[note[i]] = now_;
    was[i] = now_ - C.since[note[i]] > 0.15;
  }
  std::copy(now, now + 128, C.sounding);
  if (n == 0) return;
  // The spectra: (log2 of the partial's frequency, level), normalised to their power.
  std::vector<std::vector<std::pair<double, double>>> spec(n);
  for (int i = 0; i < n; ++i) {
    const double f0 = 440 * std::exp2((note[i] - 69) / 12.0);
    double pw = 0;
    for (int k = 1; k <= 24 && k * f0 < 5000; ++k) {
      const double band = std::log2(k * f0 / 700);
      const double w = (1.0 / k) * (1 + 2 * std::exp(-band * band / 0.8));
      spec[i].push_back({std::log2(double(k)), w});
      pw += w * w;
    }
    for (auto& q : spec[i]) q.second /= std::sqrt(pw);
  }
  static const double kLim = knob("PJ_CHOIR_JI_LIM", 25);
  double A[64][64] = {}, b[64] = {}, c[64];
  int ny = 0;
  for (int i = 0; i < n; ++i) ny += !was[i];
  for (int i = 0; i < n; ++i) {
    const int k = note[i];
    const double l = was[i] ? 1.0 : 0.005, a = was[i] ? C.tune[k] + pull(k) - C.solved[k] : pull(k);
    A[i][i] += l;
    b[i] += l * a;
  }
  if (ny > 0) {  // (the young's mean on their pulls' mean: 4 (sum c / ny - mean b)^2)
    double mb = 0;
    for (int i = 0; i < n; ++i)
      if (!was[i]) mb += pull(note[i]);
    mb /= ny;
    for (int i = 0; i < n; ++i) {
      if (was[i]) continue;
      b[i] += 4.0 / ny * mb;
      for (int j = 0; j < n; ++j)
        if (!was[j]) A[i][j] += 4.0 / (double(ny) * ny);
    }
  }
  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j) {
      const double semis = note[j] - note[i];
      double sw = 0, sd = 0;
      for (auto& p : spec[i])
        for (auto& q : spec[j]) {
          const double delta = 1200 * (q.first + semis / 12 - p.first);
          if (std::fabs(delta) > 36) continue;
          // (the beating's depth, the weaker partial's level; the higher the partials, the less
          // the ear tunes by them, unresolved: / the higher harmonic number squared)
          const double kq = std::max(std::exp2(p.first), std::exp2(q.first));
          const double ww = std::min(p.second, q.second) * std::exp(-(delta / 20) * (delta / 20)) / (kq * kq);
          sw += ww;
          sd -= ww * delta;
        }
      if (sw < 1e-6) continue;
      const double d = sd / sw;
      static const double kW = knob("PJ_CHOIR_JI_W", 4);
      sw *= kW;
      A[i][i] += sw; A[j][j] += sw; A[i][j] -= sw; A[j][i] -= sw;
      b[j] += sw * d; b[i] -= sw * d;
    }
  for (int k = 0; k < n; ++k)
    for (int r = k + 1; r < n; ++r) {
      const double f = A[r][k] / A[k][k];
      for (int cc = k; cc < n; ++cc) A[r][cc] -= f * A[k][cc];
      b[r] -= f * b[k];
    }
  for (int k = n - 1; k >= 0; --k) {
    double v = b[k];
    for (int jj = k + 1; jj < n; ++jj) v -= A[k][jj] * c[jj];
    c[k] = std::clamp(v / A[k][k], -kLim, kLim);
  }
  for (int i = 0; i < n; ++i) {
    double t = c[i];
    const int k = note[i];
    if (was[i]) {
      const double at = C.tune[k] + pull(k) - C.solved[k];
      t = std::clamp(t, at - 3, at + 3);
    }
    C.tune[k] = t;
    C.solved[k] = pull(k);
  }
  if (std::getenv("PJ_CHOIR_TUNEDBG")) {
    std::string d = "choir tune:";
    for (int i = 0; i < n; ++i) d += strf(" %d %+.1f", note[i], C.tune[note[i]]);
    say(stderr, "%s\n", d.c_str());
  }
}

void Ensemble::choirOn(int key, int velocity) {
  choir_->held[std::clamp(key, 0, 127)] = std::clamp(velocity, 1, 127);
  choirAssign();
  choirTune();
}

void Ensemble::choirOff(int key) {
  int& h = choir_->held[std::clamp(key, 0, 127)];
  if (h == 0) return;
  h = 0;
  choirAssign();
  choirTune();
}

void Ensemble::setChoirRole(int part, int index, int count) {
  if (!choir_ || !choirInst_) return;
  part = std::clamp(part, 0, kChoirParts - 1);
  const SoundingSet* band = band_;
  initChoir(*choirInst_, choirRoom_, part, index, count);
  band_ = band;
}

namespace {
// As the winds' ear (src/windear.cc): the pull followed by its weight w as w / (w + kHalf), nothing
// heard under kFloor (healing to 0 c in kHealTau); heard again every kEvery and when the band
// changes; a change under kLetBe let be; the voice moving at most kSlew c/s, no change quicker
// than kQuickest s (a singer's ear and larynx: about as quick as the lips).
constexpr double kChHalf = 0.0005, kChFloor = 3e-4, kChHealTau = 3.0, kChEvery = 0.05;
constexpr double kChSlew = 200, kChQuickest = 0.1, kChLetBe = 3;
}  // namespace

bool Ensemble::choirEar(double dt) {
  ChoirState& C = *choir_;
  bool want[128] = {}, moved = false;
  for (const ChoirState::Seat& s : C.seats)
    if (s.want >= 0) want[s.want] = true;
  for (int k = 0; k < 128; ++k) {
    if (!want[k]) { C.heard[k] = false; continue; }
    double w;
    if (!C.heard[k]) {  // (a new note: heard at once, sung there from its start)
      const double p = bandHear(0, k, &w, &C.share[k]);
      C.band[k] = C.bandTo[k] = std::clamp(p * w / (w + kChHalf), -earLim_, earLim_);
      C.bandMix[k] = w;
      C.bandAt[k] = now_;
      C.bandVer[k] = band_->version();
      C.heard[k] = true;
      if (earLog_)
        say(stderr, "choir ear: t=%.3f ch %d note %d pull %+.1f c (w %.5f) -> %+.1f c\n", band_->now(), self_, k, p, w, C.band[k]);
      moved = true;
      continue;
    }
    if (band_->version() != C.bandVer[k] || now_ - C.bandAt[k] > kChEvery) {
      C.bandVer[k] = band_->version();
      C.bandAt[k] = now_;
      const double to = std::clamp(bandHear(0, k, &w, &C.share[k]) * w / (w + kChHalf), -earLim_, earLim_);
      C.bandMix[k] = w;
      if (w >= kChFloor && std::fabs(to - C.bandTo[k]) >= kChLetBe) C.bandTo[k] = to;
    }
    if (band_->drift() != 0 && C.share[k] > 0) {  // (the ears' common healing, SoundingSet::drift)
      const double move = band_->drift() * C.share[k] * dt;
      C.band[k] = std::clamp(C.band[k] + move, -earLim_, earLim_);
      C.bandTo[k] = std::clamp(C.bandTo[k] + move, -earLim_, earLim_);
    }
    const double d = C.bandTo[k] - C.band[k];
    if (C.bandMix[k] < kChFloor) {
      C.band[k] -= C.band[k] * std::min(1.0, dt / kChHealTau);
      if (std::fabs(C.bandTo[k]) >= kChLetBe) C.bandTo[k] = 0;
    } else if (d != 0) {
      const double step = std::min(kChSlew, std::max(std::fabs(d) / kChQuickest, 10.0)) * dt;
      C.band[k] = std::fabs(d) <= step ? C.bandTo[k] : C.band[k] + std::copysign(step, d);
    }
    if (std::fabs(C.band[k] - C.solved[k]) >= 1) moved = true;
  }
  return moved;
}

double Ensemble::choirKey(int key, bool share) const {
  const ChoirState& C = *choir_;
  if (!C.heard[key]) return 0;
  return share ? C.share[key] : C.tune[key];
}

void Ensemble::choirRender(float* left, float* right, int n) {
  if (listens() && choirEar(n / 48000.0)) choirTune();
  Ensemble& kid = *kids_[0].ens;
  const float g = kids_[0].gain;
  hitL_.resize(n);
  hitR_.resize(n);
  for (int done = 0; done < n;) {
    for (ChoirState::Seat& s : choir_->seats) {
      if (s.want == s.cur || now_ < s.at) continue;
      // To its new key before it leaves the old one: a slur.
      if (s.want >= 0) kid.drive(s.slot, s.want, s.vel, true);
      if (s.cur >= 0) kid.drive(s.slot, s.cur, 0, false);
      s.cur = s.want;
    }
    for (ChoirState::Seat& s : choir_->seats)
      if (s.cur >= 0) kid.f_[s.slot].v->player().setTuning(choir_->tune[s.cur]);
    const int m = std::min(n - done, 128);
    kid.render(hitL_.data(), hitR_.data(), m);
    for (int i = 0; i < m; ++i) {
      left[done + i] = g * hitL_[i];
      right[done + i] = g * hitR_[i];
    }
    now_ += m / 48000.0;
    done += m;
  }
}

}  // namespace pajupilli
