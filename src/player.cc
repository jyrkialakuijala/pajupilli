#include "player.h"

#include <algorithm>
#include <cmath>

#include "knob.h"

namespace pajupilli {
namespace {

// Standard Boehm fingerings, C foot (research/keywork.md section 5).
// Holes 1..16: D#tr Dtr C# C B Bb A G# G#' G F# F E D# D C#.
constexpr NoteInfo kNotes[37] = {
    {"xxxxxxxxxxxxxxxx", 1},  // 60 C4
    {"xxxxxxxxxxxxxxxo", 1},  // 61 C#4
    {"xxxxxxxxxxxxxxoo", 1},  // 62 D4
    {"xxxxxxxxxxxxxooo", 1},  // 63 D#4
    {"xxxxxxxxxxxxoooo", 1},  // 64 E4
    {"xxxxxxxxxxxooooo", 1},  // 65 F4
    {"xxxxxxxxxxooxooo", 1},  // 66 F#4
    {"xxxxxxxxxooooooo", 1},  // 67 G4
    {"xxxxxxxoxooooooo", 1},  // 68 G#4
    {"xxxxxxoxoooooooo", 1},  // 69 A4
    {"xxxxxooxoooooooo", 1},  // 70 Bb4 (thumb)
    {"xxxxoooxoooooooo", 1},  // 71 B4
    {"xxxooooxoooooooo", 1},  // 72 C5
    {"xxoooooxoooooooo", 1},  // 73 C#5
    {"xxoxxxxxxxxxxxoo", 2},  // 74 D5 (C# hole vents)
    {"xxoxxxxxxxxxxooo", 2},  // 75 D#5
    {"xxxxxxxxxxxxoooo", 2},  // 76 E5
    {"xxxxxxxxxxxooooo", 2},  // 77 F5
    {"xxxxxxxxxxooxooo", 2},  // 78 F#5
    {"xxxxxxxxxooooooo", 2},  // 79 G5
    {"xxxxxxxoxooooooo", 2},  // 80 G#5
    {"xxxxxxoxoooooooo", 2},  // 81 A5
    {"xxxxxooxoooooooo", 2},  // 82 Bb5
    {"xxxxoooxoooooooo", 2},  // 83 B5
    {"xxxooooxoooooooo", 2},  // 84 C6
    {"xxoooooxoooooooo", 2},  // 85 C#6
    {"xxoxxxxxxooooooo", 3},  // 86 D6 (G4 fingering, vented)
    {"xxxxxxxoxxxxxooo", 3},  // 87 D#6
    {"xxxxxxoxoxxxoooo", 3},  // 88 E6
    {"xxxxxoxxxxxooooo", 3},  // 89 F6
    {"xxxxooxxxxooxooo", 3},  // 90 F#6
    {"xxxoxxxxxooooooo", 3},  // 91 G6
    {"xxooxxxoxooooooo", 3},  // 92 G#6
    {"xxoxxxoxoxxooooo", 3},  // 93 A6
    {"xooxxooxoxxooxoo", 3},  // 94 Bb6
    {"oxxxooxxxooooxoo", 3},  // 95 B6
    {"xxxoxxxoxxxooxoo", 4},  // 96 C7
};

}  // namespace

const NoteInfo* noteInfo(int midi) {
  return midi >= 60 && midi <= 96 ? &kNotes[midi - 60] : nullptr;
}

double noteFrequency(int midi, double a4) { return a4 * std::pow(2.0, (midi - 69) / 12.0); }

Player::Player(Flute& f) : flute_(f), fs_(f.fs()) {
  for (int i = 0; i < 128; ++i) coverTrim[i] = pressTrim[i] = 1;
}

void Player::noteOn(int midi, int velocity) {
  if (!noteInfo(midi)) return;
  held_[midi] = 1;
  if (stackTop_ < 16) stack_[stackTop_++] = midi;
  const bool legato = note_ >= 0 && lungTarget_ > 0;
  note_ = midi;
  velAccent_ = velocity / 127.0;
  applyNote(!legato);
}

void Player::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  held_[midi] = 0;
  int w = 0;
  for (int r = 0; r < stackTop_; ++r)
    if (stack_[r] != midi) stack_[w++] = stack_[r];
  stackTop_ = w;
  if (midi != note_) return;
  if (stackTop_ > 0) {  // fall back to the last held key, slurred
    note_ = stack_[stackTop_ - 1];
    applyNote(false);
  } else {
    lungTarget_ = 0;  // stop blowing; the fingering stays
  }
}

void Player::applyNote(bool tongued) {
  const NoteInfo* ni = noteInfo(note_);
  flute_.setHoles(ni->holes);
  const double f = noteFrequency(note_);
  Embouchure e = flute_.embouchure();
  // Fletcher 1975: p ~ 0.8 f Pa, lip to edge l ~ 1.8 + 100 / sqrt(f) mm, of
  // which the first 1.8 mm is where the jet forms; the jet's convective length
  // is the rest, which makes W f / Uj ~ 0.087 for every note.
  e.jetLen = 100 / std::sqrt(f) * 1e-3 * knob("PJ_W", 1);
  // Coltman: uncovered width 6 mm at C4 .. 3.5 mm at A6.
  const double t = std::clamp(std::log(f / 261.6) / std::log(1760 / 261.6), 0.0, 1.2);
  e.uncovered = (6.0 - 2.5 * t) * 1e-3 * coverTrim[note_];
  // Louder: more breath and a bigger lip opening (Fletcher: all players open the lips).
  e.lipH = (0.5 + 0.5 * dyn_) * 1e-3;
  e.lipWidth = (7 + 4 * dyn_) * 1e-3;
  e.y0 = knob("PJ_Y0", 0.3) * 1e-3;
  e.noise = knob("PJ_BREATH", 0.01);
  lungTarget_ = 0.8 * f * (0.7 + 0.6 * dyn_) * pressTrim[note_] * knob("PJ_P", 1);
  if (tongued) {
    // "t": the tongue seals, the mouth fills, then the tongue lets go.
    e.tongue = 0;
    tongueClock_ = 0;
    tongueRelease_ = 0.010 - 0.007 * velAccent_;  // harder attack = faster release
    sinceOn_ = 0;
  }
  flute_.setEmbouchure(e);
}

void Player::render(float* out, int n) {
  constexpr int kBlock = 16;
  const double bdt = kBlock / fs_;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    Embouchure e = flute_.embouchure();
    // Lungs: fast push (5 ms), slower release (30 ms).
    const double tc = lungTarget_ > lung_ ? 0.005 : 0.030;
    lung_ += (1 - std::exp(-bdt / tc)) * (lungTarget_ - lung_);
    // Tongue: closed for 4 ms while the mouth fills, then opens.
    tongueClock_ += bdt;
    sinceOn_ += bdt;
    const double topen = 0.004;
    if (tongueClock_ < topen) e.tongue = 0;
    else e.tongue = std::min(1.0, (tongueClock_ - topen) / tongueRelease_);
    // Vibrato: 5 Hz pressure modulation, fading in after 300 ms.
    vibPhase_ += 2 * M_PI * 5.0 * bdt;
    const double vib = vibDepth_ * std::min(1.0, std::max(0.0, sinceOn_ - 0.3) / 0.4);
    // Accented attack: a short pressure overshoot.
    const double accent = 1 + 0.25 * velAccent_ * std::exp(-sinceOn_ / 0.04);
    e.lung = lung_ * accent * (1 + vib * std::sin(vibPhase_));
    flute_.setEmbouchure(e);
    flute_.process(out + i, m);
  }
}

}  // namespace pajupilli
