#!/usr/bin/env python3
"""Compares two MIDI synthesizers on the same MIDI file and ranks their differences by how much
a listener would notice them (research/synth_compare.md).

    tools/synthcompare.py song.mid                    # the song's instruments, note for note:
                                                      # pajupilli against TiMidity + Jnsgm2
    tools/synthcompare.py song.mid --b timidity:/usr/share/sounds/sf2/FluidR3_GM.sf2
    tools/synthcompare.py song.mid --ref timidity:A.sf2,timidity:B.sf2
    tools/synthcompare.py profile timidity:X.sf2 --programs 0,32,drums --out x.json
    tools/synthcompare.py compare a.json b.json [ref.json ...] --out dir/
    tools/synthcompare.py song.mid --song --until 60  # the song itself, stem by stem

The default mode plays each instrument's test notes (keys a few semitones apart over the song's
range, velocities 40/64/96/127, 2 s each) and stores a profile: numbers only, per note and per
instrument (levels, velocity curve, brightness over key and velocity, key-to-key consistency,
envelope, decay, release, formants, pitch, overtones, a coarse spectrogram). Profiles are
compared without audio (Zimtohrli per note uses the cached renders when they are still there).
The rest of this note describes the --song mode.

Each MIDI channel is rendered on its own (a copy of the file with only that channel's notes) and
the whole file once. Every render is aligned to the MIDI, not to the other render. Notes that
start together on a channel are one event (a chord). Each event and each stem gets a panel of
measures (level, velocity response, evenness, spectral shape, brightness, noisiness, attack,
transient coherence, decay, length, pitch, overtones, Plomp-Levelt dissonance, modulation, stereo,
clicks, Zimtohrli). Neither synth is right: the references (by default three SoundFonts, B among
them) give the yardstick. A difference is scored as |A - B| over the larger of its JND and the
references' own spread, weighted by the channel's loudness and the measure's weight. Whether A
lies outside the references' range is reported with it.

Renderers: pajupilli[:ARGS] (default args: -director off), timidity:SF2, wav:DIR (mix.wav and
chN.wav already rendered or recorded). Renders are cached in ~/.cache/synthcompare.
"""
import argparse, hashlib, json, math, os, re, struct, subprocess, sys, time
from concurrent.futures import ThreadPoolExecutor
import numpy as np
from scipy.io import wavfile
from scipy.signal import lfilter, find_peaks, resample_poly

FS = 48000
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PJ = os.path.join(ROOT, 'pajupilli')
CACHE = os.path.expanduser('~/.cache/synthcompare')
SF_DEFAULT_B = os.path.expanduser('~/soundfonts/Jnsgm2.sf2')
SF_REFS = ['/usr/share/sounds/sf2/FluidR3_GM.sf2', '/usr/share/sounds/sf2/TimGM6mb.sf2']
ZIMT_PATHS = [os.path.expanduser('~/zimtohrli/build/lib.linux-x86_64-cpython-%d%d' % sys.version_info[:2])]

# ---------------------------------------------------------------------------------------------
# MIDI


def _vlq(b, i):
    v = 0
    while True:
        c = b[i]; i += 1; v = (v << 7) | (c & 127)
        if c < 128: return v, i


def _enc_vlq(v):
    out = [v & 127]; v >>= 7
    while v: out.append((v & 127) | 128); v >>= 7
    return bytes(reversed(out))


def read_smf(path):
    b = open(path, 'rb').read()
    assert b[:4] == b'MThd', 'not a MIDI file'
    fmt, ntr, div = struct.unpack('>HHH', b[8:14])
    i = 8 + struct.unpack('>I', b[4:8])[0]
    tracks = []
    for _ in range(ntr):
        while b[i:i + 4] != b'MTrk':  # (skip unknown chunks)
            i += 8 + struct.unpack('>I', b[i + 4:i + 8])[0]
        n = struct.unpack('>I', b[i + 4:i + 8])[0]; j = i + 8; end = j + n; i = end
        tick = 0; run = None; ev = []
        while j < end:
            d, j = _vlq(b, j); tick += d
            s = b[j]
            if s == 0xFF:
                typ = b[j + 1]; ln, k = _vlq(b, j + 2); ev.append((tick, 'meta', typ, b[k:k + ln])); j = k + ln
            elif s in (0xF0, 0xF7):
                ln, k = _vlq(b, j + 1); ev.append((tick, 'sysex', s, b[k:k + ln])); j = k + ln
            else:
                if s & 0x80: run = s; j += 1
                nd = 1 if (run & 0xF0) in (0xC0, 0xD0) else 2
                ev.append((tick, 'ch', run, bytes(b[j:j + nd]))); j += nd
        tracks.append(ev)
    return fmt, div, tracks


def tempo_map(div, tracks):
    tm = sorted((e[0], struct.unpack('>I', b'\0' + e[3])[0]) for tr in tracks for e in tr if e[1] == 'meta' and e[2] == 0x51)
    if not tm or tm[0][0] != 0: tm = [(0, 500000)] + tm
    segs = []; sec = 0.0
    for k, (t, us) in enumerate(tm):
        segs.append((t, sec, us))
        if k + 1 < len(tm): sec += (tm[k + 1][0] - t) * us / 1e6 / div
    ticks = np.array([s[0] for s in segs])

    def t2s(tick):
        g = segs[max(0, int(np.searchsorted(ticks, tick, 'right')) - 1)]
        return g[1] + (tick - g[0]) * g[2] / 1e6 / div
    return t2s


def midi_notes(path):
    fmt, div, tracks = read_smf(path); t2s = tempo_map(div, tracks)
    out = []; prog = {}
    for tr in tracks:
        on = {}
        for tick, kind, s, d in tr:
            if kind != 'ch': continue
            ch = s & 15; hi = s & 0xF0
            if hi == 0xC0: prog.setdefault(ch, d[0])
            if hi == 0x90 and d[1] > 0: on.setdefault((ch, d[0]), []).append((tick, d[1]))
            elif hi == 0x80 or (hi == 0x90 and d[1] == 0):
                L = on.get((ch, d[0]))
                if L:
                    t0, v = L.pop(0)
                    out.append(dict(ch=ch, key=d[0], vel=v, on=t2s(t0), off=t2s(tick)))
    out.sort(key=lambda n: n['on'])
    return out, prog


def write_filtered(src, dst, keep, velscale=1.0):
    """Every event, but the notes of channels not in keep (None: all); velocities times velscale."""
    fmt, div, tracks = read_smf(src)
    chunks = []
    for tr in tracks:
        body = bytearray(); last = 0
        for tick, kind, s, d in tr:
            if kind == 'ch' and (s & 0xF0) in (0x80, 0x90, 0xA0) and keep is not None and (s & 15) not in keep: continue
            body += _enc_vlq(tick - last); last = tick
            if kind == 'meta': body += bytes([0xFF, s]) + _enc_vlq(len(d)) + d
            elif kind == 'sysex': body += bytes([s]) + _enc_vlq(len(d)) + d
            else:
                if (s & 0xF0) == 0x90 and d[1] > 0 and velscale != 1.0:
                    d = bytes([d[0], min(127, max(1, int(round(d[1] * velscale))))])
                body += bytes([s]) + d
        if not tr or tr[-1][1] != 'meta' or tr[-1][2] != 0x2F: body += b'\0\xff\x2f\0'
        chunks.append(b'MTrk' + struct.pack('>I', len(body)) + bytes(body))
    open(dst, 'wb').write(b'MThd' + struct.pack('>IHHH', 6, fmt, len(chunks), div) + b''.join(chunks))


# ---------------------------------------------------------------------------------------------
# Rendering


def load_wav(path):
    r, x = wavfile.read(path)
    if x.dtype == np.int16: x = x / 32768.0
    elif x.dtype == np.int32: x = x / 2.0 ** 31
    elif x.dtype == np.uint8: x = (x - 128) / 128.0
    x = np.asarray(x, np.float64)
    if x.ndim == 1: x = np.stack([x, x], 1)
    if r != FS: x = resample_poly(x, FS, r, axis=0)
    return x


def timidity_cfg(sf):
    os.makedirs(CACHE, exist_ok=True)
    cfg = os.path.join(CACHE, 'tim_%s.cfg' % hashlib.sha1(sf.encode()).hexdigest()[:10])
    # Every program and drum key mapped again: Debian's timidity.cfg maps each program to FluidR3
    # explicitly, which a plain "soundfont" line does not override; and no amp= trims.
    L = ['soundfont "%s"' % sf, 'bank 0'] + ['%d %%font "%s" 0 %d' % (p, sf, p) for p in range(128)]
    L += ['drumset 0'] + ['%d %%font "%s" 128 0 %d' % (k, sf, k) for k in range(27, 88)]
    open(cfg, 'w').write('\n'.join(L) + '\n')
    return cfg


class Renderer:
    def __init__(self, spec):
        self.spec = spec
        kind, _, arg = spec.partition(':')
        self.kind, self.arg = kind, arg
        if kind == 'pajupilli':
            self.name = 'pajupilli' + (' ' + arg if arg else '')
            self.args = arg.split() if arg else ['-director', 'off']
        elif kind == 'timidity':
            self.arg = os.path.expanduser(arg)
            self.name = os.path.splitext(os.path.basename(self.arg))[0]
        elif kind == 'wav':
            self.name = os.path.basename(arg.rstrip('/'))
        else:
            raise SystemExit('unknown renderer ' + spec)
        self.short = self.name.split()[0]
        self.notes = []  # what the renderer printed worth reporting

    def key(self, midbytes, until):
        h = hashlib.sha1(midbytes)
        h.update(self.spec.encode()); h.update(str(until).encode())
        if self.kind == 'pajupilli':
            h.update(str(os.path.getmtime(PJ)).encode())
            h.update(json.dumps(sorted((k, v) for k, v in os.environ.items() if k.startswith('PJ_'))).encode())
        elif self.kind == 'timidity':
            h.update(str(os.path.getmtime(self.arg)).encode())
        return h.hexdigest()[:16]

    def render(self, mid, until, tag):
        """mid: a MIDI file; tag: 'mix' or 'chN'. Returns (stereo float array, log)."""
        if self.kind == 'wav':
            return load_wav(os.path.join(self.arg, tag + '.wav')), ''
        os.makedirs(CACHE, exist_ok=True)
        k = self.key(open(mid, 'rb').read(), until)
        wav, log = os.path.join(CACHE, k + '.wav'), os.path.join(CACHE, k + '.log')
        if not os.path.exists(wav):
            if self.kind == 'pajupilli':
                env = dict(os.environ, PJ_PLAY_VELMEDIAN='1000')  # (the velocities are scaled beforehand)
                if until: env['PJ_PLAY_UNTIL'] = str(until)
                cmd = [PJ] + self.args + ['-play', mid, '-o', wav + '.tmp.wav']
            else:
                env = dict(os.environ)
                cmd = ['timidity', '-c', timidity_cfg(self.arg), '-s', str(FS), '-OwS2', '-EFreverb=d', '-EFchorus=d',
                       '-o', wav + '.tmp.wav', mid]
            r = subprocess.run(cmd, env=env, capture_output=True, text=True)
            open(log, 'w').write(r.stdout + r.stderr)
            if r.returncode != 0 or not os.path.exists(wav + '.tmp.wav'):
                raise SystemExit('render failed: %s\n%s' % (' '.join(cmd), (r.stdout + r.stderr)[-2000:]))
            os.replace(wav + '.tmp.wav', wav)
        x = load_wav(wav); text = open(log).read()
        m = re.search(r'peak [\d.]+: the whole song (-?[\d.]+) dB down', text)
        if m: x = x * 10 ** (-float(m.group(1)) / 20)  # (undo pajupilli's mastering)
        if until: x = x[:int(until * FS)]
        return x, text


