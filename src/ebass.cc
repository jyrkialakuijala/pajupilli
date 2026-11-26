#include "ebass.h"
#include "heard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "knob.h"
#include "vmath.h"

namespace pajupilli {

bool pickBassInstrument(const Instrument& inst) { return std::strcmp(inst.name, "pick-bass") == 0; }
int slapBassInstrument(const Instrument& inst) {
  return std::strcmp(inst.name, "slap-bass-1") == 0 ? 1 : std::strcmp(inst.name, "slap-bass-2") == 0 ? 2 : 0;
}
bool electricBassInstrument(const Instrument& inst) {
  return std::strcmp(inst.name, "electric-bass") == 0 || pickBassInstrument(inst) || slapBassInstrument(inst);
}

namespace {

// The two-pickup bass slapped (research/slap_bass_1.md 5.1-5.2, slap_bass_2.md 5.2): the frets as
// obstacles, the modes to 10 kHz (the slap's 2-8 kHz; 128 reach 10.9 kHz on the open E). The
// action at the 17th fret (PJ_SLAP_ACTION, mm on the E; the others a little lower as set up),
// the relief (PJ_SLAP_RELIEF, mm); PJ_SLAP_FRETS=0: no frets (to hear what they make). GM 38: new
// stainless roundwounds, their partials losing 15 % less (PJ_SLAP2_LOSS1, 0.85 of loss1: the
// research's 0.65 came out brighter than every reference set).
// GM 34 (the finger bass) gets the frets only with PJ_EBASS_FRETS=1.
GuitarSpec bassFor(const Instrument& inst) {
  if (pickBassInstrument(inst)) return splitCoilBass();
  GuitarSpec g = twoPickupBass();
  const int slap = slapBassInstrument(inst);
  if (!slap) {
    if (knob("PJ_EBASS_FRETS", 0) > 0.5) g.fretContact = true;
    return g;
  }
  g.fretContact = knob("PJ_SLAP_FRETS", 1) > 0.5;
  g.topHz = knob("PJ_SLAP_TOP", 10000);
  const double act = knob("PJ_SLAP_ACTION", 2.4) * 1e-3 / g.action[0];
  for (double& a : g.action) a *= act;
  g.relief = knob("PJ_SLAP_RELIEF", 0.3) * 1e-3;
  g.fretBounce = knob("PJ_SLAP_BOUNCE", 0.6);
  g.lossByFrequency = knob("PJ_SLAP_LOSSHZ", 1) > 0.5;
  g.fretWidth = knob("PJ_SLAP_FRETWIDTH", 0.0);
  if (slap == 2) {
    const double l1 = knob("PJ_SLAP2_LOSS1", 0.85);
    for (int i = 0; i < 4; ++i) g.str[i].loss1 *= l1;
  }
  return g;
}

}  // namespace

ElectricBassModel::ElectricBassModel(const Instrument& inst)
    : Tube(inst), g_(inst.fs, bassFor(inst)) {
  // The DI: the coils (two in parallel: half the inductance and resistance, twice the
  // capacitance) with the cable's 700 pF and the pots in parallel with the input's 1 MOhm:
  // H(s) = 1 / (L C s^2 + (L / Rl + R C) s + 1 + R / Rl), bilinear, warped at its resonance.
  const bool pick = pickBassInstrument(inst);
  const GuitarSpec& s = g_.spec();
  const GuitarSpec::Pickup& p = s.pickups[s.select[0]];
  const int n = s.select[1] >= 0 ? 2 : 1;
  double coilC = p.C * n, pots = s.pots;
  if (pick) {
    // The P's tone (PJ_PBASS_TONE: 1 open, 0 closed): the 250 k audio-taper pot in series with
    // 0.047 uF across the coil, beside the 250 k volume. As a resistance and a capacitance in
    // parallel, taken at 1.2 kHz (exact closed and open; between, near the coil's resonance
    // where it matters [E]).
    const double t = std::clamp(knob("PJ_PBASS_TONE", 1.0), 0.0, 1.0);
    const double rs = 250e3 * (fastPow(10.0, 2 * t) - 1) / 99, ct = 0.047e-6, w = 2 * M_PI * 1200;
    const double x = w * ct * rs, gp = w * ct * x / (1 + x * x);
    pots = 1 / (1 / 250e3 + gp);
    coilC += ct / (1 + x * x);
  }
  // GM 38's active J (research/slap_bass_2.md 2.3): a buffer at the coils; only the coils' own
  // capacitance and the wiring's load them (~200 pF in all: the resonance from 4.4 kHz to ~8.8);
  // then its two shelves (PJ_SLAP2_TREBLE dB at 4 kHz, PJ_SLAP2_BASS
  // dB at 50 Hz). GM 37 passive, its shelves 0 unless asked (PJ_SLAP_TREBLE, PJ_SLAP_BASS: the
  // Bartolini-like smile).
  const int slap = slapBassInstrument(inst);
  const bool active = slap == 2 && knob("PJ_SLAP2_ACTIVE", 1) > 0.5;
  // (The two 250 k volumes stay in front of the buffer; the passive tone is gone: 125 k across
  // the coils, so the resonance is broad, Q ~1, not a 12 dB peak.)
  if (active) pots = knob("PJ_SLAP2_POTS", 125e3);
  const double cable = active ? knob("PJ_SLAP2_WIRING", 20e-12) : knob("PJ_EBASS_CABLE", 700e-12);
  if (slap) {
    const double tre = slap == 2 ? knob("PJ_SLAP2_TREBLE", 4) : knob("PJ_SLAP_TREBLE", 0);
    const double bas = slap == 2 ? knob("PJ_SLAP2_BASS", 4) : knob("PJ_SLAP_BASS", 0);
    // (RBJ shelves, S = 1.)
    auto shelf = [&](Shelf& b, double f, double db, bool high) {
      const double A = fastPow(10.0, db / 40), w = 2 * M_PI * f / inst.fs, cw = std::cos(w), al = std::sin(w) / 2 * std::sqrt(2.0);
      const double sA = 2 * std::sqrt(A) * al;
      double b0, b1, b2, a0, a1, a2;
      if (high) {
        b0 = A * ((A + 1) + (A - 1) * cw + sA); b1 = -2 * A * ((A - 1) + (A + 1) * cw); b2 = A * ((A + 1) + (A - 1) * cw - sA);
        a0 = (A + 1) - (A - 1) * cw + sA; a1 = 2 * ((A - 1) - (A + 1) * cw); a2 = (A + 1) - (A - 1) * cw - sA;
      } else {
        b0 = A * ((A + 1) - (A - 1) * cw + sA); b1 = 2 * A * ((A - 1) - (A + 1) * cw); b2 = A * ((A + 1) - (A - 1) * cw - sA);
        a0 = (A + 1) + (A - 1) * cw + sA; a1 = -2 * ((A - 1) + (A + 1) * cw); a2 = (A + 1) + (A - 1) * cw - sA;
      }
      b.b0 = b0 / a0; b.b1 = b1 / a0; b.b2 = b2 / a0; b.a1 = a1 / a0; b.a2 = a2 / a0;
    };
    shelf(shelf_[0], 50, bas, false);
    shelf(shelf_[1], 4000, tre, true);
    shelves_ = tre != 0 || bas != 0;
  }
  const double L = p.L / n, R = p.R / n, C = coilC + cable;
  const double Rl = 1 / (1 / pots + 1 / 1e6);
  const double A = L * C, Bq = L / Rl + R * C, D = 1 + R / Rl;
  const double w0 = 1 / std::sqrt(A), c = w0 / std::tan(std::min(w0 / (2 * inst.fs), 1.5));
  const double a0 = A * c * c + Bq * c + D;
  b0_ = 1 / a0;
  b1_ = 2 / a0;
  b2_ = 1 / a0;
  a1_ = (-2 * A * c * c + 2 * D) / a0;
  a2_ = (A * c * c - Bq * c + D) / a0;
  out_ = knob("PJ_EBASS_OUT", 1.0);
  // The amp and the studio after it (unless PJ_EBASS_DI): told the same coils, pots and cable.
  // GM 34 the fliptop and the DI, an optical compressor; GM 35 the bass stack and its 8x10 with the DI, a FET compressor.
  if (knob("PJ_EBASS_DI", 0) > 0) return;
  amp_.reset(new GuitarAmp(inst.fs, pick ? GuitarAmp::bassStack() : slap == 1 ? GuitarAmp::slapDi() : slap == 2 ? GuitarAmp::slapSquash() : GuitarAmp::fliptopDi()));
  amp_->connect({L, R, coilC, pots}, cable);
}

void ElectricBassModel::resetAir() {
  Tube::resetAir();
  g_.reset();
  z1_ = z2_ = 0;
  for (Shelf& b : shelf_) b.z1 = b.z2 = 0;
  if (amp_) amp_->reset();
}

// The strings' sample: the tests' kick (testDrive) goes into them; a non-finite state they caught
// and cleared clears the rest too (the amp, the filters), counted as a reset.
float ElectricBassModel::strings() {
  if (testDp_ != 0) { g_.kick(float(testDp_)); testDp_ = 0; }
  const float x = g_.sample();
  if (g_.nanResets() != nanSeen_) [[unlikely]] {
    nanSeen_ = g_.nanResets();
    resetAir();
    ++resets_;
  }
  return x;
}

float ElectricBassModel::sample() {
  double x = strings();
  if (shelves_) x = shelf_[1].run(shelf_[0].run(x));
  if (amp_) {
    last_ = amp_->process(float(x)) * float(out_);
    return finish(last_);
  }
  const double y = b0_ * x + z1_;
  z1_ = b1_ * x - a1_ * y + z2_;
  z2_ = b2_ * x - a2_ * y;
  last_ = float(y * out_);
  return finish(last_);
}

BassLinePlayer::BassLinePlayer(ElectricBassModel& m) : Performer(m), m_(m) {
  // (The strings not being played are kept muted: the plucking hand's thumb lies on the lower
  // ones, the fretting hand's fingers on the higher. A plucked string is let free.)
  for (int s = 0; s < kStrings; ++s) m.bass().damp(s, 40);
}

double BassLinePlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

void BassLinePlayer::noteOn(int midi, int velocity) {
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  asked_.push_back({midi, velocity});
}

void BassLinePlayer::noteOff(int midi) {
  for (size_t i = 0; i < asked_.size(); ++i)
    if (asked_[i].midi == midi) { asked_.erase(asked_.begin() + i); return; }
  for (int s = 0; s < kStrings; ++s)
    if (noteOn_[s] == midi && held_[s]) {
      held_[s] = false;
      // (The fretting hand lifts and the plucking hand lies on the string: stopped.)
      if (!hold_) m_.bass().damp(s, offDamp_);
    }
}

void BassLinePlayer::play() {
  Guitar& g = m_.bass();
  static const int kOpen[kStrings] = {28, 33, 38, 43};
  const int top = kOpen[kStrings - 1] + g.frets();
  std::sort(asked_.begin(), asked_.end(), [](const Asked& a, const Asked& b) { return a.midi < b.midi; });
  if (asked_.size() > size_t(kStrings)) asked_.erase(asked_.begin(), asked_.end() - kStrings);
  begin(int(asked_.size()));
  bool taken[kStrings] = {};
  int minString = 0;
  for (const Asked& a : asked_) {
    int midi = a.midi;
    while (midi < kOpen[0]) midi += 12;
    while (midi > top) midi -= 12;
    // The string: free, near the hand, low on the neck, an open string welcome.
    int best = -1;
    double cost = 1e9;
    for (int s = minString; s < kStrings; ++s) {
      const int fret = midi - kOpen[s];
      if (fret < 0 || fret > g.frets() || taken[s]) continue;
      double c = fret == 0 ? -0.3 : std::fabs(fret - hand_) + 0.12 * fret;
      if (fret > 0 && std::fabs(fret - hand_) > 4) c += 2.5;
      if (held_[s]) c += 6;
      c += this->cost(s, fret, a);
      if (c < cost) { cost = c; best = s; }
    }
    if (best < 0) continue;
    const int fret = midi - kOpen[best];
    taken[best] = true;
    minString = best + 1;
    // A bass line is one voice: a new note on another string stops the last one, even if its
    // key is still down (MIDI's legato overlaps), unless they came together (a double stop).
    if (!ringOn_ && last_ >= 0 && last_ != best && !hold_ && sinceNote_ > 0.03) {
      g.damp(last_, 40);
      held_[last_] = false;
    }
    strike(best, fret, a);
    noteOn_[best] = a.midi;
    held_[best] = true;
    last_ = best;
    lastFret_ = fret;
    if (fret > 0) hand_ += 0.5 * (fret - 1 - hand_);
    hand_ = std::clamp(hand_, 1.0, 14.0);
  }
  asked_.clear();
  sinceNote_ = 0;
}

void BassLinePlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  Guitar& g = m_.bass();
  if (!asked_.empty()) play();
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    sinceNote_ += m / fs_;
    // Pitch bend: the last note's string pushed (up to two semitones; a fretted note only).
    if (last_ >= 0 && lastFret_ > 0) g.bend(last_, 200 * roll_);
    m_.process(out + i, m);
  }
  blowing_ = !g.asleep();
}

