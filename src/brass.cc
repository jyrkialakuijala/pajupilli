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


// B-flat trumpet (research/trumpet.md): the Smith-Watkins trumpet that Bilbao &
// Chick 2013 measured (their Fig. 7, digitised; its transfer matrices meet
// their measured peaks 1-7 within 1 %), the cup lumped (0.92 cm^3, a popping
// frequency ~815 Hz), x from the throat. The main tuning slide pulled 44.9 mm
// (at 400 mm): measured at 20 C, the bore is ~50 cents sharp with breathed air.
// Leadpipe 8.7 -> 11.75 mm, the cylinder to ~880 mm, a Bessel bell (eps ~0.6)
// to 126.8 mm.
const BorePoint kTrumpetBore[] = {
    {0.0, 4.16}, {1.9, 4.07}, {6.0, 4.07}, {42.4, 5.79}, {47.4, 6.20}, {52.5, 6.49}, {57.2, 6.80}, {62.3, 7.10},
    {67.3, 7.49}, {77.4, 8.10}, {77.9, 9.69}, {78.4, 9.58}, {78.9, 8.70}, {81.2, 8.70}, {86.3, 8.80}, {88.2, 8.70},
    {150.5, 10.08}, {200.4, 10.78}, {250.4, 11.38}, {275.4, 11.71}, {308.3, 11.75}, {400.0, 11.75}, {761.4, 11.75},
    {776.2, 11.79}, {876.5, 11.79}, {976.4, 12.89}, {1016.2, 13.80}, {1066.2, 14.87}, {1076.3, 15.30}, {1116.5,
    16.49}, {1166.4, 18.29}, {1176.2, 18.76}, {1216.4, 20.51}, {1277.4, 24.52}, {1323.3, 29.94}, {1338.8, 33.09},
    {1352.4, 36.57}, {1362.5, 40.42}, {1378.3, 49.37}, {1383.3, 54.58}, {1389.3, 60.29}, {1393.8, 66.64}, {1398.8,
    73.67}, {1402.9, 81.41}, {1406.4, 89.98}, {1410.8, 99.44}, {1413.4, 109.90}, {1416.5, 126.77}
};

// Valves 1, 2, 3 (air meets 3, 2, 1): the loops fitted so partials 3..8 drop
// by their semitones (tools/brass.py -inst trumpet -fit). 1+3 and 1+2+3 are
// sharp (+27, +48 c): the ring finger kicks the 3rd slide out (ring13, ring123).
const ValveDef kTrumpetValves[] = {
    {765.0, 182.8, 11.75, 2},
    {710.0, 89.1, 11.75, 1},
    {655.0, 281.6, 11.75, 3},
};

// The lips (research/trumpet.md 5.3): mass, width and swept area after Harrison
// et al. 2015 (Adachi & Sato's trumpet); the horn's Q and second mode;
// Fletcher & Tarnopolsky's professional's 7 Pa/Hz, up to 20 kPa. Brassiness:
// the outgoing wave read at the end of the cylinder, ~0.9 m of equivalent
// cylinder (B ~ 0.6 x 1.47 m).
const LipParams kTrumpetLips = {0.92e-6, 3.7, 7.5, 0.010, 0.074, 1.46e-5, 7.0, 600, 20000, 0.74,
                                1.3, 3.0, 1.05, kTrumpetValves, 3, nullptr, 0, 0, 0.9, 870, 30.9, 57.5, 1.2, 2.2};

// Written F#3..C6 (sounding E3..B-flat 5): the standard chart; partial 7 unused.
const NoteInfo kTrumpetNotes[] = {
    {"xxx", 2}, {"xox", 2}, {"oxx", 2}, {"xxo", 2}, {"xoo", 2}, {"oxo", 2}, {"ooo", 2},  // F#3..C4
    {"xxx", 3}, {"xox", 3}, {"oxx", 3}, {"xxo", 3}, {"xoo", 3}, {"oxo", 3}, {"ooo", 3},  // C#4..G4
    {"oxx", 4}, {"xxo", 4}, {"xoo", 4}, {"oxo", 4}, {"ooo", 4},                          // G#4..C5
    {"xxo", 5}, {"xoo", 5}, {"oxo", 5}, {"ooo", 5},                                      // C#5..E5
    {"xoo", 6}, {"oxo", 6}, {"ooo", 6},                                                  // F5..G5
    {"oxx", 8}, {"xxo", 8}, {"xoo", 8}, {"oxo", 8}, {"ooo", 8},                          // G#5..C6
};

