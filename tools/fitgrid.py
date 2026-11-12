#!/usr/bin/env python3
"""The waveguide (./flute -imp) against the transfer-matrix model (tools/tmm.py):
the first three resonances of every first- and second-octave fingering, per
instrument, in cents. The grid's hole-position compensation is fitted with this.

    tools/fitgrid.py [-inst NAME ...] [-env K=V ...] [-modes N]
"""
import math, os, re, subprocess, sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import tmm

RANGE = {'flute': (59, 73), 'flute-c': (60, 73), 'piccolo': (62, 73), 'alto': (60, 73), 'bass': (60, 73)}


def fdtd_peaks(inst, midi, pattern=None, env=None):
    e = dict(os.environ)
    e.update(env or {})
    if pattern:
        e['PJ_IMPHOLES'] = pattern
    out = subprocess.run([os.path.join(ROOT, 'flute'), '-inst', inst, '-imp', str(midi)],
                         capture_output=True, text=True, env=e, cwd=ROOT).stdout
    return [float(v) for v in out.split(':', 1)[1].split('(')[0].split()]


def compare(inst_name, env=None, modes=3):
    """{written midi: [cents of mode 1..modes]} waveguide minus tmm."""
    inst = tmm.Instrument(inst_name)
    lo, hi = RANGE[inst_name]
    notes = [m for m in range(lo, hi + 1) if m in tmm.FINGERINGS]
    def one(m):
        p = tmm.FINGERINGS[m]
        t = 440 * 2 ** ((m + inst.transpose - 69) / 12)
        ref = [v for v in inst.peaks(p) if v > 0.8 * t][:modes]
        got = [v for v in fdtd_peaks(inst_name, m + inst.transpose, env=env) if v > 0.8 * t]
        out = []
        for r in ref:  # nearest waveguide peak to each reference peak
            g = min(got, key=lambda v: abs(math.log(v / r))) if got else float('nan')
            out.append(1200 * math.log2(g / r))
        return m, out
    with ThreadPoolExecutor(8) as ex:
        return dict(ex.map(one, notes))


if __name__ == '__main__':
    args = sys.argv[1:]
    insts, env, modes = [], {}, 3
    while args:
        a = args.pop(0)
        if a == '-inst': insts.append(args.pop(0))
        elif a == '-env':
            k, v = args.pop(0).split('=', 1)
            env[k] = v
        elif a == '-modes': modes = int(args.pop(0))
    for name in insts or ['flute', 'flute-c', 'piccolo', 'alto', 'bass']:
        r = compare(name, env, modes)
        worst = [max(abs(v[i]) for v in r.values() if len(v) > i) for i in range(modes)]
        print(f'{name}: worst ' + ', '.join(f'mode {i + 1} {w:.1f} c' for i, w in enumerate(worst)))
        for m, v in r.items():
            print(f'  {m}: ' + ' '.join(f'{x:+5.1f}' for x in v))
