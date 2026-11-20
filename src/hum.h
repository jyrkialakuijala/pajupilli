// A closed-lip hum ("mmm"; the GM2 "Humming" colour of Voice Oohs) as a
// filter on a Singer's sung "ooh": puhuja's cascade nasal pole and zero
// (~/puhuja/puhuja.cc kNasalPoleHz, kNasalBw, kNasalZeroComp), not a nasal
// branch on the line (research/choir.md 6.2, 8.3, 9.2 item 4).
//
// Why a filter: a hum is the glottal source through the pharynx and the nose
// with the mouth a closed side cavity. Its spectrum (puhuja's m): one broad
// nasal pole near 270 Hz, the closed mouth's anti-resonance near 1 kHz, little
// above ~1.5 kHz, 10-15 dB under an "aah". The Singer's [u] (F1 ~300, F2
// ~800, lips small) is already most of the way there: the same source, the
// same pharynx, F1 near the murmur's pole. What it lacks is the zero, the
// loss of the highs and some level. A 40-section nasal line would add 60-90 %
// to every singer (~0.4-0.6 % of a core each); this costs ~15 multiply-adds a
// sample (measured in tools/humtest.cc).
//
//   - The pole and the zero have the same width (250 Hz), so with the zero on
//     the pole they cancel exactly: amount 0 is the input unchanged (an open
//     "ooh"), and as the lips close (amount -> 1) the zero slides up from the
//     pole to the closed mouth's 1 kHz (the nasal murmur forms).
//   - Both are unity at DC, so everything above them falls by (pole/zero)^2
//     (the response at amount 1, levelDb included: 270 Hz -5.5 dB, 500 -17,
//     1 kHz -40, 2 kHz -32, 5 kHz -41).
//   - The murmur's highs: a 2nd-order low-pass (2.5 kHz) faded in with the
//     amount; the closed lips' level (levelDb), faded in likewise: the hum
//     ~6 dB under its ooh, ~12 under an aah (tools/humtest.cc).
//   - The amount glides (the lips close in ~20 ms, open in ~40).
#pragma once

namespace pajupilli {

class Hum {
 public:
  struct Params {
    double poleHz = 270;   // the nasal murmur's pole (puhuja kNasalPoleHz)
    double zeroHz = 1000;  // the closed mouth's anti-resonance (m)
    double bwHz = 250;     // both widths (puhuja kNasalBw)
    double lpHz = 2500;    // the murmur's low-pass
    double levelDb = -7;   // the closed lips' level (the hum against its "ooh")
  };
  void init(double fs, const Params& p);
  void init(double fs) { init(fs, Params()); }
  void reset();
  // Per control block: how far the lips are closed (0 the input as it is, an
  // open "ooh"; 1 a hum), the block's length (s).
  void control(double amount, double dt);
  // One sample of the Singer's output -> the same through the hum.
  float process(float x) {
    // The pole (a resonator, unity at DC) after the zero (its inverse at fz).
    const double z = za_ * x + zb_ * zx1_ + zc_ * zx2_;
    zx2_ = zx1_, zx1_ = x;
    const double y = pa_ * z + pb_ * py1_ + pc_ * py2_;
    py2_ = py1_, py1_ = y;
    // The low-pass (direct form I) and the cross-fades.
    const double l = lb0_ * (y + 2 * ly1x_ + ly2x_) - la1_ * ly1_ - la2_ * ly2_;
    ly2x_ = ly1x_, ly1x_ = y, ly2_ = ly1_, ly1_ = l;
    return float(g_ * (y + lpMix_ * (l - y)));
  }
  double amount() const { return amt_; }

 private:
  Params p_;
  double fs_ = 48000, amt_ = 0, set_ = -1;
  double za_ = 1, zb_ = 0, zc_ = 0, zx1_ = 0, zx2_ = 0;
  double pa_ = 1, pb_ = 0, pc_ = 0, py1_ = 0, py2_ = 0;
  double lb0_ = 0, la1_ = 0, la2_ = 0, ly1x_ = 0, ly2x_ = 0, ly1_ = 0, ly2_ = 0;
  double lpMix_ = 0, g_ = 1;
  void coefs();
};

}  // namespace pajupilli