// Alternates (research/trumpet.md 4.2): 3 for 1+2, the next partial up.
const NoteInfo kTrumpetAlt[] = {
    {nullptr, 0}, {nullptr, 0}, {nullptr, 0}, {"oox", 2}, {nullptr, 0}, {nullptr, 0}, {nullptr, 0},  // F#3..C4
    {nullptr, 0}, {nullptr, 0}, {nullptr, 0}, {"oox", 3}, {nullptr, 0}, {nullptr, 0}, {nullptr, 0},  // C#4..G4
    {nullptr, 0}, {"oox", 4}, {"xxx", 5}, {"xox", 5}, {"oxx", 5},                                    // G#4..C5
    {"xxx", 6}, {"xox", 6}, {"oxx", 6}, {"xxo", 6},                                                  // C#5..E5
    {nullptr, 0}, {"xxx", 8}, {"xox", 8},                                                            // F5..G5
    {"xxx", 9}, {"oox", 8}, {nullptr, 0}, {"xxo", 9}, {"xoo", 9},                                    // G#5..C6
};

const Design kTrumpetDesign = {kTrumpetBore, int(sizeof kTrumpetBore / sizeof kTrumpetBore[0]), nullptr, 0,
                               kTrumpetNotes, 0.0, 1416.5, false, nullptr, 0, {}, {}, &kTrumpetLips, kTrumpetAlt};

// The trumpet with a Harmon mute, stem out (GM 60, "Muted Trumpet"; the Miles
// Davis sound; research/mutes.md). Its cork seals the bell at 48 mm (55 mm in
// from the rim): all the sound goes through the mute's neck (36 -> 44 mm,
// 45 mm), its chamber (88 x 85 mm, 0.5 l) and out of a 17 mm hole with a 31 mm
// sleeve. The chamber and the hole are the bore's last cells: a parasitic
// resonance at ~160 Hz below the trumpet's own, the low notes pulled sharp,
// the high ones flat (the lips take it up), the chamber's half-wave formant
// near 2 kHz, little below 1 kHz radiated.
const BorePoint kMutedTrumpetBore[] = {
    {0.0, 4.16}, {1.9, 4.07}, {6.0, 4.07}, {42.4, 5.79}, {47.4, 6.20}, {52.5, 6.49}, {57.2, 6.80}, {62.3, 7.10},
    {67.3, 7.49}, {77.4, 8.10}, {77.9, 9.69}, {78.4, 9.58}, {78.9, 8.70}, {81.2, 8.70}, {86.3, 8.80}, {88.2, 8.70},
    {150.5, 10.08}, {200.4, 10.78}, {250.4, 11.38}, {275.4, 11.71}, {308.3, 11.75}, {400.0, 11.75}, {761.4, 11.75},
    {776.2, 11.79}, {876.5, 11.79}, {976.4, 12.89}, {1016.2, 13.80}, {1066.2, 14.87}, {1076.3, 15.30}, {1166.4,
    18.29}, {1176.2, 18.76}, {1216.4, 20.51}, {1277.4, 24.52}, {1323.3, 29.94}, {1338.8, 33.09}, {1352.4, 36.57},
    {1362.5, 40.42}, {1376.0, 48.07}, {1376.5, 36.00}, {1421.0, 44.00}, {1421.5, 88.00}, {1506.0, 88.00}, {1506.5,
    17.00}, {1537.0, 17.00}
};

const Design kMutedTrumpetDesign = {kMutedTrumpetBore, int(sizeof kMutedTrumpetBore / sizeof kMutedTrumpetBore[0]),
                                    nullptr, 0, kTrumpetNotes, 0.0, 1537.0, false, nullptr, 0, {}, {},
                                    &kTrumpetLips, kTrumpetAlt};

