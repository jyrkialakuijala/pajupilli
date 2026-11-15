#!/usr/bin/env python3
"""Measuring a piano from recordings of music (research/piano.md, section 11).

No score and no samples: the tool finds notes that begin alone (or in pairs) in a solo piano
recording and measures their partials; only the numbers go into the model (src/piano.cc,
tools/pianoboard.py).

  - onsets: where the spectrum rises (10 ms frames, 60 Hz - 8 kHz);
  - at an onset followed by at least 0.3 s without another: what the spectrum gained, its
    peaks matched to a stiff string's series n f0 sqrt(1 + B n^2) for the key that explains
    most of the gain (the low partials in a row; not an octave or a twelfth too low);
  - per partial: frequency, level in the first 0.2-0.4 s, and decay (dB/s) over the clear
    time, of partials that stand clear of what was sounding before;
  - per register of six keys: the stiffness B, the tuning against equal temperament, how far
    partials 1-3 sit from the series, the spectrum's shares by band for the quiet, middle
    and loud thirds of the notes, and the decay by band (median and quartiles).

    tools/pianofit.py recording.wav [more.wav ...]     the tables, one column set per file

The model's own notes are measured with the same functions (measure_file: single notes of
known key at known times), over the same clear time as the recordings give (about 0.55 s).
"""
import sys

import numpy as np

NFFT = 1 << 16


def onsets(x, fs):
    n, hop = 2048, int(fs * 0.01)
    w = np.hanning(n)
    nf = (len(x) - n) // hop
    fr = np.fft.rfftfreq(n, 1 / fs)
    band = (fr > 60) & (fr < 8000)
    L = np.empty((nf, band.sum()), np.float32)
    for i in range(nf):
        L[i] = np.log(np.abs(np.fft.rfft(x[i * hop:i * hop + n] * w))[band] + 1e-4)
    d = np.maximum(L[2:] - L[:-2], 0).sum(1)
    d = np.concatenate([[0, 0], d])
    # a peak that stands over the half second around it
    k = 25
    out = []
    for i in range(k, nf - k):
        seg = d[i - k:i + k + 1]
        if d[i] == seg.max() and d[i] > 1.6 * np.median(seg) + 0.008 * band.sum():
            if not out or i - out[-1] > 4:
                out.append(i)
    # the window's centre is where the rise is
    return np.array(out) * hop / fs + n / 2 / fs - 0.01, d


def spectrum(x, fs, t0, dur):
    i0 = int(t0 * fs)
    n = int(dur * fs)
    seg = x[max(i0, 0):i0 + n]
    w = np.hanning(len(seg))
    return np.abs(np.fft.rfft(seg * w, NFFT)) * 2 / w.sum()


def peaks(sp, fs, dur, floor_db=-55):
    """Local maxima (frequency, amplitude), a main lobe apart."""
    half = max(2, int(1.3 / dur * NFFT / fs))
    thr = sp.max() * 10 ** (floor_db / 20)
    idx = np.flatnonzero((sp[1:-1] > sp[:-2]) & (sp[1:-1] >= sp[2:]) & (sp[1:-1] > thr)) + 1
    idx = idx[idx > half]
    keep = []
    for i in idx[np.argsort(-sp[idx])]:
        if all(abs(i - j) > half for j in keep):
            keep.append(i)
    keep = np.array(sorted(keep), int)
    if len(keep) == 0:
        return np.zeros(0), np.zeros(0)
    a, b, c = np.log(sp[keep - 1] + 1e-12), np.log(sp[keep] + 1e-12), np.log(sp[keep + 1] + 1e-12)
    off = 0.5 * (a - c) / (a - 2 * b + c - 1e-12)
    return (keep + off) * fs / NFFT, sp[keep]


def prior_b(key):
    """A grand's inharmonicity, roughly: wound bass about 1e-4, rising 2x per 8 keys above the break."""
    k = key - 20
    return 1.2e-4 * 2 ** (max(k - 22, 0) / 8.5) if k > 22 else 1.2e-4 * (1 + (22 - k) / 40)


