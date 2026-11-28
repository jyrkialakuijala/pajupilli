// Shakers and jingles (research/shakers.md): -inst maracas, shaker, cabasa,
// tambourine, jingle-bell; General MIDI's drum keys 70, 82, 69, 54, 83.
//
// Many small hard things (seeds, grains, beads, jingle discs, bells' balls)
// thrown about by the hand's stroke, each collision a click ringing a body's
// few broad resonances (PhISEM's insight: what is heard is the statistics of
// the collisions). Here the things themselves are simulated, cheaply, so that
// the stroke's timing and the state left by the last one are physical:
//   - rattle (maracas, shaker): tens of particles in 1-D in a shell; the hand
//     stops, they fly on and hit the far wall together (the "chick"), bounce
//     with little restitution and scatter off each other;
//   - rattlers (tambourine, jingle bell): jingle discs in their slots (or a
//     bell's ball in its bell) thrown against their stops and each other,
//     every hit ringing the discs' (bells') own modes; the frame knocks;
//   - scraper (cabasa): bead chains ridden over a corrugated cylinder by the
//     twist: dense clicks at the ridge rate, into a broad metal resonance.
// One object for the whole song: a stroke meets what is still moving.
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "rng.h"
#include "tambgesture.h"
#include "tube.h"

namespace pajupilli {

bool shakerInstrument(const Instrument& inst);  // maracas, shaker, cabasa, tambourine, jingle-bell

class Shaker {
 public:
  enum class Type { kMaracas, kShaker, kCabasa, kTambourine, kJingleBell };
  Shaker(double fs, Type type, uint32_t seed = 1);
  Type type() const { return type_; }
  // A stroke: the hand's speed (m/s) when it stops; hand: which maraca (0, 1),
  // or the shaker's push (0) and pull (1).
  void stroke(double speed, int hand);
  // The cabasa: the twist goes on while the key is held, and eases off at its
  // release.
  void release();
  // The cabasa played in one of its ways (ShakerPlayer picks by velocity):
  // turning on one way (the chains keep sliding, no reversal), twisting back
  // and forth (each reversal throws the chains over), shaken in the air (the
  // chains thrown along it, a short scrape), or struck against the other hand
  // (the palm's thump on the cylinder, every chain thrown, the beads sliding).
  enum class Cabasa { kTurn, kTwist, kShake, kSlap };
  void cabasa(double speed, Cabasa how, int dir);
  // A maraca (hand h) played in one of its ways: the plain slap; a flick
  // (the wrist snapped: every seed thrown in a tight bunch, a firmer slap);
  // two slaps in one movement (out and back, the second weaker, gap s
  // later); from the chest (lead s of the seeds starting to slide as the hand
  // moves off, a small woosh, then the slap). The slap lands lead s after
  // this call (the player's notes come that much early in a song).
  enum class Maraca { kSlap, kFlick, kDouble, kChest };
  void maraca(int h, double speed, Maraca how, double lead, double gap);
  // Held on: the maraca turned in loops, the seeds rolling round the wall,
  // until stopped.
  void maracaRoll(int h, double speed);
  void maracaStop(int h);
  // The tambourine played by a gesture (research/tambourine.md 1.2): what moves
  // the jingles, where it is held, its tilt, strength, lift and what follows.
  // A roll with dur 0 goes on until tambourineRollEnd() (a held key).
  void tambourine(const TambGesture& g);
  void tambourineRollEnd();
  bool tambourineRolling() const { return roll_.on; }
  float sample();
  float side() const { return side_; }
  bool asleep() const { return asleep_; }
  void reset();
  void seed(uint32_t s) { rng_.reseed(s * 2654435761u + 1u); }

