// The chromatic harmonica (General MIDI 23, -inst harmonica; research/harmonica.md §10): a 16-hole
// C chromatic, C3..D7 (MIDI 48..98), solo tuning, 64 reeds (src/freereed.h), A = 440.
//
// One player, one breath: the lungs (a signed pressure: + blow, - draw) feed the vocal tract (one
// Helmholtz mode: its inertance and the mouth's volume), the mouth feeds every hole under the lips
// in parallel, each hole's chamber its blow and draw reed, and the chambers sound into the cupped
// hands (a second Helmholtz cavity whose opening radiates). So a chord takes more air and every
// reed of it gets a little less; a bend (the tract tuned just above the wanted pitch) moves every
// note of a chord; the hands colour all of it.
//
//   - One breath direction at a time: only the reeds of the breath's direction are ever driven
//     (the invariant; PJ_HARP_CHECK=1 counts violations). A chord asking for blow and draw notes
//     sounds the side with the higher average velocity (a bonus for more notes; the lungs' fill
//     favours drawing when they run empty, blowing when full); the other side is silent.
//   - The slide is one bar: pressed, it travels over ~20 ms (back on its spring in ~15), the
//     natural row's holes closing as the sharp row's open, both partly open midway.
//   - The tongue blocks every hole under the lips but the notes' (tongue blocking: the player's
//     usual technique for chords, and clean single notes).
//   - The hands: cupped while playing; opened at a phrase's end and now and then on a long note,
//     closed again at the next note; CC1 (and the player's habit on long notes) flutters them
//     (the hand tremolo, ~6.5 Hz).
//   - Pitch bend down: the tract tuned above the bent pitch, the breath harder; up: a few cents,
//     the breath softer.
#pragma once

#include <vector>

#include "freereed.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool harmonicaInstrument(const Instrument& inst);  // -inst harmonica

class HarmonicaModel : public Tube {
 public:
  explicit HarmonicaModel(const Instrument& inst);
  ~HarmonicaModel() override;
  static constexpr int kHoles = 16;
  static constexpr int kLo = 48, kHi = 98;
  // The note of hole h (0..15), slide row (0 out, 1 in), direction (0 blow, 1 draw).
  static int noteOf(int h, int row, int dir);

  // The player's controls.
  void setBreath(double pa) { breathP_ = pa; }    // the lungs' target (Pa, signed: + blow)
  void setBreathTau(double s) { breathTau_ = s; } // how fast the breath gets there
  void setSlide(bool in) { slideIn_ = in; }
  void setLips(int h, double open, double tau);   // a hole uncovered (1) or blocked (0)
  void setTract(double hz, double q) { tractHz_ = hz; tractQ_ = q; }
  void setHands(double a, double tau) { handTarget_ = a; handTau_ = tau; }  // the opening's radius (m)
  void setTremolo(double depth, double hz) { tremDepth_ = depth; tremHz_ = hz; }
  int breathDir() const { return dir_; }          // the breath's direction now (0 blow, 1 draw)
  double breath() const { return pL_; }
  double lungFill() const { return lung_; }       // 0 empty .. 1 full
  double hands() const { return handA_; }
  bool asleep() const;
  long violations() const { return violations_; }
  double testResponse() const override { return last_; }
  void resetAir() override;

 private:
  struct Reed {
    FreeReed r;
    int hole, row, dir;
  };
  void controlStep() override {}
  float sample() override;
  float step();                     // one sample at 2x
  double calibrate(int i);          // the speaking pitch of reed i, alone, at mezzo-forte
  void refreshActive();
  std::vector<Reed> reeds_;         // index ((h * 2 + row) * 2 + dir)
  std::vector<int> active_;
  double fs2_, dt_;
  // The breath and the tract.
  double breathP_ = 0, breathTau_ = 0.03, pL_ = 0, holdGap_ = 0;
  int dir_ = 0, wantDir_ = 0;
  double tractHz_ = 500, tractQ_ = 3, ut_ = 0, pm_ = 0, ct_;
  double lung_ = 0.5;
  // The lips and the slide.
  double lip_[kHoles] = {}, lipTarget_[kHoles] = {}, lipTau_[kHoles] = {};
  bool slideIn_ = false;
  double slide_ = 0;
  // The hands.
  double handA_ = 0.01, handTarget_ = 0.01, handTau_ = 0.1, ph_ = 0, uo_ = 0, ch_;
  double tremDepth_ = 0, tremHz_ = 6.5, tremPh_ = 0, tremWander_ = 0;
  double prevUo_ = 0, prevUm_ = 0;
  uint32_t rng_ = 99991;
  long violations_ = 0;
  bool check_ = false;
  float last_ = 0;
  HalfBand down_;
};

class HarmonicaPlayer : public Performer {
 public:
  explicit HarmonicaPlayer(HarmonicaModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool) override {}
  void seed(unsigned s) override { rng_ = 1234567u + 7919u * s; }
  void setAccompaniment(double busy) override { busy_ += 0.01 * (busy - busy_); }

 private:
  struct Held { int midi, vel; double at; };
  struct Place { int hole, row, dir; };
  void decide(bool attack);
  void applyBreath();
  double pressure(int vel) const;
  HarmonicaModel& m_;
  std::vector<Held> held_;
  std::vector<std::pair<int, Place>> sounding_;  // the notes that sound, and where
  int dir_ = 0, row_ = 0, hole_ = 4;
  int vel_ = 80;
  bool breathing_ = false;
  double offAt_ = -1e9, onAt_ = -1e9, noteOnAt_ = 0;
  double scoop_ = 0;           // the attack's bend (semitones, decaying)
  double handOpenAt_ = -1;     // when the hands open on this note (s after its start; -1: not)
  bool handsOpen_ = false, endBreath_ = false;
  double busy_ = 0;            // the other parts in the harmonica's register (-play)
  double habitTrem_ = 0;       // the player's own hand tremolo on this long note
  uint32_t rng_ = 1234567;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (1.0 / 16777216.0); }
};

}  // namespace pajupilli
