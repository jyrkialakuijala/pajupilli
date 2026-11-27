// The director (Director.md): one musical reading of a piece for every part of -play.
//
// A file typed into a notation program has every onset on the grid, one or two velocities, no
// expression: played literally it is mechanical, and each instrument guessing alone does not make
// an ensemble. The director reads the whole song once, before anyone plays, and decides what a
// conductor decides: the style, the phrases and their climaxes, the dynamics arc, the accents,
// the vibrato, the articulation, the ritardando and the fermatas. It knows nothing of pipes,
// reeds or bows: it gives intentions, and each musician carries them out on its instrument.
//
// How it speaks to the musicians:
//   - the time is everyone's: the song's tempo map is rewritten once (ritardando, fermata, the
//     breath before a climax), so the ensemble stays together;
//   - each part has a level curve (0 pp .. 1 ff, 0.5 as written), followed block by block (the
//     crescendos, the terraces, a long note's swell);
//   - each note comes with a cue: a vector of kCueDims numbers (below), handed to the part's
//     players just before the key (Performer::direct). What a player cannot do, it ignores.
//
// It acts only on a file with no performance of its own (quantized, flat velocities, few tempo
// events), or when told to (-director on); a channel with its own expression rides keeps its
// dynamics and gets only the shared time. The vibrato it plans for every part whatever the file
// has (Direction::vib; PJ_DIRECTOR_VIBALL 0: not): each player chooses how to make it. PJ_DIRECTOR (0..2, default 1) scales every
// deviation; PJ_DIRECTOR_DYN, _ACCENT, _VIB, _RIT (1) one kind each; PJ_DIRECTOR_MUTE (1; 0: never,
// 2: always) the mutes, which go on for the thin, quiet phrases of a slow piece (not Baroque); PJ_DIRECTOR_FERMATA (1.6) the
// hold; PJ_DIRECTOR_QUANT (0.9) the quantized share needed. Deterministic: the same file, the
// same reading.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace pajupilli {

// The cue's dimensions.
enum {
  kCueLevel,      // the dynamic level at the note's start (0 pp .. 1 ff; 0.5: as written)
  kCueLevelEnd,   // ... and at its end (a crescendo or diminuendo through the note)
  kCueSwell,      // a long note's swell (messa di voce): how much the level rises inside it
  kCueSwellPeak,  // where its peak falls (share of the note's length)
  kCueAccent,     // emphasis: -1 (light, passing) .. 0 .. +1 (the strongest: a climax, a syncopation)
  kCueVibDepth,   // vibrato: x the player's own depth (0: none, 1: its usual, more at a climax)
  kCueVibDelay,   // ... beginning this long into the note (s)
  kCueVibRate,    // ... x the player's own rate
  kCueLength,     // the note's sounding length x its written one (already applied to the key's release)
  kCueLegato,     // 1: bound to the next note of the part; 0: detached
  kCuePhraseEnd,  // 1: the last note of a phrase (it tapers; a breath follows); 2: of the piece's last phrase
  kCueMute,       // 1: with the mute on (con sordino: the strings' bridge mute; a trumpet's is another
                  //    instrument, so the file player changes the part's program instead)
  kCueOnly,       // 1: a cue of the vibrato and the phrase ends only (Direction::vib: the rest as written)
  kCueTongue,     // a fast run's tonguing, the note's place in its group: 0 none (the player's own), 1 / 2
                  //    the front / back of a pair (ta-ka, da-ga), 3 / 4 / 5 the first / second / third of
                  //    a three (triplets, a compound beat: da-da-ga or da-ga-ga, the player's habit)
  kCueDims
};
struct Cue {
  float v[kCueDims];
};

struct DirNote { long tick, tickOff; int ch, key, vel; };  // (not the drums; sorted by tick)
struct DirTempo { double t, beat, len; };                  // from t s on (at `beat` quarters), a quarter lasts len s
struct DirInput {
  std::vector<DirNote> notes;
  std::vector<DirTempo> tempo;
  int div = 480;         // ticks a quarter
  long barTicks = 0;     // a bar (0: 4/4 taken)
  int tonic = -1;
  bool minor = false;
  bool ridden[16] = {};  // the channel has its own expression (CC 11, 1, 2 or pressure)
  bool wind[16] = {};    // the channel is a wind player's (a flute, a reed, a brass: it speaks and breathes)
  bool pluck[16] = {};   // ... a harpsichord's (a plucked keyboard: its expression is time, not velocity)
  bool flute[16] = {};   // ... a flute's (a transverse flute, a recorder, a pan pipe: its high notes blown hard)
  // For the jazz reading: the drum channel's strokes (tick, key), the programs that play notes,
  // and src/midifile.cc's jazz-trio rule (a piano and an upright bass).
  std::vector<std::pair<long, int>> drums;
  bool program[128] = {};
  bool jazzBand = false;
};

struct Direction {
  bool acts = false;
  bool self = false;  // (not acting: the wind parts' own reading only, their cues and levels; the time the file's)
  // The vibrato reaches every part (vib), also those the reading leaves alone: a played file's
  // parts, a channel's own expression rides; vibOnly: not acting and no wind's reading, only that.
  bool vib = false, vibOnly = false;
  bool gridded = false;  // (the file is quantized: its note lengths are the notation's, free to articulate)
  enum Style { kUnknown, kBaroque, kClassical, kRomantic } style = kUnknown;
  std::vector<DirTempo> tempo;                        // the song's new tempo map (if acts)
  std::vector<Cue> cues;                              // one per input note
  // A wind part's key velocities read as a line and accents (src/velline.h; the winds' own reading
  // only): the line goes into the part's level, the key keeps the accent. Per input note, the
  // velocity to play its key with (0: the file's).
  std::vector<int> keyVel;
  // Per input note, its key and its release moved later by so many ticks (a trill played alive,
  // the notes inégales); empty or 0: as written.
  std::vector<long> onShift, offShift;
  std::vector<std::pair<long, float>> level[16];      // per channel: (tick, level), linear between
  std::vector<std::pair<long, long>> phrases;         // (start, end) ticks
  std::vector<std::pair<long, double>> fermatas;      // (tick, hold factor)
  std::vector<std::pair<long, bool>> mutes;           // (tick, on): where the mutes go on and come off
  std::string summary;                                // what it decided, to print
  // Is it jazz (whatever else the reading says: the style above is the classical one's): the
  // players choose their own ways from it (the electric bass's DI share). What it heard, to print.
  bool jazz = false;
  std::string jazzWhy;
};

// The director's verdict on the piece being played: jazz or not (false until a file is read).
bool& jazzPiece();

// -director on|off|auto (default auto). False: not one of those.
bool setDirectorMode(const std::string& mode);
// -style baroque|classical|romantic|auto: the reading's style forced, not read from the rhythm;
// a style given also turns the director on (as -director on), unless it is off. False: no such style.
bool setDirectorStyle(const std::string& style);
Direction direct(const DirInput& in);
// A level curve at a tick.
float levelAt(const std::vector<std::pair<long, float>>& curve, long tick);

}  // namespace pajupilli
