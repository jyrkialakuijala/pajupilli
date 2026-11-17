// The church organ (research/church_organ.md), its first ranks, each key its
// own pipe (src/organpipe.h: the jet and the column's modes), voiced for its
// pitch and calibrated when built, at its place in its division
// (src/organbox.h), every division on a divided chest: the C side on the
// left, the C# side on the right.
//
//   -inst gedackt-8       Schwellwerk Gedackt 8': stopped wood, in two swell
//                         boxes high on the gallery, ~5 m apart (the C side's
//                         box left, the C#'s right), the bass octave offset
//                         along each box's back wall; the swell pedal (the
//                         dynamics knob, the breath) opens both boxes' shutters.
//   -inst rohrgedackt-8   Positiv Rohrgedackt 8': metal with chimneys, in the
//                         Rückpositiv's open case at the gallery rail (its two
//                         halves C left, C# right), nearer the church.
//   -inst principal-8     Hauptwerk Principal 8': open metal, its bass C2..F3
//                         in the façade in front of the main case, the rest on
//                         the Hauptwerk's chest inside it (the C side left, the
//                         C# right); each pipe sounds from its mouth and its top.
//   -inst organ           the stops in PJ_ORGAN_STOPS (principal8, gedackt8,
//                         rohrgedackt8; default the Principal and the Gedackt).
//   -inst gedackt-rohrgedackt  both drawn, the manuals coupled: each key sounds
//                         both, from the boxes and from the rail; the swell box
//                         a little warmer, so the two flutes beat gently.
// Steady wind for now (the blower and the wind's sag: §4a, later). The keys
// have no velocity (a pallet opens the same however the key is struck).
#pragma once

#include <array>
#include <memory>

#include "parallel.h"
#include <string>
#include <vector>

#include "organbox.h"
#include "organpipe.h"
#include "organreed.h"
#include "organwind.h"
#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool organRank(const Instrument& inst);

// A pipe of either kind (a flue, a reed), one interface.
struct AnyPipe {
  FluePipe f;
  ReedPipe r;
  bool reed = false;
  void tune(double x) { reed ? r.tune(x) : f.tune(x); }
  double ratio() const { return reed ? r.ratio() : f.ratio(); }
  void open(double secs = 0) { reed ? r.open(secs) : f.open(secs); }
  void close(double secs = 0) { reed ? r.close(secs) : f.close(secs); }
  bool awake() const { return reed ? r.awake() : f.awake(); }
  bool openPipe() const { return !reed && f.openPipe(); }
  float sample(float wind, float room) { return reed ? r.sample(wind, room) : f.sample(wind, room); }
  float top() const { return reed ? 0.0f : f.top(); }
  float foot() const { return reed ? r.foot() : f.foot(); }
  double hz() const { return reed ? r.hz() : f.hz(); }
  void reset() { reed ? r.reset() : f.reset(); }
};

