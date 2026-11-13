// What every instrument here shares: the air column (Bore), its tone holes with
// moving pads or fingers (pad pumping, key clicks, landing thuds), radiation
// from every opening to a listener 1 m away, and the room's sound acting on the
// openings. The excitation (a jet, a reed) is the derived class's sample().
#pragma once

#include <cstdint>

#include "bore.h"
#include "instrument.h"

namespace pajupilli {

constexpr int kMaxHoles = 32;

// Inner end correction of a side hole (Dalmont / Nederveen), delta = b / a.
double innerCorrection(double b, double delta);
constexpr int kHoles = 17;  // the Boehm flute's (the 17th: the C key of a B foot)

class Tube {
 public:
  Tube(const Instrument& inst);
  virtual ~Tube() = default;
  const Instrument& instrument() const { return inst_; }

  // Fingering, hole 1 .. n: 'o' open, 'x' closed, 'h' half-holed (a finger
  // hole a quarter open); left out: open. fingerSpeed scales how fast the
  // fingers move (key velocity: 1 = 10 ms, 2 = 5 ms). Springs take their own time.
  virtual void setHoles(const char* pattern, double fingerSpeed = 1);
  // The next setHoles glides (a portamento) over |s| seconds: the fingers
  // slide off and onto the holes one after another (s > 0, rising: from the
  // bottom hole up; falling: from the top down), keys and valves move slowly.
  void setGlide(double s) { glide_ = s; }
  // A pan flute: the pipe for this note (others: nothing).
  virtual void selectPipe(int /*midi*/) {}
  // Another instrument and player of the same kind: its own noise and key timing.
  virtual void seed(unsigned s) {
    rng_ = 22222u + 7919u * s;
    keyRng_ = 777u + 104729u * s;
  }

  // Render n mono samples (Pa at 1 m, roughly).
  void process(float* out, int n);
  // The room's sound pressure outside (Pa) for the next samples of process(),
  // one per sample at the model's rate; it acts on the open holes, the end and
  // (a flute) the embouchure. The last value holds when it runs out.
  void setRoom(const float* p, int n) { room_ = p; roomN_ = n; roomPos_ = 0; }
  double outScale() const { return kOut_; }  // output = Pa at 1 m * outScale

  // Tests: a unit drive at the excitation on the next sample (a flute: a
  // pressure in the window; a reed: a volume flow into the mouthpiece), and
  // the response seen there (window flow; mouthpiece pressure).
  virtual void testDrive(double v) { testDp_ = v; }
  virtual double testResponse() const = 0;
  // Clear all air motion (keys and settings stay); done automatically on NaN.
  virtual void resetAir();
  int resets() const { return resets_; }
  double fs() const { return fs_; }
  const Bore& bore() const { return bore_; }

 protected:
  struct Hole {
    double x;         // centre from the top (m)
    double b;         // radius (m)
    double t;         // chimney height (m)
    double padH;      // key opening (m) (finger: lift)
    double padR;      // key cup radius (m) (finger: half its width)
    bool perforated;  // French (open-hole) key
    bool finger;      // a finger hole
    bool normallyClosed;
    int cell;
    double tSeries;   // chimney + inner + matching length (m), incl. grid offset
    double tRad;      // radiation (outer) end correction (m)
    double open = 0, target = 0;  // pad openness 0..1 (time along the stroke)
    bool closing = false;         // closing stroke: accelerates into the seat
    double stroke = 0.010;        // duration of the current stroke (s)
    double wait = 0;              // the stroke starts after this delay (s)
    float thRe = 0, thIm = 0;     // landing thud: the pad on its felt (flow, m^3/s)
    bool held = false;            // pressed into its felt by a closing stroke
    double padIn = 0;             // air the pad pushes into the bore (m^3/s)
    double tClosed = 0;           // closed chimney length, incl. matching volume (m)
    double taClosed = 0;          // series length of the closed hole (negative, m)
    double a = 0;                 // bore radius at the hole (m)
    bool present = true;          // a piccolo has no C# and D holes (D foot)
    double glideWait = 0, glideStroke = 0;  // a glide's stroke: its delay and time (s)
  };

