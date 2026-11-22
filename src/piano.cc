#include "piano.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"
#include "pianoboard_data.h"

namespace pajupilli {

namespace {

constexpr int kRing = 512;
static_assert(pianoboard::kPatches <= 64, "more patches than Piano::kMaxPatches");

// The wound bass strings' speaking lengths (m), keys 1 .. 20 (the plain
// strings' lengths follow from their stiffness, below).
double bassLength(int key) { return 2.01 * std::pow(1.33 / 2.01, (key - 1) / 19.0); }

// The strings' stiffness B (partial n at n f0 sqrt(1 + B n^2)) as measured
// on two recorded concert grands (research/piano.md, 11): doubling every
// 8.2 keys from the lowest plain string up; the wound bass least near key 13.
double stiffness(int key) {
  if (key > 20) return 7.5e-5 * std::exp2((key - 24) / 8.2);
  const double a = key <= 13 ? (key - 1) / 12.0 : 1 + (key - 13) / 7.0;
  return key <= 13 ? 8.0e-5 * std::pow(4.5e-5 / 8.0e-5, a) : 4.5e-5 * std::pow(5.5e-5 / 4.5e-5, a - 1);
}

double lerp(double a, double b, double t) { return a + (b - a) * std::clamp(t, 0.0, 1.0); }

}  // namespace

PianoDesign grandDesign() {
  PianoDesign des;
  PianoKey* sc = des.key;
  static const double kE = 2.02e11, kRho = 7850;
  const double dampRate = knob("PJ_PIANO_DAMPER", 1.0);
  for (int key = 1; key <= 88; ++key) {
    PianoKey& s = sc[key - 1];
    s.strings = key <= 8 ? 1 : key <= 13 ? 2 : 3;
    const double f = 440 * std::exp2((key - 49) / 12.0);
    s.B = stiffness(key);
    if (key <= 20) {  // copper-wound: the tension chosen, the winding makes up the mass, the core the stiffness
      s.len = bassLength(key);
      s.tension = 1300 - 15.0 * (key - 1);
      s.mu = s.tension / ((2 * s.len * f) * (2 * s.len * f));
    } else {  // plain steel, 1.2 mm thinning to 0.775 mm: as long as its stiffness says (1.90 m .. 53 mm)
      const double d = lerp(1.2e-3, 0.775e-3, (key - 21) / 67.0);
      s.mu = kRho * M_PI * d * d / 4;
      s.len = std::pow(M_PI * M_PI * M_PI * kE * d * d * d * d / (256 * s.mu * f * f * s.B), 0.25);
      s.tension = s.mu * (2 * s.len * f) * (2 * s.len * f);
    }
    s.strike = key <= 52 ? 0.122 : lerp(0.122, 0.07, (key - 52) / 36.0);
    // The hammers: 10.5 g in the bass to 4.5 g at the top. The felt's
    // hardening (the exponent) steeper towards the treble (Chaigne &
    // Askenfelt's C2, C4, C7); its stiffness theirs in the bass, and from
    // the middle up as voiced on two recorded grands: three to ten times
    // softer than their C4 and C7, by how little of a note's sound those
    // pianos have above its second partial (research/piano.md, 11).
    s.mh = 1e-3 * std::exp(lerp(std::log(10.5), std::log(4.5), (key - 1) / 87.0));
    static const double pk[] = {1, 16, 40, 76, 88}, hp[] = {2.2, 2.3, 2.5, 3.0, 3.1};
    static const double hk[] = {1, 16, 30, 49, 64, 76, 88};
    static const double hK[] = {1.5e8, 4.0e8, 1.3e9, 1.7e9, 2.5e10, 1.0e12, 3.0e12};
    for (int i = 0; i < 4; ++i)
      if (key <= pk[i + 1]) {
        s.p = lerp(hp[i], hp[i + 1], (key - pk[i]) / (pk[i + 1] - pk[i]));
        break;
      }
    for (int i = 0; i < 6; ++i)
      if (key <= hk[i + 1]) {
        s.K = std::exp(lerp(std::log(hK[i]), std::log(hK[i + 1]), (key - hk[i]) / (hk[i + 1] - hk[i])));
        break;
      }
    // A hammer is soft at its surface and hard underneath: the felt
    // hardens far more steeply than the measured power laws say when it is
    // pressed deep. As soft as fitted at a gentle blow's force (kPivot newtons
    // a string), the exponent raised from there: a middling blow meets felt
    // twice as stiff, a hard one five times, and a loud note has its upper
    // partials (with the fitted softness at every force it kept only the
    // first two: hollow, like a clarinet). (Applied when the piano is built.)
    // (Less of it in the upper register: there a hard blow on felt that hard
    // turned glassy, three partials level with the fundamental, where the
    // recorded grands keep the fundamental well over the rest at any blow.)
    s.colour = lerp(knob("PJ_PIANO_COLOUR", 2.0), knob("PJ_PIANO_COLOURTOP", 0.8), (key - 40) / 24.0);
    // The top octave and a half: soft again at a gentle blow (hard felt on a light hammer
    // gave even the softest notes there a clear second partial).
    s.colour += knob("PJ_PIANO_COLOURHIGH", 2.0) * std::clamp((key - 68) / 8.0, 0.0, 1.0);
    s.K *= std::pow(knob("PJ_PIANO_KHIGH", 0.3), std::clamp((key - 68) / 8.0, 0.0, 1.0));
    s.pivot = knob("PJ_PIANO_PIVOT", 2.0);
    {
      // The nap: thinner and firmer up the keyboard, as the hammers are lighter and the notes' periods shorter.
      const double oct = (key - 40) / 12.0;
      s.nap = knob("PJ_PIANO_NAP", 0.9e-3) * std::exp2(-knob("PJ_PIANO_NAPTHIN", 2.0) * std::max(oct, 0.0));
      s.napK = knob("PJ_PIANO_NAPK", 1200) * std::exp2(oct * (oct > 0 ? knob("PJ_PIANO_NAPSLOPE", 2.5) : knob("PJ_PIANO_NAPSLOPEBASS", 0.8)));
    }
    s.loss = knob("PJ_PIANO_LOSS", 0.05) * std::exp2(3.5 * (key - 1) / 87.0);
    // The loss's rise with frequency (1/s per Hz): the wire's own Q; the wound bass strings lose more.
    s.b2 = lerp(knob("PJ_PIANO_LOSS2BASS", 3.5e-4), knob("PJ_PIANO_LOSS2", 2e-4), (key - 16) / 24.0);
    s.b3 = knob("PJ_PIANO_LOSS3", 3e-8);    // and with frequency^2 (1/s per Hz^2)
    s.damper = s.strike + 0.035;
    s.damperLen = lerp(0.07, 0.025, (key - 1) / 69.0);
    // The damper: a length of felt; a mode loses by its mean square motion under it.
    // (A released note is gone in about 0.15 s in the middle, 0.3 s in the bass: from memory, not
    // measured. A key struck again sooner meets what is left of its own sound.) The top 18 keys have none.
    s.hasDamper = key <= 70;
    s.damperRate = dampRate * lerp(16, 38, (key - 1) / 69.0) / 0.2;
    // What lands: the head (felt on a wooden block, double wedges in the bass), its wire and the
    // underlever's share at the head, ~20 g in the bass to ~12 g at the last damper (the heads
    // grow lighter up the keyboard; the task's and the technicians' 10-20 g).
    s.damperMass = lerp(0.020, 0.012, (key - 1) / 69.0);
  }
  des.pedalNoise = knob("PJ_PIANO_PEDALNOISE", 1.0);
  des.longShare = knob("PJ_PIANO_LONG", 0.7);  // (the stretching's push on the bridge: its share moving the board at 1 kHz)
  des.damperContact = knob("PJ_PIANO_DAMPERCONTACT", des.damperContact);  // (the felt's cushion, s)
  des.rimShare = knob("PJ_PIANO_RIMSHARE", des.rimShare);                // (a knock in the case reaching the board)
  tuneByPartials(&des);
  const double mistune = knob("PJ_PIANO_MISTUNE", 1.0);  // the unisons' spread (x the usual)
  des.unisonCents[0] = mistune * 0.08;
  des.unisonCents[1] = mistune * 0.03;
  // (How far off the vertical the blow sets the string swinging: the
  // hammer's face, the string's seat on the bridge's pins.)
  des.tilt = knob("PJ_PIANO_TILT", 0.4);
  des.sideTuneCents = knob("PJ_PIANO_SIDETUNE", 1.0);
  des.sideLoad = knob("PJ_PIANO_SIDE", 0.4);  // the bridge's share of the sideways swing
  des.top = knob("PJ_PIANO_TOP", 10000);
  des.topTreble = knob("PJ_PIANO_TOPTREBLE", 20000);
  des.hammerWidth = knob("PJ_PIANO_HAMW", 0.010);
  des.board.n = pianoboard::kModes;
  des.board.patches = pianoboard::kPatches;
  des.board.freq = pianoboard::kFreq;
  des.board.loss = pianoboard::kLoss;
  des.board.phi = pianoboard::kPhi;
  des.board.patchVol = pianoboard::kPatchVol;
  des.board.patchPos = pianoboard::kPatchPos;
  des.boardLoss = knob("PJ_PIANO_ETA", 1.0);  // the board's own loss (x the wood's and the air's)
  des.stringLoad = knob("PJ_PIANO_STRINGLOAD", 1.0);
  // The two microphones: pressure-gradient ("differential", figure-of-eight)
  // ones, close together over the strings 50 cm from the keyboard's end of
  // the board (nearer, the short treble strings' end of the board is heard as
  // evenly as the rest) and 65 cm above it, one turned 45 degrees towards the bass
  // side, the other towards the treble side (a crossed pair). Each hears the
  // difference of the pressure across itself: a patch of the board by the
  // cosine of its angle off the microphone's axis, behind 90 degrees with
  // its sign turned. A partial sits in the picture where the board moves
  // with it; the two together are one such microphone looking straight down
  // (no gaps from two distances).
  const double mx = knob("PJ_PIANO_MICX", 0.5), mz = knob("PJ_PIANO_MICZ", 0.65);
  const double myc = knob("PJ_PIANO_MICY", 0.70), space = knob("PJ_PIANO_MICSPACE", 0.17);
  const double ang = knob("PJ_PIANO_MICANGLE", 60), pat = knob("PJ_PIANO_MICPATTERN", 1.0);
  for (int c = 0; c < 2; ++c) {
    const double sgn = c ? 1 : -1;
    des.mic[c] = {mx, myc + sgn * space / 2, mz, sgn * ang, pat};
  }
  return des;
}

void tuneByPartials(PianoDesign* d, int lo, int hi) {
  PianoKey* sc = d->key;
  const int keys = d->keys;
  // Tuned as a tuner does: an octave in the middle by the fundamental, then
  // octave by octave, a partial of the lower note against one of the upper
  // (2 with 1 going up, 6 with 3 going down, as the recorded grands sit):
  // the stiff wires' sharp partials stretch the octaves.
  auto part = [&](int key, int n) { return n * std::sqrt((1 + sc[key - 1].B * n * n) / (1 + sc[key - 1].B)); };
  for (int key = lo; key <= hi; ++key) sc[key - 1].f1 = 440 * std::exp2((key - 49) / 12.0);
  for (int key = hi + 1; key <= keys; ++key) sc[key - 1].f1 = sc[key - 13].f1 * part(key - 12, 2);
  for (int key = lo - 1; key >= 1; --key) sc[key - 1].f1 = sc[key + 11].f1 * part(key + 12, 3) / part(key, 6);
}

Piano::Piano(double fs, const PianoDesign& design) : d_(design), fs_(fs), dt_(float(1 / fs)) { build(); }

Piano::~Piano() { std::free(mem_); }

void Piano::build() {
  nKeys_ = std::min(d_.keys, int(kKeys));
  const bool board = d_.bridge == PianoDesign::Bridge::kBoard && d_.board.n > 0;
  nB_ = board ? d_.board.n : 0;
  nP_ = board ? std::min(d_.board.patches, int(kMaxPatches)) : 0;
  pickup_ = d_.output == PianoDesign::Output::kPickup || !board;
  kSide_ = float(d_.sideLoad);
  pickSide_ = float(d_.pickupSide);
  rng_ = d_.seed;
  const int kB = nB_, kP = nP_, kKeys = nKeys_;
  // The felt, hardening more steeply from a gentle blow's force up.
  PianoKey sc[PianoDesign::kMaxKeys];
  for (int k = 0; k < kKeys; ++k) {
    sc[k] = d_.key[k];
    if (sc[k].colour != 0) {
      const double dRef = std::pow(sc[k].pivot / sc[k].K, 1 / sc[k].p);  // the felt's compression at that force (m)
      sc[k].K *= std::pow(dRef, -sc[k].colour);
      sc[k].p += sc[k].colour;
    }
  }
  // How many modes each string has: in whole registers of 16.
  const double top = d_.top, topTreble = std::min(0.45 * fs_, d_.topTreble);
  int count[PianoDesign::kMaxKeys];
  total_ = 0;
  str_.clear();
  for (int k = 0; k < kKeys; ++k) {
    const PianoKey& s = sc[k];
    const double fmax = s.top > 0 ? std::min(0.45 * fs_, s.top) : lerp(top, topTreble, (k + 1 - 40) / 12.0);
    const double f0 = s.f1 / std::sqrt(1 + s.B);
    int n = 0;
    while (n < 176 && (n + 1) * f0 * std::sqrt(1 + s.B * (n + 1) * (n + 1)) < fmax) ++n;
    count[k] = (std::max(n, 1) + 15) / 16 * 16;
    key_[k].s0 = int(str_.size());
    key_[k].ns = s.strings;
    key_[k].f1 = s.f1;
    key_[k].B = s.B;
    key_[k].len = s.len;
    key_[k].mh = float(s.mh);
    key_[k].K = float(s.K);
    key_[k].p = float(s.p);
    key_[k].nap = float(s.nap);
    key_[k].napK = float(s.napK);
    key_[k].pluckK = float(s.pluckK);
    key_[k].pluckLimit = float(s.pluckLimit);
    key_[k].pluckSide = float(s.pluckSide);
    key_[k].hasDamper = s.hasDamper;
    key_[k].damperDown = key_[k].hasDamper;
    key_[k].engage = key_[k].hasDamper ? 1 : 0;
    for (int j = 0; j < s.strings; ++j) {
      Str st;
      st.key = k;
      st.off = total_;
      st.n = count[k];
      rng_ = rng_ * 1664525u + 1013904223u;
      const double r = (rng_ >> 8) / 16777216.0;
      st.tanH = float((j == 1 ? -1 : 1) * d_.tilt * (0.55 + 0.9 * r));
      total_ += st.n;
      str_.push_back(st);
    }
  }
  const size_t words = size_t(total_) * 16 + size_t(kB) * 12 + size_t(kB) * (kKeys + kP) + size_t(kP) * (kRing + 16) + 64;
  mem_ = static_cast<float*>(std::aligned_alloc(64, (words * sizeof(float) + 63) / 64 * 64));
  std::memset(mem_, 0, words * sizeof(float));
  float* p = mem_;
  auto take = [&](size_t n) { float* q = p; p += (n + 15) / 16 * 16; return q; };
  x_[0] = take(total_); x_[1] = take(total_); v_[0] = take(total_); v_[1] = take(total_);
  cs_ = take(total_); sn_ = take(total_); cs1_ = take(total_); sn1_ = take(total_); in_ = take(total_); out_ = take(total_);
  damp_ = take(total_); hk_ = take(total_); hc_ = take(total_); hs_ = take(total_); dk_ = take(total_); nw_ = take(total_);
  bx_ = take(kB); bv_ = take(kB); bcs_ = take(kB); bsn_ = take(kB); bf_ = take(kB); ba_ = take(kB); binv_ = take(kB);
  bacc_ = take(kB);
  phi_ = take(size_t(kB) * kKeys);
  pvol_ = take(size_t(kB) * kP);
  ring_ = take(size_t(kP) * kRing);

  const double hamW = d_.hammerWidth;
  uint32_t rng2 = d_.seed * 2654435761u + 1;  // (the out-of-tune piano's own draws: the others stay as they are)
  auto draw2 = [&]() { rng2 = rng2 * 1664525u + 1013904223u; return (rng2 >> 8) / 16777216.0; };
  if (d_.tackHard > 0) {
    // (the tacks: each key's hammer meets one or two of its strings with a metal head, the rest
    // with softer felt around it; its face uneven, each string met at its own moment; its own draws)
    uint32_t rng3 = d_.seed * 40503u + 7;
    auto draw3 = [&]() { rng3 = rng3 * 1664525u + 1013904223u; return (rng3 >> 8) / 16777216.0; };
    for (int k = 0; k < keys(); ++k) {
      const int ns = key_[k].ns;
      const int hit = int(draw3() * ns), two = ns > 2 && draw3() < 0.4 ? (hit + 1 + int(draw3() * (ns - 1))) % ns : -1;
      const double key = 0.5 + draw3();  // (how well this key's tacks sit)
      for (int j = 0; j < ns; ++j) {
        Str& st = str_[key_[k].s0 + j];
        const bool tacked = j == hit || j == two;
        st.tack = float(tacked ? d_.tackHard * key * (0.5 + draw3()) : 0);
        st.hard = float(0.75 + 0.5 * draw3());  // (the felt around the tacks, uneven)
        st.gap = float(d_.tackGap * draw3());
      }
    }
  }
  for (Str& st : str_) {
    const PianoKey& s = sc[st.key];
    const double b2 = s.b2, b3 = s.b3;
    const int j = int(&st - &str_[key_[st.key].s0]);
    // The unison: the strings of a key a little apart (cents): the middle
    // one true, the outer ones either side, never quite evenly.
    rng_ = rng_ * 1664525u + 1013904223u;
    const double r = (rng_ >> 8) / 16777216.0;
    const double spread = lerp(d_.unisonCents[0], d_.unisonCents[1], st.key / 87.0) * (0.7 + 0.6 * r);
    double cents = s.strings == 1 ? 0 : s.strings == 2 ? (j == 0 ? -0.5 : 0.5) * spread : (j - 1) * spread;
    // (an out-of-tune piano: a trichord's outer strings either side of its true middle one, by a
    // spread growing from the bass (x 0.67) to the treble (x 1.33), each key x 0.6 .. 1.4; the
    // wound bichords clean)
    if (d_.honkyCents > 0 && s.strings == 3 && j != 1) {
      const double reg = 0.67 + 0.66 * std::clamp((st.key - 20) / 60.0, 0.0, 1.0);
      cents += (j == 0 ? -1 : 1) * d_.honkyCents * reg * (0.6 + 0.8 * draw2());
    }
    // (or by hertz, every string of a key its own: the outer ones about honkyHz either side, the
    // middle one of three a little; the beats about as fast all over the keyboard, capped at 20 c
    // (PJ_HONKY_CAP: 1.5 Hz is 40 c at C2, sour))
    if (d_.honkyHz > 0 && s.strings > 1) {
      const double side = s.strings == 2 ? (j == 0 ? -1 : 1) : j - 1.0;
      const double hz = (side != 0 ? side * (0.6 + 0.8 * draw2()) : 0.4 * (draw2() - 0.5) * 2 * 0.5) * d_.honkyHz;
      static const double kCap = knob("PJ_HONKY_CAP", 20.0);
      cents += std::clamp(1200 * std::log2(1 + hz / s.f1), -kCap, kCap);
    }
    // (the treble's strings drift flat too, each its own: uniform 0 .. -1 Hz above middle C,
    // 0 .. -2 Hz above C5; PJ_HONKY_TREBLEFLAT scales it)
    if (d_.honkyHz > 0 && st.key + 21 > 60) {
      static const double kTf = knob("PJ_HONKY_TREBLEFLAT", 1.0);
      const double hz = (st.key + 21 > 72 ? 2.0 : 1.0) * kTf * draw2();
      cents += 1200 * std::log2(1 - hz / s.f1);
    }
    if (d_.flatShare > 0 && draw2() < d_.flatShare) {
      // (a string gone flat: by up to flatMax cents, or by about flatHz hertz, x 0.7 .. 1.3,
      // capped at 40 c)
      if (d_.flatHz > 0) cents -= std::min(40.0, -1200 * std::log2(1 - std::min(0.5, d_.flatHz * (0.7 + 0.6 * draw2()) / s.f1)));
      else cents -= d_.flatMax * draw2();
    }
    // (A support that does not move takes from every mode alike.)
    const double support = nB_ ? 0.0 : s.tension / (s.len * std::max(d_.supportR, 1.0));
    const double supportFc = d_.supportFc;
    const double f0 = s.f1 / std::sqrt(1 + s.B) * std::exp2(cents / 1200);
    const double modalMass = s.mu * s.len / 2;
    // Sideways the string is tuned a little apart from up-and-down (the
    // bridge gives differently the two ways): a cent or so, either way.
    // Without that the two swings, pushing one bridge at one pitch, would
    // settle at once into the one mix that moves it and the one that does
    // not (silent): no aftersound, a tone as steady as a wind instrument's.
    // Apart, the sideways swing is heard by itself: weaker, far longer, and
    // drifting against the other.
    const double kSideTune = d_.sideTuneCents;
    rng_ = rng_ * 1664525u + 1013904223u;
    const double rs = (rng_ >> 8) / 16777216.0;
    const double sideTune = std::exp2((rs < 0.5 ? -1 : 1) * kSideTune * (0.5 + std::fmod(rs * 2, 1.0)) / 1200);
    const double rate0 = s.damperRate;
    double comply = 0;
    for (int i = 0; i < st.n; ++i) {
      const int n = i + 1, m = st.off + i;
      const double f = n * f0 * std::sqrt(1 + s.B * n * n);
      if (f > 0.47 * fs_) continue;  // (left at rest: all its numbers 0)
      st.live = i + 1;
      const double w = 2 * M_PI * f, wdt = w / fs_;
      double sigma = s.loss + b2 * f + b3 * f * f + (supportFc > 0 ? support / (1 + (supportFc / f) * (supportFc / f)) : support);
      // The top two and a half octaves (from F#5, fully from C6): the second partial dies away
      // faster (its own loss x PJ_PIANO_TOPH2, 1.6: with the board's share ~35 % faster overall),
      // so the tone settles onto the fundamental; the partials above it
      // optionally too (x PJ_PIANO_TOPHI).
      {
        static const double kTopH2 = knob("PJ_PIANO_TOPH2", 1.6), kTopHi = knob("PJ_PIANO_TOPHI", 1.0);
        const double top = std::clamp((st.key + 21 - 78) / 6.0, 0.0, 1.0);  // (key 0 is A0, MIDI 21)
        if (n == 2) sigma *= 1 + (kTopH2 - 1) * top;
        else if (n > 2) sigma *= 1 + (kTopHi - 1) * top;
      }
      const double g = std::exp(-sigma / fs_);
      cs_[m] = float(g * std::cos(wdt));
      sn_[m] = float(g * std::sin(wdt));
      cs1_[m] = float(g * std::cos(wdt * sideTune));
      sn1_[m] = float(g * std::sin(wdt * sideTune));
      const double sign = n % 2 ? 1 : -1;  // (-1)^(n+1)
      in_[m] = float(-2 * sign / (n * M_PI) * w);
      nw_[m] = float(n / w);
      out_[m] = float(s.mu * s.len * sign / (n * M_PI));
      const double a = n * M_PI * s.strike, hw = n * M_PI * hamW / (2 * s.len);
      const double shape = std::sin(a) * (hw > 1e-6 ? std::sin(hw) / hw : 1.0);
      hk_[m] = float(shape / modalMass / fs_);
      hc_[m] = float(shape * cs_[m] / w);
      hs_[m] = float(shape * sn_[m] / w);
      comply += hk_[m] * hs_[m];
      // (The mean of sin^2 over the felt's length.)
      const double c0 = n * M_PI * s.damper, hl = n * M_PI * s.damperLen / (2 * s.len);
      const double mean = 0.5 * (1 - std::cos(2 * c0) * (hl > 1e-6 ? std::sin(2 * hl) / (2 * hl) : 1.0));
      damp_[m] = float(std::exp(-rate0 * mean / fs_));
      // (The felt's push, spread over its length: the mean of sin there.)
      dk_[m] = float(std::sin(c0) * (hl > 1e-6 ? std::sin(hl) / hl : 1.0) / modalMass / fs_);
    }
    st.comply = float(comply);
    // Its lengthwise modes. The wire's core carries the stretch (a wound string's copper winding
    // only weighs): its diameter from the stiffness, B = pi^3 E d^4 / 64 T L^2, the string's
    // whole mass moving with it: the first at c / 2 L, c = sqrt(E A / mu) (A0: a 1.44 mm core,
    // 440 Hz, 16 x its fundamental; the lowest plain string, 1.9 m: 1.3 kHz, 15 x; Conklin's
    // and Bank's ~16-20 x). Driven by the strain's square, E A / 2 (dy/dx)^2: mode k by the
    // partials' pairs k apart (i, i + k; their sum, f_i + f_(i+k), is what it hears: at a
    // resonance, the "phantom" partials ring out) and those adding up to k; its push on the
    // bridge E A du/dx there (Bank & Sujbert, JASA 117, 2005). It grows with the square of the
    // blow and dies with the product of two partials' decays: the loud bass note's short
    // metallic ring over the first half second.
    st.nl = 0;
    if (d_.longShare > 0 && s.B > 0) {
      static constexpr double kE = 2.02e11;
      const double d4 = 64 * s.B * s.tension * s.len * s.len / (M_PI * M_PI * M_PI * kE);
      const double EA = kE * M_PI * std::sqrt(d4) / 4, cL2 = EA / s.mu, fL = std::sqrt(cL2) / (2 * s.len);
      // (The push along the string, at the bridge's top some 3 cm over the board, is a moment on
      // it: it moves the board as a force of (height x the board's bending wavenumber) of it
      // would, ~0.7 at 1 kHz for the ~7 mm spruce under the bass bridge (k = (w^2 rho h /
      // D)^(1/4), 24 /m; 21 /m at 9 mm), as sqrt(f). An estimate, not a measurement: Conklin
      // found the bridge as mobile along the strings as across them.)
      auto share = [&](double f) { return d_.longShare * std::sqrt(f / 1000); };
      if (fL < d_.longTop) {
        // (The transverse modes under 5 kHz and the lengthwise ones under it: above, the phantoms
        // are far under the partials, and the cost is in these products.)
        st.nlx = 0;
        while (st.nlx < st.n && (st.nlx + 1) * f0 * std::sqrt(1 + s.B * (st.nlx + 1) * (st.nlx + 1)) < 5000) ++st.nlx;
        for (int k = 1; k <= Str::kLong && k * fL < std::min(5000.0, 0.45 * fs_); ++k) {
          const double w = 2 * M_PI * k * fL, g = std::exp(-w / (2 * d_.longQ) / fs_);
          const int i = st.nl++;
          st.lcs[i] = float(g * std::cos(w / fs_));
          st.lsn[i] = float(g * std::sin(w / fs_));
          st.ldrive[i] = float(-cL2 / 2 * k * std::pow(M_PI / s.len, 3) / fs_);
          st.lout[i] = float(share(k * fL) * EA * (k % 2 ? -1 : 1) * k * M_PI / s.len / w / fs_);
          st.lrest[i] = float(share(k * fL) * EA * (k % 2 ? -1 : 1) * (M_PI / s.len) * (M_PI / s.len) / 2 / fs_);
        }
        // (the even tension: mostly at twice the fundamental)
        st.ltension = float(share(2 * f0) * EA * M_PI * M_PI / (4 * s.len * s.len) / fs_);
      }
    }
  }
  // The soundboard, and what the damped strings standing on its bridges do
  // to it. A string under its damper is no resonator but a load: from the
  // bridge it looks like a line that swallows most of what is sent down it
  // (impedance -i Zc cot(k L), the damper's loss in k): a resistance near and
  // above its own pitch, a spring (its tension over its length) below. Each
  // resonance of the board is damped and raised by the 200 of them by what
  // it moves at their bridge points; without them the lowest resonances are
  // so sharp that a key whose fundamental meets one loses it at once (a wolf
  // no grand has). A lifted damper takes its key's load away: the strings
  // are then themselves, computed.
  if (kB) std::memcpy(phi_, d_.board.phi, sizeof(float) * kB * kKeys);
  loadR_.assign(size_t(kKeys) * kB, 0.0f);
  loadX_.assign(size_t(kKeys) * kB, 0.0f);
  const double kSideLoad = d_.sideLoad;
  const double loadScale = d_.stringLoad;
  for (int k = 0; k < kKeys; ++k) {
    if (!key_[k].hasDamper) continue;
    const PianoKey& s = sc[k];
    const double zc = std::sqrt(s.tension * s.mu), f0 = s.f1;
    const double rate0 = s.damperRate, b2 = s.b2;
    for (int m = 0; m < kB; ++m) {
      const double f = d_.board.freq[m], n = f / f0;
      const double c0 = n * M_PI * s.damper, hl = n * M_PI * s.damperLen / (2 * s.len);
      const double sigma = s.loss + b2 * f + rate0 * 0.5 * (1 - std::cos(2 * c0) * (hl > 1e-6 ? std::sin(2 * hl) / (2 * hl) : 1.0));
      const std::complex<double> arg(M_PI * n, -sigma / (2 * f0));
      const std::complex<double> z = std::complex<double>(0, -1) * zc * std::cos(arg) / std::sin(arg);
      const double ph = phi_[k * kB + m], c = loadScale * s.strings * (1 + kSideLoad * kSideLoad) * ph * ph;
      loadR_[size_t(k) * kB + m] = float(c * z.real());
      loadX_[size_t(k) * kB + m] = float(c * z.imag());
    }
  }
  boardNow();
  // The trapwork's knock enters the board through the rim, along its edge: in this board's
  // resonances, at the two ends of the bridges, which lie nearest it (the bass bridge's end by
  // the bentside, the long bridge's treble end by the belly rail and the cheek, where the lift
  // rail's stop is).
  noise_ = float(std::max(0.0, d_.pedalNoise));
  trapPhi_.assign(kB, 0.0f);
  for (int m = 0; m < kB; ++m) trapPhi_[m] = 0.5f * (phi_[m] + phi_[size_t(kKeys - 1) * kB + m]);
  {
    // Each damper's regulation: how much of the lift it still has to fall when the others touch
    // (0 .. damperSpread m), fixed for the instrument; its own draws.
    uint32_t r = d_.seed * 2246822519u + 3;
    for (int k = 0; k < kKeys; ++k) {
      r = r * 1664525u + 1013904223u;
      key_[k].landDelay = float(d_.damperSpread * ((r >> 8) / 16777216.0));
    }
    noiseRng_ = d_.seed * 3266489917u + 11;
  }
  // (The board's data: [patch][resonance], this board's count of each.)
  for (int p = 0; p < kP; ++p)
    std::memcpy(pvol_ + size_t(p) * kB, d_.board.patchVol + size_t(p) * d_.board.n, sizeof(float) * kB);
  // The microphones. Each patch is a small source in a wall: its sound
  // arrives after its distance, weaker by it (rho / 2 pi r times the air's
  // acceleration), and by the microphone's pattern at its angle.
  for (int c = 0; c < 2; ++c) {
    const PianoDesign::Mic& mic = d_.mic[c];
    const double mx = mic.x, my = mic.y, mz = mic.z, ang = mic.angleDeg * M_PI / 180, pat = mic.pattern;
    const double ay = std::sin(ang), az = -std::cos(ang);  // the axis: down and to its side
    for (int p = 0; p < kP; ++p) {
      const double dx = d_.board.patchPos[2 * p] - mx, dy = d_.board.patchPos[2 * p + 1] - my;
      const double r = std::sqrt(dx * dx + dy * dy + mz * mz);
      const double d = std::min(r / 345.0 * fs_, double(kRing - 2));
      const double cosine = (dy * ay - mz * az) / r;
      tapI_[c][p] = int(d);
      tapF_[c][p] = float(d - int(d));
      tapG_[c][p] = float(1.18 / (2 * M_PI * r) * ((1 - pat) + pat * cosine));
    }
  }
  reset();
}

// The board's resonances under the strings that now rest damped on it.
void Piano::boardNow() {
  const double etaScale = d_.boardLoss;
  const int kB = nB_, kKeys = nKeys_;
  std::vector<double> r(kB, 0.0), x(kB, 0.0);
  for (int k = 0; k < kKeys; ++k) {
    if (!key_[k].hasDamper || !key_[k].damperDown) continue;
    const float *lr = &loadR_[size_t(k) * kB], *lx = &loadX_[size_t(k) * kB];
    for (int m = 0; m < kB; ++m) { r[m] += lr[m]; x[m] += lx[m]; }
  }
  for (int m = 0; m < kB; ++m) {
    const double w0 = 2 * M_PI * d_.board.freq[m];
    // (velocity * (R + iX) opposes the motion: R damps, -w X adds to the stiffness.)
    const double w = std::sqrt(std::clamp(w0 * w0 - w0 * x[m], 0.5 * w0 * w0, 2.0 * w0 * w0)), wdt = w / fs_;
    const double rate = 0.5 * (etaScale * d_.board.loss[m] * w + std::max(r[m], 0.0));
    const double g = std::exp(-std::min(rate, 0.35 * w) / fs_);
    bcs_[m] = float(g * std::cos(wdt));
    bsn_[m] = float(g * std::sin(wdt));
    binv_[m] = float(1 / w);
  }
  boardStale_ = false;
}

void Piano::reset() {
  const int kB = nB_, kP = nP_;
  for (int d = 0; d < 2; ++d) {
    std::memset(x_[d], 0, sizeof(float) * total_);
    std::memset(v_[d], 0, sizeof(float) * total_);
  }
  std::memset(bx_, 0, sizeof(float) * kB);
  std::memset(bv_, 0, sizeof(float) * kB);
  std::memset(ba_, 0, sizeof(float) * kB);
  for (Key& k : key_) {
    k.hammerOn = false;
    k.acc = k.force = 0;
    k.landWait = k.landLeft = 0;
  }
  landing_ = knocks_ = 0;
  for (Knock& q : knock_) q.left = 0;
  for (Str& s : str_) {
    s.active = !key_[s.key].damperDown;
    s.dPrev = 0;
    s.longLeft = 0;
    for (int i = 0; i < Str::kLong; ++i) s.lx[i] = s.lv[i] = 0;
  }
  std::memset(ring_, 0, sizeof(float) * kP * kRing);
  side_ = 0;
  asleep_ = true;
  census();
}

void Piano::census() {
  const int kKeys = nKeys_;
  nActive_ = 0;
  for (int k = 0; k < kKeys; ++k) {
    bool any = false;
    for (int j = 0; j < key_[k].ns; ++j) any = any || str_[key_[k].s0 + j].active;
    key_[k].active = any;
    if (any) activeKeys_[nActive_++] = k;
    else key_[k].acc = 0;
  }
}

int Piano::activeStrings() const {
  int n = 0;
  for (const Str& s : str_) n += s.active;
  return asleep_ ? 0 : n;
}

void Piano::wake() { asleep_ = false; }

void Piano::strike(int key, double speed) {
  if (key < 0 || key >= nKeys_) return;
  Key& k = key_[key];
  // The hammer starts just under the lowest of its strings.
  float lowest = 0;
  for (int j = 0; j < k.ns; ++j) {
    Str& s = str_[k.s0 + j];
    float y = 0;
    for (int i = 0; i < s.n; ++i) y += hc_[s.off + i] * x_[0][s.off + i] + hs_[s.off + i] * v_[0][s.off + i];
    lowest = std::min(lowest, y);
    s.active = true;
    s.dPrev = 0;
    if (s.nl > 0) {
      s.longLeft = int(d_.longTime * fs_);
      s.longPeak = 0;
    }
  }
  k.eta = k.pluckK > 0 ? lowest : lowest - 1e-4f;  // (a plectrum starts touching its string)
  k.vh = float(speed);
  k.hammerOn = true;
  wake();
  census();
}

void Piano::damper(int key, bool down, double speed) {
  if (key < 0 || key >= nKeys_) return;
  Key& k = key_[key];
  if (!k.hasDamper || k.damperDown == down) return;
  k.damperDown = down;
  boardStale_ = true;
  if (!down) {
    k.engage = 0;
    if (k.landWait > 0 || k.landLeft > 0) --landing_;  // (lifted again before it landed)
    k.landWait = k.landLeft = 0;
    for (int j = 0; j < k.ns; ++j) str_[k.s0 + j].active = true;
    census();
  } else if (noise_ > 0 && d_.key[key].damperMass > 0) {
    // The damper falls onto its strings (research/piano.md 6). Its momentum m v is taken up by
    // the felt (a cushion: the head does not bounce, it stays on the strings) in a half sine of
    // the contact time, 8 ms (the soft damper felt a spring of ~2.5 kN/m under a 16 g head:
    // pi sqrt(m / k)): a peak force of pi m v / 2 T, shared by the key's strings; under ~150 Hz
    // mostly, the strings' low partials and the board's lowest resonances: a thump, no click. A key let go
    // lets its damper fall at about 0.12 m/s (the key rising under the hammer's and the
    // damper's weight: 4-6 mm of fall in some 40 ms), each time a little its own (x 0.75 .. 1.25).
    // It touches after its regulation: the share of the lift it still has to fall when the first
    // ones touch, over its speed (0 .. ~4 ms at 0.12 m/s: the landings of a pedal's release
    // smeared into a "chuff", not one click).
    if (speed < 0) {
      noiseRng_ = noiseRng_ * 1664525u + 1013904223u;
      speed = d_.damperFall * (0.75 + 0.5 * ((noiseRng_ >> 8) / 16777216.0));
    }
    const double T = d_.damperContact;
    k.landN = std::max(2, int(T * fs_ + 0.5));
    k.landLeft = k.landN;
    k.landWait = int(k.landDelay / std::max(speed, 0.02) * fs_);
    k.landAmp = float(noise_ * d_.damperSide * M_PI * d_.key[key].damperMass * speed / (2 * T) / k.ns);
    ++landing_;
    for (int j = 0; j < k.ns; ++j) str_[k.s0 + j].active = true;
    census();
    wake();
  }
}

// The lift rail and the pedal's lever: released, they fall with the dampers and land on their
// felt stop (the rail's ~0.6 kg with the rod and the lever's share: a half sine of 8 ms, felt on
// wood, its force's energy under ~200 Hz), the case takes the knock and half of it reaches the
// board through the rim (rimShare, an estimate: the thud then sits ~15 dB under the dampers'
// own landing, the strings' "chuff" first, a dull thud under it). Pressed, the
// rail meets the underlevers and lifts them off the strings: a softer knock (railPress of it);
// the dampers leaving the strings give up only their weight's rest (some 0.15 N each, its step
// under 1/10 of a landing's push at 100 Hz), left out.
void Piano::pedal(bool down, double speed) {
  if (noise_ <= 0 || speed <= 0) return;
  knock(noise_ * (down ? d_.railPress : 1.0) * d_.railMass * speed, d_.railContact);
}

// A knock in the case (impulse N s, a half sine of T s), the rim's share of it into the board.
void Piano::knock(double impulse, double T) {
  if (!nB_ || impulse <= 0) return;
  Knock* q = &knock_[0];
  for (Knock& k : knock_)
    if (k.left < q->left) q = &k;  // (a free one, or the nearest done)
  if (q->left == 0) ++knocks_;
  q->n = std::max(2, int(T * fs_ + 0.5));
  q->left = q->n;
  q->amp = float(d_.rimShare * M_PI * impulse / (2 * T));
  wake();
}

// The string's stretch this sample: the strain's square in its lengthwise modes' terms (both
// directions of swing stretch it), their step, and their push on the bridge (x dt). Slowly
// (far under the lengthwise modes) the tension is even along the string, E A / 2L times the
// integral of the slope's square: the partials' squares only, at twice their frequencies (and
// the pitch's rise at a loud blow, not followed here); what the modes do beyond that, near
// their own frequencies, is their motion less what they would hold at rest: the phantoms.
float Piano::stretch(Str& s) {
  const int n = s.nlx;
  float b[2][256];
  const float* nw = nw_ + s.off;
  for (int d = 0; d < 2; ++d) {
    const float* x = x_[d] + s.off;
    for (int i = 0; i < n; ++i) b[d][i] = x[i] * nw[i];
  }
  // (Over its last 0.3 s the drive fades, the modes ringing out: no step when it stops.)
  const float fade = std::min(1.0f, s.longLeft * dt_ / 0.3f);
  float push = 0;
  for (int d = 0; d < 2; ++d)
    for (int i = 0; i < n; ++i) push += s.ltension * b[d][i] * b[d][i];
  for (int j = 0; j < s.nl; ++j) {
    const int k = j + 1;
    float S = 0;
    for (int d = 0; d < 2; ++d) {
      const float* q = b[d];
      for (int i = 0; i + k < n; ++i) S += q[i] * q[i + k];      // (pairs k apart: mode i + 1 with i + 1 + k)
      float h = 0;
      for (int i = 0; i < k - 1; ++i) h += q[i] * q[k - 2 - i];  // (pairs adding up to k)
      S += 0.5f * h;
    }
    const float vv = s.lv[j] + fade * s.ldrive[j] * S;
    const float xn = s.lx[j] * s.lcs[j] + vv * s.lsn[j];
    s.lv[j] = vv * s.lcs[j] - s.lx[j] * s.lsn[j];
    s.lx[j] = xn;
    push += s.lout[j] * xn + s.lrest[j] * S;
  }
  if (--s.longLeft == 0)
    for (int j = 0; j < s.nl; ++j) s.lx[j] = s.lv[j] = 0;
  return push;
}

// A damper's felt pushing its strings this sample, while it lands; its grip comes on with it.
void Piano::land(Key& k) {
  if (k.landWait > 0) { --k.landWait; return; }
  if (k.landLeft <= 0) return;
  const float f = k.landAmp * std::sin(float(M_PI) * (k.landN - k.landLeft + 0.5f) / k.landN);
  for (int j = 0; j < k.ns; ++j) {
    const Str& s = str_[k.s0 + j];
    float* v0 = v_[0] + s.off;
    const float* dk = dk_ + s.off;
    for (int i = 0; i < s.n; ++i) v0[i] += dk[i] * f;
  }
  if (--k.landLeft == 0) --landing_;
}

// The hammer against its strings, one sample: the felt's compression on each
// string at the end of the sample depends on the force during it (the string
// gives way, the hammer is slowed): solved for, not stepped (a hard blow on a
// stiff felt would otherwise overshoot).
// The plectrum: a spring between the rising jack and the string. It bends
// (the string hardly moving: this takes some milliseconds and is not heard)
// until its force passes the limit; then the string is off its tip.
void Piano::pluck(Key& k) {
  const float reach = k.eta + dt_ * k.vh;  // the jack a sample on
  float total = 0;
  bool gone = false;
  for (int j = 0; j < k.ns; ++j) {
    const Str& s = str_[k.s0 + j];
    float* x = x_[0] + s.off;
    float* v0 = v_[0] + s.off;
    float* v1 = v_[1] + s.off;
    const float *hc = hc_ + s.off, *hs = hs_ + s.off, *hk = hk_ + s.off;
    float y = 0;
    for (int i = 0; i < s.n; ++i) y += hc[i] * x[i] + hs[i] * v0[i];
    const float f = reach > y ? k.pluckK * (reach - y) / (1 + k.pluckK * s.comply) : 0;
    if (f >= k.pluckLimit * voicing_) { gone = true; continue; }
    if (f <= 0) continue;
    const float f1 = f * k.pluckSide;
    for (int i = 0; i < s.n; ++i) {
      v0[i] += hk[i] * f;
      v1[i] += hk[i] * f1;
    }
    total += f;
  }
  k.eta = reach;
  k.force = total;
  if (gone) {
    k.hammerOn = false;
    k.force = 0;
  }
}

void Piano::hammer(Key& k) {
  if (k.pluckK > 0) { pluck(k); return; }
  static const float kHyst = float(knob("PJ_PIANO_HYST", 0.25));  // s/m: more force pressing than releasing
  const float K0 = k.K * voicing_, p = k.p;
  const float hm = dt_ * dt_ / k.mh;
  float yFree[kMaxStr], F[kMaxStr] = {0, 0, 0};
  for (int j = 0; j < k.ns; ++j) {
    const Str& s = str_[k.s0 + j];
    const float* x = x_[0] + s.off;
    const float* v = v_[0] + s.off;
    const float* hc = hc_ + s.off;
    const float* hs = hs_ + s.off;
    float y = 0;
    for (int i = 0; i < s.n; ++i) y += hc[i] * x[i] + hs[i] * v[i];
    yFree[j] = y;
  }
  bool touching[kMaxStr];
  for (int j = 0; j < k.ns; ++j) touching[j] = str_[k.s0 + j].dPrev > 0;
  const float reach = k.eta + dt_ * k.vh;  // the hammer's place a sample on, unopposed
  float total = 0;
  for (int sweep = 0; sweep < 3; ++sweep) {
    for (int j = 0; j < k.ns; ++j) {
      Str& s = str_[k.s0 + j];
      const float others = total - F[j];
      const float A = s.comply + hm, b = reach - s.gap - yFree[j] - hm * others;
      const float K = K0 * s.hard;
      // d + A f(d) = b, f(d) = K d^p (1 + hyst (d - dPrev) / dt) for d > 0.
      float d = std::max(b, 0.0f), f = 0;
      if (b > 0 && k.nap > 0) {
        // The nap on the felt, two springs in a row: at the force g the nap
        // is pressed nap g / (g + c) (c = napK nap) and the felt under it
        // (g / K)^(1/p). g + A g h(d(g)) ... = b, solved for g (Newton, from
        // below: the left side rises with g and is concave).
        const float c = k.napK * k.nap, ip = 1 / p, iK = 1 / K;
        float g = 0;
        d = 0;
        for (int it = 0; it < 12; ++it) {
          const float dn = k.nap * g / (g + c), dm = std::pow(g * iK, ip);
          d = dn + dm;
          const float dd = k.nap * c / ((g + c) * (g + c)) + (g > 1e-12f ? ip * dm / g : 1e6f);
          const float h = std::max(0.0f, 1 + kHyst * (d - s.dPrev) / dt_);
          const float r = d + A * g * h - b;
          const float dr = dd + A * (h + g * kHyst / dt_ * dd);
          g = std::max(0.0f, g - r / dr);
          if (it == 0 && g <= 0) g = 1e-6f;
        }
        d = std::min(b, k.nap * g / (g + c) + std::pow(g * iK, ip));
        f = g * std::max(0.0f, 1 + kHyst * (d - s.dPrev) / dt_);
      } else if (b > 0) {
        d = std::min(b, std::max(s.dPrev, 1e-7f));
        for (int it = 0; it < 6; ++it) {
          const float pw = K * std::pow(d, p - 1);
          const float h = std::max(0.0f, 1 + kHyst * (d - s.dPrev) / dt_);
          f = pw * d * h;
          const float df = pw * p * h + pw * d * kHyst / dt_;
          const float step = (d + A * f - b) / (1 + A * df);
          d = std::clamp(d - step, 0.0f, b);
        }
        f = K * std::pow(d, p) * std::max(0.0f, 1 + kHyst * (d - s.dPrev) / dt_);
      }
      total += f - F[j];
      F[j] = f;
      if (sweep == 2) s.dPrev = b > 0 ? d : 0;
    }
  }
  // (a tack's metal head meets its string first: a sharp knock, a half sine of tackTime)
  for (int j = 0; j < k.ns; ++j) {
    Str& s = str_[k.s0 + j];
    if (s.tack <= 0) continue;
    if (!touching[j] && s.dPrev > 0 && k.vh > 0) {
      s.tickLeft = std::max(2, int(d_.tackTime / dt_ + 0.5));
      s.tickAmp = std::min(s.tack * k.vh, 4 * s.tack);  // (capped: the head gives at a hard blow)
      s.tickN = s.tickLeft;
    }
    if (s.tickLeft > 0) {
      F[j] += s.tickAmp * std::sin(float(M_PI) * (s.tickN - s.tickLeft + 0.5f) / s.tickN);
      --s.tickLeft;
    }
  }
  for (int j = 0; j < k.ns; ++j) {
    if (F[j] <= 0) continue;
    const Str& s = str_[k.s0 + j];
    float* v0 = v_[0] + s.off;
    float* v1 = v_[1] + s.off;
    const float* hk = hk_ + s.off;
    const float f0 = F[j], f1 = F[j] * s.tanH;
    for (int i = 0; i < s.n; ++i) {
      v0[i] += hk[i] * f0;
      v1[i] += hk[i] * f1;
    }
  }
  k.force = total;
  // (The hammer falls back: its weight and the action's spring.)
  k.vh -= dt_ * (total / k.mh + 30.0f);
  k.eta += dt_ * k.vh;
  // Falling away from the strings: caught by the action.
  if (total <= 0 && k.vh < 0) {
    float lowest = 1e9f;
    for (int j = 0; j < k.ns; ++j) lowest = std::min(lowest, yFree[j]);
    if (k.eta < lowest - 5e-4f) {
      k.hammerOn = false;
      k.force = 0;
    }
  }
}

// One sample of a string's modes, both directions of swing: the bridge's motion dy0 / dy1 into
// them, the damper's grip e (0: free), and what they passed back to the bridge (sum0 / sum1).
static inline void swing(int n, float e, float dy0, float dy1, const float* __restrict in,
                         const float* __restrict out, const float* __restrict dm, float* __restrict x0,
                         float* __restrict v0, float* __restrict x1, float* __restrict v1,
                         const float* __restrict c0, const float* __restrict s0, const float* __restrict c1,
                         const float* __restrict s1, float* sum0p, float* sum1p) {
  float sum0 = 0, sum1 = 0;
  if (e > 0) {
    for (int i = 0; i < n; ++i) {
      const float g = 1 - e * (1 - dm[i]);
      const float xa = x0[i] + in[i] * dy0, xb = x1[i] + in[i] * dy1;
      const float va = g * (v0[i] * c0[i] - xa * s0[i]), vb = g * (v1[i] * c1[i] - xb * s1[i]);
      x0[i] = g * (xa * c0[i] + v0[i] * s0[i]);
      x1[i] = g * (xb * c1[i] + v1[i] * s1[i]);
      sum0 += out[i] * (va - v0[i]);
      sum1 += out[i] * (vb - v1[i]);
      v0[i] = va;
      v1[i] = vb;
    }
  } else {
    for (int i = 0; i < n; ++i) {
      const float xa = x0[i] + in[i] * dy0, xb = x1[i] + in[i] * dy1;
      const float va = v0[i] * c0[i] - xa * s0[i], vb = v1[i] * c1[i] - xb * s1[i];
      x0[i] = xa * c0[i] + v0[i] * s0[i];
      x1[i] = xb * c1[i] + v1[i] * s1[i];
      sum0 += out[i] * (va - v0[i]);
      sum1 += out[i] * (vb - v1[i]);
      v0[i] = va;
      v1[i] = vb;
    }
  }
  *sum0p = sum0;
  *sum1p = sum1;
}

// Four sums over the board's modes at once (independent chains: no wait on each other).
static inline void dot4(const float* const* r, const float* __restrict x, int n, float* out) {
  const float *__restrict a = r[0], *__restrict b = r[1], *__restrict c = r[2], *__restrict d = r[3];
  float sa = 0, sb = 0, sc = 0, sd = 0;
  for (int m = 0; m < n; ++m) {
    sa += a[m] * x[m];
    sb += b[m] * x[m];
    sc += c[m] * x[m];
    sd += d[m] * x[m];
  }
  out[0] = sa, out[1] = sb, out[2] = sc, out[3] = sd;
}

// y += the four rows times their weights.
static inline void axpy4(float* __restrict y, const float* const* r, const float* f, int n) {
  const float *__restrict a = r[0], *__restrict b = r[1], *__restrict c = r[2], *__restrict d = r[3];
  const float fa = f[0], fb = f[1], fc = f[2], fd = f[3];
  for (int m = 0; m < n; ++m) y[m] += a[m] * fa + b[m] * fb + c[m] * fc + d[m] * fd;
}

float Piano::sample(float testForce) {
  if (boardStale_) boardNow();
  if (asleep_) {
    if (testForce == 0) return 0;
    wake();
  }
  const int kB = nB_, kP = nP_;
  const float kSide = kSide_;
  for (int a = 0; a < nActive_; ++a) {
    Key& k = key_[activeKeys_[a]];
    if (k.hammerOn) hammer(k);
  }
  std::memset(bf_, 0, sizeof(float) * kB);
  float pick = 0;
  // The keys four at a time: how far the bridge moved under each in the last sample (four
  // independent sums over the board's modes), their strings, their push on the board (its force
  // read and written once for the four).
  for (int a0 = 0; a0 < nActive_; a0 += 4) {
    const int g = std::min(4, nActive_ - a0);
    const float* ph[4];
    for (int u = 0; u < 4; ++u) ph[u] = phi_ + size_t(activeKeys_[a0 + std::min(u, g - 1)]) * kB;
    float acc[4], F4[4] = {0, 0, 0, 0};
    dot4(ph, ba_, kB, acc);
    for (int u = 0; u < g; ++u) {
      const int ki = activeKeys_[a0 + u];
      Key& k = key_[ki];
      k.acc = acc[u];
      if (k.landWait > 0 || k.landLeft > 0) {
        // (Landing: no grip until the felt touches; then half of it within the push (the felt
        // closing on the swinging strings: the first contact is quick), the rest as it beds in.)
        const bool touching = k.landWait == 0;
        land(k);
        if (touching) k.engage = std::min(1.0f, k.engage + 0.5f / k.landN);
      } else if (k.damperDown && k.engage < 1) k.engage = std::min(1.0f, k.engage + dt_ / 0.05f);
      const float e = k.engage;
      float F = 0, Fp = 0;
      for (int j = 0; j < k.ns; ++j) {
        const Str& s = str_[k.s0 + j];
        if (!s.active) continue;
        // Both directions of swing in one pass (the bridge's coupling and output weights shared).
        float sum0, sum1;
        const size_t o = s.off;
        swing(s.live, e, k.acc, kSide * k.acc, in_ + o, out_ + o, damp_ + o, x_[0] + o, v_[0] + o, x_[1] + o,
              v_[1] + o, cs_ + o, sn_ + o, cs1_ + o, sn1_ + o, &sum0, &sum1);
        F -= sum0 + kSide * sum1;
        if (pickup_) Fp -= sum0 + pickSide_ * sum1;
        if (s.longLeft > 0) {
          const float fl = stretch(str_[k.s0 + j]);
          F += fl;
          if (pickup_) Fp += fl;
        }
      }
      pick += d_.pickupGain[ki] * Fp;
      F4[u] = F;
    }
    axpy4(bf_, ph, F4, kB);
  }
  if (knocks_ > 0) {  // (the knocks in the case, at the rim)
    float f = 0;
    for (Knock& q : knock_) {
      if (q.left <= 0) continue;
      f += q.amp * std::sin(float(M_PI) * (q.n - q.left + 0.5f) / q.n) * dt_;
      if (--q.left == 0) --knocks_;
    }
    const float* tp = trapPhi_.data();
    for (int m = 0; m < kB; ++m) bf_[m] += tp[m] * f;
  }
  if (testForce != 0 && kB) {  // (tests: a force at middle C's bridge point)
    const float* ph = phi_ + size_t(39) * kB;
    for (int m = 0; m < kB; ++m) bf_[m] += ph[m] * testForce * dt_;
  }
  // The soundboard's resonances.
  const float fs = float(fs_);
  for (int m = 0; m < kB; ++m) {
    const float vv = bv_[m] + bf_[m];
    const float xn = bx_[m] * bcs_[m] + vv * bsn_[m];
    const float vn = vv * bcs_[m] - bx_[m] * bsn_[m];
    const float a = (vn - bv_[m]) * fs;
    ba_[m] = (xn - bx_[m]) * binv_[m];  // how far it moved
    bx_[m] = xn;
    bv_[m] = vn;
    bacc_[m] = a;
  }
  // (How far the bridge moved under each sounding key: taken at the next sample's start.)
  // The air each patch of the board moves, and what of it reaches the two
  // microphones now.
  ringPos_ = (ringPos_ + 1) & (kRing - 1);
  float l = 0, r = 0;
  for (int p0 = 0; p0 < kP; p0 += 4) {
    const int g = std::min(4, kP - p0);
    const float* pv[4];
    for (int u = 0; u < 4; ++u) pv[u] = pvol_ + size_t(p0 + std::min(u, g - 1)) * kB;
    float q4[4];
    dot4(pv, bacc_, kB, q4);
    for (int u = 0; u < g; ++u) {
      const int p = p0 + u;
      float* ring = ring_ + size_t(p) * kRing;
      ring[ringPos_] = q4[u];
      const int i0 = (ringPos_ - tapI_[0][p]) & (kRing - 1), i1 = (ringPos_ - tapI_[1][p]) & (kRing - 1);
      l += tapG_[0][p] * (ring[i0] + tapF_[0][p] * (ring[(i0 - 1) & (kRing - 1)] - ring[i0]));
      r += tapG_[1][p] * (ring[i1] + tapF_[1][p] * (ring[(i1 - 1) & (kRing - 1)] - ring[i1]));
    }
  }
  side_ = pickup_ ? 0 : 0.5f * (l - r);
  pick_ = pick * fs;  // (what the strings gave the bridge in this sample, as a force)
  // Now and then: strings that have come to rest under their dampers stop
  // being computed; with every string and the board at rest, the piano sleeps.
  if ((++clock_ & 1023) == 0) {
    bool changed = false, hammers = false;
    const int wasQuiet = quiet_;
    quiet_ = 0;
    float loud = 0;
    for (Str& s : str_) {
      if (!s.active) continue;
      const Key& k = key_[s.key];
      hammers = hammers || k.hammerOn;
      float en = 0;
      for (int d = 0; d < 2; ++d) {
        const float* x = x_[d] + s.off;
        const float* v = v_[d] + s.off;
        for (int i = 0; i < s.n; ++i) en += x[i] * x[i] + v[i] * v[i];
      }
      loud = std::max(loud, en);
      // (its stretching: computed until the swing has lost 20 dB; its square then 40 dB under
      // what it was, the phantoms 20 dB further under the partials than at their loudest)
      if (s.longLeft > int(0.3 * fs_)) {
        s.longPeak = std::max(s.longPeak, en);
        if (en < 0.01f * s.longPeak) s.longLeft = int(0.3 * fs_);
      }
      if (k.damperDown && k.engage >= 1 && !k.hammerOn && k.landLeft == 0 && en < 1e-13f) {
        s.active = false;
        for (int d = 0; d < 2; ++d) {
          std::memset(x_[d] + s.off, 0, sizeof(float) * s.n);
          std::memset(v_[d] + s.off, 0, sizeof(float) * s.n);
        }
        changed = true;
      }
    }
    if (changed) census();
    float board = 0;
    for (int m = 0; m < kB; ++m) board += bx_[m] * bx_[m] + bv_[m] * bv_[m];
    if (!hammers && !landing_ && !knocks_ && loud < 1e-13f && board < 1e-16f && (quiet_ = wasQuiet + 1) > 2) {  // (and the air has carried the last of it)
      for (int d = 0; d < 2; ++d) {
        std::memset(x_[d], 0, sizeof(float) * total_);
        std::memset(v_[d], 0, sizeof(float) * total_);
      }
      std::memset(bx_, 0, sizeof(float) * kB);
      std::memset(bv_, 0, sizeof(float) * kB);
      std::memset(ba_, 0, sizeof(float) * kB);
      for (Key& k : key_) k.acc = 0;
      std::memset(ring_, 0, sizeof(float) * kP * kRing);
      asleep_ = true;
    }
  }
  return pickup_ ? pick_ : 0.5f * (l + r);
}

}  // namespace pajupilli
