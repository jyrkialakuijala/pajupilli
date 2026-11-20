// CHOIR FRAMEWORK READY
//
// The choir (General MIDI 53 "Choir Aahs", -inst choir; research/choir.md): a choir of singers,
// sung, not a sample or a pad.
//
// Six voice parts, each a row of singers (src/singer.h: a vocal tract as puhuja's on a glottis),
// standing across the stage, the sopranos on the left, the basses on the right:
//
//   part           range (MIDI)    at ease      tract      rows
//   bass           G1-E4 (31-64)   E2-A3        17.8 cm    male   (G1 .. D2: two in four, the oktavists)
//   baritone       A2-A4 (45-69)   A2-D4        17.3 cm    male
//   tenor          C3-C5 (48-72)   D3-G4        16.8 cm    male
//   alto           F3-F5 (53-77)   G3-C5        15.5 cm    female
//   mezzo-soprano  A3-A5 (57-81)   B3-E5        15.0 cm    female
//   soprano        C4-C6 (60-84)   E4-A5        14.5 cm    female
//
// A key is its sounding note (C2-C7; below E2 the basses sing it an octave up, above C6 the sopranos
// an octave down). How the keys are sung (Ensemble::choirAssign, src/choir.cc):
//   - One key alone: a unison of the parts at ease on it (C3: basses and baritones; C4: baritones,
//     tenors, altos, mezzos; A4: altos, mezzos, sopranos); if none is, those whose range has it.
//   - A chord: spread over the parts by pitch, as a choir sings it: the basses on its lowest key,
//     the sopranos on its highest, the middle parts on the inner keys, each part on the key nearest
//     its own place in the choir among those it can sing (at ease preferred). A key no part chose
//     is taken by the part nearest it, which divides (divisi, at most three keys a part).
//   - Within a part the singers share its keys evenly; when the keys change, as few move as will
//     do, and a singer who moves slurs (its pitch glides, the vowel goes on). Each singer has its
//     own top and bottom: near the edge of a part's range only some reach (of four sopranos one
//     sings C6): the part thins out at its top.
//   - No two start together: each singer comes in ~25 ms after the key give or take 20 (0-80 ms),
//     with a habit of its own (+-10 ms) and the part's own moment (+-8 ms); they stop by breath,
//     1.5 x as scattered.
//
// No two singers are alike (SingerPlayer::vary, fixed for the seat): its tract's length (+-4 %),
// its own vowel (the tongue hump's place and the areas: R1 +-8 %, R2 +-6 %), a pitch a few cents
// off (sd 6 c), its vibrato's rate (5.3 +- 0.35 Hz), extent (+-35 c +- 12, x the part's: basses
// 0.7 .. sopranos 1.2) and when it begins (0.3 +- 0.1 s), its level (sd 2.5 dB; one in six a
// strong voice, +4 dB at f), its breath. Per note: a fresh offset (sd 4 c), a start a little flat
// gliding in (60 ms), the vowel a few % off; in long notes each singer breathes on its own clock
// (every 8-15 s for half a second or so: the section never stops). The "chorus" is that, heard in
// the room: nothing is chorused.
//
// The singers (Singer::setVoicing): the voice part sets the tract's length; the sung vowel rows
// (src/singer_vowels_data.h, refitted on the Singer's own line toward Henrich, Smith & Wolfe 2011:
// the [ɑ] at R1/R2 ~730/1200 Hz for men at 17.5 cm, ~820/1280 for women at 15 cm); the lungs from
// ~0.4 kPa (pp) to ~3.5 kPa (ff), rising 1.4 x an octave above the part's middle (to ~5 kPa);
// resonance tuning: the women open the jaw to keep R1 just above f0 once f0 nears it (the soprano's
// R1:f0 above ~C5), the men nudge R1 onto 2 f0 when it is near.
//
// ---- The variant interface (another choir instrument: "voice-oohs" and the like) ----
// A variant is a row of kChoirVariants (src/choir.cc), found by the instrument's name, plus an
// Instrument of family "choir" in kChoirInstruments (same file; registered in src/instruments.cc and
// voiced by Voice). Everything else (the parts, the singers, the dealing of keys, the room) is
// shared. A variant sets (ChoirVariant, below):
//   - vowel[part]: the sung vowel per part, on the sung rows' axis (SungVowel in
//     src/singer_vowels_data.h: 0 kSungU u, 1 kSungUh ʊ, 2 kSungO ɔ, 3 kSungA ɑ; fractions blend the
//     neighbours): the aahs 3 for every part; an ooh e.g. 0 or 0.5;
//   - highVowel: the women's vowel at the top of their range, opened toward it from ~D5 to G5
//     (an ooh opens toward ɔ: 2), < 0 none;
//   - attack, release: the lungs' rise and fall (time constants, s, at mf; soft keys rise slower);
//     softness 0 .. 1: a gentler voice (the pressure range lower, a breathier onset);
//   - breath, vibrato, spread: x the parts' breath (the chink), x the vibrato's extent, x the
//     onset scatter;
//   - parts: a mask of the parts that sing (1 << ChoirPart index); menBelow: the men sing only keys
//     below this (128: everywhere; an "oohs" led by the women: 60);
//   - consonant 0 .. 1: a soft "d" at each fresh onset (the tongue's closure released, harder keys
//     more), as Roland's "dooh"; hum 0 .. 1: the lips closed into a hum (src/hum.h);
//   - gainDb: its level against the aahs (the levels table trims each instrument after).
// The knobs (SingerPlayer::setKnob; live's K2 and K3, -seq's "knob 2 X", "knob 3 X") are the same
// for every variant: K2 the vowel's colour from dark to bright (u ʊ ɔ ɑ e i), K3 the openness
// (closed and breathy .. the jaw open, clear); until turned, the variant's vowel and openness.
#pragma once

