#!/usr/bin/env python3
"""The acoustic pianos' keys against each other, velocity by velocity (research/piano_keys.md).

    tools/pianokeys.py measure --bin ./pajupilli --tag base [--fonts]   every key x velocity, to a JSON
    tools/pianokeys.py emit --from base                                 src/pianokeys_data.h
    tools/pianokeys.py report --before base --after new                 research/piano_keys.md

Each note alone (held 1.2 s in a 2.2 s slot), its loudest 400 ms, K-weighted (tools/gmcompare.py's
level probe), against the same synth's piano C3, C4, C5 at velocity 96 in the same file. The fonts
(TiMidity++ with FluidR3, TimGM6mb, Jnsgm2 and gm.sf2; FluidSynth with SGM, an SF3) are a comparison.
"""
import argparse, json, os, sys, traceback
from concurrent.futures import ProcessPoolExecutor, as_completed
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import numpy as np
import gmcompare as gc

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PIANOS = {0: 'grand-piano', 1: 'bright-piano', 3: 'honky-tonk'}
VELS = (16, 32, 48, 64, 80, 96, 112, 127)
KEYS = list(range(21, 109))
SLOT, HOLD = 2.2, 1.2
gc.SF['gm'] = os.path.expanduser('~/soundfonts/gm.sf2')
FONTS = [('FluidR3', 'tim', 'FluidR3'), ('TimGM6mb', 'tim', 'TimGM6mb'), ('Jnsgm2', 'tim', 'Jnsgm2'), ('gm', 'tim', 'gm'), ('SGM', 'fs', 'SGM')]


def path(tag): return os.path.join(ROOT, 'research', 'piano_keys_%s.json' % tag)


def one(p, v, name, kind, font, pj, work):
    w = os.path.join(work, 'v%d' % v); os.makedirs(w, exist_ok=True)
    return (p, v, name) + gc.lv_one(p, name, kind, font, pj, w, None, (KEYS, (v,), SLOT, HOLD))[2:]


def measure(a):
    pj = os.path.abspath(a.bin); out = {}
    jobs = [(p, v, 'pajupilli', 'pj', None) for p in PIANOS for v in VELS]
    if a.fonts: jobs += [(p, v, n, k, f) for p in PIANOS for v in VELS for n, k, f in FONTS]
    with ProcessPoolExecutor(a.jobs) as ex:
        fut = [ex.submit(one, p, v, n, k, f, pj, a.work) for p, v, n, k, f in jobs]
        for i, f in enumerate(as_completed(fut)):
            try:
                p, v, n, res = f.result()
                D = out.setdefault(str(p), {}).setdefault(n, {'anchor': [], 'notes': {}})
                D['anchor'].append(res['anchor'][0])
                for s, l in res['notes'].items(): D['notes'][s] = l[0]
            except Exception:
                print('FAILED\n' + traceback.format_exc(), file=sys.stderr)
            print('%d/%d' % (i + 1, len(fut)), file=sys.stderr, flush=True)
    for P in out.values():
        for D in P.values(): D['anchor'] = float(np.mean(D['anchor']))
    json.dump(out, open(path(a.tag), 'w'))


if __name__ == '__main__':
    ap = argparse.ArgumentParser(); ap.add_argument('cmd', choices=['measure'])
    ap.add_argument('--bin', default='./pajupilli'); ap.add_argument('--tag', default='base'); ap.add_argument('--fonts', action='store_true')
    ap.add_argument('--jobs', type=int, default=10); ap.add_argument('--work', default='/tmp/pianokeys')
    a = ap.parse_args(); measure(a)
