// A player of one instrument: turns keys into fingerings, breath, tongue and
// lips. The flutist (player.h) and the reed player (reedplayer.h) share these
// controls, the calibration trims and the ear.
#pragma once

#include "calibrate.h"
#include "tube.h"

namespace pajupilli {

class Performer {
 public:
  explicit Performer(Tube& t);
  virtual ~Performer() = default;
  virtual void noteOn(int midi, int velocity) = 0;
  virtual void noteOff(int midi) = 0;
  virtual void render(float* out, int n) = 0;
  virtual void setHold(bool h) = 0;               // keep blowing after the keys are up
  // Its own breath wander, and its own vibrato and flutter: rate, phase and
  // depth differ from player to player, so an ensemble never pulses together.
  virtual void seed(unsigned s) = 0;

  void setDynamics(double d) { dyn_ = d; }      // 0 pp .. 1 ff (breath, lip opening)
  void setBreath(double b) { breathCtl_ = b; }   // added to dynamics (joystick, pad pressure)
  void setPressureScale(double s) { pressScale_ = s; }  // multiplies the breath pressure (tests: -ramp)
  void setVibrato(double depth) { vibDepth_ = depth; }  // fraction of lung pressure
  void setVibratoRate(double hz) { vibRate_ = hz; }
  void setLipRoll(double r) { roll_ = r; }       // -1 .. 1: pitch bend with the lips
  void setNoise(double n) { noiseCtl_ = n; }     // breath noise multiplier
  void setAir(double a) { air_ = a; }            // 0 focused .. 1 airy
  void setWander(double w) { wander_ = w; }      // 0 machine-steady .. 1 natural .. 3 unsteady
  void setBrightness(double b) { brightness_ = b; }  // jet offset / reed opening
  void setFlutter(double depth) { flutter_ = depth; }  // 0 .. 1
  void setHardness(double h) { hardness_ = h; }  // attack: 0.5 soft .. 2 hard
  void setRelease(double s) { release_ = s; }    // breath release time (s)

  // Per-note corrections from calibration. cover: the pitch control of the
  // lips (a flute: the uncovered width; a duct: the breath; a reed: the lip
  // force), at mezzo-forte, soft and loud; press: the breath; jet: the lip-to-edge
  // distance (flutes).
  double coverTrim[128], coverLoTrim[128], coverHiTrim[128], dynLo[128], dynHi[128], pressTrim[128], jetTrim[128];
  int fingering[128];  // the calibration's choice of fingering (brass)
  void setCalibration(const Calibration& c) {
    for (int i = 0; i < 128; ++i) {
      coverTrim[i] = c.cover[i];
      coverLoTrim[i] = c.coverLo[i];
      coverHiTrim[i] = c.coverHi[i];
      dynLo[i] = c.dynLo[i];
      dynHi[i] = c.dynHi[i];
      pressTrim[i] = c.press[i];
      jetTrim[i] = c.jet[i];
      fingering[i] = c.fingering[i];
    }
  }
  int sounding() const { return note_; }
  bool busy() const { return blowing_; }
  // Play this note `cents` off its tempered pitch, tuned by ear with the lips.
  void setTuning(double cents) { ear_ = true; earCents_ = cents; }
  void setEar(bool on) { ear_ = on; }  // listen and tune held notes with the lips
  const NoteInfo* info(int midi) const { return noteInfo(inst_, midi); }
  // The fingering the player uses: the calibration's choice.
  const NoteInfo* played(int midi) const {
    const NoteInfo* a = noteInfo(inst_, midi, fingering[midi]);
    return a ? a : info(midi);
  }
  const Instrument& instrument() const { return inst_; }

 protected:
  // The pitch of the note from a band-pass around it: each upward zero
  // crossing gives one period; corr (log of the pitch control) follows the
  // error (dcents / dcorr = `slope`) with time constant tau (s), after `clock`
  // seconds on the note.
  // Errors within +-dead cents are let be.
  void listen(const float* x, int m, double& corr, double cents, double clock, double slope, double tau,
              double dead = 0);
  double heard_ = 0;  // the last error the ear measured (cents), 0 before
  // The lips' (the cover trim's) setting for the note at dynamics dyn, between
  // the calibrated soft, mezzo-forte and loud settings (log).
  double coverAt(int note, double dyn) const;

  Tube& tube_;
  const Instrument& inst_;
  double fs_;
  int note_ = -1;
  bool blowing_ = false, hold_ = false;
  double pressScale_ = 1;
  double dyn_ = 0.5, breathCtl_ = 0, vibDepth_ = 0.0, vibRate_ = 5.0, roll_ = 0;
  double noiseCtl_ = 1, brightness_ = 1, air_ = 0.35, wander_ = 1;
  double flutter_ = 0, hardness_ = 1, release_ = 0.030;
  bool ear_ = false;
  double earCents_ = 0, earCorr_ = 0, earClock_ = 0;
  double bp1_ = 0, bp2_ = 0, lastCross_ = -1, prevBp_ = 0;
};

}  // namespace pajupilli
