// The scraped, rubbed, rattled and blown percussion of General MIDI's drum
// keys (research/scrapers_friction.md):
//
//   - guiro (73 short, 74 long): a stick tip dragged over notches; it climbs
//     each notch and drops onto the next, a train of impacts at the stroke's
//     speed over the notch spacing, into the gourd's cavity and shell.
//   - cuica (78 mute, 79 open): a bamboo stick tied to the head's centre,
//     rubbed with a damp cloth: stick-slip (as a bow) against the head's
//     modes at its centre; the thumb outside presses the pitch up.
//   - vibraslap (58): the wire swings the box, the loose pins inside rattle
//     against its walls while the swing dies.
//   - pea-whistle (71 short, 72 long): a pea whistle, the jet driving a small
//     cavity, the pea trilling it.
//
// Each is its own instrument (one object living through the song; a new
// stroke takes the stick, cloth or breath where it is).
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool scraper(const Instrument& inst);  // -inst guiro, cuica, vibraslap, pea-whistle

// A resonance as a decaying rotation (complex state), driven by a force or
// pressure each sample; its output is the imaginary part.
struct Ring {
  float re = 0, im = 0, cr = 1, ci = 0, g = 0;
  void set(double fs, double hz, double q, double gain);
  float step(float in) {
    const float r = re * cr - im * ci + in, i = re * ci + im * cr;
    re = r;
    im = i;
    return g * im;
  }
};

class ScraperModel : public Tube {
 public:
  enum class Kind { kGuiro, kCuica, kVibraslap, kWhistle };
  explicit ScraperModel(const Instrument& inst);
  Kind kind() const { return kind_; }
  // The guiro: a stroke of length len (m) over dur (s), pressing with press
  // (N), each the other way. Strokes go in pairs over the lower notches, then
  // the upper ones (low-low, high-high); connected: the stick stays on the
  // notches between strokes and the next starts where it is (soft playing),
  // else it is lifted and set down again at the stroke's start.
  void scrape(double len, double dur, double press, bool connected = false);
  // The cuica: the cloth's stroke, peak speed (m/s), grip (N), duration (s),
  // the thumb's pitch ratio at the start and the end, extra damping (x).
  void rub(double speed, double grip, double dur, double thumb0, double thumb1, double damp);
  // The vibraslap: the slap's speed (m/s).
  void slap(double speed);
  // The whistle: the breath (0 .. 1) for dur (s).
  void blow(double breath, double dur);
  bool asleep() const { return asleep_; }
  double testResponse() const override { return last_; }
  void resetAir() override;
  // Measurements (tests): impacts so far (guiro, vibraslap), the cuica's
  // head pitch ratio now, the whistle's pea rate.
  int impacts() const { return impacts_; }
  double thumb() const { return thumb_; }

 private:
  void controlStep() override {}
  float sample() override;
  float guiro();
  float cuica();
  float vibraslap();
  float whistle();
  double ur();  // 0 .. 1
  Kind kind_;
  double dt_;
  uint32_t rng_ = 7771u;
  bool asleep_ = true;
  int quiet_ = 0, impacts_ = 0;
  float last_ = 0;

  // Guiro: the stroke (x along the notches, its time), the tip (height y,
  // speed), the hand's press; the body.
  double gx_ = 0, gt_ = 0, gDur_ = 0, gLen_ = 0, gDir_ = 1, gPress_ = 0;
  double gy_ = 0, gvy_ = 0, gHand_ = 0;
  bool gOn_ = false, gTouch_ = false;
  std::vector<Ring> gBody_;
  Ring gClick_;
  float gLastF_ = 0;
  // Each notch's own ring (the gourd's wall under it: higher towards the
  // narrow top), a few rings taken in turn as the tip drops onto notches.
  std::vector<double> gNotchHz_;
  Ring gRidge_[4];
  int gRing_ = 0, gNotch_ = -1, gCount_ = 0;
  bool gConnected_ = false;

  // Cuica: the head's (0,n) modes at its centre (displacement, speed), their
  // pitch now (the thumb), the cloth.
  static constexpr int kHead = 6;
  double hx_[kHead] = {}, hv_[kHead] = {}, hw0_[kHead] = {}, hm_[kHead] = {}, hg_[kHead] = {}, hvol_[kHead] = {};
  double thumb_ = 1, thumb0_ = 1, thumb1_ = 1, cDamp_ = 1;
  double ct_ = 0, cDur_ = 0, cSpeed_ = 0, cGrip_ = 0;
  bool cOn_ = false;
  double prevA_ = 0;

  // Vibraslap: the wire and box's swing; the pins (position in the box,
  // speed, gap, mass); the box's wood and the pins' ring.
  double sx_ = 0, sv_ = 0;
  static constexpr int kPins = 6;
  double px_[kPins] = {}, pv_[kPins] = {}, pGap_[kPins] = {}, pM_[kPins] = {};
  std::vector<Ring> sBox_, sPin_;

  // Whistle: the cavity (pressure, its rate), the breath's envelope, the pea.
  double wp_ = 0, wq_ = 0, wBreath_ = 0, wTarget_ = 0, wt_ = 0, wDur_ = 0, peaPh_ = 0, peaRate_ = 30;
  Ring wNoise_;
};

// The player: the keys as General MIDI's. Velocity is the stroke's speed and
// press (guiro), the cloth's speed and grip (cuica), the slap (vibraslap),
// the breath (whistle). Each stroke a little different (a hand).
class ScraperPlayer : public Performer {
 public:
  explicit ScraperPlayer(ScraperModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 9157u + 7919u * s; }

 private:
  double ur();
  ScraperModel& m_;
  uint32_t rng_ = 9157u;
};

}  // namespace pajupilli
