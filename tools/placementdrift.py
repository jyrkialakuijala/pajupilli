#!/usr/bin/env python3
"""Does the brush's strike place drift with the phrase? (research/stroke_placement.md 7)

On a separated drum stem (Demucs: drums.wav, and no_drums.wav or the other stems for the music),
per stroke: a place feature, the head's upper modes (500-2000 Hz) against its (0,1) band (120-500
Hz), in the 100 ms after the stroke. The stroke's loudness and the music's own bands under it (the
separation's leakage) are regressed out; strokes are grouped by their place in the bar (a fitted
beat grid: beat in the bar x on the beat or the swung eighth) and each group's mean removed (the
bar's own pattern of strokes). What is left is tested against shuffles within the groups:
  - a regular period (Lomb-Scargle over 6..64 beats);
  - the slow part (running means over 8 and 16 beats) and bar-aligned blocks (2, 4, 8 bars);
  - the blocks' lag correlations (holding, or swinging back and forth).

    tools/placementdrift.py STEMDIR T0 T1 [MUSIC.wav,...]

The stems are copyrighted recordings: they stay out of the repository (research/drums.md).
"""
import os, sys
import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, sosfiltfilt, find_peaks, lombscargle

def load(p, t0, t1):
    fs, x = wavfile.read(p); x = x.astype(float); x = x.mean(1) if x.ndim > 1 else x
    return fs, x[int(t0 * fs):int(t1 * fs)]

def strokes(fs, x):
    hp = sosfiltfilt(butter(4, [2000, 10000], 'bandpass', fs=fs, output='sos'), x)
    env = np.sqrt(sosfiltfilt(butter(2, 400, fs=fs, output='sos'), hp * hp).clip(0))
    hop = int(fs * 0.002); e = np.log(env[::hop] + 1e-6)
    fl = np.convolve(np.maximum(0, np.diff(e, prepend=e[0])), np.ones(4), 'same')
    pk, _ = find_peaks(fl, height=np.percentile(fl, 96), distance=int(0.12 / 0.002))
    on = pk * hop
    # sharpness: the 4-12 kHz envelope's peak with 0.25 ms smoothing over 5 ms smoothing
    h2 = sosfiltfilt(butter(4, [4000, 12000], 'bandpass', fs=fs, output='sos'), x) ** 2
    fast = sosfiltfilt(butter(2, 2000, fs=fs, output='sos'), h2).clip(0)
    slow = sosfiltfilt(butter(2, 100, fs=fs, output='sos'), h2).clip(1e-15)
    sharp = []
    for i in on:
        a, b = max(0, i - int(0.005 * fs)), i + int(0.02 * fs)
        sharp.append(10 * np.log10(fast[a:b].max() / slow[a:b].max()))
    return on, np.array(sharp)

BANDS = [(120, 300), (300, 500), (500, 800), (800, 1300), (1300, 2000)]
def feats(fs, x, on):
    out = []
    for i in on:
        seg = x[i + int(0.003 * fs): i + int(0.10 * fs)]
        if len(seg) < int(0.09 * fs): out.append([np.nan] * (len(BANDS) + 1)); continue
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
        f = np.fft.rfftfreq(len(seg), 1 / fs)
        e = [10 * np.log10(sp[(f >= a) & (f < b)].sum() + 1e-12) for a, b in BANDS]
        out.append(e + [10 * np.log10(sp[(f >= 120) & (f < 2000)].sum() + 1e-12)])
    return np.array(out)



def load(p, t0, t1):
    fs, x = wavfile.read(p); x = x.astype(float); x = x.mean(1) if x.ndim > 1 else x
    return fs, x[int(t0 * fs):int(t1 * fs)]

def grid(t):
    """Beat period and phase: the stroke times' best fit to a regular grid (period searched)."""
    best = None
    for b in np.linspace(0.25, 1.0, 1501):
        ph = np.angle(np.exp(2j * np.pi * t / b).mean())
        s = np.abs(np.exp(2j * np.pi * t / b).mean()) * (1 if b > 0.28 else 0)
        # prefer the beat over its subdivisions: weight by the share of strokes near a beat
        if best is None or s > best[0]: best = (s, b, ph)
    _, b, ph = best
    # refine: a linear fit of beat number to time (tempo drift allowed: quadratic)
    k = np.round(t / b - ph / (2 * np.pi))
    c = np.polyfit(k, t, 2)
    return b, k.astype(int), c