class OrganModel : public Tube {
 public:
  explicit OrganModel(const Instrument& inst);
  ~OrganModel() override;
  static constexpr int kLo = 36, kHi = 96;
  void open(int midi);
  void close(int midi);
  void setSwell(double s);  // 0 closed .. 1 open
  // The crescendo roller (-inst organ-crescendo, or "crescendo" among the
  // stops): 0 silence, then ppp .. fff, the stops drawn in turn.
  bool crescendo() const { return crescendo_; }
  bool zones() const { return zones_; }
  // Playing by manuals (a piece's registration plan; tools: an organ part's
  // tracks on the Hauptwerk 0, the Schwellwerk 1, the Positiv 2, the Pedal 3):
  // a key held on a division sounds that division's drawn stops (and those
  // coupled to it). Built with "all" among the stops: every stop, none drawn.
  // touch: 0 normal .. 1 the softest finger (a long high note: its pallet
  // opened up to 2.5x slower, closed up to 2x, its attack gentler).
  void openOn(int division, int midi, double touch = 0);
  void closeOn(int division, int midi);
  // The registration now: stops' tokens and presets, couplers (sw-hw, pos-hw,
  // sw-pos, hw-ped, sw-ped, pos-ped), tremulants (tremsw, trempos); every
  // other stop retired.
  void setRegistration(const std::string& stops);
  // The pedal's stops on the lowest key held on any manual below the split
  // (when a part has no pedal of its own).
  void setAutoPedal(bool on) {
    autoPedal_ = on;
    refreshKeys();
  }
  void setRoller(double x);
  static const char* dynamicName(double x);
  const OrganWind& wind() const { return wind_; }
  bool asleep() const;
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  struct Pipe {
    AnyPipe p;
    int enc = 0, src = 0, srcTop = -1;  // its enclosure, its mouth there (and an open pipe's top)
    int near[4] = {-1, -1, -1, -1};  // the neighbours on its chest (a whole tone, two, away)
    float nearG[4] = {};
    float y = 0;     // its last sample
    float gain = 1;  // its rank's level (the voicer's regulation)
    double hz = 0, target = 0;
    int key = 0, stop = 0, os = 1;  // (os: its oversampling)
    int div = 0;      // its division (its chest's wind)
    float flow = 0;   // its flow at the working pressure (L/s)
    bool pedal = false;
    bool keyed = false;  // its key down and its stop drawn
    double baseRatio = 1;  // (its calibrated tuning, before the pajupilli intonation's offset)
    // Its spectrum, from the model: each harmonic's level in its own output,
    // settled, alone (read once, when calibrated, and cached; [k - 1] the
    // k-th's, up to 40 x or 16 kHz), normalised to the strongest: for tuning
    // by the spectra.
    std::vector<float> spectrum;
    float jiCents = 0;     // (the offset it is tuned to now)
    // (PJ_ORGAN_PITCHLOG: its frequency as it plays: a band-pass at its pitch,
    // its zero crossings timed once it has spoken)
    struct PitchLog {
      double x1 = 0, x2 = 0, y1 = 0, y2 = 0, z1 = 0, z2 = 0, b0 = 0, a1 = 0, a2 = 0;  // (two band-passes in cascade)
      double prev = 0, lastCross = -1, sum = 0, sum2 = 0, lo = 1e9, hi = -1e9;
      long n = 0, since = 0, sounding = 0;
    } log;
    Vec3 at;
    float hear = 1;  // how strongly its jet hears the field at its mouth
    // (a pedal reed on its open chest inside the case: its sound's mids and
    // highs shelved down by the casework on their way out; caseG 1: none)
    float caseA = 1, caseG = 1, caseLp = 0, caseLp2 = 0, caseHz = 0;
  };
  void controlStep() override {}
  float sample() override;
  void addRank(int stop);
  void calibrate();
  void linkNeighbours();
  void pedalUpdate();
  void refreshKeys();
  bool pitchLog_ = false;
  // The pajupilli intonation (PJ_ORGAN_JI, on by default): the held keys'
  // offsets solved towards just intervals (the flutes' rules, src/ensemble.cc's),
  // each key's pipes gliding to its offset (+-30 c; a pedal key's +-15, the
  // highest key's +-10).
  bool ji_ = true;
  long now_ = 0;                  // samples
  double jiTarget_[128] = {}, jiNow_[128] = {};
  double jiRate_[128] = {}, jiGoal_[128] = {};  // (each key's glide: c a ms, towards the goal it was set for)
  long jiSince_[128] = {};        // when each key went down (samples; -1 up)
  long jiSolved_ = 0;
  long long jiSig_ = 0;
  void justSolve();
  void justGlide();
  void writePitchLog() const;
  double moveSecs_ = 0;  // (> 0 while a registration changes: the sliders' time)
  // Each key's pallet on each division (one for all its stops): its opening
  // or closing time, drawn afresh at each press and release (the base +-
  // PJ_ORGAN_PALLETVAR; the bass's and the pedal's bigger pallets slower).
  float palletOpen_[4][128] = {}, palletClose_[4][128] = {};
  float touch_[128] = {};    // (each key's touch at its press: its release as soft)
  // The melody's finger, played live (decided at each key-down from what has
  // been played): after a long melody note (1 s or more, at C4 or above, in
  // the last 7 s), a note rising above the previous one in the register is
  // played softer: the more the leap, the more it stands above the recent
  // high notes (their mean and spread), the quieter the playing, the softer.
  double liveTouch(int midi);
  void noteReleased(int midi);
  std::vector<std::pair<long, int>> recent_;  // (presses in the last 7 s: when, which key)
  long longAt_ = -1;   // (the last long melody note's release, or -1)
  int prevHigh_ = -1;  // (the register's previous onset's top note)
  int groupTop_ = -1;  // (this onset's top so far: a chord's keys within 20 ms one onset)
  long groupAt_ = -1;
  long pressAt_[128] = {};   // (when each key went down: a long-held high note released softly)
  Rng palletRng_{0x9A11E7};
  void drawPallet(int div, int key, bool opening);
  void headStart(Pipe& pp);
  double kSympScale = 1.0;  // (the field's scale against a mode's state: set when built)
  std::vector<float> organHist_ = std::vector<float>(2048, 0.0f);
  int organPos_ = 0;
  std::vector<char> drawn_;  // by kStops' index
  bool crescendo_ = false;
  double roller_ = -1;
  std::vector<int> byPedal_[128];
  bool held_[128] = {}, pedalOn_[128] = {};
  bool heldOn_[4][128] = {};
  bool coupler_[4][4] = {};  // [from][to]: the from-division's stops sound on the to-division's keys
  bool byManual_ = false, autoPedal_ = false, zones_ = false;    // ("all": playing by manuals, the registration set by setRegistration)
  bool manualRanks_ = false;
  bool tremSw_ = false, tremPos_ = false;
  int split_ = 55;
  std::vector<Pipe> pipes_;
  std::vector<int> byKey_[128];
  std::vector<Enclosure> enc_;
  // To the two microphones in the nave (each enclosure's): its low band to
  // both alike, its highs a little to its side, the far one a little later.
  struct Mic {
    float lo1 = 0, lo2 = 0;   // the crossover's low-pass
    float gL = 1, gR = 1;     // the highs' level to each
    int delayL = 0, delayR = 0;
    float ringL[64] = {}, ringR[64] = {};
  };
  std::vector<Mic> mic_;
  // The rendering, by blocks: each enclosure (its pipes, their field) on its
  // own (they meet only in the wind, which moves slowly: its pressures read
  // at a block's start, its flows averaged over it), on a few threads.
  static constexpr int kBlock = 16;
  void renderBlock();
  void renderEnclosure(int e);
  std::vector<std::vector<int>> encPipes_;
  std::vector<std::array<float, kBlock>> encOut_;
  std::vector<std::array<double, 4>> encDemand_;
  float church_[kBlock] = {}, windNow_[4] = {1, 1, 1, 1};
  int blockPos_ = kBlock;
  std::unique_ptr<WorkPool> pool_;
  OrganWind wind_;
  bool windOn_ = true;
  float xoverA_ = 0;
  int micPos_ = 0;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

class OrganPlayer : public Performer {
 public:
  explicit OrganPlayer(OrganModel& m);
  // A part played by manuals (a MIDI file's organ part: src/organplan.h):
  // a key on a division; a registration; the swell's opening.
  void noteOnManual(int division, int midi, double touch = 0) { m_.openOn(division, midi, touch); }
  void noteOffManual(int division, int midi) { m_.closeOn(division, midi); }
  void setRegistration(const std::string& stops) { m_.setRegistration(stops); }
  void setSwellOpening(double s) { swellFixed_ = s; }
  void setAutoPedal(bool on) { m_.setAutoPedal(on); }
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}

 private:
  void zoneLevel();
  OrganModel& m_;
  const char* lastDyn_ = nullptr;
  double swellFixed_ = -1;  // (>= 0: set by a plan, not the dynamics)
  // The keyboard in zones (-inst organ-88): each key's division and the key
  // it plays there, kept for its release; the level last registered.
  bool zones_ = false;
  int split1_ = 48, split2_ = 65, pedalUp_ = 12;
  int keyDiv_[128], keyTo_[128];
  int level_ = -2;
  const char* lastLevel_ = nullptr;
};

}  // namespace pajupilli
