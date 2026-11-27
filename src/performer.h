// A player of one instrument: turns keys into fingerings, breath, tongue and
// lips. The flutist (player.h) and the reed player (reedplayer.h) share these
// controls, the calibration trims and the ear.
#pragma once

#include <algorithm>
#include <cmath>

#include "calibrate.h"
#include "director.h"
#include "tube.h"

namespace pajupilli {

class SoundingSet;  // (src/band.h)

// A player's vibrato: a rotating complex number z (a phasor), turned each
// block by its angle, an amplitude loop holding |z| at 1; its real part is the
// lung pressure's pulse (the abdomen and the throat). Being a rotation, not a
// sine of a phase, its speed can wander without a jump:
//   - Breathing: the speed (~15 %) and the depth (~20 %) wander, each a noise
//     through two low-passes: the speed's 0.15 s (~1 Hz: it drifts within a
//     few pulses, never within one), the depth's 0.6 s (~0.3 Hz); scaled by
//     the player's wander.
//   - The breath: harder support (dynamics) swings a little faster.
//   - The onset, a random path per attack: none at first; after ~0.3 s the
//     depth comes in over ~0.4 s (a smoothstep to a random power); most notes
//     at their rate from the start, ~10 % starting slower (55-80 %).
// PJ_VIBNOISE scales the breathing.
class Vibrato {
 public:
  void seed(unsigned s) { rng_ = 31337u + 104729u * s; }
  void setPhase(double ph) { re_ = std::cos(ph); im_ = std::sin(ph); }
  // One block: rate (Hz), the time since the attack (s: a new attack when it
  // goes back), the support (0..1, dynamics), the wander (0 steady .. 1
  // natural). Returns Re z with the breathing depth; env() is the onset.
  double step(double dt, double rate, double sinceOn, double support, double wander);
  double env() const { return env_; }
  double rateMul = 1, depthMul = 1;  // this player's own

 private:
  double re_ = 1, im_ = 0, n1_ = 0, rateN_ = 0, d1_ = 0, depthN_ = 0, env_ = 0, last_ = 1e9;
  double delay_ = 0.3, fade_ = 0.4, curve_ = 1, start_ = 1;
  uint32_t rng_ = 31337;
  double urand() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) * (2.0 / 16777216.0) - 1; }
};

class Performer {
 public:
  explicit Performer(Tube& t);
  virtual ~Performer() = default;
  virtual void noteOn(int midi, int velocity) = 0;
  virtual void noteOff(int midi) = 0;
  virtual void render(float* out, int n) = 0;
  virtual void setHold(bool h) = 0;               // keep blowing after the keys are up
  // The sustain pedal as its controller moves (CC 64, 0..127; held from 64): a piano hears how
  // fast it moves (its dampers' landing), the rest only whether it is held.
  virtual void sustainPedal(int cc) { setHold(cc >= 64); }
  // Its own breath wander, and its own vibrato and flutter: rate, phase and
  // depth differ from player to player, so an ensemble never pulses together.
  virtual void seed(unsigned s) = 0;
  // One of many alike in a section (src/stringsect.cc): this player's own ways, fixed for its
  // seat (seat 1, 2, ...): how true it stops, its vibrato, its bow. A player alone is not varied.
  virtual void vary(unsigned /*seat*/) {}
  // (-play) How much the other parts are playing in this instrument's register just now (0:
  // nothing, ~1: a full texture). A soloist plays with more drama over less (the harmonica's hands).
  virtual void setAccompaniment(double /*busy*/) {}
  // (-play) What the other channels sound (src/band.h; `self`: this part's channel, left out),
  // read at the start of each block; and what this player tunes its key to now (cents against
  // equal temperament), written back into it (0: a fixed pitch). The fretless bass listens.
  virtual void setBand(const SoundingSet* /*band*/, int /*self*/) {}
  virtual double tuning(int /*key*/) const { return 0; }
  // (-play) The director's cue for the next key (director.h): its vector of intentions. Here
  // what every player can do with it: a harder or softer attack for the emphasis, the vibrato's
  // depth and speed. A player that can do more (the bow's own vibrato and its delay) adds that.
  // The level comes separately, block by block, as the breath (setBreath).
  virtual void direct(const Cue& c) {
    cue_ = c;
    cued_ = true;
    hardness_ = std::clamp(1.0 + 0.7 * c.v[kCueAccent], 0.6, 1.9);
    vibDepth_ = 0.05 * c.v[kCueVibDepth] * (inst_.design ? inst_.design->reed.vibScale : 1.0);
    vibRate_ = 5.0 * c.v[kCueVibRate];
  }
  // (live) An instrument's own use of the controller's knobs K1..K8 (k 0..7, x 0..1); true if it took
  // the knob (the Tonewheel's drawbars on K1-K4), false to leave it to the common map.
  virtual bool setKnob(int /*k*/, double /*x*/) { return false; }

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
  // The syllable a brass or reed player says into the instrument ("da", "ta", "te", "ti", "to",
  // "du"; "-": the player's own). The consonant is the tongue: -1 a soft "d" (a slower release,
  // the breath rising slower) .. 1 a hard "t". The vowel is the mouth, shaped by the tongue into
  // a resonance held through the note (Embouchure::tract): u 350, o 450, a 650, e 1100, i 1800 Hz.
  // Which of these colours a note carries at which dynamics: tools/colours.py.
  void setSyllable(double consonant, double vowelHz, double amount = 1) {
    sylHard_ = consonant;
    vowelHz_ = vowelHz;
    vowelAmt_ = vowelHz > 0 ? amount : 0;
  }
  bool setSyllable(const char* s) {
    double c = 0, hz = 0;
    for (; *s; ++s) {
      switch (*s) {
        case 'd': c = -1; break;
        case 't': c = 1; break;
        case 'u': hz = 350; break;
        case 'o': hz = 450; break;
        case 'a': hz = 650; break;
        case 'e': hz = 1100; break;
        case 'i': hz = 1800; break;
        case '-': break;
        default: return false;
      }
    }
    setSyllable(c, hz);
    return true;
  }
  void setRelease(double s) { release_ = s; }    // breath release time (s)
  void setFormantSpread(double s) { formantSpread_ = s; }  // synths: each key its own filter (0..1)