void ElectricBassPlayer::strike(int s, int fret, const Asked& a) {
  Guitar& g = m_.bass();
  // The fingers: index and middle in turn while notes come quickly; the middle a little
  // longer (deeper, a few mm further along); each stroke its own.
  const bool quick = sinceNote_ < 0.4;
  finger_ = quick ? 1 - finger_ : 0;
  const double v = std::clamp(a.velocity, 1, 127) / 127.0;
  const double depth = (0.4e-3 + 4.6e-3 * fastPow(v, 1.4)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) *
                       (finger_ ? 1.06 : 1.0) * (0.93 + 0.14 * ur());
  const double from = std::clamp(0.10 * std::exp2(1 - brightness_) * (1.05 - 0.15 * v) + (finger_ ? 0.006 : 0.0) +
                                     0.008 * (ur() - 0.5),
                                 0.04, 0.3);
  // A rest stroke: along the body and a little into it (the finger ends on the next string).
  const double angle = 0.35 + 0.15 * ur();
  g.fret(s, fret);
  // A hard stroke: the string snaps off the fingertip's firm edge (a fraction of a ms) instead
  // of rolling off the soft pad (more than one): the attack's growl and click.
  const double snap = knob("PJ_EBASS_SNAP", 0.4);
  g.pluckPad(s, depth, from, angle, 0.010, 0.002 + 0.002 * (1 - v) - 0.0008 * snap * v * v * v,
             0.0008 + 0.0006 * (1 - v) - 0.0005 * snap * v * v * v * v);
}

