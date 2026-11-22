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
// "dry" (no room), "studio", "hall" (default), "concert" (a symphony hall), "church"; nullptr if unknown.
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
  // A source whose sound differs with direction (Tube::takeDipole): its three axes in the room
  // (3 x 3: x to the listener's right, y towards the listener, z up). Every path from it to a
  // microphone, direct or off the walls, leaves it in its own direction (a reflection's as
  // mirrored by its walls) and carries the three parts by how much of each axis lies along it:
  // what moves the two halves of a top against each other reaches the left wall and the right
  // with opposite signs, and nothing straight ahead. The reverberant field takes the three
  // parts' power (a third of each: its mean over all directions).
  void setAxes(int s, const float* axes);
  // The sources' three parts for the next process() ([3 * source + k], n samples each; null: none).
  void setDipoles(const float* const* dip) { dip_ = dip; }
  // A source made of parts at their own places (a vibraphone's bars; Tube::parts): np parts at
  // places (3 each, m from the source: x to the listener's right, y towards the listener, z up).
  // Each path to a microphone, direct or off the walls, then takes each part by its own length
  // (the path's direction as it leaves the source, along the part's offset: the bass bar's
  // reflection off the left wall comes a little sooner than the treble's), instead of the
  // source's own sound; the network and the other players still hear the source's whole sound.
  void setParts(int s, int np, const float* places);
  // The parts' sound for the next process() ([partFrom(s) + j], n samples each; null: none).
  void setPartSignals(const float* const* p) { part_ = p; }
  int partFrom(int s) const { return partFrom_.empty() ? 0 : partFrom_[s]; }
  int partCount() const { return partFrom_.empty() ? 0 : partFrom_.back(); }
  int sources() const { return int(x_.size()); }
  // The microphones' gain on everything the room delivers (early and late):
  // an instrument bringing its own early sound to the listener takes it too.
  float micGain() const { return micGain_; }
  const RoomSpec& spec() const { return spec_; }
  // Source s's first reflections off the side walls: when, and how much louder
  // in the right channel than the left.
  std::string lateral(int s) const;

 private:
  void block(const float* const* src, int o, int n, float* const* atSource, float* left, float* right,
             const float* listenGain);
  std::vector<int> quiet_;      // [source] samples of silence so far (up to forget_)
  int forget_ = 0;              // silence after which a player's rings and diffusers hold nothing
  std::vector<int> tapFrom_;    // [source] its first path in early_ (and one past the last source's)
  std::vector<float> din_;      // [source][kBlock] the block through the diffusers
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
  static constexpr int kRing = 16384, kSpan = kRing + kBlock;  // (its first block again past its end)
  std::vector<float> ring_;     // [source][kSpan]
  int rpos_ = 0;
  std::vector<int> dDelay_;     // between players: [to][from] samples
  std::vector<float> dGain_;
  // The gate (RoomSpec::gate): the input's envelope, its recent peak, the hold left, the gain.
  float gateFast_ = 0, gateSlow_ = 0, gatePeak_ = 0, gateHold_ = 0, gateGain_ = 0;
  float compEnv_ = 0, compPeak_ = 0;  // the ambience's compressor: its level, its recent peak
  struct Tap {
    int src, mic, delay;
    float gain;
    int u, w, z;              // the image (walls hit)
    float n[3] = {0, 0, 0};   // the path's direction as it leaves the source
    float dg[3] = {0, 0, 0};  // the source's three directional parts' gains on it
  };
  const float* const* dip_ = nullptr;
  // Parts (setParts): [source] the first part's index (and one past the last source's); each
  // part's ring and how long since it last sounded; for each of a source's taps (in early_'s
  // order), each part's delay and gain.
  const float* const* part_ = nullptr;
  std::vector<int> partFrom_;
  std::vector<float> pring_;      // [part][kSpan]
  std::vector<int> pquiet_;       // [part]
  std::vector<int> pDelay_;       // [part][tap of its source]
  std::vector<float> pGain_;
  std::vector<int> pTap_;         // [source] its parts' first entry in pDelay_ / pGain_
  std::vector<char> hasDip_;    // [source]
  std::vector<float> dring_;    // [3 * source + k][kSpan]
  std::vector<float> inDirDip_; // [3 * source + k][line], unit length
  std::vector<Tap> early_;      // to the microphones: direct and reflections
};

}  // namespace pajupilli