#include <memory>

#include "hum.h"
#include "performer.h"
#include "singer.h"
#include "tube.h"

namespace pajupilli {

class Ensemble;

// A voice part (research/choir.md 4.1, 9.3).
struct ChoirPart {
  const char* name;
  int lo, hi;          // its range (MIDI)
  int easyLo, easyHi;  // at ease
  int maxDiv;          // at most this many keys at once (divisi)
  double lengthCm;     // the tract's mean length
  bool female;         // the female vowel rows; R1:f0 tuning (else R1:2f0)
  double breath;       // the chink (Singer's breath 0..1)
  double vibScale;     // x the vibrato's extent
};
enum { kChoirBass, kChoirBaritone, kChoirTenor, kChoirAlto, kChoirMezzo, kChoirSoprano, kChoirParts };
extern const ChoirPart kChoirPart[kChoirParts];

struct ChoirVariant {
  const char* name;           // the instrument
  double vowel[kChoirParts];  // per part, bass .. soprano (the sung axis: 0 u, 1 ʊ, 2 ɔ, 3 ɑ)
  double highVowel;           // the women's top opens toward this (< 0: none)
  double attack, release;     // the lungs' rise and fall (s, at mf)
  double softness;            // 0 .. 1
  double breath, vibrato, spread;  // x
  unsigned parts;             // which parts sing (bits by ChoirPart index)
  int menBelow;               // the men only on keys below this
  double consonant, hum;      // 0 .. 1
  double gainDb;
};
// The variant of a choir instrument (by name; the first, "choir", if none matches).
const ChoirVariant& choirVariant(const Instrument& inst);
bool choirInstrument(const Instrument& inst);  // family "choir": the whole choir (-inst choir)
bool choirSinger(const Instrument& inst);      // one singer of it (a part's seat)
extern const Instrument kChoirInstruments[];
extern const int kNumChoirInstruments;

// One singer as an instrument: its Singer, the controls its player sets each block.
class SingerModel : public Tube {
 public:
  explicit SingerModel(const Instrument& inst);
  Singer& singer() { return g_; }
  int part() const { return part_; }
  struct Controls { double f0 = 220, vowel = 3, lung = 0, breath = 0.2, vibDepth = 0, vibRate = 5, gain = 1, hum = 0; };
  void setControls(const Controls& c) { c_ = c; }
  // Silent long enough, it is not computed (asleep); a note wakes it, the air at rest.
  void wake();
  bool asleep() const { return asleep_; }
  int seat = 0;  // (its seat in the choir: PJ_CHOIR_ONLY=seat sounds only this singer, for measuring)
  double testResponse() const override { return last_; }
  void resetAir() override;