PickBassPlayer::PickBassPlayer(ElectricBassModel& m) : BassLinePlayer(m) {
  downOnly_ = knob("PJ_PICK_DOWN", 0) > 0.5;
  place_ = knob("PJ_PICK_PLACE", 0.10);
  // The pick (research/pick_bass.md 4.1, 9.2b [E]): a cantilever, its tip's stiffness as its
  // thickness cubed or so (0.6 mm ~2 kN/m, 1.0 ~6, 1.5 ~20); a thin one lets the string past
  // more slowly.
  const double mm = std::clamp(knob("PJ_PICK_MM", 1.0), 0.4, 3.0);
  kp_ = std::clamp(6000 * fastPow(mm, 2.6), 1500.0, 40000.0);
  slip_ = std::clamp(0.00015 * fastPow(mm, -1.36), 0.00008, 0.0005);
  mute_ = std::clamp(knob("PJ_PICK_MUTE", 0), 0.0, 1.0);
  felt_ = knob("PJ_PICK_FELT", 0) > 0.5 ? knob("PJ_PICK_FELTDEPTH", 2.0) : 0;
  ringOn_ = knob("PJ_PICK_CHORDS", 0) > 0.5;
  upGain_ = knob("PJ_PICK_UP", 1.3);
  pushPp_ = knob("PJ_PICK_PUSHPP", 0.012);
  pushFf_ = knob("PJ_PICK_PUSHFF", 0.003);
  velExp_ = knob("PJ_PICK_VELEXP", 1.8);
  // (The felt lies under all four strings, near the saddle, always.)
  if (felt_ > 0)
    for (int s = 0; s < kStrings; ++s) m.bass().mute(s, felt_, 0.010);
}

