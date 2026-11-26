// The lead singer (General MIDI "Lead 6 (voice)"): a jazz singer who scats the part.
//
// The voice is puhuja's (src/puhuja: the speech synthesizer's string folds on its transmission-line
// tract), sung instead of spoken: each note a syllable, its consonant before the beat, its vowel
// on the note's pitch for the note's length. A speech synthesizer needs to know a sound's length
// before it begins it, so the singer reads the part ahead: it is given the whole part (-play) and
// sings it phrase by phrase, each phrase rendered a little before it is heard. Played live, with
// no part to read, there is no such singer (the choir's lead singers sing instead: src/choir.h).
//
// What it sings (the syllables) follows what the author asked for (2026-11-25):
//   the consonant is the articulation, the vowel the colour;
//   fast and swung lines: plosives (d, b, t, p);  triplets and rubato: liquids and glides (l, w, y:
//   "doo-dl-ah");  ghost notes: a nasal ("dn");  joy: an open "ah";  intimacy: "oo", "oh" on soft
//   consonants;  urgency: k, g and diphthongs;  a ballad: mostly vowels;  bebop: sharp consonants on
//   bright vowels;  the figures "doo-bah", "doo-wee", "dah-ba", "doo-dl-ah";
//   other styles: Baroque "la", "ti", "di" on pure a and e;  Classical: one pure vowel;  Romantic:
//   open a, o with liquids;  folk: "diddle-um".
//
// Its pitch: the folds are a physical model, their pitch not a number to set. Two controls hold
// it, as the author described a singer's: what the singer knows (the tension a pitch takes, learnt
// once by singing a scale: puhuja's setTensionMap) and what it hears (puhuja's pitch controller,
// tightened: setPitchControl). The voice holds its pitch from about E2 to C4 (measured); the part
// is taken down by octaves into that range. The larynx stays low and still (setLarynx).
#ifndef PAJUPILLI_JAZZSINGER_H_
#define PAJUPILLI_JAZZSINGER_H_

#include <string>
#include <vector>

namespace pajupilli {

struct SungNote {
  double on = 0, off = 0;  // s from the start of the rendering
  int key = 60, vel = 80;
  float accent = 0;        // the director's emphasis, -1 .. 1 (0: none given)
  bool phraseEnd = false;  // the director's phrase end
};

class JazzSinger {
 public:
  enum Style { kJazz, kBallad, kBebop, kBaroque, kClassical, kRomantic, kFolk, kAuto };
  // The part, and when the other parts' notes begin (how busy the accompaniment is around each
  // note). style kAuto: jazz, a ballad or bebop by how fast the part moves (PJ_SINGER_STYLE:
  // jazz, ballad, bebop, baroque, classical, romantic, folk).
  void setScore(std::vector<SungNote> notes, std::vector<double> others, Style style = kAuto);
  bool hasScore() const { return !phrase_.empty(); }
  // The next n samples (48 kHz, mono), in order from the start of the rendering.
  void render(float* out, int n);
  // What it sings, a phrase a line: "doo bah doo wee | ..." (for the log and the tests).
  std::string words() const;
  int transposed() const { return shift_; }  // semitones the part was moved by (octaves)

  // One note's syllable. cons: up to two consonants before the beat ('d', 'b', 't', 'p', 'k', 'g',
  // 'l', 'w', 'j', 'n', 'm', 's'; 0: none); nucleus: what is sung on the note ('a' ah, 'u' oo,
  // 'o' oh, 'i' ee, 'e' eh, '@' the neutral vowel, 'E' the diphthong ei, 'l' and 'n': a sung
  // consonant, "dl", "dn"); coda: a consonant closing it ('m' of "um"; 0: none).
  struct Syllable { char cons[3] = {0, 0, 0}; char nucleus = 'a'; char coda = 0; };

 private:
  struct Phrase {
    int first = 0, count = 0;  // its notes
    double start = 0;          // where its sound begins in the rendering (s), once rendered
    double lead = 0;           // ... and how long before its first beat that is (the breath, the consonant)
    std::vector<float> pcm;
    bool done = false;
  };
  void plan(Style style);
  void sing(Phrase& p);
  std::vector<float> take(const Phrase& p, const std::vector<double>& vowelMs, const std::vector<double>& restMs,
                          std::vector<double>* vowelAt, double* lead) const;
  std::vector<SungNote> note_;
  std::vector<Syllable> syl_;
  std::vector<double> others_;
  std::vector<Phrase> phrase_;
  int shift_ = 0;
  long at_ = 0;  // samples rendered
  size_t next_ = 0;  // the first phrase not yet sung
  std::vector<float> ahead_;  // the sung sound: from at_ on at ahead_[head_]
  size_t head_ = 0;
  double lastEnd_ = -1e9;  // when the last phrase's last note ended (s)
};

}  // namespace pajupilli

#endif  // PAJUPILLI_JAZZSINGER_H_