  // Per-note corrections from calibration. cover: the pitch control of the
  // lips (a flute: the uncovered width; a duct: the breath; a reed: the lip
  // force), at mezzo-forte, soft and loud; press: the breath; jet: the lip-to-edge
  // distance (flutes).
  double coverTrim[128], coverLoTrim[128], coverHiTrim[128], dynLo[128], dynHi[128], pressTrim[128], jetTrim[128];
  int fingering[128];  // the calibration's choice of fingering (brass)
  // An instrument's own note (NoteInfo::natural): its pitch with the lips at rest, cents off
  // the tempered one; the ear tunes it from there (a chord's correction on top), not to the
  // tempered pitch. 0 for every other note.
  double ownCents[128] = {};
  void setCalibration(const Calibration& c) {
    for (int i = 0; i < 128; ++i) {
      const NoteInfo* ni = inst_.design && i >= inst_.lo && i <= inst_.hi ? noteInfo(inst_, i) : nullptr;
      ownCents[i] = ni && ni->natural && c.ok[i] ? c.cents[i] : 0;
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
  double vibrato() const { return vibDepth_; }  // the vibrato knob's depth (setVibrato)
  // Play this note `cents` off its tempered pitch, tuned by ear with the lips.
  void setTuning(double cents) { ear_ = true; earCents_ = cents; }
  void setEar(bool on) { ear_ = on; }  // listen and tune held notes with the lips
  double earCorr() const { return earCorr_; }  // (what the ear has trimmed the lips by now: log)
  const NoteInfo* info(int midi) const { return noteInfo(inst_, midi); }
  // The fingering the player uses: the calibration's choice.
  const NoteInfo* played(int midi) const {
    const NoteInfo* a = noteInfo(inst_, midi, fingering[midi]);
    return a ? a : info(midi);
  }
  const Instrument& instrument() const { return inst_; }
  // The last note-on's articulation (Performer::articulate), by name.
  const char* articulation() const {
    static const char* const kNames[] = {"slur", "glide", "tongued legato", "du", "tu", "breath", "accent"};
    return kNames[lastArt_];
  }

 protected:
  Cue cue_{};         // the director's last cue (cued_: there was one)
  bool cued_ = false;
  // The pitch of the note from a band-pass around it: each upward zero
  // crossing gives one period; corr (log of the pitch control) follows the
  // error (dcents / dcorr = `slope`) with time constant tau (s), after `clock`
  // seconds on the note.
  // Errors within +-dead cents are let be.
  // Errors beyond +-range cents are another regime, not a matter for the lips: ignored.
  void listen(const float* x, int m, double& corr, double cents, double clock, double slope, double tau,
              double dead = 0, double range = 1e9);
  double heard_ = 0;  // the last error the ear measured (cents), 0 before
  // The lips' (the cover trim's) setting for the note at dynamics dyn, between
  // the calibrated soft, mezzo-forte and loud settings (log).
  double coverAt(int note, double dyn) const;

  // How a new note starts, decided at its key with no look-ahead
  // (research/playing_techniques.md 5.1), from the overlap with the key
  // before, the gap since the last release, the key's hardness and how it
  // compares with the key before:
  //   - overlap, a similar velocity: a slur (the fingers only);
  //   - overlap, a much harder key (or a very hard one): tongued legato, a
  //     light "du" on the moving air;
  //   - overlap, a much softer key pressed very softly (velocity <= 45: a
  //     deliberate touch, not legato's ordinary spread): a portamento, the fingers sliding off
  //     and onto the holes (keys, valves half-way) while the lips follow,
  //     over ~60 ms + 25 ms per semitone, longer the softer the key; only
  //     where the fingering lets the pitch glide (not a pan flute's pipes,
  //     not a jet's register change on one fingering);
  //   - no overlap, inside the release or a gap under 80 ms: "du" (a hard
  //     key: "tu");
  //   - from silence: a breath attack, "tu" or an accent by velocity.
  enum Articulation { kArtSlur, kArtGlide, kArtTongueLegato, kArtDu, kArtTu, kArtBreath, kArtAccent };
  // Call at the note-on before note_ changes; overlap: a key still held (or the pedal).
  Articulation articulate(int midi, int velocity, bool overlap);
  Articulation decide(int midi, int velocity, bool overlap);
  // Call at the last key's release (no key left, no pedal): a short note
  // (held under ~120 ms) ends with the tongue (staccato), a longer one with the breath.
  void released();
  // Each block.
  void tick(double dt) { now_ += dt; glideT_ += dt; stopClock_ += dt; }
  bool gliding() const { return glideFrom_ >= 0 && glideT_ < glideDur_; }
  // Progress of the glide, 0 .. 1 (smooth).
  double glideMix() const;
  // The pitch the glide passes (fractional MIDI note) and its frequency.
  double glideNote() const { return gliding() ? glideFrom_ + (note_ - glideFrom_) * glideMix() : note_; }
  double glideFrequency() const { return 440 * std::exp2((glideNote() - 69) / 12); }
  // The lips' setting along the glide: the old note's, moving to the new one's (x this).
  double glideCover(int note, double dyn) const;
  // The fingers' glide for the next fingering (signed s: + rising), once.
  double takeHoleGlide() { const double g = holeGlide_; holeGlide_ = 0; return g; }
  void cancelGlide() { glideFrom_ = -1; holeGlide_ = 0; }
  // The ear during a glide: it stops listening; its correction on the old
  // note fades out along the glide (dropped at once, it jolts the lips).
  void glideEar() {
    earClock_ = 0;
    earCorr_ = glideEar_ * (1 - glideMix());
  }
  // The tongue's stop at a staccato release: multiplies the tongue's opening.
  double tongueStop() const { return stopping_ ? std::max(0.0, 1 - stopClock_ / 0.004) : 1; }

  Tube& tube_;
  const Instrument& inst_;
  double fs_;
  int note_ = -1;
  bool blowing_ = false, hold_ = false;
  double pressScale_ = 1;
  double dyn_ = 0.5, breathCtl_ = 0, vibDepth_ = 0.0, vibRate_ = 5.0, roll_ = 0;
  double sylHard_ = 0, vowelHz_ = 0, vowelAmt_ = 0;  // (setSyllable)
  double noiseCtl_ = 1, brightness_ = 1, air_ = 0.35, wander_ = 1;
  double flutter_ = 0, hardness_ = 1, release_ = 0.030, formantSpread_ = 0;
  bool ear_ = false;
  double earCents_ = 0, earCorr_ = 0, earClock_ = 0;
  double bp1_ = 0, bp2_ = 0, lastCross_ = -1, prevBp_ = 0;
  // Articulation.
  double now_ = 0, lastOnAt_ = -1e9, lastOffAt_ = -1e9, lastVel_ = 64;
  int glideFrom_ = -1;
  double glideT_ = 1e9, glideDur_ = 0, holeGlide_ = 0, glideEar_ = 0;
  bool stopping_ = false;
  int lastArt_ = 0;
  double stopClock_ = 1e9;
  double kRetongue_, kDvTongue_, kDvGlide_, kGlideVel_, kGlide_, kStaccato_, kDetache_;
};

}  // namespace pajupilli