void PickBassPlayer::begin(int /*count*/) {
  // One stroke for what comes together (a double stop is one sweep): down; up after a down when
  // the notes come within 0.3 s (alternate picking), unless downstrokes only.
  up_ = !downOnly_ && !up_ && sinceNote_ < 0.3;
}

void PickBassPlayer::strike(int s, int fret, const Asked& a) {
  Guitar& g = m_.bass();
  const double v = std::clamp(a.velocity, 1, 127) / 127.0;
  // How far the hand carries the tip: 0.2-3.4 mm (a soft stroke hardly moves the string, a hard
  // one as far as a heavy pick holds it: v40 0.6, v80 1.6, v120 3.1). The up stroke lifts the
  // string a little out of the body, and the pickup hears less of it: the player digs ~30 % deeper
  // (PJ_PICK_UP), and it still comes out 2-3 dB weaker. Each stroke its own (+-7 %).
  const double depth = (0.2e-3 + 3.2e-3 * fastPow(v, velExp_)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) *
                       (up_ ? upGain_ : 1.0) * (0.93 + 0.14 * ur());
  // How quickly: a hard stroke is a fast one (12 ms at pp, 3 at ff, +-20 %); and the string
  // runs off the tip the faster, the faster the hand goes (the slip as the square root of the
  // hand's speed against 0.3 m/s [E]: 0.3 ms at pp, under 0.1 at ff with a 1 mm pick).
  const double push = (pushPp_ + (pushFf_ - pushPp_) * v) * (0.8 + 0.4 * ur());
  const double speed = std::max(depth, 1e-4) / push;
  const double slip = slip_ * std::sqrt(0.3 / speed);
  // Where: 10 cm from the saddle, over the pickup's bridge end (+-6 mm a stroke).
  const double from = std::clamp(place_ * std::exp2(1 - brightness_) + 0.012 * (ur() - 0.5), 0.03, 0.3);
  // Down along the body and a little into it; up along it and a little out (+-0.05 rad).
  const double angle = (up_ ? M_PI + 0.18 : 0.25) + 0.1 * (ur() - 0.5);
  // The palm on the string near the saddle (lifted a little on accents), or the felt.
  double palm = felt_;
  if (mute_ > 0) palm = std::max(palm, 8 * mute_ * (1 - 0.4 * v * v));
  g.fret(s, fret);
  g.mute(s, palm, mute_ > 0 ? 0.012 : 0.010);
  g.pickContact(s, depth, from, angle, push, slip, kp_, 0.0015);
}

