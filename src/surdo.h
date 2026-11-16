// The surdo (-inst surdo; General MIDI 2's 86 mute and 87 open, XG's 13 and
// 14): the deep two-headed bass drum of samba (research/clap_claves_surdo.md
// 4). A 22" x 55 cm primeira, heavy heads tuned low and booming (62 Hz),
// built as the toms are (tom.h: two heads on a shell of air, the tension
// solved for the note), struck with a padded mallet: a 0.15 kg felt head, a
// soft contact of a few ms, little click. The other hand mutes: a mute
// stroke (86) is played with the hand on the batter, damping it at once (and
// whatever still rings); an open stroke (87) lifts the hand first.
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tom.h"
#include "tube.h"

namespace pajupilli {

struct Instrument;
bool surdo(const Instrument& inst);  // -inst surdo
TomSpec surdoSpec();

class SurdoModel : public Tube {
 public:
  explicit SurdoModel(const Instrument& inst);
  Tom& drum() { return drum_; }
  void setHand(bool on);  // the hand on the batter (the mute)
  bool hand() const { return hand_; }
  double testResponse() const override { return last_; }
  const float* takeSide(int n) override;
  void resetAir() override;

 private:
  void controlStep() override {}
  float sample() override;
  Tom drum_;
  bool hand_ = false;
  float last_ = 0;
  std::vector<float> side_, sideOut_;
  std::vector<float> delay_;  // the room's pressure, for the far head
  int delayPos_ = 0;
};

class SurdoPlayer : public Performer {
 public:
  explicit SurdoPlayer(SurdoModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int) override {}
  void render(float* out, int n) override;
  void setHold(bool) override {}
  void seed(unsigned s) override { rng_ = 811u + 7919u * s; }
  static double malletSpeed(int velocity, double dynamics);  // m/s

 private:
  double ur();
  SurdoModel& m_;
  uint32_t rng_ = 811u;
};

}  // namespace pajupilli
