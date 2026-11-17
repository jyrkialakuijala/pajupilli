// The Leslie 122 (research/leslie.md §12): the organ's console signal into the cabinet's tube amp,
// an 800 Hz crossover, the treble horn and the bass drum's scoop each turned by its own motor and
// belt, heard by two microphones (stereo).
//
//   - The amp: the driver and the push-pull pair as a soft, slightly asymmetric clip whose drive
//     follows the organ's expression; its supply sags under a loud passage.
//   - The rotors: each speeds up and slows down by its own inertia (the horn ~0.5 s up, 0.8 s
//     down; the drum ~3.5 s up, 2.5 s down), so a speed change is a gesture, the drum lagging.
//   - The horn: one mouth on a 0.17 m arm. To each microphone its direct path and two wall
//     reflections (image sources turning with it), each a delay line read at the moving distance
//     (the Doppler) and weighted by the mouth's directivity in three bands (the highs beam), plus
//     the base's leakage that does not turn.
//   - The drum: below ~200 Hz it does not modulate; above, the scoop's beam turns (amplitude and
//     a little Doppler).
//   - The cabinet: a short reverberation (a 4-line FDN, ~40 ms) of both rotors.
//   - The microphones: a pair ±60° off the front at 0.4 m: left and right.
#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

namespace pajupilli {

class Leslie {
 public:
  void setup(double fs, uint32_t seed);
  void setFast(bool fast) { fast_ = fast; brake_ = false; }
  void setBrake(bool on) { brake_ = on; }
  void setDrive(double d) { drive_ = d; }  // the amp's input gain (1: clean at mezzo-forte)
  bool fast() const { return fast_; }
  double hornHz() const { return hornW_; }
  double drumHz() const { return drumW_; }
  // One sample of the console signal in; left and right out.
  void process(float in, float& left, float& right, float* rays = nullptr);
  // The cabinet radiating into the room: the sound leaving it toward kRays directions round it
  // (0: the front, toward the listener; then +-60, +-120, 180 degrees), each with its Doppler
  // (the horn's distance along it) and the horn's beam toward it, at 1 m (filled when rays is
  // given to process()).
  static constexpr int kRays = 6;
  static double rayAngle(int k) { static const double a[kRays] = {0, 60, -60, 120, -120, 180}; return a[k] * M_PI / 180; }

 private:
  struct Ring {
    std::vector<float> b;
    int mask = 0, w = 0;
    void init(int n) { int s = 1; while (s < n) s <<= 1; b.assign(s, 0.0f); mask = s - 1; w = 0; }
    void push(float x) { b[w] = x; w = (w + 1) & mask; }
    float read(double delay) const;  // samples back (>= 1), cubic
  };
  double fs_ = 48000, dt_ = 1 / 48000.0;
  bool fast_ = false, brake_ = false;
  double drive_ = 1, sag_ = 0;
  double hornW_ = 0.8, drumW_ = 0.67, hornPh_ = 0, drumPh_ = 0;
  double hornSlow_, hornFast_, drumSlow_, drumFast_;
  // The crossover (2nd order at 800 Hz) and the bands.
  double lp1_[2] = {}, hp1_[2] = {};
  double xa_[3], xb_[3], xhb_[3];  // (the low-pass's numerator, the shared denominator, the high-pass's numerator)
  double hornLo_ = 0, hornMid_ = 0, aLo_, aMid_;
  double drumLo_ = 0, aDrum_;
  Ring horn_[3], drum_;  // the horn's three bands, the drum's turning part
  float drumFixed_[2048] = {};
  int dfw_ = 0;
  // The cabinet's tail.
  // The cabinet's own flaws (seeded by its serial; PJ_LESLIE_AGE scales them, PJ_LESLIE_NOISE the
  // machinery's noises).
  double age_ = 1, noise_ = 1;
  double hornEcc_ = 0, hornEccPh_ = 0, drumEcc_ = 0, drumEccPh_ = 0;  // a rotor's speed within a turn
  double beltEcc_[2] = {}, beltPh_[2] = {};                          // its belt's own wobble
  double huntH_ = 0, huntD_ = 0;                                     // the motors' hunting
  int ctl_ = 0;
  double micX_[2] = {}, micY_[2] = {};
  double windLp_ = 0, windHp_ = 0;                                   // the horn's wind
  double rattleX_ = 0, rattleV_ = 0, rattleW_ = 1000, rattleQ_ = 20, rattleGap_ = 1, rattleHp_ = 0, rattleIn_ = 0;
  double click_ = 0, clickPh_ = 0, t_ = 0;
  bool lastFast_ = false;
  double normal();
  std::vector<float> fdn_[4];
  int fdnPos_[4] = {}, fdnLen_[4] = {};
  double fdnG_[4] = {};
  uint32_t rng_ = 1;
};

}  // namespace pajupilli
