// A large harpsichord (research/harpsichord.md, research/harpsichord_clavi.md):
// a North German double of the Hass kind, 61 keys F1 .. F6, four choirs of
// strings: two at pitch (8'), one an octave up (4'), one an octave down
// (16'). On the grand's engine (piano.h): each string is one "key" of it, a
// plectrum in the hammer's place, a cloth damper in every jack.
//
//   - A register is a row of jacks plucking one choir. The registers are
//     switches: on, its jacks pluck; off, its strings keep their dampers on
//     and only ring along a little through the bridge.
//   - The pluck: the jack rises at the key's speed, its plectrum bends under
//     the string and lets go at a fixed force (the voicing). How hard the key
//     is struck changes the sound hardly at all, as on the instrument; the
//     rows do not pluck together but one after another (the stagger).
//   - The rows pluck at different places: the back 8' an eighth of the
//     string from the nut in the bass to a third in the treble, the front 8'
//     15 mm further on (rounder), the 16' nearer the nut.
//   - The two directions a string swings in are tuned 0.8 % apart by the
//     bridge pin: the tone's slow beating and its two-stage decay.
//
// (A first form: the soundboard is the grand's, each string on the bridge
// point of the piano key of its pitch; the lengths behind the bridge, the
// air in the case, the buff and lute stops and the pedal instrument of the
// plan are not built yet.)
#pragma once

#include <vector>

#include "performer.h"
#include "piano.h"
#include "pianoinst.h"

namespace pajupilli {

bool harpsichord(const Instrument& inst);  // -inst harpsichord, harpsichord-8, harpsichord-full
PianoDesign harpsichordDesign();

// The player. The registers: `harpsichord` 8' + 8' (General MIDI's), `-8` the
// back 8' alone, `-full` 16' 8' 8' 4'; PJ_HPSI_STOPS (e.g. "16,8,4") says
// otherwise. The sustain pedal is a register pedal, as the revival
// instruments had: down, it adds the 16' and the 4' to what is drawn (they
// take hold at the next pluck). Keys outside F1 .. F6 are folded in by octaves.
class HarpsichordPlayer : public Performer {
 public:
  static constexpr int kChoirs = 4, kKeys = 61, kLow = 29;  // choirs: 8' back, 8' front, 4', 16'
  explicit HarpsichordPlayer(PianoModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned) override {}

 private:
  int keyOf(int midi) const;
  PianoModel& m_;
  bool on_[kChoirs] = {true, true, false, false};
  struct Due { double in; int key; double speed; };
  std::vector<Due> due_;  // jacks on their way up
};

}  // namespace pajupilli
