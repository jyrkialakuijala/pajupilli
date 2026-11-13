// The brass: a lip-reed in a cup mouthpiece on a long bore with a bell, the
// notes chosen by valves and the partial (src/horn.cc, src/hornplayer.cc).
// The bores are designed with tools/brass.py (transfer matrices): keep the
// two in step.
#include "instrument.h"

namespace pajupilli {
namespace {

// Single horn in F (research/horns.md section 9). From the lips: the cup (a
// volume), the throat (4.4 mm), the backbore to 8 mm, a conical leadpipe to
// the 11.6 mm cylindrical valve section, a long cone to the bell and a Bessel
// flare to 305 mm; the leadpipe and the cone meet the cylinder smoothly (a
// kink reflects: every other partial came out 2-3 dB weaker). The flare and
// the cone's start are fitted for the most even partials, the length to put
// them on F1 (43.65 Hz): partials 2..16 within 10 cents (tools/brass.py). No
// hand in the bell yet: the partials above ~600 Hz are weak, as on an open horn.
const BorePoint kHornBore[] = {
    {0.0, 4.40}, {11.6, 4.40}, {60.0, 8.00}, {300.0, 9.35}, {325.0, 9.49}, {350.0, 9.62},
    {375.0, 9.75}, {400.0, 9.88}, {425.0, 10.00}, {450.0, 10.11}, {475.0, 10.23}, {500.0, 10.33},
    {525.0, 10.44}, {550.0, 10.54}, {575.0, 10.63}, {600.0, 10.72}, {625.0, 10.81}, {650.0, 10.89},
    {675.0, 10.96}, {700.0, 11.04}, {725.0, 11.11}, {750.0, 11.17}, {775.0, 11.23}, {800.0, 11.28},
    {825.0, 11.33}, {850.0, 11.38}, {875.0, 11.42}, {900.0, 11.46}, {925.0, 11.49}, {950.0, 11.52},
    {975.0, 11.55}, {1000.0, 11.56}, {1025.0, 11.58}, {1050.0, 11.59}, {1075.0, 11.60}, {1100.0, 11.60},
    {1519.3, 11.60}, {1544.3, 11.62}, {1569.3, 11.66}, {1594.3, 11.74}, {1619.3, 11.84}, {1644.3, 11.98},
    {1669.3, 12.15}, {1694.3, 12.34}, {1719.3, 12.57}, {1744.3, 12.83}, {1769.3, 13.12}, {1794.3, 13.43},
    {1819.3, 13.78}, {1844.3, 14.16}, {1869.3, 14.57}, {1894.3, 15.01}, {1919.3, 15.48}, {1944.3, 15.98},
    {1969.3, 16.51}, {1994.3, 17.07}, {2019.3, 17.66}, {2044.3, 18.28}, {2069.3, 18.94}, {2094.3, 19.62},
    {2119.3, 20.33}, {2144.3, 21.07}, {2169.3, 21.85}, {2194.3, 22.65}, {2219.3, 23.48}, {2244.3, 24.35},
    {2269.3, 25.24}, {2294.3, 26.16}, {2319.3, 27.12}, {3391.0, 68.70}, {3432.1, 73.10}, {3469.9, 77.79},
    {3504.6, 82.77}, {3536.5, 88.07}, {3565.8, 93.72}, {3592.7, 99.72}, {3617.3, 106.11}, {3640.0, 112.91},
    {3660.9, 120.15}, {3680.0, 127.84}, {3697.6, 136.04}, {3713.7, 144.75}, {3728.6, 154.03}, {3742.2, 163.90},
    {3754.7, 174.40}, {3766.2, 185.57}, {3776.7, 197.47}, {3786.4, 210.12}, {3795.3, 223.58}, {3803.5, 237.91},
    {3811.0, 253.15}, {3817.9, 269.37}, {3824.3, 286.63}, {3830.1, 305.00}
};

// Valves 1, 2, 3 (whole tone, semitone, minor third): the loops fitted so
// partials 4..12 drop by their semitones (tools/brass.py -fit). Combinations
// are sharp, as on a real horn (1-2-3 by ~55 cents): the player's lips and the
// calibration take it up.
const ValveDef kHornValves[] = {
    {950.0, 477.8, 11.6, 2},
    {1000.0, 228.1, 11.6, 1},
    {1050.0, 741.0, 11.6, 3},
};

// The lips: mass and opening after Velut et al. 2017 and Harrison et al. 2015,
// Q 7.5 (heavier below 190 Hz, src/horn.cc; a sharper lip splits each slot in two regimes and a slur can leave the note in the sharp one); the outward mode at 0.85 of the note (a one-mass lip only meets the
// bore's resonance in phase well above its own), the inward one 1.3x higher.
// Mezzo-forte mouth pressure ~1.25x Fletcher & Tarnopolsky's threshold (6 Pa/Hz)
// at middle C, rising as f^1.45 (src/hornplayer.cc): harder, the lips pull the
// note sharp of the bore.
const LipParams kHornLips = {1.5e-6, 5.0, 7.5, 0.010, 0.074, 1.5e-5, 7.5, 300, 9000, 0.85,
                             1.3, 3.0, 0.7, kHornValves, 3};

// Written C3..C6 (sounding F2..F5): the standard single F horn fingerings,
// "123" = valves 1, 2, 3 ('x' pressed); the partial of the open or valved horn.
const NoteInfo kHornNotes[] = {
    {"ooo", 2},   // C3
    {"xxx", 3},   // C#3
    {"xox", 3},   // D3
    {"oxx", 3},   // Eb3
    {"xxo", 3},   // E3
    {"xoo", 3},   // F3
    {"oxo", 3},   // F#3
    {"ooo", 3},   // G3
    {"oxx", 4},   // G#3
    {"xxo", 4},   // A3
    {"xoo", 4},   // Bb3
    {"oxo", 4},   // B3
    {"ooo", 4},   // C4
    {"xxo", 5},   // C#4
    {"xoo", 5},   // D4
    {"oxo", 5},   // Eb4
    {"ooo", 5},   // E4
    {"xoo", 6},   // F4
    {"oxo", 6},   // F#4
    {"ooo", 6},   // G4
    {"oxx", 8},   // G#4
    {"xxo", 8},   // A4
    {"xoo", 8},   // Bb4
    {"oxo", 8},   // B4
    {"ooo", 8},   // C5
    {"xxo", 10},  // C#5
    {"xoo", 10},  // D5
    {"oxo", 10},  // Eb5
    {"ooo", 10},  // E5
    {"xoo", 12},  // F5
    {"oxo", 12},  // F#5
    {"ooo", 12},  // G5
    {"oxx", 16},  // G#5
    {"xxo", 16},  // A5
    {"xoo", 16},  // Bb5
    {"oxo", 16},  // B5
    {"ooo", 16},  // C6
};

// Alternates (written): the same note on a neighbouring partial with other
// valves, as horn players use them when the standard one is out of tune or
// insecure (the calibration chooses per note). Up high the partials lie a
// semitone apart: partial 16 with valves is sharp, partial 15 the remedy.
const NoteInfo kHornAlt[] = {
    {nullptr, 0},  // C3
    {nullptr, 0},  // C#3
    {nullptr, 0},  // D3
    {nullptr, 0},  // Eb3
    {nullptr, 0},  // E3
    {nullptr, 0},  // F3
    {"xxx", 4},   // F#3
    {"xox", 4},   // G3
    {nullptr, 0},  // G#3
    {nullptr, 0},  // A3
    {"xxx", 5},   // Bb3
    {"xox", 5},   // B3
    {"oxx", 5},   // C4
    {"xxx", 6},   // C#4
    {"xox", 6},   // D4
    {"oxx", 6},   // Eb4
    {"xxo", 6},   // E4
    {nullptr, 0},  // F4
    {"xxx", 8},   // F#4
    {"xox", 8},   // G4
    {"xxx", 9},   // G#4
    {"xox", 9},   // A4
    {"oxx", 9},   // Bb4
    {"xxo", 9},   // B4
    {"xoo", 9},   // C5
    {"oxo", 9},   // C#5
    {"ooo", 9},   // D5
    {"oxx", 12},  // Eb5
    {"xxo", 12},  // E5
    {"xxx", 15},  // F5
    {"xox", 15},  // F#5
    {"oxx", 15},  // G5
    {"xxo", 15},  // G#5
    {"xoo", 15},  // A5
    {"oxo", 15},  // Bb5
    {"ooo", 15},  // B5
    {nullptr, 0},  // C6
};

const Design kHornDesign = {kHornBore, int(sizeof kHornBore / sizeof kHornBore[0]), nullptr, 0, kHornNotes,
                            0.0, 3830.1, false, nullptr, 0, {}, {}, &kHornLips, kHornAlt};

}  // namespace

// Horn in F: sounds a fifth below written; 32 kHz (dx 11.2 mm, 342 cells and
// the loops).
extern const Instrument kBrass[] = {
    {"horn", -7, 48, 84, 1, 1, 1, 32000, 0, false, false, 0, 0.3, Kind::kLip, &kHornDesign, "horn"},
};
extern const int kNumBrass = 1;

}  // namespace pajupilli
