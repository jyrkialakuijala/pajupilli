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
// tightened: setPitchControl). The larynx stays low and still (setLarynx).
//
// Its voice: one of nine, basso profondo to coloratura soprano, chosen by the part's range (or asked for:
// -reinstrument flute=voice-lead:soprano, PJ_SINGER_VOICE). They differ as singers do: the folds'
// mass (a soprano's about half a baritone's), the pitch above which less and less of the fold
// swings (the passaggio; puhuja's setFoldMass, the mass following the pitch within 3 ms), and the
// tract's length. With the speaker's heavy folds alone the voice held its pitch only to about D4.
// A part outside every voice is moved by octaves (-reinstrument flute=voice-lead:-12 asks for a
// transposition; :soprano:-12 for both).
//
// Its tuning: by the spectra ("jyrki-spectral", as the organ's and the winds' by ear: src/band.h's
// bandPull), each note placed where its partials meet those of the other parts' notes sounding
// then, at most PJ_SINGER_TUNE_LIM cents (15) from equal temperament. The singer reads the others'
// notes ahead as it reads its own. PJ_SINGER_TUNING=tet12: equal temperament. The scale it learns
// its folds' tension on is sung off the tempered pitches too (a third of its notes 15 cents high,
// a third as much low), as the tuning will ask.
//
// Its drama, hairpins and vibrato are the director's (the note's cue): the level through each
// note is the breath's pressure (PJ_SINGER_DRAMA_DB from pp to ff, 14; a swell on top), the
// vibrato the director's share of the singer's own (PJ_SINGER_VIB cents either way, 30, at
// PJ_SINGER_VIB_HZ, 5.4), beginning when the director says. Without a director the singer's own
// habit: the long notes' vibrato coming in late, no hairpins.
//
// Weighted and unweighted syllables (the author, 2026-11-26): a syllable is sung with weight (the
// breath pressed for it before it begins) or without (no new push: it rides on the breath of the
// one before, easing). The syllables fall into words (a scat figure is one: "doo-bah",
// "doo-dl-ah"), and each word has one weighted syllable:
//   - a word's last syllable is most often unweighted, a phrase's last one above all;
//   - a syllable on the bar's first beat is often weighted;
//   - the rest is the language's. Latin (the Baroque, Classical and Romantic styles): the syllable
//     before the last if it is long, else the one before that; a word of two, the first. English
//     (jazz, a ballad, bebop, folk): where speech has the stress, the content words' main
//     syllables, set on the strong beats and the melody's peaks; the others light and reduced, not
//     lengthened and not punched even on a longer note.
// The singer scats: its "content" is the syllables with a full vowel, its reduced ones the "ba",
// "dl", "dn", "um". (English words' own rules, for a singer of lyrics: two-syllable nouns and
// adjectives on the first, verbs and prepositions on the second; -tion, -sion, -ic: the syllable
// before; -cy, -ty, -phy, -gy, -al: two before the end; a compound noun on its first part, a
// compound verb on its second. puhuja's English reader has the dictionary's stresses.)
// In the mix (the author, 2026-11-26: a jazz lead vocal's natural intimacy, standing in the band's
// room; the practice of the mixing desk taken as a hint, done with the room we have):
//   - at the centre of the stage, in the hall every part shares (the choir's middle seat); two
//     lead singers a tenth of the stage either side;
//   - close: a microphone near the voice blended in, its direct and early sound PJ_SINGER_NEAR_DB
//     (6 dB) over what the hall's main microphones have of it, the reverberation it raises as it
//     was. The voice stays in front and the room blooms behind it, which a desk does with a
//     reverb's pre-delay;
//   - held level gently, as a hand on the fader would: 2 : 1 above PJ_SINGER_COMP_DB (-18 dB),
//     slowly (PJ_SINGER_COMP ratio; 1: none);
//   - (off: a reverberation plate of its own, a steel sheet's modes from 200 Hz to 6 kHz fed 50 ms
//     late. The hall does this better; PJ_SINGER_PLATE_DB -16 turns it on, PJ_SINGER_PLATE_T60,
//     PJ_SINGER_PLATE_PREDELAY.)
// Practised (./pajupilli -singer-practice, src/singer_data.h; PJ_SINGER_PRACTISED=0: not used):
// each note's trim by its vowel, and the way from note to note by the interval, slurred and direct.
// Stress in scat (the author, 2026-11-26) comes less from the lungs than from the larynx and the
// mouth, the three working apart. So a weighted syllable in the jazz styles also has: the folds
// pressed a little together (brighter, a longer closed phase: puhuja's Expr::adduct 0.85), the
// mouth a little more open, its stop consonant held longer and let go harder (the pressure
// built behind the closure, not in the chest), and only a brief, small pulse of breath at its
// beginning (1.5 dB over 60 ms; PJ_SINGER_PULSE_DB). An unweighted one has the folds let apart
// (1.25, a ghost note 1.4: breathy, darker). An accented short note before a rest is a "bop" or
// a "dat": closed by a stop.
// The play: the level and the colour swing through the song as two lightly damped springs (a
// differential equation, integrated over the notes), pushed at every phrase and harder when the
// phrase has been sung before, so a repeated phrase comes mf, then p, then f, in another colour,
// its accents elsewhere (src/jazzsinger.cc plan(); PJ_SINGER_PLAY).
// The syllables, after the great scat singers: d and b before "oo" and "ee" above all ("doo",
// "dee", "dih", "boo", "bee"), "dn" and "dm" for ghost notes, "dl" to connect; in bebop mostly d,
// with an l or a flipped r now and then; playing: "bibby bibby doo dap", "zoyt", "shah".
// A lexicon of the scat singers' words is drawn from where a word fits the notes (its stops on
// short notes, its sung consonants on quick ones, "dwee" and "oop" on a rise, "dow" on a fall), the
// common ones always, the rare ones as the play rises; a phrase sung again draws anew. Quick and
// unweighted syllables' vowels go towards the neutral vowel (puhuja's Expr::neutral). In swing the
// weighted syllables lie 18 ms behind the beat (PJ_SINGER_LAYBACK_MS).
// The shortest notes (under 0.2 s, PJ_SINGER_PLAIN_UNDER) are sung plainly, "doo-bah-dah" and its
// turns, most of them with the mouth left open (no closing consonant, the air let out), and carry
// no stress of their own: by the author's ear they were nervous otherwise.
// A classical singer keeps the jaw open throughout (PJ_SINGER_CLASSICAL_OPEN 0.3).
// Two schools of the one voice: the classical styles sing with the larynx low and a vibrato all
// through the note (5.8 Hz); the jazz styles, after the African traditions jazz singing comes
// from, with the larynx at speech level (PJ_SINGER_JAZZ_LARYNX -0.5 cm: brighter, forward) and a
// straight tone whose vibrato is a late ornament.
// No long note stands still (the author, 2026-11-26: "something should be changing -- the vowel
// color, the opening of the mouth, the lung pressure, air flow"): a note of 0.6 s or more develops,
// the breath, the mouth's opening, the folds' closure and the vowel's clarity moving together, the
// way the play is going there: growing if the level is on its way up, receding if on its way
// down, else out and back (a swell), rarely in and out again. PJ_SINGER_DEVELOP (1): how much.
// And rarely, where a scat note would be too long (1.3 s or more, about one in four, more when the
// play is up), it begins with a turn on its upper neighbour: up, up, the note, up, and home
// ("--_-____", 85 ms a step; PJ_SINGER_TURN 0: never).
// A man scatting holds a long note straight and lets the vibrato come about half-way through it
// (four times in five; from the note's beginning 15 times in a hundred; none but a trace at the
// very end 5 times), deeper over rich harmony (x 0.75 .. 1.35) and when little else plays (the
// soloist heard alone: x 1.3 down to 0.7 under a busy band). The director's depth still counts.
// PJ_SINGER_WEIGHT_DB (3): a weighted syllable's breath over an unweighted one's (0, with
// PJ_SINGER_REDUCE=0 and PJ_SINGER_STRESS_SCOOP=0: all syllables alike). By the author's ear,
// 2026-11-26: weighted is better.
#ifndef PAJUPILLI_JAZZSINGER_H_
#define PAJUPILLI_JAZZSINGER_H_

