// The room the instruments play in: a reverberant space of its own that they
// all sound into and that sounds back into them.
//
// - The room is a shoebox (2 : 1.4 : 0.7) of the given volume. The players sit
//   on a line across the stage near the front wall (high instruments on the
//   left); the listener is a spaced pair of microphones (1 m either side of the
//   centre) further into the room.
// - Early sound: the direct sound and the reflections off the six surfaces up
//   to second order (image sources: 24 per player and microphone), each 1/r,
//   r/c late and reflected sqrt(1 - alpha) per wall (alpha = A / S). They come
//   from both sides within the first tens of milliseconds.
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

 private:
  RoomSpec spec_;
  std::vector<double> x_;
  double fs_;
  // The network.
  static constexpr int kLineRing = 16384;
  std::vector<float> line_;     // [line][kLineRing]
  int len_[kLines], pos_ = 0;
  float b0_[kLines], a1_[kLines], lp_[kLines] = {};
  float inGain_, micGain_ = 1;
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
  struct Tap { int src, mic, delay; float gain; };
  std::vector<Tap> early_;      // to the microphones: direct and reflections
};

}  // namespace pajupilli
