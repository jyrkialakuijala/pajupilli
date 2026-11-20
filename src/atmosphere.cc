#include "atmosphere.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "knob.h"

namespace pajupilli {

bool fxAtmosphere(const Instrument& inst) { return std::strcmp(inst.name, "fx-atmosphere") == 0; }

namespace {

constexpr double kTwoPi = 6.283185307179586;
// The places along the string (from the bridge, over its length): the plectrum, the pickup (its
// window's width), the magnet's pole.
constexpr double kPluckAt = 0.27, kPickupAt = 0.11, kPickupWidth = 0.02, kMagnetAt = 0.16;
constexpr double kPlectrumWidth = 0.015;
constexpr double kStiff = 4e-5;  // (a thin steel string's inharmonicity B)
// The string's losses: sigma = b1 + b3 w^2 (air and the board at the bottom, the wire's own
// friction at the top; Chaigne and Askenfelt's form). b1 from T60 ~9 s at middle C.
constexpr double kB3 = 3e-8;
// The loop: its gain over the string's own loss of the mode it drives (kappa: the growth
// (kappa - 1) sigma once the magnet is on), the amplifier's tuning (Q), the magnet's corner (its
// inductance over its resistance), its bias (the force's square-law term at the full current).
constexpr double kKappa = 4, kQ = 6, kCoilHz = 3000, kBias = 0.3;
constexpr double kLevel = 0.6;
constexpr int kMaxNotes = 24;

double shapeAt(int j, double x, double width) {
  // sin(j pi x), averaged over a window (the plectrum's leather, the pickup's pole)
  const double a = j * M_PI * width / 2;
  return std::sin(j * M_PI * x) * (a > 1e-9 ? std::sin(a) / a : 1.0);
}

void bandPass(AtmosphereModel::Biquad& b, double fc, double q, double fs) {
  const double w0 = kTwoPi * std::min(fc, 0.45 * fs) / fs, al = std::sin(w0) / (2 * q), a0 = 1 + al;
  b.b0 = al / a0;
  b.b2 = -al / a0;
  b.a1 = -2 * std::cos(w0) / a0;
  b.a2 = (1 - al) / a0;
}

}  // namespace

AtmosphereModel::AtmosphereModel(const Instrument& inst) : Tube(inst) {
  // (the knobs' starting places; live's K1-K4 and -seq's "knob K X" move them)
  sustain_ = std::clamp(knob("PJ_ATMO_SUSTAIN", 0.5), 0.0, 1.0);
  harmonic_ = std::clamp(knob("PJ_ATMO_HARMONIC", 0.5), 0.0, 1.0);
  pluck_ = std::clamp(knob("PJ_ATMO_PLUCK", 0.5), 0.0, 1.0);
  chorus_ = std::clamp(knob("PJ_ATMO_CHORUS", 0.5), 0.0, 1.0);
}

void AtmosphereModel::resetAir() {
  Tube::resetAir();
  notes_.clear();
  side_.clear();
  dcIn_ = dcOut_ = dcSideIn_ = dcSideOut_ = 0;
}

const float* AtmosphereModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

// The rotations (bent) and the amplifier's tuning (f0: the course's note, unbent).
void AtmosphereModel::tune(String& s, double f0) {
  const double fs = instrument().fs;
  for (int j = 0; j < s.modes; ++j) {
    Mode& m = s.mode[j];
    m.w = kTwoPi * std::min(m.f * bk_, 0.47 * fs);
    m.c = std::cos(m.w / fs);
    m.s = std::sin(m.w / fs);
  }
  bandPass(s.bp1, f0 * bk_, kQ, fs);
  bandPass(s.bp2, 2 * f0 * std::sqrt(1 + 4 * kStiff) * bk_, kQ, fs);
}

void AtmosphereModel::setBend(double semitones) {
  if (semitones == bend_) return;
  bend_ = semitones;
  bk_ = std::exp2(semitones / 12);
  for (Note& n : notes_)
    for (String& s : n.str) tune(s, 440 * std::exp2((n.midi - 69) / 12.0));
}

// Each mode's decay a sample: its own, and the felt's once the key is up.
void AtmosphereModel::damp(Note& n) {
  const double fs = instrument().fs;
  for (String& s : n.str) {
    const double felt = n.down ? 0.0 : 6.908 / std::clamp(2.4 * std::pow(s.f / 261.6, -0.25), 0.8, 4.0);
    for (int j = 0; j < s.modes; ++j) {
      Mode& m = s.mode[j];
      m.r = std::exp(-(m.sigma + felt * (1 + 0.15 * j)) / fs);
    }
  }
}

void AtmosphereModel::keyOn(int midi, int velocity) {
  const double fs = instrument().fs;
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  for (Note& n : notes_)
    if (n.midi == midi && n.down) keyOff(midi);
  if (int(notes_.size()) >= kMaxNotes) {  // the oldest released note, else the oldest, gives way
    auto it = std::find_if(notes_.begin(), notes_.end(), [](const Note& n) { return !n.down; });
    notes_.erase(it != notes_.end() ? it : notes_.begin());
  }
  Note n;
  n.midi = midi;
  const double f0 = 440 * std::exp2((midi - 69) / 12.0);
  // The voicing: the top octave's few, short modes are lifted (a gain for the scaling of its
  // strings and board, not modelled).
  n.voice = std::max(1.0, std::pow(f0 / 2093.0, 1.8));
  // The plectrum: it presses the string aside (a softer touch, longer) and slips off it (the
  // harder, the quicker: the brighter; K3 x 3 .. 1/3).
  n.push = (0.008 - 0.005 * v);
  n.slip = (0.0004 - 0.0003 * v) * std::pow(3.0, 1 - 2 * pluck_);
  const double beta = kPluckAt + 0.02 * (urand() - 0.5);
  // The course: three strings a few cents apart (K4 0 .. 2x), the middle one near the note.
  const double d = (0.5 + 1.0 * urand()) * 2 * chorus_;
  const double fr[kStrings] = {f0 * std::exp2(-d / 1200), f0 * std::exp2(0.3 * d * (urand() - 0.5) / 1200),
                               f0 * std::exp2(d / 1200)};
  const double pan[kStrings] = {-0.15, 0.0, 0.15};
  // (the plectrum meets the three strings one after another, a little less each time)
  const double delay[kStrings] = {0.0, 0.0015 + 0.002 * urand(), 0.004 + 0.003 * urand()};
  const double share[kStrings] = {0.85, 1.0, 0.75};
  // (the magnet sits over the middle string; the side strings feel it weaker: the course
  // sustains on the middle string, the others beating with it, slowly)
  const double magnet[kStrings] = {0.35, 1.0, 0.3};
  const double centre = std::clamp((midi - 64) / 40.0, -0.4, 0.4);
  const double t1 = std::clamp(9.0 * std::pow(f0 / 261.6, -0.35), 3.0, 16.0);
  for (int k = 0; k < kStrings; ++k) {
    String& s = n.str[k];
    s.f = fr[k];
    s.pan = std::clamp(centre + pan[k], -1.0, 1.0);
    s.delay = delay[k];
    s.share = share[k];
    s.magnet = magnet[k];
    s.modes = 0;
    for (int j = 1; j <= kModes; ++j) {
      const double f = j * s.f * std::sqrt(1 + kStiff * j * j);
      if (j > 1 && f > std::min(0.42 * fs, 12000.0)) break;
      Mode& m = s.mode[s.modes++];
      m.f = f;
      m.sigma = 6.908 / t1 + kB3 * std::pow(kTwoPi * f, 2);
      m.pl = shapeAt(j, beta, kPlectrumWidth);
      m.pu = shapeAt(j, kPickupAt, kPickupWidth);
      m.dr = shapeAt(j, kMagnetAt, 0.0);
    }
    tune(s, f0);
  }
  // The plectrum's force (per unit modal mass) that presses the string aside by v at its place
  // (the static shape's sum over the modes), and the fundamental's swing that leaves.
  const String& s0 = n.str[1];
  double stat = 0;
  for (int j = 0; j < s0.modes; ++j) stat += s0.mode[j].pl * s0.mode[j].pl / (s0.mode[j].w * s0.mode[j].w);
  n.fpk = v / stat;
  // The amplifiers, two channels into the magnet: the note's band and its octave's, each with
  // its loop gain kappa times its mode's loss. The limits: the note's (the touch, K1) where the
  // saturated drive (its fundamental 4/pi of the limit) holds the fundamental at the pluck's
  // swing; the octave's (K2) where it holds the second mode at a share of that (one channel
  // alone would not do: the fundamental, saturating it, starves the octave).
  // (the bass's magnets set weaker: its pluck rings long by itself)
  n.bass = std::clamp(std::pow(f0 / 261.6, 0.3), 0.5, 1.0);
  for (String& s : n.str) {
    const Mode& m1 = s.mode[0];
    const double a1 = n.fpk * m1.pl / (m1.w * m1.w);
    s.g1 = 2 * kKappa * m1.sigma / (m1.pu * m1.dr);
    s.lim1 = s.magnet * a1 * 2 * m1.sigma * m1.w * M_PI / (4 * m1.dr);
    if (s.modes > 1) {
      const Mode& m2 = s.mode[1];
      s.g2 = 2 * kKappa * m2.sigma / (m2.pu * m2.dr);
      s.lim2 = s.lim1 * (m2.sigma * m2.w * m1.dr) / (m1.sigma * m1.w * m2.dr);  // (a2 = a1)
    }
  }
  notes_.push_back(n);
  damp(notes_.back());
}

void AtmosphereModel::keyOff(int midi) {
  for (Note& n : notes_)
    if (n.midi == midi && n.down) {
      n.down = false;
      damp(n);
    }
}

float AtmosphereModel::sample() {
  const double fs = instrument().fs, h = 1 / fs;
  const double ramp = 1 - std::exp(-1 / (0.02 * fs));        // (the amplifiers switch: 20 ms)
  const double kc = 1 - std::exp(-kTwoPi * kCoilHz / fs);    // (the magnet's inductance)
  const double octave = 0.25 * harmonic_;  // (K2: the octave held at up to a quarter of the note)
  double out = 0, side = 0;
  for (Note& n : notes_) {
    n.amp += ((n.down ? 1.0 : 0.0) - n.amp) * ramp;
    const double rho = 1.2 * sustain_ * n.bass;  // (K1: the fundamental held at 0 .. 1.2 x the pluck's)
    double e2 = 0;
    for (String& s : n.str) {
      // The plectrum: pressing (a raised cosine), then slipping off.
      const double t = n.t - s.delay;
      double fp = 0;
      if (t < 0) fp = 0;
      else if (t < n.push) fp = s.share * n.fpk * 0.5 * (1 - std::cos(M_PI * t / n.push));
      else if (t < n.push + n.slip) fp = s.share * n.fpk * 0.5 * (1 + std::cos(M_PI * (t - n.push) / n.slip));
      const double drive = n.amp * s.drive;
      double o = 0, pick = 0;
      for (int j = 0; j < s.modes; ++j) {
        Mode& m = s.mode[j];
        const double F = m.pl * fp + m.dr * drive;
        const double re = m.r * (m.re * m.c - m.im * m.s);
        const double im = m.r * (m.re * m.s + m.im * m.c) - h * F / m.w;
        m.re = re;
        m.im = im;
        o += (j + 1) * re;          // the slope at the bridge (the force on it)
        pick -= m.pu * m.w * im;    // the velocity under the pickup
        e2 += re * re + im * im;
      }
      // The amplifier: tuned to the note (and its octave), saturating; the magnet's current
      // lags it; the force goes as the flux squared: the current and, by the bias, its square.
      const double imax = rho * s.lim1, imax2 = octave * rho * s.lim2;
      if (imax > 1e-30 && n.amp > 1e-4) {
        double cur = imax * std::tanh(s.g1 * s.bp1.run(pick) / imax);
        if (imax2 > 1e-30) cur += imax2 * std::tanh(s.g2 * s.bp2.run(pick) / imax2);
        s.coil += (cur - s.coil) * kc;
        s.drive = s.coil * (1 + kBias * s.coil / imax);
      } else {
        s.drive = 0;
      }
      out += n.voice * o;
      side += n.voice * s.pan * o;
    }
    n.t += h;
    n.quiet = (!n.down && n.amp < 1e-3 && e2 < 1e-16) ? n.quiet + 1 : 0;
  }
  notes_.erase(std::remove_if(notes_.begin(), notes_.end(), [](const Note& n) { return n.quiet > 256; }), notes_.end());
  // (the magnets' steady pull and the plectrum's push bend the strings slowly: no sound; the
  // board radiates little below ~20 Hz)
  const double a = 1 - kTwoPi * 20 / fs;
  dcOut_ = a * dcOut_ + out - dcIn_;
  dcIn_ = out;
  dcSideOut_ = a * dcSideOut_ + side - dcSideIn_;
  dcSideIn_ = side;
  side_.push_back(float(outScale() * kLevel * dcSideOut_));
  last_ = float(kLevel * dcOut_);
  return finish(last_);
}

// --- The player.

AtmospherePlayer::AtmospherePlayer(AtmosphereModel& m) : Performer(m), m_(m) {}

void AtmospherePlayer::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  articulate(midi, velocity, false);
  keyDown_[midi] = sounding_[midi] = true;
  note_ = midi;
  blowing_ = true;
  // (the director's accent: a harder touch)
  m_.keyOn(midi, std::clamp(int(std::lround(velocity * hardness_)), 1, 127));
}

void AtmospherePlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  keyDown_[midi] = false;
  if (!hold_) {
    m_.keyOff(midi);
    sounding_[midi] = false;
  }
}

void AtmospherePlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int k = 0; k < 128; ++k)
      if (sounding_[k] && !keyDown_[k]) {
        m_.keyOff(k);
        sounding_[k] = false;
      }
}

bool AtmospherePlayer::setKnob(int k, double x) {
  x = std::clamp(x, 0.0, 1.0);
  switch (k) {
    case 0: m_.setSustain(x); return true;   // K1: the sustain
    case 1: m_.setHarmonic(x); return true;  // K2: the octave
    case 2: m_.setPluck(x); return true;     // K3: the plectrum
    case 3: m_.setChorus(x); return true;    // K4: the chorus
    default: return false;
  }
}

void AtmospherePlayer::render(float* out, int n) {
  constexpr int kBlock = 64;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    m_.setBend(2.0 * roll_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.asleep();
}

}  // namespace pajupilli
