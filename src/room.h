// The room the instruments play in: a reverberant space of its own that they
// all sound into and that sounds back into them.
//
// - The room is a shoebox (2 : 1.4 : 0.7) of the given volume. The players sit
//   on a line across the stage near the front wall (high instruments on the
//   left); the listener is a pair of microphones 2 m apart further into the
//   room, aimed out to the sides (55 degrees, between omni and cardioid).
// - Early sound: the direct sound and the reflections off the six surfaces up
//   to second order (image sources: 24 per player and microphone), each 1/r,
//   r/c late and reflected sqrt(1 - alpha) per wall (alpha = A / S; hard side
//   walls, an absorbing audience floor), picked up by the microphone facing
//   them: a player on the left is heard off the right wall in the right
//   channel (the stage mirrored), within the first tens of milliseconds.
// - The reverberant field is a feedback delay network: 16 delay lines (lengths
//   around the room's mean free path 4V/S), mixed by an orthogonal matrix
//   (Hadamard / 4), each line losing what the walls and the air absorb in its
//   length (Sabine RT60, shorter at high frequencies). Each player feeds it
//   through three allpass diffusers (an echo becomes a dense burst) along its
//   own random direction over the lines, and every listener (the microphones,
//   the players) takes its own random combination: the field soon comes from
//   everywhere, the same for a player on the left as on the right. Its level
//   follows diffuse-field theory: p_rev^2 / p(1 m)^2 = 16 pi / A, with the
//   absorption area A = 0.161 V / RT60.
// - Each player hears the reverberant field plus every other player's direct
//   sound (1/r, delayed by r/c). That pressure acts on its open holes,
//   embouchure and foot, playing or not: a silent flute in the pool rings in
//   sympathy.
//
// The instruments are computed in blocks (kBlock samples); what they hear in a
// block is what the room had at the end of the one before. The direct paths are
// shortened by one block for that; the room's own field arrives later than a
// block anyway.
#pragma once

#include <string>
#include <vector>

namespace pajupilli {

struct RoomSpec {
  const char* name;
  double volume;    // m^3
  double rt60;      // s, mid frequencies
  double listener;  // listener distance from the players (m)
  // A gated reverb (Phil Collins, the 80s snare and toms): the reverberant
  // field at the microphones (the ambience mics), compressed hard (its body
  // held up, not decaying), passes through a gate keyed by the dry sound:
  // each hit opens it for `gate` seconds (s; 0: none), then it shuts fast.
  double gate = 0;
};
// "dry" (no room), "studio", "hall" (default), "church"; nullptr if unknown.
const RoomSpec* findRoom(const std::string& name);

class Room {
 public:
  static constexpr int kBlock = 32;  // samples per block (0.67 ms)
  static constexpr int kLines = 16;
  // x: each source's place across the stage (m, left negative).
  Room(const RoomSpec& spec, const std::vector<double>& x, double fs = 48000);
  // One block of n <= kBlock samples. src[i]: source i's sound at 1 m (Pa).
  // atSource[i]: the room's pressure at source i (Pa) for the next block.
  // left/right: the listener (Pa; the direct sound plus the room about as loud
  // as a source alone at 1 m), added to; each source's early sound (direct and
  // reflections) times listenGain[i] (a microphone's balance).
  void process(const float* const* src, int n, float* const* atSource, float* left, float* right,
               const float* listenGain);
  int sources() const { return int(x_.size()); }
  const RoomSpec& spec() const { return spec_; }
  // Source s's first reflections off the side walls: when, and how much louder
  // in the right channel than the left.
  std::string lateral(int s) const;

 private:
  RoomSpec spec_;
  std::vector<double> x_;
  double fs_;
  // The network.
  static constexpr int kLineRing = 16384;
  std::vector<float> line_;     // [line][kLineRing]
  int len_[kLines], pos_ = 0;
  float b0_[kLines], a1_[kLines], lp_[kLines] = {};
  float inGain_, micGain_ = 1, micDiffuse_ = 1;
  // The microphones' pattern a + (1 - a) cos (0.6: between omni and cardioid,
  // 14 dB down at the back) and their angle out from the stage (55 degrees).
  static constexpr double kMicPattern = 0.6, kMicAngle = 55 * 3.14159265358979 / 180;
  std::vector<float> inDir_;    // [source][line], unit length
  std::vector<float> outDir_;   // [tap][line], unit length; taps: sources, then left, right
  // Input diffusers: three allpasses per source.
  static constexpr int kDiff = 3, kDiffRing = 1024;
  int diffLen_[kDiff];
  std::vector<float> diff_;     // [source][kDiff][kDiffRing]
  // Each source's recent sound, for the direct paths and the reflections.
  static constexpr int kRing = 16384;
  std::vector<float> ring_;     // [source][kRing]
  int rpos_ = 0;
  std::vector<int> dDelay_;     // between players: [to][from] samples
  std::vector<float> dGain_;
  // The gate (RoomSpec::gate): the input's envelope, its recent peak, the hold left, the gain.
  float gateFast_ = 0, gateSlow_ = 0, gatePeak_ = 0, gateHold_ = 0, gateGain_ = 0;
  float compEnv_ = 0, compPeak_ = 0;  // the ambience's compressor: its level, its recent peak
  struct Tap { int src, mic, delay; float gain; int u, w, z; };  // u, w, z: the image (walls hit)
  std::vector<Tap> early_;      // to the microphones: direct and reflections
};

}  // namespace pajupilli
