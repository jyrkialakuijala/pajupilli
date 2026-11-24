#!/usr/bin/env python3
"""The players' practice: how each slur and trill is best played (src/slur_data.h).

A wind player knows in advance what each slur needs: where the lips lead, where the air pushes
or eases, where the tongue touches. Here every player practises its slurs on the model, by
interval, register and dynamics, and the results are written as a table the players read
(src/slurs.h). The trills' fastest playable speeds are measured the same way.

  tools/slurs.py survey INST [-o FILE]   play the grid with the built-in table, print what fails
  tools/slurs.py practice INST           search the gestures, write tools/slurs/INST.json
  tools/slurs.py trills INST             measure the trills, add them to tools/slurs/INST.json
  tools/slurs.py emit                    rewrite src/slur_data.h from tools/slurs/*.json
  tools/slurs.py verify INST             (after a rebuild) play the grid again, compare with the json
  tools/slurs.py recalibrate [INST ...]  practice + trills + emit + make + verify, all of it
  tools/slurs.py one INST FROM TO DYN [NAME=VALUE ...]   one slur, its numbers (and -w FILE.wav)

Options: -j N workers (default 12), -quick (a coarser grid).
Run as: python3 -I tools/slurs.py ...
"""
import json
import math
import os
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor

import numpy as np
from scipy.io import wavfile
from scipy.signal import medfilt, resample_poly

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BIN = os.path.join(ROOT, 'pajupilli')
WORK = os.path.join(ROOT, 'research', 'gm_compare', 'work', 'slur', 'tmp')
DATA = os.path.join(ROOT, 'tools', 'slurs')
HEADER = os.path.join(ROOT, 'src', 'slur_data.h')

# The instruments that practise: sounding range, and the gesture's numbers (src/slurs.h:
# the same order as SlurGesture's fields), each with its range for the search.
INSTRUMENTS = {
    'horn': dict(lo=41, hi=77, kind='lip'),
    'clarinet': dict(lo=50, hi=82, kind='reed'),
    'oboe': dict(lo=58, hi=85, kind='reed'),
}
DYNS = [0.3, 0.5, 0.75]
INTERVALS = [1, 2, 3, 4, 5, 7, 9, 12]
HOLD = 0.6  # s on each note


def freq(m):
    return 440.0 * 2 ** ((m - 69) / 12)


