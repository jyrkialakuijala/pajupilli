#include "snareinst.h"

#include <cmath>
#include <cstring>

#include "knob.h"
#include "songclock.h"

namespace pajupilli {

namespace {
constexpr double kPi = 3.14159265358979323846;
}

bool brushSnare(const Instrument& inst) { return std::strcmp(inst.name, "brush-snare") == 0; }

SnareModel::SnareModel(const Instrument& inst) : Tube(inst), snare_(inst.fs) {
  // (The sideways parts add to what the listener gets, about as much power again at 1: the whole
  // down by that, so the drum stays as loud as it was set against the other kits.)
  const double a = knob("PJ_SNARE_AIM", 0.65);
  aim_ = a > 0;
  trim_ = float(1 / std::sqrt(1 + 0.95 * a * a));
}

void SnareModel::resetAir() {
  Tube::resetAir();
  snare_.reset();
}

float SnareModel::sample() {
  // (The microphones' pressure, Pa at half a metre over the head: a firm
  // tap peaks near 2 Pa.)
  static const float g0 = float(knob("PJ_SNARE_OUT", 1.0 / 8));
  const float g = g0 * trim_;
  last_ = g * snare_.sample();
  side_.push_back(g * float(outScale()) * snare_.side());
  if (aim_)
    for (int k = 0; k < 2; ++k) {
      if (dip_[k].size() > 4096) dip_[k].clear();  // (nobody takes them: no room)
      dip_[k].push_back(g * float(outScale()) * snare_.dipole(k));
    }
  return finish(last_);
}

SnarePlayer::SnarePlayer(SnareModel& m) : Performer(m), m_(m) {
  bpm_ = knob("PJ_SNARE_BPM", 120);
  // The pull: the hand this much faster at its middle (1: twice as fast),
  // over a part of the circle `pullWide_` wide (rad), at pullAt_ of the turn.
  held_ = knob("PJ_SNARE_HELD", 0.42);     // ... of the swirl asked for (key 40), on top of that (x)
  vary_ = knob("PJ_SNARE_VARY", 1.0);      // how much one round differs from the next (0: all alike)
  swoosh_ = knob("PJ_SNARE_SWOOSH", 0.28);  // the circling hands' weight on the head (x)
  pull_ = knob("PJ_SNARE_PULL", 0.8);
  pullWide_ = knob("PJ_SNARE_PULLWIDE", 1.0);
  pullAt_ = 0.75;
  pulse_ = 60 / knob("PJ_SNARE_BPM", 120);
  // The hand's pace round the circle, and the time it has taken to each place.
  {
    double t = 0;
    for (int i = 0; i < kWarp; ++i) {
      double d = 2 * kPi * ((i + 0.5) / kWarp - pullAt_);
      d -= 2 * kPi * std::floor(d / (2 * kPi) + 0.5);
      warpW_[i] = 1 + pull_ * std::exp(-d * d / (2 * pullWide_ * pullWide_));
      warpT_[i] = t;
      t += 1 / warpW_[i] / kWarp;
    }
    warpN_ = t;  // (so that a turn takes its time whatever the pull)
    for (int i = 0; i < kWarp; ++i) warpT_[i] /= t;
  }
  sweeps_ = knob("PJ_SNARE_TAPSWEEP", 1) > 0;
  meat_ = knob("PJ_SNARE_MEAT", 1.0);            // the quarter-note strokes this much harder while brushing (dB)
  flatShare_ = knob("PJ_SNARE_FLAT", 0.22);      // the share of them that land flatter: a fuller slap
  flatHard_ = knob("PJ_SNARE_FLATHARD", 0.8);    // ... the hand that much slower for those (landing flat is louder by itself)
  tapSwoosh_ = knob("PJ_SNARE_TAPSWOOSH", 0.7);  // how much of a sweep a quarter-note stroke is while brushing (0: a plain tap-sweep)  // a tap is drawn on across the head
  loop_ = knob("PJ_SNARE_LOOP", 1) > 0;  // the left hand circles of its own accord between brush strokes
}

double SnarePlayer::ur() {
  rng_ = rng_ * 1664525u + 1013904223u;
  return (rng_ >> 8) * (1.0 / 16777216.0);
}

double SnarePlayer::strokeSpeed(int velocity, double dynamics) {
  const double v = std::clamp(velocity, 1, 127) / 127.0;
  return (0.12 + 2.6 * std::pow(v, 1.5)) * (0.7 + 0.6 * std::clamp(dynamics, 0.0, 1.0));
}

void SnarePlayer::noteOn(int midi, int velocity) {
  if (midi == 42) { rhAway_ = true; return; }  // (the right hand gone to a cymbal, to swirl there)
  articulate(midi, velocity, false);
  note_ = midi;
  blowing_ = true;
  Snare& s = m_.snare();
  s.wake();
  const double R = s.batter().radius();
  if (midi == 40) {  // the swirl: the left hand
    swirl_ = true;
    load_ = 0.15 + 1.1 * std::pow(std::clamp(velocity, 1, 127) / 127.0, 1.3);
    if (!brushing_) ownLoad_ = held_ * load_;  // (and the brushing goes on when the key is let go)
    brushing_ = true;
    return;
  }
  if (midi == 37) {  // the cross-stick: no brush stroke, and the left hand is not circling
    s.crossStick(0.3 + 2.7 * std::pow(std::clamp(velocity, 1, 127) / 127.0, 1.4));
    sinceTap_ = 100;  // (that hand holds the stick)
    return;
  }
  // How far apart the strokes come (the middle of the last few gaps): the
  // left hand's swoosh fills each gap. Its weight follows the strokes'.
  if (sinceTap_ > 0.06 && sinceTap_ < 2.0) {
    gaps_[nGaps_++ % 4] = sinceTap_;
    const int n = std::clamp(nGaps_, 1, 4);
    double g[4] = {gaps_[0], gaps_[1], gaps_[2], gaps_[3]};
    for (int i = 1; i < n; ++i)  // (in order)
      for (int j = i; j > 0 && g[j] < g[j - 1]; --j) std::swap(g[j], g[j - 1]);
    gap_ = n >= 3 ? g[n / 2] : g[n - 1];
  } else if (sinceTap_ >= 2.0) {
    nGaps_ = 0;
  }
  // The pulse heard in the strokes (when no song gives the beat): a gap is
  // a beat, or two thirds, a half, a third of one, or two; it moves the
  // pulse only if one of those readings is near what the pulse already is.
  // It takes six strokes in a row that fit nowhere to start afresh, so it
  // does not jump to double or half.
  if (sinceTap_ > 0.06 && sinceTap_ < 2.5) {
    if (pulse_ <= 0 || misses_ >= 6) {
      pulse_ = sinceTap_;
      while (pulse_ < 0.35) pulse_ *= 2;
      while (pulse_ > 1.0) pulse_ /= 2;
      misses_ = 0;
      turnBeats_ = 0;
    } else {
      double best = 1e9, c = 0;
      for (double m : {1.0, 1.5, 2.0, 3.0, 0.5}) {
        const double d = std::fabs(std::log(sinceTap_ * m / pulse_));
        if (d < best) { best = d; c = sinceTap_ * m; }
      }
      if (best < 0.14) { pulse_ += 0.2 * (c - pulse_); misses_ = 0; }
      else ++misses_;
    }
  }
  sinceTap_ = 0;
  brushing_ = true;
  const double w = 0.12 + 0.75 * std::pow(std::clamp(velocity, 1, 127) / 127.0, 1.3);
  ownLoad_ = nGaps_ == 0 ? w : ownLoad_ + 0.3 * (w - ownLoad_);
  // A stroke of the right hand: on its own side of the head, where the
  // brightness knob puts it (the middle .. towards the edge), each one a
  // little elsewhere.
  Brush& b = s.hand(1);
  double r = (0.25 + 0.3 * std::clamp(brightness_ - 0.3, 0.0, 2.2) / 2.2) * R;
  if (midi == 36) r = 0.08 * R;
  if (midi == 41) r = 0.72 * R;
  r += 0.04 * R * (ur() - 0.5);
  const double th = -0.6 + 0.25 * (ur() - 0.5);
  rhAng_ = 1.2 + 0.2 * (ur() - 0.5);
  b.jump(r * std::cos(th), r * std::sin(th), rhAng_);
  const double v = strokeSpeed(velocity, dyn_), vel = std::clamp(velocity, 1, 127) / 127.0;
  if (midi == 39) {  // the slap: flat, pressed in, not drawn
    b.strike(v, 0.5e-3, 3e-3, 0.035);
    rhT_ = -1;
    return;
  }
  // The tap is the start of a sweep: the brush lands already moving sideways
  // and is drawn on across the head, slowing, for the first part of the gap
  // to the next stroke, then lifts: "t-shh". (Its own swoosh; the left
  // hand's follows it.)
  // While the brushing is on, a stroke with room after it (a quarter note,
  // not the quick notes of the swing) is more of a sweep still: it comes in
  // at a flatter slant (its wire ends arriving over a longer time), leans
  // on the head a little more, and is drawn further and for longer.
  const double room = std::clamp((gap_ / std::max(pulse_, 0.1) - 0.5) / 0.4, 0.0, 1.0);  // 0: half a beat or less .. 1: a beat
  const double sw = sweeps_ ? tapSwoosh_ * (brushing_ ? 0.5 + 0.5 * room : 0.4) : 0;
  rhDur_ = sweeps_ ? std::clamp((0.45 + 0.25 * sw) * gap_, 0.05, 0.50) * (0.9 + 0.2 * ur()) : 0;
  rhV0_ = 0.25 + 0.6 * vel + 0.2 * sw;
  rhX_ = r * std::cos(th); rhY_ = r * std::sin(th);
  const double dir = th + (ur() < 0.7 ? 1 : -1) * (1.3 + 0.5 * ur());  // across, mostly towards the drummer's left
  rhDx_ = std::cos(dir); rhDy_ = std::sin(dir);
  rhT_ = 0;
  // The quarter-note strokes of the brushing have some meat on them: a
  // little harder than written (meat_, dB). And now and then (never twice
  // running) one lands flatter: the fan nearly level with the head, so its
  // wire ends arrive together, nearer the middle of the head, a touch harder
  // still and let further in: a fuller slap with more of the drum in it.
  double hard = 1, spread = (2.5 + 1.5 * ur() + 1.2 * sw) * 1e-3, follow = (0.6 + 1.0 * vel + 0.3 * sw) * 1e-3;
  if (brushing_) hard = 1 + (std::pow(10.0, meat_ / 20) - 1) * room;
  const bool flat = brushing_ && room > 0.5 && !lastFlat_ && ur() < flatShare_;
  lastFlat_ = flat;
  if (flat) {
    hard *= flatHard_;
    spread = (0.7 + 0.6 * ur()) * 1e-3;
    follow += 0.5e-3;
    rhX_ *= 0.65; rhY_ *= 0.65;
    b.jump(rhX_, rhY_, rhAng_);
  }
  b.strike(v * (1 - 0.15 * sw) * hard, spread, follow, rhDur_);
}

void SnarePlayer::noteOff(int midi) {
  if (midi == 40) swirl_ = false;
  if (midi == 42) rhAway_ = false;
}

void SnarePlayer::control(double dt) {
  // The beat runs on (a MIDI file sets it; alone, at its own tempo).
  const double beatLen = 60 / std::max(bpm_, 20.0);
  beat_ = std::fmod(beat_ + dt / beatLen, 4.0);
  sinceTap_ += dt;
  Snare& s = m_.snare();
  Brush &lh = s.hand(0), &rh = s.hand(1);
  const double R = s.batter().radius(), dynK = swoosh_ * (0.6 + 0.8 * std::clamp(dyn_, 0.0, 1.0));
  // The right hand's stroke, drawn on across the head.
  bool rhBusy = !rh.up() && rhT_ >= 0;
  if (rhT_ >= 0) {
    rhT_ += dt;
    if (rhT_ < rhDur_) {
      const double d = rhV0_ * (rhT_ - 0.35 * rhT_ * rhT_ / rhDur_);
      double x = rhX_ + rhDx_ * d, y = rhY_ + rhDy_ * d;
      const double rr = std::hypot(x, y);
      if (rr > 0.85 * R) { x *= 0.85 * R / rr; y *= 0.85 * R / rr; }
      rh.place(x, y, rhAng_);
    } else if (rh.up()) {
      rhT_ = -1;
      rhBusy = false;
    }
  }
  // Both brushes on the head. Asked for (key 40 held), or of the drummer's
  // own accord: brush strokes are being played, so between them the head is
  // never left alone, and for a while after the last.
  const bool asked = swirl_ || hold_;
  // Once begun (a brush stroke asks for it), the brushing goes on: through
  // bars with no stroke, for as long as the band plays. It stops for a
  // general pause or the song's end (alone, with no song to go by: after
  // twelve seconds without a stroke), and begins again with the next stroke.
  {
    const SongClock& c = SongClock::get();
    if (c.valid ? c.pause : sinceTap_ > 12) brushing_ = false;
  }
  const bool own = loop_ && brushing_;
  if (!asked && !own) {
    swirlFor_ = 0;
    lh.rest(0);
    if (!rhBusy && !rh.up() && rhT_ < 0) rh.rest(0);
    return;
  }
  const bool time = own && sinceTap_ < 1.5 * gap_;  // strokes are coming
  // A swirl going on and on: after eight bars of it, its phrases (four bars) now and then
  // lighter (about one in three, to 55 .. 70 %), easing there over a beat; and when no strokes
  // come, a slap of the right hand into the round now and then, on two or four (fewer in the
  // lighter phrases). Seeded by the phrase and the beat: the same song, the same shaping.
  {
    static const double kArc = knob("PJ_SNARE_ARC", 1), kSlaps = knob("PJ_SNARE_SLAPS", 1);
    const SongClock& ck = SongClock::get();
    swirlFor_ += dt;
    const double beatNow = ck.valid ? ck.beat : swirlFor_ / std::max(pulse_, 0.2);
    const double barLen = 4 * (ck.valid ? ck.beatLen : std::max(pulse_, 0.2));
    if (kArc > 0 && swirlFor_ > 8 * barLen) {
      const long phrase = ck.valid ? long(beatNow / 16) : long(swirlFor_ / 8);
      if (phrase != arcPhrase_) {
        arcPhrase_ = phrase;
        uint32_t hsh = 2654435761u * uint32_t(phrase + 7);
        hsh ^= hsh >> 13;
        const double u = (hsh & 0xffff) / 65536.0, w = ((hsh >> 16) & 0xffff) / 65536.0;
        arcTo_ = u < 0.35 ? 1 - kArc * (0.3 + 0.15 * w) : 1;
      }
    } else {
      arcTo_ = 1;
    }
    arc_ += (arcTo_ - arc_) * std::min(1.0, dt / (barLen / 4));
    const long beatI = long(std::floor(beatNow));
    if (beatI != lastBeat_) {
      lastBeat_ = beatI;
      const int inBar = int(((beatI % 4) + 4) % 4);
      uint32_t hsh = 40503u * uint32_t(beatI + 11);
      hsh ^= hsh >> 11;
      const double u = (hsh & 0xffff) / 65536.0;
      if (kSlaps > 0 && swirlFor_ > 8 * barLen && (inBar == 1 || inBar == 3) && !time && !rhAway_ && rhT_ < 0 &&
          sinceTap_ > 2 * barLen / 4 && u < kSlaps * 0.22 * arc_) {
        const double R0 = s.batter().radius();
        const double r = (0.3 + 0.15 * ur()) * R0, th = -0.6 + 0.3 * (ur() - 0.5);
        rh.jump(r * std::cos(th), r * std::sin(th), 1.2);
        rh.strike(strokeSpeed(55 + int(25 * ur()), dyn_), 0.5e-3, 3e-3, 0.035);
        rhT_ = -1;
        slapT_ = 0.15;
      }
    }
  }
  // One whoosh that never stops: the left hand goes round in smooth circles.
  //   - Its tempo is the song's beat (a MIDI file's own; alone: the pulse
  //     heard in the strokes), a turn every two beats; at a slow beat (two
  //     of them longer than 1.7 s) one turn a beat, at a very fast one a
  //     turn a bar. Once chosen it is kept: only when a turn would take less
  //     than 0.4 s or more than 2 s does the hand change to another.
  //   - Each turn has its pull: through one part of the circle (drawn
  //     towards the drummer) the hand speeds up and bears down, and eases
  //     round the rest. The pull falls on the beat: the second and the
  //     fourth when a turn takes two.
  const SongClock& clock = SongClock::get();
  const double B = clock.valid ? clock.beatLen : pulse_;
  if (turnBeats_ == 0 || turnBeats_ * B < 0.4 || turnBeats_ * B > 2.0) turnBeats_ = 2 * B > 1.7 ? 1 : 2 * B < 0.45 ? 4 : 2;
  const double P = turnBeats_ * B;
  // Where in its turn the hand is (as a share of the turn's time), and where
  // the song says it should be: it leans gently towards that.
  const double at = phase_ / (2 * kPi);
  const int ti = std::min(int(at * kWarp), kWarp - 1);
  double lock = 0;
  if (clock.valid) {
    // (The pull's middle half a turn's time after the turn's first beat.)
    const double want = std::fmod(clock.beat / turnBeats_ + 0.5 + warpT_[int(pullAt_ * kWarp)], 1.0);
    double err = want - warpT_[ti];
    err -= std::floor(err + 0.5);
    lock = std::clamp(2.0 * err, -0.3, 0.3);
  }
  // No two rounds alike. At the top of each turn the hand takes new aims for
  // the next one and eases into them over the turn:
  //   - where on the head: for a few turns it works nearer the rim (a
  //     brighter hiss), then nearer the middle (warmer), then back (the
  //     drummer leaning: it holds two to four turns, then more often than
  //     not goes over to the other side);
  //   - how wide and how round the circle is, a little;
  //   - how the brush is held: its slant (the fan's angle) and its weight;
  //   - how strong that turn's pull is;
  //   - and now and then, at the end of a phrase (every eighth turn, most
  //     times), one turn is not a circle but a sweep across and back, flat.
  if (phase_ < lastPhase_) {
    ++turn_;
    const double v = vary_;
    if (--zoneHold_ <= 0) {
      if (ur() < 0.65) zone_ = -zone_;
      zoneHold_ = 2 + int(3 * ur());
    }
    for (int h = 0; h < 2; ++h) {
      Round& r = aim_[h];
      const double z = zone_ * (h ? -1 : 1);  // (as one hand goes out the other comes in)
      r.cx = v * (0.11 * z + 0.06 * (ur() - 0.5));
      r.cy = v * 0.08 * (ur() - 0.5);
      r.rx = 1 + v * (0.16 * z + 0.26 * (ur() - 0.5));
      r.ry = 1 + v * (0.16 * z + 0.26 * (ur() - 0.5));
      r.ang = v * 0.5 * (ur() - 0.5);
      r.load = 1 + v * (0.2 * z + 0.5 * (ur() - 0.5));
      r.pull = 1 + v * 1.0 * (ur() - 0.5);
      r.wipe = v > 0 && turn_ % 8 == 7 && ur() < 0.7 * v && h == 0 ? 1 : 0;
    }
  }
  lastPhase_ = phase_;
  const double ease = std::min(1.0, dt * 4 / P);  // (a quarter of a turn to get there)
  for (int h = 0; h < 2; ++h) {
    Round &r = now_[h], &a = aim_[h];
    r.cx += (a.cx - r.cx) * ease; r.cy += (a.cy - r.cy) * ease;
    r.rx += (a.rx - r.rx) * ease; r.ry += (a.ry - r.ry) * ease;
    r.ang += (a.ang - r.ang) * ease; r.load += (a.load - r.load) * ease;
    r.pull += (a.pull - r.pull) * ease; r.wipe += (a.wipe - r.wipe) * ease;
  }
  const Round& rl = now_[0];
  const double pull = 1 + rl.pull * (warpW_[ti] - 1);  // 1 at ease .. more at the pull's middle
  const double load = (asked ? held_ * load_ : ownLoad_) * arc_;
  phase_ += 2 * kPi / P * (1 + rl.pull * (warpN_ - 1)) * pull * (1 + lock) * dt;
  if (phase_ >= 2 * kPi) phase_ -= 2 * kPi;
  // (The sweep across and back: the same turn, its far side folded flat.)
  const double sy = (1 - rl.wipe) * std::sin(phase_) + rl.wipe * 0.3 * std::sin(2 * phase_);
  const double xl = (-0.26 + rl.cx + 0.36 * rl.rx * std::cos(phase_)) * R, yl = (0.08 + rl.cy + 0.33 * rl.ry * sy) * R;
  if (lh.up()) lh.jump(xl, yl, 2.2 + rl.ang);
  lh.place(xl, yl, 2.2 + rl.ang);
  lh.rest(load * rl.load * (0.8 + 0.5 * (pull - 1) / std::max(pull_, 1e-6)) * dynK);
  // The right hand, when it has no stroke to play: circling too, on its own
  // side and the other way round, half a turn behind (its pull falls where
  // the left hand eases). Between strokes that are coming it is in the air
  // (its stroke's own sweep done), on its way to the next.
  if (rhAway_ && !rhBusy && rhT_ < 0) rh.rest(0);  // (swirling on a cymbal: only the left hand circles here)
  slapT_ -= dt;
  if (!time && !rhBusy && !rhAway_ && slapT_ <= 0) {
    const Round& rr = now_[1];
    double pr = phase_ + kPi;
    if (pr >= 2 * kPi) pr -= 2 * kPi;
    const double pullR = 1 + rr.pull * (warpW_[std::min(int(pr / (2 * kPi) * kWarp), kWarp - 1)] - 1);
    const double xr = (0.28 + rr.cx + 0.34 * rr.rx * std::cos(-pr)) * R, yr = (-0.06 + rr.cy + 0.32 * rr.ry * std::sin(-pr)) * R;
    if (rh.up()) rh.jump(xr, yr, 1.0 + rr.ang);
    rh.place(xr, yr, 1.0 + rr.ang);
    rh.rest(0.8 * load * rr.load * (0.8 + 0.5 * (pullR - 1) / std::max(pull_, 1e-6)) * dynK);
    rhT_ = -1;
  }
  s.wake();
}

void SnarePlayer::render(float* out, int n) {
  constexpr int kBlock = 16;
  for (int i = 0; i < n; i += kBlock) {
    const int m = std::min(kBlock, n - i);
    tick(m / fs_);
    control(m / fs_);
    m_.process(out + i, m);
  }
  blowing_ = !m_.snare().asleep();
}

}  // namespace pajupilli
