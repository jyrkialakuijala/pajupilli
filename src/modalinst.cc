#include "modalinst.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "knob.h"

namespace pajupilli {

namespace {

// Every key a note (the range is the instrument's own).
const NoteInfo kModalNotes[128] = {
#define N {"", 1}
#define N8 N, N, N, N, N, N, N, N
    N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8
#undef N8
#undef N
};
const Design kModalDesign = {nullptr, 0, nullptr, 0, kModalNotes, 0, 0, false, nullptr, 0, {}, {}};

double hz(double midi) { return 440 * std::exp2((midi - 69) / 12); }

// A string on the bridge (at x = 0) and the nut: n modes, inharmonicity B,
// decay t60(f) = 1 / (a + b f^2) style: t60 of mode k = t0 / (1 + (f_k / fc)^2)
// (air and internal losses: the high modes go first), the pluck at `at`
// (share of L from the bridge).
void stringDef(ModalSet::Def& d, double f0, double L, double T, double B, int n, double t0, double fc, double at,
               double split) {
  const double mu = T / (4 * L * L * f0 * f0);  // f0 = sqrt(T / mu) / 2L
  d.n = std::min(n, int(ModalSet::kModes));
  d.modalMass = float(mu * L / 2);
  d.split = float(split);
  for (int k = 1; k <= d.n; ++k) {
    const double f = k * f0 * std::sqrt(1 + B * k * k);
    d.freq[k - 1] = float(f);
    d.t60[k - 1] = float(t0 / (1 + (f / fc) * (f / fc)));
    d.exc[k - 1] = float(stringShape(k, at * L, L));
    d.bridge[k - 1] = float(k * M_PI * T / L);
    d.back[k - 1] = float(-2 / (k * M_PI));
  }
}


// An obstacle under a string (a jawari, a sawari): a patch centred x m from
// the bridge, w wide (cosine-weighted), its surface h m (below the string:
// negative), stiffness K (N/m).
void addContact(ModalSet::Def& d, double L, double x, double w, double h, double K) {
  if (d.nc >= ModalSet::kContacts) return;
  const int c = d.nc++;
  constexpr int kPts = 7;
  double gs = 0;
  for (int j = 0; j < kPts; ++j) gs += 0.5 * (1 + std::cos(M_PI * (2.0 * j / (kPts - 1) - 1) * 0.999));
  for (int k = 1; k <= d.n; ++k) {
    double sh = 0;
    for (int j = 0; j < kPts; ++j) {
      const double u = 2.0 * j / (kPts - 1) - 1, g = 0.5 * (1 + std::cos(M_PI * u * 0.999));
      sh += g * stringShape(k, x + 0.5 * w * u, L);
    }
    d.cShape[c][k - 1] = float(sh / gs);
  }
  d.cHeight[c] = float(h);
  d.cK = float(K);
}

// A tine's or a bar's modes driving a support (the bridge): its root force per
// unit modal displacement w_k^2 m g_k, and the support's acceleration drives it
// back by g_k (its participation).
void supportCoupling(ModalSet::Def& d, const float* part) {
  for (int k = 0; k < d.n; ++k) {
    const double w = 2 * M_PI * d.freq[k];
    d.bridge[k] = float(w * w * d.modalMass * part[k]);
    d.back[k] = -part[k];
  }
}

// Struck things: a mode list (ratios to the note, t60 at the note's pitch
// scaled by (f_ref / f)^tExp, amplitudes at the usual strike point,
// radiation weights), retuned per key.
struct ModeList {
  int n;
  const float* ratio;
  const float* t60;
  const float* amp;
  float tExp, fRef;
  float split;  // doublets: each mode's twin x (1 + split)
};

void struckDef(ModalSet::Def& d, const ModeList& m, double f0, double mass, uint32_t seed) {
  d = ModalSet::Def();
  d.n = std::min(m.n, int(ModalSet::kModes));
  d.modalMass = float(mass);
  d.split = m.split;
  d.zShare = 1;  // (both of a doublet radiate)
  for (int k = 0; k < d.n; ++k) {
    const double f = f0 * m.ratio[k];
    d.freq[k] = float(f);
    d.t60[k] = float(m.t60[k] * std::pow(m.fRef / std::max(f, 20.0), m.tExp));
    d.exc[k] = m.amp[k];
    seed = seed * 1664525u + 1013904223u;
    d.rad[k] = m.amp[k];
    d.side[k] = float(0.4 * m.amp[k] * (((seed >> 9) & 1) ? 1 : -1));
  }
}

}  // namespace

extern const Instrument kModals[] = {
    // GM 105-112: the ethnic family.
    {"sitar", 0, 48, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "sitar"},
    {"banjo", 0, 50, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "banjo"},
    {"shamisen", 0, 48, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "shamisen"},
    {"koto", 0, 48, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "koto"},
    {"kalimba", 0, 48, 88, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "kalimba"},
    {"fiddle", 0, 55, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "fiddle"},
    // GM 113-120: the percussive family (melodic tom and synth drum: src/lead.cc).
    {"tinkle-bell", 0, 48, 108, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "bell"},
    {"agogo", 0, 48, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "agogo"},
    {"steel-drums", 0, 52, 90, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "pan"},
    {"woodblock", 0, 48, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "block"},
    {"taiko", 0, 24, 72, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "taiko"},
    {"reverse-cymbal", 0, 36, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "cymbal"},
    // GM 12: the vibraphone (a Musser M55, F3-F6).
    {"vibraphone", 0, 53, 89, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "vibraphone"},
    // (The same vibraphone played with Mike Balter's 25R "jazz" mallets (rattan): very hard.)
    {"vibraphone-25r", 0, 53, 89, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "vibraphone"},
    // GM 121-128: the sound effects (src/sfx.cc).
    {"fret-noise", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "sfx"},
    {"breath-noise", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "sfx"},
    {"seashore", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "sfx"},
    {"bird-tweet", 0, 48, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "sfx"},
    {"telephone", 0, 48, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "sfx"},
    {"helicopter", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "sfx"},
    {"applause", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "sfx"},
    {"gunshot", 0, 36, 84, 1, 1, 1, 48000, 0, false, false, 0, 1.0, Kind::kModal, &kModalDesign, "sfx"},
};
extern const int kNumModals = int(sizeof kModals / sizeof kModals[0]);

ModalModel::ModalModel(const Instrument& inst) : Tube(inst), set_(inst.fs) {
  static const char* names[] = {"sitar", "banjo", "shamisen", "koto", "kalimba", "fiddle", "tinkle-bell",
                                "agogo", "steel-drums", "woodblock", "taiko", "reverse-cymbal", "vibraphone", "fret-noise",
                                "breath-noise", "seashore", "bird-tweet", "telephone", "helicopter", "applause",
                                "gunshot"};
  type_ = kSitar;
  for (int i = 0; i < int(sizeof names / sizeof names[0]); ++i)
    if (std::strcmp(inst.name, names[i]) == 0) type_ = Type(i);
  if (std::strncmp(inst.name, "vibraphone", 10) == 0) type_ = kVibraphone;  // (and its mallet variants)
  build();
  // (PJ_VIB_BARS=0: the vibraphone as one point in the room, its bars' places only in its sides)
  if (type_ == kVibraphone && !sfx && knob("PJ_VIB_BARS", 1) != 0) {
    for (int i = 0; i < kVibBars; ++i) {
      double x, y;
      vibPlace(i, &x, &y);
      static const double kSpread = knob("PJ_VIB_SPREAD", 1);  // (tests: 0 puts every bar at the centre)
      partPlace_.insert(partPlace_.end(), {float(kSpread * x), float(kSpread * y), 0.0f});
    }
    partPlace_.insert(partPlace_.end(), {0.0f, 0.0f, 0.0f});
    part_.resize(kVibBars + 1);
    set_.keepEach(true);
  }
}

// research/vibraphone.md 1: the bars 381 -> 175 mm long and 57 -> 38 mm wide, geometric from F3
// to F6, in one plane on two rows either side of the damper felt: the naturals on the player's
// side, the accidentals beyond them, each bar's middle (over its resonator) half its length and
// ~9 mm (half the gap the felt presses into) from the felt's line, so the rows close in from
// ~39 cm apart at F3 to ~19 cm at F6. Along the frame the naturals in order, ~12 mm apart (the
// 22 under the 1295 mm felt), an accidental over the gap between its two neighbours.
void ModalModel::vibPlace(int i, double* x, double* y) {
  auto len = [](int b) { return 0.381 * std::pow(0.175 / 0.381, b / 36.0); };
  auto wid = [](int b) { return 0.057 * std::pow(0.038 / 0.057, b / 36.0); };
  auto natural = [](int midi) { const int pc = midi % 12; return !(pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10); };
  constexpr double kGap = 0.012, kFelt = 0.018;
  // The naturals' middles along the frame, from F3's left edge.
  double at[kVibBars], edge = 0;
  for (int b = 0; b < kVibBars; ++b) {
    if (!natural(kVibLo + b)) continue;
    at[b] = edge + wid(b) / 2;
    edge += wid(b) + kGap;
  }
  const double span = edge - kGap;
  for (int b = 0; b < kVibBars; ++b)
    if (!natural(kVibLo + b)) at[b] = 0.5 * (at[b - 1] + at[b + 1]);  // (F3 and F6 are naturals)
  *x = at[i] - span / 2;
  *y = (natural(kVibLo + i) ? -1 : 1) * (len(i) / 2 + kFelt / 2);
}

const float* ModalModel::takePart(int n, int j) {
  if (j < 0 || j >= int(part_.size())) return nullptr;
  std::vector<float>& p = part_[j];
  partOut_.assign(n, 0.0f);
  const int have = int(p.size()), m = std::min(n, have);
  std::copy(p.end() - m, p.end(), partOut_.end() - m);
  p.clear();
  return partOut_.data();
}

void ModalModel::resetAir() {
  Tube::resetAir();
  set_.reset();
  if (sfx) sfx->reset();
  revPos = -1;
}

const float* ModalModel::takeSide(int n) {
  sideOut_.assign(n, 0.0f);
  const int have = int(side_.size()), m = std::min(n, have);
  std::copy(side_.end() - m, side_.end(), sideOut_.end() - m);
  side_.clear();
  return sideOut_.data();
}

void ModalModel::stopString(int si, double f) {
  Str& st = strings[si];
  const double r = f / st.f0, L = st.L / r;
  ModalSet::Def d;
  // (Stopped, a sitar's string ends on a fingertip pressing it to a curved
  // fret: more loss, the more so the higher up; a shamisen's on the bare
  // finger (no frets); a banjo's on a metal fret: none.)
  const double soft = type_ == kSitar ? 0.5 / std::sqrt(r) : type_ == kShamisen ? 0.7 : 1.0;
  const double t0 = r > 1.0001 ? st.t0 * soft : st.t0;
  stringDef(d, f, L, st.T, st.B * r * r, st.n, t0, st.fc, st.pluckAt / r, st.split);
  d.one = st.one;
  for (const Str::Contact& c : st.contacts)
    if (c.x < 0.5 * st.L) addContact(d, L, c.x, c.w, c.h, c.K);  // (the bridge's: the nut's is behind the finger)
    else if (r < 1.0001) addContact(d, L, c.x, c.w, c.h, c.K);
  set_.setVib(st.vib, d);
  st.fStop = f;
}

float ModalModel::Bowed::step(float* bridgeForce) {
  // Delays: the whole loop is fs / f (less the filter's few samples), split at the bow.
  const double fs = 48000;
  const double base = std::max(8.0, fs / fNow - 2.5);
  const double dB = std::max(1.0, base * beta), dN = std::max(1.0, base - dB);
  auto read = [&](const float* buf, double d) {
    const double p = pos - d;
    const int i = int(std::floor(p));
    const float fr = float(p - i);
    return buf[i & (kN - 1)] * (1 - fr) + buf[(i + 1) & (kN - 1)] * fr;
  };
  // The bridge reflects through a gentle low-pass (its losses), the nut inverts.
  const float fromBridge = read(bridge, dB), fromNut = read(neck, dN);
  lp += 0.55f * (fromBridge - lp);
  const float bridgeRefl = -damp * lp, nutRefl = -damp * fromNut;
  const float stringVel = bridgeRefl + nutRefl;
  float newVel = 0;
  if (press > 1e-4) {
    // STK's bow table: the friction's admittance, falling steeply as the
    // hair slips faster (slope with the pressure).
    const float dv = float(vel) - stringVel;
    const float slope = float(5 - 4 * press);
    const float t = std::pow(std::fabs((dv + 0.001f) * slope) + 0.75f, -4.0f);
    newVel = dv * std::min(1.0f, t);
  }
  pos = (pos + 1) & (kN - 1);
  neck[pos] = bridgeRefl + newVel;
  bridge[pos] = nutRefl + newVel;
  // The force on the bridge: the wave arriving there (and its reflection).
  *bridgeForce = fromBridge - bridgeRefl;
  return stringVel;
}

float ModalModel::sample() {
  if (type_ == kFiddle) {
    float F = 0;
    for (Bowed& b : bowed) {
      if (!b.active) continue;
      // Speed and pressure glide (the arm).
      b.vel += 0.002 * (b.velTo - b.vel);
      b.press += 0.002 * (b.pressTo - b.press);
      float fb;
      b.step(&fb);
      F += fb;
    }
    set_.push(F * 2.0f);
  }
  float y, sd;
  if (sfx) {
    y = sfx->sample(&sd);
  } else {
    y = set_.sample();
    sd = set_.side();
  }
  if (testDp_ != 0) {  // (tests: a NaN or a kick reaches the modes through a pluck)
    if (!std::isfinite(testDp_)) y = float(testDp_);
    testDp_ = 0;
  }
  if (revPos >= 0) {  // the reversed crash, read backwards at the key's rate
    const int i = int(revPos);
    if (i >= 1 && i < int(rev.size())) {
      const double f = revPos - i;
      const float r = float(revGain * (rev[i] * (1 - f) + rev[i - 1] * f));
      y += r;
      revPos -= revRate;
    } else {
      revPos = -1;
    }
  }
  if (!std::isfinite(y)) {
    resetAir();
    ++resets_;
    y = 0;
    sd = 0;
  }
  last_ = y;
  side_.push_back(float(gain * outScale()) * sd);
  const float out = finish(float(gain) * y);
  if (!part_.empty()) {
    if (part_[0].size() >= 1 << 15)  // (nobody takes them: played without a room)
      for (auto& p : part_) p.clear();
    const float g = float(gain * outScale());
    float sum = 0;
    for (int i = 0; i < kVibBars; ++i) {
      const float b = g * set_.each(i);
      part_[i].push_back(b);
      sum += b;
    }
    part_[kVibBars].push_back(out - sum);
  }
  return out;
}

}  // namespace pajupilli