SlapBassPlayer::SlapBassPlayer(ElectricBassModel& m, int program) : BassLinePlayer(m), prog_(program) {
  wait_ = noteLeadsApplied() ? noteLead(m.instrument()) : 0.0;
  offDamp_ = knob("PJ_SLAP_OFFDAMP", 60);
  popBias_ = std::max(0.2, knob("PJ_SLAP_POP", 1.0));
  thumbV_ = knob("PJ_SLAP_THUMBV", 1.0);
  thumbAt_ = knob("PJ_SLAP_THUMBAT", 0.265);
  ghostV_ = knob("PJ_SLAP_GHOST", 40);
  log_ = knob("PJ_SLAP_LOG", 0) > 0.5;
  force_ = int(knob("PJ_SLAP_FORCE", 0));
  flesh_ = knob("PJ_SLAP_FLESH", 0.0);
  thumbAngle_ = knob("PJ_SLAP_THUMBANGLE", 1.3);
  thumbLight_ = knob("PJ_SLAP_THUMBLIGHT", 0.5);
  for (int i = 0; i < 128; ++i) { tech_[i] = kThumb; rise_[i] = 0; delay_[i] = 0; offAt_[i] = -10; }
}

SlapBassPlayer::~SlapBassPlayer() {
  if (knob("PJ_SLAP_COUNT", 0) > 0.5)
    std::fprintf(stderr, "slap: thumbs %d pops %d ghosts %d hammers %d pulls %d upstrokes %d\n", count_[0], count_[1], count_[2], count_[3],
                 count_[4], count_[5]);
}

double SlapBassPlayer::ur2() {
  return ur();
}

void SlapBassPlayer::noteOn(int midi, int velocity) {
  midi = std::clamp(midi, 0, 127);
  Pend p{midi, std::clamp(velocity, 1, 127), now_, true};
  // (Legato: the key before it still down, or let go only just now.)
  p.prevHeld = lastArr_ >= 0 && (keyDown_[lastArr_] || now_ - offAt_[lastArr_] < 0.03);
  lastArr_ = midi;
  keyDown_[midi] = true;
  pend_.push_back(p);
  blowing_ = true;
}

void SlapBassPlayer::noteOff(int midi) {
  midi = std::clamp(midi, 0, 127);
  keyDown_[midi] = false;
  offAt_[midi] = now_;
  pend_.push_back({midi, 0, now_, false});
}