// Tenor trombone with an F attachment: the Conn 8H that Bilbao & Chick 2013
// measured (their Fig. 7, read from the preprint's vector paths; its transfer
// matrices meet their measured peaks 1-16 within ~1 %; research/trombone.md),
// a Denis Wick 5AL mouthpiece (cup 9.6 cm^3, lumped 9.0: the lips take some;
// popping ~520 Hz), x from the throat. The slide's two legs (13.6 mm) and its
// crook 62.6..1567.4 mm; the tuning slide pulled 75 mm (at 1720 mm) for warm,
// breathed air; the bell section's cone and a Bessel-like flare to 213.7 mm.
const BorePoint kTromboneBore[] = {
    {0.0, 8.00}, {2.5, 7.23}, {17.5, 7.69}, {22.5, 8.08}, {27.5, 8.37}, {32.5, 8.89}, {42.5, 9.87}, {47.5, 10.63},
    {52.6, 12.72}, {57.6, 12.76}, {62.6, 13.60}, {662.5, 13.60}, {737.7, 14.79}, {892.3, 13.60}, {1567.4, 13.60},
    {1639.4, 13.84}, {1667.7, 14.23}, {1720.0, 14.33}, {1843.2, 14.43}, {1878.3, 14.48}, {1979.8, 15.16}, {2079.9,
    17.26}, {2180.2, 19.38}, {2187.0, 19.38}, {2289.1, 21.46}, {2376.8, 24.44}, {2389.3, 25.50}, {2476.3, 29.98},
    {2488.9, 31.04}, {2522.0, 33.07}, {2562.1, 36.62}, {2589.0, 39.00}, {2600.3, 40.40}, {2632.3, 44.75}, {2656.6,
    49.36}, {2676.1, 54.47}, {2689.2, 60.53}, {2705.5, 66.79}, {2716.8, 73.69}, {2726.2, 81.31}, {2734.3, 90.03},
    {2743.1, 99.34}, {2749.3, 109.61}, {2755.6, 121.36}, {2761.8, 133.91}, {2771.8, 164.17}, {2777.5, 181.15},
    {2781.8, 199.87}, {2783.7, 213.74}
};

// The F trigger: a rotor at the bell section's start, its loop fitted so
// partials 3..8 drop a fourth (tools/brass.py -inst trombone -fit).
const ValveDef kTromboneValves[] = {
    {1673.0, 966.7, 13.9, 5},
};

// The slide (mm of tube added, twice the travel): positions 1..7 fitted so
// partials 3..8 drop by their semitones (travel 86 .. 592 mm: "about 23 in"
// to 7th), then the F side's (the trigger down: wider apart; C2 in its 6th
// lies just past the B-flat 7th, its 7th out of reach).
const double kTromboneSlide[14] = {0, 171.9, 354.3, 546.8, 748.9, 960.8, 1183.9,
                                   0, 218.0, 456.1, 703.7, 965.6, 1242.5, 1500.0};

// The lips (research/trombone.md 5): width 13 mm (Boutin et al. 2020); mass
// 3.6 kg/m^2, the trumpet's (Mattéoli's 9.1 split the slots: a louder note
// jumped sharp, no dynamics left), its Q and second mode; mezzo-forte ~1.4 kPa
// at B-flat 2. Brassiness: the slide dominates (B = 0.71): the wave read at
// the end of the cylinder (~1.9 m), ~2 m of equivalent cylinder.
const LipParams kTromboneLips = {9.0e-6, 3.6, 7.5, 0.013, 0.074, 1.9e-5, 12.0, 500, 12000, 0.88,
                                 1.3, 3.0, 1.05, kTromboneValves, 1, kTromboneSlide, 62.6, 1567.4, 2.0, 1900,
                                 0, 0, 1.2, 2.2};

