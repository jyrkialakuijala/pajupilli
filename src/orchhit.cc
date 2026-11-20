// The orchestra hit (General MIDI 56): a symphony orchestra's tutti stab, played, not a sample.
//
// The sampler's orchestra hit is one recorded chord (the first sfff eighth of the Infernal Dance of
// Stravinsky's Firebird, 1919 suite) moved up and down the keyboard: an octave away the piccolo is a
// bass flute or a whistle, the tuba a trumpet. Here each key is scored for the orchestra instead: the
// key's note in octaves, every section in its own register, as that page is laid out (piccolo, flutes,
// oboes, clarinets, bassoons, horns, trumpets, trombones, tuba, bass drum, piano, violins, cellos, basses).
// A section's notes are the key plus its octave offsets, each folded by octaves into the range where
// the section sounds well, so the voicing changes with the key as an orchestrator's would: up the
// keyboard the bass instruments stay in the bass and the piccolo stops climbing.
//
// The orchestra plays as long as the key is held (these are players, not a recording: the chord
// sustains as a tutti would), and at least PJ_HIT_MIN seconds (0.10) for a file's very short note.
// PJ_HIT_LEN > 0: an eighth note instead, the wind, brass and string players stop after that long
// (the hall and the piano's and drum's ring carry on).
// Not only a thump: as on the page, the piccolo and the first flute run up into their note and the piano
// rolls its chord (hitOn).
//
// Not built yet, so not playing: the harp (the timpani are built but not yet in the hit; the taiko stands in for the bass drum; the
// cello plays the violas' notes too; one fiddle the violins').
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "ensemble.h"
#include "knob.h"

