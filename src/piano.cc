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
  }
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
  const size_t words = size_t(total_) * 14 + size_t(kB) * 12 + size_t(kB) * (kKeys + kP) + size_t(kP) * (kRing + 16) + 64;
  mem_ = static_cast<float*>(std::aligned_alloc(64, (words * sizeof(float) + 63) / 64 * 64));
  std::memset(mem_, 0, words * sizeof(float));
  float* p = mem_;
  auto take = [&](size_t n) { float* q = p; p += (n + 15) / 16 * 16; return q; };
  x_[0] = take(total_); x_[1] = take(total_); v_[0] = take(total_); v_[1] = take(total_);
  cs_ = take(total_); sn_ = take(total_); cs1_ = take(total_); sn1_ = take(total_); in_ = take(total_); out_ = take(total_);
  damp_ = take(total_); hk_ = take(total_); hc_ = take(total_); hs_ = take(total_);
  bx_ = take(kB); bv_ = take(kB); bcs_ = take(kB); bsn_ = take(kB); bf_ = take(kB); ba_ = take(kB); binv_ = take(kB);
  bacc_ = take(kB);
  phi_ = take(size_t(kB) * kKeys);
  pvol_ = take(size_t(kB) * kP);
  ring_ = take(size_t(kP) * kRing);

  const double hamW = d_.hammerWidth;
  uint32_t rng2 = d_.seed * 2654435761u + 1;  // (the out-of-tune piano's own draws: the others stay as they are)
  auto draw2 = [&]() { rng2 = rng2 * 1664525u + 1013904223u; return (rng2 >> 8) / 16777216.0; };
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
    if (d_.honkyCents > 0 && s.strings > 1 && !(s.strings == 3 && j == 1))
      cents += (draw2() < 0.5 ? -1 : 1) * d_.honkyCents * (0.5 + 0.5 * draw2());
    if (d_.flatShare > 0 && draw2() < d_.flatShare) cents -= d_.flatMax * draw2();
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
    }
    st.comply = float(comply);
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
  for (Key& k : key_) {
    k.hammerOn = false;
    k.acc = k.force = 0;
  }
  for (Str& s : str_) {
    s.active = !key_[s.key].damperDown;
    s.dPrev = 0;
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
  }
  k.eta = k.pluckK > 0 ? lowest : lowest - 1e-4f;  // (a plectrum starts touching its string)
  k.vh = float(speed);
  k.hammerOn = true;
  wake();
  census();
}

void Piano::damper(int key, bool down) {
  if (key < 0 || key >= nKeys_) return;
  Key& k = key_[key];
  if (!k.hasDamper || k.damperDown == down) return;
  k.damperDown = down;
  boardStale_ = true;
  if (!down) {
    k.engage = 0;
    for (int j = 0; j < k.ns; ++j) str_[k.s0 + j].active = true;
    census();
  }
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
  const float K = k.K * voicing_, p = k.p;
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
  const float reach = k.eta + dt_ * k.vh;  // the hammer's place a sample on, unopposed
  float total = 0;
  for (int sweep = 0; sweep < 3; ++sweep) {
    for (int j = 0; j < k.ns; ++j) {
      Str& s = str_[k.s0 + j];
      const float others = total - F[j];
      const float A = s.comply + hm, b = reach - yFree[j] - hm * others;
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
  for (int a = 0; a < nActive_; ++a) {
    const int ki = activeKeys_[a];
    Key& k = key_[ki];
    if (k.damperDown && k.engage < 1) k.engage = std::min(1.0f, k.engage + dt_ / 0.05f);
    const float e = k.engage;
    float F = 0, Fp = 0;
    for (int j = 0; j < k.ns; ++j) {
      const Str& s = str_[k.s0 + j];
      if (!s.active) continue;
      const float* in = in_ + s.off;
      const float* out = out_ + s.off;
      const float* dm = damp_ + s.off;
      for (int d = 0; d < 2; ++d) {
        float* x = x_[d] + s.off;
        float* v = v_[d] + s.off;
        const float* cs = (d == 0 ? cs_ : cs1_) + s.off;
        const float* sn = (d == 0 ? sn_ : sn1_) + s.off;
        const float dy = d == 0 ? k.acc : kSide * k.acc;
        float sum = 0;
        if (e > 0) {
          for (int i = 0; i < s.n; ++i) {
            const float g = 1 - e * (1 - dm[i]);
            const float xx = x[i] + in[i] * dy;
            const float vn = g * (v[i] * cs[i] - xx * sn[i]);
            x[i] = g * (xx * cs[i] + v[i] * sn[i]);
            sum += out[i] * (vn - v[i]);
            v[i] = vn;
          }
        } else {
          for (int i = 0; i < s.n; ++i) {
            const float xx = x[i] + in[i] * dy;
            const float vn = v[i] * cs[i] - xx * sn[i];
            x[i] = xx * cs[i] + v[i] * sn[i];
            sum += out[i] * (vn - v[i]);
            v[i] = vn;
          }
        }
        F -= d == 0 ? sum : kSide * sum;
        if (pickup_) Fp -= d == 0 ? sum : pickSide_ * sum;
      }
    }
    pick += d_.pickupGain[ki] * Fp;
    const float* ph = phi_ + size_t(ki) * kB;
    for (int m = 0; m < kB; ++m) bf_[m] += ph[m] * F;
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
  // How far the bridge moved under each sounding key.
  for (int a = 0; a < nActive_; ++a) {
    const int ki = activeKeys_[a];
    const float* ph = phi_ + size_t(ki) * kB;
    float acc = 0;
    for (int m = 0; m < kB; ++m) acc += ph[m] * ba_[m];
    key_[ki].acc = acc;
  }
  // The air each patch of the board moves, and what of it reaches the two
  // microphones now.
  ringPos_ = (ringPos_ + 1) & (kRing - 1);
  float l = 0, r = 0;
  for (int p = 0; p < kP; ++p) {
    const float* pv = pvol_ + size_t(p) * kB;
    float q = 0;
    for (int m = 0; m < kB; ++m) q += pv[m] * bacc_[m];
    float* ring = ring_ + size_t(p) * kRing;
    ring[ringPos_] = q;
    const int i0 = (ringPos_ - tapI_[0][p]) & (kRing - 1), i1 = (ringPos_ - tapI_[1][p]) & (kRing - 1);
    l += tapG_[0][p] * (ring[i0] + tapF_[0][p] * (ring[(i0 - 1) & (kRing - 1)] - ring[i0]));
    r += tapG_[1][p] * (ring[i1] + tapF_[1][p] * (ring[(i1 - 1) & (kRing - 1)] - ring[i1]));
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
      if (k.damperDown && k.engage >= 1 && !k.hammerOn && en < 1e-13f) {
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
    if (!hammers && loud < 1e-13f && board < 1e-16f && (quiet_ = wasQuiet + 1) > 2) {  // (and the air has carried the last of it)
      for (int d = 0; d < 2; ++d) {
        std::memset(x_[d], 0, sizeof(float) * total_);
        std::memset(v_[d], 0, sizeof(float) * total_);
      }
      std::memset(bx_, 0, sizeof(float) * kB);
      std::memset(bv_, 0, sizeof(float) * kB);
      for (Key& k : key_) k.acc = 0;
      std::memset(ring_, 0, sizeof(float) * kP * kRing);
      asleep_ = true;
    }
  }
  return pickup_ ? pick_ : 0.5f * (l + r);
}

}  // namespace pajupilli