#include <memory>
#include <string>
#include <vector>

struct SingerTrim;  // (a practised note: src/singer_data.h)

namespace pajupilli {

struct SungNote {
  double on = 0, off = 0;  // s from the start of the rendering
  int key = 60, vel = 80;
  float accent = 0;        // the director's emphasis, -1 .. 1 (0: none given)
  bool phraseEnd = false;  // the director's phrase end
  float cents = 0;         // its tuning against equal temperament (the singer's own: tune())
  // The director's reading of the note (src/director.h's cue; cued: one was given): the level at
  // its start and its end (0 pp .. 1 ff, 0.5 as written: the drama and the hairpins), a long
  // note's swell and where it peaks, the vibrato (x the singer's own depth and rate, and when in
  // the note it begins, s).
  // Where it falls in the bar: 1 the bar's first beat, 0.75 its middle in an even metre, 0.5 another
  // beat, 0.2 off the beat; < 0: not known.
  float beat = -1;
  // ... in quarter notes after the bar's first beat (< 0: not known), and a quarter note's length there (s).
  float inBar = -1, beatLen = 0;
  bool cued = false;
  float level = 0.5f, levelEnd = 0.5f, swell = 0, swellPeak = 0.5f;
  float vibDepth = 1, vibDelay = 0.25f, vibRate = 1;
  // How rich the harmony is under the note (0 a unison or an octave .. 1 five pitch classes or
  // more in the other parts), and how much the others play around it (their notes a second).
  float harm = 0.3f, busy = 4;
};

// A note of another part, as the singer hears it (its tuning: below).
struct HeardNote {
  double on = 0, off = 0;  // s from the start of the rendering
  int ch = 0, pitch = 60, vel = 80;
  int fam = 0;             // BandFamily (src/band.h)
};

class JazzSinger {
 public:
  enum Style { kJazz, kBallad, kBebop, kBaroque, kClassical, kRomantic, kFolk, kAuto };
  // The part, and when the other parts' notes begin (how busy the accompaniment is around each
  // note). style kAuto: jazz, a ballad or bebop by how fast the part moves (PJ_SINGER_STYLE:
  // jazz, ballad, bebop, baroque, classical, romantic, folk).
  // heard: the other parts' notes (for the tuning; none: equal temperament).
  void setScore(std::vector<SungNote> notes, std::vector<double> others, std::vector<HeardNote> heard = {}, Style style = kAuto);
  // The cents each note is sung at against equal temperament, in the part's order (for the tests).
  std::vector<float> tuning() const;
  bool hasScore() const { return !phrase_.empty(); }
  // The next n samples (48 kHz, mono), in order from the start of the rendering.
  void render(float* out, int n);
  // The plate's return for the n samples last rendered, added to the part's two channels.
  // side: -1 .. 1, where the singer stands (a duet's two a little apart).
  void halo(float* l, float* r, int n, float side = 0);
  // What it sings, a phrase a line: "doo bah doo wee | ..." (for the log and the tests).
  std::string words() const;
  // ... as words, the weighted syllables in capitals: "DOO-bah DAH doo-DL-ah".
  std::string weighted() const;
  int transposed() const { return shift_; }  // semitones the part was moved by (octaves)
  // The part moved by this many semitones instead of the singer's own choice (before setScore).
  // The notes are then sung where they fall, also outside the range the voice holds steadily.
  void setShift(int semitones) { asked_ = true; shift_ = semitones; }
  bool shiftAsked() const { return asked_; }
  // The voice: profondo (basso profondo), bass, baritone, tenor, countertenor, contralto (alto), mezzo, soprano, coloratura
  // (before setScore; not set: the one whose range holds the part best). false: no such voice.
  bool setVoice(const std::string& name);
  const char* voiceName() const;
  std::string range() const;  // "C4-C6"
  int outside() const { return outside_; }  // notes outside the voice's range as sung

