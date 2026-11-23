#include "bassinst.h"
#include "heard.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <cstring>

#include "knob.h"

namespace pajupilli {

namespace {

// E1 .. G4 (MIDI 28 .. 67): every note is a string and a place on it. Above,
// to C6 (MIDI 84): harmonics.
const NoteInfo kBassNotes[64] = {
#define N {"", 1}
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N
#undef N
};
const Design kBassDesign = {nullptr, 0, nullptr, 0, kBassNotes, 0, 0, false, nullptr, 0, {}, {}};
const NoteInfo kPianoNotes[88] = {
#define N {"", 1}
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N
#undef N
};
const Design kPianoDesign = {nullptr, 0, nullptr, 0, kPianoNotes, 0, 0, false, nullptr, 0, {}, {}};
// (the open strings and the top of the stopped notes are the instrument's: BassSpec::open, top;
// the bass E1 A1 D2 G2 and G4)
const int kThumbVel = 45;  // up to this velocity: the thumb
const int kFlatVel = 72;   // up to this: the flat finger; above: the stacked pull

// A string player tunes a stopped note by ear, to ring with the open strings (spectral tuning):
// where one of its first six overtones can meet one of an open string's, within 15 c of equal
// temperament, the note goes there (the simplest such meeting, then the nearest); else it stays
// equal-tempered. (E on the viola rings with the open A as a fourth, +2 c; F with the C as a
// fourth's octave, -2 c; the open strings stay as tuned, in pure fifths.)
double ringCents(const BassSpec& sp, double fEt) {
  double best = 0, bestScore = 1e9;
  for (int o = 0; o < 4; ++o)
    for (int m = 1; m <= 6; ++m)
      for (int n = 1; n <= 6; ++n) {
        if (std::gcd(m, n) != 1) continue;
        const double dev = 1200 * std::log2(sp.strings[o].f0 * m / n / fEt);
        const double score = std::fabs(dev) + 2.0 * (m + n);
        if (std::fabs(dev) <= 15 && score < bestScore) bestScore = score, best = dev;
      }
  return best;
}

// The sounding length that stops string s semis above its open note (0: open).
double stopLength(const Bass& b, int s, int semis) {
  if (!semis) return 0;
  static const double kRingTune = knob("PJ_RING_TUNE", 1.0);
  const BassSpec& sp = b.spec();
  const double et = semis + sp.openFlat[s];
  const double fEt = sp.strings[s].f0 * std::exp2(et / 12.0);
  return b.openLength(s) / std::exp2((et + kRingTune * ringCents(sp, fEt) / 100) / 12.0);
}

}  // namespace

extern const Instrument kPlucked[] = {
    {"double-bass", 0, 28, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kBassDesign, "bass"},
    {"contrabass", 0, 28, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kBassDesign, "bass"},
    // The cello under the bow (research/cello.md): the bass's construction at its size, C2 .. A5
    // stopped, harmonics to C7.
    {"cello", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kBassDesign, "cello"},
    // The viola (research/viola.md): the cello's construction with a viola's strings, body and
    // bridge, C3 .. E6 stopped, harmonics to E7.
    {"viola", 0, 48, 100, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kBassDesign, "viola"},
    // The violin (research/viola.md, Toward the violin): the viola's construction at its size, G3 ..
    // C7 stopped, harmonics to A7.
    {"violin", 0, 55, 105, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kBassDesign, "violin"},
    // The concert grand (src/piano.cc, src/pianoinst.cc): A0 .. C8.
    {"grand-piano", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    // The bright piano (General MIDI 2): a Shigeru Kawai SK-7, the 229 cm grand (src/sk7.cc).
    {"bright-piano", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    // A 121 cm upright (src/upright.cc): its scale and board.
    {"upright-piano", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    // The honky-tonk (General MIDI 4): the upright out of tune, its hammers worn (src/pianoinst.cc).
    {"honky-tonk", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    // The Yamaha CP-80 electric grand (src/cp80.cc): A0 .. C8.
    {"electric-grand", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    // The Fender Rhodes (src/rhodes.cc), General MIDI 5: the 88's harp, A0 .. C8.
    {"electric-piano-1", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    // The DX7's E.PIANO 1 (src/dx7.cc), General MIDI 6: A0 .. C8.
    {"electric-piano-2", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    // FX 8 (sci-fi) (src/scifi.cc), General MIDI 104: a folded discharge and an FM'd filter ping, A0 .. C8.
    {"fx-sci-fi", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "fx"},
    // FX 1 (rain) (src/rain.cc), General MIDI 97: rain on tuned glass bowls, A0 .. C8.
    {"fx-rain", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "fx"},
    // FX 2 (soundtrack) (src/soundtrack.cc), General MIDI 98: a keyed aeolian harp, A0 .. C8.
    {"fx-soundtrack", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "fx"},
    // FX 4 (atmosphere) (src/atmosphere.cc), General MIDI 100: a keyed psaltery with magnetic
    // resonators, A0 .. C8.
    {"fx-atmosphere", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "fx"},
    // FX 5 (brightness) (src/brightness.cc), General MIDI 101: a keyboard of singing arcs, A0 .. C8.
    {"fx-brightness", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "fx"},
    // FX 6 (goblins) (src/goblins.cc), General MIDI 102: a pyrophone (singing flames in glass
    // pipes), A0 .. C8.
    {"fx-goblins", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "fx"},
    // FX 7 (echoes) (src/echoes.cc), General MIDI 103: a tuning-fork keyboard into a tape echo,
    // A0 .. C8.
    {"fx-echoes", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "fx"},
    // The Hohner Clavinet D6, E7 and C (src/clavinet.cc): F1 .. E6.
    {"clavinet", 0, 29, 88, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    {"clavinet-e7", 0, 29, 88, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    {"clavinet-c", 0, 29, 88, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "piano"},
    // The snare drum under brushes (src/snare.cc): General MIDI's drum keys 37 .. 41.
    {"brush-snare", 0, 36, 48, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The hi-hat (src/hihat.cc): General MIDI's drum keys 42, 44, 46.
    {"hi-hat", 0, 36, 48, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The chromatic percussion (src/bars.cc), at sounding pitch: the celesta C3 .. F8, the
    // glockenspiel G5 .. C8, the marimba C2 .. C7, the xylophone F4 .. C8, tubular bells C4 .. F5,
    // the music box C4 .. C8.
    {"celesta", 0, 48, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "celesta"},
    {"glockenspiel", 0, 36, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "glockenspiel"},
    {"marimba", 0, 36, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "marimba"},
    {"xylophone", 0, 36, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "xylophone"},
    {"tubular-bells", 0, 36, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "bells"},
    {"music-box", 0, 36, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "music box"},
    // The classical guitar (src/guitar.cc): E2 .. B5; notes outside are moved in by octaves.
    {"nylon-guitar", 0, 28, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "guitar"},
    {"clean-guitar", 0, 28, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "guitar"},
    // The steel-string acoustic (GM 26; src/guitar.cc steelGuitar()): a dreadnought, flatpicked.
    {"steel-guitar", 0, 28, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "guitar"},
    // The jazz guitar (GM 27; src/guitar.cc jazzGuitar(), src/jazzguitar.cc): an ES-175, flatwounds, into a
    // clean jazz amp; E2 .. C6 (notes outside folded in by octaves).
    {"jazz-guitar", 0, 28, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "guitar"},
    {"muted-guitar", 0, 28, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "guitar"},
    {"overdriven-guitar", 0, 28, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "guitar"},
    // The electric bass (src/ebass.cc): a Jazz Bass, fingerstyle; E1 .. D#4, notes outside moved in by octaves.
    {"electric-bass", 0, 24, 72, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "bass"},
    // GM 35 (src/ebass.cc PickBassPlayer): a Precision Bass played with a pick, into an SVT; E1 .. D#4.
    {"pick-bass", 0, 24, 72, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "bass"},
    // GM 36 (src/fretless.cc): the Jazz Bass defretted, its board under the finger (the mwah), slides,
    // scoops, vibrato, tuned by ear to the band; E1 .. D#4 stopped, natural harmonics above (to E4).
    {"fretless-bass", 0, 24, 76, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "bass"},
    // GM 37, 38 (src/ebass.cc SlapBassPlayer): the Jazz Bass slapped and popped, its frets
    // struck; 38 active, stainless, pop-forward; E1 .. D#4.
    {"slap-bass-1", 0, 24, 72, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "bass"},
    {"slap-bass-2", 0, 24, 72, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "bass"},
    {"distortion-guitar", 0, 28, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "guitar"},
    // GM 32: the clean Strat in harmonics (src/guitarinst.cc HarmonicsPlayer): E3 .. G#7 as heard.
    {"guitar-harmonics", 0, 52, 104, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "guitar"},
    // The harpsichord (src/harpsichord.cc, on the piano's strings): F1 .. F6; 8' + 8', the back 8'
    // alone, and all four choirs (16' 8' 8' 4').
    {"harpsichord", 0, 24, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "harpsichord"},
    {"harpsichord-8", 0, 24, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "harpsichord"},
    {"harpsichord-full", 0, 24, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "harpsichord"},
    // The hammered dulcimer (src/dulcimer.cc, on the piano's strings): G2 .. A6.
    {"dulcimer", 0, 36, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "dulcimer"},
    // The jazz bass drum, 18" (and 20") (src/kick.cc): General MIDI's drum keys 35, 36.
    {"jazz-kick", 0, 35, 36, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"jazz-kick-20", 0, 35, 36, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // Shakers and jingles (src/shaker.cc): General MIDI's drum keys 70, 69, 54; GM2's 82, 83.
    {"maracas", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"shaker", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"cabasa", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"tambourine", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"jingle-bell", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The cymbals (src/cymbal.cc): General MIDI's drum keys 49, 51, 52, 53, 55, 57, 59 (24 up: brushed).
    {"ride-cymbal", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"ride-cymbal-2", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"crash-cymbal", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"crash-cymbal-2", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"splash-cymbal", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"china-cymbal", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The toms (src/tom.cc): General MIDI's drum keys 41 .. 50, one drum a key (24 up: brushed).
    {"tom-10", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"tom-12", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"tom-13", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"tom-14", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"tom-16", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"tom-16-low", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The timpani (src/timpani.cc): five kettledrums, C2 .. E4 (32 in. to 20 in.); outside, the outer drums.
    {"timpani", 0, 28, 72, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "timpani"},
    // The concert harp (src/harp.cc, src/harpinst.cc): 47 strings, C1 .. G7; outside, moved in by octaves.
    {"harp", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "harp"},
    // The triangle and the cowbell (src/metalperc.cc): General MIDI's drum keys 80, 81 and 56.
    {"triangle", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"cowbell", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The guiro, cuica, vibraslap and whistle (src/scrapers.cc): General MIDI's drum keys 73/74, 78/79, 58, 71/72 (the pea whistle).
    {"guiro", 0, 73, 74, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"cuica", 0, 78, 79, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"vibraslap", 0, 58, 58, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"pea-whistle", 0, 71, 72, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The hand drums (src/handdrum.cc): General MIDI's 60, 61 (bongos), 62 .. 64 (congas), 65, 66 (timbales).
    {"conga", 0, 62, 64, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"bongo", 0, 60, 61, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"timbale", 0, 65, 66, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The small sounds below 35 (src/percextras.cc; research/gm2_extras.md).
    {"perc-extras", 0, 20, 34, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The hand clap, claves and wood blocks, surdo, bell tree (src/clap.cc, woodperc.cc, surdo.cc,
    // belltree.cc; research/clap_claves_surdo.md): General MIDI's 39, 75 .. 77; GM2's 86/87, 84.
    {"hand-clap", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"claves", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"wood-blocks", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"surdo", 0, 13, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    {"bell-tree", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The snare with sticks (src/sticksnare.cc; research/stick_snare.md): 36 roll .. 44.
    {"stick-snare", 0, 36, 44, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "drum"},
    // The church organ's first rank (src/organ.cc; research/church_organ.md): Gedackt 8', C2..C7.
    {"gedackt-8", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "organ"},
    {"rohrgedackt-8", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "organ"},
    {"gedackt-rohrgedackt", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "organ"},
    {"principal-8", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "organ"},
    {"organ", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "organ"},
    {"organ-crescendo", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "organ"},
    {"organ-88", 0, 21, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "organ"},
    {"organ-plan", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "organ"},
    // The free reeds with a bellows (src/accordion.cc; research/free_reeds.md): the accordion's musette
    // and bass (General MIDI 22), the bandoneón (24, the tango accordion).
    {"accordion", 0, 24, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "accordion"},
    {"bandoneon", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "bandoneon"},
    // The chromatic harmonica (src/harmonica.cc; research/harmonica.md): 16 holes, C3..D7 (General MIDI 23).
    {"harmonica", 0, 48, 98, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "harmonica"},
    // The reed organ (src/harmonium.cc; research/reed_organ.md): a pressure harmonium, C2..C7 (General MIDI 21).
    {"reed-organ", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "reed organ"},
    // The Hammond B-3 through its Leslie 122 (src/hammond.cc, src/leslie.cc; research/hammond_b3.md,
    // research/leslie.md): General MIDI 17 (drawbar-organ), 18 (percussive-organ), 19 (rock-organ).
    {"hammond", 0, 24, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "hammond"},
    {"drawbar-organ", 0, 24, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "hammond"},
    {"percussive-organ", 0, 24, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "hammond"},
    {"rock-organ", 0, 24, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "hammond"},
    // (routing: one Hammond and its Leslie, the notes on this division: -reinstrument piano=hammond-upper-manual)
    {"hammond-upper-manual", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "hammond"},
    {"hammond-lower-manual", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "hammond"},
    {"hammond-pedal", 0, 24, 48, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kString, &kPianoDesign, "hammond"},
};
extern const int kNumPlucked = sizeof(kPlucked) / sizeof(kPlucked[0]);

bool bowedBass(const Instrument& inst) {
  return std::strcmp(inst.name, "contrabass") == 0 || std::strcmp(inst.name, "cello") == 0 ||
         std::strcmp(inst.name, "viola") == 0 || std::strcmp(inst.name, "violin") == 0;
}

static const BassSpec& specOf(const Instrument& inst) {
  return std::strcmp(inst.name, "cello") == 0    ? kCelloSpec
         : std::strcmp(inst.name, "viola") == 0  ? kViolaSpec
         : std::strcmp(inst.name, "violin") == 0 ? kViolinSpec
                                                 : kDoubleBassSpec;
}

BassModel::BassModel(const Instrument& inst) : Tube(inst), bass_(inst.fs, specOf(inst)), bowed_(bowedBass(inst)) {
  // Under the bow the bass is heard from further away (a hall's microphone:
  // the whole body), with a little of the f-hole and the bridge.
  if (bowed_) bass_.setMics(knob("PJ_ARCO_FAR", 0.55), knob("PJ_ARCO_HOLE", 0.3), knob("PJ_ARCO_PICKUP", 0.15));
  // How the instrument stands in the room (x to the listener's right, y towards the listener,
  // z up), for its body's three axes: across (towards the treble side), along (towards the
  // tail), out of the top.
  //   - A bass or a cello stands on its pin facing the listener: its top towards them, the neck
  //     up, its low strings to the listener's right.
  //   - A viola lies under the chin, its neck out to the player's left (the listener's right),
  //     its top up and tilted some 30 degrees towards the listener, the treble side nearer them.
  static const float kStanding[9] = {-1, 0, 0, 0, 0, -1, 0, 1, 0};
  static const float kUnderChin[9] = {0, 0.866f, -0.5f, -1, 0, 0, 0, 0.5f, 0.866f};
  std::copy_n(std::strcmp(inst.name, "viola") == 0 || std::strcmp(inst.name, "violin") == 0 ? kUnderChin : kStanding, 9, axes_);
  // (tests: PJ_SEAT n: this instrument built as a section's n-th, Bass::vary)
  static const int kSeat = int(knob("PJ_SEAT", 0));
  if (kSeat > 0) bass_.vary(unsigned(kSeat));
}

void BassModel::resetAir() {
  Tube::resetAir();
  bass_.reset();
}

float BassModel::sample() {
  last_ = bass_.sample(float(testDp_));  // (tests: a force on the bridge's bass foot)
  testDp_ = 0;
  // (A mezzo-forte pluck peaks near 0.1; the bow gives the string far more.)
  const float g = bowed_ ? 1.0f : 0.7f;
  side_.push_back(g * float(outScale()) * bass_.side());
  for (int k = 0; k < 3; ++k) {
    if (dip_[k].size() > 4096) dip_[k].clear();  // (nobody takes them: no room)
    dip_[k].push_back(g * float(outScale()) * bass_.dipole(k));
  }
  return finish(g * last_);
}

// The plucking place from the brightness knob (0.3 .. 2.5, the knob's whole
// turn): the share of the sounding length from the bridge, or -1 for the
// wrong side of the bridge.
//   knob 0     over the fingerboard, the middle of the string (sul tasto):
//              dark, woody, booming: the fundamental, few overtones
//   knob 0.2   up the fingerboard
//   knob 0.32  the usual place, 10 cm above the fingerboard's end (at rest)
//   knob 0.5   at the fingerboard's end
//   knob 0.8   by the bridge (sul ponticello): tight, bright
//   knob 0.9+  behind the bridge
double BassPlayer::pluckPlace(double brightness) {
  const double x = std::clamp((brightness - 0.3) / 2.2, 0.0, 1.0);
  if (x > 0.88) return -1;
  static const double kx[] = {0, 0.2, 0.32, 0.5, 0.8, 0.88}, kw[] = {0.5, 0.36, 0.29, 0.19, 0.07, 0.05};
  for (int i = 0; i < 5; ++i)
    if (x <= kx[i + 1]) return kw[i] + (kw[i + 1] - kw[i]) * (x - kx[i]) / (kx[i + 1] - kx[i]);
  return kw[5];
}

const char* BassPlayer::pluckPlaceName(double brightness) {
  const double w = pluckPlace(brightness);
  return w < 0 ? "behind the bridge" : w > 0.42 ? "over the fingerboard (sul tasto)" : w > 0.33 ? "up the fingerboard"
       : w > 0.24 ? "the usual place" : w > 0.14 ? "at the fingerboard's end" : "by the bridge (sul ponticello)";
}

BassPlayer::BassPlayer(BassModel& m) : Performer(m), m_(m), open_(m.bass().spec().open), top_(m.bass().spec().top) {}

void BassPlayer::seed(unsigned s) { rng_ = 777u + 7919u * s; }

void BassPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int s = 0; s < 4; ++s)
      if (key_[s] < 0) m_.bass().damp(s);
}

// Which string: as a bassist chooses, by the line, not note by note. Each
// string that has the note (and is free) costs:
//   - its position: 0.5 a semitone up the neck (the low positions are home);
//   - an open string 0.8 (taken on purpose: it cannot be bent or shaken, and
//     it rings on), unless the hand has rested (then it is as good as any);
//   - leaving the string of the last note: 1.2;
//   - moving the hand: 0.6 a semitone from where it is (an open string needs
//     no hand; the cost fades as the hand rests: gone after about 2 s).
// So a phrase stays on its string while the hand can reach, and crosses to
// the next when that is nearer than a shift.
int BassPlayer::pickString(int midi) const {
  int best = -1;
  double bestCost = 1e9;
  const double fresh = std::exp(-sinceAny_ / 0.8);  // how present the last note still is
  for (int k = 0; k < 4; ++k) {
    const int pos = midi - open_[k];
    if (pos < 0 || pos > 26) continue;
    if (key_[k] >= 0 && key_[k] != midi) continue;  // held by another key
    double cost = 0.5 * pos;
    if (pos == 0) cost += 0.8 * fresh;
    else if (hand_ >= 0) cost += 0.6 * std::abs(pos - hand_) * fresh;
    if (lastString_ >= 0 && k != lastString_) cost += 1.2 * fresh;
    if (cost < bestCost) { bestCost = cost; best = k; }
  }
  if (best >= 0) return best;
  // All that have it are held: the lowest position's note gives way.
  for (int k = 3; k >= 0; --k)
    if (midi >= open_[k]) return k;
  return 0;
}

void BassPlayer::noteOn(int midi, int velocity) {
  if (midi < inst_.lo || midi > inst_.hi) return;
  static const double glideVel = knob("PJ_GLIDEVEL", 45), dvGlide = knob("PJ_DVGLIDE", 30);
  Bass& b = m_.bass();
  // The left hand alone: a soft key under a held one, on that key's string.
  if (lastString_ >= 0 && key_[lastString_] >= 0 && velocity <= glideVel && velocity - lastVel_ <= -dvGlide &&
      midi >= open_[lastString_] && midi - open_[lastString_] <= 24 && midi <= top_ && !harm_[lastString_]) {
    const int s = lastString_;
    const int from = key_[s], semis = midi - open_[s];
    const double len = stopLength(b, s, semis);
    b.setLength(s, len, std::abs(midi - from) <= 2 ? 0.012 : 0.04 + 0.012 * std::abs(midi - from));
    key_[s] = midi;
    since_[s] = 0.1;  // (the vibrato goes on)
    note_ = midi;
    return;
  }
  // Above G4: harmonics. A natural one where an open string has it (its
  // 2nd to 6th: 12, 19, 24, 28, 31 semitones over the open string, pure, so
  // the 5th is 14 c flat, as on the instrument), on the lowest such harmonic;
  // otherwise an artificial one: the note an octave (a twelfth, two octaves)
  // lower stopped, and touched at its half (third, quarter).
  int harmonic = 0, stopNote = midi;
  int s = -1;
  // The same note again (within ~1.2 s): a bassist does not pluck it twice alike. The two make
  // one gesture: the second changes the spectrum, the envelope or the resonance, one or two of
  //   - the finger: the other one, or a fingertip after the stacked pull, the flat finger or
  //     the thumb (clearer, punchier);
  //   - the place: nearer the bridge (brighter, focused), or (an echo) further up the board;
  //   - the pull: turned a little up, off the fingerboard (a snap against the ebony);
  //   - the string: the same pitch on the next one (open to stopped, or back), its own colour
  //     and decay;
  //   - a slide into it from a quarter- to a semitone below, on the attack;
  //   - a quick, narrow vibrato blooming on it (the first one straight);
  // and the two are never as loud: soft to strong (an upbeat) or strong to soft (an echo), when
  // the keys themselves are alike. The finger lands on the still swinging string a moment
  // before it pulls (Bass::pluck's holdDamp): a clean second attack. In a run of the same note
  // the pairs follow one another. (PJ_BASS_PAIRS=0: off.)
  static const bool kPairs = knob("PJ_BASS_PAIRS", 1) > 0;
  const bool again = kPairs && midi == lastMidi_ && midi <= top_ && sinceAny_ < 1.2 && pluckPlace(brightness_) >= 0;
  pairPos_ = again ? pairPos_ ^ 1 : 0;
  const bool second = again && pairPos_ == 1;
  enum { kWFinger = 1, kWPlace = 2, kWAngle = 4, kWString = 8, kWSlide = 16, kWVib = 32 };
  int ways = 0;
  bool echo = false;
  if (second) {
    static const int kAll[6] = {kWFinger, kWPlace, kWAngle, kWString, kWSlide, kWVib};
    static const double kP[6] = {0.25, 0.25, 0.12, 0.14, 0.1, 0.14};  // (how often each is the one)
    for (int pick = 0; pick < 2; ++pick) {
      double r = 0.5 * (urand() + 1);
      for (int w = 0; w < 6; ++w)
        if ((r -= kP[w]) <= 0 || w == 5) { ways |= kAll[w]; break; }
      if (urand() < 0) break;  // (one way, half the time)
    }
    echo = urand() < -0.2;  // (an upbeat more often than an echo)
  }
  if (midi > top_) {
    static const int kSemis[5] = {12, 19, 24, 28, 31};
    for (int n = 2; n <= 6 && s < 0; ++n)
      for (int k = 3; k >= 0 && s < 0; --k)
        if (midi == open_[k] + kSemis[n - 2] && key_[k] < 0) { s = k; harmonic = n; stopNote = open_[k]; }
    if (s < 0) {
      harmonic = midi - 12 <= top_ ? 2 : midi - 19 <= top_ ? 3 : 4;
      stopNote = midi - (harmonic == 2 ? 12 : harmonic == 3 ? 19 : 24);
      s = pickString(stopNote);
    }
  } else {
    s = pickString(midi);
    if (ways & kWString) {  // the same pitch on a neighbouring string, if the hand reaches it
      int alt = -1;
      for (int k : {s - 1, s + 1})
        if (k >= 0 && k < 4 && midi - open_[k] >= 0 && midi - open_[k] <= 9 && (key_[k] < 0 || key_[k] == midi)) {
          alt = k;
          break;
        }
      if (alt >= 0) {
        if (key_[s] == midi) key_[s] = -1;
        b.damp(s, 45);  // (the first string let go under the hand)
        s = alt;
      } else {
        ways = (ways & ~kWString) | kWPlace;
      }
    }
  }
  const int semis = stopNote - open_[s];
  // The pull: 2 mm (pp) to 9 mm (ff), with the dynamics knob; no two alike.
  const double v = velocity / 127.0;
  double depth = std::min(0.012, (0.002 + 0.007 * v * v) * (0.6 + 0.8 * dyn_) * (1 + 0.06 * urand()));
  if (second && std::abs(velocity - lastVel_) < 12)  // (keys alike: the hand shapes the two)
    depth = std::min(0.012, depth * (echo ? 0.78 : 1.2));
  // Where: the brightness knob (pluckPlace); loud notes a little nearer the
  // bridge.
  const double place = pluckPlace(brightness_);
  if (place < 0) {  // the wrong side of the bridge: the short length there pings
    const int sb = pickString(std::min(midi, top_));
    b.pluckBehind(sb, 0.6 * depth);
    lastMidi_ = -1;
    key_[sb] = midi;
    since_[sb] = 0;
    harm_[sb] = true;
    lastVel_ = velocity;
    lastString_ = -1;
    note_ = midi;
    blowing_ = true;
    sinceAny_ = 0;
    return;
  }
  double where = std::clamp(place - 0.03 * (v - 0.5) + 0.01 * urand(), 0.06, 0.5);
  // Fast notes (the last pluck under 250 ms ago, fully by 120 ms): the hand
  // moves to the gap by the bridge (sul ponticello: tight and clear), and
  // the two fingertips take turns.
  const double fast = std::clamp((0.25 - sinceAny_) / 0.13, 0.0, 1.0);
  where += fast * (std::min(where, 0.09) - where);
  if (ways & kWPlace) where = std::clamp(where * (echo ? 1.35 : 0.7), 0.06, 0.5);
  // The pull's direction, the air knob: along the top (round, sustaining) at
  // its rest and below; turned up, more and more outward, so that the string
  // swings back onto the fingerboard (the growl, then the slap).
  // A hard key does the same by itself: from velocity 100 the pull turns
  // outward (the snap), fully at 127 (the slap).
  const double snap = std::clamp((velocity - 100) / 27.0, 0.0, 1.0);
  const double angle = std::max(std::clamp(0.05 + 1.25 * (air_ - 0.35) / 0.65, 0.0, 1.3),
                                1.25 * snap * snap * (3 - 2 * snap)) + 0.06 * urand() + (ways & kWAngle ? 0.3 : 0.0);
  // The hand, by the key's velocity: soft (up to 45) the thumb; then (to
  // 72) the flat finger, the index's side along the string; then the stacked
  // pull (index and middle as one); hard (from 100) the same pull turned
  // outward: snap, then slap. Fingertips taking turns when the
  // notes come fast (under ~210 ms apart); the thumb always with the
  // hardness knob turned down.
  int finger = hardness_ <= 0.75 || (velocity <= kThumbVel && fast <= 0.3) ? 3
               : fast > 0.3 ? -1 : velocity <= kFlatVel ? 4 : 2;
  if ((ways & kWFinger) && hardness_ > 0.75)  // (the other finger: a tip after a pad, the other tip after a tip)
    finger = lastFinger_ == 0 ? 1 : lastFinger_ == 1 ? 0 : (urand() < 0 ? 0 : 1);
  lastFinger_ = finger;
  if (harmonic) {  // a fingertip near the bridge; the other hand's finger touches
    b.pluck(s, semis ? b.openLength(s) / std::exp2((semis + b.spec().openFlat[s]) / 12.0) : 0, 1.3 * depth, 0.085 + 0.01 * urand(), 0.05, 0);
    b.touch(s, harmonic);
  } else {
    b.pluck(s, semis ? b.openLength(s) / std::exp2((semis + b.spec().openFlat[s]) / 12.0) : 0, depth, where, angle, finger);
  }
  harm_[s] = harmonic != 0;
  sinceAny_ = 0;
  // A ghost note (the mute pad held): the left hand lies on the string, the
  // pluck is a short thud of no clear pitch.
  if (flutter_ > 0 && !harmonic) {
    b.damp(s, 150);
    for (int k = 0; k < 4; ++k)  // (the hand lies across the others too)
      if (k != s && key_[k] < 0) b.damp(k, 60);
  }
  // Stopped notes are slid into a little from below (5-20 c, more when loud;
  // gone in ~50 ms): the finger settles.
  scoop_[s] = semis > 0 && !harmonic && fast < 0.3 ? (5 + 15 * v) * (0.5 + 0.5 * std::fabs(urand())) : 0;
  if ((ways & kWSlide) && semis > 0) scoop_[s] = 50 + 50 * std::fabs(urand());  // (a quarter- to a semitone below)
  vibSoon_[s] = (ways & kWVib) && semis > 0;
  lastMidi_ = midi;
  if (semis > 0) hand_ = semis;
  key_[s] = midi;
  since_[s] = 0;
  lastVel_ = velocity;
  lastString_ = s;
  note_ = midi;
  blowing_ = true;
}

void BassPlayer::noteOff(int midi) {
  bool any = false;
  for (int s = 0; s < 4; ++s) {
    if (key_[s] == midi) {
      key_[s] = -1;
      // The left hand stops the string. A short note is stopped firmly (a
      // clean, dry end); a note held long is let go slowly: the finger lifts
      // and the sound trails off. The release knob scales it (turned up, the
      // note rings on).
      const double held = since_[s];
      const double firm = held < 0.15 ? 1.6 : held > 0.6 ? 0.55 : 1.6 - 1.05 * (held - 0.15) / 0.45;
      if (!hold_) {
        m_.bass().damp(s, std::clamp(firm * 1.2 / std::max(0.01, release_), 4.0, 120.0));
        // A dry stop takes the strings that rang along too (the hand lies on
        // them); after a long note they are left to ring.
        if (firm > 1.2)
          for (int k = 0; k < 4; ++k)
            if (k != s && key_[k] < 0) m_.bass().damp(k, 30);
      }
    }
    any |= key_[s] >= 0;
  }
  if (!any) {
    blowing_ = false;
    released();
  } else if (midi == note_) {
    for (int s = 0; s < 4; ++s)
      if (key_[s] >= 0) note_ = key_[s];
  }
}

void BassPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    tick(bdt);
    sinceAny_ += bdt;
    // Vibrato: slow and wide, on stopped strings, coming in after 250 ms.
    vibPhase_ += 2 * M_PI * 0.85 * vibRate_ * bdt;
    if (vibPhase_ > 2 * M_PI) vibPhase_ -= 2 * M_PI;
    for (int s = 0; s < 4; ++s) {
      since_[s] += bdt;
      if (key_[s] < 0) continue;
      const double in = std::clamp((since_[s] - 0.25) / 0.3, 0.0, 1.0);
      // The stopping finger: the joystick slides it (sideways: up to two
      // semitones either way; pushed forward: it rocks, a vibrato on top of
      // the knob's), and it settles into each note from a little below.
      // (the second of a pair blooming: its own narrow vibrato, from 80 ms, ~12 c)
      const double soon = vibSoon_[s] ? 12 / 230.0 * std::clamp((since_[s] - 0.08) / 0.15, 0.0, 1.0) : 0.0;
      const double vib = 230 * (vibDepth_ * in + soon + 0.3 * std::max(0.0, breathCtl_)) * std::sin(vibPhase_);
      m_.bass().bend(s, key_[s] > open_[s] && !harm_[s] ? vib + 200 * roll_ - scoop_[s] * std::exp(-since_[s] / 0.05) : 0.0);
    }
    // The strings' height over the fingerboard, the air knob: at its rest and
    // below, high and clean; turned up, lower and lower: the strings graze
    // the board through the note (the growl).
    m_.bass().setAction(std::clamp(1.15 - 1.2 * (air_ - 0.35), 0.35, 1.5));
    // The mute pad struck: a rake, the fingers drawn across three muted
    // strings (G, D, A), 28 ms apart, as hard as the pad was hit.
    if (flutter_ > 0 && !rakeDown_) {
      rakeDown_ = true;
      rakeNext_ = 3;
      rakeClock_ = 0;
      rakeDepth_ = 0.002 + 0.006 * flutter_;
    }
    if (flutter_ <= 0) rakeDown_ = false;
    if (rakeNext_ > 0 && (rakeClock_ -= bdt) <= 0) {
      const int rs = rakeNext_;  // strings 3, 2, 1
      if (key_[rs] < 0) {
        m_.bass().pluck(rs, 0, rakeDepth_ * (1 + 0.1 * urand()), 0.2, 0.05, 0);
        m_.bass().damp(rs, 170);
      }
      --rakeNext_;
      rakeClock_ = 0.028;
      blowing_ = true;
    }
    m_.process(out + i, m);
  }
}

// ---------------------------------------------------------------- the bow

// The bow's place from the brightness knob (0.3 .. 2.5), as a share of the
// sounding length from the bridge:
//   knob 0     over the fingerboard's end (sul tasto): 0.20
//   knob 0.32  the usual place (at rest): 0.11
//   knob 1     at the bridge (sul ponticello): 0.03
double BowPlayer::bowPlace(double brightness) {
  const double x = std::clamp((brightness - 0.3) / 2.2, 0.0, 1.0);
  return x < 0.32 ? 0.20 * std::pow(0.11 / 0.20, x / 0.32) : 0.11 * std::pow(0.03 / 0.11, (x - 0.32) / 0.68);
}

const char* BowPlayer::bowPlaceName(double brightness) {
  const double w = bowPlace(brightness);
  return w > 0.16 ? "over the fingerboard (sul tasto)" : w > 0.125 ? "towards the fingerboard" : w > 0.09 ? "the usual place"
       : w > 0.055 ? "towards the bridge" : "at the bridge (sul ponticello)";
}

BowPlayer::BowPlayer(BassModel& m) : BassPlayer(m), wait_(m.bass().spec().pizzWait), waitOwn_(wait_) {}

void BowPlayer::seed(unsigned s) {
  BassPlayer::seed(s);
  vib_.seed(s);
  vib_.rateMul = 1 + 0.08 * urand();
  vib_.depthMul = 1 + 0.15 * urand();
  vib_.setPhase(M_PI * (1 + urand()));
}

// One of a section: no two players stop a note at quite the same place (up to 6 cents either
// way, fixed for the player: open strings are what they are), shake alike (the vibrato's rate
// 12 % either way: 5.3 to 6.7 Hz on a violin; its depth 30 %; beginning up to 0.15 s later), or
// draw the bow alike (speed and pressure 10 %). Each is somewhere else in its bow, so the
// section's bow changes never fall together.
void BowPlayer::vary(unsigned seat) {
  uint32_t r = seat * 2246822519u + 4099u;
  auto u = [&]() {  // -1 .. 1
    r = r * 1664525u + 1013904223u;
    return (r >> 8) * (2.0 / 16777216.0) - 1;
  };
  seatCents_ = 6 * u();
  vib_.rateMul = 1 + 0.12 * u();
  vib_.depthMul = 1 + 0.3 * u();
  seatVibDelay_ = 0.075 * (1 + u());
  seatSpeed_ = 1 + 0.1 * u();
  seatPress_ = 0.1 * u();
  seatBow_ = 0.85 + 0.15 * u();  // (70 to 100 % of the hair)
  travel_ = 0.5 * (1 + u()) * seatBow_ * m_.bass().spec().bowLength;
  dir_ = u() > 0 ? 1 : -1;
  seated_ = true;
  seatRng_ = r | 1;
  // Its section's habits, shared by the players of one kind of instrument and different from
  // the other sections': the vibrato's rate (7 % either way) and depth (15 %), when it begins,
  // the bow's speed (8 %: so its turning points) and pressure.
  r = uint32_t(m_.bass().spec().strings[0].f0 * 16) * 2654435761u + 977u;
  u();
  vib_.rateMul *= 1 + 0.07 * u();
  vib_.depthMul *= 1 + 0.15 * u();
  seatVibDelay_ += 0.03 * (1 + u());
  seatSpeed_ *= 1 + 0.08 * u();
  seatPress_ += 0.05 * u();
  seatBow_ = std::min(1.0, seatBow_ * (1 + 0.08 * u()));
}

void BowPlayer::setHold(bool h) {
  hold_ = h;
  if (!h)
    for (int s = 0; s < 4; ++s)
      if (on_[s] && key_[s] < 0) lift(s);
}

// Which string, stopped where (as the plucked bass chooses), or which
// harmonic.
void BowPlayer::place(int& s, int midi, int& stopNote, int& harmonic) const {
  harmonic = 0;
  stopNote = midi;
  s = -1;
  if (midi > top_) {
    // Harmonics: an open string's 2nd to 4th where there is one (the bow
    // holds those well); otherwise the note an octave (a twelfth, two
    // octaves) lower, stopped, and touched at its half (third, quarter).
    static const int kSemis[3] = {12, 19, 24};
    for (int n = 2; n <= 4 && s < 0; ++n)
      for (int k = 3; k >= 0 && s < 0; --k)
        if (midi == open_[k] + kSemis[n - 2] && key_[k] < 0) { s = k; harmonic = n; stopNote = open_[k]; }
    if (s < 0) {
      // (Not stopped in the last fifth of the neck if a lower stop will do:
      // the short string there is hard to hold in two.)
      harmonic = midi - 12 <= top_ - 7 ? 2 : midi - 19 <= top_ - 7 ? 3 : midi - 24 <= top_ - 7 ? 4 : midi - 12 <= top_ ? 2 : 3;
      stopNote = midi - (harmonic == 2 ? 12 : harmonic == 3 ? 19 : 24);
      s = pickString(stopNote);
    }
  } else {
    s = pickString(midi);
  }
}

// The bow comes to string s for this key (newStroke: from the air; else it
// is already moving: a slur across the strings).
void BowPlayer::start(int s, int midi, int stopNote, int harmonic, int velocity, bool newStroke) {
  Bass& b = m_.bass();
  const int semis = stopNote - open_[s];
  // (A stiff string's harmonics run sharp: the hand stops an artificial one
  // ~7 c lower, by ear.)
  b.finger(s, harmonic ? (semis ? b.openLength(s) / std::exp2((semis + b.spec().openFlat[s] - 0.07) / 12.0) : 0) : stopLength(b, s, semis));
  harmN_[s] = harmonic ? harmonic : 1;
  if (harmonic) b.touch(s, harmonic, 1e9, knob("PJ_ARCO_TOUCH", 6000));
  key_[s] = midi;
  since_[s] = 0;
  harm_[s] = harmonic != 0;
  vel_[s] = velocity;
  on_[s] = true;
  strokeT_[s] = newStroke ? 0 : 0.5;
  if (semis > 0) hand_ = semis;
  lastVel_ = velocity;
  lastString_ = s;
  note_ = midi;
  blowing_ = true;
  sinceAny_ = 0;
}

void BowPlayer::lift(int s) {
  Bass& b = m_.bass();
  on_[s] = false;
  // The bow leaves (the release knob: slower); the string rings on under
  // the finger, which then lets it go.
  b.bow(s, dir_ * speed_[s], 0, dist_[s], std::clamp(0.7 * release_, 0.015, 0.4));
  b.damp(s, std::clamp(0.3 / std::max(0.01, release_), 1.5, 14.0));
}

// Pizzicato from the keyboard: a key let go within the instrument's wait (the cello: 100 ms,
// PJ_PIZZ_MS) is a pluck, not a stroke. Nobody can know that when the key goes down, so every
// key and release of this instrument is acted on that much later (a file's notes are sent that
// much early: noteLead); a key released while it still waits is plucked at its time.
void BowPlayer::noteOn(int midi, int velocity) {
  if (midi < inst_.lo || midi > inst_.hi) return;
  if (wait_ <= 0) { strike(midi, velocity); return; }
  pend_.push_back({midi, velocity, 0.0, true, false});
}

void BowPlayer::noteOff(int midi) {
  if (wait_ <= 0) { letGo(midi); return; }
  for (Pending& p : pend_)
    if (p.on && !p.pizz && p.midi == midi && midi <= top_) { p.pizz = true; return; }
  pend_.push_back({midi, 0, 0.0, false, false});
}

void BowPlayer::due(double dt) {
  for (Pending& p : pend_) p.age += dt;
  size_t n = 0;
  for (; n < pend_.size() && pend_[n].age >= wait_; ++n) {
    const Pending p = pend_[n];
    if (!p.on) { letGo(p.midi); continue; }
    if (!p.pizz) { strike(p.midi, p.vel); continue; }
    for (int s = 0; s < 4; ++s)
      if (on_[s] && key_[s] < 0) { m_.bass().bow(s, 0, 0, dist_[s], 0.01); on_[s] = false; }
    // The nail plucks (finger 5: it catches the string and lets it go at once, every overtone);
    // the key's velocity is only how far it pulls the string aside: 0.8 mm (pp) to 6 mm (ff),
    // over the fingerboard's end. Below PJ_PIZZ_FLESH (velocity; default 0: never) the
    // fingertip's flesh plucks instead, rounder. The stopping finger holds on while the string
    // rings, then lets it go.
    static const double kFlesh = knob("PJ_PIZZ_FLESH", 0);
    Bass& b = m_.bass();
    const int st = pickString(p.midi), semis = p.midi - open_[st];
    if (on_[st]) { b.bow(st, 0, 0, dist_[st], 0.005); on_[st] = false; }
    const double v = p.vel / 127.0;
    b.setSkin(0.35);  // (a cellist's fingertip: softer than a jazz bassist's)
    // How far the finger pulls the string aside: by its length, not by a fixed distance (the
    // same pull bends a short string more sharply), from PJ_PIZZ_PP of the sounding length at
    // the softest key to that + PJ_PIZZ_FF at the hardest, rising with the velocity squared as
    // the bow's speed does; and less again on a string stopped high up (the sounding length
    // against the open string's, to the power PJ_PIZZ_UP): the short string is stiff under the
    // finger and there is little room over the fingerboard. Set so that a pizzicato is softer
    // than the same key bowed (about 6 dB; measured over three notes and four velocities each on
    // the viola and the cello): plucked by a fixed 0.8 .. 6 mm it was up to 20 dB louder. The
    // lowest string open: 0.4 mm (pp) to 5 mm (ff) on the viola, 0.7 to 9 mm on the cello.
    static const double kPizzPp = knob("PJ_PIZZ_PP", 0.00105), kPizzFf = knob("PJ_PIZZ_FF", 0.0126),
                        kPizzUp = knob("PJ_PIZZ_UP", 2.0);
    const double plen = b.length(st) > 0 ? stopLength(b, st, semis) : 0;
    const double sounding = semis ? plen : b.openLength(st);
    // (... and by the string: the heavy low strings are pulled furthest, each higher string
    // less, as its open frequency against the lowest string's: pulled alike, the thin A string
    // came out 10 dB louder against its bowed sound than the C string against its own.)
    const double pull = sounding * (kPizzPp + kPizzFf * v * v) * std::pow(sounding / b.openLength(st), kPizzUp) *
                        b.spec().strings[0].f0 / b.spec().strings[st].f0;
    b.pluck(st, stopLength(b, st, semis), pull * (1 + 0.05 * urand()), 0.20 + 0.01 * urand(), 0.2, p.vel < kFlesh ? 0 : 5);
    static const double kRing = knob("PJ_PIZZ_RING", 7.0);
    b.damp(st, kRing);
    key_[st] = -1;
    since_[st] = 0;
    harm_[st] = false;
    if (semis > 0) hand_ = semis;
    lastString_ = st;
    lastVel_ = p.vel;
    note_ = p.midi;
    blowing_ = true;
    sinceAny_ = 0;
    lastOn_ = 0;
  }
  if (n) pend_.erase(pend_.begin(), pend_.begin() + n);
}

// A key, at once or after the pizzicato wait: the bow.
void BowPlayer::strike(int midi, int velocity) {
  static const double glideVel = knob("PJ_GLIDEVEL", 45), dvGlide = knob("PJ_DVGLIDE", 30);
  Bass& b = m_.bass();
  down_[midi] = true;
  order_[midi] = ++clock_;
  int sounding = 0;
  for (int s = 0; s < 4; ++s) sounding += on_[s] && key_[s] >= 0;
  const bool chord = sounding > 0 && lastOn_ < 0.06;
  lastOn_ = 0;
  int s, stopNote, harmonic;
  if (sounding > 0 && !chord) {
    // Legato: the same stroke goes on; the note last begun moves.
    int from = lastString_ >= 0 && on_[lastString_] && key_[lastString_] >= 0 ? lastString_ : -1;
    for (int k = 0; k < 4 && from < 0; ++k)
      if (on_[k] && key_[k] >= 0) from = k;
    const int old = key_[from];
    const bool wasHarm = harm_[from];
    key_[from] = -1;
    place(s, midi, stopNote, harmonic);
    if (s == from && !harmonic && !wasHarm) {
      // The left hand alone: a finger down or up, a shift for a wider step;
      // the glide gesture (a soft key under a held one): a slow slide.
      const int semis = stopNote - open_[s], step = std::abs(midi - old);
      const bool slide = velocity <= glideVel && velocity - lastVel_ <= -dvGlide;
      b.setLength(s, stopLength(b, s, semis),
                  slide ? 0.07 + 0.022 * step : step <= 4 ? 0.018 : 0.03 + 0.004 * step);
      key_[s] = midi;
      since_[s] = std::min(since_[s], 0.25);  // (the vibrato eases back in)
      if (!slide) vel_[s] = velocity;
      if (semis > 0) hand_ = semis;
      lastVel_ = velocity;
      note_ = midi;
      sinceAny_ = 0;
      return;
    }
    lift(from);  // the bow crosses to another string, moving on
    m_.bass().damp(from, 25);
    start(s, midi, stopNote, harmonic, velocity, false);
    return;
  }
  if (!chord) {  // a new stroke: the other way (after a rest: a down-bow)
    dir_ = sinceAny_ > 1.0 ? 1 : -dir_;
    travel_ = 0;
    if (seated_) {  // (one of a section: anywhere in the first 3/4 of its bow, and either way)
      seatRng_ = seatRng_ * 1664525u + 1013904223u;
      travel_ = 0.75 * ((seatRng_ >> 8) / 16777216.0) * seatBow_ * m_.bass().spec().bowLength;
      if (sinceAny_ > 1.0 && (seatRng_ & 0x80000000u)) dir_ = -1;
    }
  }
  place(s, midi, stopNote, harmonic);
  if (on_[s]) b.damp(s, 60);
  start(s, midi, stopNote, harmonic, velocity, true);
}

void BowPlayer::letGo(int midi) {
  down_[midi] = false;
  for (int s = 0; s < 4; ++s) {
    if (key_[s] != midi) continue;
    if (hold_) {  // the pedal: the bow stays on
      key_[s] = -1;
      continue;
    }
    // A key still held that no string plays (the note this one was slurred
    // from): back to it, in the same stroke.
    int back = -1;
    for (int k = inst_.lo; k <= inst_.hi; ++k) {
      if (!down_[k] || (back >= 0 && order_[k] < order_[back])) continue;
      bool playing = false;
      for (int t = 0; t < 4; ++t) playing |= key_[t] == k;
      if (!playing) back = k;
    }
    key_[s] = -1;
    if (back >= 0) {
      int s2, stopNote, harmonic;
      place(s2, back, stopNote, harmonic);
      if (s2 == s && !harmonic && !harm_[s]) {
        const int semis = stopNote - open_[s], step = std::abs(back - midi);
        m_.bass().setLength(s, stopLength(m_.bass(), s, semis), step <= 4 ? 0.018 : 0.03 + 0.004 * step);
        key_[s] = back;
        note_ = back;
        if (semis > 0) hand_ = semis;
      } else {
        lift(s);
        m_.bass().damp(s, 25);
        start(s2, back, stopNote, harmonic, vel_[s], false);
      }
      continue;
    }
    lift(s);
  }
  bool any = false;
  for (int s = 0; s < 4; ++s) any |= key_[s] >= 0;
  if (!any) {
    blowing_ = false;
    for (int s = 0; s < 4; ++s) blowing_ |= on_[s];
    if (!blowing_) released();
  } else if (midi == note_) {
    for (int s = 0; s < 4; ++s)
      if (key_[s] >= 0) note_ = key_[s];
  }
}

void BowPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  Bass& b = m_.bass();
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / fs_;
    tick(bdt);
    sinceAny_ += bdt;
    lastOn_ += bdt;
    if (!pend_.empty()) due(bdt);
    // The arm is not a motor: the bow's speed drifts a little (~1 Hz).
    drift1_ += std::sqrt(6 * bdt / 0.25) * urand() - bdt / 0.25 * drift1_;
    drift_ += (drift1_ - drift_) * bdt / 0.25;
    // Nor is the stopping finger a fret: the pitch trembles by a few cents
    // (the hand's own tremor, a noise under ~4 Hz). So a note never sits
    // exactly on an open string's overtone, which would swallow it (the open
    // string, ringing along at exactly that pitch, holds the bridge still).
    static const double kTremor = knob("PJ_ARCO_TREMOR", 3.0);
    trem1_ += std::sqrt(6 * bdt / 0.04) * urand() - bdt / 0.04 * trem1_;
    tremor_ += (trem1_ - tremor_) * bdt / 0.04;
    // Tremolo (the mute pad held): the bow shakes, ~13 strokes a second.
    const bool trem = flutter_ > 0;
    bool turned = false;
    if (trem && (tremClock_ -= bdt) <= 0) {
      dir_ = -dir_;
      tremClock_ = (1 / 13.0) * (1 + 0.15 * urand());
      turned = true;
    }
    double sinceOn = 1e9, fastest = 0;
    for (int s = 0; s < 4; ++s)
      if (on_[s] && key_[s] > open_[s] && !harm_[s]) sinceOn = std::min(sinceOn, since_[s]);
    const double support = std::clamp(dyn_ + breathCtl_, 0.0, 1.0);
    const double vib = vib_.step(bdt, b.spec().vibRate * vibRate_, sinceOn, support, wander_) * vib_.env() * vib_.depthMul;
    const double cents = 230 * (vibDepth_ + 0.04 * std::max(0.0, breathCtl_)) * vib;
    for (int s = 0; s < 4; ++s) {
      since_[s] += bdt;
      if (!on_[s]) continue;
      strokeT_[s] += bdt;
      const double v = vel_[s] / 127.0;
      // The bow's speed: 5 cm/s (pp) to ~50 cm/s (ff), with the dynamics knob
      // and the joystick's swell.
      double speed = (0.05 + b.spec().bowSpeed * v * v) * (0.6 + 0.8 * dyn_) * (1 + 1.3 * std::max(0.0, breathCtl_)) *
                     (1 + 0.05 * wander_ * drift_) * seatSpeed_;
      if (trem) speed *= 0.75;
      // By ear: a note the bass answers weakly gets more bow, a booming one
      // less (tools/bowlevels.py: 70 % of the difference, at most 6 dB).
      static const double kEven = knob("PJ_ARCO_EVEN", 0.7);
      const int li = key_[s] - b.spec().bowLevelLo;
      if (li >= 0 && li < b.spec().bowLevelN)
        speed *= std::pow(10.0, std::clamp(-kEven * double(b.spec().bowLevels[li]), -6.0, 6.0) / 20);
      // Its place: the knob's; a louder note nearer the bridge. Between the
      // bridge and the fingerboard's end.
      const double beta = bowPlace(brightness_) * (1.12 - 0.3 * v);
      // (no nearer the bridge than the hair's width allows, not past the fingerboard's end)
      const double dist = std::clamp(beta * b.length(s) / harmN_[s], 0.75 * b.spec().hair, 0.95 * b.spec().boardEnd);
      // Its pressure, the air knob; a hard key bites at the start (martele);
      // a harmonic wants a light, fast bow.
      const double bite = std::clamp((vel_[s] - 100) / 27.0, 0.0, 1.0) * std::max(0.0, 1 - strokeT_[s] / 0.07);
      double pressure = std::clamp(0.5 + 1.1 * (air_ - 0.35) + 0.3 * bite + seatPress_, -0.3, 1.2);
      if (harm_[s]) pressure -= 0.15;
      // How fast the stroke starts: a soft key eases in over ~130 ms, a hard
      // one in ~12 ms (the hardness knob scales it); a change of direction
      // takes ~15 ms.
      const double attack = std::clamp((0.14 - 0.13 * v) * b.spec().attackScale / hardness_, 0.006, 0.25);
      static const double kTurn = knob("PJ_ARCO_TURN", 0.015);
      const double rise = trem ? 0.006 : strokeT_[s] < 3 * attack ? attack : kTurn;
      // (The hair is laid on the string first, then drawn: the force is there
      // before the speed. Drawn before it grips, the string whistles.)
      static const double kLead = knob("PJ_ARCO_LEAD", 0.25);
      b.bow(s, dir_ * speed, b.bowForce(s, speed, dist, pressure, harmN_[s]), dist, rise, std::max(0.004, kLead * rise));
      speed_[s] = speed;
      dist_[s] = dist;
      fastest = std::max(fastest, speed);
      // (the instrument's own vibrato, with no knob: the same finger travel is wider in cents on
      // the shorter string up the neck, to 3.5 x)
      static const double kVibOwn = knob("PJ_ARCO_VIB", 1.0);  // (the player's own vibrato x; 0: none)
      const double own = kVibOwn * b.spec().vibNat * std::min(3.5, b.openLength(s) / std::max(0.05, b.length(s))) * vib * vibScale_ *
                         std::clamp((since_[s] - vibDelay_ - seatVibDelay_) / 0.15, 0.0, 1.0);
      b.bend(s, key_[s] > open_[s] && !harm_[s] ? cents + own + 200 * roll_ + kTremor * wander_ * tremor_ + seatCents_ : 0.0);
    }
    // The bow has a length (~55 cm of hair): used up, it turns.
    travel_ += fastest * bdt;
    if (turned) travel_ = 0;
    if (travel_ > seatBow_ * b.spec().bowLength && !trem) {
      dir_ = -dir_;
      static const bool kLog = std::getenv("PJ_BOW_LOG") != nullptr;  // (the bow changes, for the tests)
      if (kLog) say(stderr, "bowturn %.0f Hz %p %.3f\n", b.spec().strings[0].f0, (void*)this, sinceAny_);
      travel_ = 0;
    }
    m_.process(out + i, m);
  }
}

}  // namespace pajupilli