namespace pajupilli {

namespace {

// --- Mode lists of the struck things (research/bells_blocks.md,
// steelpan_taiko.md; replaced by the measured ones where found).
// research/bells_blocks.md. A small tuned English handbell: the note (2,0)
// and its twelfth (3,0) (Rossing & Perrin, Hansen 2013's chart), the cluster
// above; Q ~2000 (a long ring; decay ~1/f). Doublets 0.1 %.
const float kBellRatio[] = {1, 3.0f, 4.8f, 5.4f, 5.9f, 7.4f, 7.6f, 9.0f, 10.2f, 11.7f};
const float kBellT60[] = {4.5f, 1.6f, 1.0f, 0.9f, 0.8f, 0.65f, 0.63f, 0.5f, 0.45f, 0.4f};
const float kBellAmp[] = {1, 0.7f, 0.35f, 0.3f, 0.25f, 0.2f, 0.18f, 0.15f, 0.12f, 0.1f};

// Below C5 a church bell, tuned (Hemony, Simpson; research/bells_blocks.md
// §1.3), relative to the prime (the key): the hum an octave below, the
// tierce a minor third above (1.2), the quint, the nominal an octave above,
// the deciem, undeciem, superquint and octave nominal. The hum and the prime
// ring longest, the tierce nearly as long (the minor-third colour of the
// decay); the upper partials die in a second or two; the nominal is the
// loudest at the strike. Cast, less round than a turned handbell: wider
// doublets (0.3 %: beats of ~1 Hz at C4).
const float kChurchRatio[] = {0.5f, 1, 1.2f, 1.5f, 2, 2.5f, 2.67f, 3.0f, 4.0f};
const float kChurchT60[] = {2.5f, 2.0f, 1.8f, 0.6f, 1.2f, 0.5f, 0.5f, 0.6f, 0.4f};
const float kChurchAmp[] = {0.6f, 0.8f, 0.9f, 0.25f, 1, 0.25f, 0.2f, 0.45f, 0.35f};

// An agogo bell: a thin conical iron shell. Its partials as measured on a
// recorded pair (FluidR3's General MIDI agogos, 1000 and 1499 Hz: the same
// ratios on both; research/triangle_cowbell.md 3): 2.0, 2.25, 2.55 (weak),
// 3.73 and 5.12 the strongest (-20, -15 dB), 6.46, 7.45, 7.94; their T60s
// 0.3 - 0.8 s whatever the pitch (the samples' own, a little longer). The amp
// is each mode's shape at the stroke and its radiation alike: the sound
// ~ amp^2 f, so amp = sqrt(10^(dB/20) / ratio), then each fitted on the
// model's own stroke (the stick's contact takes the upper ones down). The
// oval mouth splits each pair (3 %: a roughness more than a beat).
const float kAgogoRatio[] = {1, 2.0f, 2.25f, 2.55f, 3.73f, 5.12f, 6.46f, 7.45f, 7.94f};
const float kAgogoT60[] = {0.7f, 0.35f, 0.31f, 0.5f, 0.65f, 0.6f, 0.45f, 0.5f, 0.4f};
const float kAgogoAmp[] = {1, 0.37f, 0.36f, 0.166f, 0.184f, 0.24f, 0.13f, 0.116f, 0.194f};

// A wood (temple) block: the lip and the cavity under it (a bass-reflex pair,
// 1 : 1.15, Yuasa's mokugyo), the lip's partials (3.65, 5.34, 7.91: the
// Taiwanese temple blocks); wood's Q ~60-100, the cavity's ~20.
const float kBlockRatio[] = {1, 1.15f, 3.65f, 5.34f, 7.91f};
const float kBlockT60[] = {0.2f, 0.06f, 0.06f, 0.04f, 0.03f};
const float kBlockAmp[] = {1, 0.8f, 0.4f, 0.25f, 0.15f};

// research/steelpan_taiko.md. A tenor pan's note: modes at 1, 2, 4 x (the
// octave and the double octave tuned in; Monteil et al. 2015's G3: damping
// 0.0045 / 0.001 / 0.001), more above (4.8, 5.85, 7.7). Q constant over the
// pan (Maloney: 100..300 and more for the tuned ones).
const float kPanRatio[] = {1, 2, 4, 4.8f, 5.85f, 7.7f};
const float kPanT60[] = {0.9f, 2.0f, 1.0f, 0.3f, 0.25f, 0.2f};  // at 440 Hz (x 440 / f)
const float kPanAmp[] = {1, 0.5f, 0.3f, 0.15f, 0.1f, 0.06f};

// A nagado-daiko (48 cm; Suzuki et al. 2009): 1 : 1.567 : 2.138 : 2.266 :
// 2.611 : 2.734 : 2.985 : 3.295; the lowest split by the two heads through
// the air (0.975 and 1.059 x: Suzuki & Hwang 2008); the fundamental falls
// 44 dB/s, the next 22 dB/s (Nishio 1999).
const float kTaikoRatio[] = {0.975f, 1.059f, 1.567f, 2.138f, 2.266f, 2.611f, 2.734f, 2.985f, 3.295f};
const float kTaikoT60[] = {1.35f, 1.2f, 2.8f, 1.0f, 0.9f, 0.7f, 0.6f, 0.5f, 0.45f};
const float kTaikoAmp[] = {1, 0.6f, 0.35f, 0.4f, 0.25f, 0.2f, 0.15f, 0.12f, 0.1f};

}  // namespace

// Each instrument to the common loudness (tools/levels.py's measure: BS.1770,
// three notes at the default dynamics), dB: their raw levels span 50 dB (a
// koto's heavy body, a bell's light shell), more than the trims' +-18. The
// struck and the brightly plucked are then held down so the hardest hit
// peaks near full scale (a woodblock's click at the median loudness peaked
// at 5.6, a crash's at 7.7): sitar -2.3, banjo -10.6, shamisen -4.9, bell
// -2.5, agogo -7.5, pan -1.4, woodblock -15, reverse cymbal -17.7 dB. (The agogo: its bell made
// heavier and its partials as recorded, 2026-11-16: +10.2 dB, held down so
// velocity 127 peaks near full scale.) The
// sound effects measured over a 3 s held key (a helicopter spins up, a wave
// takes seconds); applause and the gunshot (all peak) held down likewise.
static const double kModalLevel[21] = {20.4, 9.5, 3.3, 33.8, 24.4, -15.8, 37.2, 30.8, 23.7, 28.8, 1.7, 35.7,
                                       -11.9, 49.4, 34.9, -16.0, 1.4, 8.5, 28.8, 21.8, 22.7};

static ModalSet::Def S_def(ModalSet& S, int i) { return S.def(i); }

void ModalModel::build() {
  if (type_ >= kFretNoise) sfx.reset(new Sfx(Sfx::Type(type_ - kFretNoise), fs_));
  else build1();
  gain *= std::pow(10.0, kModalLevel[type_] / 20);
}

void ModalModel::build1() {
  ModalSet& S = set_;
  strings.clear();
  auto addString = [&](double f0, double L, double T, double B, int n, double t0, double fc, double at,
                       double split) {
    ModalSet::Def d;
    stringDef(d, f0, L, T, B, n, t0, fc, at, split);
    Str st;
    st.f0 = f0;
    st.L = L;
    st.T = T;
    st.mu = T / (4 * L * L * f0 * f0);
    st.pluckAt = at;
    st.vib = int(strings.size());
    st.B = B;
    st.n = n;
    st.t0 = t0;
    st.fc = fc;
    st.split = split;
    S.setVib(st.vib, d);
    strings.push_back(st);
    return st.vib;
  };
  auto body = [&](std::initializer_list<float> f, std::initializer_list<float> t60, std::initializer_list<float> m,
                  std::initializer_list<float> rad, std::initializer_list<float> side) {
    S.setBody(int(f.size()), f.begin(), t60.begin(), m.begin(), rad.begin(), side.begin());
  };
  switch (type_) {
    case kSitar: {
      // research/sitar_shamisen.md. Sa = C#3. The main string (baaj) is Ma,
      // F#3, steel 0.305 mm, 0.88 m (~61 N, B 1.8e-5); the jod (Sa, C#3); the
      // bass drones (C#2, G#2, bronze); two chikari (C#4, C#5), shorter. The
      // losses as the tanpura's fit (t60 11 s at 130 Hz, 3 s at 2.6 kHz).
      // Each plays over the jawari: a broad bone bridge curving away from
      // the string, gap a x^2 with a = 0.6 m^-1 (Vyasarayani's measured 0.58;
      // steeper, 1.2 and more, the string soon leaves it and the buzz is only
      // in the attack; at 0.6 it blooms ~0.3 s in, the centroid up from ~1 to
      // ~2.6 kHz, and fades over seconds), three patches 3..13 mm from its
      // edge, ~1e8 N/m each (steel on bone): the string wraps on and off it
      // each period. (Four patches cost a fifth more; three keep the bloom.)
      struct SD { int midi; double L, T, B; int n; } main[] = {{54, 0.88, 61, 1.8e-5, 64}, {49, 0.88, 40, 1.5e-5, 48},
                                                             {37, 0.88, 35, 3e-5, 32}, {44, 0.88, 40, 3e-5, 32},
                                                             {61, 0.60, 30, 1.2e-5, 32}, {73, 0.45, 28, 1e-5, 16}};
      static const double kJaw = knob("PJ_JAWARI", 1);  // the jawari's curve, x (0: none)
      for (const SD& m : main) {
        const int i = addString(hz(m.midi), m.L, m.T, m.B, m.n, 11, 1600, 0.22, 0.002);
        ModalSet::Def d = S_def(S, i);
        d.one = m.midi < 49;  // (the drones: y only)
        strings[i].one = d.one;
        for (int c = 0; c < 3; ++c) {
          const double x = 0.003 + 0.005 * c, h = -0.6 * x * x * kJaw, K = 1.3e8 * (kJaw > 0);
          addContact(d, m.L, x, 0.005, h, K);
          strings[i].contacts.push_back({x, 0.004, h, K});
        }
        S.setVib(i, d);
      }
      // The tarafs (0.23 mm steel), tuned to the raga's notes (C#4 .. F5),
      // under the main strings: they ring in sympathy through the bridge.
      const int taraf[13] = {61, 62, 63, 64, 66, 68, 70, 71, 73, 74, 75, 76, 77};
      for (int t : taraf) {
        const int i = addString(hz(t), 0.55, 18, 1e-5, 16, 8, 1600, 0.12, 0.003);
        ModalSet::Def d = S_def(S, i);
        d.one = true;  // (y only: they ring along, never plucked)
        strings[i].one = true;
        S.setVib(i, d);
      }
      // The gourd and the top (tabli): no measurement; a high-pass like the
      // tanpura's body.
      body({180, 320, 520, 800, 1150, 1700, 2500, 3600}, {0.08f, 0.09f, 0.08f, 0.07f, 0.06f, 0.05f, 0.04f, 0.03f},
           {0.4f, 0.25f, 0.18f, 0.15f, 0.13f, 0.12f, 0.12f, 0.12f}, {0.3f, 0.6f, 0.9f, 1, 1, 0.9f, 0.8f, 0.6f},
           {0.3f, -0.4f, 0.5f, -0.3f, 0.3f, -0.2f, 0.2f, -0.2f});
      gain = 2.0;
      break;
    }
    case kBanjo: {
      // A Deering Eagle II (Woodhouse, Politzer & Mansour 2021; research/
      // banjo_koto.md): open G, the 5th string 0.50 m, the others 0.67 m;
      // tensions as measured; the modes die in 100-200 ms below 1 kHz.
      addString(hz(67), 0.50, 54.5, 3e-5, 48, 1.4, 2500, 0.14, 0.002);  // 5th: g4
      addString(hz(50), 0.67, 56.5, 5.6e-5, 48, 1.4, 2500, 0.104, 0.002);  // 4th: D3 (wound)
      addString(hz(55), 0.67, 45.7, 3e-5, 48, 1.4, 2500, 0.104, 0.002);    // 3rd: G3
      addString(hz(59), 0.67, 51.9, 2.5e-5, 48, 1.4, 2500, 0.104, 0.002);  // 2nd: B3
      addString(hz(62), 0.67, 60.7, 1.4e-5, 48, 1.4, 2500, 0.104, 0.002);  // 1st: D4
      // The head (Mylar, 268 mm, 5.33 kN/m) at the bridge: its lowest mode
      // (290 Hz with the air, Q ~3: it radiates hard), the higher ones sharp
      // (Q 50..80; Stephey & Moore's list); the 2.2 g bridge on the head's
      // stiffness (~20 kN/m): the 500-800 Hz formant; the bridge hills (3, 5 kHz).
      body({277.0f, 550.0f, 580.0f, 831.0f, 842.0f, 978.0f, 1084.0f, 1320.0f, 1549.0f, 1771.0f, 1991.0f, 600.0f, 3000.0f, 5000.0f},
           {0.0238267f, 0.2f, 0.189655f, 0.158845f, 0.15677f, 0.157464f, 0.142066f, 0.133333f, 0.113622f, 0.0993789f, 0.0883978f, 0.022f, 0.00366667f, 0.0022f},
           {0.008f, 0.006f, 0.006f, 0.006f, 0.006f, 0.006f, 0.006f, 0.006f, 0.006f, 0.006f, 0.006f, 0.0022f, 0.01f, 0.01f},
           {1.0f, 0.9f, 0.9f, 0.8f, 0.8f, 0.8f, 0.7f, 0.7f, 0.6f, 0.6f, 0.5f, 0.5f, 0.4f, 0.3f},
           {0.3f, -0.4f, 0.4f, -0.4f, 0.4f, -0.3f, 0.3f, -0.3f, 0.3f, -0.2f, 0.2f, 0.1f, -0.2f, 0.2f});
      gain = 1.0;
      break;
    }
    case kShamisen: {
      // research/sitar_shamisen.md. Silk, 0.80 m: 0.90 / 0.65 / 0.40 mm,
      // honchoshi C3 F3 C4 (~39, 35, 30 N); silk's losses (t60 ~3.5 s at
      // 131 Hz). The first string skips the nut and lies on the sawari, a
      // small ridge ~10 mm from it, just at the string's height: it buzzes.
      addString(hz(48), 0.80, 39, 3e-5, 48, 3.5, 2500, 0.08, 0.003);
      {
        ModalSet::Def d = S_def(S, 0);
        addContact(d, 0.80, 0.80 - 0.010, 0.002, -0.02e-3, 1.5e7);
        strings[0].contacts.push_back({0.80 - 0.010, 0.002, -0.02e-3, 1.5e7});
        S.setVib(0, d);
      }
      addString(hz(53), 0.80, 35, 3e-5, 48, 3.2, 2500, 0.08, 0.003);
      addString(hz(60), 0.80, 30, 3e-5, 48, 3.0, 2500, 0.08, 0.003);
      // The skin (Kokubun 2012): its first mode 151.7 Hz (Q ~50), the higher
      // ones a square membrane's; the frame 570, 631, 672 Hz; the whole
      // instrument 67.6, 85.4 Hz. The bridge (koma) is light on the skin.
      body({67.6f, 85.4f, 151.7f, 240, 303, 339, 387, 443, 570, 631, 672, 900},
           {0.5f, 0.4f, 0.72f, 0.4f, 0.3f, 0.28f, 0.25f, 0.22f, 0.1f, 0.1f, 0.1f, 0.08f},
           {0.2f, 0.2f, 0.006f, 0.006f, 0.006f, 0.007f, 0.007f, 0.008f, 0.03f, 0.03f, 0.03f, 0.01f},
           {0.2f, 0.2f, 1, 0.9f, 0.9f, 0.8f, 0.8f, 0.7f, 0.5f, 0.5f, 0.5f, 0.6f},
           {0.1f, -0.1f, 0.3f, -0.4f, 0.4f, -0.3f, 0.3f, -0.3f, 0.2f, -0.2f, 0.2f, -0.2f});
      gain = 1.0;
      break;
    }
    case kKoto: {
      // Thirteen strings at ~147 N, 1.2 g/m (polyester; Chinen et al. 2011),
      // each as long as its pitch needs (0.2..1.3 m: the ji, the movable
      // bridges, set for hirajoshi on D: MIDI 50 55 57 58 62 63 67 69 70 74
      // 75 79 81). Plucked 3 cm from the fixed bridge (the tsume).
      const int tune[13] = {50, 55, 57, 58, 62, 63, 67, 69, 70, 74, 75, 79, 81};
      const double c = std::sqrt(147 / 1.2e-3);
      for (int t : tune) {
        const double L = c / (2 * hz(t));
        addString(hz(t), L, 147, 1e-4, 48, 4, 3000, std::min(0.25, 0.03 / L), 0.002);
      }
      // The kiri body, 1.8 m (Coaldrake: the air mode 84 Hz, the top's 100 Hz,
      // a 50 Hz component; Ando's 132, 181, 211 Hz), Q ~30..40 (no measurement).
      body({50.0f, 84.0f, 100.0f, 132.0f, 181.0f, 211.0f, 300.0f, 450.0f, 700.0f, 1100.0f}, {1.32f, 1.04762f, 0.88f, 0.666667f, 0.486188f, 0.417062f, 0.293333f, 0.195556f, 0.125714f, 0.08f},
           {1.8f, 1.2f, 1.2f, 1.0f, 0.9f, 0.9f, 0.75f, 0.75f, 0.6f, 0.6f}, {0.6f, 1.2f, 1, 0.9f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f},
           {0.2f, -0.2f, 0.4f, -0.3f, 0.3f, -0.4f, 0.3f, -0.3f, 0.2f, -0.2f});
      gain = 1.5;
      break;
    }
    case kFiddle: {
      // research/kalimba_fiddle.md. G3 D4 A4 E5, 325 mm; mass per length
      // 2.8 / 1.29 / 0.72 / 0.41 g/m (steel strings, a fiddler's).
      const int tune[4] = {55, 62, 69, 76};
      const double mu[4] = {2.8e-3, 1.29e-3, 0.72e-3, 0.41e-3};
      // (The strings are bowed waveguides, not modes: a modal string's sharp
      // Helmholtz corner needs modes that turn too far in one sample for the
      // friction's solve; the waveguide keeps the corner.)
      for (int i = 0; i < 4; ++i) {
        Str st;
        st.f0 = hz(tune[i]);
        st.L = 0.325;
        st.mu = mu[i];
        st.T = mu[i] * 4 * 0.325 * 0.325 * st.f0 * st.f0;
        st.pluckAt = 0.08;
        st.vib = i;
        strings.push_back(st);
        bowed[i].f = bowed[i].fNow = st.f0;
      }
      // The body at the bridge (Woodhouse 2014's mobilities): A0 275 Hz Q 20
      // (0.02 m/s/N), CBR ~420, B1- 470, B1+ 540 (Q ~35, 0.13 m/s/N), more
      // above; the bridge on the top: the "bridge hill" ~2.5 kHz, Q 3.5.
      body({275, 420, 470, 540, 700, 1000, 1400, 2500, 4000}, {0.16f, 0.16f, 0.16f, 0.14f, 0.09f, 0.07f, 0.05f, 0.003f, 0.01f},
           {0.58f, 0.38f, 0.09f, 0.08f, 0.15f, 0.15f, 0.15f, 0.001f, 0.01f}, {1.5f, 0.5f, 1, 1, 0.8f, 0.8f, 0.8f, 0.6f, 0.5f},
           {0.2f, -0.4f, 0.4f, -0.3f, 0.3f, -0.3f, 0.2f, -0.2f, 0.2f});
      gain = 1.0;
      break;
    }
    case kKalimba: {
      // research/kalimba_fiddle.md: sixteen tines, retuned per key (a
      // chromatic kalimba). Spring steel over a bridge and a pressure bar:
      // the first overtone ~5.9 x the fundamental (Chapman: 5.3..6.8, below
      // the ideal cantilever's 6.27: the part behind the bridge), the next
      // ~16.5 x; the fundamental rings ~3.4 s, the overtones 0.1..0.4 s.
      pool = 16;
      // (The pressure bar holds the tine: a small share of its root force
      // reaches the box.)
      static const float part[3] = {0.25f, 0.13f, 0.08f};
      static const float ratio[3] = {1, 5.9f, 16.5f};
      for (int i = 0; i < pool; ++i) {
        ModalSet::Def d;
        d.n = 3;
        d.modalMass = 2e-3f;
        d.split = 0.0005f;
        d.zShare = 0.2f;
        for (int k = 0; k < 3; ++k) {
          d.freq[k] = float(hz(60) * ratio[k]);
          d.t60[k] = k == 0 ? 3.4f : k == 1 ? 0.35f : 0.12f;
          d.exc[k] = float(cantileverShape(k + 1, 0.95, 1) / 2);
          d.rad[k] = 0.05f;
        }
        supportCoupling(d, part);
        S.setVib(i, d);
        Str st;
        st.vib = i;
        strings.push_back(st);
      }
      // The box: its air at ~400 Hz (a 30 mm hole), Q ~20; the board above.
      body({400, 900, 1500}, {0.11f, 0.06f, 0.04f}, {0.5f, 0.8f, 0.8f}, {1.5f, 0.6f, 0.4f}, {0.3f, -0.3f, 0.3f});
      gain = 1.0;
      break;
    }
    case kVibraphone: {
      // research/vibraphone.md: a Musser M55, 37 bars F3-F6 of 2024
      // aluminium, 12.7 mm thick, 381 -> 175 mm long, arched underneath to
      // tune the overtones; a closed tube under each, tuned to the bar.
      // Each bar one vibrator (y only: the mallet strikes down), its modes:
      //   0-3 bending: 1 : 4 : 10 : ~18 low; the third tuned flatter (9.6)
      //       from D#4, only the fundamental tuned from G#5 (the arch does
      //       less on a short bar: towards the flat bar's 2.76 : 5.40 : 8.93);
      //   4   the first torsion (struck off the centre line): ~7 x at F3,
      //       ~2.7 x at F6; 5 the first lateral, ~12 x .. 4.7 x (barely struck);
      //   6   the air in the tube, 5 c under the bar (a closed quarter-wave
      //       tube: only its fundamental is near any of the bar's modes; Q
      //       ~89 at F3 .. ~12 at F6), coupled to bending 1 by a spring
      //       k = 2 w kappa, the low bars near critical coupling: the tube
      //       takes ~80 % of the fundamental's energy and radiates it (alone
      //       25 s at F3, with the tube open ~3.5 s).
      // The bars alone ring T60 ~ 25 s x (175 / f)^0.75.
      for (int i = 0; i < kVibBars; ++i) {
        const int midi = kVibLo + i;
        const double f = hz(midi), u = i / double(kVibBars - 1);  // 0 at F3, 1 at F6
        // (its place along the frame, -1 at F3's end .. 1 at F6's: the sides follow it)
        double px, py_;
        vibPlace(i, &px, &py_);
        {
          double x0, x1, yy;
          vibPlace(0, &x0, &yy);
          vibPlace(kVibBars - 1, &x1, &yy);
          px = (2 * px - (x0 + x1)) / (x1 - x0);
        }
        auto lerp = [](double a, double b, double t) { return a + (b - a) * std::clamp(t, 0.0, 1.0); };
        auto logl = [](double a, double b, double t) { return a * std::pow(b / a, std::clamp(t, 0.0, 1.0)); };
        double r[6];
        r[0] = 1;
        r[1] = midi <= 79 ? 4.0 : lerp(4.0, 3.1, (midi - 79) / 10.0);
        r[2] = midi <= 62 ? 10.0 : midi <= 67 ? 9.6 : lerp(9.6, 6.2, (midi - 67) / 22.0);
        r[3] = lerp(18.0, 10.0, u);
        r[4] = logl(7.0, 2.7, u);
        r[5] = logl(12.0, 4.7, u);
        // Struck between the centre and the edge: bending 2 (a node at the
        // centre) and the torsion come in part.
        static const float exc[6] = {1.0f, 0.55f, 0.5f, 0.3f, 0.3f, 0.05f};
        // Each bar a dipole: its low modes radiate little (the tube does
        // that); higher, more.
        static const float rad[6] = {0.12f, 0.45f, 0.5f, 0.4f, 0.15f, 0.08f};
        ModalSet::Def d;
        d.one = true;
        d.n = 7 + kVibHigh;
        d.modalMass = float(0.12 * std::pow(175 / f, 0.6));  // kg (a 0.5 kg bar at F3)
        // On the instrument the bar hangs on cords through holes at its
        // fundamental's nodes: the overtones' nodes lie elsewhere, so the cords
        // and their felts damp them (a free bar's overtones ring 6-10 s,
        // research 2.1; mounted, they fade while the tube carries the
        // fundamental on): their T60 x PJ_VIB_SUPPORT (0.2, a design value).
        // The bright start of a hard stroke that turns into the tone: "blium".
        static const double kSupport = knob("PJ_VIB_SUPPORT", 0.2);
        for (int k = 0; k < 6; ++k) {
          const double fk = f * r[k];
          d.freq[k] = float(fk);
          d.t60[k] = float(25 * std::pow(175 / fk, 0.75) * (k == 0 ? 1.0 : kSupport));
          d.exc[k] = exc[k];
          d.rad[k] = rad[k];
          d.side[k] = float(0.3 * rad[k] * (k % 2 ? 1 : -1));
        }
        const double fa = f * std::exp2(-5 / 1200.0), Q = 89 * std::pow(f / 175, -0.963);
        d.freq[kVibAir] = float(fa);
        d.t60[kVibAir] = float(6.91 / (M_PI * fa / Q));
        d.exc[kVibAir] = 0;
        static const double kAirRad = knob("PJ_VIBAIR", 6);  // the tube's mouth: a monopole
        d.rad[kVibAir] = float(kAirRad);
        d.side[kVibAir] = float(kAirRad * 0.4 * -px);  // low bars left, high right (the player's view; left = out + side)
        for (int k = 0; k < 6; ++k) d.side[k] += float(rad[k] * 0.4 * -px);
        // The bar's dense high modes (bending beyond the fourth, torsion's
        // overtones), ~6-16 kHz: only a hard mallet's short contact reaches
        // them, and they die in a fraction of a second (radiation and the
        // metal's losses grow fast up there): the bright "bl" of a hard stroke
        // that turns at once into the ringing tone. Their decay is a design
        // value (no measurement found): PJ_VIB_HIGHT60 at 8 kHz, ~f^-0.5.
        static const double kHiT60 = knob("PJ_VIB_HIGHT60", 0.15), kHiExc = knob("PJ_VIB_HIGHEXC", 0.35);
        for (int k = 0; k < kVibHigh; ++k) {
          const double fk = std::min(17000.0, f * r[3] * std::pow(1.32, k + 1) * (1 + 0.03 * std::sin(7.3 * (k + 1) + i)));
          const int m = 7 + k;
          d.freq[m] = float(fk);
          d.t60[m] = float(kHiT60 * std::sqrt(8000 / fk));
          d.exc[m] = float(kHiExc * (k % 2 ? 0.7 : 1.0) / (1 + 0.15 * k));
          d.rad[m] = 0.5f;
          d.side[m] = float(0.5 * 0.4 * -px * (k % 2 ? 1 : -1));
        }
        d.nl = 1;
        d.lin[0] = {0, kVibAir, float(7e3 * std::pow(f / 175, 1.739))};
        S.setVib(i, d);
        Str st;
        st.vib = i;
        st.f0 = f;
        strings.push_back(st);
      }
      gain = 1.0;
      break;
    }
    case kTinkle:
    case kAgogo:
    case kWoodblock:
    case kTaiko: {
      pool = type_ == kTaiko ? 6 : 12;
      // (A mezzo-forte hit near a tenth of full scale.)
      gain = type_ == kTinkle ? 0.0008 : type_ == kAgogo ? 0.02 : type_ == kWoodblock ? 0.03 : 0.4;
      for (int i = 0; i < pool; ++i) {
        ModalSet::Def d;
        d.n = 1;
        d.freq[0] = 440;
        d.t60[0] = 0.1f;
        S.setVib(i, d);
        Str st;
        st.vib = i;
        strings.push_back(st);
      }
      break;
    }
    case kSteelpan: {
      // One note area per key (C4..F#6), all on one pan: they share its
      // skirt (a light body: the octave notes ring in sympathy). Each note's
      // modes 0, 1, 2 coupled as Monteil et al. measured (G3: alpha5..8 =
      // 0.24, 0.34, 0.88, 0.53 in mm and time x the fundamental): the octave
      // takes the energy within ~20 ms and gives it back, the steel pan's bloom.
      const ModeList m{6, kPanRatio, kPanT60, kPanAmp, 1.0f, 440, 0.002f};
      for (int k = 0; k < 31; ++k) {
        ModalSet::Def d;
        const double f = hz(60 + k), w = 2 * M_PI * f;
        struckDef(d, m, f, 0.015, 77u + k);
        // x0'' += -c5 x0 x1, x1'' += -c6 x0^2 - c7 x1 x2, x2'' += -c8 x1^2.
        const double w2 = w * w * 1000;
        d.nq = 4;
        d.quad[0] = {0, 0, 1, float(0.24 * w2)};
        d.quad[1] = {1, 0, 0, float(0.34 * w2)};
        d.quad[2] = {1, 1, 2, float(0.88 * w2)};
        d.quad[3] = {2, 1, 1, float(0.53 * w2)};
        static const float part[6] = {0.1f, 0.06f, 0.04f, 0.02f, 0.02f, 0.02f};
        supportCoupling(d, part);
        S.setVib(k, d);
        Str st;
        st.f0 = f;
        st.vib = k;
        strings.push_back(st);
      }
      // The skirt and the drum's shell, ~19 dB under a note.
      body({180, 330, 610}, {0.3f, 0.2f, 0.15f}, {2.0f, 2.0f, 2.0f}, {0.2f, 0.2f, 0.2f}, {0.2f, -0.2f, 0.2f});
      gain = 0.05;
      break;
    }
    case kRevCymbal: {
      // research/bells_blocks.md 4: a 16 in crash, rendered once and kept
      // backwards (the swell; as GM modules play it). Its modes are dense
      // (~14 Hz apart: 192 of them here, log-spread from its lowest, ~81 Hz,
      // to 14 kHz), the damping measured as xi = 8e-3 w^-0.4 (t60 18 s at
      // 100 Hz, 4.5 s at 1 kHz, 1.7 s at 5 kHz); struck by a stick for ~1 ms.
      ModalSet c(fs_);
      uint32_t r = 1234567u;
      auto rnd = [&]() { r = r * 1664525u + 1013904223u; return (r >> 8) * (1.0 / 16777216.0); };
      for (int v = 0; v < 3; ++v) {
        ModalSet::Def d;
        d.n = ModalSet::kModes;
        d.modalMass = 0.05f;
        d.zShare = 1;
        d.split = 0.002f;
        for (int k = 0; k < d.n; ++k) {
          const double f = 81 * std::pow(14000.0 / 81, (v + 3.0 * k + rnd()) / (3.0 * d.n));
          d.freq[k] = float(f);
          d.t60[k] = float(4.5 * std::pow(1000 / f, 0.6));
          d.exc[k] = float(0.6 + 0.4 * rnd()) * (rnd() < 0.5 ? -1 : 1);
          // (The energy moves up the modes in the first 100 ms of a real
          // crash; here the highs are struck and radiate the more.)
          d.rad[k] = float(std::pow(f / 1000, 0.8));
          d.side[k] = float(0.5 * d.rad[k] * (rnd() < 0.5 ? -1 : 1));
        }
        c.setVib(v, d);
      }
      for (int v = 0; v < 3; ++v) c.strike(v, 0.05, 4, 2e9, 1.5);
      const int n = int(3.0 * fs_);
      rev.resize(n);
      double peak = 1e-9;
      for (int i = 0; i < n; ++i) {
        rev[i] = c.sample();
        peak = std::max(peak, double(std::fabs(rev[i])));
      }
      for (float& x : rev) x = float(x / peak * 0.3);
      gain = 1.0;
      break;
    }
    default:
      break;  // (the sound effects: sfx.cc)
  }
}

ModalPlayer::ModalPlayer(ModalModel& m) : Performer(m), m_(m), s_(m.set()) {
  std::fill(keyOf_, keyOf_ + ModalSet::kMaxVib, -1);
  std::fill(vibOf_, vibOf_ + 128, -1);
}

void ModalPlayer::seed(unsigned s) { rng_ = 4711u + 7919u * s; }

bool ModalPlayer::setKnob(int k, double x) {
  if (m_.type() != ModalModel::kVibraphone || (k != 3 && k != 4)) return false;
  if (k == 3) fanDepth_ = x;
  else fanRate_ = 0.7 + 4.3 * x;
  return true;
}

void ModalPlayer::setHold(bool h) {
  hold_ = h;
  if (h) return;
  for (int k = 0; k < 128; ++k)
    if (!held_[k] && vibOf_[k] >= 0) noteOff(k);
}

// The string for a key: the highest whose open note is not above it, free if
// possible (the low positions); the strings beyond the playable ones (a
// sitar's chikari and tarafs) are not fingered.
int ModalPlayer::pickString(int midi) const {
  const double f = hz(midi);
  int playable = int(m_.strings.size());
  if (m_.type() == ModalModel::kSitar) playable = 2;
  if (m_.type() == ModalModel::kBanjo) playable = 5;
  int best = -1;
  for (int i = 0; i < playable; ++i) {
    const ModalModel::Str& st = m_.strings[i];
    if (st.f0 > f * 1.0001) continue;
    if (m_.type() == ModalModel::kBanjo && i == 0 && std::fabs(12 * std::log2(f / st.f0)) > 0.01) continue;  // the 5th: open only
    if (best < 0 || (keyOf_[st.vib] < 0 && keyOf_[m_.strings[best].vib] >= 0) ||
        ((keyOf_[st.vib] < 0) == (keyOf_[m_.strings[best].vib] < 0) && st.f0 > m_.strings[best].f0))
      best = i;
  }
  if (best < 0) {  // below every string: the lowest, tuned down (a scordatura)
    best = 0;
    for (int i = 1; i < playable; ++i)
      if (m_.strings[i].f0 < m_.strings[best].f0) best = i;
  }
  return best;
}

int ModalPlayer::nextInPool() {
  // The quietest of the pool (a ringing one keeps ringing; a held key's
  // only a little preferred: stopping a loud one dead is a click).
  int best = 0;
  double e = 1e30;
  for (int k = 0; k < m_.pool; ++k) {
    const int i = (next_ + k) % m_.pool;
    const double ei = s_.energy(i) * (keyOf_[i] >= 0 && held_[keyOf_[i]] ? 4 : 1);
    if (ei < e) { e = ei; best = i; }
  }
  next_ = (best + 1) % m_.pool;
  return best;
}

void ModalPlayer::noteOn(int midi, int velocity) {
  if (midi < 0 || midi > 127) return;
  const ModalModel::Type t = m_.type();
  const Instrument& in = m_.instrument();
  if (midi < in.lo || midi > in.hi) return;
  held_[midi] = true;
  const double v = std::clamp(velocity / 127.0, 0.02, 1.0);
  const double dyn = std::clamp(0.4 + 0.6 * dyn_ + breathCtl_, 0.2, 1.6);
  // A struck or plucked thing: the hardest key the hardest hit, the stroke's
  // speed (or the pluck's force) ~ velocity^1.6 from nothing. The A-weighted
  // level then spans ~30 dB from velocity 16 to 127, as a General MIDI player
  // does (tools/velcurve.py: the faster stroke's shorter contact is also
  // brighter, so ^2 gave 36-48 dB; with a floor, as before, a pianissimo key
  // was still struck at a fifth of full speed: 4-16 dB).
  // (Struck hard bars, bells and the taiko's head: a softer stroke stays on
  // them longer, its strike darker, A-weighted the quieter: 47-55 dB at ^1.6;
  // their stroke's speed ~ velocity^1.2, the bells' ~ velocity.)
  const bool stiff = t == ModalModel::kVibraphone || t == ModalModel::kTinkle || t == ModalModel::kTaiko;
  // (The vibraphone's mallet also hardens with the velocity, brighter and
  // louder on top: its speed ~ velocity^1.2 keeps the ~30 dB.)
  static const double kVibExp = knob("PJ_VIB_VELEXP", 1.2);
  const double ex = t == ModalModel::kTinkle ? 1.0 : t == ModalModel::kVibraphone ? kVibExp : stiff ? 1.2 : 1.6;
  const double amp = std::min(1.0, std::pow(v, ex) * dyn);
  if (m_.sfx) {
    // (A hard key while the hall applauds: someone shouts "bravo"; the
    // applause goes on.)
    if (velocity >= 100 && m_.sfx->applauding()) {
      held_[midi] = false;
      m_.sfx->bravo(v);
      return;
    }
    m_.sfx->noteOn(midi, std::min(1.0, v * dyn));
    return;
  }
  // A key already sounding: let it go first.
  if (vibOf_[midi] >= 0) keyOf_[vibOf_[midi]] = -1;
  int vib = -1;
  switch (t) {
    case ModalModel::kFiddle: {
      // The bow: a new stroke on the key's string (the lower positions), the
      // other way; the last string's bow lifts. Speed and pressure from the
      // key (STK's units: speed 0.03..0.23, pressure 0..1, here 0.3..0.85).
      const int si = pickString(midi);
      vib = si;
      if (keyOf_[vib] >= 0 && keyOf_[vib] != midi) vibOf_[keyOf_[vib]] = -1;
      for (int k = 0; k < 4; ++k)
        if (k != si) { m_.bowed[k].velTo = 0; m_.bowed[k].pressTo = 0; m_.bowed[k].damp = 0.99f; }
      ModalModel::Bowed& b = m_.bowed[si];
      b.f = b.fNow = hz(midi);
      b.beta = std::clamp(0.08 * m_.strings[si].f0 / b.f * 1.3, 0.05, 0.14);
      b.active = true;
      b.damp = 0.995f;
      bowDir_ = -bowDir_;
      bowKey_ = midi;
      bowClock_ = 0;
      b.velTo = bowDir_ * (0.05 + 0.18 * v);
      b.pressTo = 0.3 + 0.55 * std::min(1.0, amp);
      break;
    }
    case ModalModel::kSitar:
    case ModalModel::kBanjo:
    case ModalModel::kShamisen: {
      // A sitar's meend: a soft key above a held one on the main string,
      // within a fifth, is the string pulled sideways along its fret up to
      // it (the tension rising), no new stroke.
      if (t == ModalModel::kSitar && velocity <= 45) {
        const int mv = m_.strings[0].vib, held = keyOf_[mv];
        if (held >= 0 && midi > held && midi - held <= 7 && sinceOn_[mv] < 4) {
          vibOf_[held] = -1;
          keyOf_[mv] = midi;
          vibOf_[midi] = mv;
          const double fs0 = m_.strings[0].fStop > 0 ? m_.strings[0].fStop : m_.strings[0].f0;
          s_.retune(mv, hz(midi) / fs0, 0.12 + 0.03 * (midi - held));
          return;
        }
      }
      const int si = pickString(midi);
      const ModalModel::Str& st = m_.strings[si];
      vib = st.vib;
      if (keyOf_[vib] >= 0 && keyOf_[vib] != midi) vibOf_[keyOf_[vib]] = -1;
      s_.damp(vib, 0);
      // The finger stops the string (its exc: the pluck place, now a larger
      // share of the shorter string, made with it).
      m_.stopString(si, hz(midi));
      {
        const double F = (t == ModalModel::kSitar ? 6 : t == ModalModel::kShamisen ? 5 : 3) * 1.25 * amp;
        const double rise = t == ModalModel::kBanjo ? 0.002 : 0.0015, slip = 0.0002;
        s_.pluck(vib, F, rise, 0.001, slip, 0.5 + 0.3 * urand());
        // The shamisen's bachi meets the skin a moment after the string (its
        // snap: ~20-50 % of the attack).
        if (t == ModalModel::kShamisen) s_.knock(-6 * amp, 0.0008, 0.001);
        if (t == ModalModel::kSitar && velocity > 90) {  // the chikari, struck after a loud stroke
          for (int c = 3; c < 5; ++c) s_.pluck(m_.strings[c].vib, 1.5 * amp, 0.0015, 0.001, 0.0002, 0.7);
        }
      }
      break;
    }
    case ModalModel::kKoto: {
      // The string tuned nearest below; pressed behind its bridge (oshide) up
      // to two semitones, the press reaching its pitch just after the pluck.
      int best = -1;
      double bd = 1e9;
      for (int i = 0; i < 13; ++i) {
        const double d = 12 * std::log2(hz(midi) / m_.strings[i].f0);
        if (d < -0.01 || d > 2.01) continue;
        if (d < bd) { bd = d; best = i; }
      }
      if (best < 0) {  // nothing within reach: the nearest string, retuned (its bridge moved)
        for (int i = 0; i < 13; ++i) {
          const double d = std::fabs(12 * std::log2(hz(midi) / m_.strings[i].f0));
          if (d < bd) { bd = d; best = i; }
        }
      }
      vib = m_.strings[best].vib;
      if (keyOf_[vib] >= 0 && keyOf_[vib] != midi) vibOf_[keyOf_[vib]] = -1;
      s_.damp(vib, 0);
      const double ratio = hz(midi) / m_.strings[best].f0;
      if (ratio > 1.01 && ratio < 1.13) {
        s_.retune(vib, 1 + 0.8 * (ratio - 1));
        s_.retune(vib, ratio, 0.06);
      } else {
        s_.retune(vib, ratio);
      }
      s_.pluck(vib, 6.25 * amp, 0.002, 0.001, 0.0003, 0.6);
      break;
    }
    case ModalModel::kKalimba: {
      vib = nextInPool();
      if (keyOf_[vib] >= 0) vibOf_[keyOf_[vib]] = -1;
      if (s_.energy(vib) < 1e-6) s_.silence(vib);
      ModalSet::Def d = s_.def(vib);
      const double r = hz(midi) / d.freq[0];
      for (int k = 0; k < d.n; ++k) d.freq[k] = float(d.freq[k] * r);
      // Shorter tines (higher) ring a little shorter.
      d.t60[0] = float(3.4 * std::pow(262 / hz(midi), 0.4));
      static const float part[3] = {0.25f, 0.13f, 0.08f};
      supportCoupling(d, part);
      s_.setVib(vib, d);
      s_.pluck(vib, 3.125 * amp, 0.02, 0.002, 0.00015, 1.4);  // a slow press, the nail's quick release
      break;
    }
    case ModalModel::kTinkle:
    case ModalModel::kAgogo:
    case ModalModel::kWoodblock:
    case ModalModel::kTaiko: {
      vib = nextInPool();
      if (keyOf_[vib] >= 0) vibOf_[keyOf_[vib]] = -1;
      // (Another bell: the quietest one's last ring is cut, if it is all but
      // gone; still ringing, its motion carries over into the new tuning:
      // stopped dead, the sound would click.)
      if (s_.energy(vib) < 1e-6) s_.silence(vib);
      ModalSet::Def d;
      double mallet = 0.02, K = 1e8, alpha = 1.5, speed = 3.0 * amp, mass = 0.01;
      if (t == ModalModel::kTinkle && midi < 72) {
        // A church bell (below C5), struck by a soft felt hammer (felt
        // stiffens as it is pressed, F ~ c^2.5; a few ms on the bell: the
        // strike blooms, no click), much lighter than the bell (one clean
        // blow: a hammer as heavy as the bell's modal mass would bounce on the
        // rim and comb out the tierce).
        mass = 0.05;
        struckDef(d, {9, kChurchRatio, kChurchT60, kChurchAmp, 1.0f, 1000, 0.003f}, hz(midi), mass, rng_);
        for (int k = 0; k < d.n; ++k) d.rad[k] *= 3.5f;  // (as loud as the handbells above)
        mallet = 0.02; K = 1e9; alpha = 2.5;
      } else if (t == ModalModel::kTinkle) {
        mass = 0.005;
        struckDef(d, {10, kBellRatio, kBellT60, kBellAmp, 1.0f, 1000, 0.001f}, hz(midi), mass, rng_);
        mallet = 0.01; K = 5e9;  // the clapper: contact ~0.1 ms
      } else if (t == ModalModel::kAgogo) {
        // (The bell's modal mass against the stick's tip: a tip as heavy as
        // the bell rode its fundamental for half a cycle, the partials gone;
        // the many modes of a real shell make its point stiff, the contact
        // short.)
        static const double bellMass = knob("PJ_AGOGO_MASS", 0.08), tip = knob("PJ_AGOGO_TIP", 0.006);
        mass = bellMass;
        struckDef(d, {9, kAgogoRatio, kAgogoT60, kAgogoAmp, 0.0f, 800, 0.03f}, hz(midi), mass, rng_);
        mallet = tip; K = 3e9;  // a stick's tip: ~0.05 ms
      } else if (t == ModalModel::kWoodblock) {
        mass = 0.03;
        struckDef(d, {5, kBlockRatio, kBlockT60, kBlockAmp, 1.0f, 800, 0}, hz(midi), mass, rng_);
        // The stick on wood ~0.5 ms (a harder, shorter contact clicks: its
        // step in velocity spreads over every band); the lip's upper partials
        // a little down too: the knock's click ~8 dB under what it was.
        for (int k = 2; k < d.n; ++k) {
          d.exc[k] *= 0.8f;
          d.rad[k] *= 0.8f;
        }
        mallet = 0.02; K = 5e7;
      } else {
        // The head (cowhide ~3.4 kg/m^2, 48 cm): ~0.3 kg moving at its centre;
        // the bachi (~170 g) in contact 3-6 ms; a hard hit stretches the head:
        // up to a semitone sharp at first (tension modulation, settling in
        // ~0.25 s).
        mass = 0.3;
        struckDef(d, {9, kTaikoRatio, kTaikoT60, kTaikoAmp, 0.0f, 100, 0.002f}, hz(midi), mass, rng_);
        d.glide = 0.12f;
        d.glideRef = 3e-3f;
        mallet = 0.17; K = 3e6; speed = 7.5 * amp;
      }
      s_.setVib(vib, d);
      s_.strike(vib, mallet, speed, K, alpha);
      break;
    }
    case ModalModel::kVibraphone: {
      // research/vibraphone.md: a yarn mallet (~22 g; the wound head stiffens
      // as it is pressed, alpha 2.5): K 2.6e10 a medium one (1.5 ms on the
      // bar), the hardness knob from soft (2.3e9, 3 ms) to hard (~3e11, 0.7 ms).
      // The mallet's material follows the velocity: a soft stroke lands on the
      // yarn alone (softer, longer on the bar, darker), a hard one goes
      // through it to the firm core (harder, shorter, brighter): x0.75 of the
      // hardness at the softest, ~x1.05 at velocity 90, x1.35 at 127
      // (PJ_VIB_VELHARD: how much; 0 = one mallet for all).
      vib = midi - ModalModel::kVibLo;
      if (vib < 0 || vib >= ModalModel::kVibBars) return;
      s_.damp(vib, 0);
      {
        // The mallets: Mike Balter's Pro Vibe series, PJ_VIB_MALLET 21 (yellow
        // cord, hard), 22 (green, medium hard), 23 (blue, medium: the default),
        // 24 (red, soft), 25 (silver, "jazz": very hard, its thin
        // cord over the core firm from the first touch, alpha 2.0; the
        // instrument vibraphone-25r). Hardness factors on K^(1/3.5).
        static const int kMallet = int(knob("PJ_VIB_MALLET", 23));
        const int mallet = std::strcmp(m_.instrument().name, "vibraphone-25r") == 0 ? 25 : kMallet;
        // (All but the softest made harder, so a stroke's bright transient
        // changes quickly into the tone: the "blium".)
        const double base = mallet == 21 ? 2.0 : mallet == 22 ? 1.7 : mallet == 24 ? 0.6 : mallet == 25 ? 2.3 : 1.4;
        const double alpha = mallet == 25 ? 2.0 : 2.5;
        static const double kVel = knob("PJ_VIB_VELHARD", 1.0);
        const double w = std::clamp(velocity, 1, 127) / 127.0;
        const double h = hardness_ * base * (1 + kVel * (-0.25 + 0.6 * w * w));
        // (alpha 2.0 against 2.5: K rescaled to the same force at 0.2 mm.)
        const double K = 2.6e10 * std::pow(h, 3.5) * std::pow(2e-4, 2.5 - alpha);
        s_.strike(vib, 0.022, 2.5 * amp, K, alpha);
      }
      break;
    }
    case ModalModel::kSteelpan: {
      vib = midi - 60;
      if (vib < 0 || vib >= 31) return;
      s_.damp(vib, 0);
      // A rubber-tipped stick (~30 g, Hertz ~5e6: 1-2 ms on the note).
      s_.strike(vib, 0.03, 2.6 * amp, 5e6, 1.5);
      break;
    }
    case ModalModel::kRevCymbal: {
      // The swell: the crash backwards from 2.2 s before its hit, faster for higher keys.
      m_.revRate = std::exp2((midi - 60) / 24.0);
      m_.revPos = std::min(double(m_.rev.size() - 1), 2.2 * m_.fs() * m_.revRate);
      m_.revGain = 0.3 + amp;
      vibOf_[midi] = -1;
      return;
    }
    default:
      return;
  }
  if (vib >= 0) {
    keyOf_[vib] = midi;
    vibOf_[midi] = vib;
    sinceOn_[vib] = 0;
  }
}

void ModalPlayer::noteOff(int midi) {
  if (midi < 0 || midi > 127) return;
  held_[midi] = false;
  if (hold_) return;
  if (m_.sfx) {
    m_.sfx->noteOff(midi);
    return;
  }
  const int vib = vibOf_[midi];
  if (m_.type() == ModalModel::kRevCymbal) { m_.revPos = -1; return; }
  if (vib < 0) return;
  vibOf_[midi] = -1;
  if (keyOf_[vib] != midi) return;
  keyOf_[vib] = -1;
  switch (m_.type()) {
    case ModalModel::kFiddle:
      m_.bowed[vib].velTo = 0;
      m_.bowed[vib].pressTo = 0;
      m_.bowed[vib].damp = 0.99f;  // the finger lifts: the string dies in ~0.2 s
      if (bowKey_ == midi) bowKey_ = -1;
      break;
    case ModalModel::kBanjo:
    case ModalModel::kShamisen:
      s_.damp(vib, 25);  // the left hand lifts
      break;
    case ModalModel::kSitar:
      s_.damp(vib, 2.5);  // the finger leaves the fret, lying on: a slow fade
      break;
    case ModalModel::kSteelpan:
      s_.damp(vib, 3);
      break;
    case ModalModel::kVibraphone:
      s_.damp(vib, 25);  // the damper bar (pedal up): ~0.3 s
      break;
    default:
      break;  // the rest ring on
  }
}

void ModalPlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    const double bdt = m / m_.fs();
    clock_ += bdt;
    for (int v = 0; v < ModalSet::kMaxVib; ++v) sinceOn_[v] += bdt;
    // The bend (joystick): a sitar's meend pulls the string sideways (only up,
    // to a fourth); the others a semitone either way; a fiddle's vibrato.
    if (keyOf_[0] >= 0 || true) {
      const ModalModel::Type t = m_.type();
      if (t == ModalModel::kSitar || t == ModalModel::kBanjo || t == ModalModel::kShamisen || t == ModalModel::kFiddle ||
          t == ModalModel::kKoto) {
        vibPh_ += 2 * M_PI * 5.5 * bdt;
        for (int k = 0; k < 128; ++k) {
          const int v = vibOf_[k];
          if (v < 0) continue;
          if (t == ModalModel::kFiddle) {
            const double cents = 100 * roll_ + 100 * vibDepth_ * 4 * std::sin(vibPh_) * std::min(1.0, sinceOn_[v] / 0.4);
            m_.bowed[v].fNow = m_.bowed[v].f * std::exp2(cents / 1200);
            continue;
          }
          const ModalModel::Str& sv = m_.strings[v];
          const double base = hz(k) / (sv.fStop > 0 ? sv.fStop : sv.f0);
          double cents = t == ModalModel::kSitar ? 500 * std::max(0.0, roll_) : 100 * roll_;
          if (t == ModalModel::kFiddle) cents += 100 * vibDepth_ * 4 * std::sin(vibPh_) * std::min(1.0, sinceOn_[v] / 0.4);
          // Only when the bend moves (a meend's or an oshide's glide runs on).
          if (std::fabs(cents - lastCents_[v]) < 0.2) continue;
          lastCents_[v] = cents;
          s_.retune(v, base * std::exp2(cents / 1200));
        }
      }
    }
    // The vibraphone's fan: discs on a shaft over the tubes' mouths, open
    // twice a turn: the opening a rectified sine (open, the tube couples to
    // its bar fully; closed, ~15 %). The motor runs by default at the
    // slowest tremolo, 0.7 Hz; the mod wheel speeds it up to 5 Hz (live:
    // K5 its speed, K4 how deep, 0 the discs left open). (Not the voice's
    // vibrato: a vibist sets the motor and leaves it.)
    if (m_.type() == ModalModel::kVibraphone) {
      // (Tests: PJ_FAN the depth, PJ_FANHZ the tremolo's rate, in place of the knobs.)
      static const double kFan = knob("PJ_FAN", -1), kFanHz = knob("PJ_FANHZ", -1), kFanPh = knob("PJ_FANPH", 0);
      const double depth = kFan >= 0 ? kFan : std::clamp(fanDepth_, 0.0, 1.0);
      const double rate = kFanHz > 0 ? kFanHz : std::min(5.0, fanRate_ + 4.3 * std::clamp(flutter_, 0.0, 1.0));
      if (depth > 0) fanPh_ = std::fmod(fanPh_ + M_PI * rate * bdt, 2 * M_PI);
      // Two shafts, the naturals' and the accidentals', on one belt: in step, but the accidentals'
      // set apart by how its pulley was tightened (PJ_FAN_ROWS rad of disc angle, ~11 degrees) and
      // swaying a little against the other once a turn (the belt's give); each disc a few degrees
      // off its shaft's angle (its set screw: PJ_FAN_DISC rad at most, fixed for its seat).
      static const double kRows = knob("PJ_FAN_ROWS", 0.2), kDisc = knob("PJ_FAN_DISC", 0.06);
      const double rowAcc = kRows + 0.15 * kRows * std::sin(fanPh_);
      for (int v = 0; v < ModalModel::kVibBars; ++v) {
        const int pc = (ModalModel::kVibLo + v) % 12;
        const bool acc = pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
        uint32_t h = 2654435761u * uint32_t(v + 1);
        h ^= h >> 15;
        const double disc = kDisc * ((h & 0xffff) / 32767.5 - 1);
        const double open = 0.15 + 0.85 * std::fabs(std::cos(fanPh_ + kFanPh + disc + (acc ? rowAcc : 0)));
        s_.setCoupling(v, float(1 - depth * (1 - open)));
      }
    }
    m_.process(out + i, m);
  }
}

}  // namespace pajupilli