// What this note is (research/slap_bass_1.md 5.5; slap_bass_2.md 5.2), from the line so far and
// what of the coming one the player has already seen (its release, the next key).
void SlapBassPlayer::decide(size_t k) {
  Pend& p = pend_[k];
  p.decided = true;
  const double v = p.vel;
  double dur = -1, next = -1;
  int nextMidi = -1;
  bool chordTop = false, chordAny = false;
  int chordLow = p.midi;
  for (size_t j = 0; j < pend_.size(); ++j) {
    const Pend& o = pend_[j];
    if (j > k && !o.on && o.midi == p.midi && dur < 0) dur = o.t - p.t;
    if (j > k && o.on && next < 0 && o.t - p.t > 0.03) { next = o.t - p.t; nextMidi = o.midi; }
    if (j != k && o.on && std::fabs(o.t - p.t) <= 0.03) {
      chordAny = true;
      if (o.midi < p.midi) chordTop = true;
      chordLow = std::min(chordLow, o.midi);
    }
  }
  if (!chordAny && chord_ > 1) chord_ = 1;
  if (p.t - lowT_ > 2.0 || p.midi < lowMidi_) { lowMidi_ = p.midi; lowT_ = p.t; }
  const int d = lastMidi_ >= 0 ? p.midi - lastMidi_ : 99;
  const double gap = p.t - lastT_;
  // A ghost: soft, and short (if seen: its release or the next key within 90 ms).
  const bool shortNote = (dur >= 0 && dur < 0.09) || (dur < 0 && next >= 0 && next < 0.09);
  const bool ghost = prog_ == 2 ? v < ghostV_ && (dur < 0 || dur < 0.12) : v < ghostV_ || (v < ghostV_ + 15 && shortNote);
  // A pop wanted? GM 37: a leap up from the last thumbed note (9-16 semitones within 1 s), an
  // accented high note, the top of a double stop. GM 38: everything from D2 up, except a note a
  // pop a tenth or an octave up comes straight after (that one is the thumb's).
  bool pop = false;
  const int dt = thumbMidi_ >= 0 ? p.midi - thumbMidi_ : -99;
  // (A double stop all on the D and G strings: both popped, index and middle.)
  if (chordAny && chordLow >= 38) chordTop = true;
  if (prog_ == 1) {
    pop = (p.t - thumbT_ < 1.0 && dt >= 9 / popBias_ && dt <= 16) || (p.midi >= lowMidi_ + 12 && v >= meanVel_ + 8 / popBias_ && p.midi >= 45) ||
          chordTop || p.midi >= 64 - 8 * (popBias_ - 1);
  } else {
    pop = p.midi >= 38 || chordTop;
    if (pop && !chordTop && nextMidi >= 0 && next < 0.3 && nextMidi - p.midi >= 10 && nextMidi - p.midi <= 16 && p.midi < 50) pop = false;
  }
  // (PJ_SLAP_FORCE, for measuring: 1 every note thumbed, 2 every note popped.)
  if (force_ == 1) pop = false;
  if (force_ == 2) pop = true;
  if (ghost && !force_) {
    p.tech = kGhost;
    p.pre = pop && p.midi >= 38 ? 0.010 : 0.0015;
    rise_[p.midi] = pop && p.midi >= 38 ? 0.010 : 0;
    return;
  }
  if (!force_ && p.prevHeld && !chordAny && gap < 0.2 && std::abs(d) >= 1 && std::abs(d) <= 3 && v <= lastVel_ + 10) {
    p.tech = d > 0 ? kHammer : kPull;
    p.pre = 0;
  } else if (pop) {
    p.tech = kPop;
    // The hook: 12-25 ms, slower on soft pops.
    p.pre = (0.025 - 0.012 * v / 127) * (0.9 + 0.2 * ur2());
    rise_[p.midi] = p.pre;
  } else {
    p.tech = kThumb;
    p.pre = 0.0015;
  }
  lastMidi_ = p.midi;
  lastT_ = p.t;
  lastVel_ = p.vel;
  meanVel_ += 0.15 * (v - meanVel_);
  if (p.tech == kThumb) { thumbMidi_ = p.midi; thumbT_ = p.t; }
}

void SlapBassPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    // Each key decided 30 ms before the hook may begin, struck when its time comes (its hook
    // that much early), in the order they came; a release at its time.
    for (size_t k = 0; k < pend_.size(); ++k)
      if (pend_[k].on && !pend_[k].decided && now_ - pend_[k].t >= wait_ - 0.03) decide(k);
    while (!pend_.empty()) {
      const Pend& p = pend_.front();
      const double age = now_ - p.t;
      if (p.on) {
        // (Keys that came together are struck together, when the one that needs longest
        // begins; the others wait in their stroke: the thumb starts further off, the hook is slower.)
        size_t g = 1;
        double pre = p.pre;
        bool ready = p.decided;
        for (; g < pend_.size() && pend_[g].on && pend_[g].t - p.t <= 0.03; ++g) {
          pre = std::max(pre, pend_[g].pre);
          ready = ready && pend_[g].decided;
        }
        if (!ready || age < wait_ - pre) break;
        for (size_t j = 0; j < g; ++j) {
          tech_[pend_[j].midi] = pend_[j].tech;
          delay_[pend_[j].midi] = wait_ > 0 ? pre - pend_[j].pre : 0.0;
          BassLinePlayer::noteOn(pend_[j].midi, pend_[j].vel);
        }
        pend_.erase(pend_.begin(), pend_.begin() + g);
        continue;
      } else {
        if (age < wait_) break;
        BassLinePlayer::noteOff(p.midi);
      }
      pend_.erase(pend_.begin());
    }
    BassLinePlayer::render(out + i, m);
    now_ += m / fs_;
    if (log_) {
      // (PJ_SLAP_LOG: the fret contacts, block by block: time, string, samples touching, frets.)
      for (int s = 0; s < kStrings; ++s) {
        uint32_t w = 0;
        const int h = m_.bass().fretHits(s, &w);
        if (h) say(stderr, "fret %.5f %d %d %x\n", now_, s, h, w);
      }
    }
  }
  if (!pend_.empty()) blowing_ = true;
}

void SlapBassPlayer::begin(int count) {
  chord_ = count;
  chordIdx_ = 0;
}

double SlapBassPlayer::cost(int s, int fret, const Asked& a) {
  const int t = tech_[std::clamp(a.midi, 0, 127)];
  // (A double stop: leave the strings above for the notes above.)
  if (s > kStrings - chord_ + chordIdx_) return 20;
  if (t == kPop) {
    // (GM 38 keeps the hand up the neck to pop on the D and G strings; GM 37 a little.)
    if (s < 2) return prog_ == 2 ? 8.0 : 3.0;
    return prog_ == 2 ? 0.15 * std::max(0, fret - 9) : 0.0;
  }
  if ((t == kHammer || t == kPull) && s == last_ && held_[s]) return -10;
  return 0;
}

