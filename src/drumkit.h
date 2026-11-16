// The drum kit: General MIDI's drum keys (channel 10: 35 .. 81) on the
// percussion that is built. The MIDI file player (-play) asks drumFor() which
// instrument plays a key; live (-inst drums, -pads drums) plays a DrumKit:
// one small pool for each of those instruments, all in the same room.
//
// (The kit's own models, cymbals, hi-hat, snare (research/jazz_drums.md,
// research/cymbal_plan.md), are to be routed here.)
#pragma once

#include <memory>
#include <string>
#include <vector>

#include "calibrate.h"
#include "ensemble.h"
#include "instrument.h"

namespace pajupilli {

// A drum key to an instrument built and the key it plays there (before that
// instrument's transposition); {nullptr, -1}: not built, silent.
struct Drum { const char* inst; int key; };
// Which drummer plays. kBrush: the file's drum channel selects a Brush kit
// (its program, as sent: 40 .. 47; Yamaha XG uses the same numbers), or
// PJ_KIT=brush asks for it: brushes, and key 40 is the held swirl. kJazz: a
// jazz group (-play: the file has a piano and an upright bass): the brushed
// snare. kStandard: the usual file: the stick snare.
enum class Kit { kStandard, kJazz, kBrush };
inline Kit kitFor(int program) {
  return program >= 40 && program <= 47 ? Kit::kBrush : program >= 32 && program <= 39 ? Kit::kJazz : Kit::kStandard;
}
bool brushKitWanted();
// gs: the file says it is Roland GS or General MIDI 2 (what the keys below 35
// mean; else they are read as Yamaha XG's).
Drum drumFor(int gmKey, Kit kit = brushKitWanted() ? Kit::kBrush : Kit::kStandard, bool gs = false);
const char* drumName(int gmKey);  // General MIDI's name of the key ("" outside 35 .. 81)

class DrumKit {
 public:
  // voices: strokes ringing at once on each instrument.
  // Live, the kit is the Brush kit unless told otherwise.
  explicit DrumKit(const RoomSpec* room = nullptr, bool verbose = false, int voices = 4, Kit kit = Kit::kBrush);
  // A stroke on a drum key; false: nothing is built for it.
  bool noteOn(int gmKey, int velocity);
  void noteOff(int gmKey);
  void allOff();
  // 48 kHz stereo, added to what is in left and right.
  void renderAdd(float* left, float* right, int n);
  int resets() const;
  std::string describe() const;  // the instruments, and the keys that are silent

 private:
  struct Part {
    const Instrument* inst;
    std::unique_ptr<Calibration> cal;
    std::unique_ptr<Ensemble> ens;
  };
  std::vector<Part> parts_;
  int part_[128], key_[128];  // per drum key: its part (-1: none) and the key played there
  bool held_[128] = {};
  std::vector<float> l_, r_;
};

}  // namespace pajupilli