def fit_series(pf, pa, f0, B, fmax):
    """Match peaks to n f0 sqrt(1 + B n^2), widening from the low partials; refit f0 and B."""
    nmax = int(min(48, fmax / f0))
    match = {}
    for stage in (4, 8, 14, 24, 48):
        match = {}
        for n in range(1, min(stage, nmax) + 1):
            f = n * f0 * np.sqrt(1 + B * n * n)
            if f > fmax:
                break
            tol = min(0.30 * f0, 0.018 * f + 2.0)
            j = np.argmin(np.abs(pf - f))
            if abs(pf[j] - f) < tol:
                match[n] = j
        if len(match) >= 3:
            ns = np.array(sorted(match))
            y = (pf[[match[n] for n in ns]] / ns) ** 2
            w = pa[[match[n] for n in ns]] ** 0.5
            A = np.stack([np.ones(len(ns)), ns ** 2.0], 1)
            sol = np.linalg.lstsq(A * w[:, None], y * w, rcond=None)[0]
            if sol[0] > 0:
                f0n, Bn = np.sqrt(sol[0]), max(sol[1] / sol[0], 0.0)
                if abs(1200 * np.log2(f0n / f0)) < 40 and Bn < 20 * B + 1e-3:
                    f0, B = f0n, Bn
        if stage >= nmax:
            break
    return f0, B, match


def note_at(x, fs, t, gap, two=True):
    dur = float(np.clip(gap - 0.05, 0.18, 0.37))
    post = spectrum(x, fs, t + 0.025, dur)
    pre = spectrum(x, fs, t - 0.015 - dur, dur)
    d = np.sqrt(np.maximum(post ** 2 - 2.0 * pre ** 2, 0))
    lo = int(25 * NFFT / fs)
    d[:lo] = 0
    pf, pa = peaks(d, fs, dur)
    if len(pf) < 2:
        return None
    total = (pa ** 2).sum()
    fmax = 0.45 * fs
    found = []
    left = np.ones(len(pf), bool)
    for rnd in range(2 if two else 1):
        if left.sum() < 2:
            break
        qf, qa = pf[left], pa[left]
        strong = np.flatnonzero(qa > qa.max() * 10 ** (-30 / 20))[:6]
        best = None
        tried = set()
        for j in strong:
            for n0 in (1, 2, 3, 4, 5, 6):
                key = int(round(69 + 12 * np.log2(qf[j] / n0 / 440)))
                if key < 21 or key > 108 or key in tried:
                    continue
                tried.add(key)
                f0 = 440 * 2 ** ((key - 69) / 12)
                if 1 / dur * 1.6 > f0 * 0.5:      # the window cannot part these partials
                    continue
                f0, B, m = fit_series(qf, qa, f0, prior_b(key), fmax)
                if len(m) < (2 if key > 88 else 3 if key > 76 else 4):
                    continue
                e = sum(qa[j2] ** 2 for j2 in m.values())
                odd = sum(qa[j2] ** 2 for n, j2 in m.items() if n % 2)
                third = sum(qa[j2] ** 2 for n, j2 in m.items() if n % 3)
                if odd < 0.03 * e or third < 0.03 * e:   # an octave or a twelfth too low
                    continue
                # a real note has its low partials in a row (the bass may lack the first two)
                first = 3 if key < 40 else 1
                want = [n for n in range(first, first + 8) if n * f0 < fmax and n * f0 < qf[strong].max() * 1.5 + f0]
                got = sum(n in m for n in want)
                if len(want) >= 3 and got < 0.75 * len(want):
                    continue
                e *= got / max(len(want), 1)
                if best is None or e > best[0]:
                    best = (e, key, f0, B, m)
        if best is None:
            break
        e, key, f0, B, m = best
        e = sum(qa[j2] ** 2 for j2 in m.values())
        found.append(dict(key=key, f0=f0, B=B, share=e / total, partials=sorted(m)))
        idx = np.flatnonzero(left)
        for j2 in m.values():
            left[idx[j2]] = False
        if e / total > 0.8:
            break
    if not found or sum(n['share'] for n in found) < 0.8:
        return None
    return found


def track(x, fs, t0, t1, freqs, win=0.060, hop=0.020):
    """Amplitude of each frequency over [t0, t1): one windowed Fourier bin per frame."""
    n = int(win * fs)
    h = int(hop * fs)
    i0, i1 = int(t0 * fs), int(t1 * fs) - n
    starts = np.arange(i0, i1, h)
    if len(starts) == 0:
        return np.zeros(0), np.zeros((0, len(freqs)))
    w = np.hanning(n)
    tt = np.arange(n) / fs
    E = np.exp(-2j * np.pi * np.outer(freqs, tt)) * w * 2 / w.sum()
    out = np.empty((len(starts), len(freqs)))
    for k, s in enumerate(starts):
        out[k] = np.abs(E @ x[s:s + n])
    return (starts - i0) / fs + win / 2, out