// C2..F5: the standard positions ("1".."7"; "A".."G" with the trigger) and
// partials. From A-flat 4 up the bore's partials 8..12 are 8..35 cents sharp:
// the player lowers the position ("+75": mm of tube more; the bore's error
// from tools/brass.py, then trimmed by the calibration's remaining error).
const NoteInfo kTromboneNotes[] = {
    {"Fx", 2},  // C2
    {"Ex", 2},  // C#2
    {"Dx", 2},  // D2
    {"Cx", 2},  // Eb2
    {"7o", 2},  // E2
    {"6o", 2},  // F2
    {"5o", 2},  // F#2
    {"4o", 2},  // G2
    {"3o", 2},  // Ab2
    {"2o", 2},  // A2
    {"1o", 2},  // Bb2
    {"7o", 3},  // B2
    {"6o", 3},  // C3
    {"5o", 3},  // C#3
    {"4o", 3},  // D3
    {"3o", 3},  // Eb3
    {"2o", 3},  // E3
    {"1o", 3},  // F3
    {"5o", 4},  // F#3
    {"4o", 4},  // G3
    {"3o", 4},  // Ab3
    {"2o", 4},  // A3
    {"1o", 4},  // Bb3
    {"4o", 5},  // B3
    {"3o", 5},  // C4
    {"2o", 5},  // C#4
    {"1o", 5},  // D4
    {"3o", 6},  // Eb4
    {"2o", 6},  // E4
    {"1o", 6},  // F4
    {"5o", 8},  // F#4
    {"4o", 8},  // G4
    {"3o+75", 8},  // Ab4
    {"2o+65", 8},  // A4
    {"1o+50", 8},  // Bb4
    {"4o+40", 10},  // B4
    {"3o+45", 10},  // C5
    {"2o+45", 10},  // C#5
    {"1o+40", 10},  // D5
    {"3o+85", 12},  // Eb5
    {"2o+90", 12},  // E5
    {"1o+60", 12},  // F5
};

// Alternates: the trigger for F2, E2, C3, B2; the long positions on the next
// partial up (players choose them to save slide travel).
const NoteInfo kTromboneAlt[] = {
    {nullptr, 0},  // C2
    {nullptr, 0},  // C#2
    {nullptr, 0},  // D2
    {nullptr, 0},  // Eb2
    {"Bx", 2},  // E2
    {"Ax", 2},  // F2
    {nullptr, 0},  // F#2
    {nullptr, 0},  // G2
    {nullptr, 0},  // Ab2
    {nullptr, 0},  // A2
    {nullptr, 0},  // Bb2
    {"Bx", 3},  // B2
    {"Ax", 3},  // C3
    {nullptr, 0},  // C#3
    {nullptr, 0},  // D3
    {nullptr, 0},  // Eb3
    {"7o", 4},  // E3
    {"6o", 4},  // F3
    {nullptr, 0},  // F#3
    {nullptr, 0},  // G3
    {"7o", 5},  // Ab3
    {"6o", 5},  // A3
    {"5o", 5},  // Bb3
    {"7o", 6},  // B3
    {"6o", 6},  // C4
    {"5o", 6},  // C#4
    {"4o", 6},  // D4
    {"6o", 7},  // Eb4
    {"5o", 7},  // E4
    {"4o", 7},  // F4
    {"3o", 7},  // F#4
    {"2o", 7},  // G4
    {nullptr, 0},  // Ab4
    {"4o", 9},  // A4
    {"3o", 9},  // Bb4
    {"2o", 9},  // B4
    {"1o", 9},  // C5
    {"5o", 12},  // C#5
    {"4o", 12},  // D5
    {nullptr, 0},  // Eb5
    {nullptr, 0},  // E5
    {nullptr, 0},  // F5
};

const Design kTromboneDesign = {kTromboneBore, int(sizeof kTromboneBore / sizeof kTromboneBore[0]), nullptr, 0,
                                kTromboneNotes, 0.0, 2783.7, false, nullptr, 0, {}, {}, &kTromboneLips,
                                kTromboneAlt};

