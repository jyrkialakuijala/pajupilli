// The winds by ear (-play; research/fretless_bass.md 7.6): a brass or reed part tunes each held
// note to what the rest of the band sounds (src/band.h), by the fretless bass's rule
// (src/fretless.cc FretlessBassPlayer::steer; the lips slower), and its section's own just
// intervals are solved about it (Ensemble::retune). The rule is in src/ensemble.h's header.
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "band.h"
#include "heard.h"
#include "brasssect.h"
#include "ensemble.h"
#include "knob.h"

namespace pajupilli {

namespace {
// The pull is followed by its weight w as w / (w + kHalf) (the fretless's: a bare third under a
// note pulls ~0.01, a far partial or a faint note ~1e-4); under kFloor nothing is heard and the
// note heals to 0 c in kHealTau.
constexpr double kHalf = 0.0005, kFloor = 3e-4, kHealTau = 3.0;
constexpr double kEvery = 0.05;  // (heard again this often, and whenever the band changes)
// The lips follow more slowly than the fretless's finger (1 c/ms, 25 ms): a held note retuned by
// the embouchure takes 0.1 s or more, at most 0.2 c/ms (a chord changing under it: -14 c in
// ~0.1 s); a change under 3 c is let be (the fretless: 2), so a busy texture's passing notes,
// pulling a few cents this way and that, are not chased.
constexpr double kSlew = 200, kQuickest = 0.1, kLetBe = 3;
}  // namespace

int Ensemble::windEar(const Instrument& in) {
  static const bool kOn = knob("PJ_WIND_EAR", 1) > 0;
  if (!kOn) return -1;
  if (brassSectionInstrument(in)) return kBandBrass;
  // (the choir: its singers hear the band as the winds do, src/choir.cc choirEar)
  if (std::strcmp(in.family, "choir") == 0) return knob("PJ_CHOIR_EAR", 1) > 0 ? kBandChoir : -1;
  const int fam = bandFamily(in.family, in.name);
  // (The lips and the reeds blown: the free reeds are organs' family, synth brass a circuit.)
  if (in.lip() && fam == kBandBrass) return kBandBrass;
  if (in.reed() && (fam == kBandReed || fam == kBandClarinet)) return fam;
  // (The bowed strings, a stopping finger: the violin, the viola, the cello, alone or in the
  // string ensembles; src/bassinst.cc BowPlayer::render. Not the double bass, the band's ground.)
  static const bool kStrings = knob("PJ_STRING_EAR", 1) > 0;
  if (kStrings && fam == kBandStrings &&
      (std::strcmp(in.name, "violin") == 0 || std::strcmp(in.name, "viola") == 0 || std::strcmp(in.name, "cello") == 0 ||
       std::strcmp(in.name, "strings") == 0 || std::strcmp(in.name, "slow-strings") == 0 ||
       std::strcmp(in.name, "tremolo-strings") == 0 || std::strcmp(in.name, "pizzicato-strings") == 0))
    return kBandStrings;
  return -1;
}

void Ensemble::setBand(const SoundingSet* band, int self, int fam) {
  band_ = band;
  self_ = self;
  earFam_ = fam;
  earLim_ = knob("PJ_WIND_EAR_LIM", 15);
  earLog_ = int(knob("PJ_WIND_EAR_LOG", 0));
  // (A lone brass player sets its lips for the pull; the brass section's seats are set for their
  // tuning by their own players, src/hornplayer.cc, and their roll is the section's bend.)
  rollFeed_ = fam >= 0 && !brass_ && !f_.empty() && f_[0].v->instrument().lip();
  if (brass_)  // (the brass section's players: its seats listen)
    for (Section& k : kids_) k.ens->setBand(band, self, fam);
  if (strings_) stringsBand(band, self, fam);  // (the string ensemble's players, src/stringsect.cc)
}

// The band's pull on the voice in `slot` playing `note` (c, before the weight's share), and its
// weight: its own template against the others' (bandPull), in the order of hearing.
double Ensemble::bandHear(int slot, int note, double* weight, double* earShare) const {
  // (a shared string orchestra's player: the part whose key it plays, src/stringsect.cc)
  // (the choir asks for its singers, which are not this ensemble's own voices: no slot of ours)
  const int self = slot >= 0 && slot < int(f_.size()) && f_[slot].earSelf >= -1 ? f_[slot].earSelf : self_;
  const double f0 = 440 * std::exp2((note - 69) / 12.0);
  double pl[16], pw[16], pk[16];
  int np = 0;
  for (int k = 1; k <= 16 && k * f0 < 5000; ++k) {
    pl[np] = std::log2(double(k));
    pw[np] = bandPartial(earFam_, f0, k);
    pk[np] = k;
    ++np;
  }
  BandListener me;
  me.self = self;
  me.f0 = f0;
  me.pl = pl;
  me.pw = pw;
  me.pk = pk;
  me.np = np;
  me.pitch = note;
  // (Its note's onset; a note of its own the band does not have, a voicing's added note in the
  // brass section: as new, hearing every note before it.)
  const BandNote* own = band_->find(self, note);
  me.at = own ? own->at : band_->now();
  if (own) me.pitch = own->pitch;
  double we = 0;
  const double pull = bandPull(*band_, me, weight, &we);
  // (what of it is other winds': nothing heard, all of it, the note free to move with them)
  *earShare = *weight >= kFloor ? we / *weight : 1.0;
  return pull;
}

bool Ensemble::bandEar(double dt) {
  bool moved = false;
  for (int i = 0; i < int(f_.size()); ++i) {
    Slot& s = f_[i];
    const Performer& p = s.v->player();
    if (!s.held || p.sounding() < 0) {
      s.earNote = -1;  // (let go: its tuning stays as it rings out; the next note lands anew)
      continue;
    }
    const int note = p.sounding();
    double w;
    if (note != s.earNote) {
      // A new note: heard at once, and played there from its start.
      const double pull = bandHear(i, note, &w, &s.earShare);
      s.earNote = note;
      s.band = s.bandTo = std::clamp(pull * w / (w + kHalf), -earLim_, earLim_);
      s.bandMix = w;
      s.bandAt = now_;
      s.bandVersion = band_->version();
      if (earLog_)
        say(stderr, "wind ear: t=%.3f ch %d %s note %d pull %+.1f c (w %.5f) -> %+.1f c\n", band_->now(), self_,
                     s.v->instrument().name, note, pull, w, s.band);
      continue;
    }
    // Heard again when the band changes and every 50 ms.
    if (band_->version() != s.bandVersion || now_ - s.bandAt > kEvery) {
      s.bandVersion = band_->version();
      s.bandAt = now_;
      const double want = std::clamp(bandHear(i, note, &w, &s.earShare) * w / (w + kHalf), -earLim_, earLim_);
      s.bandMix = w;
      // (A change under 3 c let be; with next to nothing to hear, back to the tempered pitch.)
      if (w >= kFloor && std::fabs(want - s.bandTo) >= kLetBe) {
        if (earLog_ >= 2)
          say(stderr, "wind ear: t=%.3f ch %d note %d to %+.1f c (w %.5f)\n", band_->now(), self_, note, want, w);
        s.bandTo = want;
      }
    }
    // The winds' common healing (SoundingSet::drift): every held wind note at once, as far as
    // its pull is not the fixed-pitch parts' (a note hearing nothing: all of it).
    if (band_->drift() != 0 && s.earShare > 0) {
      const double move = band_->drift() * s.earShare * dt;
      s.band = std::clamp(s.band + move, -earLim_, earLim_);
      s.bandTo = std::clamp(s.bandTo + move, -earLim_, earLim_);
    }
    const double d = s.bandTo - s.band;
    if (s.bandMix < kFloor) {
      s.band -= s.band * std::min(1.0, dt / kHealTau);  // (healing, ~3 s)
      if (std::fabs(s.bandTo) >= kLetBe) s.bandTo = 0;
    } else if (d != 0) {
      const double step = std::min(kSlew, std::max(std::fabs(d) / kQuickest, 10.0)) * dt;
      s.band = std::fabs(d) <= step ? s.bandTo : s.band + std::copysign(step, d);
    }
    if (s.tunedNote == note && std::fabs(s.band - s.bandSolved) >= 1) moved = true;  // (retune's to solve again)
  }
  return moved;
}

void Ensemble::bandSend() {
  for (Slot& s : f_) {
    if (s.earNote < 0) continue;
    // (A voice the section's solve has tuned: its solution, made about the pull (retune); one it
    // has not yet solved keeps what it had; a voice alone, or with PJ_JI=0: the pull.)
    if (f_.size() > 1 && ji_ && s.tunedNote != s.earNote) continue;
    const double c = (s.tunedNote == s.earNote ? s.cents : s.band) * jiScale_;
    if (earLog_ >= 2 && c != s.shown) {
      say(stderr, "wind tune: t=%.4f ch %d note %d band %+.2f -> %+.2f c\n", band_->now(), self_, s.earNote, s.band, c);
      s.shown = c;
    }
    if (c == s.sent) continue;
    s.sent = c;
    s.v->player().setTuning(c);
  }
  if (rollFeed_) bandRoll();
}

// A lone brass player's lips: set for the pull as for the pitch wheel (the lip roll: the lips'
// resonance x e^(0.06 roll), PJ_ROLLLIP; ~600 c for the factor's log, the ear's own slope: 36 c a
// unit), over the wheel's own roll; its ear (src/hornplayer.cc: a slow trim of at most 0.8 %, 5 c
// let be) alone reaches only ~5 c.
void Ensemble::bandRoll() {
  static const double kCentsPerRoll = 600 * knob("PJ_ROLLLIP", 0.06);
  for (Slot& s : f_) s.v->player().setLipRoll(wheel_ + s.sent / kCentsPerRoll);
}

void Ensemble::bandRhs(int n, const int* idx, const bool* young, int ny, double kMean, int top, double topPull, double* b) const {
  if (kMean > 0) {  // kMean (sum of the young's c / ny - their pulls' mean)^2
    double mb = 0;
    for (int i = 0; i < n; ++i)
      if (young[i]) mb += f_[idx[i]].band;
    mb /= ny;
    for (int i = 0; i < n; ++i)
      if (young[i]) b[i] += kMean / ny * mb;
  }
  if (top >= 0) b[top] += topPull * f_[idx[top]].band;
}

double Ensemble::keyShare(int key) const {
  if (!listens() || key < 0 || key > 127) return 0;
  if (choir_) return choirKey(key, true);
  if (brass_) {
    for (const Section& k : kids_)
      for (const Slot& s : k.ens->f_)
        if (s.earNote >= 0 && (s.earNote - key) % 12 == 0) return s.earShare;
    return 0;
  }
  const int o = owner_[key];
  return o < 0 || f_[o].earNote < 0 ? 0.0 : f_[o].earShare;
}

double Ensemble::keyCents(int key) const {
  if (!listens() || key < 0 || key > 127) return 0;
  if (choir_) return choirKey(key, false);
  if (brass_) {  // (the seat playing it: the key's own note, or an octave of it)
    for (const Section& k : kids_) {
      const Ensemble& kid = *k.ens;
      for (const Slot& s : kid.f_)
        if (s.earNote == key) return s.sent;
      for (const Slot& s : kid.f_)
        if (s.earNote >= 0 && (s.earNote - key) % 12 == 0) return s.sent;
    }
    return 0;
  }
  const int o = owner_[key];
  if (o < 0 || f_[o].earNote < 0) return 0;
  return f_[o].sent;
}

}  // namespace pajupilli
