#include "voice.h"

#include <algorithm>
#include <cmath>
#include <numeric>

#include "flute.h"
#include "horn.h"
#include "hornplayer.h"
#include "bassinst.h"
#include "modalinst.h"
#include "barsinst.h"
#include "guitarinst.h"
#include "jazzguitar.h"
#include "ebass.h"
#include "organ.h"
#include "accordion.h"
#include "harmonica.h"
#include "harmonium.h"
#include "hammond.h"
#include "harpsichord.h"
#include "hihatinst.h"
#include "kick.h"
#include "percextras.h"
#include "clap.h"
#include "woodperc.h"
#include "surdo.h"
#include "belltree.h"
#include "shaker.h"
#include "scrapers.h"
#include "cymbal.h"
#include "timpani.h"
#include "harpinst.h"
#include "tom.h"
#include "metalperc.h"
#include "handdrum.h"
#include "pianoinst.h"
#include "snareinst.h"
#include "sticksnare.h"
#include "clavinet.h"
#include "dx7.h"
#include "rhodes.h"
#include "scifi.h"
#include "rain.h"
#include "soundtrack.h"
#include "atmosphere.h"
#include "brightness.h"
#include "goblins.h"
#include "echoes.h"
#include "cp80.h"
#include "choir.h"
#include "lead.h"
#include "player.h"
#include "reed.h"
#include "reedplayer.h"