  // How the voice goes from one note to the next, as practised (-singer-practice): when the pitch
  // leaves the first note (ms before its end) and when it is on the second (ms after its
  // beginning), how far it aims past the second at first (cents, in the leap's direction) and for
  // how long (ms), and the breath eased or pressed at the arrival (dB, fading over 80 ms).
  struct Way { float leaveMs = 35, arriveMs = 35, over = 0, overMs = 60, dipDb = 0; };
  // The practice (./pajupilli -singer-practice): as the winds practise their slurs and starts
  // (tools/slurs.py), each voice sings every note of its range on every vowel until the vowels
  // agree: the pitch trimmed onto the note, the breath set so that every vowel is as loud on a
  // pitch as the others (a loud vowel on less breath: the least effort), a vowel opened where the
  // pitch climbs past its first resonance. Then every interval, slurred (vowel to vowel) and
  // direct (a consonant between), on one vowel and on a changing one, trying ways of going until
  // the new note is there soonest and cleanest.
  // Writes src/singer_data.h (jobs processes; 0: one a core). Returns 0 if it went well.
  static int practise(const char* outPath, int jobs);

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
    double breath = 0;         // ... and how much of its beginning is the breath in (s)
    std::vector<float> pcm;
    bool done = false;
  };
  void plan(Style style);
  void tune(std::vector<HeardNote> heard);
  void sing(Phrase& p);
  std::vector<float> take(const Phrase& p, const std::vector<double>& vowelMs, const std::vector<double>& restMs,
                          const std::vector<double>& sungAt, std::vector<double>* vowelAt, double* lead) const;
  std::vector<SungNote> note_;
  std::vector<Syllable> syl_;
  void weigh(Style style);
  // (the practice's trials: a way for every change of note, a trim for every note, or nothing practised used at all)
  const Way* tryWay_ = nullptr;
  const ::SingerTrim* tryNote_ = nullptr;
  bool tryNoteSet_ = false, plain_ = false, noLevel_ = false;
  std::vector<float> weight_;    // each syllable's weight, 0 (unweighted) .. 1
  std::vector<char> wordStart_;  // it begins a word
  std::vector<double> others_;
  std::vector<Phrase> phrase_;
  int voice_ = -1;  // (index into the voices; -1: not chosen yet)
  Style style_ = kJazz;  // (as planned)
  std::vector<float> play_, bright_, mischief_;  // the play at each note: level, colour, mischief (plan())
  std::vector<char> flip_;                       // ... its phrase's accents moved off the beat
  std::vector<char> again_;                      // ... how many times its phrase was sung before
  std::vector<float> playV_, brightV_;           // ... which way the level and the colour are moving there
  int shift_ = 0, outside_ = 0;
  bool asked_ = false;
  long at_ = 0;  // samples rendered
  size_t next_ = 0;  // the first phrase not yet sung
  std::vector<float> ahead_;  // the sung sound: from at_ on at ahead_[head_]
  size_t head_ = 0;
  struct Plate;
  std::shared_ptr<Plate> plate_;
  std::vector<float> sent_;  // the voice as rendered, waiting for halo()
  double compEnv_ = 0;
  double lastEnd_ = -1e9;  // when the last phrase's last note ended (s)
  mutable double breathEnd_ = 0;  // take(): where the breath in ends in the sound it returned (s)
};

}  // namespace pajupilli

#endif  // PAJUPILLI_JAZZSINGER_H_