namespace pajupilli {

namespace {

const NoteInfo kHitNotes[128] = {
#define N {"", 1}
#define N8 N, N, N, N, N, N, N, N
    N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8, N8
#undef N8
#undef N
};
const Design kHitDesign = {nullptr, 0, nullptr, 0, kHitNotes, 0, 0, false, nullptr, 0, {}, {}};

// A section: its instrument, how many players, their notes as semitones from the key (each folded by
// octaves into lo..hi, sounding), its weight in the mix, and whether its players stop with the eighth.
struct SectionDef {
  const char* inst;
  int players;
  int offs[5];
  int nOffs;
  int lo, hi;
  double gain;
  bool stops;
  bool run = false;   // slurs a quick figure up into its note (the page's grace notes)
  bool roll = false;  // rolls its chord from the bottom
};
const SectionDef kSections[] = {
    {"piccolo", 1, {36}, 1, 81, 103, 0.5, true, true},
    {"flute", 1, {24}, 1, 72, 96, 0.7, true, true},
    {"flute", 1, {12}, 1, 67, 91, 0.7, true},
    {"oboe", 2, {12, 24}, 2, 60, 84, 0.7, true},
    {"clarinet", 2, {12, 0}, 2, 52, 82, 0.7, true},
    {"bassoon", 2, {-12, -24}, 2, 34, 60, 0.8, true},
    {"horn", 2, {0, -12}, 2, 43, 72, 1.4, true},
    {"trumpet", 2, {12, 0}, 2, 55, 82, 1.0, true},
    {"trombone", 2, {0, -12}, 2, 40, 70, 1.0, true},
    {"tuba", 1, {-24}, 1, 28, 52, 1.4, true},
    {"fiddle", 1, {24, 12, 0}, 3, 55, 93, 1.4, true},
    {"cello", 1, {0, -12}, 2, 36, 69, 1.0, true},
    {"contrabass", 1, {-24}, 1, 28, 52, 1.0, true},
    {"grand-piano", 1, {-24, -12, 0, 12, 24}, 5, 24, 103, 0.4, false, false, true},
    {"taiko", 1, {-24}, 1, 26, 37, 0.7, false},
};

}  // namespace

extern const Instrument kOrchestraHit = {"orchestra-hit", 0, 24, 96, 1, 1, 1, 48000, 0, false, false, 0, 1.0,
                                         Kind::kModal, &kHitDesign, "orchestra"};

// The string ensembles (General MIDI 49, 50; src/stringsect.cc): E1 .. G7, sounding.
extern const Instrument kStringEnsemble1 = {"strings", 0, 28, 103, 1, 1, 1, 48000, 0, false, false, 0, 1.0,
                                            Kind::kModal, &kHitDesign, "orchestra"};
extern const Instrument kStringEnsemble2 = {"slow-strings", 0, 28, 103, 1, 1, 1, 48000, 0, false, false, 0, 1.0,
                                            Kind::kModal, &kHitDesign, "orchestra"};
// ... and the same section in tremolo (GM 45: every bow shaking back and forth, each player on its
// own clock) and pizzicato (GM 46: every key plucked, the nail; a few dB under the bowed section).
extern const Instrument kStringTremolo = {"tremolo-strings", 0, 28, 103, 1, 1, 1, 48000, 0, false, false, 0, 1.0,
                                          Kind::kModal, &kHitDesign, "orchestra"};
extern const Instrument kStringPizzicato = {"pizzicato-strings", 0, 28, 103, 1, 1, 1, 48000, 0, false, false, 0, 1.0,
                                            Kind::kModal, &kHitDesign, "orchestra"};

void Ensemble::initOrchestra(const RoomSpec* room) {
  static const double kGain = knob("PJ_HIT_GAIN", 0.45);
  lo_ = kOrchestraHit.lo;
  hi_ = kOrchestraHit.hi;
  for (const SectionDef& d : kSections) {
    const Instrument* in = findInstrument(d.inst);
    static const char* const only = std::getenv("PJ_HIT_ONLY");  // (listening to one section: its instrument's name)
    if (!in || (only && std::strcmp(only, d.inst) != 0)) continue;
    kidCals_.emplace_back(new Calibration(std::getenv("PJ_NOCAL") ? Calibration() : calibrate(*in, 440, nullptr)));
    Section s;
    s.ens.reset(new Ensemble(std::vector<Member>(d.players, Member{in, kidCals_.back().get()}), false, room));
    s.ens->quiet_ = true;
    s.offs.assign(d.offs, d.offs + d.nOffs);
    s.lo = d.lo;
    s.hi = d.hi;
    s.gain = float(d.gain * kGain);
    s.stops = d.stops;
    s.lead = noteLead(*in);
    s.run = d.run;
    s.roll = d.roll;
    kids_.push_back(std::move(s));
  }
}

// A key: the tutti's notes per section, as timed events. The run-ups (the page's grace notes into
// the chord: piccolo and first flute slur up a quick figure to their note, the piano rolls its chord
// from the bottom) take kLead seconds. Played from a file the key comes that much early (noteLead)
// and the tutti lands on the beat; live the tutti sounds at once and the runs arrive kLead later.
void Ensemble::hitOn(int key, int velocity) {
  static const double kLead = knob("PJ_HIT_LEAD", 0.12);
  key = std::clamp(key, lo_, hi_);
  hitOff(key, true, true);
  Hit h;
  h.key = key;
  h.tuttiAt = now_ + (noteLeadsApplied() ? kLead : 0);
  const double runEnd = now_ + kLead;
  for (Section& s : kids_) {
    const int k = int(&s - kids_.data());
    std::vector<int> notes;
    for (int o : s.offs) {
      int p = key + o;
      while (p > s.hi) p -= 12;
      while (p < s.lo) p += 12;
      if (p > s.hi) continue;  // (a range under an octave: nothing fits)
      if (std::find(notes.begin(), notes.end(), p) == notes.end()) notes.push_back(p);
    }
    // (a note another held hit already plays in this section stays that hit's)
    for (const Hit& other : hits_)
      for (const HitEvent& e : other.ev)
        if (e.section == k && e.on) notes.erase(std::remove(notes.begin(), notes.end(), e.pitch), notes.end());
    if (notes.empty()) continue;
    if (s.run) {
      // Three grace notes up to the note, slurred (each key down before the last is let go).
      static const int kSteps[3] = {-7, -5, -2};
      int prev = -1;
      for (int i = 0; i <= 3; ++i) {
        const int p = i < 3 ? notes[0] + kSteps[i] : notes[0];
        const double t = now_ + (runEnd - now_) * i / 3;
        if (p < s.lo - 7) continue;
        h.ev.push_back({t, k, p, i < 3 ? std::max(1, velocity - 25) : velocity, true});
        if (prev >= 0) h.ev.push_back({t + 0.012, k, prev, 0, false});
        prev = p;
      }
    } else if (s.roll) {
      std::sort(notes.begin(), notes.end());
      for (size_t i = 0; i < notes.size(); ++i)
        h.ev.push_back({now_ + (runEnd - now_) * double(i) / double(notes.size() - 1 ? notes.size() - 1 : 1), k, notes[i],
                        velocity, true});
    } else {
      // (a section that acts late on its keys, the cello for its pizzicato, is given them early)
      for (int p : notes) h.ev.push_back({std::max(now_, h.tuttiAt - s.lead), k, p, velocity, true});
    }
  }
  std::stable_sort(h.ev.begin(), h.ev.end(), [](const HitEvent& a, const HitEvent& b) { return a.t < b.t; });
  hits_.push_back(std::move(h));
}

// The key let go: the hit ends, but not before the tutti has sounded kMin seconds (a file's very
// short note is still a hit). force: at once (the key struck again).
void Ensemble::hitOff(int key, bool all, bool force) {
  static const double kMin = knob("PJ_HIT_MIN", 0.10);
  for (size_t i = 0; i < hits_.size(); ++i) {
    Hit& h = hits_[i];
    if (h.key != key) continue;
    if (all && !force && now_ < h.tuttiAt + kMin) { h.offAt = h.tuttiAt + kMin; continue; }
    for (size_t e = 0; e < h.next; ++e) {
      const HitEvent& ev = h.ev[e];
      if (!ev.on || (!all && !kids_[ev.section].stops)) continue;
      kids_[ev.section].ens->noteOff(ev.pitch);
    }
    if (all) {
      hits_.erase(hits_.begin() + i);
      --i;
    } else {
      // (the sections that stop play nothing more of this hit)
      for (size_t e = h.next; e < h.ev.size(); ++e)
        if (kids_[h.ev[e].section].stops) h.ev[e].section = -1;
    }
  }
}

void Ensemble::hitRender(float* left, float* right, int n) {
  static const double kLen = knob("PJ_HIT_LEN", 0);  // (> 0: the players stop after this long, a held key or not)
  std::fill(left, left + n, 0.0f);
  std::fill(right, right + n, 0.0f);
  hitL_.resize(n);
  hitR_.resize(n);
  for (int done = 0; done < n;) {
    for (size_t i = 0; i < hits_.size(); ++i) {
      Hit& h = hits_[i];
      for (; h.next < h.ev.size() && h.ev[h.next].t <= now_; ++h.next) {
        const HitEvent& e = h.ev[h.next];
        if (e.section < 0) continue;
        if (e.on) kids_[e.section].ens->noteOn(e.pitch, e.vel);
        else kids_[e.section].ens->noteOff(e.pitch);
      }
      if (kLen > 0 && !h.stopped && now_ - h.tuttiAt >= kLen) { hitOff(h.key, false, true); h.stopped = true; }
      if (h.offAt >= 0 && now_ >= h.offAt) { hitOff(h.key, true, true); --i; }
    }
    const int m = std::min(n - done, 128);
    for (Section& s : kids_) {
      s.ens->render(hitL_.data(), hitR_.data(), m);
      for (int i = 0; i < m; ++i) {
        left[done + i] += s.gain * hitL_[i];
        right[done + i] += s.gain * hitR_[i];
      }
    }
    now_ += m / 48000.0;
    done += m;
  }
}

}  // namespace pajupilli
