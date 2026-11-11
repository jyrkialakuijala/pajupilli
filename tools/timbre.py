#!/usr/bin/env python3
"""Timbre sweep: harmonic levels of the radiated sound against a target.

  tools/timbre.py                         current defaults
  tools/timbre.py PJ_SPREAD=0.2,0.3 PJ_Y0=0.3,1   all combinations

Target levels (dB re H1, mezzo-forte, radiated) are estimates from the flute
literature (Fletcher 1975; UNSW spectra): rich low register with H2 near H1,
rolling off ~6-8 dB per harmonic; nearly sinusoidal high register.
"""
import itertools, os, subprocess, sys, tempfile
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
NOTES = {60: [0, -7, -14, -20, -25], 67: [-2, -9, -16, -22, -27],
         72: [-6, -14, -21, -27, -32], 81: [-12, -21, -29, -35, -40],
         88: [-18, -28, -36, -42, -46]}

def spectrum(env, note, dyn=0.5):
    with tempfile.NamedTemporaryFile(suffix='.wav') as f:
        subprocess.run([os.path.join(ROOT, 'flute'), '-note', str(note), '-secs', '0.8',
                        '-dyn', str(dyn), '-o', f.name], env=env, check=True,
                       stdout=subprocess.DEVNULL)
        x = np.fromfile(f.name, dtype=np.float32)[11:]
    seg = x[len(x) // 2:]
    seg = seg - seg.mean()
    f0 = 440 * 2 ** ((note - 69) / 12)
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 17))
    fr = np.fft.rfftfreq(1 << 17, 1 / 48000)
    dom = fr[np.argmax(sp)] / f0
    h = [sp[(fr > f0 * k * 0.97) & (fr < f0 * k * 1.03)].max() for k in range(1, 7)]
    lv = [20 * np.log10(v / h[0] + 1e-12) for v in h[1:]]
    ok = abs(dom - round(dom)) < 0.05 and np.sqrt((seg ** 2).mean()) > 1e-3
    # the fundamental must be the sounding note (dominant may be a harmonic)
    ok = ok and h[0] > 0.05 * max(h)
    return lv, ok, np.sqrt((seg ** 2).mean())

def evaluate(over):
    env = dict(os.environ, **over)
    err, bad, rows = [], 0, []
    for n, tgt in NOTES.items():
        lv, ok, r = spectrum(env, n)
        if not ok: bad += 1
        err += [np.clip(a - b, -20, 20) for a, b in zip(lv, tgt)]
        rows.append(f"{n}:" + ' '.join(f"{v:+.0f}" for v in lv) + ('' if ok else '!'))
    score = float(np.sqrt(np.mean(np.square(err)))) + 15 * bad
    return score, rows

def main():
    axes = [(a.split('=')[0], a.split('=')[1].split(',')) for a in sys.argv[1:]]
    combos = list(itertools.product(*[v for _, v in axes])) or [()]
    res = []
    for c in combos:
        over = {k: v for (k, _), v in zip(axes, c)}
        s, rows = evaluate(over)
        res.append((s, over, rows))
        print(f"{s:6.1f}  {' '.join(f'{k}={v}' for k, v in over.items()):40s} {'  '.join(rows)}", flush=True)
    if len(res) > 1:
        s, over, rows = min(res, key=lambda t: t[0])
        print(f"best {s:.1f}: {over}")

if __name__ == '__main__':
    main()
