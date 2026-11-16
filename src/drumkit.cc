#include "drumkit.h"

#include <cstdlib>
#include <cstring>

namespace pajupilli {

bool brushKitWanted() {
  const char* k = std::getenv("PJ_KIT");
  return k && std::strcmp(k, "brush") == 0;
}

Drum drumFor(int k, Kit kit, bool gs) {
  const bool brush = kit == Kit::kBrush;
  // The usual snare is played with sticks (src/sticksnare.cc: the same drum
  // under sticks, with its rim shot and roll). A jazz group's (Kit::kJazz: a
  // file with a piano and an upright bass) and a Brush kit's is the brushed
  // snare.
  const bool sticks = kit == Kit::kStandard;
  if (sticks) {
    if (k == 37) return {"stick-snare", 37};                  // the cross-stick
    if (k == 38 || k == 40) return {"stick-snare", k};
    if (!gs && k == 34) return {"stick-snare", 41};           // XG open rim shot
    if (!gs && k == 29) return {"stick-snare", 36};           // XG snare roll (held)
    if (gs && k == 25) return {"stick-snare", 36};            // GS / GM2 snare roll (held)
    if (!gs && k == 31) return {"stick-snare", 38};           // XG low snare
  }
  // Otherwise the snare is the brushed snare. The Brush kit: 38 a tap, 39 a
  // slap, 40 the swirl, held. The Jazz kit, where 40 is a second snare and
  // not a held sound: 38 a tap, 40 a tap nearer the edge, 39 (the clap) a
  // slap. 37: the cross-stick, on the same drum.
  if (k == 39 && kit != Kit::kBrush) return {"hand-clap", 39};  // (the Brush kit's 39 is the slap)
  if (k == 37) return {"brush-snare", 37};
  if (k >= 38 && k <= 40) return {"brush-snare", brush || k != 40 ? k : 41};
  if (k == 42 || k == 44 || k == 46) return {"hi-hat", k};  // closed, the foot, open
  // Below 35 General MIDI 1 defines nothing. Files that play there are
  // Yamaha XG's (its brush strokes and extra snares live there) unless they
  // say they are Roland GS or General MIDI 2 (clicks, scratches, sticks).
  if (k < 35) {
    if (gs) {
      if (k == 25) return {"brush-snare", 38};                 // snare roll (until one is built: a tap)
      if (k >= 26 && k <= 34) return {"perc-extras", k};
      return {nullptr, -1};
    }
    switch (k) {
      case 15: return {"perc-extras", 27};
      case 16: return {"perc-extras", 28};
      case 17: return {"perc-extras", 29};
      case 18: return {"perc-extras", 30};
      case 19: return {"perc-extras", 26};
      case 20: return {"perc-extras", 20};
      case 21: return {"perc-extras", 33};
      case 22: return {"perc-extras", 34};
      case 23: return {"perc-extras", 23};
      case 24: return {"perc-extras", 24};
      case 30: return {"perc-extras", 21};                     // castanets
      case 32: return {"perc-extras", 31};                     // sticks
      case 25: return {"brush-snare", 38};                     // brush tap
      case 26: return {"brush-snare", 40};                     // brush swirl
      case 27: return {"brush-snare", 39};                     // brush slap
      case 28: return {"brush-snare", 40};                     // brush swirl, the lighter one
      case 29: return {"brush-snare", 38};                     // snare roll (until one is built)
      case 31: return {"brush-snare", 38};                     // low snare
      case 34: return {"brush-snare", 37};                     // open rim shot (until one is built: the cross-stick)
      case 33: return {"jazz-kick-20", 36};                    // low bass drum
      case 13: return {"surdo", 86};                           // surdo, mute
      case 14: return {"surdo", 87};                           // surdo, open
      default: return {nullptr, -1};
    }
  }
  if (k == 85) return {"perc-extras", 21};                     // castanets (General MIDI 2)
  switch (k) {
    case 35: case 36: return {"jazz-kick", k};                                // bass drum (35: lower on the head, 36: nearer its middle)
    case 75: return {"claves", 75};
    case 76: case 77: return {"wood-blocks", k};                             // high, low
    case 84: return {"bell-tree", 84};                                       // General MIDI 2
    case 86: case 87: return {"surdo", k};                                   // General MIDI 2: mute, open
    // The toms (src/tom.cc): six drums; a key of 60 up there is a brush stroke, below a stick.
    case 41: return {"tom-16-low", brush ? 65 : 41};  // low floor tom
    case 43: return {"tom-16", brush ? 67 : 43};      // high floor tom
    case 45: return {"tom-14", brush ? 69 : 45};      // low tom
    case 47: return {"tom-13", brush ? 71 : 47};      // low-mid tom
    case 48: return {"tom-12", brush ? 72 : 48};      // hi-mid tom
    case 50: return {"tom-10", brush ? 74 : 50};      // high tom
    case 56: return {"cowbell", 56};                                          // cowbell
    case 67: case 68: return {"agogo", k == 68 ? 83 : 90};                    // agogo, low / high
    case 60: case 61: return {"bongo", k};                                   // bongos: macho, hembra
    case 62: case 63: case 64: return {"conga", k};                          // mute hi, open hi, low conga
    case 65: case 66: return {"timbale", k};                                 // high, low timbal
    case 58: return {"vibraslap", 58};
    case 71: case 72: return {"pea-whistle", k};                             // 71 short, 72 long
    case 73: case 74: return {"guiro", k};                                   // 73 short, 74 long
    case 78: case 79: return {"cuica", k};                                   // 78 mute, 79 open
    case 80: case 81: return {"triangle", k};                                // mute, open triangle
    // The cymbals (src/cymbal.cc): a brushed stroke is the same key + 24.
    case 51: return {"ride-cymbal", brush ? 75 : 51};                        // ride: the bow
    case 53: return {"ride-cymbal", brush ? 77 : 53};                        // the same ride's bell
    case 59: return {"ride-cymbal-2", brush ? 83 : 59};                      // the second ride
    case 49: return {"crash-cymbal", brush ? 73 : 49};                       // 16" crash
    case 57: return {"crash-cymbal-2", brush ? 81 : 57};                     // 18" crash
    case 55: return {"splash-cymbal", brush ? 79 : 55};                      // 10" splash
    case 52: return {"china-cymbal", brush ? 76 : 52};                       // 18" china
    case 54: return {"tambourine", 54};
    case 69: return {"cabasa", 69};                                          // (twists while held)
    case 70: return {"maracas", 70};
    case 82: return {"shaker", 82};                                          // General MIDI 2
    case 83: return {"jingle-bell", 83};                                     // General MIDI 2
    default: return {nullptr, -1};                                           // not built
  }
}

const char* drumName(int k) {
  static const char* const kNames[47] = {
      "bass drum 2", "bass drum 1", "side stick", "snare 1", "hand clap", "snare 2", "low tom 2", "closed hi-hat",
      "low tom 1", "pedal hi-hat", "mid tom 2", "open hi-hat", "mid tom 1", "high tom 2", "crash 1", "high tom 1",
      "ride 1", "china", "ride bell", "tambourine", "splash", "cowbell", "crash 2", "vibraslap", "ride 2",
      "high bongo", "low bongo", "mute high conga", "open high conga", "low conga", "high timbale", "low timbale",
      "high agogo", "low agogo", "cabasa", "maracas", "short whistle", "long whistle", "short guiro", "long guiro",
      "claves", "high wood block", "low wood block", "mute cuica", "open cuica", "mute triangle", "open triangle"};
  if (k == 82) return "shaker";
  if (k == 83) return "jingle bell";
  if (k == 84) return "bell tree";
  if (k == 85) return "castanets";
  if (k == 86) return "mute surdo";
  if (k == 87) return "open surdo";
  return k >= 35 && k <= 81 ? kNames[k - 35] : "";
}

DrumKit::DrumKit(const RoomSpec* room, bool verbose, int voices, Kit kit) {
  for (int k = 0; k < 128; ++k) {
    part_[k] = key_[k] = -1;
    const Drum d = drumFor(k, kit, false);
    const Instrument* in = d.inst ? findInstrument(d.inst) : nullptr;
    if (!in) continue;
    int p = 0;
    while (p < int(parts_.size()) && parts_[p].inst != in) ++p;
    if (p == int(parts_.size())) {
      Part np;
      np.inst = in;
      np.cal.reset(new Calibration(std::getenv("PJ_NOCAL") ? Calibration() : calibrate(*in, 440, nullptr)));
      np.ens.reset(new Ensemble(*in, *np.cal, voices, verbose, room));
      np.ens->setKeepTails(1.5);  // (a stroke rings on past its key's release)
      parts_.push_back(std::move(np));
    }
    part_[k] = p;
    key_[k] = d.key + in->transpose;
  }
}

bool DrumKit::noteOn(int k, int velocity) {
  if (k < 0 || k > 127 || part_[k] < 0) return false;
  parts_[part_[k]].ens->noteOn(key_[k], velocity);
  held_[k] = true;
  return true;
}

void DrumKit::noteOff(int k) {
  if (k < 0 || k > 127 || !held_[k]) return;
  parts_[part_[k]].ens->noteOff(key_[k]);
  held_[k] = false;
}

void DrumKit::allOff() {
  for (int k = 0; k < 128; ++k) noteOff(k);
}

void DrumKit::renderAdd(float* left, float* right, int n) {
  l_.resize(n);
  r_.resize(n);
  for (Part& p : parts_) {
    p.ens->render(l_.data(), r_.data(), n);
    for (int i = 0; i < n; ++i) { left[i] += l_[i]; right[i] += r_[i]; }
  }
}

int DrumKit::resets() const {
  int n = 0;
  for (const Part& p : parts_) n += p.ens->resets();
  return n;
}

std::string DrumKit::describe() const {
  std::string s;
  for (int p = 0; p < int(parts_.size()); ++p) {
    s += std::string("  ") + parts_[p].inst->name + ":";
    for (int k = 35; k <= 87; ++k)
      if (part_[k] == p) s += std::string(" ") + std::to_string(k) + " " + drumName(k) + ",";
    s.back() = '\n';
  }
  s += "  silent (not built):";
  for (int k = 35; k <= 87; ++k)
    if (part_[k] < 0 && *drumName(k)) s += std::string(" ") + std::to_string(k) + " " + drumName(k) + ",";
  s.back() = '\n';
  return s;
}

}  // namespace pajupilli