namespace pajupilli {
namespace {

double besselI0(double x) {
  double s = 1, t = 1;
  for (int k = 1; k < 30; ++k) {
    t *= (x / (2 * k)) * (x / (2 * k));
    s += t;
  }
  return s;
}

}  // namespace

Resampler::Resampler(int inRate, int outRate) {
  const int g = std::gcd(inRate, outRate);
  L_ = outRate / g;
  M_ = inRate / g;
  if (identity()) return;
  // Prototype at L x the input rate; cut-off at 0.45 of the lower rate.
  const int n = L_ * kTaps;
  const double fc = 0.45 * std::min(inRate, outRate) / (double(inRate) * L_);  // cycles / sample
  const double beta = 8.0, c = 0.5 * (n - 1);
  std::vector<double> p(n);
  double sum = 0;
  for (int i = 0; i < n; ++i) {
    const double t = i - c;
    const double sinc = t == 0 ? 2 * fc : std::sin(2 * M_PI * fc * t) / (M_PI * t);
    const double r = t / c;
    p[i] = sinc * besselI0(beta * std::sqrt(std::max(0.0, 1 - r * r))) / besselI0(beta);
    sum += p[i];
  }
  h_.resize(n);
  for (int ph = 0; ph < L_; ++ph)
    for (int k = 0; k < kTaps; ++k) h_[ph * kTaps + k] = float(p[k * L_ + ph] * L_ / sum);
  buf_.assign(kTaps, 0.0f);
}

int Resampler::needed(int n) const {
  if (identity()) return n;
  return int((j_ + n - 1) * M_ / L_ + 1 - nin_);
}

void Resampler::process(const float* in, float* out, int n) {
  if (identity()) {
    std::copy(in, in + n, out);
    return;
  }
  const int need = needed(n);
  // History of kTaps: an upsampling call can start one input back.
  const int hist = kTaps;
  buf_.resize(hist + need);
  std::copy(in, in + need, buf_.begin() + hist);
  // buf_[hist + m] is input nin_ + m.
  for (int o = 0; o < n; ++o, ++j_) {
    const long long u = j_ * M_;
    const long long i = u / L_;
    const int ph = int(u - i * L_);
    const float* x = buf_.data() + hist + (i - nin_);
    const float* hp = h_.data() + ph * kTaps;
    float s = 0;
    for (int k = 0; k < kTaps; ++k) s += hp[k] * x[-k];
    out[o] = s;
  }
  nin_ += need;
  std::copy(buf_.end() - hist, buf_.end(), buf_.begin());
  buf_.resize(hist);
}

const float* Voice::side(int n) {
  if (!rs_.identity()) return nullptr;
  return model_->takeSide(n);
}

const float* Voice::dipole(int n, int k) {
  if (!rs_.identity()) return nullptr;
  return model_->takeDipole(n, k);
}

const float* Voice::dipoleAxes() const { return rs_.identity() ? model_->dipoleAxes() : nullptr; }

bool Voice::ownEarly() const { return rs_.identity() && model_->ownEarly(); }
bool Voice::ownLate() const { return ownEarly() && model_->ownLate(); }

void Voice::early(int n, float* l, float* r, float g) {
  if (ownEarly()) model_->takeEarly(n, l, r, g);
}

std::unique_ptr<Tube> Voice::makeModel(const Instrument& inst) {
  if (inst.reed()) return std::unique_ptr<Tube>(new Reed(inst));
  if (inst.lip()) return std::unique_ptr<Tube>(new Horn(inst));
  if (inst.circuit()) return std::unique_ptr<Tube>(new Lead(inst));
  if (inst.plucked() && (grandPiano(inst) || harpsichord(inst))) return std::unique_ptr<Tube>(new PianoModel(inst));
  if (inst.plucked() && choirSinger(inst)) return std::unique_ptr<Tube>(new SingerModel(inst));
  if (inst.plucked() && choirInstrument(inst)) return std::unique_ptr<Tube>(new ChoirModel(inst));
  if (inst.plucked() && electricGrand(inst)) return std::unique_ptr<Tube>(new Cp80Model(inst));
  if (inst.plucked() && clavinet(inst)) return std::unique_ptr<Tube>(new ClavinetModel(inst));
  if (inst.plucked() && dx7Piano(inst)) return std::unique_ptr<Tube>(new Dx7Model(inst));
  if (inst.plucked() && rhodes(inst)) return std::unique_ptr<Tube>(new RhodesModel(inst));
  if (inst.plucked() && sciFi(inst)) return std::unique_ptr<Tube>(new SciFiModel(inst));
  if (inst.plucked() && fxRain(inst)) return std::unique_ptr<Tube>(new RainModel(inst));
  if (inst.plucked() && fxSoundtrack(inst)) return std::unique_ptr<Tube>(new AeolianModel(inst));
  if (inst.plucked() && fxAtmosphere(inst)) return std::unique_ptr<Tube>(new AtmosphereModel(inst));
  if (inst.plucked() && fxBrightness(inst)) return std::unique_ptr<Tube>(new SingingArcModel(inst));
  if (inst.plucked() && fxGoblins(inst)) return std::unique_ptr<Tube>(new PyrophoneModel(inst));
  if (inst.plucked() && fxEchoes(inst)) return std::unique_ptr<Tube>(new EchoesModel(inst));
  if (inst.plucked() && brushSnare(inst)) return std::unique_ptr<Tube>(new SnareModel(inst));
  if (inst.plucked() && stickSnare(inst)) return std::unique_ptr<Tube>(new StickSnareModel(inst));
  if (inst.plucked() && hiHat(inst)) return std::unique_ptr<Tube>(new HiHatModel(inst));
  if (inst.plucked() && guitarInstrument(inst)) return std::unique_ptr<Tube>(new GuitarModel(inst));
  if (inst.plucked() && electricBassInstrument(inst)) return std::unique_ptr<Tube>(new ElectricBassModel(inst));
  if (inst.plucked() && organRank(inst)) return std::unique_ptr<Tube>(new OrganModel(inst));
  if (inst.plucked() && accordionInstrument(inst)) return std::unique_ptr<Tube>(new AccordionModel(inst));
  if (inst.plucked() && harmonicaInstrument(inst)) return std::unique_ptr<Tube>(new HarmonicaModel(inst));
  if (inst.plucked() && harmoniumInstrument(inst)) return std::unique_ptr<Tube>(new HarmoniumModel(inst));
  if (inst.plucked() && hammondInstrument(inst)) return std::unique_ptr<Tube>(new HammondModel(inst));
  if (inst.plucked() && barInstrument(inst)) return std::unique_ptr<Tube>(new BarsModel(inst));
  if (inst.plucked() && jazzKick(inst)) return std::unique_ptr<Tube>(new KickModel(inst));
  if (inst.plucked() && shakerInstrument(inst)) return std::unique_ptr<Tube>(new ShakerModel(inst));
  if (inst.plucked() && cymbal(inst)) return std::unique_ptr<Tube>(new CymbalModel(inst));
  if (inst.plucked() && jazzTom(inst)) return std::unique_ptr<Tube>(new TomModel(inst));
  if (inst.plucked() && timpaniInstrument(inst)) return std::unique_ptr<Tube>(new TimpaniModel(inst));
  if (inst.plucked() && harpInstrument(inst)) return std::unique_ptr<Tube>(new HarpModel(inst));
  if (inst.plucked() && metalPerc(inst)) return std::unique_ptr<Tube>(new MetalPercModel(inst));
  if (inst.plucked() && scraper(inst)) return std::unique_ptr<Tube>(new ScraperModel(inst));
  if (inst.plucked() && handDrum(inst)) return std::unique_ptr<Tube>(new HandDrumModel(inst));
  if (inst.plucked() && percExtras(inst)) return std::unique_ptr<Tube>(new PercExtrasModel(inst));
  if (inst.plucked() && handClap(inst)) return std::unique_ptr<Tube>(new ClapModel(inst));
  if (inst.plucked() && woodPerc(inst)) return std::unique_ptr<Tube>(new WoodPercModel(inst));
  if (inst.plucked() && surdo(inst)) return std::unique_ptr<Tube>(new SurdoModel(inst));
  if (inst.plucked() && bellTree(inst)) return std::unique_ptr<Tube>(new BellTreeModel(inst));
  if (inst.plucked()) return std::unique_ptr<Tube>(new BassModel(inst));
  if (inst.modal()) return std::unique_ptr<Tube>(new ModalModel(inst));
  return std::unique_ptr<Tube>(new Flute(inst));
}

std::unique_ptr<Performer> Voice::makePlayer(Tube& t) {
  if (t.instrument().reed()) return std::unique_ptr<Performer>(new ReedPlayer(static_cast<Reed&>(t)));
  if (t.instrument().lip()) return std::unique_ptr<Performer>(new HornPlayer(static_cast<Horn&>(t)));
  if (t.instrument().circuit()) return std::unique_ptr<Performer>(new LeadPlayer(static_cast<Lead&>(t)));
  if (t.instrument().plucked() && harpsichord(t.instrument()))
    return std::unique_ptr<Performer>(new HarpsichordPlayer(static_cast<PianoModel&>(t)));
  if (t.instrument().plucked() && grandPiano(t.instrument()))
    return std::unique_ptr<Performer>(new PianoPlayer(static_cast<PianoModel&>(t)));
  if (t.instrument().plucked() && choirSinger(t.instrument()))
    return std::unique_ptr<Performer>(new SingerPlayer(static_cast<SingerModel&>(t)));
  if (t.instrument().plucked() && choirInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new ChoirPlayer(static_cast<ChoirModel&>(t)));
  if (t.instrument().plucked() && electricGrand(t.instrument()))
    return std::unique_ptr<Performer>(new Cp80Player(static_cast<Cp80Model&>(t)));
  if (t.instrument().plucked() && clavinet(t.instrument()))
    return std::unique_ptr<Performer>(new ClavinetPlayer(static_cast<ClavinetModel&>(t)));
  if (t.instrument().plucked() && dx7Piano(t.instrument()))
    return std::unique_ptr<Performer>(new Dx7Player(static_cast<Dx7Model&>(t)));
  if (t.instrument().plucked() && rhodes(t.instrument()))
    return std::unique_ptr<Performer>(new RhodesPlayer(static_cast<RhodesModel&>(t)));
  if (t.instrument().plucked() && sciFi(t.instrument()))
    return std::unique_ptr<Performer>(new SciFiPlayer(static_cast<SciFiModel&>(t)));
  if (t.instrument().plucked() && fxRain(t.instrument()))
    return std::unique_ptr<Performer>(new RainPlayer(static_cast<RainModel&>(t)));
  if (t.instrument().plucked() && fxSoundtrack(t.instrument()))
    return std::unique_ptr<Performer>(new AeolianPlayer(static_cast<AeolianModel&>(t)));
  if (t.instrument().plucked() && fxAtmosphere(t.instrument()))
    return std::unique_ptr<Performer>(new AtmospherePlayer(static_cast<AtmosphereModel&>(t)));
  if (t.instrument().plucked() && fxBrightness(t.instrument()))
    return std::unique_ptr<Performer>(new SingingArcPlayer(static_cast<SingingArcModel&>(t)));
  if (t.instrument().plucked() && fxGoblins(t.instrument()))
    return std::unique_ptr<Performer>(new PyrophonePlayer(static_cast<PyrophoneModel&>(t)));
  if (t.instrument().plucked() && fxEchoes(t.instrument()))
    return std::unique_ptr<Performer>(new EchoesPlayer(static_cast<EchoesModel&>(t)));
  if (t.instrument().plucked() && barInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new BarsPlayer(static_cast<BarsModel&>(t)));
  if (t.instrument().plucked() && guitarHarmonics(t.instrument()))
    return std::unique_ptr<Performer>(new HarmonicsPlayer(static_cast<GuitarModel&>(t)));
  if (t.instrument().plucked() && jazzGuitarInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new JazzGuitarPlayer(static_cast<GuitarModel&>(t)));
  if (t.instrument().plucked() && guitarInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new GuitarPlayer(static_cast<GuitarModel&>(t)));
  if (t.instrument().plucked() && pickBassInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new PickBassPlayer(static_cast<ElectricBassModel&>(t)));
  if (t.instrument().plucked() && slapBassInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new SlapBassPlayer(static_cast<ElectricBassModel&>(t), slapBassInstrument(t.instrument())));
  if (t.instrument().plucked() && electricBassInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new ElectricBassPlayer(static_cast<ElectricBassModel&>(t)));
  if (t.instrument().plucked() && organRank(t.instrument()))
    return std::unique_ptr<Performer>(new OrganPlayer(static_cast<OrganModel&>(t)));
  if (t.instrument().plucked() && accordionInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new AccordionPlayer(static_cast<AccordionModel&>(t)));
  if (t.instrument().plucked() && harmonicaInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new HarmonicaPlayer(static_cast<HarmonicaModel&>(t)));
  if (t.instrument().plucked() && harmoniumInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new HarmoniumPlayer(static_cast<HarmoniumModel&>(t)));
  if (t.instrument().plucked() && hammondInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new HammondPlayer(static_cast<HammondModel&>(t)));
  if (t.instrument().plucked() && hiHat(t.instrument()))
    return std::unique_ptr<Performer>(new HiHatPlayer(static_cast<HiHatModel&>(t)));
  if (t.instrument().plucked() && jazzKick(t.instrument()))
    return std::unique_ptr<Performer>(new KickPlayer(static_cast<KickModel&>(t)));
  if (t.instrument().plucked() && shakerInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new ShakerPlayer(static_cast<ShakerModel&>(t)));
  if (t.instrument().plucked() && cymbal(t.instrument()))
    return std::unique_ptr<Performer>(new CymbalPlayer(static_cast<CymbalModel&>(t)));
  if (t.instrument().plucked() && jazzTom(t.instrument()))
    return std::unique_ptr<Performer>(new TomPlayer(static_cast<TomModel&>(t)));
  if (t.instrument().plucked() && timpaniInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new TimpaniPlayer(static_cast<TimpaniModel&>(t)));
  if (t.instrument().plucked() && harpInstrument(t.instrument()))
    return std::unique_ptr<Performer>(new HarpPlayer(static_cast<HarpModel&>(t)));
  if (t.instrument().plucked() && metalPerc(t.instrument()))
    return std::unique_ptr<Performer>(new MetalPercPlayer(static_cast<MetalPercModel&>(t)));
  if (t.instrument().plucked() && brushSnare(t.instrument()))
    return std::unique_ptr<Performer>(new SnarePlayer(static_cast<SnareModel&>(t)));
  if (t.instrument().plucked() && stickSnare(t.instrument()))
    return std::unique_ptr<Performer>(new StickSnarePlayer(static_cast<StickSnareModel&>(t)));
  if (t.instrument().plucked() && scraper(t.instrument()))
    return std::unique_ptr<Performer>(new ScraperPlayer(static_cast<ScraperModel&>(t)));
  if (t.instrument().plucked() && handDrum(t.instrument()))
    return std::unique_ptr<Performer>(new HandDrumPlayer(static_cast<HandDrumModel&>(t)));
  if (t.instrument().plucked() && percExtras(t.instrument()))
    return std::unique_ptr<Performer>(new PercExtrasPlayer(static_cast<PercExtrasModel&>(t)));
  if (t.instrument().plucked() && handClap(t.instrument()))
    return std::unique_ptr<Performer>(new ClapPlayer(static_cast<ClapModel&>(t)));
  if (t.instrument().plucked() && woodPerc(t.instrument()))
    return std::unique_ptr<Performer>(new WoodPercPlayer(static_cast<WoodPercModel&>(t)));
  if (t.instrument().plucked() && surdo(t.instrument()))
    return std::unique_ptr<Performer>(new SurdoPlayer(static_cast<SurdoModel&>(t)));
  if (t.instrument().plucked() && bellTree(t.instrument()))
    return std::unique_ptr<Performer>(new BellTreePlayer(static_cast<BellTreeModel&>(t)));
  if (t.instrument().plucked()) {
    BassModel& b = static_cast<BassModel&>(t);
    return std::unique_ptr<Performer>(bowedBass(t.instrument()) ? new BowPlayer(b) : new BassPlayer(b));
  }
  if (t.instrument().modal()) return std::unique_ptr<Performer>(new ModalPlayer(static_cast<ModalModel&>(t)));
  return std::unique_ptr<Performer>(new Player(static_cast<Flute&>(t)));
}

Voice::Voice(const Instrument& inst)
    : model_(makeModel(inst)), player_(makePlayer(*model_)), rs_(int(inst.fs), 48000) {}

void Voice::render(float* out, int n, const float* room) {
  const float g = float(instrument().gain * levelTrim(instrument()));
  if (rs_.identity()) {
    if (room) model_->setRoom(room, n);
    player_->render(out, n);
  } else {
    const int need = rs_.needed(n);
    in_.resize(need);
    if (room) {
      // Linear interpolation to the model's rate (the room's sound is mostly
      // below 5 kHz: the bass flute's 24 kHz sees little aliasing).
      room_.resize(need);
      for (int i = 0; i < need; ++i) {
        const double t = (i + 1) * double(n) / need - 1;  // in input samples
        const int k = int(std::floor(t));
        const double f = t - k;
        const float a = k < 0 ? roomLast_ : room[std::min(k, n - 1)];
        const float b = room[std::min(k + 1, n - 1)];
        room_[i] = float(a + f * (b - a));
      }
      roomLast_ = room[n - 1];
      model_->setRoom(room_.data(), need);
    }
    player_->render(in_.data(), need);
    rs_.process(in_.data(), out, n);
  }
  if (g != 1)
    for (int i = 0; i < n; ++i) out[i] *= g;
}

}  // namespace pajupilli