 private:
  void controlStep() override;
  float sample() override;
  Singer g_;
  Hum hum_;
  Controls c_;
  int part_ = 0, quiet_ = 0;
  bool asleep_ = true;
  float last_ = 0;
};

// A singer of the choir: the key into its lungs, pitch, vowel and jaw.
class SingerPlayer : public Performer {
 public:
  explicit SingerPlayer(SingerModel& m);
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override { hold_ = h; }
  void seed(unsigned s) override { rng_ = 9157u + 7919u * s; }
  void vary(unsigned seat) override;
  void setVariant(const ChoirVariant& v) { var_ = &v; }
  // The knobs (live's K1..K8 as k 0..7, -seq's "knob K X"): K2 the vowel's colour, dark to bright
  // (u ʊ ɔ ɑ e i over the knob: ɑ at 0.6); K3 the voice's openness (0 closed and breathy, 0.5 as
  // the variant sings, 1 the jaw open and the tone clear). Untouched, the variant's own vowel.
  bool setKnob(int k, double x) override;

 private:
  double rnd() { rng_ = rng_ * 1664525u + 1013904223u; return (rng_ >> 8) / 16777216.0; }
  double gauss() { return (rnd() + rnd() + rnd() + rnd() - 2) * 1.73; }
  SingerModel& m_;
  const ChoirVariant* var_;
  const ChoirPart& part_;
  uint32_t rng_ = 9157u;
  // The seat's own.
  double vibRate_ = 5.3, vibExt_ = 35, vibDelay_ = 0.3, levelDb_ = 0, breath_ = 0.2, vowelSeat_ = 0;
  // The knobs: the vowel (-1: the variant's), the openness; both glided (~40 ms).
  double knobVowel_ = -1, knobOpen_ = 0.5, vowelNow_ = -1, openNow_ = 0.5;
  bool strong_ = false;
  // The note.
  int cur_ = -1, vel_ = 64;
  bool releasing_ = false, breathing_ = false;
  double t_ = 0, noteNow_ = 60, noteCents_ = 0, startOff_ = 0, jawJit_ = 0, lung_ = 0, onsetAir_ = 0, tuneNow_ = 0;
  double nextBreath_ = 10, breathEnd_ = 0, dClock_ = 1e9;
};

// The choir as one voice (-inst choir -note, -seq: a bare Voice): the whole choir inside, dry.
// (An Ensemble of a choir instrument, as -play and live make, deals the singers itself, in its
// room: Ensemble::initChoir.)
class ChoirModel : public Tube {
 public:
  explicit ChoirModel(const Instrument& inst);
  ~ChoirModel() override;
  Ensemble& ensemble() { return *ens_; }
  double testResponse() const override { return 0; }

 private:
  void controlStep() override {}
  float sample() override { return 0; }
  std::unique_ptr<Ensemble> ens_;
};

class ChoirPlayer : public Performer {
 public:
  explicit ChoirPlayer(ChoirModel& m) : Performer(m), m_(m) {}
  void noteOn(int midi, int velocity) override;
  void noteOff(int midi) override;
  void render(float* out, int n) override;
  void setHold(bool h) override;
  void seed(unsigned) override {}
  bool setKnob(int k, double x) override;  // (the singers': SingerPlayer::setKnob)

 private:
  ChoirModel& m_;
};

}  // namespace pajupilli