def measure(x, fs, t, gap, f0, B, others=()):
    """Partials of one note: n, frequency, level at the start (dB), decay (dB/s) over the clear time."""
    dur = float(np.clip(gap - 0.05, 0.18, 0.37))
    post = spectrum(x, fs, t + 0.025, dur)
    pre = spectrum(x, fs, t - 0.015 - dur, dur)
    fmax = 0.45 * fs
    res = []
    ns, fs_ = [], []
    for n in range(1, 49):
        f = n * f0 * np.sqrt(1 + B * n * n)
        if f > fmax:
            break
        if any(abs(f / g - 1) < 0.025 for g in others):
            continue
        b = int(round(f * NFFT / fs))
        r = max(2, int(0.012 * f * NFFT / fs))
        j = b - r + int(np.argmax(post[b - r:b + r + 1]))
        if j <= b - r or j >= b + r:
            continue
        a, c, e = np.log(post[j - 1]), np.log(post[j]), np.log(post[j + 1])
        fpk = (j + 0.5 * (a - e) / (a - 2 * c + e - 1e-12)) * fs / NFFT
        if post[j] < 3.0 * pre[max(j - r, 0):j + r + 1].max():   # not clear of what was sounding
            continue
        ns.append(n)
        fs_.append(fpk)
        res.append([n, fpk, 20 * np.log10(post[j])])
    if not res:
        return np.zeros((0, 5)), None
    res = np.array(res)
    # decay: frames long enough to part the partials
    win = float(np.clip(4.0 / f0, 0.04, 0.12))
    tt, env = track(x, fs, t + 0.03, t + gap - 0.03, np.array(fs_), win=win, hop=0.02)
    slope = np.full(len(res), np.nan)
    if len(tt) >= 10:
        db = 20 * np.log10(env + 1e-9)
        noise = 20 * np.log10(np.abs(x[int((t + 0.03) * fs):int((t + gap) * fs)]).max() + 1e-9) - 75
        for k in range(len(res)):
            ok = db[:, k] > max(noise, db[:6, k].max() - 40)
            m = int(np.argmin(ok)) if not ok.all() else len(tt)
            if m >= 10 and tt[m - 1] - tt[0] >= 0.2:
                slope[k] = np.polyfit(tt[:m], db[:m, k], 1)[0]
    return np.column_stack([res, slope, np.full(len(res), gap)]), (tt, env)


# The tables.

def good(n):
    k = n['key']; need = 8 if k < 52 else 6 if k < 64 else 4 if k < 76 else 3
    return len(n['tab']) >= need and n['share'] >= 0.6
BANDS = [(0, 250), (250, 500), (500, 1000), (1000, 2000), (2000, 4000), (4000, 8000), (8000, 20000)]
def level(n):
    return 10 * np.log10((10 ** (n['tab'][:, 2] / 10)).sum())
def shares(n):
    t = n['tab']; p = 10 ** (t[:, 2] / 10); tot = p.sum()
    return np.array([10 * np.log10(p[(t[:, 1] >= lo) & (t[:, 1] < hi)].sum() / tot + 1e-9) for lo, hi in BANDS])
def resid(n):
    t = n['tab']; out = {}
    for row in t:
        k = int(row[0])
        if k <= 3: out[k] = 1200 * np.log2(row[1] / (k * n['f0'] * np.sqrt(1 + n['B'] * k * k)))
    return out
