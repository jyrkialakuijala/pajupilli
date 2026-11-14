// The GM sound effects (121-128) as physical systems (the ensemble's
// -inst fret-noise, breath-noise, seashore, bird-tweet, telephone,
// helicopter, applause, gunshot), on the modal instrument's frame
// (modalinst.h):
//
//   - fret-noise: a fingertip sliding along a wound guitar string: each
//     winding it crosses knocks the string (at the slide's speed / the
//     winding's pitch, 0.25 mm: a squeak from ~1 to ~6 kHz), the string and a
//     guitar body (modal.h) ring with it.
//   - breath-noise: the choir's singer (singer.h, a puhuja-like tract) with
//     the folds held apart: the glottis's turbulence through the tract, the
//     key choosing the mouth's shape (u .. i).
//   - seashore: a wave breaking is bubbles: each rings at Minnaert's
//     frequency (f = 3.26 / R, R the radius in m), damped and chirping upward
//     (van den Doel 2005), their sizes from a power law, their rate following
//     the wave (the crash, the wash, the backwash), a wave every 7-11 s.
//   - bird-tweet: the syrinx's labia, a self-sustained valve (a van der Pol
//     oscillator: x'' - mu (p - x^2) x' + w^2 x = 0, p the air sac's
//     pressure, w the syringeal muscles' tension), driven by gestures (Gardner
//     et al. 2001: a syllable a pressure pulse and a tension sweep), the
//     beak's tract a resonance that follows the note.
//   - telephone: a polarised ringer: the ring current (20 Hz, 2 s on, 4 s
//     off) through the coils turns the armature against its spring, its
//     clapper (a steel ball) swinging into one gong, then the other (Hertz
//     contacts, solved with each step); each gong 16 modes of 4 numbers
//     (frequency, decay, its shape where the ball strikes, its radiation).
//   - helicopter: the main rotor's blades slapping their tip vortices once a
//     passage (a sharp pressure pulse, ~20 Hz), their thickness noise, the
//     tail rotor (~100 Hz), the turbine's whine; spinning up and down.
//   - applause: an audience of up to 960 clappers (40 rows x 24 seats, the
//     key filling it from the front), each its own hands (Peltola et al.
//     2007: the air between the palms a Helmholtz resonance, 0.7-2.5 kHz,
//     Q 3-10, struck by the hands' impact) and place; most keep their own
//     rhythm (3.5-6 Hz, jittered), some seek the beat: slower (2-2.6 Hz, the
//     period doubling Neda et al. 2000 measured in rhythmic applause), their
//     phases pulled toward their nearest fellow seekers' and the hall's beat
//     (Kuramoto). A hard key while they clap: 1-3 of them shout "bravo"
//     (the singer's tract, its lips and tongue tip closing: b, r, v).
//   - gunshot: the bullet's shock (an N-wave, ~0.3 ms) arriving first, then
//     the muzzle blast (Friedlander's wave: p(t) = P (1 - t/T) e^(-b t/T),
//     T 1.5-4 ms), its reflection from the ground.
#pragma once

#include <cstdint>
#include <vector>

#include "modal.h"
#include "singer.h"

namespace pajupilli {

class Sfx {
 public:
  enum Type { kFret, kBreath, kSea, kBird, kPhone, kHeli, kApplause, kGun };
  Sfx(Type t, double fs);
  void noteOn(int midi, double vel);
  void noteOff(int midi);
  float sample(float* side);  // one sample (pressure-like); its left-right difference
  void reset();
  // Applause: is the hall clapping; a shout of "bravo" from 1-3 of them
  // (strength 0..1: how many, how loud).
  bool applauding() const;
  void bravo(double strength);

 private:
  float urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0f / 16777216.0f) - 1; }
  float u01() { return 0.5f * (urand() + 1); }
  Type t_;
  double fs_, dt_;
  uint32_t rng_ = 271828;
  int key_ = 60;
  double vel_ = 0.5, clock_ = 0;
  bool held_ = false;
  ModalSet set_;
  Singer singer_;
  int ctl_ = 0;
  // fret-noise
  double slideT_ = 1e9, slideDur_ = 0.3, vMax_ = 0.5, windPh_ = 0;
  // breath-noise / helicopter / applause / seashore: an envelope 0..1
  double env_ = 0;
  // seashore
  struct Bubble { float re, im, c, s, decay, chirp, amp, pan; bool on; };
  std::vector<Bubble> bub_;
  double waveT_ = 0, wavePeriod_ = 9, waveSize_ = 1, hissLp_ = 0, hissHp_ = 0;
  // bird
  double bx_ = 0, by_ = 0, sylT_ = 1e9, sylDur_ = 0.06, gap_ = 0.04, fHi_ = 6000, fLo_ = 3000, tract1_ = 0, tract2_ = 0;
  int syl_ = 0, sylN_ = 0;
  // telephone: the gongs' modes stepped exactly (2x2 per mode), the clapper
  // (its place, m from the middle, + toward gong 0; its speed), the ring
  // current's envelope and phase.
  struct Gong {
    static constexpr int kN = 16;
    float c00[kN], c01[kN], c10[kN], c11[kN], q[kN], v[kN], exc[kN], rad[kN];
  };
  Gong gong_[2];
  double ringT_ = 0, clapX_ = 0, clapV_ = 0, ringEnv_ = 0, ringPh_ = 0;
  void tuneGongs(double scale);
  // helicopter
  double rotPh_ = 0, tailPh_ = 0, rotHz_ = 5, spin_ = 0, whinePh_ = 0, heliLp_ = 0;
  float pulseAmp_[8] = {};
  // applause
  // applause: an audience (rows x seats), each clapper a phase oscillator
  // (its own rate, this beat's jitter); the ones who seek the beat pulled
  // toward their neighbours' phases (k > 0), the others keeping their own.
  struct Clapper {
    double rate, ph, beat, t, k, stopAt;
    float a1, a2, b0, y1, y2, amp, pan, x;
    double cph, sph;  // cos, sin of 2 pi ph (this block's)
    int nb[6];
    bool on;
  };
  std::vector<Clapper> cl_;
  std::vector<int> ringing_;
  // The shouters (puhuja's tract, singer.h, with its constrictions): each
  // "bra-vo" a score of the lungs, the folds' pitch, the vowel and the lips'
  // and tongue tip's closures.
  struct Shout {
    Singer v;
    double t = 1e9, f = 160, lung = 1500, note = 45;
    float pan = 0, gain = 1;
  };
  Shout shout_[3];
  int shoutCtl_ = 0, nextShout_ = 0;
  uint32_t shoutRng_ = 31337;  // (their own: a shout leaves the clapping as it was)  // the clappers whose palms' air rings (or who clap within this block)
  int clapCtl_ = 0;
  // gunshot
  double shotT_ = 1e9, shotT0_ = 0.002, shotP_ = 1, delayN_ = 0.0;
};

}  // namespace pajupilli
