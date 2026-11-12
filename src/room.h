// The room the instruments play in: a reverberant space of its own that they
// all sound into and that sounds back into them.
//
// - The reverberant field is a feedback delay network: 16 delay lines (lengths
//   around the room's mean free path 4V/S), mixed by an orthogonal 16x16
//   Hadamard matrix, each line losing what the walls and the air absorb in its
//   length (Sabine RT60, shorter at high frequencies). Its level follows the
//   diffuse-field theory: p_rev^2 / p_direct(1 m)^2 = 16 pi / A, with the
//   absorption area A = 0.161 V / RT60.
// - The instruments sit on an arc (high to low, left to right). Each hears the
//   room's field (its own taps of the network: decorrelated) plus every other
//   instrument's direct sound (1/r, delayed by r/c). That pressure acts on its
//   open holes, embouchure and foot, playing or not: a silent flute in the pool
//   rings in sympathy.
// - The listener is a spaced pair of microphones in front (stereo).
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
  // left/right: the listener (Pa, scaled by the listener distance so a source
  // there sounds as at 1 m), added to; each source's direct sound to the
  // listener times listenGain[i] (a microphone's balance).
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
  float inGain_;
  std::vector<float> inSign_;   // [source][line]
  std::vector<float> outSign_;  // [tap][line], taps: sources, then left, right
  // Direct sound: each source's recent output.
  static constexpr int kRing = 4096;
  std::vector<float> ring_;     // [source][kRing]
  int rpos_ = 0;
  std::vector<int> dDelay_;     // [to][from] samples (to: sources, then left, right)
  std::vector<float> dGain_;
};

}  // namespace pajupilli