 private:
  struct Mode {  // a two-pole resonator: state, rotation per sample, output gain
    float re = 0, im = 0, pr = 0, pi = 0, g = 0;
    float hz = 0;  // (its frequency: what a hard contact reaches)
  };
  struct Particle {
    float x = 0, u = 0;  // place in the cavity (m), speed (m/s)
    float m = 0, e = 0;  // mass (kg), restitution against the wall
    bool moving = false;
  };
  struct Shell {  // a maraca, the shaker's tube, the cabasa's bead chains (their swing)
    std::vector<Particle> p;
    std::vector<Mode> modes;
    float L = 0.06f;             // the cavity's free length (m)
    float in[4] = {}, lp[4] = {};  // per mode: the impulses this sample; the contact's low-pass
    float tick = 0, tickLp = 0;  // the particles' own clicks against each other
    int moving = 0;
    float pan = 0;
  };
  struct Rattler {  // a jingle pair in its slot, or a bell with its ball
    float z[2] = {}, u[2] = {};  // the masses' places along the pin (m), their speeds
    int masses = 2;
    float gap = 2e-3f, m = 6e-3f, e = 0.6f;
    int mode0 = 0, nmode = 0;    // its own modes in rattleModes_
    int perDisc = 3;             // ... of them each disc's (a bell's)
    bool moving = false;
    // This stroke's arrangement: when the frame's kick reaches it (samples to
    // go, the speed it brings), how bouncy (tilted on its pin) and how much
    // of its slot is free.
    int delay = 0;
    float kick = 0, eS = 0.6f, gapS = 2e-3f;
  };
  float ur();  // uniform [0, 1)
  float un();  // about normal (zero mean, unit variance)
  void addMode(std::vector<Mode>& v, double hz, double q, double gain);
  void excite(std::vector<Mode>& v, int from, int n, float J);
  // speed: how fast the disc met its stop or the other disc (m/s); with
  // hard_ > 0 a faster meeting is a briefer contact, reaching higher modes.
  void exciteRim(std::vector<Mode>& v, int from, int n, float J, float speed = 0);
  float ring(std::vector<Mode>& v, int from, int n);
  void throwShell(Shell& s, double speed, double dir, double spreadK = 1, double frac = -1);
  void stepGesture(int h);
  struct Gest {  // a maraca's gesture in time
    bool on = false, rolling = false;
    Maraca how = Maraca::kSlap;
    double t = 0, lead = 0, gap = 0, speed = 0, rollSpeed = 0;
    int dir = 1;
    bool thrown = false, second = false;
  };
  Gest gest_[2];
  double rollT_[2] = {};
  void hit(Shell& s, float J);
  float stepShell(Shell& s);
  void hitRattlers(double speed);
  // The jingles thrown: each pair (with probability frac) kicked by speed,
  // the frame rocking by rock about the angle psi (the pairs on that side
  // harder), the kicks spread over spread s; twist: in order round the ring,
  // the far ones harder; eK: their bounce scaled.
  void throwRattlers(double speed, double frac, double rock, double psi, double spread, bool twist, double eK);
  void tambStroke(const TambGesture& g);  // the stroke itself, now
  void tambChoke();
  struct TPend {  // a part of a gesture still to come
    double at;
    int what;  // 0 the stroke (g), 1 the lift (a smear before it), 2 a choke
    TambGesture g;
  };
  std::vector<TPend> pend_;
  double tNow_ = 0;
  struct Roll {
    bool on = false, thumb = false;
    double t0 = 0, end = 0, next = 0;  // s (end: far away while the key is held)
    float s0 = 1, s1 = 1;               // the speed at its start and end
    int dir = 1;
    float tilt = 0.9f;
    bool endStroke = true;  // (false: the next stroke ends it, a lead-in)
  } roll_;
  void stepRoll();
  float hiLp_ = 0;                   // the height's shelf: the low part, now
  float height_ = 0.5f, heightT_ = 0.5f;
  float gTrim_ = 1;  // (played by gestures: their level to the single stroke's, PJ_TAMB_GTRIM dB)
  float hAt_ = 0.5f, hHi_ = 1, hAll_ = 1, hLpA_ = 0;  // (the shelf's gains at height hAt_; its low-pass)
  int choke_ = 0;                    // samples left of the hand pressing the jingles still
  std::vector<Mode> head_;           // the head (struck, or the thigh against it): two damped modes
  float stepRattlers();
  float stepScraper();

  Type type_;
  double fs_, dt_;
  Rng rng_{1};
  std::vector<Shell> shells_;
  std::vector<Rattler> rat_;
  std::vector<Mode> rattleModes_, frame_;
  float gEff_ = 9.81f;  // gravity along the jingles' pins (the frame's tilt), per stroke
  float contactA_ = 0;  // the contact's low-pass coefficient (~40 us)
  // The jingles' contact: the frequency a meeting at 1 m/s still reaches
  // (Hz), and how it grows with the speed (Hertz: ~v^0.2 for the contact's
  // brevity; more here, the discs' edges and cupping); 0: every mode alike.
  float hardHz_ = 0, hard_ = 0;
  // The cabasa's twist: speed now (m/s), its course.
  double twistS_ = 0, twistPeak_ = 0, twistT_ = 0, twistDur_ = 0;
  bool twistHeld_ = false, twistOn_ = false, releasing_ = false;
  double releaseT_ = 0, releaseFrom_ = 0;
  std::vector<Mode> metal_;
  std::vector<Mode> thump_;  // the cabasa's handle and cylinder against the palm
  int twistDir_ = 1;
  float scrapeLp_ = 0, scrapeHp_ = 0, scrapeX_ = 0;
  float side_ = 0, gain_ = 1, prevY_ = 0, prevS_ = 0;
  bool asleep_ = true;
  int quiet_ = 0;
};

class ShakerModel : public Tube {
 public:
  explicit ShakerModel(const Instrument& inst);
  Shaker& shaker() { return sh_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Shaker sh_;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
};

// The hands. Velocity is the hand's speed at its stop (0.3 .. 2.5 m/s, about
// as velocity^1.2: General MIDI's 30 dB from 16 to 127). Maracas: the two
// hands in turn; the shaker: push and pull in turn (the pull weaker); the
// cabasa: a twist as long as the key is held (at least 60 .. 150 ms).
class ShakerPlayer : public Performer {
 public:
  explicit ShakerPlayer(ShakerModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool) override {}
  void seed(unsigned s) override { m_.shaker().seed(s + 17); }
  static double handSpeed(int velocity, double dynamics, double exponent = 1.2);  // m/s

 private:
  ShakerModel& m_;
  int hand_ = 0;
  int cabDir_ = 1, cabCount_ = 0;  // the cabasa: the way it turns; notes counted (live: the bar)
  int marHand_ = -1;                // the maraca whose key is held, and for how long (s)
  double marHeld_ = 0, marSpeed_ = 0;
  bool marRolling_ = false;
  // The tambourine (research/tambourine.md 1.2): its gesture planned for the song (src/tambplan.h),
  // or, live, chosen from the velocity and how soon the key came after the last; a key held on
  // live becomes a roll.
  void tambourineOn(int velocity);
  Rng hand_rng_{7};
  double clock_ = 0, lastTamb_ = -1e9;  // s
  bool tambHeld_ = false, tambPlanned_ = false;
  double tambHeldT_ = 0;
  int tambVel_ = 0, tambDir_ = 1;
};

}  // namespace pajupilli