def pj_velscale(notes):
    v = sorted(n['vel'] for n in notes)
    if not v: return 1.0
    med = v[len(v) // 2]
    return 96.0 / med if med > 96 else 1.0


# ---------------------------------------------------------------------------------------------
# Signal tools


def kweight(x):
    f0, g, q = 1681.974450955533, 3.999843853973347, 0.7071752369554196
    k = math.tan(math.pi * f0 / FS); vh = 10 ** (g / 20); vb = vh ** 0.4996667741545416
    a0 = 1 + k / q + k * k
    b = [(vh + vb * k / q + k * k) / a0, 2 * (k * k - vh) / a0, (vh - vb * k / q + k * k) / a0]
    a = [1, 2 * (k * k - 1) / a0, (1 - k / q + k * k) / a0]
    x = lfilter(b, a, x, axis=0)
    f0, q = 38.13547087602444, 0.5003270373238773; k = math.tan(math.pi * f0 / FS)
    return lfilter([1, -2, 1], [1, 2 * (k * k - 1) / (1 + k / q + k * k), (1 - k / q + k * k) / (1 + k / q + k * k)], x, axis=0)


def lufs(x):
    """Integrated loudness (BS.1770, gated)."""
    y = kweight(x); p = (y ** 2).sum(axis=1)
    B, H = int(0.4 * FS), int(0.1 * FS)
    if len(p) < B: return -99.0
    c = np.concatenate([[0], np.cumsum(p)])
    idx = np.arange(0, len(p) - B, H)
    z = (c[idx + B] - c[idx]) / B
    l = -0.691 + 10 * np.log10(z + 1e-20); z = z[l > -70]
    if not len(z): return -99.0
    g = -0.691 + 10 * np.log10(z.mean()) - 10
    z = z[-0.691 + 10 * np.log10(z) > g]
    return float(-0.691 + 10 * np.log10(z.mean()))


def erbn(f): return 21.4 * np.log10(1 + 0.00437 * f)
def inv_erbn(e): return (10 ** (e / 21.4) - 1) / 0.00437


EDGES = inv_erbn(np.arange(erbn(40), erbn(16000), 1.0))
FC = np.sqrt(EDGES[:-1] * EDGES[1:])
NB = len(FC)
# Coarse bands for envelopes (transient coherence, modulation): 8 bands, ~equal on the ERB scale.
CEDGES = inv_erbn(np.linspace(erbn(50), erbn(12000), 9))


def spectrum(m, nfft=None):
    n = len(m)
    nfft = nfft or max(4096, 1 << int(math.ceil(math.log2(max(n, 2)))))
    P = np.abs(np.fft.rfft(m * np.hanning(n), nfft)) ** 2
    return np.fft.rfftfreq(nfft, 1 / FS), P


_band_cache = {}


def erb_bands(f, P):
    key = (len(f), f[1])
    if key not in _band_cache:
        _band_cache[key] = np.searchsorted(f, EDGES)
    idx = _band_cache[key]
    c = np.concatenate([[0], np.cumsum(P)])
    return c[idx[1:]] - c[idx[:-1]]


def db(x, floor=1e-14): return 10 * np.log10(np.maximum(x, floor))


def band_envelopes(m, hop=0.002):
    """Energy envelopes of the 8 coarse bands, one value per hop (STFT, 256-sample frames)."""
    H = int(hop * FS); N = 512
    if len(m) < N + H: return None
    nfr = (len(m) - N) // H + 1
    idx = np.arange(N)[None, :] + H * np.arange(nfr)[:, None]
    F = np.abs(np.fft.rfft(m[idx] * np.hanning(N), axis=1)) ** 2
    f = np.fft.rfftfreq(N, 1 / FS)
    b = np.searchsorted(f, CEDGES)
    return np.stack([F[:, b[i]:b[i + 1]].sum(axis=1) for i in range(8)], 1)  # (frames, 8)


def sethares(freqs, amps):
    """Plomp-Levelt sensory dissonance of a set of partials (Sethares' parameterisation), amplitudes
    normalised to the largest; the min of each pair's amplitudes weights it."""
    if len(freqs) < 2: return 0.0
    a = amps / amps.max()
    f1, f2 = np.meshgrid(freqs, freqs); a1, a2 = np.meshgrid(a, a)
    fmin = np.minimum(f1, f2); df = np.abs(f2 - f1)
    s = 0.24 / (0.0207 * fmin + 18.96)
    d = np.minimum(a1, a2) * (np.exp(-3.51 * s * df) - np.exp(-5.75 * s * df))
    return float(np.triu(d, 1).sum())


def spectral_peaks(f, P, top=24, floor_db=-50):
    L = db(P); mx = L.max()
    pk, _ = find_peaks(L, height=mx + floor_db, distance=max(1, int(15 / f[1])))
    pk = pk[(f[pk] > 30) & (f[pk] < 12000)]
    if len(pk) > top: pk = pk[np.argsort(L[pk])[-top:]]
    return f[pk], np.sqrt(P[pk])


def harmonic_f0(m, fe, harmonics=8):
    """f0 (Hz) near the expected fe: harmonic-sum search +-60 c, refined on the strongest harmonic."""
    f, P = spectrum(m - m.mean(), 1 << 19); S = np.sqrt(P)
    cand = fe * 2 ** (np.linspace(-60, 60, 241) / 1200)
    hs = np.arange(1, harmonics + 1)
    sc = np.array([np.interp(hs * c, f, S).sum() for c in cand])
    c = cand[int(np.argmax(sc))]
    h = int(hs[np.argmax(np.interp(hs * c, f, S))]); i = int(round(h * c / f[1]))
    if i < 4 or i + 4 >= len(S): return c, f, P
    i = i - 3 + int(np.argmax(S[i - 3:i + 4]))
    a, b, d = np.log(S[i - 1:i + 2] + 1e-20); den = a - 2 * b + d
    p = 0.5 * (a - d) / den if den != 0 else 0
    return (f[i] + p * f[1]) / h, f, P


def partials(f, P, f0, n=12):
    """Level (power) and frequency of partials 1..n: the largest bin within +-3 % of n*f0 (stretched
    search allowed for stiff strings)."""
    lv, fq = [], []
    for k in range(1, n + 1):
        fk = k * f0
        if fk > 16000: break
        lo, hi = np.searchsorted(f, [fk * 0.97, fk * 1.03])
        if hi <= lo: break
        j = lo + int(np.argmax(P[lo:hi]))
        lv.append(P[max(j - 2, 0):j + 3].sum()); fq.append(f[j])
    return np.array(lv), np.array(fq)


# ---------------------------------------------------------------------------------------------
# Events and features


def make_events(notes, ch, until):
    """Notes of a channel starting within 30 ms of each other: one event."""
    ns = [n for n in notes if n['ch'] == ch and n['on'] < until - 0.3]
    ev = []
    for n in ns:
        if ev and n['on'] - ev[-1]['on'] < 0.03: ev[-1]['notes'].append(n)
        else: ev.append(dict(on=n['on'], notes=[n]))
    allns = [n for n in notes if n['ch'] == ch]
    for i, e in enumerate(ev):
        e['next'] = ev[i + 1]['on'] if i + 1 < len(ev) else until
        e['off'] = max(n['off'] for n in e['notes'])
        e['vel'] = max(n['vel'] for n in e['notes'])
        e['key'] = max(n['key'] for n in e['notes'])
        # A single note with nothing else of the channel sounding during its first 0.6 s.
        others = [n for n in allns if n not in e['notes'] and n['on'] < e['on'] + 0.6 and n['off'] > e['on'] + 0.02]
        e['solo'] = len(e['notes']) == 1 and not others
        e['clear'] = not [n for n in allns if n not in e['notes'] and n['on'] < e['on'] and n['off'] > e['on'] - 0.05]
    return ev


def onset_strength(m, hop=0.005):
    H = int(hop * FS); N = 1024
    nfr = (len(m) - N) // H + 1
    if nfr < 4: return np.zeros(1)
    idx = np.arange(N)[None, :] + H * np.arange(nfr)[:, None]
    S = np.log10(np.abs(np.fft.rfft(m[idx] * np.hanning(N), axis=1)) ** 2 + 1e-10)
    d = np.maximum(np.diff(S, axis=0), 0).sum(axis=1)
    return d - d.mean()


def global_offset(x, notes, maxlag=0.6):
    """Seconds the render lags the MIDI: cross-correlation of the onset strength with the note-ons."""
    o = onset_strength(x.mean(axis=1))
    t = np.zeros(len(o))
    for n in notes:
        i = int(n['on'] / 0.005)
        if 0 <= i < len(t): t[i] += n['vel'] / 127
    t = np.convolve(t, np.exp(-0.5 * (np.arange(-4, 5) / 1.5) ** 2), 'same')
    L = int(maxlag / 0.005)
    lags = np.arange(-L // 4, L)
    cc = [np.dot(o[max(l, 0):len(o) + min(l, 0)], t[max(-l, 0):len(t) - max(l, 0)]) for l in lags]
    return float(lags[int(np.argmax(cc))] * 0.005)


def event_features(x, e, off, drum):
    """The per-event measures of one render's stem. x: stereo stem; off: the render's lag (s)."""
    F = {}
    t0 = e['on'] + off
    i0 = int(t0 * FS)
    nxt = e['next'] + off
    span = min(nxt - t0, 1.5)
    if i0 < int(0.03 * FS) or i0 + int(0.05 * FS) >= len(x) or span < 0.06: return F
    m = x[:, 0] + x[:, 1]
    seg = m[i0 - int(0.03 * FS):i0 + int(span * FS)]
    # 1 ms envelope, onset, attack
    p = seg ** 2; n1 = len(p) // 48
    env = p[:n1 * 48].reshape(n1, 48).mean(axis=1)
    envd = db(np.convolve(env, np.ones(3) / 3, 'same'))
    w = envd[:30 + min(150, n1 - 30)]
    pk = w.max(); base = np.median(envd[:25])
    if pk - base > 6:
        j10 = int(np.argmax(w >= pk - 20)); j90 = int(np.argmax(w >= pk - 1)); j6 = int(np.argmax(w >= pk - 6))
        F['delay_ms'] = float(j6 - 30)
        F['log_attack'] = float(math.log10(max(j90 - j10, 0.5) / 1000))
        d = np.diff(envd[max(j10 - 2, 0):j90 + 3])
        F['onset_sharp'] = float(d.max()) if len(d) else 0.0
    body = m[i0 + int(0.03 * FS):i0 + int(min(span, 0.3) * FS)]
    if len(body) < int(0.025 * FS): return F
    f, P = spectrum(body)
    B = erb_bands(f, P); tot = B.sum()
    if tot <= 0: return F
    F['level'] = float(db(tot / len(body)))
    F['erb'] = db(B / tot)
    w8 = B / tot
    F['centroid'] = float((erbn(FC) * w8).sum())
    sel = (f > 100) & (f < 8000)
    if sel.any():
        F['flatness'] = float(10 * (np.log10(P[sel] + 1e-20).mean() - np.log10(P[sel].mean() + 1e-20)))
    att = m[i0:i0 + int(0.03 * FS)]
    fa, Pa = spectrum(att, len(f) * 2 - 2)
    Ba = erb_bands(fa, Pa)
    if Ba.sum() > 0: F['attack_bright'] = float((erbn(FC) * Ba / Ba.sum()).sum() - F['centroid'])
    fr, ar = spectral_peaks(f, P)
    F['dissonance'] = sethares(fr, ar)
    # stereo
    L, R = x[i0:i0 + len(body), 0], x[i0:i0 + len(body), 1]
    pl, pr = (L ** 2).sum(), (R ** 2).sum()
    if pl > 0 and pr > 0:
        F['ild'] = float(10 * math.log10(pl / pr))
        F['icc'] = float(abs((L * R).sum()) / math.sqrt(pl * pr))
    # transient coherence: each coarse band's time to reach 6 dB under its own peak
    E = band_envelopes(m[i0 - int(0.02 * FS):i0 + int(min(span, 0.25) * FS)])
    if E is not None and len(E) > 20:
        Ed = db(E); en = E.max(axis=0)
        ok = en > en.max() * 1e-3
        if ok.sum() >= 3:
            tt = np.array([np.argmax(Ed[:, b] >= Ed[:, b].max() - 6) * 2.0 for b in range(8)])
            wgt = en[ok] / en[ok].sum(); tm = (tt[ok] * wgt).sum()
            F['trans_spread_ms'] = float(math.sqrt((wgt * (tt[ok] - tm) ** 2).sum()))
            lo, hi = tt[ok][:len(tt[ok]) // 2], tt[ok][len(tt[ok]) // 2:]
            F['trans_order_ms'] = float(hi.mean() - lo.mean())
    # decay, sounding length, sustain flux
    if span >= 0.4:
        a = int(0.05 * 1000) + 30; dd = envd[a:]
        if len(dd) > 100:
            k = np.arange(len(dd)) / 1000
            sl = np.polyfit(k, dd, 1)[0]
            F['decay_db_s'] = float(sl)
        E2 = band_envelopes(m[i0 + int(0.1 * FS):i0 + int(span * FS)], hop=0.02)
        if E2 is not None and len(E2) > 4:
            F['flux'] = float(np.abs(np.diff(db(E2), axis=0)).mean())
    dur = e['off'] - e['on']
    if e['next'] - e['off'] >= 0.3 and dur >= 0.05:
        a = 30 + int(np.argmax(envd[30:30 + 150])) if n1 > 60 else 30
        pkv = envd[a]
        below = np.nonzero(envd[a:] < pkv - 20)[0]
        if len(below):
            F['length_ratio'] = float(math.log2(max(below[0] + (a - 30), 1) / 1000 / dur))
        j = int((dur + 0.2) * 1000) + 30
        if j + 200 < len(envd): F['tail_db'] = float(envd[j:j + 200].mean() - pkv)
    if drum or not e['solo']: return F
    # pitch, overtones (a single note alone)
    n = e['notes'][0]
    fe = 440 * 2 ** ((n['key'] - 69) / 12)
    pseg = m[i0 + int(0.12 * FS):i0 + int(min(span, 0.6) * FS)]
    if len(pseg) >= int(0.12 * FS) and fe > 25:
        f0, fp, Pp = harmonic_f0(pseg, fe)
        F['f0_cents'] = float(1200 * math.log2(f0 / fe))
        lv, fq = partials(fp, Pp, f0)
        if len(lv) >= 4 and lv.sum() > 0:
            tot = lv.sum(); r = lv / tot
            F['harm'] = db(np.pad(r, (0, 12 - len(r)), constant_values=1e-14))
            F['tri1'] = float(r[0]); F['tri2'] = float(r[1:4].sum()); F['tri3'] = float(r[4:].sum())
            odd, even = lv[2::2].sum(), lv[1::2].sum()
            if odd > 0 and even > 0: F['odd_even_db'] = float(db(odd / even))
            ld = db(lv)
            F['irregularity'] = float(np.mean(np.abs(ld[1:-1] - (ld[:-2] + ld[1:-1] + ld[2:]) / 3)))
            F['pitch_salience'] = float(db(tot / Pp[(fp > 0.8 * f0) & (fp < 16 * f0)].sum()))
            ks = np.arange(1, len(fq) + 1)
            good = lv > lv.max() * 1e-4
            if good.sum() >= 4:
                F['stretch_cents'] = float(np.polyfit(ks[good], 1200 * np.log2(fq[good] / (ks[good] * f0)), 1)[0])
        early = m[i0 + int(0.02 * FS):i0 + int(0.12 * FS)]
        if len(early) > int(0.08 * FS):
            fe0, _, _ = harmonic_f0(early, f0)
            F['glide_cents'] = float(1200 * math.log2(fe0 / f0))
        # vibrato depth and partial-envelope correlation over the note
        if span >= 0.5:
            w = int(0.08 * FS); hop = int(0.04 * FS); cs = []; envs = []
            for s0 in range(i0 + int(0.1 * FS), i0 + int(min(span, 1.2) * FS) - w, hop):
                fv, fpp, Ppp = harmonic_f0(m[s0:s0 + w], f0, 4)
                cs.append(1200 * math.log2(fv / f0))
                lv2, _ = partials(fpp, Ppp, f0, 6)
                if len(lv2) == 6: envs.append(db(lv2))
            if len(cs) >= 6: F['vib_cents'] = float(np.std(cs) * 2 * math.sqrt(2))
            if len(envs) >= 6:
                Em = np.array(envs); C = np.corrcoef(Em.T)
                if np.all(np.isfinite(C)): F['partial_corr'] = float(C[np.triu_indices(6, 1)].mean())
    return F


def stem_features(x, notes_ch, off, until):
    """Per-stem measures that are not per event."""
    F = {}
    m = x[:, 0] + x[:, 1]
    # LTAS on the ERB bands
    N = 8192; H = 4096
    nfr = (len(m) - N) // H + 1
    if nfr > 2:
        acc = np.zeros(N // 2 + 1)
        for i in range(nfr): acc += np.abs(np.fft.rfft(m[i * H:i * H + N] * np.hanning(N))) ** 2
        B = erb_bands(np.fft.rfftfreq(N, 1 / FS), acc)
        F['ltas'] = db(B / B.sum())
    # modulation: the coarse bands' log envelopes at 10 ms, power in 0.5-2, 2-8, 8-32 Hz re total
    E = band_envelopes(m, hop=0.01)
    if E is not None and len(E) > 400:
        Ed = db(E / E.max()); Ed = np.maximum(Ed, -60)
        sp = np.abs(np.fft.rfft(Ed - Ed.mean(axis=0), axis=0)) ** 2
        fm = np.fft.rfftfreq(len(Ed), 0.01)
        tot = sp[(fm > 0.3) & (fm < 50)].sum(axis=0) + 1e-12
        for lo, hi, nm in ((0.5, 2, 'mod_slow'), (2, 8, 'mod_mid'), (8, 32, 'mod_fast')):
            F[nm] = float(db((sp[(fm >= lo) & (fm < hi)].sum(axis=0) / tot).mean()))
    # stereo over the stem
    pl, pr = (x[:, 0] ** 2).sum(), (x[:, 1] ** 2).sum()
    if pl > 0 and pr > 0:
        F['stem_ild'] = float(10 * math.log10(pl / pr))
        F['stem_icc'] = float(abs((x[:, 0] * x[:, 1]).sum()) / math.sqrt(pl * pr))
    # clicks: >4 kHz envelope peaks 20 dB over their 50 ms median, not within 40 ms of a note-on
    hp = lfilter([1, -1], [1, -0.55], m)
    hp = lfilter(*_butter_hp4k(), hp)
    e1 = (hp ** 2)[:len(hp) // 48 * 48].reshape(-1, 48).mean(axis=1)
    if len(e1) > 100:
        from scipy.ndimage import median_filter
        med = median_filter(e1, 51)
        cand = np.nonzero(db(e1) > db(med) + 20)[0]
        ons = np.array(sorted(n['on'] + off for n in notes_ch))
        k = 0; last = -1
        for c in cand:
            t = c / 1000
            j = np.searchsorted(ons, t)
            near = (j < len(ons) and ons[j] - t < 0.04) or (j > 0 and t - ons[j - 1] < 0.04)
            if not near and db(e1[c]) > db(e1.max()) - 50 and c - last > 20: k += 1; last = c
        F['clicks_per_min'] = k / (len(e1) / 60000)
    # noise floor in gaps (nothing of the channel sounding for 1.5 s after the last note-off)
    spans = sorted((n['on'] + off, n['off'] + off) for n in notes_ch)
    gaps = []; end = 0.0
    for a, b in spans:
        if a - end > 2.0: gaps.append((end + 1.5, a - 0.05))
        end = max(end, b)
    if until - end > 2.0: gaps.append((end + 1.5, until - 0.1))
    gl = [m[int(a * FS):int(b * FS)] for a, b in gaps if b - a > 0.2]
    if gl:
        g = np.concatenate(gl)
        F['noise_db'] = float(db((g ** 2).mean()) - db((m ** 2).mean()))
    # dissonance over time, 200 ms frames where the stem sounds
    ds = []
    for s0 in range(0, len(m) - 9600, 9600):
        fr = m[s0:s0 + 9600]
        if (fr ** 2).mean() < (m ** 2).mean() * 0.05: continue
        f, P = spectrum(fr); a, b = spectral_peaks(f, P)
        ds.append(sethares(a, b))
    if ds: F['stem_dissonance'] = float(np.median(ds))
    return F


_bhp = None


def _butter_hp4k():
    global _bhp
    if _bhp is None:
        from scipy.signal import butter
        _bhp = butter(4, 4000, 'highpass', fs=FS)
    return _bhp


# Each scalar measure: (label, unit, JND, weight, how it is pooled over a channel's events)
MEASURES = {
    'balance': ('balance in the mix (stem LUFS - mix LUFS)', 'LU', 1.0, 1.5),
    'vel_range': ('loudness range over velocity 32..127', 'dB', 2.0, 1.0),
    'evenness': ('note-to-note level scatter (residual sd)', 'dB', 0.7, 0.8),
    'level_by_key': ('level slope over the keyboard', 'dB/oct', 1.0, 0.6),
    'centroid': ('brightness (spectral centroid, first 30-300 ms)', 'ERB', 0.3, 1.0),
    'attack_bright': ('attack brighter than the body by', 'ERB', 0.5, 0.6),
    'flatness': ('noisiness (spectral flatness, 0.1-8 kHz)', 'dB', 2.0, 0.6),
    'delay_ms': ('onset delay after the MIDI note-on', 'ms', 10.0, 0.8),
    'log_attack': ('attack time (log10 s, 10-90 %)', '', 0.1, 0.9),
    'onset_sharp': ('onset sharpness (steepest rise)', 'dB/ms', 2.0, 0.5),
    'trans_spread_ms': ('transient coherence: attack spread over bands', 'ms', 5.0, 0.7),
    'trans_order_ms': ('high bands start later than low by', 'ms', 5.0, 0.4),
    'decay_db_s': ('decay rate over a held note', 'dB/s', 3.0, 0.8),
    'flux': ('how much the sustain moves (spectral flux)', 'dB/frame', 0.5, 0.5),
    'length_ratio': ('sounding length / MIDI length (log2)', 'oct', 0.15, 0.8),
    'tail_db': ('release tail 0.2-0.4 s after note-off, re peak', 'dB', 3.0, 0.6),
    'f0_cents': ('pitch (median re 12-TET A440)', 'c', 5.0, 0.8),
    'f0_scatter': ('pitch scatter between notes', 'c', 3.0, 0.8),
    'glide_cents': ('pitch at the start re the settled pitch', 'c', 5.0, 0.5),
    'vib_cents': ('vibrato / pitch wobble depth (p-p)', 'c', 5.0, 0.6),
    'tri1': ('tristimulus 1 (fundamental share)', '', 0.07, 0.6),
    'tri2': ('tristimulus 2 (partials 2-4 share)', '', 0.07, 0.5),
    'tri3': ('tristimulus 3 (partials 5+ share)', '', 0.07, 0.6),
    'odd_even_db': ('odd / even partials', 'dB', 2.0, 0.5),
    'irregularity': ('spectral irregularity (partial to partial)', 'dB', 1.0, 0.4),
    'pitch_salience': ('pitch salience (partials re all energy)', 'dB', 1.0, 0.4),
    'stretch_cents': ('partial stretch (inharmonicity)', 'c/partial', 0.5, 0.5),
    'partial_corr': ('partial envelopes alike (correlation)', '', 0.1, 0.4),
    'dissonance': ('Plomp-Levelt dissonance per note/chord (log2)', 'oct', 0.3, 0.7),
    'ild': ('left-right level per note', 'dB', 1.0, 0.3),
    'icc': ('interchannel coherence per note', '', 0.075, 0.3),
    'stem_ild': ('stem position (left-right level)', 'dB', 1.0, 0.4),
    'stem_icc': ('stem width (interchannel coherence)', '', 0.075, 0.4),
    'mod_slow': ('envelope modulation 0.5-2 Hz', 'dB', 1.5, 0.3),
    'mod_mid': ('envelope modulation 2-8 Hz (vibrato, tremolo, beats)', 'dB', 1.5, 0.4),
    'mod_fast': ('envelope modulation 8-32 Hz (roughness, Rotary)', 'dB', 1.5, 0.4),
    'clicks_per_min': ('clicks per minute', '/min', 1.0, 0.8),
    'noise_db': ('noise floor in gaps re stem', 'dB', 6.0, 0.5),
    'stem_dissonance': ('Plomp-Levelt dissonance over the stem (log2)', 'oct', 0.3, 0.6),
    'zimt': ('Zimtohrli distance (relative to the references\' spread)', '', None, 0.5),
}
VECTORS = {
    'erb': ('spectral shape per note (ERB bands, re total)', 1.0, 1.2),
    'ltas': ('long-term spectrum of the stem (ERB bands)', 1.0, 1.0),
    'harm': ('overtone profile (partials 1-12, re total)', 1.5, 0.8),
}
LOG2_MEASURES = {'dissonance', 'stem_dissonance'}


def channel_summary(evf, events, stem, bal):
    """Pools one render's per-event measures of a channel into channel numbers."""
    S = dict(stem); S['balance'] = bal
    keys = set(k for f in evf for k in f)
    for k in keys:
        if k in VECTORS:
            vs = [f[k] for f in evf if k in f]
            if vs:
                # (energy-weighted mean in power, back to dB)
                S[k] = db(np.mean([10 ** (v / 10) for v in vs], axis=0))
            continue
        vs = [f[k] for f in evf if k in f]
        if len(vs) < 3: continue
        v = np.array(vs)
        if k in LOG2_MEASURES: v = np.log2(np.maximum(v, 1e-6))
        S[k] = float(np.median(v))
        if k == 'f0_cents': S['f0_scatter'] = float(np.median(np.abs(v - np.median(v))) * 1.4826)
    # velocity response and evenness: level ~ a + b log v + c key
    rows = [(e['vel'], e['key'], f['level']) for e, f in zip(events, evf) if 'level' in f and e['clear']]
    if len(rows) >= 8:
        R = np.array(rows, float)
        lv = np.log10(R[:, 0] / 127)
        X = np.stack([np.ones(len(R)), lv, (R[:, 1] - 60) / 12], 1)
        if np.ptp(R[:, 0]) >= 10:
            c, *_ = np.linalg.lstsq(X, R[:, 2], rcond=None)
            S['vel_range'] = float(c[1] * math.log10(127 / 32))
        else:
            X = X[:, [0, 2]]; c, *_ = np.linalg.lstsq(X, R[:, 2], rcond=None); c = np.r_[c[0], 0, c[1]]
            X = np.stack([np.ones(len(R)), lv, (R[:, 1] - 60) / 12], 1)
        S['level_by_key'] = float(c[2])
        res = R[:, 2] - X @ c
        S['evenness'] = float(np.std(res))
    return S


# ---------------------------------------------------------------------------------------------
# Zimtohrli


def zimt():
    for p in ZIMT_PATHS:
        if os.path.isdir(p) and p not in sys.path: sys.path.insert(0, p)
    try:
        import pyohrli
        return pyohrli.Pyohrli()
    except Exception:
        return None


def zmono(x, seconds=None):
    m = x.mean(axis=1)
    if seconds: m = m[:int(seconds * FS)]
    pk = np.abs(m).max()
    return (m / pk * 0.5 if pk > 0 else m).astype(np.float32)


# ---------------------------------------------------------------------------------------------
# Main


GM = ('Acoustic Grand Piano,Bright Acoustic Piano,Electric Grand Piano,Honky-tonk Piano,Electric Piano 1,Electric Piano 2,'
      'Harpsichord,Clavi,Celesta,Glockenspiel,Music Box,Vibraphone,Marimba,Xylophone,Tubular Bells,Dulcimer,Drawbar Organ,'
      'Percussive Organ,Rock Organ,Church Organ,Reed Organ,Accordion,Harmonica,Tango Accordion,Nylon Guitar,Steel Guitar,'
      'Jazz Guitar,Clean Guitar,Muted Guitar,Overdriven Guitar,Distortion Guitar,Guitar Harmonics,Acoustic Bass,'
      'Finger Bass,Pick Bass,Fretless Bass,Slap Bass 1,Slap Bass 2,Synth Bass 1,Synth Bass 2,Violin,Viola,Cello,'
      'Contrabass,Tremolo Strings,Pizzicato Strings,Orchestral Harp,Timpani,String Ensemble 1,String Ensemble 2,'
      'Synth Strings 1,Synth Strings 2,Choir Aahs,Voice Oohs,Synth Voice,Orchestra Hit,Trumpet,Trombone,Tuba,Muted Trumpet,'
      'French Horn,Brass Section,Synth Brass 1,Synth Brass 2,Soprano Sax,Alto Sax,Tenor Sax,Baritone Sax,Oboe,English Horn,'
      'Bassoon,Clarinet,Piccolo,Flute,Recorder,Pan Flute,Blown Bottle,Shakuhachi,Whistle,Ocarina').split(',')


def chan_name(ch, prog):
    if ch == 9: return 'ch%d drums' % (ch + 1)
    p = prog.get(ch, 0)
    return 'ch%d %s' % (ch + 1, GM[p] if p < len(GM) else 'program %d' % p)


def main_song(a):

    t_start = time.time()
    notes, prog = midi_notes(a.midi)
    if not notes: raise SystemExit('no notes')
    end = max(n['off'] for n in notes) + 2.0
    until = min(a.until, end) if a.until else end
    chs = sorted(set(n['ch'] for n in notes if n['on'] < until - 0.3))
    if a.channels: chs = [c for c in chs if c + 1 in {int(s) for s in a.channels.split(',')}]
    A = Renderer(a.a); B = Renderer(a.b)
    refs = [Renderer(s) for s in a.ref.split(',') if s and Renderer(s).spec != B.spec]
    rends = [A, B] + refs
    work = os.path.join(CACHE, 'mid'); os.makedirs(work, exist_ok=True)
    base = os.path.splitext(os.path.basename(a.midi))[0]
    h = hashlib.sha1(open(a.midi, 'rb').read()).hexdigest()[:8]
    vs = pj_velscale(notes)

    def midfile(tag, keep, scale):
        p = os.path.join(work, '%s_%s_%s%s.mid' % (base, h, tag, '_v%.3f' % scale if scale != 1 else ''))
        if not os.path.exists(p): write_filtered(a.midi, p, keep, scale)
        return p

    jobs = []
    for r in rends:
        sc = vs if r.kind == 'pajupilli' else 1.0
        jobs.append((r, 'mix', midfile('mix', None, sc)))
        for c in chs: jobs.append((r, 'ch%d' % c, midfile('ch%d' % c, {c}, sc)))
    print('%s: %.0f s of %.0f, channels %s; A = %s, B = %s, references: %s' % (
        os.path.basename(a.midi), until, end - 2, ', '.join(chan_name(c, prog) for c in chs), A.name, B.name,
        ', '.join(r.name for r in refs) or 'none'), file=sys.stderr)
    if vs != 1: print('  (pajupilli scales this file\'s velocities by %.2f: the median is over 96)' % vs, file=sys.stderr)
    audio = {}

    def run(j):
        r, tag, mid = j
        t = time.time(); x, log = r.render(mid, until, tag)
        return r.spec, tag, x, time.time() - t
    # pajupilli renders one at a time (it uses every core itself); the others side by side
    with ThreadPoolExecutor(max_workers=8) as ex:
        futs = [ex.submit(run, j) for j in jobs if j[0].kind != 'pajupilli']
        for j in jobs:
            if j[0].kind == 'pajupilli':
                spec, tag, x, dt = run(j); audio[spec, tag] = x
                print('  rendered %s %s (%.0f s)' % (j[0].short, tag, dt), file=sys.stderr)
        for f in futs:
            spec, tag, x, dt = f.result(); audio[spec, tag] = x
    n = int(until * FS)
    for k in audio:
        x = audio[k]
        audio[k] = np.vstack([x, np.zeros((max(0, n - len(x)), 2))])[:n]

    # Gains: every render's mix to the same loudness (the volume knob says nothing); stems alike.
    gain, mixl, offs, info = {}, {}, {}, {}
    for r in rends:
        l = lufs(audio[r.spec, 'mix']); mixl[r.spec] = l
        gain[r.spec] = 10 ** ((-23 - l) / 20)
        mixsum = sum(audio[r.spec, 'ch%d' % c] for c in chs)
        res = audio[r.spec, 'mix'] - mixsum
        add = db((res ** 2).mean()) - db((audio[r.spec, 'mix'] ** 2).mean())
        offs[r.spec] = global_offset(audio[r.spec, 'mix'], [x for x in notes if x['on'] < until])
        info[r.spec] = dict(name=r.name, mix_lufs=l, additivity_db=float(add), offset_ms=offs[r.spec] * 1000,
                            peak=float(np.abs(audio[r.spec, 'mix']).max()))
        for k in list(audio):
            if k[0] == r.spec: audio[k] = audio[k] * gain[r.spec]

    # Measures
    events = {c: make_events(notes, c, until) for c in chs}
    summ = {}  # spec -> ch -> dict
    share = {}
    for r in rends:
        summ[r.spec] = {}
        ml = lufs(audio[r.spec, 'mix'])
        for c in chs:
            x = audio[r.spec, 'ch%d' % c]
            evf = [event_features(x, e, offs[r.spec], c == 9) for e in events[c]]
            st = stem_features(x, [q for q in notes if q['ch'] == c and q['on'] < until], offs[r.spec], until)
            bal = lufs(x) - ml
            summ[r.spec][c] = channel_summary(evf, events[c], st, bal)
            share.setdefault(c, []).append(bal)
        print('  measured %s' % r.short, file=sys.stderr)

    # Zimtohrli per stem: A-B, and among the references (the yardstick)
    Z = None if a.no_zimt else zimt()
    zres = {}
    if Z is not None:
        zs = min(until, 30.0)
        for c in ['mix'] + chs:
            tag = c if c == 'mix' else 'ch%d' % c
            sig = {r.spec: zmono(audio[r.spec, tag], zs) for r in rends}
            dAB = Z.distance(sig[A.spec], sig[B.spec])
            dA = [Z.distance(sig[A.spec], sig[r.spec]) for r in rends[1:]]
            dR = [Z.distance(sig[p.spec], sig[q.spec]) for i, p in enumerate(rends[1:]) for q in rends[i + 2:]]
            zres[tag] = dict(ab=float(dAB), a_refs=float(np.median(dA)), refs=float(np.median(dR)) if dR else None)
        print('  Zimtohrli done', file=sys.stderr)

    # Differences
    items = []
    loud = {c: 10 ** (np.median(share[c]) / 10) for c in chs}
    tot = sum(loud.values())
    for c in chs:
        wch = math.sqrt(loud[c] / tot)
        sa, sb = summ[A.spec][c], summ[B.spec][c]
        R = [summ[r.spec][c] for r in rends[1:]]
        for k, (label, unit, jnd, wt) in MEASURES.items():
            if k == 'zimt': continue
            if k not in sa or k not in sb: continue
            rv = [s[k] for s in R if k in s]
            spread = (max(rv) - min(rv)) / 2 if len(rv) >= 2 else 0.0
            d = sa[k] - sb[k]
            scale = max(jnd, spread)
            z = abs(d) / scale
            outside = bool(rv) and (sa[k] < min(rv) - jnd / 2 or sa[k] > max(rv) + jnd / 2)
            items.append(dict(ch=c, chan=chan_name(c, prog), measure=k, label=label, unit=unit, a=sa[k], b=sb[k],
                              refs=rv, diff=d, jnd=jnd, spread=spread, z=z, outside=outside,
                              score=min(z, 8) * wt * wch * (1.0 if outside else 0.5)))
        for k, (label, jnd, wt) in VECTORS.items():
            if k not in sa or k not in sb: continue
            va, vb = sa[k], sb[k]
            rv = [s[k] for s in R if k in s]
            ok = (va > -45) | (vb > -45)  # (bands with something in them)
            if ok.sum() < 3: continue
            d = va - vb
            dm = d[ok] - d[ok].mean() if k != 'harm' else d[ok]
            if len(rv) >= 2:
                RV = np.array(rv)[:, ok]
                half = (RV.max(axis=0) - RV.min(axis=0)) / 2
                lo, hi = RV.min(axis=0), RV.max(axis=0)
                excess = np.maximum(0, np.maximum(lo - va[ok], va[ok] - hi))
            else:
                half = np.zeros(ok.sum()); excess = np.abs(dm)
            rms = float(np.sqrt((dm ** 2).mean()))
            spread = float(np.sqrt((half ** 2).mean()))
            z = rms / max(jnd, spread)
            ex = float(np.sqrt((excess ** 2).mean()))
            # the bands where A differs most (outside the references)
            if k == 'harm':
                where = ['partial %d %+.0f dB' % (i + 1, d[ok][i]) for i in np.argsort(-np.abs(d[ok]))[:3]]
            else:
                fcs = FC[ok]
                # group into octave-ish regions
                where = []
                for i in np.argsort(-(excess if len(rv) >= 2 else np.abs(dm)))[:3]:
                    where.append('%s Hz %+.0f dB' % (('%.0f' % fcs[i]) if fcs[i] < 1000 else ('%.1fk' % (fcs[i] / 1000)), dm[i]))
            items.append(dict(ch=c, chan=chan_name(c, prog), measure=k, label=label, unit='dB rms', a=rms, b=0.0,
                              refs=[], diff=rms, jnd=jnd, spread=spread, z=z, outside=ex > jnd, excess=ex, where=where,
                              tilt=float(np.polyfit(erbn(FC[ok]), d[ok], 1)[0] * 10) if k != 'harm' else None,
                              score=min(z, 8) * wt * wch * (1.0 if ex > jnd else 0.5)))
        tag = 'ch%d' % c
        if tag in zres and zres[tag]['refs']:
            zr = zres[tag]; z = zr['ab'] / max(zr['refs'], 1e-6)
            outside = zr['a_refs'] > zr['refs'] * 1.3
            items.append(dict(ch=c, chan=chan_name(c, prog), measure='zimt', label=MEASURES['zimt'][0], unit='',
                              a=zr['ab'], b=zr['refs'], refs=[zr['a_refs']], diff=zr['ab'], jnd=None, spread=zr['refs'],
                              z=z, outside=outside, score=min(z, 8) * MEASURES['zimt'][3] * wch * (1.0 if outside else 0.5)))
    items.sort(key=lambda i: -i['score'])

    # Report
    L = []
    L.append('# %s: %s against %s' % (os.path.basename(a.midi), A.name, B.name))
    L.append('')
    L.append('First %.0f s. References (the yardstick): %s. Each difference is |A - B| over the larger of its JND and '
             'half the references\' range; the score weights that by the measure and by the channel\'s loudness, and '
             'halves it when A lies inside the references\' range (then B differs as much from the other SoundFonts).'
             % (until, ', '.join([B.name] + [r.name for r in refs])))
    L.append('')
    L.append('## Renders')
    L.append('')
    L.append('| render | mix LUFS | offset re MIDI | stems sum to mix within | peak (normalised) |')
    L.append('|---|---|---|---|---|')
    for r in rends:
        i = info[r.spec]
        L.append('| %s | %.1f | %+.0f ms | %.0f dB | %.2f |' % (r.name, i['mix_lufs'], i['offset_ms'], i['additivity_db'], i['peak']))
    L.append('')
    L.append('## Balance (stem LUFS re mix)')
    L.append('')
    L.append('| channel | ' + ' | '.join(r.short for r in rends) + ' |')
    L.append('|---|' + '---|' * len(rends))
    for c in chs:
        L.append('| %s | ' % chan_name(c, prog) + ' | '.join('%+.1f' % summ[r.spec][c]['balance'] for r in rends) + ' |')
    L.append('')
    L.append('## The biggest differences, ranked')
    L.append('')

    def fmt(v):
        if isinstance(v, float):
            return '%.3g' % v if abs(v) < 10 else '%.0f' % v
        return str(v)
    for k, it in enumerate(items[:a.top]):
        flag = 'outside the references' if it['outside'] else 'within the references\' range'
        if it['measure'] in VECTORS:
            s = ('%d. **%s: %s** differs by %.1f dB rms (references among themselves %.1f); %s; most at %s'
                 % (k + 1, it['chan'], it['label'], it['diff'], it['spread'], flag, ', '.join(it['where'])))
            if it.get('tilt') is not None: s += '; tilt %+.1f dB per 10 ERB (A re B)' % it['tilt']
        elif it['measure'] == 'zimt':
            s = ('%d. **%s: Zimtohrli** A-B %.4f, A to the references %.4f, the references among themselves %.4f '
                 '(%.1f x); %s' % (k + 1, it['chan'], it['a'], it['refs'][0], it['b'], it['z'], flag))
        else:
            s = ('%d. **%s: %s**: A %s, B %s %s (others %s); %.1f x the yardstick; %s'
                 % (k + 1, it['chan'], it['label'], fmt(it['a']), fmt(it['b']), it['unit'],
                    ', '.join(fmt(v) for v in it['refs'][1:]) or '-', it['z'], flag))
        L.append(s + '  [score %.2f]' % it['score'])
    L.append('')
    L.append('## Every measure, per channel')
    for c in chs:
        L.append('')
        L.append('### ' + chan_name(c, prog))
        L.append('')
        L.append('| measure | ' + ' | '.join(r.short for r in rends) + ' | JND |')
        L.append('|---|' + '---|' * (len(rends) + 1))
        for k, (label, unit, jnd, wt) in MEASURES.items():
            if k == 'zimt' or all(k not in summ[r.spec][c] for r in rends): continue
            L.append('| %s (%s) | ' % (label, unit or '-') + ' | '.join(
                fmt(summ[r.spec][c][k]) if k in summ[r.spec][c] else '-' for r in rends) + ' | %s |' % fmt(jnd))
        tag = 'ch%d' % c
        if tag in zres:
            L.append('| Zimtohrli to B / to the references / references among themselves | %.4f / %.4f / %s | | | |'
                     % (zres[tag]['ab'], zres[tag]['a_refs'], '%.4f' % zres[tag]['refs'] if zres[tag]['refs'] else '-'))
    if 'mix' in zres:
        L.append('')
        L.append('Zimtohrli on the mix (first %.0f s): A-B %.4f, A to the references %.4f, the references among themselves %s.'
                 % (min(until, 30), zres['mix']['ab'], zres['mix']['a_refs'], '%.4f' % zres['mix']['refs'] if zres['mix']['refs'] else '-'))
    elif Z is None and not a.no_zimt:
        L.append('')
        L.append('(Zimtohrli not found: build ~/zimtohrli for this Python.)')
    L.append('')
    L.append('_%.0f s; %s._' % (time.time() - t_start, time.strftime('%Y-%m-%d %H:%M')))
    text = '\n'.join(L) + '\n'
    out = a.out or os.path.join('/tmp', 'synthcompare_' + base)
    os.makedirs(out, exist_ok=True)
    open(os.path.join(out, 'report.md'), 'w').write(text)

    def js(o):
        if isinstance(o, np.ndarray): return o.tolist()
        if isinstance(o, (np.floating, np.integer)): return o.item()
        return str(o)
    json.dump(dict(midi=a.midi, until=until, renders=info, items=items,
                   summary={rs: {str(c): v for c, v in d.items()} for rs, d in summ.items()}, zimtohrli=zres),
              open(os.path.join(out, 'report.json'), 'w'), default=js, indent=1)
    print(text)
    print('wrote %s/report.md and report.json' % out, file=sys.stderr)



# =============================================================================================
# Note profiles: a synth characterised by numbers, one note at a time.
#
# `profile` plays every instrument's test notes (keys a few semitones apart, velocities 40, 64,
# 96, 127, each held 2 s, then 2 s of silence) on one synth and stores, per note and per
# instrument, only numbers: levels, ERB spectra, envelope curves, a coarse spectrogram, pitch,
# overtones, and the instrument's curves (velocity, brightness, key-to-key consistency, decay,
# formants). `compare` reads two or more profiles and ranks the differences; no audio is needed
# (Zimtohrli is the one exception, used when the renders are still in the cache).

VELS = (40, 64, 96, 127)
SLOT, HOLD = 4.0, 2.0
NBANDS2 = 18  # (the coarse spectrogram: 2-ERB bands)
SG_HOP, SG_N = 0.05, 50


def gm_range(p):
    if p == 'drums': return list(range(35, 82))
    for lo, hi, r in ((0, 7, (21, 108)), (8, 15, (48, 96)), (16, 23, (36, 96)), (24, 31, (40, 84)), (32, 39, (28, 60)),
                      (40, 40, (55, 96)), (41, 41, (48, 84)), (42, 42, (36, 76)), (43, 43, (28, 60)), (44, 51, (36, 96)),
                      (52, 55, (48, 79)), (56, 63, (40, 84)), (64, 71, (48, 84)), (72, 79, (60, 96))):
        if lo <= p <= hi: return r
    return (36, 84)


def test_keys(p, used=None):
    """The keys to play: those of the song's range (if given) within the instrument's, a few
    semitones apart, at most 16."""
    if p == 'drums':
        return sorted(used) if used else gm_range(p)
    lo, hi = gm_range(p)
    if used:
        lo, hi = max(lo, min(used) - 2), min(hi, max(used) + 2)
    step = max(3, int(math.ceil((hi - lo) / 15)))
    return list(range(lo, hi + 1, step))


def write_diag(path, p, keys):
    ch = 9 if p == 'drums' else 0
    tpq = 480; tick = lambda s: int(round(s * 2 * tpq))  # (120 bpm: 1 s = 2 beats)
    ev = [] if p == 'drums' else [(0, bytes([0xC0 | ch, p]))]
    k = 0
    for key in keys:
        for v in VELS:
            t = k * SLOT
            ev.append((tick(t), bytes([0x90 | ch, key, v]))); ev.append((tick(t + HOLD), bytes([0x80 | ch, key, 0]))); k += 1
    ev.sort(key=lambda e: (e[0], e[1][0] & 0xF0 == 0x90))
    body = bytearray(); last = 0
    for t, e in ev: body += _enc_vlq(t - last) + e; last = t
    body += _enc_vlq(tick(SLOT)) + b'\xff\x2f\x00'
    open(path, 'wb').write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, tpq) + b'MTrk' + struct.pack('>I', len(body)) + bytes(body))
    return [(key, v, i * SLOT) for i, (key, v) in enumerate((key, v) for key in keys for v in VELS)]


def r1(v, n=1):
    if isinstance(v, np.ndarray): return [round(float(x), n) for x in v]
    return round(float(v), n)


def note_profile(x, t, key, drum):
    """The numbers of one note. x: the stereo render; t: the note-on (s)."""
    i0 = int(t * FS)
    seg = x[i0:i0 + int(SLOT * FS)]
    if len(seg) < int(SLOT * FS * 0.9): return None
    m = seg[:, 0] + seg[:, 1]
    p = m ** 2; n1 = len(p) // 48
    env = db(p[:n1 * 48].reshape(n1, 48).mean(axis=1))  # 1 ms, dB
    pk = float(env[:1500].max())
    if pk < -100: return dict(silent=True)
    N = dict(key=key)
    N['peak_db'] = r1(pk)
    N['level_db'] = r1(db((m[:int(0.5 * FS)] ** 2).mean()))
    jpk = int(np.argmax(env[:1500]))
    j6 = int(np.argmax(env >= pk - 6)); j20 = int(np.argmax(env >= pk - 20)); j1 = int(np.argmax(env >= pk - 1))
    N['delay_ms'] = float(j6)
    N['attack_ms'] = float(max(j1 - j20, 0.5))
    d = np.diff(np.convolve(env[max(j20 - 2, 0):j1 + 3], np.ones(2) / 2, 'valid'))
    N['onset_sharp'] = r1(d.max() if len(d) else 0)
    e10 = env[:3500 // 10 * 10].reshape(-1, 10).mean(axis=1)  # 10 ms
    N['env'] = r1(np.maximum(e10[::5] - pk, -80))  # 50 ms steps, re the peak, 0..3.5 s
    # decay over the held note, release after the note-off
    a, b = jpk + 100, int(HOLD * 1000) - 50
    if b - a > 200:
        y = env[a:b]; ok = y > pk - 70
        if ok.sum() > 100:
            N['decay_db_s'] = r1(-np.polyfit(np.arange(len(y))[ok] / 1000, y[ok], 1)[0])
    off = int(HOLD * 1000)
    lvl_off = float(np.median(env[off - 30:off]))
    N['sustain_db'] = r1(lvl_off - pk)
    bel = np.nonzero(env[off:] < lvl_off - 30)[0]
    if lvl_off > pk - 60:
        N['release_ms'] = float(bel[0]) if len(bel) else float(len(env) - off)
    N['floor_db'] = r1(np.median(env[int((SLOT - 0.4) * 1000):int((SLOT - 0.05) * 1000)]) - pk)
    # spectra: attack (5-55 ms after the onset) and body (100-600 ms)
    s0 = max(j20 - 5, 0)
    body = m[s0 + int(0.1 * FS):s0 + int(0.6 * FS)]
    f, P = spectrum(body); Bb = erb_bands(f, P); tb = Bb.sum()
    if tb <= 0: return N
    N['erb_body'] = r1(np.maximum(db(Bb / tb), -90))
    att = m[s0 + int(0.005 * FS):s0 + int(0.055 * FS)]
    fa, Pa = spectrum(att, len(f) * 2 - 2); Ba = erb_bands(fa, Pa)
    if Ba.sum() > 0:
        N['erb_attack'] = r1(np.maximum(db(Ba / Ba.sum()), -90))
        N['att_bright'] = r1((erbn(FC) * Ba / Ba.sum()).sum() - (erbn(FC) * Bb / tb).sum(), 2)
    N['centroid_erb'] = r1((erbn(FC) * Bb / tb).sum(), 2)
    N['centroid_hz'] = r1((f * P).sum() / P.sum(), 0)
    sel = (f > 100) & (f < 8000)
    N['flatness_db'] = r1(10 * (np.log10(P[sel] + 1e-20).mean() - np.log10(P[sel].mean() + 1e-20)))
    fr, ar = spectral_peaks(f, P)
    N['dissonance'] = r1(sethares(fr, ar), 3)
    L, R = seg[s0:s0 + int(0.6 * FS), 0], seg[s0:s0 + int(0.6 * FS), 1]
    pl, pr = (L ** 2).sum(), (R ** 2).sum()
    if pl > 0 and pr > 0:
        N['ild_db'] = r1(10 * math.log10(pl / pr)); N['icc'] = r1(abs((L * R).sum()) / math.sqrt(pl * pr), 3)
    # transient coherence
    E = band_envelopes(m[max(s0 - int(0.02 * FS), 0):s0 + int(0.25 * FS)])
    if E is not None:
        Ed = db(E); en = E.max(axis=0); ok = en > en.max() * 1e-3
        if ok.sum() >= 3:
            tt = np.array([np.argmax(Ed[:, k] >= Ed[:, k].max() - 6) * 2.0 for k in range(8)])
            w = en[ok] / en[ok].sum(); tm = (tt[ok] * w).sum()
            N['trans_spread_ms'] = r1(math.sqrt((w * (tt[ok] - tm) ** 2).sum()))
            h = len(tt[ok]) // 2
            N['trans_order_ms'] = r1(tt[ok][h:].mean() - tt[ok][:h].mean())
    # coarse spectrogram: 2-ERB bands, 50 ms frames from the onset, dB re the note's peak frame
    H = int(SG_HOP * FS); fr_ = []
    for k in range(SG_N):
        z = m[s0 + k * H:s0 + (k + 1) * H]
        if len(z) < H: break
        ff, PP = spectrum(z, 4096); bb = erb_bands(ff, PP)
        fr_.append(bb[:NBANDS2 * 2].reshape(NBANDS2, 2).sum(axis=1) if len(bb) >= NBANDS2 * 2 else np.pad(bb, (0, NBANDS2 * 2 - len(bb))).reshape(NBANDS2, 2).sum(axis=1))
    G = db(np.array(fr_)); N['sgram'] = [r1(np.maximum(g - G.max(), -70)) for g in G]
    if drum: return N
    fe = 440 * 2 ** ((key - 69) / 12)
    pseg = m[s0 + int(0.15 * FS):s0 + int(0.7 * FS)]
    f0, fp, Pp = harmonic_f0(pseg, fe)
    N['f0_cents'] = r1(1200 * math.log2(f0 / fe))
    early = m[s0 + int(0.02 * FS):s0 + int(0.12 * FS)]
    fe0, _, _ = harmonic_f0(early, f0)
    N['glide_cents'] = r1(1200 * math.log2(fe0 / f0))
    lv, fq = partials(fp, Pp, f0, 16)
    if len(lv) >= 4 and lv.sum() > 0:
        tot = lv.sum(); r = lv / tot
        N['partials_db'] = r1(np.maximum(db(r), -90))
        N['tri'] = [r1(r[0], 3), r1(r[1:4].sum(), 3), r1(r[4:].sum(), 3)]
        odd, even = lv[2::2].sum(), lv[1::2].sum()
        if odd > 0 and even > 0: N['odd_even_db'] = r1(db(odd / even))
        ld = db(lv)
        if len(ld) >= 3: N['irregularity_db'] = r1(np.mean(np.abs(ld[1:-1] - (ld[:-2] + ld[1:-1] + ld[2:]) / 3)))
        N['salience_db'] = r1(db(tot / Pp[(fp > 0.8 * f0) & (fp < 16 * f0)].sum()))
        ks = np.arange(1, len(fq) + 1); good = lv > lv.max() * 1e-4
        if good.sum() >= 4:
            N['stretch_c'] = r1(np.polyfit(ks[good], 1200 * np.log2(fq[good] / (ks[good] * f0)), 1)[0], 2)
    w = int(0.08 * FS); hop = int(0.04 * FS); cs = []; envs = []
    for q in range(s0 + int(0.1 * FS), s0 + int(1.5 * FS) - w, hop):
        fv, fpp, Ppp = harmonic_f0(m[q:q + w], f0, 4)
        cs.append(1200 * math.log2(fv / f0))
        lv2, _ = partials(fpp, Ppp, f0, 6)
        if len(lv2) == 6: envs.append(db(lv2))
    if len(cs) >= 6: N['wobble_c'] = r1(np.std(cs) * 2 * math.sqrt(2))
    if len(envs) >= 6:
        C = np.corrcoef(np.array(envs).T)
        if np.all(np.isfinite(C)): N['partial_corr'] = r1(C[np.triu_indices(6, 1)].mean(), 3)
    return N


def _fit_resid(X, y):
    c, *_ = np.linalg.lstsq(X, y, rcond=None)
    return c, y - X @ c


def instrument_summary(notes, drum):
    """The instrument's curves and medians from its notes."""
    S = {}
    ns = [n for n in notes if n and not n.get('silent') and 'erb_body' in n]
    if len(ns) < 4: return S
    v = np.array([n['vel'] for n in ns], float); k = np.array([n['key'] for n in ns], float)
    lev = np.array([n['level_db'] for n in ns])
    at96 = v == 96
    S['level96_db'] = r1(np.median(lev[at96])) if at96.any() else r1(np.median(lev))
    curve = {}
    for vv in VELS:
        d = [lev[(v == vv) & (k == kk)][0] - lev[(v == 96) & (k == kk)][0] for kk in set(k)
             if ((v == vv) & (k == kk)).any() and ((v == 96) & (k == kk)).any()]
        if d: curve[str(vv)] = r1(np.median(d))
    S['vel_curve_db'] = curve
    if '40' in curve and '127' in curve: S['vel_range_db'] = r1(curve['127'] - curve['40'])
    # level over the keys and its scatter (velocity as a factor)
    kk = (k - 60) / 12
    X = np.stack([np.ones(len(ns))] + [(v == vv).astype(float) for vv in VELS[1:]] + ([kk, kk ** 2] if not drum else []), 1)
    c, res = _fit_resid(X, lev)
    if not drum: S['level_key_db_oct'] = r1(c[-2], 2)
    S['level_scatter_db'] = r1(np.std(res), 2)
    cen = np.array([n['centroid_erb'] for n in ns])
    S['centroid96_erb'] = r1(np.median(cen[at96]) if at96.any() else np.median(cen), 2)
    S['centroid96_hz'] = r1(np.median([n['centroid_hz'] for n in ns if n['vel'] == 96] or [n['centroid_hz'] for n in ns]), 0)
    c2, res2 = _fit_resid(X, cen)
    S['bright_vel_erb'] = r1(c2[len(VELS) - 1], 2)  # (127 re 40)
    if not drum: S['bright_key_erb_oct'] = r1(c2[-2], 2)
    S['bright_scatter_erb'] = r1(np.std(res2), 3)

    def med(name, sel=None, n=1):
        vals = [q[name] for q in ns if name in q and (sel is None or sel(q))]
        return r1(np.median(vals), n) if len(vals) >= 3 else None
    for name, key, n in (('delay_ms', 'delay_ms', 0), ('attack_ms', 'attack_ms', 1), ('onset_sharp', 'onset_sharp_db_ms', 1),
                         ('trans_spread_ms', 'trans_spread_ms', 1), ('trans_order_ms', 'trans_order_ms', 1),
                         ('decay_db_s', 'decay_db_s', 1), ('sustain_db', 'sustain_db', 1), ('release_ms', 'release_ms', 0),
                         ('floor_db', 'floor_db', 1), ('att_bright', 'attack_bright_erb', 2), ('flatness_db', 'flatness_db', 1),
                         ('ild_db', 'ild_db', 1), ('icc', 'icc', 3), ('glide_cents', 'glide_c', 1), ('wobble_c', 'wobble_c', 1),
                         ('odd_even_db', 'odd_even_db', 1), ('irregularity_db', 'irregularity_db', 2), ('salience_db', 'salience_db', 1),
                         ('stretch_c', 'stretch_c', 2), ('partial_corr', 'partial_corr', 3)):
        val = med(name, None, n)
        if val is not None: S[key] = val
    d = [q['dissonance'] for q in ns if q.get('dissonance', 0) > 0]
    if len(d) >= 3: S['dissonance_log2'] = r1(np.median(np.log2(d)), 2)
    f0 = [q['f0_cents'] for q in ns if 'f0_cents' in q]
    if len(f0) >= 3:
        S['f0_c'] = r1(np.median(f0)); S['f0_scatter_c'] = r1(np.median(np.abs(np.array(f0) - np.median(f0))) * 1.4826)
    tri = [q['tri'] for q in ns if 'tri' in q]
    if len(tri) >= 3:
        t = np.median(np.array(tri), axis=0); S['tri1'], S['tri2'], S['tri3'] = r1(t[0], 3), r1(t[1], 3), r1(t[2], 3)
    ild = [q['ild_db'] for q in ns if 'ild_db' in q]
    if len(ild) >= 4 and not drum:
        S['pan_key_db_oct'] = r1(np.polyfit([q['key'] for q in ns if 'ild_db' in q], ild, 1)[0] * 12, 2)
    # the mean body spectrum (formants / body resonances): power mean over the velocity-96 notes;
    # its peaks once the overall tilt is removed
    sel = [q for q in ns if q['vel'] == 96] or ns
    Bm = db(np.mean([10 ** (np.array(q['erb_body']) / 10) for q in sel], axis=0))
    S['spectrum_db'] = r1(Bm)
    S['formants'] = formant_peaks(Bm)
    S['env_db'] = r1(np.median(np.array([q['env'] for q in sel]), axis=0))
    return S


def formant_peaks(Bm, top=4):
    e = erbn(FC); ok = Bm > Bm.max() - 50
    if ok.sum() < 6: return []
    tilt = np.polyval(np.polyfit(e[ok], Bm[ok], 2), e)
    r = Bm - tilt
    idx = np.nonzero(ok)[0]; r = r[idx[0]:idx[-1] + 1]  # (only where the spectrum has something)
    pk, pr = find_peaks(r, prominence=2)
    pk = pk + idx[0]
    order = np.argsort(-pr['prominences'])[:top]
    return sorted([[r1(FC[pk[i]], 0), r1(pr['prominences'][i])] for i in order])


def make_profile(spec, programs, used, until_cache=None, jobs=4):
    """Renders each program's test notes on the renderer `spec`, returns the profile."""
    r = Renderer(spec)
    work = os.path.join(CACHE, 'diag'); os.makedirs(work, exist_ok=True)
    prof = dict(synth=r.name, spec=spec, made=time.strftime('%Y-%m-%d %H:%M'), vels=list(VELS), slot_s=SLOT, hold_s=HOLD,
                erb_fc=r1(FC, 0), instruments={})
    tasks = []
    for p in programs:
        keys = test_keys(p, used.get(p) if used else None)
        name = 'drums' if p == 'drums' else 'p%03d' % p
        mid = os.path.join(work, '%s_%s.mid' % (name, hashlib.sha1(repr(keys).encode()).hexdigest()[:8]))
        layout = write_diag(mid, p, keys)
        tasks.append((p, name, mid, layout))

    def one(t):
        p, name, mid, layout = t
        x, log = r.render(mid, 0, name)
        nl = []
        for key, v, t0 in layout:
            n = note_profile(x, t0, key, p == 'drums')
            if n is not None: n['vel'] = v; nl.append(n)
        k = r.key(open(mid, 'rb').read(), 0)
        return p, name, nl, os.path.join(CACHE, k + '.wav') if r.kind != 'wav' else os.path.join(r.arg, name + '.wav'), layout
    env_threads = os.environ.get('PJ_THREADS')
    if r.kind == 'pajupilli' and not env_threads:
        os.environ['PJ_THREADS'] = str(max(2, (os.cpu_count() or 8) // jobs))
    try:
        with ThreadPoolExecutor(max_workers=jobs) as ex:
            for p, name, nl, wav, layout in ex.map(one, tasks):
                gm = 'drums' if p == 'drums' else (GM[p] if p < len(GM) else 'program %d' % p)
                prof['instruments'][name] = dict(program=p, gm=gm, wav=wav, layout=layout, notes=nl,
                                                 summary=instrument_summary(nl, p == 'drums'))
                print('  %s: %s, %d notes' % (r.short, gm, len(nl)), file=sys.stderr)
    finally:
        if r.kind == 'pajupilli' and not env_threads: os.environ.pop('PJ_THREADS', None)
    return prof


# -- comparing profiles

# measure: (label, unit, JND, weight, words for A above B, A below B)
SUMMARY_MEASURES = {
    'level_rel_db': ('level against the other instruments (velocity 96)', 'dB', 1.0, 1.4, 'louder', 'quieter'),
    'vel_range_db': ('loudness range, velocity 40 to 127', 'dB', 1.5, 1.2, 'a wider dynamic range', 'a narrower dynamic range'),
    'level_key_db_oct': ('level over the keyboard', 'dB/oct', 1.0, 0.7, 'louder toward the top', 'louder toward the bottom'),
    'level_scatter_db': ('key-to-key level scatter', 'dB', 0.5, 0.9, 'less even from key to key', 'more even from key to key'),
    'centroid96_erb': ('brightness (spectral centroid, velocity 96)', 'ERB', 0.3, 1.2, 'brighter', 'darker'),
    'bright_vel_erb': ('brightening from velocity 40 to 127', 'ERB', 0.3, 0.9, 'brightens more with velocity', 'brightens less with velocity'),
    'bright_key_erb_oct': ('brightness over the keyboard', 'ERB/oct', 0.3, 0.5, 'brightens faster up the keyboard', 'brightens slower up the keyboard'),
    'bright_scatter_erb': ('key-to-key brightness scatter', 'ERB', 0.2, 0.7, 'less even in tone from key to key', 'more even in tone from key to key'),
    'attack_bright_erb': ('attack brighter than the body by', 'ERB', 0.4, 0.6, 'a brighter attack', 'a duller attack'),
    'delay_ms': ('onset delay', 'ms', 10, 0.7, 'speaks later', 'speaks sooner'),
    'attack_ms': ('attack time (-20 to -1 dB)', 'ms', None, 0.9, 'a slower attack', 'a faster attack'),
    'onset_sharp_db_ms': ('onset sharpness', 'dB/ms', 2, 0.4, 'a sharper onset', 'a softer onset'),
    'trans_spread_ms': ('transient coherence: attack spread over bands', 'ms', 5, 0.6, 'a less coherent attack', 'a more coherent attack'),
    'trans_order_ms': ('high bands lag low bands by', 'ms', 5, 0.3, 'its highs start later', 'its highs start sooner'),
    'decay_db_s': ('decay rate of a held note', 'dB/s', None, 0.9, 'decays faster', 'decays slower'),
    'sustain_db': ('level after 2 s held, re peak', 'dB', 3, 0.8, 'sustains more', 'sustains less'),
    'release_ms': ('release time (30 dB after note-off)', 'ms', None, 0.8, 'rings longer after the key', 'stops sooner after the key'),
    'floor_db': ('noise / room floor 3.6-4 s after on, re peak', 'dB', 6, 0.3, 'a higher floor', 'a lower floor'),
    'flatness_db': ('noisiness (spectral flatness)', 'dB', 2, 0.5, 'noisier', 'purer'),
    'f0_c': ('pitch re 12-TET A440 (median)', 'c', 4, 0.8, 'sharper', 'flatter'),
    'f0_scatter_c': ('pitch scatter from key to key', 'c', 3, 0.8, 'less evenly tuned', 'more evenly tuned'),
    'glide_c': ('pitch at the start re settled', 'c', 5, 0.5, 'starts sharper', 'starts flatter'),
    'wobble_c': ('pitch wobble / vibrato (p-p)', 'c', 5, 0.6, 'wobbles more', 'wobbles less'),
    'tri1': ('fundamental share (tristimulus 1)', '', 0.07, 0.6, 'more fundamental', 'less fundamental'),
    'tri2': ('partials 2-4 share (tristimulus 2)', '', 0.07, 0.5, 'stronger low partials', 'weaker low partials'),
    'tri3': ('partials 5+ share (tristimulus 3)', '', 0.07, 0.6, 'stronger high partials', 'weaker high partials'),
    'odd_even_db': ('odd / even partials', 'dB', 2, 0.5, 'more odd partials (hollower)', 'more even partials'),
    'irregularity_db': ('partial-to-partial jaggedness', 'dB', 1, 0.4, 'a more jagged overtone series', 'a smoother overtone series'),
    'stretch_c': ('partial stretch (inharmonicity)', 'c/partial', 0.5, 0.5, 'more stretched partials', 'less stretched partials'),
    'partial_corr': ('partial envelopes alike', '', 0.1, 0.4, 'partials moving more as one', 'partials living more their own lives'),
    'dissonance_log2': ('Plomp-Levelt dissonance of a note', 'log2', 0.3, 0.6, 'rougher', 'smoother'),
    'icc': ('stereo coherence', '', 0.075, 0.3, 'narrower', 'wider'),
    'pan_key_db_oct': ('panning across the keys', 'dB/oct', 1, 0.2, 'more spread by key', 'less spread by key'),
}
LOGRATIO = {'attack_ms': 0.25, 'decay_db_s': 0.25, 'release_ms': 0.25}  # (compared as log2 ratios; the JND in octaves)


def _env_dist(a, b):
    a, b = np.maximum(np.array(a), -60), np.maximum(np.array(b), -60)
    n = min(len(a), len(b)); a, b = a[:n], b[:n]
    ok = (a > -59) | (b > -59)
    return float(np.sqrt(((a - b)[ok] ** 2).mean())) if ok.any() else 0.0


def _shape_dist(a, b):
    a, b = np.array(a), np.array(b); ok = (a > -60) | (b > -60)
    if ok.sum() < 3: return 0.0
    d = np.maximum(a, -60)[ok] - np.maximum(b, -60)[ok]
    return float(np.sqrt(((d - d.mean()) ** 2).mean()))


def _sg_dist(a, b):
    a, b = np.array(a), np.array(b); n = min(len(a), len(b))
    if n < 3: return 0.0
    a, b = a[:n].ravel(), b[:n].ravel(); ok = (a > -69) | (b > -69)
    if ok.sum() < 10: return 0.0
    c = np.corrcoef(a[ok], b[ok])[0, 1]
    return float(1 - c) if np.isfinite(c) else 1.0


PAIR_MEASURES = {  # per-note distances: (label, function on two notes, weight)
    'spectrum': ('spectral shape of the body, note for note', lambda p, q: _shape_dist(p['erb_body'], q['erb_body']), 1.0),
    'attack_spectrum': ('spectral shape of the attack, note for note', lambda p, q: _shape_dist(p['erb_attack'], q['erb_attack']) if 'erb_attack' in p and 'erb_attack' in q else None, 0.6),
    'envelope': ('envelope curve, note for note (dB rms)', lambda p, q: _env_dist(p['env'], q['env']), 0.9),
    'sgram': ('spectrogram (1 - correlation), note for note', lambda p, q: _sg_dist(p['sgram'], q['sgram']), 0.9),
    'overtones': ('overtone levels, note for note (dB rms)', lambda p, q: _env_dist(p['partials_db'][:8], q['partials_db'][:8]) if 'partials_db' in p and 'partials_db' in q else None, 0.6),
}


def _match(pa, pb):
    ia = {(n['key'], n['vel']): n for n in pa['notes'] if 'erb_body' in n}
    ib = {(n['key'], n['vel']): n for n in pb['notes'] if 'erb_body' in n}
    return [(ia[k], ib[k]) for k in sorted(set(ia) & set(ib))]


def _pair_median(pa, pb, f):
    v = [f(a, b) for a, b in _match(pa, pb)]
    v = [x for x in v if x is not None]
    return float(np.median(v)) if v else None


def _zimt_notes(pa, pb, Z, vel=96):
    """Median Zimtohrli distance of the velocity-`vel` notes, each note to the same loudness."""
    if Z is None or not (os.path.exists(pa['wav']) and os.path.exists(pb['wav'])): return None
    xa, xb = load_wav(pa['wav']), load_wav(pb['wav'])
    la = {(k, v): t for k, v, t in pa['layout']}; lb = {(k, v): t for k, v, t in pb['layout']}
    d = []
    for kv in sorted(set(la) & set(lb)):
        if kv[1] != vel: continue
        sa = xa[int(la[kv] * FS):int((la[kv] + 3) * FS)].mean(axis=1); sb = xb[int(lb[kv] * FS):int((lb[kv] + 3) * FS)].mean(axis=1)
        if len(sa) < FS or len(sb) < FS: continue
        ra, rb = np.sqrt((sa ** 2).mean()), np.sqrt((sb ** 2).mean())
        if ra <= 0 or rb <= 0: continue
        d.append(Z.distance((sa / ra * 0.1).astype(np.float32), (sb / rb * 0.1).astype(np.float32)))
    return float(np.median(d)) if d else None


def compare_profiles(profs, out, top_inst=6, top_all=10, use_zimt=True):
    """profs[0] is A, profs[1] B, the rest references (B is a reference too)."""
    A, B = profs[0], profs[1]; refs = profs[1:]
    names = [p['synth'] for p in profs]
    common = [k for k in A['instruments'] if all(k in p['instruments'] for p in profs[1:])]
    if not common: raise SystemExit('no instrument in common')
    # The set's level: each profile's median velocity-96 level over the common instruments.
    base = [np.median([p['instruments'][k]['summary'].get('level96_db', -99) for k in common]) for p in profs]
    Z = zimt() if use_zimt else None
    items = []; per = {}
    for k in common:
        I = [p['instruments'][k] for p in profs]; S = [i['summary'] for i in I]
        for s, b0 in zip(S, base): s['level_rel_db'] = r1(s.get('level96_db', -99) - b0)
        gm = I[0]['gm']; its = []
        for m, (label, unit, jnd, wt, up, down) in SUMMARY_MEASURES.items():
            if m not in S[0] or m not in S[1]: continue
            vals = [s.get(m) for s in S]
            if m in LOGRATIO:
                if any(v is None or v <= 0 for v in vals[:2]): continue
                tr = lambda v: math.log2(v) if v and v > 0 else None
                jnd = LOGRATIO[m]
            else:
                tr = lambda v: v
            ta, tb = tr(vals[0]), tr(vals[1]); rv = [tr(v) for v in vals[1:] if v is not None and tr(v) is not None]
            spread = (max(rv) - min(rv)) / 2 if len(rv) >= 2 else 0.0
            d = ta - tb; z = abs(d) / max(jnd, spread)
            outside = len(rv) >= 2 and (ta < min(rv) - jnd / 2 or ta > max(rv) + jnd / 2)
            if m in LOGRATIO: how = '%.1f x' % (2 ** abs(d))
            else: how = '%+.3g %s' % (d, unit)
            text = '%s %s (%s: %s, %s: %s%s)' % (names[0], up if d > 0 else down, names[0], _fmt(vals[0], unit), names[1], _fmt(vals[1], unit),
                                                ''.join(', %s: %s' % (names[i + 2], _fmt(v, unit)) for i, v in enumerate(vals[2:])))
            its.append(dict(inst=gm, measure=m, label=label, a=vals[0], b=vals[1], refs=vals[2:], diff=how, z=z, outside=outside,
                            text=text, score=min(z, 8) * wt * (1 if outside else 0.5)))
        for m, (label, f, wt) in PAIR_MEASURES.items():
            dab = _pair_median(I[0], I[1], f)
            if dab is None: continue
            dar = [x for x in (_pair_median(I[0], i, f) for i in I[1:]) if x is not None]
            drr = [x for x in (_pair_median(I[i], I[j], f) for i in range(1, len(I)) for j in range(i + 1, len(I))) if x is not None]
            yard = float(np.median(drr)) if drr else None
            z = dab / yard if yard else dab
            outside = bool(yard) and np.median(dar) > 1.3 * yard
            text = '%s: %s to %s %.3g; to the references %.3g; the references among themselves %s' % (
                label, names[0], names[1], dab, np.median(dar), '%.3g' % yard if yard else '-')
            its.append(dict(inst=gm, measure=m, label=label, a=dab, b=yard, refs=dar, diff='%.3g' % dab, z=z, outside=outside,
                            text=text, score=min(z, 8) * wt * 0.6 * (1 if outside else 0.5)))
        if Z is not None:
            dab = _zimt_notes(I[0], I[1], Z)
            if dab is not None:
                dar = [x for x in (_zimt_notes(I[0], i, Z) for i in I[1:]) if x is not None]
                drr = [x for x in (_zimt_notes(I[i], I[j], Z) for i in range(1, len(I)) for j in range(i + 1, len(I))) if x is not None]
                yard = float(np.median(drr)) if drr else None
                z = dab / yard if yard else 1.0
                outside = bool(yard) and np.median(dar) > 1.3 * yard
                its.append(dict(inst=gm, measure='zimtohrli', label='Zimtohrli per note (velocity 96, loudness matched)', a=dab, b=yard,
                                refs=dar, diff='%.4f' % dab, z=z, outside=outside,
                                text='Zimtohrli per note: %s to %s %.4f; to the references %.4f; the references among themselves %s' % (
                                    names[0], names[1], dab, np.median(dar), '%.4f' % yard if yard else '-'),
                                score=min(z, 8) * 0.4 * (1 if outside else 0.5)))
        # formants and the velocity curve, as text
        extra = []
        fa, fb = S[0].get('formants', []), S[1].get('formants', [])
        extra.append('resonance peaks (Hz, prominence dB): %s %s; %s %s' % (
            names[0], ', '.join('%.0f (%.0f)' % tuple(x) for x in fa) or '-', names[1], ', '.join('%.0f (%.0f)' % tuple(x) for x in fb) or '-'))
        extra.append('velocity curve (dB re velocity 96, at %s): %s' % (
            ', '.join(str(v) for v in VELS), '; '.join('%s %s' % (n, ' '.join('%+.1f' % s.get('vel_curve_db', {}).get(str(v), float('nan')) for v in VELS))
                                                    for n, s in zip(names, S))))
        if 'spectrum_db' in S[0] and 'spectrum_db' in S[1]:
            d = np.array(S[0]['spectrum_db']) - np.array(S[1]['spectrum_db'])
            ok = (np.array(S[0]['spectrum_db']) > -50) | (np.array(S[1]['spectrum_db']) > -50)
            dd = np.where(ok, d - d[ok].mean(), 0)
            regions = []
            for lo, hi in ((40, 150), (150, 400), (400, 1000), (1000, 2500), (2500, 6000), (6000, 16000)):
                sel = ok & (FC >= lo) & (FC < hi)
                if sel.any(): regions.append('%s %+.0f' % ('%g-%g kHz' % (lo / 1000, hi / 1000) if lo >= 1000 else '%d-%d Hz' % (lo, hi), dd[sel].mean()))
            extra.append('mean spectrum, %s re %s (dB, level removed): %s' % (names[0], names[1], ', '.join(regions)))
        its.sort(key=lambda i: -i['score'])
        per[gm] = dict(items=its, extra=extra, summaries={n: s for n, s in zip(names, S)})
        items += its
    items.sort(key=lambda i: -i['score'])
    L = ['# %s against %s, note for note' % (names[0], names[1]), '']
    L.append('References (the yardstick, %s among them): %s. Each instrument plays keys a few semitones apart at velocities '
             '%s, held %g s. A difference counts in units of the larger of its JND and half the references\' range; '
             '"outside" means %s lies outside that range (else the SoundFonts disagree as much among themselves).'
             % (names[1], ', '.join(names[1:]), ', '.join(map(str, VELS)), HOLD, names[0]))
    L += ['', '## The %d biggest differences over the whole set' % top_all, '']
    for n, it in enumerate(items[:top_all]):
        L.append('%d. **%s, %s**: %s; %.1f x the yardstick%s.' % (n + 1, it['inst'], it['label'], it['text'], it['z'],
                                                                   ', outside the references' if it['outside'] else ''))
    for gm, P in per.items():
        L += ['', '## ' + gm, '']
        for n, it in enumerate(P['items'][:top_inst]):
            L.append('%d. **%s**: %s; %.1f x%s.' % (n + 1, it['label'], it['text'], it['z'], ', outside' if it['outside'] else ''))
        L.append('')
        for e in P['extra']: L.append('- ' + e)
        L += ['', '<details><summary>every measure</summary>', '', '| measure | ' + ' | '.join(names) + ' | x yardstick |', '|---|' + '---|' * (len(names) + 1)]
        for it in sorted(P['items'], key=lambda i: i['measure']):
            if it['measure'] in SUMMARY_MEASURES:
                L.append('| %s | %s | %.1f |' % (it['label'], ' | '.join(_fmt(v, '') for v in [it['a'], it['b']] + list(it['refs'])), it['z']))
        L += ['', '</details>']
    text = '\n'.join(L) + '\n'
    os.makedirs(out, exist_ok=True)
    open(os.path.join(out, 'notes_report.md'), 'w').write(text)
    json.dump(dict(synths=names, items=items, per={k: dict(items=v['items'], extra=v['extra']) for k, v in per.items()}),
              open(os.path.join(out, 'notes_report.json'), 'w'), indent=1, default=lambda o: o.tolist() if isinstance(o, np.ndarray) else str(o))
    return text


def _fmt(v, unit):
    if v is None: return '-'
    if isinstance(v, (int, float)):
        s = ('%.3g' % v) if abs(v) < 10 else '%.0f' % v
        return s + (' ' + unit if unit else '')
    return str(v)


def programs_of(midi):
    """The song's instruments: program -> the keys it plays ('drums' for channel 10)."""
    fmt, div, tracks = read_smf(midi)
    used = {}; cur = {}
    evs = sorted((t, s, d) for tr in tracks for t, k, s, d in tr if k == 'ch')
    for t, s, d in evs:
        ch, hi = s & 15, s & 0xF0
        if hi == 0xC0: cur[ch] = d[0]
        elif hi == 0x90 and d[1] > 0:
            p = 'drums' if ch == 9 else cur.get(ch, 0)
            used.setdefault(p, set()).add(d[0])
    return used


def load_or_make_profile(spec, programs, used, jobs):
    r = Renderer(spec)
    h = hashlib.sha1(json.dumps([spec, [str(p) for p in programs], {str(k): sorted(v) for k, v in (used or {}).items() if k in programs},
                                 VELS, SLOT, HOLD, r.key(b'', 0)]).encode()).hexdigest()[:16]
    path = os.path.join(CACHE, 'profile_%s.json' % h)
    if os.path.exists(path):
        return json.load(open(path)), path
    prof = make_profile(spec, programs, used, jobs=jobs)
    json.dump(prof, open(path, 'w'))
    return prof, path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('args', nargs='*', help='song.mid | profile SYNTH | compare A.json B.json [REF.json ...]')
    ap.add_argument('--a', default='pajupilli')
    ap.add_argument('--b', default='timidity:' + SF_DEFAULT_B)
    ap.add_argument('--ref', default=','.join('timidity:' + s for s in SF_REFS if os.path.exists(s)),
                    help='third opinions, comma-separated (B is always one); empty: none')
    ap.add_argument('--programs', default='', help='GM programs (0-127, "drums"), comma-separated; default: the song\'s')
    ap.add_argument('--song', action='store_true', help='compare the song itself, stem by stem (the older mode)')
    ap.add_argument('--until', type=float, default=60.0, help='--song: seconds of the piece (0: all)')
    ap.add_argument('--channels', default='', help='--song: e.g. 1,2,10 (1-based)')
    ap.add_argument('--out', default='', help='directory for the reports (or the profile file)')
    ap.add_argument('--top', type=int, default=25)
    ap.add_argument('--jobs', type=int, default=4, help='renders side by side')
    ap.add_argument('--no-zimt', action='store_true')
    a = ap.parse_args()
    if not a.args: ap.error('nothing to do')
    if a.args[0] == 'compare':
        profs = [json.load(open(p)) for p in a.args[1:]]
        if len(profs) < 2: ap.error('compare needs two profiles')
        print(compare_profiles(profs, a.out or '.', use_zimt=not a.no_zimt))
        return
    progs = [p if p == 'drums' else int(p) for p in a.programs.split(',') if p] if a.programs else None
    if a.args[0] == 'profile':
        if len(a.args) < 2 or not progs: ap.error('profile SYNTH --programs ...')
        prof = make_profile(a.args[1], progs, None, jobs=a.jobs)
        path = a.out or 'profile.json'
        json.dump(prof, open(path, 'w')); print('wrote ' + path, file=sys.stderr)
        return
    a.midi = a.args[0]
    if a.song:
        main_song(a); return
    used = programs_of(a.midi)
    progs = progs or sorted(used, key=lambda p: (p == 'drums', p if p != 'drums' else 0))
    specs = [a.a, a.b] + [s for s in a.ref.split(',') if s and s != a.b]
    print('%s: instruments %s; %s' % (os.path.basename(a.midi), ', '.join('drums' if p == 'drums' else GM[p] for p in progs),
                                      ', '.join(Renderer(s).name for s in specs)), file=sys.stderr)
    profs = []
    for s in specs:
        t = time.time(); prof, path = load_or_make_profile(s, progs, used, a.jobs); profs.append(prof)
        print('  profile %s: %s (%.0f s)' % (prof['synth'], path, time.time() - t), file=sys.stderr)
    out = a.out or os.path.join('/tmp', 'synthcompare_' + os.path.splitext(os.path.basename(a.midi))[0])
    print(compare_profiles(profs, out, top_all=10, use_zimt=not a.no_zimt))
    print('wrote %s/notes_report.md and notes_report.json' % out, file=sys.stderr)


if __name__ == '__main__':
    main()
