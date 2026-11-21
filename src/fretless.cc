#include "fretless.h"
#include "heard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "band.h"
#include "instrument.h"
#include "knob.h"

namespace pajupilli {

namespace {
constexpr int kOpen[4] = {28, 33, 38, 43};  // E1 A1 D2 G2
constexpr int kTopStopped = 43 + 20;        // D#4: the G string at the board's end
// The band's pull is followed by its weight w as w / (w + kHalf): a bare third under the bass
// (w ~0.007) nearly all of it, a fifth's octave alone (~0.0017) three quarters.
constexpr double kHalf = 0.0005;
}  // namespace

bool fretlessBassInstrument(const Instrument& inst) { return std::strcmp(inst.name, "fretless-bass") == 0; }

FretlessBassModel::FretlessBassModel(const Instrument& inst) : Tube(inst), g_(inst.fs, fretlessBass()) {
  // The DI as the Jazz Bass's (src/ebass.cc): the coils with the cable's 700 pF and the pots into
  // 1 MOhm, a biquad; or (PJ_EBASS_DI unset) the amp, told the same.
  const GuitarSpec& s = g_.spec();
  const GuitarSpec::Pickup& p = s.pickups[s.select[0]];
  const int n = s.select[1] >= 0 ? 2 : 1;
  const double coilC = p.C * n, pots = s.pots, cable = knob("PJ_EBASS_CABLE", 700e-12);
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
  // The chorus: 7 +- 2 ms, 0.6 Hz, a triangle; the wet path above 150 Hz.
  chorus_ = std::clamp(knob("PJ_FRETLESS_CHORUS", 0), 0.0, 1.0);
  if (chorus_ > 0) {
    line_.assign(1024, 0.0f);
    lfoStep_ = 0.6 / inst.fs;
    hpA_ = std::exp(-2 * M_PI * 150 / inst.fs);
  }
  if (knob("PJ_EBASS_DI", 0) > 0) return;
  amp_.reset(new GuitarAmp(inst.fs, GuitarAmp::fretlessDi()));
  amp_->connect({L, R, coilC, pots}, cable);
}

void FretlessBassModel::resetAir() {
  Tube::resetAir();
  g_.reset();
  z1_ = z2_ = 0;
  hpX_ = hpY_ = 0;
  std::fill(line_.begin(), line_.end(), 0.0f);
  if (amp_) amp_->reset();
}

const float* FretlessBassModel::takeSide(int n) {
  if (chorus_ <= 0) return nullptr;
  sideOut_.assign(n, 0.0f);
  const int m = std::min(n, int(side_.size()));
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

// The strings' sample: the tests' kick (testDrive) goes into them; a non-finite state they caught
// and cleared clears the rest too (the amp, the filters), counted as a reset.
float FretlessBassModel::strings() {
  if (testDp_ != 0) { g_.kick(float(testDp_)); testDp_ = 0; }
  const float x = g_.sample();
  if (g_.nanResets() != nanSeen_) [[unlikely]] {
    nanSeen_ = g_.nanResets();
    resetAir();
    ++resets_;
  }
  return x;
}

float FretlessBassModel::sample() {
  const double x = strings();
  double y;
  if (amp_) {
    y = amp_->process(float(x)) * out_;
  } else {
    y = b0_ * x + z1_;
    z1_ = b1_ * x - a1_ * y + z2_;
    z2_ = b2_ * x - a2_ * y;
    y *= out_;
  }
  if (chorus_ > 0) {
    // A bucket brigade's sweep: the wet path (its lows taken off) read at 7 +- 2 ms, the left
    // and the right sweeping against each other.
    hpY_ = hpA_ * (hpY_ + y - hpX_);
    hpX_ = y;
    const int N = int(line_.size());
    line_[pos_] = float(hpY_);
    lfo_ += lfoStep_;
    if (lfo_ >= 1) lfo_ -= 1;
    const double tri = 4 * std::fabs(lfo_ - 0.5) - 1;
    auto tap = [&](double ms) {
      const double d = ms * 1e-3 * instrument().fs;
      const int i = int(d);
      const double f = d - i;
      const float a = line_[(pos_ - i + N) % N], b = line_[(pos_ - i - 1 + N) % N];
      return a + f * (b - a);
    };
    const double wl = tap(7 + 2 * tri), wr = tap(7 - 2 * tri);
    pos_ = (pos_ + 1) % N;
    const double mix = 0.35 * chorus_;
    side_.push_back(float(outScale() * mix * 0.5 * (wl - wr)));
    y += mix * 0.5 * (wl + wr);
  }
  last_ = float(y);
  return finish(last_);
}

FretlessBassPlayer::FretlessBassPlayer(FretlessBassModel& m) : Performer(m), m_(m) {
  // (The strings not played kept quiet, as the Jazz Bass's player keeps them.)
  for (int s = 0; s < kStrings; ++s) m.bass().damp(s, 40);
  wait_ = noteLeadsApplied() ? noteLead(m.instrument()) : knob("PJ_FRETLESS_WAIT", 0);
  errSd_ = std::max(0.0, knob("PJ_FRETLESS_ERR", 8));
  slides_ = knob("PJ_FRETLESS_SLIDES", 1);
  scoop_ = knob("PJ_FRETLESS_SCOOP", 1);
  ji_ = knob("PJ_FRETLESS_JI", 1);
  lim_ = knob("PJ_FRETLESS_JI_LIM", 15);
  vibDepth_ = knob("PJ_FRETLESS_VIB", 1);
  harmonics_ = knob("PJ_FRETLESS_HARMONICS", 0) > 0.5;
  log_ = int(knob("PJ_FRETLESS_LOG", 0));
  string_ = int(knob("PJ_FRETLESS_STRING", 0));
  pad_ = knob("PJ_FRETLESS_PAD", 0.018);
  snap_ = knob("PJ_FRETLESS_SNAP", 0);
  rise_ = knob("PJ_FRETLESS_RISE", 1.8);
  slip_ = knob("PJ_FRETLESS_SLIP", 3.5);
  angle_ = knob("PJ_FRETLESS_ANGLE", 0.65);
  place_ = knob("PJ_FRETLESS_PLACE", 0.09);
  seed(0);
}

FretlessBassPlayer::~FretlessBassPlayer() {
  if (log_)
    std::fprintf(stderr, "fretless: %ld struck, %ld slid; board contact %ld of %ld samples (%.3f %%), the last at %.3f s\n", struck_, slid_,
                 contact_, samples_, 100.0 * contact_ / std::max(1L, samples_), lastContact_);
}

void FretlessBassPlayer::seed(unsigned s) {
  rng_ = 0x9E3779B97F4A7C15ull * (s + 1) ^ 9001u;
  // The player's own: a bias (0 +- 3 c), his vibrato's rate (4.5-5.5 Hz).
  bias_ = errSd_ > 0 ? 3 * (2 * ur() - 1) : 0.0;
  vibRate_ = 4.6 + 0.8 * ur();
}

double FretlessBassPlayer::ur() {
  // (xorshift64*: the landings' statistics want better than a 32-bit LCG's low-order runs)
  rng_ ^= rng_ >> 12;
  rng_ ^= rng_ << 25;
  rng_ ^= rng_ >> 27;
  return ((rng_ * 2685821657736338717ull) >> 11) * (1.0 / 9007199254740992.0);
}

double FretlessBassPlayer::gauss() {
  const double a = std::max(1e-9, ur()), b = ur();
  return std::sqrt(-2 * std::log(a)) * std::cos(2 * M_PI * b);
}

void FretlessBassPlayer::noteOn(int midi, int velocity) {
  pend_.push_back({std::clamp(midi, 0, 127), std::clamp(velocity, 1, 127), t_, true});
  note_ = midi;
  blowing_ = true;
}

void FretlessBassPlayer::noteOff(int midi) { pend_.push_back({std::clamp(midi, 0, 127), 0, t_, false}); }

double FretlessBassPlayer::tuning(int key) const {
  for (const Line& l : line_)
    if (l.midi == key && l.fret > 0 && !l.harmonic) return 100 * (l.pos - l.fret);
  return 0;
}

// A natural harmonic for `midi` (research/fretless_bass.md 3.5): string and n (2-6: the 12th, 7th,
// 5th, 4th, 3rd positions) whose harmonic lies within 20 c of it, the nearest (the lower n on a
// tie); -1: none.
int FretlessBassPlayer::harmonicFor(int midi, int* n) const {
  int best = -1;
  double err = 21;
  for (int s = kStrings - 1; s >= 0; --s)
    for (int k = 2; k <= 6; ++k) {
      const double c = 1200 * std::log2(double(k)) - 100 * (midi - kOpen[s]);
      if (std::fabs(c) < err - 0.5) { err = std::fabs(c); best = s; *n = k; }
    }
  return best;
}

// Was the key before held on over this one for 40 ms or more? (What the player has seen of it:
// the keys come wait_ early; live, a key still down is legato enough.)
bool FretlessBassPlayer::overlapped(size_t k, int prevMidi) const {
  if (wait_ < 0.035) return true;
  for (size_t j = k + 1; j < pend_.size(); ++j)
    if (!pend_[j].on && pend_[j].midi == prevMidi) return pend_[j].t - pend_[k].t >= 0.04;
  return true;
}

double FretlessBassPlayer::bandPull(int s, int key, double* weight) const {
  *weight = 0;
  if (!band_ || ji_ <= 0) return 0;
  // The bass's own partials: its string's, stiffness and all.
  const double B = m_.bass().spec().str[s].B;
  const double f0 = 440 * std::exp2((key - 69) / 12.0);
  double pl[16], pw[16], pk[16];
  int np = 0;
  for (int k = 1; k <= 16 && k * f0 < 5000; ++k) {
    pl[np] = std::log2(k * std::sqrt(1 + B * k * k));
    pw[np] = bandPartial(kBandBass, f0, k);
    pk[np] = k;
    ++np;
  }
  BandListener me;
  me.self = self_;
  me.f0 = f0;
  me.pl = pl;
  me.pw = pw;
  me.pk = pk;
  me.np = np;
  // (its note's onset: the order of hearing among the parts that tune by ear, src/band.h)
  if (const BandNote* own = band_->find(self_, line_[s].midi)) {
    me.at = own->at;
    me.pitch = own->pitch;
  }
  return pajupilli::bandPull(*band_, me, weight);
}

// Where the finger means to be (c): the band's pull, the note's residue (halved under the
// vibrato, which hides it), the player's bias.
double FretlessBassPlayer::aim(const Line& l) const {
  const bool vib = l.held && l.age > l.vibDelay;
  return std::clamp(l.band + (vib ? 0.5 : 1.0) * l.residue + bias_ - (vib && errSd_ > 0 ? 2.0 : 0.0), -lim_, lim_);
}

void FretlessBassPlayer::start(size_t k) {
  const Pend& p = pend_[k];
  Guitar& g = m_.bass();
  int midi = p.midi;
  while (midi < kOpen[0]) midi += 12;
  int hn = 0, hs = -1;
  if (midi > kTopStopped || (harmonics_ && midi >= 60)) hs = harmonicFor(midi, &hn);
  if (hs < 0)
    while (midi > kTopStopped) midi -= 12;
  const double v = p.vel / 127.0;
  // A slide: the key before still down over this one, the same string, at most 4 semitones.
  if (hs < 0 && last_ >= 0) {
    Line& o = line_[last_];
    const int d = midi - o.key, nf = midi - kOpen[last_];
    if (o.held && !o.harmonic && o.fret > 0 && d != 0 && std::abs(d) <= 4 && nf >= 1 && nf <= g.frets() && slides_ > 0 &&
        !jazzBandFile() && overlapped(k, o.midi)) {
      const double prob = slides_ >= 2 ? 1.0 : slides_ * (std::abs(d) <= 2 ? 0.6 : 0.3);
      if (ur() < prob) {
        const int s = last_;
        Line& l = line_[s];
        l.slFrom = l.pos;
        l.slT = 0;
        l.slDur = std::min(0.3, 0.06 + 0.035 * std::abs(d));
        l.slide = true;
        l.midi = p.midi;
        l.key = midi;
        l.fret = nf;
        l.age = 0;
        l.vibDelay = 0.2 + 0.15 * ur();
        double w;
        const double want = bandPull(s, midi, &w);
        l.band = l.bandTo = std::clamp(want * w / (w + kHalf), -lim_, lim_);
        l.bandMix = w;
        l.lastSolve = t_;
        l.residue = errSd_ > 0 ? (2 * ur() - 1) * 0.8 * (2 + 0.3 * errSd_) : 0.0;
        l.slErr = errSd_ > 0 ? 0.5 * errSd_ * gauss() : 0.0;
        l.zeta = 0.6;
        l.wn = 28;
        // (A slow, hard note: the plucking finger adds a light stroke as the slide begins.)
        if (sinceNote_ > 0.3 && p.vel > 90) g.pluckPad(s, (0.3e-3 + 1.2e-3 * v) * 0.6, 0.09, 0.55, 0.010, 0.003, 0.0012);
        ++slid_;
        sinceNote_ = 0;
        hand_ = std::clamp(hand_ + 0.5 * (nf - 1 - hand_), 1.0, 14.0);
        if (log_)
          say(stderr, "fretless slide t=%.3f key %d string %d fret %d over %.0f ms band %+.1f (w %.4f)\n", t_, p.midi, s, nf,
                       1000 * l.slDur, l.band, w);
        return;
      }
    }
  }
  // The string: near the hand, low on the neck; open strings only in quick lines (they cannot
  // sing: no vibrato, no scoop, no slide) and not at all once the pitch wheel is in use.
  int best = hs;
  int fret = 0;
  if (best < 0) {
    double cost = 1e9;
    for (int s = 0; s < kStrings; ++s) {
      const int f = midi - kOpen[s];
      if (f < 0 || f > g.frets()) continue;
      if (sinceNote_ < 0.03 && line_[s].held && line_[s].age < 0.03) continue;  // (a double stop: taken)
      double c = f == 0 ? (sinceNote_ < 0.25 ? -0.3 : 3.0) + (bendSeen_ ? 3.0 : 0.0) : std::fabs(f - hand_) + 0.12 * f;
      if (f > 0 && std::fabs(f - hand_) > 4) c += 2.5;
      if (line_[s].held) c += 6;
      if (string_ > 0 && s != string_ - 1) c += 100;  // (PJ_FRETLESS_STRING: tests)
      if (c < cost) { cost = c; best = s; }
    }
    if (best < 0) return;
    fret = midi - kOpen[best];
  }
  const int s = best;
  // One voice: the last string stops as this one is plucked, unless they came together.
  if (last_ >= 0 && last_ != s && !hold_ && sinceNote_ > 0.03) {
    g.damp(last_, 40);
    line_[last_].held = false;
  }
  Line& l = line_[s];
  l = Line();
  l.midi = p.midi;
  l.key = hs >= 0 ? p.midi : midi;
  l.held = true;
  l.harmonic = hn;
  l.fret = hs >= 0 ? 0 : fret;
  l.vibDelay = 0.2 + 0.15 * ur();
  l.vibPhase = 2 * M_PI * ur();
  l.vibRate = vibRate_ * (0.95 + 0.1 * ur());
  double e0 = 0, w = 0;
  bool scooped = false;
  if (l.fret > 0) {
    // The landing (3.1, 3.2): the human error; on slower, harder notes now and then a scoop from
    // 15-35 c flat (PJ_FRETLESS_SCOOP 2: every stopped note, 25-35 c), the finger sliding up into
    // the note, past it and back: a lightly damped approach. Not while the file bends its own.
    e0 = errSd_ > 0 ? errSd_ * gauss() : 0.0;
    double pScoop = 0;
    if (scoop_ >= 2) pScoop = 1;
    else if (scoop_ > 0 && !bendSeen_ && p.vel >= 80) pScoop = scoop_ * (sinceNote_ >= 0.35 ? 0.3 : sinceNote_ >= 0.2 ? 0.1 : 0.0);
    if (pScoop > 0 && ur() < pScoop) {
      scooped = true;
      e0 += scoop_ >= 2 ? -(25 + 10 * ur()) : -(15 + 20 * ur());
      l.zeta = 0.28;
      l.wn = 36;
    } else {
      l.zeta = 0.6;
      l.wn = 28;
    }
    const double want = bandPull(s, midi, &w);
    l.band = l.bandTo = std::clamp(want * w / (w + kHalf), -lim_, lim_);
    l.bandMix = w;
    l.lastSolve = t_;
    l.bandVersion = band_ ? band_->version() : 0;
    l.residue = errSd_ > 0 ? (2 * ur() - 1) * 0.8 * (2 + 0.3 * errSd_) : 0.0;
    l.x = aim(l) + e0;
    l.xv = 0;
    l.pos = std::max(0.3, l.fret + (l.x + 200 * roll_) / 100);
    g.stop(s, l.pos);
  } else {
    l.pos = 0;
    g.stop(s, 0);
  }
  // The pluck: index and middle in turn, a rest stroke between the pickups nearer the bridge,
  // more into the board than a fretted player's (the into-board share sets the mwah), and round:
  // the fingertip's pad (18 mm, PJ_FRETLESS_PAD) rolling off the string over 3-5 ms
  // (PJ_FRETLESS_SLIP x the Jazz Bass player's; no snap off the fingertip's edge, PJ_FRETLESS_SNAP).
  // The stroke gives the string its low partials; the board, as it grazes, gives it the upper ones
  // a moment later: the bloom (FluidR3's recorded fretless: h3-h11 from 30-45 dB down at the attack
  // to 15-25 dB down at 0.2-0.4 s). A bright, snapping stroke leaves the board nothing to add.
  finger_ = sinceNote_ < 0.4 ? 1 - finger_ : 0;
  const int finger = finger_;
  double depth = (0.4e-3 + 4.6e-3 * std::pow(v, 1.4)) * (0.7 + 0.6 * std::clamp(dyn_, 0.0, 1.0)) * (finger ? 1.06 : 1.0) * (0.93 + 0.14 * ur());
  const double from = std::clamp(place_ * std::pow(2.0, 1 - brightness_) * (1.05 - 0.15 * v) + (finger ? 0.006 : 0.0) + 0.008 * (ur() - 0.5), 0.04, 0.3);
  const double angle = angle_ - 0.1 + 0.2 * ur();
  if (hn) {
    // A natural harmonic: the fingertip on the node from the stroke to 40-60 ms after; dug in harder.
    depth *= 1.6;
    g.touch(s, hn, 0.04 + 0.02 * ur(), 300, 0.004, (ur() - 0.5) * 0.0014);
  }
  g.pluckPad(s, depth, from, angle, pad_, (0.002 + 0.002 * (1 - v) - 0.0008 * snap_ * v * v * v) * rise_,
             (0.0008 + 0.0006 * (1 - v) - 0.0005 * snap_ * v * v * v * v) * slip_);
  if (log_)
    say(stderr, "fretless note t=%.3f key %d v %d string %d fret %d%s e0 %+.1f%s band %+.1f (w %.4f) residue %+.1f bias %+.1f\n", t_,
                 p.midi, p.vel, s, l.fret, hn ? " harmonic" : "", e0, scooped ? " scoop" : "", l.band, w, l.residue, bias_);
  ++struck_;
  last_ = s;
  sinceNote_ = 0;
  if (l.fret > 0) hand_ = std::clamp(hand_ + 0.5 * (l.fret - 1 - hand_), 1.0, 14.0);
}

void FretlessBassPlayer::release(int midi) {
  for (int s = 0; s < kStrings; ++s)
    if (line_[s].midi == midi && line_[s].held) {
      line_[s].held = false;
      // (The fretting hand eases off and the plucking hand lies on the string: stopped.)
      if (!hold_) m_.bass().damp(s, 40);
    }
}

// The stopping finger, each block: the band's pull followed, the landing corrected, the slide,
// the vibrato, the pitch wheel; to Guitar::stop.
void FretlessBassPlayer::steer(int s, double dt) {
  Line& l = line_[s];
  if (l.midi < 0) return;
  l.age += dt;
  if (l.fret <= 0 || l.harmonic) return;  // (the nut, or a harmonic: nothing to steer)
  // The band, heard again when it changes and every 50 ms.
  if (band_ && ji_ > 0 && (band_->version() != l.bandVersion || t_ - l.lastSolve > 0.05)) {
    l.bandVersion = band_->version();
    l.lastSolve = t_;
    double w;
    const double want = std::clamp(bandPull(s, l.key, &w) * w / (w + kHalf), -lim_, lim_);
    l.bandMix = w;
    // (A change under 2 c let be; with next to nothing to hear, back to the tempered pitch.)
    if (w >= 3e-4 && std::fabs(want - l.bandTo) >= 2) l.bandTo = want;
  }
  {
    const double d = l.bandTo - l.band;
    if (l.bandMix < 3e-4) {
      l.band -= l.band * std::min(1.0, dt / 3.0);  // (healing, ~3 s)
      if (std::fabs(l.bandTo) >= 2) l.bandTo = 0;
    } else if (d != 0) {
      // At most 1 c/ms, no change quicker than 25 ms.
      const double step = std::min(1000.0, std::max(std::fabs(d) / 0.025, 40.0)) * dt;
      l.band = std::fabs(d) <= step ? l.bandTo : l.band + std::copysign(step, d);
    }
  }
  const double a = aim(l);
  double cents;
  if (l.slide) {
    l.slT += dt;
    const double u = std::min(1.0, l.slT / l.slDur);
    const double to = l.fret + (a + l.slErr) / 100;
    l.pos = l.slFrom + (to - l.slFrom) * (0.5 - 0.5 * std::cos(M_PI * u));
    if (u >= 1) {
      l.slide = false;
      l.x = a + l.slErr;
      l.xv = 0;
    }
    cents = 100 * (l.pos - l.fret);
  } else {
    // The correction: a second-order approach to the aim.
    const double acc = l.wn * l.wn * (a - l.x) - 2 * l.zeta * l.wn * l.xv;
    l.xv += acc * dt;
    l.x += l.xv * dt;
    cents = l.x;
  }
  // The vibrato: after its delay on a held note, in over 0.2 s; wider up the neck; the director's
  // cue and the mod wheel.
  double depth = 10 * vibDepth_ * (1 + 0.25 * l.fret / 12.0);
  double rate = l.vibRate;
  if (cued_) {
    depth *= std::clamp(double(cue_.v[kCueVibDepth]), 0.0, 2.5);
    rate *= std::clamp(double(cue_.v[kCueVibRate]), 0.6, 1.5);
  }
  const double tv = l.age - l.vibDelay;
  const double env = l.held && tv > 0 ? (tv < 0.2 ? 0.5 - 0.5 * std::cos(M_PI * tv / 0.2) : 1.0) : 0.0;
  l.vibPhase += 2 * M_PI * rate * dt;
  if (l.vibPhase > 2 * M_PI) l.vibPhase -= 2 * M_PI;
  const double vib = (depth * env + 25 * std::clamp(flutter_, 0.0, 1.0) * (l.held ? 1.0 : 0.0)) * std::sin(l.vibPhase);
  const double pos = std::max(0.3, (l.slide ? l.pos : l.fret + cents / 100) + (vib + 200 * roll_) / 100);
  if (!l.slide) l.pos = l.fret + cents / 100;
  m_.bass().stop(s, pos);
  if (log_ >= 2 && s == last_) say(stderr, "finger t=%.4f string %d cents %+.2f band %+.2f to %+.2f at %.4f\n", t_, s, 100 * (pos - l.fret), l.band, l.bandTo, pos);
}

void FretlessBassPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  Guitar& g = m_.bass();
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double dt = m / fs_;
    // The keys, in the order they came, each when its time comes.
    while (!pend_.empty() && t_ - pend_.front().t >= wait_) {
      if (pend_.front().on) start(0);
      else release(pend_.front().midi);
      pend_.erase(pend_.begin());
    }
    if (std::fabs(roll_) > 0.02) bendSeen_ = true;
    for (int s = 0; s < kStrings; ++s) steer(s, dt);
    tick(dt);
    t_ += dt;
    sinceNote_ += dt;
    m_.process(out + i, m);
    for (int s = 0; s < kStrings; ++s) {
      const int h = g.boardHits(s);
      if (h) { contact_ += h; lastContact_ = t_; }
    }
    samples_ += m;
  }
  blowing_ = !g.asleep() || !pend_.empty();
}

}  // namespace pajupilli
