// The six-operator FM synth's factory voice E.PIANO 1 (ROM1A voice 11), General MIDI 6 "Electric Piano 2"
// (-inst electric-piano-2; research/electric_pianos.md section 6).
//
// The voice is played as the FM synth plays it: six sine operators, algorithm 5 (three carrier <-
// modulator pairs: 1 <- 2, 3 <- 4, 5 <- 6, operator 6 feeding back on itself at level 6), each
// with its four-rate four-level envelope, output level, key velocity sensitivity, rate scaling and
// (operator 6) level scaling over the keyboard, its ratio and detune; oscillator key sync off (the
// operators' phases run free). The arithmetic is that of the reverse-engineered FM synth engine (Raph
// Levien's Music Synthesizer for Android, as in Dexed): the envelope's integer steps every 64
// samples, its attack jump and exponential rise, the output-level and velocity tables, the
// level-scaling curves, the detune measured on an FM synth, the gains interpolated over each block. The
// voice's 128 bytes are the factory bank's, unpacked here.
//
// Unlike an FM synth, each key is played like a piano's unison: in the middle and the treble three
// whole FM synth voices (generators), 1/7 Hz apart, each with its own slightly different output levels
// and envelope speeds; in the low range two; in the bass one (that one exactly the FM synth's).
#pragma once

#include <cstdint>
#include <vector>

#include "performer.h"
#include "tube.h"

namespace pajupilli {

bool fmPiano(const Instrument& inst);  // -inst electric-piano-2

class FmSynthModel : public Tube {
 public:
  explicit FmSynthModel(const Instrument& inst);
  void keyOn(int midi, int velocity);
  void keyOff(int midi);
  void setBend(double semitones);
  bool asleep() const { return notes_.empty(); }
  double testResponse() const override { return last_; }
  void resetAir() override;

  // The FM synth envelope generator (MSFA's Env): levels in 1/2^24 octave, a step every block.
  struct Env {
    int rates[4], levels[4];
    int outlevel = 0, rateScaling = 0;
    int32_t level = 0, target = 0, inc = 0;
    int ix = 0;
    bool rising = false, down = true;
    double incScale = 1;  // (the sample rate's, and a generator's own speed)
    void init(const int r[4], const int l[4], int ol, int rs, double scale);
    void advance(int i);
    int32_t step();
    void keyDown(bool d);
  };
  struct Op {
    Env env;
    uint32_t phase = 0;
    int32_t freq = 0;     // phase step per sample (2^24 a cycle)
    int32_t gain = 0;     // the gain reached at the end of the last block (2^24 = 1)
    int32_t logfreq = 0;  // without the bend
  };
  struct Gen {
    Op op[6];             // op[0] = operator 6 .. op[5] = operator 1, as the FM synth stores them
    int32_t fb[2] = {0, 0};
  };
  struct Note {
    int midi = 0;
    bool down = true;
    std::vector<Gen> gens;
    double norm = 1;
    int quiet = 0;
  };

 private:
  void controlStep() override {}
  float sample() override;
  void block();
  void retune(Note& n);
  std::vector<Note> notes_;
  int pos_ = 64;         // sample within the current block
  double bend_ = 0;      // semitones
  double gains_[64] = {};
  unsigned rng_ = 1;
  float last_ = 0;
  float buf_[64] = {};
};

// Velocity is the FM synth's (its key velocity sensitivity per operator); the sustain pedal holds the
// keys; the pitch bend is +-2 semitones.
class FmSynthPlayer : public Performer {
 public:
  explicit FmSynthPlayer(FmSynthModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}

 private:
  FmSynthModel& m_;
  bool keyDown_[128] = {}, sounding_[128] = {};
};

}  // namespace pajupilli