// CC tuba, 4 valves (research/tuba.md): no published bore was found, so the
// shape is fitted (tools/brass.py's transfer matrices) to the resonances
// measured on a Besson BE994 (Mattéoli et al. 2022), transposed to C, within
// 4.3 cents rms on peaks 2..12: an 8.3 mm throat, a cone to the 19.5 mm valve
// cylinder, a long two-part cone and a Bessel bell to 470 mm; 5.05 m open.
// The cup (9 cm^3) lumped.
const BorePoint kTubaBore[] = {
    {0.0, 8.30}, {14.0, 8.30}, {90.0, 13.00}, {534.7, 18.12}, {558.7, 18.38}, {582.7, 18.62}, {606.7, 18.82},
    {630.7, 19.00}, {654.7, 19.15}, {678.7, 19.28}, {702.7, 19.38}, {726.7, 19.44}, {750.7, 19.49}, {774.7, 19.50},
    {1211.0, 19.50}, {1235.0, 19.52}, {1259.0, 19.58}, {1283.0, 19.67}, {1307.0, 19.81}, {1331.0, 19.98}, {1355.0,
    20.19}, {1379.0, 20.44}, {1403.0, 20.73}, {1427.0, 21.05}, {1451.0, 21.41}, {3423.6, 52.89}, {3447.6, 53.37},
    {3471.6, 54.07}, {3495.6, 54.96}, {3519.6, 56.07}, {3543.6, 57.38}, {3567.6, 58.90}, {3591.6, 60.62}, {3615.6,
    62.55}, {3639.6, 64.69}, {3663.6, 67.03}, {4259.9, 127.79}, {4364.6, 134.91}, {4456.0, 142.44}, {4535.8,
    150.38}, {4605.4, 158.77}, {4666.3, 167.62}, {4719.4, 176.97}, {4765.8, 186.84}, {4806.3, 197.25}, {4841.7,
    208.25}, {4872.5, 219.87}, {4899.5, 232.13}, {4923.0, 245.07}, {4943.6, 258.74}, {4961.5, 273.17}, {4977.2,
    288.40}, {4990.9, 304.48}, {5002.8, 321.46}, {5013.3, 339.39}, {5022.4, 358.31}, {5030.3, 378.29}, {5037.3,
    399.39}, {5043.3, 421.66}, {5048.6, 445.18}, {5053.2, 470.00}
};

// Valves 1, 2, 3 and 4 (a fourth), non-compensating, the loops fitted to
// partials 2..10 of each alone.
const ValveDef kTubaValves[] = {
    {900.0, 638.6, 19.5, 2},
    {800.0, 300.8, 19.5, 1},
    {1000.0, 999.1, 19.5, 3},
    {1100.0, 1804.6, 19.5, 5},
};

// The lips: research/tuba.md 5 (w 16 mm in a 33 mm cup, H up to 2.5 mm);
// the trumpet's and trombone's mass, Q, second mode. Mezzo-forte ~6 Pa/Hz x
// 2 (the low notes a few hundred Pa) up to 8 kPa. Brassiness small (B ~0.36:
// a cone), read after the valves.
const LipParams kTubaLips = {9.0e-6, 3.6, 7.5, 0.016, 0.074, 3.2e-5, 12.0, 300, 8000, 0.88,
                             1.3, 3.0, 1.05, kTubaValves, 4, nullptr, 0, 0, 0.5, 1300,
                             0, 0, 1.2, 2.2};

// E1..F4 (Everett's CC chart, the 4-valve choices: 4 for 1-3, 2-4 for
// 1-2-3). The combinations, and a few single valves, are sharp: "+613" pulls
// the last pressed valve's slide out (mm of tube; tools/brass.py), as tuba
// players do. Partials 3..5 of the open and single-valve series are flat by
// 25..50 cents: lipped up.
const NoteInfo kTubaNotes[] = {
    {"oxxx+613", 2},  // E1
    {"xxox+480", 2},  // F1
    {"oxox+584", 2},  // F#1
    {"ooox+303", 2},  // G1
    {"oxxo+216", 2},  // Ab1
    {"xxoo+52", 2},  // A1
    {"xooo", 2},  // Bb1
    {"oxoo", 2},  // B1
    {"oooo", 2},  // C2
    {"oxox+186", 3},  // C#2
    {"ooox+168", 3},  // D2
    {"oxxo+366", 3},  // Eb2
    {"xxoo+409", 3},  // E2
    {"xooo+334", 3},  // F2
    {"oxoo+150", 3},  // F#2
    {"oooo", 3},  // G2
    {"oxxo", 4},  // Ab2
    {"xxoo", 4},  // A2
    {"xooo", 4},  // Bb2
    {"oxoo", 4},  // B2
    {"oooo", 4},  // C3
    {"xxoo", 5},  // C#3
    {"xooo", 5},  // D3
    {"oxoo", 5},  // Eb3
    {"oooo", 5},  // E3
    {"xooo+85", 6},  // F3
    {"oxoo+59", 6},  // F#3
    {"oooo", 6},  // G3
    {"oxxo+77", 8},  // Ab3
    {"xxoo+42", 8},  // A3
    {"xooo", 8},  // Bb3
    {"oxoo", 8},  // B3
    {"oooo", 8},  // C4
    {"oxoo", 9},  // C#4
    {"oooo", 9},  // D4
    {"oxoo", 10},  // Eb4
    {"oooo", 10},  // E4
    {"xooo", 12},  // F4
};

