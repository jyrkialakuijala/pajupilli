#include "pianoinst.h"

#include "heard.h"

#include <cstdio>

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"
#include "sk7.h"

namespace pajupilli {

namespace {
constexpr int kLowest = 21;  // A0
}

// The upright (src/upright.cc, when it is built in).
PianoDesign uprightDesign() __attribute__((weak));

static bool honkyTonk(const Instrument& inst) { return std::strcmp(inst.name, "honky-tonk") == 0; }
static bool upright(const Instrument& inst) { return std::strcmp(inst.name, "upright-piano") == 0 || honkyTonk(inst); }

static bool dulcimer(const Instrument& inst) { return std::strcmp(inst.name, "dulcimer") == 0; }

// The bright piano (General MIDI 2): a Shigeru Kawai SK-7 (src/sk7.cc).
static bool brightPiano(const Instrument& inst) { return std::strcmp(inst.name, "bright-piano") == 0; }

bool grandPiano(const Instrument& inst) {
  return std::strcmp(inst.name, "grand-piano") == 0 || brightPiano(inst) || upright(inst) || dulcimer(inst);
}

// The honky-tonk (General MIDI 4; research/piano_variants.md section 3.3-3.4): the upright, every
// string of a key its own ~1 Hz off (the outer ones of three either side, the middle one a
// little; both of a bichord; x 0.6-1.4 each, capped at 20 c; PJ_HONKY_HZ; the cents recipe
// PJ_HONKY_CENTS instead), some strings ~3 Hz flat (15 %, x 0.7-1.3, capped at 40 c: a neglected
// piano's; PJ_HONKY_FLAT, _FLATHZ), its hammers worn (the strings' grooves flatten and harden the felt: K x 8, the felt
// law's exponent 0.3 lower (no more: under ~1 the contact is unstable) (F = K x^p with x far under a metre: a lower p is a harder felt), the
// nap pressed flat, each key's hardness 30 % its own): every blow bright with overtones.
static PianoDesign honkyTonkDesign(PianoDesign d) {
  d.name = "honky-tonk";
  d.honkyCents = knob("PJ_HONKY_CENTS", 0.0);  // (the cents recipe: off)
  d.honkyHz = knob("PJ_HONKY_HZ", 1.0);        // (each string of a key its own ~1 Hz off)
  d.flatShare = knob("PJ_HONKY_FLAT", 0.15);
  d.flatMax = knob("PJ_HONKY_FLATMAX", 25.0);
  d.flatHz = knob("PJ_HONKY_FLATHZ", 3.0);     // (those strings ~3 Hz flat)
  d.tackHard = knob("PJ_HONKY_TACK", 5.0);    // (one or two strings of a key met first by metal: N per m/s)
  d.tackTime = knob("PJ_HONKY_TACKTIME", 3e-4);
  d.tackGap = knob("PJ_HONKY_TACKGAP", 0.3e-3);  // (the face uneven by up to 0.3 mm)
  d.seed = 1923;
  uint32_t rng = 777u;
  auto normal = [&rng] {
    rng = rng * 1664525u + 1013904223u;
    const double a = std::max((rng >> 8) * (1.0 / 16777216.0), 1e-12);
    rng = rng * 1664525u + 1013904223u;
    return std::sqrt(-2 * std::log(a)) * std::cos(2 * M_PI * (rng >> 8) * (1.0 / 16777216.0));
  };
  static const double kWorn = knob("PJ_HONKY_WORN", 8.0);
  // (a bar-room upright's bright, metallic voice: the hammers hard and narrow, the strings' highs
  // ringing longer)
  static const double kHard = knob("PJ_HONKY_HARD", -0.3), kNarrow = knob("PJ_HONKY_NARROW", 1.0), kRing = knob("PJ_HONKY_RING", 1.0);
  static const double kNap = knob("PJ_HONKY_NAP", 0.0);
  d.hammerWidth *= kNarrow;
  // (no hiss over the top: the strings' modes kept to ~9 kHz, the hard hammers exciting nothing
  // above; PJ_HONKY_TOP)
  static const double kTop = knob("PJ_HONKY_TOP", 9000);
  d.top = std::min(d.top, kTop);
  d.topTreble = std::min(d.topTreble, kTop);
  for (int k = 0; k < d.keys; ++k) d.key[k].top = d.key[k].top > 0 ? std::min(d.key[k].top, kTop) : kTop;
  for (int k = 0; k < d.keys; ++k) {
    PianoKey& s = d.key[k];
    if (s.K <= 0) continue;
    s.K *= kWorn * std::exp(0.3 * normal());
    s.p += kHard;
    s.b2 *= kRing;
    s.b3 *= kRing;
    s.nap *= kNap;  // (the felt's loose outer layer pressed flat by years of playing)
  }
  return d;
}

static PianoDesign designFor(const Instrument& inst) {
  if (dulcimer(inst)) return dulcimerDesign();
  if (std::strncmp(inst.name, "harpsichord", 11) == 0) return harpsichordDesign();
  if (honkyTonk(inst) && uprightDesign) return honkyTonkDesign(uprightDesign());
  if (brightPiano(inst)) {  // (its felt packs down as the grand's does: it stays the brighter)
    PianoDesign d = sk7Design();
    d.napPack = knob("PJ_PIANO_NAPPACK", 1.0);
    return d;
  }
  if (upright(inst) && uprightDesign) return uprightDesign();
  PianoDesign d = grandDesign();
  d.topBridge = knob("PJ_PIANO_TOPBRIDGE", 4.8);
  d.topUp = knob("PJ_PIANO_TOPUP", 1.5);
  d.topF0 = knob("PJ_PIANO_TOPF0", 1.0);
  d.napPack = knob("PJ_PIANO_NAPPACK", 1.0);
  return d;
}

PianoModel::PianoModel(const Instrument& inst) : Tube(inst), piano_(inst.fs, designFor(inst)) {
  // (the action's knock: the hammered pianos)
  if (std::strcmp(inst.name, "grand-piano") == 0 || std::strcmp(inst.name, "bright-piano") == 0 ||
      std::strcmp(inst.name, "upright-piano") == 0) {
    actGain_ = float(knob("PJ_PIANO_ACTION", 3.0));  // (by ear: ~-26 dB under a middle C at its start, ~-15 dB in the top octave)
    actOn_ = actGain_ > 0;
    pearlGain_ = float(knob("PJ_PIANO_PEARL", 0.6));
    tickD_ = float(std::exp(-6.9078 / (knob("PJ_PIANO_TICKT60", 0.006) * inst.fs)));  // (-60 dB in ~6 ms)
    tickA_ = float(std::exp(-2 * M_PI * 4000 / inst.fs));                               // (its high-pass, ~4 kHz)
    static const double kF[kActModes] = {260, 640, 1150, 2300}, kT60[kActModes] = {0.03, 0.025, 0.02, 0.015};
    for (int b = 0; b < kActBands; ++b)
      for (int m = 0; m < kActModes; ++m) {
        const double f = kF[m] * (0.92 + 0.05 * b), g = std::exp(-6.9078 / (kT60[m] * inst.fs));
        actC_[b][m] = float(g * std::cos(2 * M_PI * f / inst.fs));
        actS_[b][m] = float(g * std::sin(2 * M_PI * f / inst.fs));
      }
  }
  // (the grand: its board's regions at their places in the room)
  if (std::strcmp(inst.name, "grand-piano") == 0 && knob("PJ_PIANO_PARTS", 1) != 0) {
    piano_.keepRegions(true);
    double cx = 0, cy = 0;
    for (int j = 0; j < Piano::kRegions; ++j) {
      double x, y;
      piano_.regionPlace(j, &x, &y);
      cx += x / Piano::kRegions;
      cy += y / Piano::kRegions;
    }
    for (int j = 0; j < Piano::kRegions; ++j) {
      double x, y;
      piano_.regionPlace(j, &x, &y);
      partPlace_.insert(partPlace_.end(), {float(y - cy), float(-(x - cx)), 0.0f});
    }
    partPlace_.insert(partPlace_.end(), {0.0f, 0.0f, 0.0f});
    part_.resize(Piano::kRegions + 1);
  }
  if (honkyTonk(inst)) {
    treble_ = float(std::pow(10.0, knob("PJ_HONKY_TREBLE", 6.0) / 20) - 1);
    trebleA_ = float(1 - std::exp(-2 * M_PI * knob("PJ_HONKY_TREBLEHZ", 2000) / inst.fs));
    air_ = float(std::pow(10.0, knob("PJ_HONKY_AIR", -6.0) / 20) - 1);  // (the top shelved down: no hiss)
    airA_ = float(1 - std::exp(-2 * M_PI * knob("PJ_HONKY_AIRHZ", 6000) / inst.fs));
    box_ = float(knob("PJ_HONKY_BOX", 1.0));
    boxFb_ = float(knob("PJ_HONKY_BOXFB", 0.65));
    boxLp_ = float(1 - std::exp(-2 * M_PI * knob("PJ_HONKY_BOXLP", 1500) / inst.fs));
    const double dims[3] = {1.4, 1.2, 0.3};
    for (int i = 0; i < 3; ++i) boxLen_[i] = std::clamp(int(2 * dims[i] / 343 * inst.fs + 0.5), 2, Box::kMax);
    const double fc = knob("PJ_HONKY_LP", 7000);
    if (fc > 0 && fc < 0.45 * inst.fs) {
      const double w = 2 * M_PI * fc / inst.fs, al = std::sin(w) / (2 * 0.707), a0 = 1 + al;
      lpB_[0] = lpB_[2] = float((1 - std::cos(w)) / 2 / a0);
      lpB_[1] = float((1 - std::cos(w)) / a0);
      lpA_[0] = float(-2 * std::cos(w) / a0);
      lpA_[1] = float((1 - al) / a0);
      lpOn_ = true;
    }
  }
}

void PianoModel::resetAir() {
  Tube::resetAir();
  piano_.reset();
  // (the honky-tonk's filters and case: a NaN held there would come back every block)
  trebleM_ = trebleS_ = airM_ = airS_ = 0;
  lpZm_[0] = lpZm_[1] = lpZs_[0] = lpZs_[1] = 0;
  for (Box* b : {&boxM_, &boxS_}) *b = Box();
}

float PianoModel::sample() {
  // (The microphones' pressure, Pa at 0.65 m over the strings: a mezzo-forte
  // middle C peaks near 1 Pa.)
  static const float g = float(knob("PJ_PIANO_GAIN", 1.0 / 6));
  float y = g * piano_.sample(float(testDp_));  // (tests: a force on the bridge at middle C)
  testDp_ = 0;
  float sd = g * float(outScale()) * piano_.side();
  if (treble_ != 0) {
    // (the honky-tonk: the bar-room upright with its front off, miked close to the strings: its
    // highs lifted from ~2 kHz, the top above ~6 kHz shelved down again (a bright piano, no hiss))
    trebleM_ += trebleA_ * (y - trebleM_);
    trebleS_ += trebleA_ * (sd - trebleS_);
    y += treble_ * (y - trebleM_);
    sd += treble_ * (sd - trebleS_);
    airM_ += airA_ * (y - airM_);
    airS_ += airA_ * (sd - airS_);
    y += air_ * (y - airM_);
    sd += air_ * (sd - airS_);
    // (and a gentle 12 dB/octave roll-off from ~7 kHz: the wooden case, no hiss)
    auto lp = [&](float x, float* z) {
      const float o = lpB_[0] * x + z[0];
      z[0] = lpB_[1] * x - lpA_[0] * o + z[1];
      z[1] = lpB_[2] * x - lpA_[1] * o;
      return o;
    };
    if (lpOn_) {
      y = lp(y, lpZm_);
      sd = lp(sd, lpZs_);
    }
    // (the closed case: its air rings between the walls, ~1.4 x 1.2 x 0.3 m inside: three
    // round trips, each losing its highs to the wood and the felt, the boxy sound of an upright
    // with its lid down)
    if (box_ > 0) {
      auto box = [&](float x, Box& b) {
        float out = 0;
        for (int i = 0; i < 3; ++i) {
          const int n = boxLen_[i];
          float& lpS = b.lp[i];
          const float d = b.line[i][(b.pos[i] + 1) % n];
          lpS += boxLp_ * (d - lpS);
          b.line[i][b.pos[i]] = x + boxFb_ * lpS;
          b.pos[i] = (b.pos[i] + 1) % n;
          out += lpS;
        }
        return x + box_ * out / 3;
      };
      y = box(y, boxM_);
      sd = box(sd, boxS_);
    }
  }
  // The action's knock, in its quarters of the keyboard.
  float act[kActBands] = {};
  if (actOn_)
    for (int b = 0; b < kActBands; ++b)
      for (int m = 0; m < kActModes; ++m) {
        const float re = actRe_[b][m], im = actIm_[b][m];
        actRe_[b][m] = actC_[b][m] * re - actS_[b][m] * im;
        actIm_[b][m] = actS_[b][m] * re + actC_[b][m] * im;
        act[b] += actIm_[b][m];
      }
  if (actOn_)
    for (Pearl& p : pearl_) {
      if (!p.on) continue;
      float e = 0;
      for (int m = 0; m < kPearlModes; ++m) {
        const float re = p.re[m], im = p.im[m];
        p.re[m] = p.c[m] * re - p.s[m] * im;
        p.im[m] = p.s[m] * re + p.c[m] * im;
        act[p.band] += p.im[m];
        e += std::fabs(p.re[m]) + std::fabs(p.im[m]);
      }
      if (e < 1e-8f) p.on = false;
    }
  if (actOn_)
    for (int b = 0; b < kActBands; ++b) {
      if (tick_[b] < 1e-7f) continue;
      tickRng_ = tickRng_ * 1664525u + 1013904223u;
      float w = tick_[b] * ((tickRng_ >> 8) * (2.0f / 16777216.0f) - 1);
      tick_[b] *= tickD_;
      for (float& z : tickHp_[b]) {  // (two one-pole high-passes)
        const float lo = z + (1 - tickA_) * (w - z);
        z = lo;
        w -= lo;
      }
      act[b] += w;
    }
  if (hallAct_ != 1)
    for (float& a : act) a *= hallAct_;
  for (int b = 0; b < kActBands; ++b) y += act[b];
  last_ = y;
  side_.push_back(sd);
  const float out = finish(last_);
  if (!part_.empty()) {
    if (part_[0].size() >= 1 << 15)  // (nobody takes them: played without a room)
      for (auto& p : part_) p.clear();
    const float gp = g * float(outScale());
    float sum = 0;
    for (int j = 0; j < Piano::kRegions; ++j) {
      // (a band's knock from its front region: the board's regions are 2 x band + 0 at the front)
      const float b = gp * piano_.region(j) + (j % 2 == 0 ? float(outScale()) * act[j / 2] : 0.0f);
      part_[j].push_back(b);
      sum += b;
    }
    part_[Piano::kRegions].push_back(out - sum);
  }
  return out;
}

void PianoModel::concertHall(bool on) {
  piano_.hallSound(on);
  hallAct_ = inConcertHall() ? float(knob("PJ_PIANO_HALLACT", 0.5)) : 1.0f;
}

void PianoModel::actionKnock(int key, double hammerSpeed) {
  if (!actOn_ || hammerSpeed <= 0) return;
  const int b = std::clamp(key * kActBands / std::max(1, piano_.keys()), 0, kActBands - 1);
  // (Pa at the microphones: a forte stroke's ~0.05 Pa; its four resonances by their share)
  static const float kAmp[kActModes] = {1.0f, 0.4f, 0.15f, 0.05f};  // the thump kept, the wood's click above ~600 Hz -5, -10, -16 dB under what it was (by ear)
  const float a = float(actGain_ * 0.004 * std::pow(hammerSpeed, 1.5));
  const int midi = key + 21;
  // (up the keyboard the wood's upper resonances (640 Hz - 2.3 kHz), which sit on the upper
  // partials of the notes from C5 up and made their first 30 ms a bright honky burst over a
  // rounder tone, give way: from C4, nearly gone (x 0.1) by C6; the thump under 300 Hz stays, and
  // from C5 the pearls take over, fully from C6)
  const double up = std::clamp((midi - 60) / 24.0, 0.0, 1.0);
  const double top2 = std::clamp((midi - 72) / 12.0, 0.0, 1.0);
  // (each resonance struck into its sine phase: the output (im) starts from 0; set into the
  // cosine it stepped at once to its peak, a click of every frequency)
  for (int m = 0; m < kActModes; ++m) actRe_[b][m] += a * kAmp[m] * float(m == 0 ? 1.0 : 1.0 - 0.9 * up);
  if (top2 > 0 && pearlGain_ > 0) {
    Pearl& p = pearl_[pearlNext_];
    pearlNext_ = (pearlNext_ + 1) % kPearls;
    const double f0 = 440 * std::exp2((midi - 69) / 12.0);
    static const double kDiv[kPearlModes] = {2, 4, 8}, kT60[kPearlModes] = {0.035, 0.05, 0.06}, kA[kPearlModes] = {0.45, 0.8, 0.55};
    for (int m = 0; m < kPearlModes; ++m) {
      const double f = f0 / kDiv[m];
      const double g = f < 110 ? 0.0 : std::exp(-6.9078 / (kT60[m] * fs_));
      p.c[m] = float(g * std::cos(2 * M_PI * f / fs_));
      p.s[m] = float(g * std::sin(2 * M_PI * f / fs_));
      p.im[m] = 0;  // (into the sine phase, as the knock)
      p.re[m] = g > 0 ? float(pearlGain_ * top2 * kA[m]) * a : 0.0f;
    }
    p.band = b;
    p.on = true;
  }
  // The top keys' tick (from D6, fully from E7): faint, high and noisy, dying at once.
  static const double kTick = knob("PJ_PIANO_TICK", 0.5);
  const double top = std::clamp((key + 21 - 86) / 14.0, 0.0, 1.0);
  if (top > 0) tick_[b] += float(kTick * top) * a;
}

const float* PianoModel::takePart(int n, int j) {
  if (j < 0 || j >= int(part_.size())) return nullptr;
  std::vector<float>& p = part_[j];
  partOut_.assign(n, 0.0f);
  const int have = int(p.size()), m = std::min(n, have);
  std::copy(p.end() - m, p.end(), partOut_.end() - m);
  p.clear();
  return partOut_.data();
}

PianoPlayer::PianoPlayer(PianoModel& m) : Performer(m), m_(m) {
  // (PJ_PIANO_PEDAL: the sustain pedal held from the start: every string free, the most work.)
  if (knob("PJ_PIANO_PEDAL", 0) > 0) {
    pedalDown_ = true;  // (lifted before the first note: no knock)
    setHold(true);
  }
  if (dulcimer(m.instrument())) { lo_ = 43; hi_ = 93; }  // G2 .. A6
}

int PianoPlayer::fold(int midi) const {  // (an instrument of a smaller compass: by octaves into it)
  while (midi < lo_) midi += 12;
  while (midi > hi_) midi -= 12;
  return midi;
}

double PianoPlayer::hammerSpeed(int velocity, double dynamics) {
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return (0.35 + 5.0 * std::pow(v, 1.7)) * (0.7 + 0.6 * std::clamp(dynamics, 0.0, 1.0));
}

void PianoPlayer::noteOn(int midi, int velocity) {
  midi = fold(midi);
  const int key = midi - kLowest;
  if (key < 0 || key >= Piano::kKeys) return;
  articulate(midi, velocity, false);
  down_[key] = true;
  note_ = midi;
  blowing_ = true;
  m_.piano().damper(key, false);
  const double hs = hammerSpeed(velocity, dyn_);
  m_.piano().strike(key, hs);
  m_.actionKnock(key, hs);
}

void PianoPlayer::noteOff(int midi) {
  midi = fold(midi);
  const int key = midi - kLowest;
  if (key < 0 || key >= Piano::kKeys) return;
  down_[key] = false;
  if (!hold_) m_.piano().damper(key, true);
}

void PianoPlayer::setHold(bool h) {
  if (h != held_) {  // (the pedal's changes on stdout, as they are heard: src/heard.h)
    sayAt(t_, std::string(m_.instrument().name) + ": sustain pedal " + (h ? "down" : "up"));
    held_ = h;
  }
  hold_ = h;
  const double speed = pedalMoved(h);
  for (int k = 0; k < Piano::kKeys; ++k)
    if (h) m_.piano().damper(k, false);
    else if (!down_[k]) m_.piano().damper(k, true, speed, m_.piano().design().pedalUp);
}

// The pedal's speed. Its controller moving through the values between (a continuous pedal
// sensor): the foot's speed, from how far it moved since the last value, over the 6 mm the
// pedal lifts the dampers by; never more than they fall by themselves (~0.3 m/s: 5 mm under
// their weight, the underlevers' inertia with them). Only on and off (0 and 127, as most files
// and keyboards send): a release as a pianist's foot lets the pedal up, 0.15 m/s at the
// dampers (the pedal's 10 mm in some 40 ms), each its own (x 0.75 .. 1.25, seeded).
void PianoPlayer::sustainPedal(int cc) {
  cc = std::clamp(cc, 0, 127);
  nextSpeed_ = -1;
  if ((cc >= 64) != pedalDown_) {
    const bool between = (cc > 0 && cc < 127) || (pedalCC_ > 0 && pedalCC_ < 127);
    const double dt = now_ - pedalT_;
    if (between && pedalT_ >= 0 && dt > 0 && dt < 0.25)
      nextSpeed_ = std::min(0.3, std::abs(pedalCC_ - cc) / 127.0 / std::max(dt, 1e-3) * 0.006);
    // (an on/off release the player planned: its speed, when the controller gave none)
    if (nextSpeed_ < 0 && cc < 64 && plannedUp_ > 0) nextSpeed_ = plannedUp_;
  }
  if (cc < 64) plannedUp_ = -1;
  pedalCC_ = cc;
  pedalT_ = now_;
  setHold(cc >= 64);
}

// The pedal pressed or let go (or neither: -1): the rail's knock (piano.h's pedal()), and the
// speed the dampers of the keys not held fall back onto their strings at.
double PianoPlayer::pedalMoved(bool h) {
  if (h == pedalDown_) return -1;
  pedalDown_ = h;
  double speed = nextSpeed_;
  nextSpeed_ = -1;
  if (speed <= 0) {
    pedalRng_ = pedalRng_ * 1664525u + 1013904223u;
    speed = 0.15 * (0.75 + 0.5 * ((pedalRng_ >> 8) / 16777216.0));
  }
  m_.piano().pedal(h, speed);
  return speed;
}

void PianoPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    t_ += m / fs_;
    tick(m / fs_);
    m_.piano().setVoicing(std::pow(4.0, brightness_ - 1) * hardness_);
    m_.process(out + i, m);
  }
  now_ += n / fs_;
  blowing_ = !m_.piano().asleep();
}

}  // namespace pajupilli