def render(inst, seq, dyn, knobs=None, keep=None, tag='x'):
    """Play key events; returns (fs, samples)."""
    os.makedirs(WORK, exist_ok=True)
    path = keep or os.path.join(WORK, '%d_%s.wav' % (os.getpid(), tag))
    env = dict(os.environ)
    env['PJ_WANDER'] = '0'
    env['PJ_THREADS'] = '12'  # (a calibration after a rebuild: no more workers than this)
    for k, v in (knobs or {}).items():
        env[k] = repr(float(v))
    r = subprocess.run([BIN, '-inst', inst, '-dyn', repr(dyn), '-seq', seq, '-o', path], env=env,
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if r.returncode != 0:
        raise RuntimeError('pajupilli: ' + r.stderr.strip())
    fs, x = wavfile.read(path)
    if not keep:
        os.remove(path)
    if x.ndim > 1:
        x = x.mean(axis=1)
    if x.dtype.kind == 'i':
        x = x / float(np.iinfo(x.dtype).max)
    return fs, x.astype(np.float64)


def track(x, fs, flo, fhi, hop=0.002):
    """Pitch (Hz, 0 where none), clarity and level (dB) per frame: McLeod's normalized
    square difference over 2.6 periods of the lowest pitch looked for."""
    # (no more samples than the pitch needs: the work goes with the square of the rate)
    q = int(min(8, fs // (40 * flo), fs // (8 * fhi)))  # (the shortest period 8 samples or more)
    if q > 1:
        x = resample_poly(x, 1, q)
        fs = fs / q
    lmin = int(fs / fhi)
    lmax = int(math.ceil(fs / flo))
    w = int(1.3 * lmax)  # the summed part
    n = w + lmax
    h = max(1, int(hop * fs))
    starts = np.arange(0, len(x) - n, h)
    idx = starts[:, None] + np.arange(w)[None, :]
    a = x[idx]
    ea = (a * a).sum(axis=1)
    cs = np.concatenate([[0.0], np.cumsum(x * x)])
    nsdf = np.zeros((len(starts), lmax + 2))
    for lag in range(max(1, lmin - 1), lmax + 2):
        b = x[idx + lag] if idx[-1, -1] + lag < len(x) else None
        if b is None:
            xx = np.concatenate([x, np.zeros(lag + 1)])
            b = xx[idx + lag]
        eb = cs[np.minimum(starts + lag + w, len(x))] - cs[np.minimum(starts + lag, len(x))]
        nsdf[:, lag] = 2 * (a * b).sum(axis=1) / (ea + eb + 1e-20)
    f = np.zeros(len(starts))
    cl = np.zeros(len(starts))
    for i in range(len(starts)):
        r = nsdf[i]
        # peaks: local maxima within the lag range
        k = np.arange(lmin, lmax + 1)
        pk = k[(r[k] > r[k - 1]) & (r[k] >= r[k + 1]) & (r[k] > 0)]
        if len(pk) == 0:
            continue
        top = r[pk].max()
        p = pk[r[pk] >= 0.9 * top][0]
        y0, y1, y2 = r[p - 1], r[p], r[p + 1]
        d = y0 - 2 * y1 + y2
        off = 0.5 * (y0 - y2) / d if d < 0 else 0.0
        f[i] = fs / (p + off)
        cl[i] = y1
    lev = 10 * np.log10(ea / w + 1e-20)
    t = (starts + n / 2) / fs
    return t, f, cl, lev


def cents(f, ref):
    return 1200 * np.log2(np.maximum(f, 1e-9) / ref)


def analyse(x, fs, a, b, ton, hold=HOLD):
    """A slur from note a to note b at ton (s): what happened."""
    fa, fb = freq(a), freq(b)
    lo, hi = min(fa, fb), max(fa, fb)
    i0, i1 = max(0, int((ton - 0.36) * fs)), min(len(x), int((ton + hold + 0.03) * fs))
    t, f, cl, lev = track(x[i0:i1], fs, lo / 1.35, min(hi * 3.3, 0.45 * fs))
    t = t + i0 / fs
    res = dict(ok=False, settle=1e3 * hold, over=0.0, junk=0.0, dip=0.0, bump=0.0, hole=0.0, off=0.0, why='')

    def steady(t0, t1, fn):
        m = (t >= t0) & (t <= t1)
        if m.sum() < 5:
            return None
        good = m & (cl > 0.8)
        if good.sum() < 0.7 * m.sum():
            return None
        c = cents(f[good], fn)
        # (the note's own unrest: breath noise in the short frames, the ear's trimming)
        return (float(np.median(c)), float(np.median(lev[m])), float(np.percentile(c, 90) - np.percentile(c, 10)) / 2,
                float(np.percentile(lev[m], 90) - np.percentile(lev[m], 10)) / 2)

    # (0.3 s where there is room: some notes sway by themselves a few times a second, the oboe's
    # B4 +-17 c, the clarinet's G4; a slur is not to be blamed for that)
    sa = steady(ton - min(0.3, hold - 0.1), ton - 0.02, fa)
    sb = steady(ton + hold - min(0.3, hold - 0.15), ton + hold - 0.02, fb)
    if sa is None or abs(sa[0]) > 40:
        res['why'] = 'from-note unsteady' if sa is None else 'from-note %+.0f c' % sa[0]
        res['skip'] = True  # not a matter for the slur
        return res
    if sb is None or abs(sb[0]) > 40:
        res['why'] = 'not there at the end' if sb is None else 'ends %+.0f c' % sb[0]
        return res
    pa, pb = sa[0], 1200 * math.log2(fb / fa) + sb[0]  # cents re fa
    La, Lb = sa[1], sb[1]
    m = (t >= ton) & (t <= ton + hold - 0.02)
    tt, c, clm, lv = t[m] - ton, cents(f[m], fa), cl[m], lev[m]
    c = medfilt(c, 7)  # (14 ms: single frames fooled by noise)
    tol = max(10.0, 2.0 * sb[2])
    there = (np.abs(c - pb) < tol) & (clm > 0.75) & (np.abs(lv - Lb) < 2.0 + sb[3])
    bad = np.nonzero(~there)[0]
    settle = 0.0 if len(bad) == 0 else tt[min(bad[-1] + 1, len(tt) - 1)]
    if len(bad) and bad[-1] >= len(tt) - 3:
        res['why'] = 'never settles'
        return res
    w = tt <= settle + 0.02
    voiced = w & (clm > 0.85) & (lv > min(La, Lb) - 15)
    lo_c, hi_c = min(pa, pb), max(pa, pb)
    out = np.maximum(c - hi_c, lo_c - c) - max(sa[2], sb[2])
    hop = float(tt[1] - tt[0]) if len(tt) > 1 else 0.002
    res.update(ok=True, settle=1e3 * settle,
               over=float(out[voiced & (out < 150)].max(initial=0.0)),
               junk=1e3 * hop * int((voiced & (out >= 150)).sum()),
               dip=float(max(0.0, min(La, Lb) - lv[w].min())),
               bump=float(max(0.0, lv[w].max() - max(La, Lb))),
               hole=1e3 * hop * int((lv[w] < min(La, Lb) - 6).sum()),
               # (off: how far from the note it sits on the way, cents x s over the first 0.35 s
               # past 30 ms, beyond 5 c: a note that arrives sharp and drifts home for 200 ms)
               off=float(hop * np.minimum(60.0, np.maximum(0.0, np.abs(c - pb) - 5.0))[(tt > 0.03) & (tt < 0.35) & (clm > 0.7)].sum()),
               pb=sb[0], La=La, Lb=Lb)
    return res


def cost(r):
    """One number: ms of settling, the rest converted to ms."""
    if r.get('skip'):
        return 0.0
    if not r['ok']:
        return 1000.0
    return (r['settle'] + 3.0 * max(0.0, r['over'] - 3) + 2.0 * r['junk'] + 6.0 * max(0.0, r['dip'] - 3)
            + 6.0 * max(0.0, r['bump'] - 2) + 1.0 * r['hole'] + 50.0 * r['off'])


def slur(inst, a, b, dyn, knobs=None, keep=None, tag='x', vel=64, hold=HOLD):
    seq = 'on %d %d wait %g on %d %d off %d wait %g off %d' % (a, vel, hold, b, vel, a, hold, b)
    fs, x = render(inst, seq, dyn, knobs, keep, tag)
    r = analyse(x, fs, a, b, hold, hold)
    if not r['ok'] and not r.get('skip') and not speaks(inst, b, dyn, vel, tag):
        r['skip'] = True  # the note is not there tongued either: not a matter for the slur
        r['why'] = 'the note does not speak tongued'
    return r


_speaks = {}


def speaks(inst, m, dyn, vel, tag):
    """Is the note there (in tune, steady) when simply tongued?"""
    key = (inst, m, round(dyn, 3), vel)
    if key not in _speaks:
        fs, x = render(inst, 'on %d %d wait 0.7 off %d' % (m, vel, m), dyn, None, tag=tag + 'n')
        t, f, cl, lev = track(x, fs, freq(m) / 1.35, min(freq(m) * 3.3, 0.45 * fs))
        w = (t > 0.4) & (t < 0.68)
        good = w & (cl > 0.8)
        _speaks[key] = bool(good.sum() >= 0.7 * w.sum() and abs(np.median(cents(f[good], freq(m)))) <= 40)
    return _speaks[key]


_warm = set()


def warm(inst):
    """One run alone first: after a rebuild the instrument calibrates (many threads), and
    a dozen runs started together would each do it."""
    if inst not in _warm:
        info = INSTRUMENTS[inst]
        render(inst, 'on %d 64 wait 0.05 off %d' % (info['lo'] + 12, info['lo'] + 12), 0.5, tag='warm')
        _warm.add(inst)


def grid(inst, quick=False):
    info = INSTRUMENTS[inst]
    step = 6 if quick else 3
    cells = []
    for a in range(info['lo'], info['hi'] + 1, step):
        for iv in INTERVALS:
            for s in (1, -1):
                b = a + s * iv
                if info['lo'] <= b <= info['hi']:
                    for d in ([0.5] if quick else DYNS):
                        cells.append((a, b, d))
    return cells


NAMES = 'C C# D D# E F F# G G# A A# B'.split()


def name(m):
    return '%s%d' % (NAMES[m % 12], m // 12 - 1)


def fmt(r):
    if r.get('skip'):
        return 'skip (%s)' % r['why']
    if not r['ok']:
        return 'FAIL (%s)' % r['why']
    return 'settle %3.0f ms  over %4.1f c  off %4.2f c s  junk %3.0f ms  dip %4.1f dB  bump %4.1f dB  hole %3.0f ms  cost %4.0f' % (
        r['settle'], r['over'], r['off'], r['junk'], r['dip'], r['bump'], r['hole'], cost(r))


_pool = None


def pmap(fn, items, jobs):
    """fn: a function of this module (the workers are processes: the analysis is Python)."""
    global _pool
    if jobs <= 1:
        return [fn(x) for x in items]
    if _pool is None:
        _pool = ProcessPoolExecutor(max_workers=jobs)
    return list(_pool.map(fn, items, chunksize=1))


def job_slur(w):
    """(inst, a, b, dyn, knobs, vel, hold, tag) -> the slur's numbers."""
    inst, a, b, dyn, knobs, vel, hold, tag = w
    return slur(inst, a, b, dyn, knobs, tag=tag, vel=vel, hold=hold)


def job_both(w):
    """A slur on both occasions: the worse of the two."""
    inst, a, b, d, knobs, tag = w
    rs = [slur(inst, a, b, min(1.0, d + dd), knobs, tag=tag, vel=vel, hold=hold) for (hold, dd, vel) in variants(True)]
    if any(r.get('skip') for r in rs):
        return [r for r in rs if r.get('skip')][0]
    return max(rs, key=cost)


def job_trill(w):
    inst, lo, iv, tag = w
    best = 0
    for r in TRILL_RATES:
        if not trill_ok(inst, lo, iv, r, tag):
            break
        best = r
    return best


def survey(inst, jobs, quick, out=None, knobs=None):
    warm(inst)
    cells = grid(inst, quick)
    rs = pmap(job_slur, [(inst, a, b, d, knobs, 64, HOLD, str(i)) for i, (a, b, d) in enumerate(cells)], jobs)
    rows = []
    for (a, b, d), r in zip(cells, rs):
        rows.append(dict(a=a, b=b, dyn=d, **r))
    fails = [x for x in rows if not x['ok'] and not x.get('skip')]
    oks = [x for x in rows if x['ok']]
    print('%s: %d slurs, %d fail, %d skipped' % (inst, len(rows), len(fails), sum(1 for x in rows if x.get('skip'))))
    if oks:
        for k in ('settle', 'over', 'off', 'junk', 'dip', 'bump', 'hole'):
            v = np.array([x[k] for x in oks])
            print('  %-6s median %6.1f  90 %% %6.1f  max %6.1f' % (k, np.median(v), np.percentile(v, 90), v.max()))
        print('  cost   mean %.1f (all: %.1f)' % (np.mean([cost(x) for x in oks]), np.mean([cost(x) for x in rows])))
    for x in sorted(rows, key=lambda x: -cost(x))[:25]:
        print('  %-4s -> %-4s dyn %.2f  %s' % (name(x['a']), name(x['b']), x['dyn'], fmt(x)))
    if out:
        with open(out, 'w') as f:
            json.dump(rows, f, indent=0)
    return rows



# ---- The practice ----
# The gesture's numbers (src/slurs.h, SlurGesture): name, lowest, highest, the search's first
# step (x: a factor), and where a player who knows nothing starts.
PARAMS = [
    ('fingers', 0.4, 2.5, ('x', 1.8)),
    ('lead', 0.0, 0.04, ('+', 0.015)),
    ('air', -0.7, 1.0, ('+', 0.35)),
    ('airT', 0.01, 0.15, ('x', 2.0)),
    ('lip', -0.5, 0.6, ('+', 0.15)),
    ('lipT', 0.01, 0.12, ('x', 2.0)),
    ('dip', -0.3, 0.5, ('+', 0.12)),
    ('dipT', 0.03, 0.25, ('x', 1.8)),
    ('tongue', 0.0, 0.012, ('+', 0.005)),
    ('floor', 0.0, 0.6, ('+', 0.3)),
]
PLAIN = dict(fingers=1, lead=0, air=0, airT=0.03, lip=0, lipT=0.03, dip=0, dipT=0.08, tongue=0, floor=0)
# The horn's way before the practice (src/hornplayer.cc's knobs): where its search starts.
START = {
    'horn': {1: dict(PLAIN, lip=0.3, air=0.5), -1: dict(PLAIN, lip=0.3, air=0.3, dip=0.18)},
    'clarinet': {1: dict(PLAIN), -1: dict(PLAIN)},
    'oboe': {1: dict(PLAIN), -1: dict(PLAIN)},
}
FAIL = 600.0  # a failed slur's cost in a mean (ms)


def knobs_of(g):
    k = {'PJ_SLURTRY': 1}
    for name, _, _, _ in PARAMS:
        k['PJ_SLURTRY_' + name.upper()] = g[name]
    return k


def moves(g, scale):
    """The gesture with one number stepped up or down."""
    out = []
    for name, lo, hi, (kind, step) in PARAMS:
        for sign in (1, -1):
            if kind == 'x':
                v = g[name] * step ** (sign * scale)
            else:
                v = g[name] + sign * step * scale
            v = min(hi, max(lo, v))
            if abs(v - g[name]) > 1e-9:
                h = dict(g)
                h[name] = round(v, 5)
                out.append(h)
    return out


def variants(robust):
    # (hold, dynamics offset, key velocity): a gesture must work on more than one occasion
    return [(HOLD, 0.0, 64), (0.4, 0.04, 70)] if robust else [(HOLD, 0.0, 64)]


class Searcher:
    def __init__(self, inst, jobs):
        self.inst, self.jobs, self.n = inst, jobs, 0
        self.memo = {}

    def costs(self, gestures, cells, robust):
        """Mean cost of each gesture over the cells (a, b, dyn)."""
        work = []
        for gi, g in enumerate(gestures):
            for (a, b, d) in cells:
                for (hold, dd, vel) in variants(robust):
                    work.append((gi, a, b, min(1.0, d + dd), vel, hold))

        keys = [(a, b, d, vel, hold, tuple(sorted(gestures[gi].items()))) for (gi, a, b, d, vel, hold) in work]
        todo = sorted(set(k for k in keys if k not in self.memo))
        rs = pmap(job_slur, [(self.inst, k[0], k[1], k[2], knobs_of(dict(k[5])), k[3], k[4], 's%d' % i)
                             for i, k in enumerate(todo)], self.jobs)
        for k, r in zip(todo, rs):
            self.memo[k] = min(FAIL, cost(r)) if not r.get('skip') else None
        self.n += len(todo)
        tot = [[] for _ in gestures]
        for w, k in zip(work, keys):
            if self.memo[k] is not None:
                tot[w[0]].append(self.memo[k])
        # (the worst cells count double: a way that leaves one slur failing is not learned)
        return [float(np.mean(t) * 0.7 + np.max(t) * 0.3) if t else 0.0 for t in tot]

    def search(self, start, cells, scales, robust):
        g = dict(start)
        best = self.costs([g], cells, robust)[0]
        for scale in scales:
            cand = moves(g, scale)
            cs = self.costs(cand, cells, robust)
            better = [(c, h) for c, h in zip(cs, cand) if c < best - 0.5]
            if not better:
                continue
            better.sort(key=lambda ch: ch[0])
            # the best single move, and all the helpful moves together (one each number)
            combo = dict(g)
            seen = set()
            for c, h in better:
                name = [n for n in h if h[n] != g[n]][0]
                if name not in seen:
                    seen.add(name)
                    combo[name] = h[name]
            cc = self.costs([combo], cells, robust)[0] if len(seen) > 1 else 1e9
            if cc < better[0][0]:
                g, best = combo, cc
            else:
                best, g = better[0]
        return g, best


def anchors(inst, iv, step=3):
    info = INSTRUMENTS[inst]
    return [a for a in range(info['lo'], info['hi'] + 1, step) if info['lo'] <= a + iv <= info['hi']]


def practice(inst, jobs, quick):
    warm(inst)
    S = Searcher(inst, jobs)
    info = INSTRUMENTS[inst]
    general = []
    # 1. The general way with each interval: mezzo-forte first (each interval starting from the
    #    one before), then soft and loud from there.
    for sign in (1, -1):
        prev = dict(START[inst][sign])
        for iv in INTERVALS:
            cells = [(a, a + sign * iv, 0.5) for a in anchors(inst, sign * iv, 6 if quick else 3)]
            if not cells:
                continue
            c0 = S.costs([prev], cells, False)[0]
            g, c = S.search(prev, cells, [1, 1, 0.5, 0.5, 0.25] if not quick else [1, 0.5], False)
            prev = g
            general.append(dict(interval=sign * iv, dyn=0.5, g=g, cost=c))
            print('%s %+3d mf: %5.0f -> %5.0f  %s' % (inst, sign * iv, c0, c, show(g)), flush=True)
            for d in ([] if quick else [0.3, 0.75]):
                cd = [(a, b, d) for (a, b, _) in cells]
                c0 = S.costs([g], cd, False)[0]
                gd, c = S.search(g, cd, [0.5, 0.5, 0.25], False)
                general.append(dict(interval=sign * iv, dyn=d, g=gd, cost=c))
                print('%s %+3d %.2f: %5.0f -> %5.0f  %s' % (inst, sign * iv, d, c0, c, show(gd)), flush=True)
    gen = {(r['interval'], r['dyn']): r for r in general}
    # 2. Every slur with its general way, on two occasions: which need a way of their own?
    cells = []
    for a in range(info['lo'], info['hi'] + 1, 2 if quick else 1):
        for iv in INTERVALS:
            for sign in (1, -1):
                b = a + sign * iv
                if info['lo'] <= b <= info['hi']:
                    for d in ([0.5] if quick else DYNS):
                        cells.append((a, b, d))

    rs = pmap(job_both, [(inst, a, b, d, knobs_of(gen[(b - a, d)]['g']), 'p%d' % i) for i, (a, b, d) in enumerate(cells)], jobs)
    base = [None if r.get('skip') else min(FAIL, cost(r)) for r in rs]
    own = []
    hard = []
    for (a, b, d), c in zip(cells, base):
        if c is None:
            continue
        typical = gen[(b - a, d)]['cost']
        if c >= FAIL or c > max(1.5 * typical, typical + 40):
            hard.append((a, b, d, c))
    print('%s: %d slurs, %d need their own way' % (inst, len(cells), len(hard)), flush=True)
    # 3. Those practise alone (on both occasions; the worse counts).
    S2 = Searcher(inst, jobs)
    for (a, b, d, c0) in hard:
        g0 = gen[(b - a, d)]['g']
        S2.memo = {}
        # (start from the neighbour's way if it has one: the table stays smooth)
        starts = [g0] + [o['g'] for o in own if o['interval'] == b - a and abs(o['from'] - a) <= 2 and not o['fails']][-1:]
        cs = S2.costs(starts, [(a, b, d)], True)
        k = int(np.argmin(cs))
        g, c = S2.search(starts[k], [(a, b, d)], [1, 1, 0.5, 0.25], True)
        fails = c >= 0.7 * FAIL
        if fails or c < 0.8 * cs[0]:
            own.append({'from': a, 'interval': b - a, 'dyn': d, 'g': g, 'cost': c, 'before': cs[0], 'fails': fails})
        print('%s %-4s -> %-4s %.2f: %5.0f -> %5.0f %s %s' % (inst, name(a), name(b), d, cs[0], c,
                                                              'FAILS' if fails else '', show(g)), flush=True)
    os.makedirs(DATA, exist_ok=True)
    path = os.path.join(DATA, inst + '.json')
    old = {}
    if os.path.exists(path):
        with open(path) as f:
            old = json.load(f)
    data = dict(inst=inst, general=general, own=own, trills=old.get('trills', []), renders=S.n + S2.n + 2 * len(cells))
    with open(path, 'w') as f:
        json.dump(data, f, indent=1)
    print('%s: %d renders, wrote %s' % (inst, data['renders'], path))


def show(g):
    return ' '.join('%s %g' % (n, g[n]) for n, _, _, _ in PARAMS if abs(g[n] - PLAIN[n]) > 1e-9)


# ---- Trills ----
TRILL_RATES = [3, 4, 6, 8, 10, 12, 14]  # notes per second; 14 (7 Hz): about the fingers' own limit
TRILL_INTERVALS = [1, 2, 3, 4]


def trill_ok(inst, lo, iv, rate, tag):
    """Does a trill lo / lo + iv at `rate` notes per second speak: both notes arriving?"""
    T = 1.0 / rate
    n = 14
    seq = 'on %d 64 wait 0.35' % lo
    cur = lo
    for k in range(n):
        nxt = lo + iv if cur == lo else lo
        seq += ' on %d 64 off %d wait %g' % (nxt, cur, T)
        cur = nxt
    seq += ' off %d' % cur
    fs, x = render(inst, seq, 0.5, None, tag=tag)
    fl, fh = freq(lo), freq(lo + iv)
    t, f, cl, lev = track(x, fs, fl / 1.35, min(fh * 3.3, 0.45 * fs), hop=0.001)
    ref = np.median(lev[(t > 0.2) & (t < 0.33)])
    good = 0
    tot = 0
    for k in range(3, n):
        t0 = 0.35 + k * T
        target = fh if k % 2 == 0 else fl
        m = (t >= t0 + 0.4 * T) & (t <= t0 + T)
        if m.sum() < 2:
            continue
        tot += 1
        c = cents(f[m], target)
        hit = (np.abs(c) < 30) & (cl[m] > 0.7) & (lev[m] > ref - 8)
        good += hit.mean() >= 0.6
    return tot > 0 and good >= 0.85 * tot


def trills(inst, jobs, quick):
    warm(inst)
    info = INSTRUMENTS[inst]
    work = [(lo, iv) for iv in TRILL_INTERVALS for lo in range(info['lo'], info['hi'] - iv + 1, 3 if quick else 1)]

    rates = pmap(job_trill, [(inst, lo, iv, 't%d' % i) for i, (lo, iv) in enumerate(work)], jobs)
    rows = [dict(lo=lo, interval=iv, rate=r) for (lo, iv), r in zip(work, rates)]
    path = os.path.join(DATA, inst + '.json')
    with open(path) as f:
        data = json.load(f)
    data['trills'] = rows
    with open(path, 'w') as f:
        json.dump(data, f, indent=1)
    for iv in TRILL_INTERVALS:
        v = [(r['lo'], r['rate']) for r in rows if r['interval'] == iv]
        print('%s trill +%d: %s' % (inst, iv, ' '.join('%s:%d' % (name(lo), r) for lo, r in v)))


# ---- The table ----
def cfloat(v):
    t = '%g' % v
    return t + ('f' if ('.' in t or 'e' in t) else '.0f')


def emit(write=True):
    """Rewrites src/slur_data.h from the json; write=False: only returns its text."""
    out = ['// Written by tools/slurs.py emit from tools/slurs/*.json: do not edit (src/slurs.h).',
           '// {instrument, the key left (-1: the general way), interval, dynamics,',
           '//  {fingers, lead, air, airT, lip, lipT, dip, dipT, tongue, floor, fails}}',
           'static const SlurRow kSlurRows[] = {']
    tr = []

    def row(inst, frm, r):
        g = r['g']
        return '    {"%s", %d, %d, %s, {%s, %s}},' % (
            inst, frm, r['interval'], cfloat(r['dyn']), ', '.join(cfloat(g[n]) for n, _, _, _ in PARAMS),
            'true' if r.get('fails') else 'false')

    for fn in sorted(os.listdir(DATA)) if os.path.isdir(DATA) else []:
        if not fn.endswith('.json'):
            continue
        with open(os.path.join(DATA, fn)) as f:
            d = json.load(f)
        inst = d['inst']
        out.append('    // %s: the general way (%d rows), then %d slurs with their own' % (inst, len(d['general']), len(d['own'])))
        for r in d['general']:
            out.append(row(inst, -1, r))
        for r in sorted(d['own'], key=lambda r: (r['from'], r['interval'], r['dyn'])):
            out.append(row(inst, r['from'], r))
        for r in d.get('trills', []):
            tr.append('    {"%s", %d, %d, %s},' % (inst, r['lo'], r['interval'], cfloat(r['rate'])))
    out.append('    {"", 0, 0, 0, {}},')
    out.append('};')
    out.append('// {instrument, the lower key, interval, the fastest trill that speaks (notes per second)}')
    out.append('static const TrillRow kTrillRows[] = {')
    out += tr
    out.append('    {"", 0, 0, 0},')
    out.append('};')
    text = '\n'.join(out) + '\n'
    if write:
        with open(HEADER, 'w') as f:
            f.write(text)
        print('wrote %s (%d lines)' % (HEADER, len(out)))
    return text


def verify(inst, jobs, quick):
    """With the table built in: every slur of the grid on both occasions."""
    warm(inst)
    info = INSTRUMENTS[inst]
    cells = [(a, a + s * iv, d) for a in range(info['lo'], info['hi'] + 1, 3 if quick else 1)
             for iv in INTERVALS for s in (1, -1) for d in DYNS if info['lo'] <= a + s * iv <= info['hi']]

    rs = pmap(job_both, [(inst, a, b, d, None, 'v%d' % i) for i, (a, b, d) in enumerate(cells)], jobs)
    rows = [dict(a=a, b=b, dyn=d, **r) for (a, b, d), r in zip(cells, rs)]
    played = [x for x in rows if not x.get('skip')]
    fails = [x for x in played if not x['ok']]
    print('%s: %d slurs, %d fail' % (inst, len(played), len(fails)))
    oks = [x for x in played if x['ok']]
    for k in ('settle', 'over', 'off', 'junk', 'dip', 'bump', 'hole'):
        v = np.array([x[k] for x in oks])
        print('  %-6s median %6.1f  90 %% %6.1f  max %6.1f' % (k, np.median(v), np.percentile(v, 90), v.max()))
    print('  cost   mean %.1f' % np.mean([min(FAIL, cost(x)) for x in played]))
    for x in sorted(played, key=lambda x: -cost(x))[:20]:
        print('  %-4s -> %-4s dyn %.2f  %s' % (name(x['a']), name(x['b']), x['dyn'], fmt(x)))
    return rows


def main(argv):
    jobs, quick, out, wav = 12, False, None, None
    args = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == '-j':
            i += 1
            jobs = min(15, int(argv[i]))
        elif a == '-quick':
            quick = True
        elif a == '-o':
            i += 1
            out = argv[i]
        elif a == '-w':
            i += 1
            wav = argv[i]
        else:
            args.append(a)
        i += 1
    if not args:
        print(__doc__)
        return 1
    cmd = args[0]
    insts = args[1:] or list(INSTRUMENTS)
    if cmd == 'survey':
        survey(args[1], jobs, quick, out)
    elif cmd == 'one':
        knobs = dict(kv.split('=') for kv in args[5:])
        r = slur(args[1], int(args[2]), int(args[3]), float(args[4]), knobs, keep=wav)
        print('%s -> %s dyn %s: %s' % (name(int(args[2])), name(int(args[3])), args[4], fmt(r)))
    elif cmd == 'practice':
        for inst in insts:
            practice(inst, jobs, quick)
    elif cmd == 'trills':
        for inst in insts:
            trills(inst, jobs, quick)
    elif cmd == 'emit':
        emit()
    elif cmd == 'verify':
        for inst in insts:
            verify(inst, jobs, quick)
    elif cmd == 'recalibrate':
        # The whole process. The practice plays with the table out of the way (its own trial
        # gestures); the trills and the check play the rebuilt table.
        def make():
            subprocess.run(['make', '-C', ROOT, 'pajupilli'], check=True, stdout=subprocess.DEVNULL)
            _warm.clear()
            global _pool
            if _pool is not None:  # (the workers remember which notes speak: of the old binary)
                _pool.shutdown()
                _pool = None
        for inst in insts:
            practice(inst, jobs, quick)
        emit()
        make()
        for inst in insts:
            trills(inst, jobs, quick)
        emit()
        make()
        for inst in insts:
            verify(inst, jobs, quick)
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