  // Geometry, in this order: the bore (diameter at x from the top, the
  // excitation at exciteX, the end), then the holes, then the radiators.
  void setGeometry(const std::function<double(double)>& diameter, double exciteX, double endX, bool closed);  // (Webster for designs)
  void addHole(double x, double b, double t, double padH, double padR, int flags, bool present = true);
  void finishHoles();          // cells, grid compensation, series lengths; holes at rest
  void addRadiators(bool window, double windowX);  // the window (a flue), the end, each hole cell
  void restHoles(const char* pattern);  // the fingers at rest (no strokes)

  void updateHoles(double dtBlock);
  // The branch of every hole at cell c, summed (several holes may share a cell).
  Branch cellBranch(int c, bool* any);
  // Nonlinear losses at the open holes: at playing levels the flow in a small
  // hole separates and sheds vortices (Dalmont et al. 2002; Atig et al. 2004),
  // a resistance ~ rho |u| / (2 Cd^2 S^2). Linearised for the flow's amplitude
  // over each control block (describing function, 8 / 3 pi), per hole cell.
  void holeLosses();
  int nLossCells_ = 0;
  int lossCell_[kMaxHoles];
  double lossAcc_[kMaxHoles] = {}, lossR_[kMaxHoles] = {}, lossK_[kMaxHoles] = {};
  double kHoleNl_ = 0;
  double kHalf_ = 0.25;  // a half-holed finger hole: the share of its area left open
  double keyDelay();  // a random 0..PJ_KEYSPREAD start delay (s)
  Branch holeBranch(const Hole& h) const;
  // One bore step with the pads' puffs and thuds; the caller then solves the
  // excitation's cell (pOld, J: see Bore::step).
  void stepBore(float* pOld, float* jIn);
  // Radiation: d(flow)/dt of each opening, delayed to the listener; the window's
  // flow (a flue) is `windowQ`. Returns this sample's pressure (before extras).
  float radiate(double windowQ);
  // Key ticks, DC blocker, output scale.
  float finish(float y);

  double glide_ = 0;  // setGlide
  virtual void controlStep() = 0;
  virtual float sample() = 0;

  const Instrument& inst_;
  double fs_;
  Bore bore_;
  Hole hole_[kMaxHoles];
  int nHoles_ = 0;
  int ec_ = 0;  // the excitation's cell
  std::function<double(double)> diam_;  // bore diameter (m) at x (m) from the top
  double endX_ = 0;
  double holeT_ = 0;  // default chimney (flute: tube wall + drawn chimney)
  // Radiators.
  int nRad_ = 0;
  int radCell_[kMaxHoles + 2];
  int radDelay_[kMaxHoles + 2];
  float radGain_[kMaxHoles + 2];
  float radPrev_[kMaxHoles + 2] = {}, radPrev2_[kMaxHoles + 2] = {};
  static constexpr int kOutRing = 1024;
  float outRing_[kOutRing] = {};
  int outPos_ = 0;
  uint32_t rng_ = 22222;
  double noiseNorm_ = 1;
  double whiteNoise() {
    rng_ = rng_ * 1664525u + 1013904223u;
    return int32_t(rng_) * (1.0 / 2147483648.0) * 1.732 * noiseNorm_;
  }
  float dcX_ = 0, dcY_ = 0, dcPole_ = 0.99935f;
  int ctl_ = 0;
  double testDp_ = 0;
  const float* room_ = nullptr;
  int roomN_ = 0, roomPos_ = 0;
  float pRoom_ = 0;
  int resets_ = 0;
  int kRadOnly_;
  double kOut_;
  double kSeries_ = 1;
  double fingerSpeed_ = 1, kPadPump_, kKeyClick_, kKeySpread_, kThud_, kRelease_;
  float thudCos_ = 1, thudSin_ = 0, thudDecay_ = 0;
  int thudLeft_ = 0;  // samples until every thud has died out
  int nPumping_ = 0;
  uint32_t keyRng_ = 777;
  // Key impact: a few damped modes of the key and the tube wall.
  static constexpr int kTickModes = 3;
  double tickRe_[kTickModes] = {}, tickIm_[kTickModes] = {}, tickCos_[kTickModes] = {},
         tickSin_[kTickModes] = {}, tickDecay_[kTickModes] = {};
};

}  // namespace pajupilli
