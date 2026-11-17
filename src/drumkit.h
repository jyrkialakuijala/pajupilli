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

// The brush player's own swirls on a cymbal (-play, with the brushed snare:
// research/jazz_drums.md 5.7). Now and then, for a few bars, the drummer
// takes the right hand off the snare and swirls that brush on the ride (or
// a crash) while the left hand goes on circling on the snare: in quiet,
// sparse passages and ballads, leading into a phrase's or a section's end;
// two to eight bars; at most a few times in a piece; never on a cymbal the
// file strikes then (the swirl ends just before a stroke on it).
// These keys are not General MIDI's: they are what -play sends for it on
// the drum channel (with any kit but the Standard), held from the swirl's
// start to its end.
constexpr int kKeyRightHandAway = 124;  // the brushed snare's key 42: its right hand away
constexpr int kKeySwirlCrash = 125, kKeySwirlCrash2 = 126, kKeySwirlRide = 127;  // the cymbal's key 84: swirled
struct DrumPlanNote {
  double t;      // s
  int key, velocity;
  bool drum;     // on the drum channel (key: General MIDI's)
  Kit kit;       // (the drum channel's kit then)
};
struct DrumTempo { double t, beat, len; };  // from t (s), at `beat` quarter notes, a quarter lasts len s
struct CymbalSwirl {
  double on, off;      // s
  int key, velocity;   // a kKeySwirl* key
  int bar0, bars;      // (where: the bar it begins on, from 0, and how many)
};
// The swirls for a song: its note-ons in time order and its tempo map. The
// same notes give the same swirls (the choice is seeded from them).
// PJ_SWIRL=0: none; PJ_SWIRL_MAX: at most this many.
std::vector<CymbalSwirl> planCymbalSwirls(const std::vector<DrumPlanNote>& notes, const std::vector<DrumTempo>& tempo, bool gs);

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