def table(notes, step=6):
    res = {}
    for k0 in range(21, 109, step):
        ns = [n for n in notes if k0 <= n['key'] < k0 + step]
        if len(ns) < 3: continue
        r = dict(n=len(ns))
        r['B'] = np.median([n['B'] for n in ns]); r['dev'] = np.median([1200 * np.log2(n['f0'] / (440 * 2 ** ((n['key'] - 69) / 12))) for n in ns])
        rs = [resid(n) for n in ns]
        r['res'] = [np.median(np.abs([x[k] for x in rs if k in x])) if sum(k in x for x in rs) >= 3 else np.nan for k in (1, 2, 3)]
        L = np.array([level(n) for n in ns]); S = np.array([shares(n) for n in ns]); o = np.argsort(L); m = max(len(ns) // 3, 1)
        r['L'] = [np.median(L[o[:m]]), np.median(L[o[m:len(ns) - m]]) if len(ns) > 2 * m else np.median(L), np.median(L[o[-m:]])]
        r['S'] = [np.median(S[o[:m]], 0), np.median(S[o[m:len(ns) - m]], 0) if len(ns) > 2 * m else np.median(S, 0), np.median(S[o[-m:]], 0)]
        d = []
        for lo, hi in BANDS[:6]:
            s = np.concatenate([n['tab'][(n['tab'][:, 1] >= lo) & (n['tab'][:, 1] < hi) & np.isfinite(n['tab'][:, 3]), 3] for n in ns if n['gap'] >= 0.45] + [np.zeros(0)])
            d.append((np.percentile(s, 25), np.median(s), np.percentile(s, 75), len(s)) if len(s) >= 5 else None)
        r['D'] = d
        res[k0] = r
    return res
def show(name, r):
    print(f"  {name:9s} n {r['n']:3d} B {r['B']:.2e} dev {r['dev']:+5.1f}c  off-series 1,2,3: " + ' '.join(f'{x:4.1f}' for x in r['res']) + 'c')
    for i, tag in enumerate(('quiet', 'mid', 'loud')):
        print(f"      {tag:5s} {r['L'][i]:6.1f} dB  shares: " + ' '.join(f'{x:6.1f}' if x > -80 else '     -' for x in r['S'][i]))
    print('      decay dB/s (quartiles/n): ' + '  '.join(f'{d[1]:5.1f} [{d[0]:5.1f},{d[2]:5.1f}]/{d[3]}' if d else '      -      ' for d in r['D']))


def load(path):
    from scipy.io import wavfile
    fs, d = wavfile.read(path)
    d = d.astype(np.float64) / (32768.0 if d.dtype == np.int16 else 1.0)
    return fs, (d.mean(1) if d.ndim > 1 else d)


def find_notes(x, fs):
    """Every well-measured note of a recording: dicts of key, f0, B, gap, tab (per partial:
    n, frequency, level dB, decay dB/s, gap)."""
    on, _ = onsets(x, fs)
    gaps = np.diff(np.append(on, len(x) / fs))
    notes = []
    for t, g in zip(on, gaps):
        if g < 0.3:
            continue
        g = min(g, 6.0)
        f = note_at(x, fs, t, g)
        if f is None:
            continue
        for i, n in enumerate(f):
            others = []
            for j, o in enumerate(f):
                if j != i:
                    others += [k * o['f0'] * np.sqrt(1 + o['B'] * k * k) for k in range(1, 49)]
            tab, _ = measure(x, fs, t, g, n['f0'], n['B'], others)
            if len(tab) >= 2:
                notes.append(dict(t=t, gap=g, key=n['key'], f0=n['f0'], B=n['B'], share=n['share'], tab=tab))
    return [n for n in notes if good(n)]


def measure_file(x, fs, key, t, gap=0.55):
    """One note of known key (MIDI) starting at t in x: the same measures (for the model's notes)."""
    dur = float(np.clip(gap - 0.05, 0.18, 0.37))
    sp = spectrum(x, fs, t + 0.025, dur)
    pfq, pa = peaks(sp, fs, dur)
    f0, B, _ = fit_series(pfq, pa, 440 * 2 ** ((key - 69) / 12), prior_b(key), 0.45 * fs)
    tab, _ = measure(x, fs, t, gap, f0, B)
    return dict(t=t, gap=gap, key=key, f0=f0, B=B, share=1.0, tab=tab)


def main():
    sets = []
    for path in sys.argv[1:]:
        fs, x = load(path)
        notes = find_notes(x, fs)
        print(f'{path}: {len(notes)} notes measured')
        sets.append((path.split('/')[-1][:9], table(notes)))
    print('bands (Hz):', BANDS)
    for k0 in range(21, 109, 6):
        if any(k0 in t for _, t in sets):
            print(f'keys {k0}-{k0 + 5}')
        for name, t in sets:
            if k0 in t:
                show(name, t[k0])


if __name__ == '__main__':
    main()