void SlapBassPlayer::strike(int s, int fret, const Asked& a) {
  Guitar& g = m_.bass();
  const int midi = std::clamp(a.midi, 0, 127);
  int t = tech_[midi];
  const double v = std::clamp(a.velocity, 1, 127) / 127.0;
  const double dynK = 0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0);
  const int idx = chordIdx_++;
  const double L = g.spec().scale;
  if (log_) say(stderr, "strike %.4f key %d v %d string %d fret %d tech %d\n", now_, midi, a.velocity, s, fret, t);
  // Slurs: only on the string still ringing, the fret moved.
  if (t == kHammer || t == kPull) {
    if (s == last_ && held_[s] && fret != lastFret_ && (t == kHammer) == (fret > lastFret_)) {
      const int from = lastFret_;
      g.fret(s, fret);
      g.mute(s, 0);
      const double len = L * std::exp2(-fret / 12.0);
      if (t == kHammer) {
        // The fingertip comes down on the string just behind its fret: a small knock (5 g at
        // ~1 m/s); mirrored to the saddle's end (the modes' shapes are the same there).
        g.thump(s, (0.6 + 0.6 * v) * dynK, 0.010, 1.45, 0.006, 0.005, 2e4, 40, 0.0003);
        ++count_[3];
      } else {
        // The finger leaving its fret pulls the string a little aside as it goes.
        const double x = len - L * std::exp2(-from / 12.0);
        g.pluckPad(s, (0.5e-3 + 1.0e-3 * v) * dynK, std::max(0.02, x), 0.3, 0.008, 0.002, 0.0006);
        ++count_[4];
      }
      return;
    }
    t = s >= 2 && (prog_ == 2 ? midi >= 38 : false) ? kPop : kThumb;
  }
  if (t == kPop && s < 2) t = kThumb;
  g.mute(s, 0);
  if (t == kGhost) {
    // The fretting hand lies on the string without pressing it (where it would have fretted):
    // what sounds is the stroke and the frets, no pitch.
    const double at = L * std::exp2(-std::max(double(fret), hand_) / 12.0) - 0.01;
    g.fret(s, 0);
    g.mute(s, knob("PJ_SLAP_GHOSTMUTE", 200), std::max(0.05, at));
    ++count_[2];
    if (rise_[midi] > 0 && s >= 2) {
      g.pop(s, (0.8e-3 + 0.8e-3 * v) * dynK, 0.21 + 0.01 * ur(), -1.45, 0.006, rise_[midi] + delay_[midi], 0.0003);
    } else {
      const double sp = (0.8 + 0.8 * v) * dynK * thumbV_;
      g.thump(s, sp, thumbAt_ + 0.016 * (ur() - 0.5), 1.3, 0.015, 0.02, 3e4, 30, 0.002 + sp * delay_[midi], flesh_);
    }
    return;
  }
  g.fret(s, fret);
  if (t == kPop) {
    // The pop: the finger hooks under the string between the neck's end and the neck pickup,
    // pulls it out from the body and lets it slip off its callus; it slams back onto the frets.
    const double depth = (prog_ == 2 ? 0.6e-3 + 6.5e-3 * fastPow(v, 1.6) : 0.5e-3 + 6.0e-3 * fastPow(v, 1.6)) * dynK *
                         (0.9 + 0.2 * ur());
    const double from = std::clamp((prog_ == 2 ? 0.20 : 0.215) * std::exp2(0.35 * (1 - brightness_)) + 0.02 * (ur() - 0.5), 0.12, 0.3);
    const double angle = -1.45 + 0.16 * (ur() - 0.5);
    const double width = prog_ == 2 ? 0.003 : 0.006;
    const double slip = prog_ == 2 ? 0.0003 - 0.0001 * v : 0.00035 - 0.0002 * v;
    double rise = (rise_[midi] > 0 ? rise_[midi] : 0.02) + delay_[midi];
    if (idx > 0) rise += 0.003 + 0.005 * ur();  // (a double stop: the second finger a little after)
    g.pop(s, depth, from, angle, width, rise, slip);
    ++count_[1];
    return;
  }
  // The thumb: down (the bony side of the joint, 15 mm), or up (the nail's side, 8 mm, weaker)
  // when thumbed notes come within 110 ms of each other.
  up_ = now_ - lastThumbAt_ < 0.11 ? !up_ : false;
  lastThumbAt_ = now_;
  // (The thinner strings struck as hard sound weaker, 10 dB from the E to the G: the player
  // digs in a little more on them, kLight^PJ_SLAP_THUMBLIGHT (0.5): their lightness to the 1/8.)
  static const double kLight[kStrings] = {1.0, 1.11, 1.23, 1.47};
  const double speed = (0.4 + 2.9 * fastPow(v, 1.4)) * thumbV_ * dynK * (0.93 + 0.14 * ur()) * (up_ ? 0.65 : 1.0) *
                       fastPow(kLight[s], thumbLight_);
  const double from = std::clamp(thumbAt_ * std::exp2(0.35 * (1 - brightness_)) + 0.016 * (ur() - 0.5), 0.15, 0.4);
  const double angle = up_ ? -thumbAngle_ + 0.16 * (ur() - 0.5) : thumbAngle_ + 0.16 * (ur() - 0.5);
  // A slow heavy note: a rest stroke (the thumb goes on and lies on the next string).
  const bool rest = !up_ && s < 3 && sinceNote_ > 0.35 && v > 0.6 && chord_ == 1;
  g.thump(s, speed, from, angle, up_ ? 0.008 : 0.015, 0.02, 3e4, rest ? 8 : 30, 0.002 + speed * delay_[midi], flesh_);
  if (rest && !held_[s + 1]) g.damp(s + 1, 80);
  ++count_[up_ ? 5 : 0];
}

}  // namespace pajupilli
