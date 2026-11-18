#!/usr/bin/env python3
"""tools/reedcheck.py BIN INST LO HI (sounding MIDI range): each note at six key velocities (tongued, 0.9 s, then a rest) and slurs from
neighbours; the pitch error over each note's second half. Prints the notes more than 20 c off."""
import sys, os, subprocess, tempfile, numpy as np
from concurrent.futures import ThreadPoolExecutor
from scipy.io import wavfile
binp, inst, lo, hi = sys.argv[1], sys.argv[2], int(sys.argv[3]), int(sys.argv[4])
VELS = [20, 45, 64, 90, 110, 127]
def cents(x, fs, f0):
    x = x - x.mean()
    if np.sqrt(np.mean(x * x)) < 1e-4: return None
    ac = np.correlate(x, x, 'full')[len(x) - 1:]
    a, b = int(fs / (f0 * 2.6)), int(fs / (f0 / 2.6))
    k = a + np.argmax(ac[a:b])
    for j in range(a + 1, k):  # the shortest period nearly as good (not its double)
        if ac[j] > 0.92 * ac[k] and ac[j] >= ac[j - 1] and ac[j] >= ac[j + 1]:
            k = j
            break
    d = 0.5 * (ac[k - 1] - ac[k + 1]) / (ac[k - 1] - 2 * ac[k] + ac[k + 1])
    return 1200 * np.log2(fs / (k + d) / f0)
def run(seq, path):
    subprocess.run([binp, '-inst', inst, '-seq', seq, '-o', path], capture_output=True, text=True)
    fs, x = wavfile.read(path); x = x.astype(float); x = x if x.ndim == 1 else x.mean(1)
    return fs, x / (32768 if np.abs(x).max() > 2 else 1)
def one(m):
    f0 = 440 * 2 ** ((m - 69) / 12); out = []
    with tempfile.TemporaryDirectory(dir=os.environ.get('VEL_TMP')) as d:
        fs, x = run(' '.join(f'on {m} {v} wait 0.9 off {m} wait 0.35' for v in VELS), os.path.join(d, 'a.wav'))
        for i, v in enumerate(VELS):
            t = i * 1.25
            seg = x[int((t + 0.45) * fs):int((t + 0.85) * fs)]
            out.append((f'v{v}', cents(seg, fs, f0), 20 * np.log10(np.sqrt(np.mean(seg ** 2)) + 1e-9)))
        for dm in (-7, -2, -1, 1, 2, 7):  # slurred into from a neighbour
            n = m + dm
            if n < lo or n > hi: continue
            fs, x = run(f'on {n} 80 wait 0.6 on {m} 80 wait 0.05 off {n} wait 0.85 off {m}', os.path.join(d, 'b.wav'))
            seg = x[int(1.05 * fs):int(1.45 * fs)]
            out.append((f's{dm:+d}', cents(seg, fs, f0), 0))
    return m, out
with ThreadPoolExecutor(14) as ex:
    res = list(ex.map(one, range(lo, hi + 1)))
bad = 0; tot = 0; lv = []
for m, out in res:
    b = [f'{k}:{"silent" if c is None else f"{c:+.0f}"}' for k, c, _ in out if c is None or abs(c) > 20]
    tot += len(out); bad += len(b)
    lv.append([l for k, c, l in out if k[0] == 'v'])
    if b: print(m, ' '.join(b))
lv = np.array(lv)
print(f'{inst}: {bad} of {tot} off by more than 20 c; level by velocity {VELS} (dB re v90, mean over notes):',
      ' '.join(f'{v:+.1f}' for v in (lv - lv[:, 3:4]).mean(0)), f'; spread over notes at v90 {lv[:,3].max() - lv[:,3].min():.1f} dB')