const NoteInfo kTubaAlt[] = {
    {nullptr, 0},  // E1
    {"xoox", 2},  // F1
    {nullptr, 0},  // F#1
    {"xoxo", 2},  // G1
    {nullptr, 0},  // Ab1
    {"ooxo", 2},  // A1
    {nullptr, 0},  // Bb1
    {nullptr, 0},  // B1
    {"xoox", 3},  // C2
    {"xxxo", 3},  // C#2
    {"xoxo", 3},  // D2
    {nullptr, 0},  // Eb2
    {"ooxo", 3},  // E2
    {nullptr, 0},  // F2
    {"oxox", 4},  // F#2
    {"xoxo", 4},  // G2
    {nullptr, 0},  // Ab2
    {"ooxo", 4},  // A2
    {nullptr, 0},  // Bb2
    {nullptr, 0},  // B2
    {"oxxo", 5},  // C3
    {"xxxo", 6},  // C#3
    {"xoxo", 6},  // D3
    {"oxxo", 6},  // Eb3
    {"xxoo", 6},  // E3
    {nullptr, 0},  // F3
    {nullptr, 0},  // F#3
    {nullptr, 0},  // G3
    {nullptr, 0},  // Ab3
    {nullptr, 0},  // A3
    {nullptr, 0},  // Bb3
    {nullptr, 0},  // B3
    {"xooo", 9},  // C4
    {"xxoo", 10},  // C#4
    {"xooo", 10},  // D4
    {"oxxo", 10},  // Eb4
    {"xxoo", 12},  // E4
    {nullptr, 0},  // F4
};

const Design kTubaDesign = {kTubaBore, int(sizeof kTubaBore / sizeof kTubaBore[0]), nullptr, 0, kTubaNotes, 0.0,
                            5053.2, false, nullptr, 0, {}, {}, &kTubaLips, kTubaAlt};

}  // namespace

// Horn in F: sounds a fifth below written; 32 kHz (dx 11.2 mm, 342 cells and
// the loops).
extern const Instrument kBrass[] = {
    {"horn", -7, 48, 84, 1, 1, 1, 32000, 0, false, false, 0, 0.3, Kind::kLip, &kHornDesign, "horn"},
    // B-flat trumpet: sounds a tone below written; 48 kHz (dx 7.5 mm, 190 cells and the loops).
    {"trumpet", -2, 54, 84, 1, 1, 1, 48000, 0, false, false, 0, 0.3, Kind::kLip, &kTrumpetDesign, "trumpet"},
    // Tenor trombone (F attachment), at pitch; 32 kHz (dx 11.2 mm, 249 cells, the
    // slide's 134 stretched up to 2x).
    {"trombone", 0, 36, 77, 1, 1, 1, 32000, 0, false, false, 0, 0.3, Kind::kLip, &kTromboneDesign, "trombone"},
    // CC tuba, at pitch; 20 kHz (dx 17.9 mm, 282 cells and the loops).
    {"tuba", 0, 28, 65, 1, 1, 1, 20000, 0, false, false, 0, 0.3, Kind::kLip, &kTubaDesign, "tuba"},
    {"muted-trumpet", -2, 54, 84, 1, 1, 1, 48000, 0, false, false, 0, 0.3, Kind::kLip, &kMutedTrumpetDesign, "trumpet"},
};
extern const int kNumBrass = int(sizeof kBrass / sizeof kBrass[0]);

}  // namespace pajupilli
