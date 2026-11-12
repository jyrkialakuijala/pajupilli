// Live: ALSA audio out, ALSA sequencer MIDI in (the Akai MPK mini mk3).
//
// One thread: read the pending MIDI events, render one period, write it.
// The blocking write paces the loop. Latency from a key to sound: the MIDI
// event waits at most one period (1.3 ms), the tongue seals for 2-4 ms, then
// the audio buffer (2-3 periods, 2.7-4 ms) plus the sound server.
#include "live.h"

#include <alsa/asoundlib.h>
#include <sched.h>
#include <sys/mman.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <vector>

#include "ensemble.h"
#include "player.h"

namespace pajupilli {
namespace {

std::atomic<bool> gStop{false};
void onSignal(int) { gStop = true; }

snd_pcm_t* openPcm(const LiveOptions& o, bool* isFloat, snd_pcm_uframes_t* period) {
  snd_pcm_t* pcm = nullptr;
  // Ask PipeWire for a graph quantum of one period (else it may run at 1024
  // frames, 21 ms). The user's own PIPEWIRE_LATENCY wins.
  char lat[32];
  std::snprintf(lat, sizeof lat, "%d/48000", o.period);
  setenv("PIPEWIRE_LATENCY", lat, 0);
  int err = snd_pcm_open(&pcm, o.device.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
  if (err < 0) {
    std::fprintf(stderr, "audio: cannot open %s: %s\n", o.device.c_str(), snd_strerror(err));
    return nullptr;
  }
  snd_pcm_hw_params_t* hw;
  snd_pcm_hw_params_alloca(&hw);
  snd_pcm_hw_params_any(pcm, hw);
  snd_pcm_hw_params_set_access(pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
  *isFloat = snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_FLOAT_LE) == 0;
  if (!*isFloat) snd_pcm_hw_params_set_format(pcm, hw, SND_PCM_FORMAT_S16_LE);
  snd_pcm_hw_params_set_channels(pcm, hw, 2);
  unsigned rate = 48000;
  snd_pcm_hw_params_set_rate_near(pcm, hw, &rate, nullptr);
  snd_pcm_uframes_t per = o.period, buf = o.period * o.periods;
  snd_pcm_hw_params_set_period_size_near(pcm, hw, &per, nullptr);
  snd_pcm_hw_params_set_buffer_size_near(pcm, hw, &buf);
  if ((err = snd_pcm_hw_params(pcm, hw)) < 0) {
    std::fprintf(stderr, "audio: hw params: %s\n", snd_strerror(err));
    snd_pcm_close(pcm);
    return nullptr;
  }
  snd_pcm_hw_params_get_period_size(hw, &per, nullptr);
  snd_pcm_hw_params_get_buffer_size(hw, &buf);
  if (rate != 48000) std::fprintf(stderr, "audio: warning, rate is %u, not 48000\n", rate);
  snd_pcm_sw_params_t* sw;
  snd_pcm_sw_params_alloca(&sw);
  snd_pcm_sw_params_current(pcm, sw);
  snd_pcm_sw_params_set_start_threshold(pcm, sw, per);
  snd_pcm_sw_params_set_avail_min(pcm, sw, per);
  snd_pcm_sw_params(pcm, sw);
  *period = per;
  std::fprintf(stderr, "audio: %s, %s, period %lu, buffer %lu frames (%.1f ms)\n",
               o.device.c_str(), *isFloat ? "float" : "s16", (unsigned long)per,
               (unsigned long)buf, buf * 1000.0 / 48000);
  return pcm;
}

struct Midi {
  snd_seq_t* seq = nullptr;
  int port = -1;
  bool connected = false;
};

// Connect to every MIDI source whose client name contains `want` (all
// hardware sources if want is empty). Returns true if any was connected.
bool connectSources(Midi& m, const std::string& want, bool verbose) {
  snd_seq_client_info_t* ci;
  snd_seq_port_info_t* pi;
  snd_seq_client_info_alloca(&ci);
  snd_seq_port_info_alloca(&pi);
  snd_seq_client_info_set_client(ci, -1);
  const int self = snd_seq_client_id(m.seq);
  bool any = false;
  while (snd_seq_query_next_client(m.seq, ci) >= 0) {
    const int c = snd_seq_client_info_get_client(ci);
    const char* name = snd_seq_client_info_get_name(ci);
    if (c == self || c == SND_SEQ_CLIENT_SYSTEM) continue;
    if (!want.empty() && !strstr(name, want.c_str())) continue;
    if (want.empty() && (strstr(name, "Through") || strstr(name, "PipeWire"))) continue;
    snd_seq_port_info_set_client(pi, c);
    snd_seq_port_info_set_port(pi, -1);
    while (snd_seq_query_next_port(m.seq, pi) >= 0) {
      const unsigned caps = snd_seq_port_info_get_capability(pi);
      if ((caps & SND_SEQ_PORT_CAP_READ) && (caps & SND_SEQ_PORT_CAP_SUBS_READ)) {
        const int p = snd_seq_port_info_get_port(pi);
        if (snd_seq_connect_from(m.seq, m.port, c, p) >= 0 || errno == EBUSY) {
          any = true;
          if (verbose)
            std::fprintf(stderr, "midi: listening to %s / %s (%d:%d)\n", name,
                         snd_seq_port_info_get_name(pi), c, p);
        }
      }
    }
  }
  return any;
}

bool openMidi(Midi& m) {
  if (snd_seq_open(&m.seq, "default", SND_SEQ_OPEN_INPUT, SND_SEQ_NONBLOCK) < 0) {
    std::fprintf(stderr, "midi: cannot open the ALSA sequencer\n");
    return false;
  }
  snd_seq_set_client_name(m.seq, "pajupilli");
  m.port = snd_seq_create_simple_port(m.seq, "flute in",
                                      SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                      SND_SEQ_PORT_TYPE_MIDI_GENERIC | SND_SEQ_PORT_TYPE_APPLICATION);
  return m.port >= 0;
}

// The MPK mini mk3 map (research/playing_techniques.md section 5, research/
// player_and_controller.md part B). Keys on any channel but 10; pads on 10.
class Controls {
 public:
  Controls(Ensemble& p, const Instrument& inst, const LiveOptions& o) : p_(p), o_(o) {
    // Sounding range; keys outside it fold in by octaves.
    lo_ = p.lo();
    hi_ = p.hi();
    transpose_ = o.transpose != LiveOptions::kAutoTranspose
                     ? o.transpose
                     : 12 + 12 * int(std::lround(inst.transpose / 12.0));
  }
  int fold(int n) const {
    n += transpose_;
    while (n < lo_) n += 12;
    while (n > hi_) n -= 12;
    return n;
  }
  void handle(const snd_seq_event_t* ev) {
    switch (ev->type) {
      case SND_SEQ_EVENT_NOTEON:
        if (ev->data.note.velocity == 0) noteOff(ev->data.note.channel, ev->data.note.note);
        else noteOn(ev->data.note.channel, ev->data.note.note, ev->data.note.velocity);
        break;
      case SND_SEQ_EVENT_NOTEOFF:
        noteOff(ev->data.note.channel, ev->data.note.note);
        break;
      case SND_SEQ_EVENT_CONTROLLER:
        cc(ev->data.control.param, ev->data.control.value);
        break;
      case SND_SEQ_EVENT_PITCHBEND:  // joystick X: roll the flute in / out
        p_.all(&Player::setLipRoll, ev->data.control.value / 8192.0);
        break;
      case SND_SEQ_EVENT_CHANPRESS:  // pad pressure (channel mode)
        padPressure(-1, ev->data.control.value);
        break;
      case SND_SEQ_EVENT_KEYPRESS:  // pad pressure (poly mode)
        padPressure(ev->data.note.note, ev->data.note.velocity);
        break;
      default:
        break;
    }
  }

 private:
  bool isPad(int ch) const { return ch == 9; }
  void noteOn(int ch, int note, int vel) {
    if (isPad(ch)) {
      const int pad = (note - 36) % 8;  // bank A 36-43, bank B 44-51
      if (pad == 0) { breathPad_ = true; p_.all(&Player::setBreath, 0); }
      else if (pad == 1) p_.all(&Player::setFlutter, 0.4 + 0.6 * vel / 127.0);
      if (o_.verbose) std::fprintf(stderr, "pad %d vel %d\n", pad + 1, vel);
      return;
    }
    const int n = fold(note);
    sounding_[note] = n;
    p_.noteOn(n, vel);
  }
  void noteOff(int ch, int note) {
    if (isPad(ch)) {
      const int pad = (note - 36) % 8;
      if (pad == 0) { breathPad_ = false; p_.all(&Player::setBreath, joyBreath_); }
      else if (pad == 1) p_.all(&Player::setFlutter, 0);
      return;
    }
    if (sounding_[note] >= 0) p_.noteOff(sounding_[note]);
    sounding_[note] = -1;
  }
  void padPressure(int note, int value) {
    const int pad = note < 0 ? (breathPad_ ? 0 : 1) : (note - 36) % 8;
    if (pad == 0 && breathPad_) p_.all(&Player::setBreath, -0.5 + value / 127.0);  // breath controller
    else if (pad == 1) p_.all(&Player::setFlutter, 0.3 + 0.7 * value / 127.0);
  }
  void cc(int num, int v) {
    const double x = v / 127.0;
    switch (num) {
      case 1:  // joystick Y: breath push
        joyBreath_ = 0.5 * x;
        if (!breathPad_) p_.all(&Player::setBreath, joyBreath_);
        break;
      case 64: p_.setHold(v >= 64); break;  // sustain pedal: keep blowing
      case 70: p_.all(&Player::setDynamics, x); break;
      case 71: p_.all(&Player::setBrightness, 0.3 + 2.2 * x); break;
      case 72: p_.all(&Player::setAir, x); break;
      case 73: p_.all(&Player::setVibrato, 0.15 * x); break;
      case 74: p_.all(&Player::setVibratoRate, 4 + 3 * x); break;
      case 75: p_.all(&Player::setHardness, 0.5 + 1.5 * x); break;
      case 76: p_.all(&Player::setRelease, 0.015 + 0.2 * x); break;
      case 77: gain_ = 2 * x * x; break;
      default: break;
    }
    if (o_.verbose) std::fprintf(stderr, "cc %d = %d\n", num, v);
  }
  Ensemble& p_;
  const LiveOptions& o_;
  int sounding_[128] = {};
  int lo_, hi_, transpose_;
  bool breathPad_ = false;
  double joyBreath_ = 0;

 public:
  double gain_ = 1;
  Controls* init() { std::fill(sounding_, sounding_ + 128, -1); return this; }
  void allOff() {
    for (int n = 0; n < 128; ++n)
      if (sounding_[n] >= 0) { p_.noteOff(sounding_[n]); sounding_[n] = -1; }
    p_.setHold(false);
  }
};

}  // namespace

int runLive(const Instrument& inst, const LiveOptions& o, Ensemble& player) {
  std::signal(SIGINT, onSignal);
  std::signal(SIGTERM, onSignal);
  bool isFloat = true;
  snd_pcm_uframes_t period = 64;
  snd_pcm_t* pcm = openPcm(o, &isFloat, &period);
  if (!pcm) return 1;
  Midi midi;
  if (!openMidi(midi)) return 1;
  // Hear about ports coming and going (hot-plug) from the system announce port.
  snd_seq_connect_from(midi.seq, midi.port, SND_SEQ_CLIENT_SYSTEM, SND_SEQ_PORT_SYSTEM_ANNOUNCE);
  midi.connected = connectSources(midi, o.midiName, true);
  if (!midi.connected)
    std::fprintf(stderr, "midi: no '%s' yet; plug it in any time\n", o.midiName.c_str());
  // Real-time priority if allowed; keep going without it.
  sched_param sp{};
  sp.sched_priority = 70;
  if (sched_setscheduler(0, SCHED_FIFO, &sp) != 0 && o.verbose)
    std::fprintf(stderr, "note: no real-time priority (run with rtprio to reduce xruns)\n");
  mlockall(MCL_CURRENT | MCL_FUTURE);

  player.all(&Player::setVibrato, 0.05);
  Controls ctl(player, inst, o);
  ctl.init();
  std::fprintf(stderr, "playing: %s\n", player.describe().c_str());

  std::vector<float> left(period), right(period), f32(period * 2);
  std::vector<int16_t> s16(period * 2);
  long frames = 0, xruns = 0;
  float gain = float(ctl.gain_);
  int demoStep = 0;
  // Prefill one period of silence so the first write starts the stream.
  std::fill(f32.begin(), f32.end(), 0.0f);
  std::fill(s16.begin(), s16.end(), 0);
  for (int i = 0; i < std::max(1, o.periods - 1); ++i) {
    if (isFloat) snd_pcm_writei(pcm, f32.data(), period);
    else snd_pcm_writei(pcm, s16.data(), period);
  }
  std::fprintf(stderr, "playing; Ctrl-C to stop\n");
  while (!gStop) {
    snd_seq_event_t* ev = nullptr;
    bool portStarted = false;
    while (snd_seq_event_input(midi.seq, &ev) >= 0 && ev) {
      if (ev->type == SND_SEQ_EVENT_PORT_START) portStarted = true;
      else if (ev->type == SND_SEQ_EVENT_PORT_EXIT) ctl.allOff();  // unplugged mid-note
      else ctl.handle(ev);
    }
    // A port appeared (the MPK plugged in again): connect. Rare, so the few
    // syscalls here are fine in this thread.
    if (portStarted) midi.connected = connectSources(midi, o.midiName, true) || midi.connected;
    if (o.selfTest) {  // a slurred arpeggio without a keyboard
      const int notes[] = {60, 64, 67, 72, 76, 79, 84, 79, 76, 72, 67, 64, 60};
      const long step = 48000 / 4;
      if (frames / step != demoStep - 1 && demoStep < 14) {
        if (demoStep < 13) player.noteOn(ctl.fold(notes[demoStep] - 12), 80);
        if (demoStep > 0) player.noteOff(ctl.fold(notes[demoStep - 1] - 12));
        ++demoStep;
      }
      if (frames > 48000 * 5) gStop = true;
    }
    player.render(left.data(), right.data(), int(period));
    for (size_t i = 0; i < period; ++i) {
      gain += 0.002f * (float(ctl.gain_) - gain);  // ~10 ms volume glide, no clicks
      for (int c = 0; c < 2; ++c) {
        float y = (c ? right[i] : left[i]) * gain;
        if (!std::isfinite(y)) y = 0;  // the output guard
        y = std::max(-1.0f, std::min(1.0f, y));
        f32[2 * i + c] = y;
        s16[2 * i + c] = int16_t(y * 32767);
      }
    }
    snd_pcm_sframes_t w = isFloat ? snd_pcm_writei(pcm, f32.data(), period)
                                  : snd_pcm_writei(pcm, s16.data(), period);
    if (w < 0) {
      ++xruns;
      if (o.verbose) std::fprintf(stderr, "xrun at %.3f s (%s)\n", frames / 48000.0, snd_strerror(int(w)));
      if (snd_pcm_recover(pcm, int(w), 1) < 0) {  // the device is gone: stop, don't spin
        std::fprintf(stderr, "audio: cannot recover (%s); stopping\n", snd_strerror(int(w)));
        break;
      }
      // Refill the buffer with silence so the restarted stream has headroom.
      std::fill(f32.begin(), f32.end(), 0.0f);
      std::fill(s16.begin(), s16.end(), 0);
      for (int i = 0; i < std::max(1, o.periods - 1); ++i) {
        if (isFloat) snd_pcm_writei(pcm, f32.data(), period);
        else snd_pcm_writei(pcm, s16.data(), period);
      }
    }
    frames += long(period);
  }
  snd_pcm_drain(pcm);
  snd_pcm_close(pcm);
  snd_seq_close(midi.seq);
  std::fprintf(stderr, "stopped after %.1f s, %ld xruns, %d state resets\n", frames / 48000.0,
               xruns, player.resets());
  return 0;
}

}  // namespace pajupilli