def analyse(name, d, t0, t1, music=None):
    fs, x = load(os.path.join(d, 'drums.wav'), t0, t1)
    y = sum(load(os.path.join(d, m), t0, t1)[1] for m in music) if music else load(os.path.join(d, 'no_drums.wav'), t0, t1)[1]
    on, _ = strokes(fs, x)
    F = feats(fs, x, on); G = feats(fs, y, on)
    ok = ~np.isnan(F).any(1); t = on[ok] / fs; F = F[ok]; G = G[ok]
    place = F[:, 2:5].mean(1) - F[:, 0:2].max(1)
    b, k, c = grid(t); dev = (t - np.polyval(c, k)) / b
    cls = (k % 4) * 3 + np.clip(np.round(dev * 3).astype(int), -1, 1) + 1
    A = np.column_stack([np.ones_like(place), F[:, -1], G[:, :5], G[:, -1]])
    res = place - A @ np.linalg.lstsq(A, place, rcond=None)[0]
    for q in np.unique(cls):
        m = cls == q; res[m] -= res[m].mean()
    tb = t / b
    rng = np.random.default_rng(3)
    def shuffled():
        r = res.copy()
        for q in np.unique(cls):
            m = np.where(cls == q)[0]; r[m] = r[rng.permutation(m)]
        return r
    def slow(r, W):  # running mean over W beats: its sd
        return np.array([r[np.abs(tb - u) < W / 2].mean() for u in tb]).std()
    def blockvar(r, B):  # between-block variance for blocks of B beats, best over the offsets
        best = 0
        for off in range(0, B, 2):
            idx = ((tb + off) // B).astype(int)
            means = [r[idx == i].mean() for i in np.unique(idx) if (idx == i).sum() >= 3]
            best = max(best, np.var(means))
        return best
    out = [name, len(t)]
    for W in (8, 16):
        s0 = slow(res, W); ss = np.array([slow(shuffled(), W) for _ in range(300)])
        out.append((W, s0, np.median(ss), (ss >= s0).mean()))
    bl = []
    for B in (8, 16, 32):
        v0 = blockvar(res, B); vs = np.array([blockvar(shuffled(), B) for _ in range(200)])
        bl.append((B, v0 ** 0.5, np.median(vs) ** 0.5, (vs >= v0).mean()))
    print(f'{name:7s} {len(t):3d} strokes | slow part (running mean): ' +
          '  '.join(f'{W} beats {s0:.1f} dB vs shuffled {m:.1f} (p {p:.3f})' for W, s0, m, p in out[2:]))
    print(f'{"":7s}               | bar-aligned blocks: ' +
          '  '.join(f'{B // 4} bars {v:.1f} vs {m:.1f} dB (p {p:.3f})' for B, v, m, p in bl))


def lags(d, t0, t1, music=None):
    fs, x = load(os.path.join(d, 'drums.wav'), t0, t1)
    y = sum(load(os.path.join(d, m), t0, t1)[1] for m in music) if music else load(os.path.join(d, 'no_drums.wav'), t0, t1)[1]
    on, _ = strokes(fs, x); F = feats(fs, x, on); G = feats(fs, y, on)
    ok = ~np.isnan(F).any(1); t = on[ok] / fs; F = F[ok]; G = G[ok]
    place = F[:, 2:5].mean(1) - F[:, 0:2].max(1)
    b, k, c = grid(t); dev = (t - np.polyval(c, k)) / b
    cls = (k % 4) * 3 + np.clip(np.round(dev * 3).astype(int), -1, 1) + 1
    A = np.column_stack([np.ones_like(place), F[:, -1], G[:, :5], G[:, -1]])
    r = place - A @ np.linalg.lstsq(A, place, rcond=None)[0]
    for q in np.unique(cls):
        m = cls == q; r[m] -= r[m].mean()
    tb = t / b
    line = ''
    for B in (4, 8, 16):
        idx = (tb // B).astype(int)
        m = np.array([r[idx == i].mean() for i in range(idx.max() + 1) if (idx == i).sum() >= 3])
        c1 = np.corrcoef(m[:-1], m[1:])[0, 1]; c2 = np.corrcoef(m[:-2], m[2:])[0, 1] if len(m) > 4 else np.nan
        line += f' | {B // 4}-bar blocks: lag-1 r {c1:+.2f}, lag-2 r {c2:+.2f} (n {len(m)})'
    print('        ' + line)


if __name__ == '__main__':
    a = sys.argv
    music = a[4].split(',') if len(a) > 4 else None
    analyse(os.path.basename(a[1].rstrip('/'))[:20], a[1], float(a[2]), float(a[3]), music)
    lags(a[1], float(a[2]), float(a[3]), music)
