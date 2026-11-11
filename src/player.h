// The flutist: turns notes into fingerings, breath, tongue and lip settings,
// with the timing of tonguing, slurs and releases.
#pragma once

#include "calibrate.h"
#include "flute.h"

namespace pajupilli {

struct NoteInfo {
  const char* holes;  // 16 chars, hole 1 .. 16, 'o' open
  int harmonic;       // which resonance of the fingering sounds (1, 2, 3, 4)
};

// MIDI 60 (C4) .. 96 (C7); nullptr outside.
const NoteInfo* noteInfo(int midi);
double noteFrequency(int midi, double a4 = 440.0);

class Player {
 public:
  explicit Player(Flute& f);
  void noteOn(int midi, int velocity);
  void noteOff(int midi);
  void setDynamics(double d) { dyn_ = d; }      // 0 pp .. 1 ff (breath, lip opening)
  void setVibrato(double depth) { vibDepth_ = depth; }  // fraction of lung pressure
  void setLipRoll(double r) { roll_ = r; }       // -1 .. 1: covers more / less (pitch)
  // Per-note corrections from calibration: uncovered width scale, pressure scale.
  double coverTrim[128], pressTrim[128];
  void setCalibration(const Calibration& c) {
    for (int i = 0; i < 128; ++i) { coverTrim[i] = c.cover[i]; pressTrim[i] = c.press[i]; }
  }
  void render(float* out, int n);
  int sounding() const { return note_; }

 private:
  void applyNote(bool tongued);
  Flute& flute_;
  double fs_;
  int note_ = -1, held_[128] = {};
  int stackTop_ = 0, stack_[16];
  double dyn_ = 0.5, vibDepth_ = 0.0, roll_ = 0;
  double vibPhase_ = 0;
  double lung_ = 0, lungTarget_ = 0;
  double tongueClock_ = 1e9, tongueRelease_ = 0.005;
  double sinceOn_ = 0;
  double velAccent_ = 0;
};

}  // namespace pajupilli
