#!/usr/bin/env python3
"""The players' practice: how each slur and trill is best played (src/slur_data.h).

A wind player knows in advance what each slur needs: where the lips lead, where the air pushes
or eases, where the tongue touches. Here every player practises its slurs on the model, by
interval, register and dynamics, and the results are written as a table the players read
(src/slurs.h). The trills' fastest playable speeds are measured the same way.

  tools/slurs.py survey INST [-o FILE]   play the grid with the built-in table, print what fails
  tools/slurs.py practice INST           search the gestures, write tools/slurs/INST.json
  tools/slurs.py attacks INST           search every note's tongued start, add them to tools/slurs/INST.json
  tools/slurs.py verify-attacks INST     (after a rebuild) every start without and with the table
  tools/slurs.py trills INST             measure the trills, add them to tools/slurs/INST.json
  tools/slurs.py trillways INST          rehearse the trills' own way, each kind of trill (steady,
                                         quickening, growing, dying away, swelling, linked on), and
                                         measure how fast each goes; replaces `trills`
  tools/slurs.py emit                    rewrite src/slur_data.h from tools/slurs/*.json
  tools/slurs.py verify INST             (after a rebuild) play the grid again, compare with the json
  tools/slurs.py recalibrate [INST ...]  practice + attacks + trillways + emit + make + verify, all of it
  tools/slurs.py audition INST -o FILE   for the ear: what changed most, before and after, in one WAV
  tools/slurs.py one INST FROM TO DYN [NAME=VALUE ...]   one slur, its numbers (and -w FILE.wav)

Options: -j N workers (default 12), -quick (a coarser grid).
Run as: python3 -I tools/slurs.py ...
"""
import atexit
import json
import math
import os
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor

import numpy as np
from scipy.io import wavfile
from scipy.signal import medfilt, resample_poly

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# (the private copy's path goes through the environment: the workers are separate processes)
BIN = os.environ.get('PJ_PRACTICE_BIN') or os.path.join(ROOT, 'pajupilli')
WORK = os.path.join(ROOT, 'research', 'gm_compare', 'work', 'slur', 'tmp')
DATA = os.path.join(ROOT, 'tools', 'slurs')
HEADER = os.path.join(ROOT, 'src', 'slur_data.h')

# The instruments that practise: sounding range, and the gesture's numbers (src/slurs.h:
# the same order as SlurGesture's fields), each with its range for the search.
INSTRUMENTS = {
    'horn': dict(lo=41, hi=77, kind='lip'),
    'trumpet': dict(lo=52, hi=82, kind='lip'),
    # (a solo instrument whose slurs are the slide's: every interval practised, every slur a
    # little worse than the others rehearsed alone, the slide's speed from 0.3 x: slower can be
    # steadier, src/hornplayer.cc)
    'trombone': dict(lo=36, hi=77, kind='lip', intervals=list(range(1, 13)), thorough=True, fingers=(0.3, 2.5)),
    'tuba': dict(lo=28, hi=65, kind='lip'),
    'muted-trumpet': dict(lo=52, hi=82, kind='lip'),
    'alto-sax': dict(lo=49, hi=78, kind='reed'),
    'tenor-sax': dict(lo=44, hi=73, kind='reed'),
    'soprano-sax': dict(lo=56, hi=84, kind='reed'),
    'baritone-sax': dict(lo=37, hi=66, kind='reed', intervals=list(range(1, 13)), thorough=True),
    'bassoon': dict(lo=34, hi=63, kind='reed'),
    'english-horn': dict(lo=51, hi=78, kind='reed'),
    'clarinet': dict(lo=50, hi=82, kind='reed'),
    'oboe': dict(lo=58, hi=85, kind='reed'),
    # A transverse flute (src/player.cc): lip and dip move the jet's shape together, the flutist's
    # way into another register (going up the lips forward, the opening narrower, the aim deeper;
    # coming down the reverse); air, a pressure push or ease, only a helper (it costs a little).
    'baroque-flute': dict(lo=62, hi=93, kind='jet'),
    'flute': dict(lo=60, hi=96, kind='jet', survey_only=True),  # (not practised: its register kick only)
    # The bowed strings (src/bassinst.cc: BowPlayer): the stopped range on the bow.
    # A slur: one stroke, the left hand alone or the bow crossing to the next string; the
    # gesture's numbers read for the bow (src/bassinst.cc: BowPlayer::slurWay).
    'contrabass': dict(lo=28, hi=60, kind='bow'),
    'cello': dict(lo=36, hi=76, kind='bow'),
    'viola': dict(lo=48, hi=84, kind='bow'),
    'violin': dict(lo=55, hi=91, kind='bow'),
}
# (a flute's slur: the new note as loud, once settled, as the same note tongued alone; and among
# equally clean ways the one blown least differently: ms of cost per dB of level step past 1 dB,
# per unit of |air|)
JET_LEVEL_MS = 8.0
JET_AIR_MS = 30.0
# (a soft "d" is for a wide leap that will not slur; on a step it is not legato: ms per ms of tongue)
JET_TONGUE_MS = {True: 20.0, False: 4.0}  # (interval under a fourth: True)
DYNS = [0.3, 0.5, 0.75]
INTERVALS = [1, 2, 3, 4, 5, 7, 9, 12]
HOLD = 0.6  # s on each note
# The two speeds a slur is practised at (src/slurs.cc: kSlow, kQuick): out of a settled note, and
# out of one that has only just spoken (a run's notes: the lips and the air are still arriving).
SLOW, QUICK = 0.6, 0.1
SPEEDS = [SLOW, QUICK]
# (the new note as loud, once settled, as tongued alone: ms of cost per dB of step past 1.5 dB)
LEVEL_MS = 5.0
# (among equally clean ways the plainest: ms of cost per unit of each number away from the plain way)
EFFORT_MS = dict(air=6.0, aperture=10.0, firm=6.0, focus=1.0, vowel=6.0)


def quick_pre(a):
    """How long the note left sounds in a quick slur: ~10 periods (a tuba's note has not spoken
    in 0.1 s), 0.09 .. 0.2 s."""
    return min(0.2, max(0.09, 10.0 / freq(a)))


def freq(m):
    return 440.0 * 2 ** ((m - 69) / 12)


def snapshot():
    """Play a private copy of the binary: another build of the tree (a peer's make) must not swap
    it, or take it away, in the middle of an hour's practice."""
    global BIN
    import shutil
    os.makedirs(WORK, exist_ok=True)
    dst = os.path.join(WORK, 'pajupilli-practice-%d' % os.getpid())
    for k in range(60):  # (a build in progress: the binary is briefly not there)
        try:
            shutil.copy2(os.path.join(ROOT, 'pajupilli'), dst + '.new')
            break
        except OSError:
            time.sleep(2)
    os.replace(dst + '.new', dst)
    BIN = dst
    os.environ['PJ_PRACTICE_BIN'] = dst
    atexit.register(lambda: os.path.exists(dst) and os.remove(dst))


def render(inst, seq, dyn, knobs=None, keep=None, tag='x'):
    """Play key events; returns (fs, samples)."""
    os.makedirs(WORK, exist_ok=True)
    path = keep or os.path.join(WORK, '%d_%s.wav' % (os.getpid(), tag))
    env = dict(os.environ)
    env['PJ_WANDER'] = '0'
    env['PJ_THREADS'] = '12'  # (a calibration after a rebuild: no more workers than this)
    if INSTRUMENTS.get(inst, {}).get('kind') == 'bow':
        env['PJ_PIZZ_MS'] = '0'  # (the cello's keys wait 100 ms to know a pizzicato: not here)
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


def analyse(x, fs, a, b, ton, hold=HOLD, ref=None):
    """A slur from note a to note b at ton (s): what happened. ref: the settled level of the note
    left when it was too short to settle here (a quick slur)."""
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
    sa = (0.0, ref, 5.0, 1.0) if ref is not None else steady(ton - min(0.3, hold - 0.1), ton - 0.02, fa)
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
            + 6.0 * max(0.0, r['bump'] - 2) + 1.0 * r['hole'] + 50.0 * r['off']
            + JET_LEVEL_MS * max(0.0, abs(r.get('lstep', 0.0)) - 1.0) + JET_AIR_MS * r.get('airUse', 0.0)
            + r.get('tongueCost', 0.0)
            + LEVEL_MS * max(0.0, abs(r.get('lstepAll', 0.0)) - 1.5) + r.get('effort', 0.0)
            + TUNE_MS * max(0.0, r.get('tuneMiss', 0.0) - 5.0))


def slur(inst, a, b, dyn, knobs=None, keep=None, tag='x', vel=64, hold=HOLD, pre=None, tune=(None, None)):
    """pre: how long the note left sounds (None: as long as the new one). tune: the cents asked
    of the note left and of the new one by ear (None: the ear off)."""
    pre = hold if pre is None else pre
    ta, tb = tune
    seq = '%son %d %d wait %g %son %d %d off %d wait %g off %d' % (
        'tune %g ' % ta if ta is not None else '', a, vel, pre, 'tune %g ' % tb if tb is not None else '', b, vel, a, hold, b)
    fs, x = render(inst, seq, dyn, knobs, keep, tag)
    ref = None
    if pre < 0.3:
        ref = tongued_level(inst, a, dyn, vel, tag)
        if ref is None:
            return dict(ok=False, skip=True, why='the note left does not speak tongued', settle=1e3 * hold)
    r = analyse(x, fs, a, b, pre, hold, ref)
    if r['ok'] and tb is not None:
        r['tuneMiss'] = abs(r['pb'] - tb)
    if not r['ok'] and not r.get('skip') and not speaks(inst, b, dyn, vel, tag):
        r['skip'] = True  # the note is not there tongued either: not a matter for the slur
        r['why'] = 'the note does not speak tongued'
    if r['ok'] and INSTRUMENTS.get(inst, {}).get('kind') == 'jet':
        lt = tongued_level(inst, b, dyn, vel, tag)
        if lt is not None:
            r['lstep'] = float(r['Lb'] - lt)
        r['airUse'] = abs(float((knobs or {}).get('PJ_SLURTRY_AIR', 0.0)))
        r['tongueCost'] = 1e3 * float((knobs or {}).get('PJ_SLURTRY_TONGUE', 0.0)) * JET_TONGUE_MS[abs(b - a) < 5]
    elif r['ok']:
        lt = tongued_level(inst, b, dyn, vel, tag)
        if lt is not None:
            r['lstepAll'] = float(r['Lb'] - lt)
        r['effort'] = sum(ms * abs(float((knobs or {}).get('PJ_SLURTRY_' + n.upper(), 0.0))) for n, ms in EFFORT_MS.items())
    return r


_tlev = {}


def tongued_level(inst, m, dyn, vel, tag):
    """The note's settled level (dB, as analyse measures it) when tongued alone."""
    key = (inst, m, round(dyn, 3), vel)
    if key not in _tlev:
        fs, x = render(inst, 'on %d %d wait %g off %d' % (m, vel, HOLD + 0.1, m), dyn, None, tag=tag + 'l')
        t, f, cl, lev = track(x, fs, freq(m) / 1.35, min(freq(m) * 3.3, 0.45 * fs))
        w = (t > HOLD - 0.3) & (t < HOLD + 0.08)
        _tlev[key] = float(np.median(lev[w])) if w.sum() >= 5 else None
    return _tlev[key]


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
        for iv in intervals(inst):
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
    return 'settle %3.0f ms  over %4.1f c  off %4.2f c s  junk %3.0f ms  dip %4.1f dB  bump %4.1f dB  hole %3.0f ms%s  cost %4.0f' % (
        r['settle'], r['over'], r['off'], r['junk'], r['dip'], r['bump'], r['hole'],
        '  step %+4.1f dB' % r['lstep'] if 'lstep' in r else '', cost(r))


_pool = None


def pmap(fn, items, jobs):
    """fn: a function of this module (the workers are processes: the analysis is Python)."""
    global _pool
    if jobs <= 1:
        return [fn(x) for x in items]
    if _pool is None:
        _pool = ProcessPoolExecutor(max_workers=jobs)
    return list(_pool.map(fn, items, chunksize=1))


# Interval training (TUNED): in an ensemble a note is seldom at equal temperament: the ear
# (the band's pull, the section's just intervals) asks it up to a dozen cents either way, and
# keeps trimming it. The plain occasions play at 12-TET with the ear off, as the calibration
# does; the tuned ones ask the note left 12 c high and the new one 12 c low, and the reverse:
# the interval narrowed and widened by 24 c. A gesture must land the slur there too, on the
# pitch asked (TUNE_MS a cent of miss past 5 c).
TUNE_C = 12.0
TUNE_MS = 3.0
TUNED_KINDS = ('lip', 'reed', 'bow')


def occasions(a, speed, robust, inst=None):
    """(pre, hold, dynamics offset, key velocity, wander, tuning of the note left, of the new
    one): a gesture must work on more than one occasion: another length and touch, the player's
    own unrest (the breath and the lips wander; the practice is otherwise on a steady player),
    and the notes tuned by ear off equal temperament (None: the ear off)."""
    if speed >= SLOW:
        o = [(HOLD, HOLD, 0.0, 64, 0, None, None), (0.4, 0.4, 0.04, 70, 0, None, None), (0.5, 0.5, -0.03, 60, 1, None, None)]
        t = (HOLD, HOLD)
    else:
        q = quick_pre(a)
        o = [(q, 0.45, 0.0, 64, 0, None, None), (1.5 * q, 0.4, 0.04, 70, 0, None, None), (1.2 * q, 0.45, -0.03, 60, 1, None, None)]
        t = (q, 0.45)
    # (PJ_PRACTICE_TUNED=0: equal temperament alone, to tell the tuned occasions' failures from the rest)
    if robust and inst is not None and INSTRUMENTS[inst]['kind'] in TUNED_KINDS and inst != 'contrabass' and os.environ.get('PJ_PRACTICE_TUNED', '1') != '0':
        o += [(t[0], t[1], 0.0, 64, 0, TUNE_C, -TUNE_C), (t[0], t[1], 0.0, 64, 0, -TUNE_C, TUNE_C)]
    return o if robust else o[:1]


def play(inst, a, b, d, knobs, occ, tag):
    pre, hold, dd, vel, wander, ta, tb = occ
    k = dict(knobs or {})
    if wander:
        k['PJ_WANDER'] = wander
    return slur(inst, a, b, min(1.0, max(0.0, d + dd)), k, tag=tag, vel=vel, hold=hold, pre=pre, tune=(ta, tb))


def job_slur(w):
    """(inst, a, b, dyn, knobs, occasion, tag) -> the slur's numbers."""
    inst, a, b, dyn, knobs, occ, tag = w
    return play(inst, a, b, dyn, knobs, occ, tag)


def job_both(w):
    """A slur on all its occasions: the worst of them."""
    inst, a, b, d, speed, knobs, tag = w
    rs = [play(inst, a, b, d, knobs, occ, tag) for occ in occasions(a, speed, True, inst)]
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
    rs = pmap(job_slur, [(inst, a, b, d, knobs, occasions(a, SLOW, False)[0], str(i)) for i, (a, b, d) in enumerate(cells)], jobs)
    rows = []
    for (a, b, d), r in zip(cells, rs):
        rows.append(dict(a=a, b=b, dyn=d, **r))
    fails = [x for x in rows if not x['ok'] and not x.get('skip')]
    oks = [x for x in rows if x['ok']]
    print('%s: %d slurs, %d fail, %d skipped' % (inst, len(rows), len(fails), sum(1 for x in rows if x.get('skip'))))
    if oks:
        for k in ('settle', 'over', 'off', 'junk', 'dip', 'bump', 'hole', 'lstep'):
            v = np.array([abs(x[k]) for x in oks if k in x])
            if len(v):
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
    # The embouchure's set through the change (src/slurs.h), x e^(-(t / setT)^2): a narrower or
    # wider lip opening (brass), firmer or looser lips, a focused breath (less turbulence), the
    # tongue shaping the mouth into a resonance (a vowel) tuned vowelAt octaves above the new note.
    ('aperture', -0.6, 0.4, ('+', 0.15)),
    ('firm', -0.8, 0.8, ('+', 0.25)),
    ('focus', 0.0, 2.0, ('+', 0.5)),
    ('vowel', 0.0, 1.0, ('+', 0.25)),
    ('vowelAt', -0.5, 2.0, ('+', 0.25)),
    ('setT', 0.02, 0.25, ('x', 1.8)),
]
PLAIN = dict(fingers=1, lead=0, air=0, airT=0.03, lip=0, lipT=0.03, dip=0, dipT=0.08, tongue=0, floor=0,
             aperture=0, firm=0, focus=0, vowel=0, vowelAt=0, setT=0.06)
# Who searches which of the set's numbers (a reed has no lip opening of its own; a flute's jet
# has its own ways: src/player.cc).
SET_PARAMS = {'lip': {'aperture', 'firm', 'focus', 'vowel', 'vowelAt', 'setT'},
              'reed': {'firm', 'focus', 'vowel', 'vowelAt', 'setT'},
              'jet': set(), 'bow': {'firm', 'setT'}}
# (the numbers a kind has no use for)
UNUSED = {'bow': {'lead', 'tongue', 'floor'}}
SET_ALL = {'aperture', 'firm', 'focus', 'vowel', 'vowelAt', 'setT'}


def searched(inst):
    info = INSTRUMENTS[inst]
    allowed = SET_PARAMS[info['kind']]
    unused = UNUSED.get(info['kind'], set())
    out = [p for p in PARAMS if (p[0] not in SET_ALL or p[0] in allowed) and p[0] not in unused]
    if 'fingers' in info:
        out = [('fingers', info['fingers'][0], info['fingers'][1], p[3]) if p[0] == 'fingers' else p for p in out]
    return out


def intervals(inst):
    return INSTRUMENTS[inst].get('intervals', INTERVALS)
# The horn's way before the practice (src/hornplayer.cc's knobs): where its search starts.
START = {
    'horn': {1: dict(PLAIN, lip=0.3, air=0.5), -1: dict(PLAIN, lip=0.3, air=0.3, dip=0.18)},
    'trumpet': {1: dict(PLAIN, lip=0.3, air=0.5), -1: dict(PLAIN, lip=0.3, air=0.3, dip=0.18)},
    'trombone': {1: dict(PLAIN, lip=0.3, air=0.5), -1: dict(PLAIN, lip=0.3, air=0.3, dip=0.18)},
    'tuba': {1: dict(PLAIN, lip=0.3, air=0.5), -1: dict(PLAIN, lip=0.3, air=0.3, dip=0.18)},
    'muted-trumpet': {1: dict(PLAIN, lip=0.3, air=0.5), -1: dict(PLAIN, lip=0.3, air=0.3, dip=0.18)},
    'alto-sax': {1: dict(PLAIN), -1: dict(PLAIN)},
    'tenor-sax': {1: dict(PLAIN), -1: dict(PLAIN)},
    'soprano-sax': {1: dict(PLAIN), -1: dict(PLAIN)},
    'baritone-sax': {1: dict(PLAIN), -1: dict(PLAIN)},
    'bassoon': {1: dict(PLAIN), -1: dict(PLAIN)},
    'english-horn': {1: dict(PLAIN), -1: dict(PLAIN)},
    'contrabass': {1: dict(PLAIN), -1: dict(PLAIN)},
    'cello': {1: dict(PLAIN), -1: dict(PLAIN)},
    'viola': {1: dict(PLAIN), -1: dict(PLAIN)},
    'violin': {1: dict(PLAIN), -1: dict(PLAIN)},
    'clarinet': {1: dict(PLAIN), -1: dict(PLAIN)},
    'oboe': {1: dict(PLAIN), -1: dict(PLAIN)},
    'baroque-flute': {1: dict(PLAIN), -1: dict(PLAIN)},
}
FAIL = 600.0  # a failed slur's cost in a mean (ms)
ASTILL = 400.0  # a practised start costing this much in its mean is left out of the table


def knobs_of(g):
    k = {'PJ_SLURTRY': 1}
    for name, _, _, _ in PARAMS:
        k['PJ_SLURTRY_' + name.upper()] = g.get(name, PLAIN[name])
    return k


def moves(g, scale, inst):
    """The gesture with one number stepped up or down."""
    out = []
    g = dict(PLAIN, **g)
    for name, lo, hi, (kind, step) in searched(inst):
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


class Searcher:
    def __init__(self, inst, jobs):
        self.inst, self.jobs, self.n = inst, jobs, 0
        self.memo = {}

    def costs(self, gestures, cells, robust):
        """Mean cost of each gesture over the cells (a, b, dyn, speed)."""
        work = []
        for gi, g in enumerate(gestures):
            for (a, b, d, sp) in cells:
                for occ in occasions(a, sp, robust, self.inst):
                    work.append((gi, a, b, d, occ))

        keys = [(a, b, d, occ, tuple(sorted(gestures[gi].items()))) for (gi, a, b, d, occ) in work]
        todo = sorted(set(k for k in keys if k not in self.memo), key=repr)
        rs = pmap(job_slur, [(self.inst, k[0], k[1], k[2], knobs_of(dict(k[4])), k[3], 's%d' % i)
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
        g = dict(PLAIN, **start)
        best = self.costs([g], cells, robust)[0]
        for scale in scales:
            cand = moves(g, scale, self.inst)
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


def seeds(inst, g):
    """Other techniques to try a hard slur with, beside its general way: one step alone cannot
    find what only helps together (a vowel and where it is tuned), or far from the start."""
    kind = INSTRUMENTS[inst]['kind']
    out = []
    if kind == 'jet':
        return out
    if kind == 'bow':  # more or less pressure, a push of the bow, a quicker hand, a slower crossing
        return [dict(g, **d) for d in (dict(lip=0.3), dict(lip=-0.2), dict(air=0.35), dict(air=-0.3), dict(fingers=1.8),
                                       dict(firm=0.5, setT=0.12), dict(firm=-0.5, setT=0.03), dict(lip=0.3, air=0.35, dip=-0.12))]
    for d in (dict(firm=0.5), dict(firm=-0.5), dict(vowel=1.0, vowelAt=0.5), dict(vowel=1.0, vowelAt=1.5),
              dict(vowel=1.0, vowelAt=0.0, firm=0.5), dict(tongue=0.005, floor=0.3), dict(air=0.0, lip=0.0, dip=0.0)):
        out.append(dict(g, **d))
    if kind == 'lip':
        out += [dict(g, aperture=-0.3), dict(g, aperture=0.3), dict(g, aperture=-0.3, firm=-0.5)]
    return out


def practice(inst, jobs, quick):
    warm(inst)
    S = Searcher(inst, jobs)
    info = INSTRUMENTS[inst]
    general = []
    # 1. The general way with each interval. Out of a settled note first: mezzo-forte (each
    #    interval starting from the one before), then soft and loud from there. Then the same
    #    slur out of a note that has only just spoken, starting from the settled way.
    for sign in (1, -1):
        prev = dict(PLAIN, **START[inst][sign])
        for iv in intervals(inst):
            cells = [(a, a + sign * iv, 0.5, SLOW) for a in anchors(inst, sign * iv, 6 if quick else 3)]
            if not cells:
                continue
            c0 = S.costs([prev], cells, False)[0]
            g, c = S.search(prev, cells, [1, 1, 0.5, 0.5, 0.25] if not quick else [1, 0.5], False)
            prev = g
            general.append(dict(interval=sign * iv, dyn=0.5, pre=SLOW, g=g, cost=c))
            print('%s %+3d mf: %5.0f -> %5.0f  %s' % (inst, sign * iv, c0, c, show(g)), flush=True)
            by_dyn = {0.5: g}
            for d in ([] if quick else [0.3, 0.75]):
                cd = [(a, b, d, SLOW) for (a, b, _, _) in cells]
                c0 = S.costs([g], cd, False)[0]
                gd, c = S.search(g, cd, [0.5, 0.5, 0.25], False)
                by_dyn[d] = gd
                general.append(dict(interval=sign * iv, dyn=d, pre=SLOW, g=gd, cost=c))
                print('%s %+3d %.2f: %5.0f -> %5.0f  %s' % (inst, sign * iv, d, c0, c, show(gd)), flush=True)
            for d in ([0.5] if quick else DYNS):
                cq = [(a, b, d, QUICK) for (a, b, _, _) in cells]
                c0 = S.costs([by_dyn[d]], cq, False)[0]
                gq, c = S.search(by_dyn[d], cq, [1, 0.5, 0.5, 0.25] if not quick else [1, 0.5], False)
                general.append(dict(interval=sign * iv, dyn=d, pre=QUICK, g=gq, cost=c))
                print('%s %+3d %.2f quick: %5.0f -> %5.0f  %s' % (inst, sign * iv, d, c0, c, show(gq)), flush=True)
    gen = {(r['interval'], r['dyn'], r['pre']): r for r in general}
    # 2. Every slur with its general way, on all its occasions: which need a way of their own?
    cells = []
    for a in range(info['lo'], info['hi'] + 1, 2 if quick else 1):
        for iv in intervals(inst):
            for sign in (1, -1):
                b = a + sign * iv
                if info['lo'] <= b <= info['hi']:
                    for d in ([0.5] if quick else DYNS):
                        for sp in SPEEDS:
                            cells.append((a, b, d, sp))

    rs = pmap(job_both, [(inst, a, b, d, sp, knobs_of(gen[(b - a, d, sp)]['g']), 'p%d' % i)
                         for i, (a, b, d, sp) in enumerate(cells)], jobs)
    base = [None if r.get('skip') else min(FAIL, cost(r)) for r in rs]
    own = []
    hard = []
    # (typical: what the general way costs across the instrument on the same occasions, the median)
    groups = {}
    for (a, b, d, sp), c in zip(cells, base):
        if c is not None:
            groups.setdefault((b - a, d, sp), []).append(c)
    for (a, b, d, sp), c in zip(cells, base):
        if c is None:
            continue
        typical = float(np.median(groups[(b - a, d, sp)]))
        thorough = info.get('thorough')
        if c >= FAIL or c > (max(1.2 * typical, typical + 15) if thorough else max(1.6 * typical, typical + 50)):
            hard.append((a, b, d, sp, c))
    print('%s: %d slurs, %d need their own way' % (inst, len(cells), len(hard)), flush=True)
    # 3. Those practise alone (on all occasions; the worst counts), from the best of: the general
    #    way, a neighbour's own way (the table stays smooth), and other techniques (seeds).
    S2 = Searcher(inst, jobs)
    for (a, b, d, sp, c0) in hard:
        g0 = gen[(b - a, d, sp)]['g']
        S2.memo = {}
        near = [o['g'] for o in own if o['interval'] == b - a and o['pre'] == sp and abs(o['from'] - a) <= 2 and not o['fails']][-1:]
        other = [o['g'] for o in own if o['from'] == a and o['interval'] == b - a and o['dyn'] == d and not o['fails']][-1:]
        starts = [g0] + near + other
        cs = S2.costs(starts, [(a, b, d, sp)], True)
        if min(cs) >= 0.5 * FAIL or info.get('thorough'):  # (failing or nearly: other techniques too)
            starts += seeds(inst, g0)
            if info['kind'] == 'lip' and 'fingers' in info:  # (the slide slower, and slower still)
                starts += [dict(g0, fingers=0.6), dict(g0, fingers=0.35), dict(g0, fingers=0.6, air=0.0), dict(g0, fingers=0.35, lip=0.0, dip=0.0)]
            cs += S2.costs(starts[len(cs):], [(a, b, d, sp)], True)
        k = int(np.argmin(cs))
        g, c = S2.search(starts[k], [(a, b, d, sp)], [1, 1, 0.5, 0.25] if info.get('thorough') else [1, 0.5, 0.25], True)
        fails = c >= 0.7 * FAIL
        if fails or c < 0.8 * cs[0]:
            own.append({'from': a, 'interval': b - a, 'dyn': d, 'pre': sp, 'g': g, 'cost': c, 'before': cs[0], 'fails': fails})
        print('%s %-4s -> %-4s %.2f%s: %5.0f -> %5.0f %s %s' % (inst, name(a), name(b), d, ' quick' if sp < SLOW else '', cs[0], c,
                                                                'FAILS' if fails else '', show(g)), flush=True)
    os.makedirs(DATA, exist_ok=True)
    path = os.path.join(DATA, inst + '.json')
    old = {}
    if os.path.exists(path):
        with open(path) as f:
            old = json.load(f)
    data = dict(old, inst=inst, general=general, own=own, trills=old.get('trills', []), renders=S.n + S2.n + 3 * len(cells))
    with open(path, 'w') as f:
        json.dump(data, f, indent=1)
    print('%s: %d renders, wrote %s' % (inst, data['renders'], path))


def show(g):
    return ' '.join('%s %g' % (n, g.get(n, PLAIN[n])) for n, _, _, _ in PARAMS if abs(g.get(n, PLAIN[n]) - PLAIN[n]) > 1e-9)


# ---- Attacks ----
# A tongued start (src/slurs.h: AttackGesture), per note and dynamics: the tongue's release and
# the breath's rise (x the player's own), a push of the breath, the lips off the note at first,
# and the embouchure's set as a slur's.
APARAMS = [
    ('release', 0.3, 4.0, ('x', 1.8)),
    ('rise', 0.3, 5.0, ('x', 1.8)),
    ('push', -0.3, 1.0, ('+', 0.2)),
    ('pushT', 0.015, 0.2, ('x', 1.8)),
    ('lip', -0.08, 0.08, ('+', 0.02)),
    ('lipT', 0.01, 0.15, ('x', 2.0)),
    ('aperture', -0.6, 0.4, ('+', 0.15)),
    ('firm', -0.8, 0.8, ('+', 0.25)),
    ('vowel', 0.0, 1.0, ('+', 0.25)),
    ('vowelAt', -0.5, 2.0, ('+', 0.25)),
    ('setT', 0.02, 0.25, ('x', 1.8)),
]
APLAIN = dict(release=1, rise=1, push=0, pushT=0.05, lip=0, lipT=0.04, aperture=0, firm=0, vowel=0, vowelAt=0, setT=0.06)
AHOLD = 0.5


def aparams(inst):
    kind = INSTRUMENTS[inst]['kind']
    out = []
    for p in APARAMS:
        if p[0] == 'aperture' and kind != 'lip':
            continue
        if p[0] == 'lip' and kind == 'reed':  # (the lip force on the reed: a wider range than a horn's lips' pitch)
            p = ('lip', -0.4, 0.4, ('+', 0.1))
        out.append(p)
    return out


def aknobs(g):
    k = {'PJ_ATTACKTRY': 1}
    for name, _, _, _ in APARAMS:
        k['PJ_ATTACKTRY_' + name.upper()] = g.get(name, APLAIN[name])
    return k


def analyse_attack(x, fs, m, t0, hold=AHOLD):
    """A tongued note from t0 (s): how it speaks."""
    fm = freq(m)
    i0, i1 = max(0, int((t0 - 0.02) * fs)), min(len(x), int((t0 + hold + 0.03) * fs))
    t, f, cl, lev = track(x[i0:i1], fs, fm / 1.35, min(fm * 3.3, 0.45 * fs))
    t = t + i0 / fs - t0
    res = dict(ok=False, settle=1e3 * hold, over=0.0, junk=0.0, bump=0.0, off=0.0, why='')
    w = (t >= hold - 0.25) & (t <= hold - 0.02)
    good = w & (cl > 0.8)
    if w.sum() < 5 or good.sum() < 0.7 * w.sum():
        res['why'] = 'not there at the end'
        return res
    c_end = cents(f[good], fm)
    pb, Lb = float(np.median(c_end)), float(np.median(lev[w]))
    if abs(pb) > 40:
        res['why'] = 'ends %+.0f c' % pb
        return res
    sway = float(np.percentile(c_end, 90) - np.percentile(c_end, 10)) / 2
    lsway = float(np.percentile(lev[w], 90) - np.percentile(lev[w], 10)) / 2
    mm = (t >= 0) & (t <= hold - 0.02)
    tt, c, clm, lv = t[mm], medfilt(cents(f[mm], fm), 7), cl[mm], lev[mm]
    tol = max(12.0, 2.0 * sway)
    there = (np.abs(c - pb) < tol) & (clm > 0.75) & (lv > Lb - 3.0 - lsway)
    bad = np.nonzero(~there)[0]
    speak = 0.0 if len(bad) == 0 else tt[min(bad[-1] + 1, len(tt) - 1)]
    if len(bad) and bad[-1] >= len(tt) - 3:
        res['why'] = 'never settles'
        return res
    hop = float(tt[1] - tt[0]) if len(tt) > 1 else 0.002
    voiced = (clm > 0.85) & (lv > Lb - 15)
    dev = np.abs(c - pb)
    res.update(ok=True, settle=1e3 * speak,
               over=float(dev[voiced & (dev < 150) & (tt <= speak + 0.02)].max(initial=0.0)),
               # (junk: another partial sounding first, a crack)
               junk=1e3 * hop * int((voiced & (dev >= 150)).sum()),
               bump=float(max(0.0, lv.max() - Lb)),
               off=float(hop * np.minimum(60.0, np.maximum(0.0, dev - 5.0))[(tt > 0.03) & (tt < 0.35) & (clm > 0.7)].sum()),
               pb=pb, Lb=Lb)
    return res


def acost(r):
    if r.get('skip'):
        return 0.0
    if not r['ok']:
        return 1000.0
    return (r['settle'] + 3.0 * max(0.0, r['over'] - 5) + 4.0 * r['junk'] + 6.0 * max(0.0, r['bump'] - 4)
            + 50.0 * r['off'] + r.get('effort', 0.0) + TUNE_MS * max(0.0, r.get('tuneMiss', 0.0) - 5.0))


AEFFORT_MS = dict(push=6.0, aperture=10.0, firm=6.0, vowel=6.0)
# (key velocity, wander, re-tongued after a short rest, the cents asked by ear or None: the ear
# off): a start must work on more than one occasion. The tuned ones: in an ensemble the ear asks
# the note TUNE_C either side of equal temperament from its first moment, and a start practised
# only at the instrument's own pitch can scoop or crack there. They are scored also on landing
# the pitch asked (TUNE_MS a cent of miss past 5 c), plain and re-tongued (the lips coming from
# the note just played are not where a first note's are).
AOCC = [(64, 0, False, None), (100, 0, False, None), (64, 1, False, None), (64, 0, True, None),
        (64, 0, False, TUNE_C), (64, 0, False, -TUNE_C), (64, 0, True, TUNE_C), (64, 0, True, -TUNE_C)]


def attack(inst, m, dyn, knobs, occ, tag='a', keep=None):
    vel, wander, again, tune = occ
    tu = 'tune %g ' % tune if tune is not None else ''
    k = dict(knobs or {})
    if wander:
        k['PJ_WANDER'] = wander
    if again:
        seq, t0 = '%son %d %d wait 0.3 off %d wait 0.12 on %d %d wait %g off %d' % (tu, m, vel, m, m, vel, AHOLD, m), 0.42
    else:
        seq, t0 = '%son %d %d wait %g off %d' % (tu, m, vel, AHOLD, m), 0.0
    fs, x = render(inst, seq, dyn, k, keep, tag)
    r = analyse_attack(x, fs, m, t0)
    if r['ok'] and tune is not None:
        r['tuneMiss'] = abs(r['pb'] - tune)
    if r['ok']:
        r['effort'] = (sum(ms * abs(float(k.get('PJ_ATTACKTRY_' + n.upper(), 0.0))) for n, ms in AEFFORT_MS.items())
                       + 4.0 * abs(math.log(float(k.get('PJ_ATTACKTRY_RELEASE', 1.0))))
                       + 4.0 * abs(math.log(float(k.get('PJ_ATTACKTRY_RISE', 1.0)))))
    return r


def job_attack(w):
    inst, m, dyn, knobs, occ, tag = w
    return attack(inst, m, dyn, knobs, occ, tag)


def afmt(r):
    if not r['ok']:
        return 'FAIL (%s)' % r['why']
    return 'speaks %3.0f ms  scoop %4.1f c  off %4.2f c s  crack %3.0f ms  bump %4.1f dB  cost %4.0f' % (
        r['settle'], r['over'], r['off'], r['junk'], r['bump'], acost(r))


class AttackSearcher:
    def __init__(self, inst, jobs):
        self.inst, self.jobs, self.n, self.memo = inst, jobs, 0, {}

    def costs(self, gestures, m, d):
        keys = [(gi, occ, tuple(sorted(g.items()))) for gi, g in enumerate(gestures) for occ in AOCC]
        todo = sorted(set((k[1], k[2]) for k in keys if (k[1], k[2]) not in self.memo), key=repr)
        rs = pmap(job_attack, [(self.inst, m, d, aknobs(dict(g)), occ, 'a%d' % i) for i, (occ, g) in enumerate(todo)], self.jobs)
        for k, r in zip(todo, rs):
            self.memo[k] = min(FAIL, acost(r))
        self.n += len(todo)
        tot = [[] for _ in gestures]
        for gi, occ, g in keys:
            tot[gi].append(self.memo[(occ, g)])
        return [float(np.mean(t) * 0.6 + np.max(t) * 0.4) for t in tot]

    def moves(self, g, scale):
        out = []
        for name, lo, hi, (kind, step) in aparams(self.inst):
            for sign in (1, -1):
                v = g[name] * step ** (sign * scale) if kind == 'x' else g[name] + sign * step * scale
                v = min(hi, max(lo, v))
                if abs(v - g[name]) > 1e-9:
                    out.append(dict(g, **{name: round(v, 5)}))
        return out

    def search(self, starts, m, d, scales):
        self.memo = {}
        cs = self.costs(starts, m, d)
        k = int(np.argmin(cs))
        g, best, plain = dict(starts[k]), cs[k], cs[0]
        for scale in scales:
            cand = self.moves(g, scale)
            cc = self.costs(cand, m, d)
            better = sorted([(c, i) for i, c in enumerate(cc) if c < best - 0.5])
            if not better:
                continue
            combo, seen = dict(g), set()
            for c, i in better:
                nm = [n for n in cand[i] if cand[i][n] != g[n]][0]
                if nm not in seen:
                    seen.add(nm)
                    combo[nm] = cand[i][nm]
            c2 = self.costs([combo], m, d)[0] if len(seen) > 1 else 1e9
            if c2 < better[0][0]:
                g, best = combo, c2
            else:
                best, g = better[0][0], cand[better[0][1]]
        return g, best, plain


def aseeds(inst, g):
    kind = INSTRUMENTS[inst]['kind']
    out = [dict(g, rise=2.0), dict(g, rise=0.5, release=0.6), dict(g, push=0.4), dict(g, firm=0.5), dict(g, firm=-0.5),
           dict(g, vowel=1.0, vowelAt=0.5), dict(g, vowel=1.0, vowelAt=1.5),
           dict(g, lip=(-0.2 if kind == 'reed' else -0.04)), dict(g, lip=(0.2 if kind == 'reed' else 0.04))]
    if kind == 'lip':
        out += [dict(g, aperture=-0.3), dict(g, aperture=0.3)]
    return out


def ashow(g):
    return ' '.join('%s %g' % (n, g[n]) for n, _, _, _ in APARAMS if abs(g[n] - APLAIN[n]) > 1e-9)


def attacks(inst, jobs, quick):
    """Every note's tongued start, by dynamics: up the instrument, each note starting from the
    best of the plain way, the note below's way, and other techniques."""
    if INSTRUMENTS[inst]['kind'] in ('jet', 'bow'):
        print('%s: its starts are its player\'s own (not practised here)' % inst)
        return
    warm(inst)
    info = INSTRUMENTS[inst]
    S = AttackSearcher(inst, jobs)
    rows = []
    try:  # (the ways practised before: each note also starts its search from its own)
        with open(os.path.join(DATA, inst + '.json')) as f:
            old = {(r['key'], r['dyn']): r['g'] for r in json.load(f).get('attacks', [])}
    except (OSError, ValueError):
        old = {}
    for d in ([0.5] if quick else DYNS):
        prev = None
        for m in range(info['lo'], info['hi'] + 1, 3 if quick else 1):
            plain = dict(APLAIN)
            mine = old.get((m, d))
            starts = [plain] + ([dict(plain, **mine)] if mine and mine != plain else []) + ([prev] if prev else []) + aseeds(inst, plain)
            g, c, c0 = S.search(starts, m, d, [1, 0.5] if quick else [1, 1, 0.5, 0.25])
            if c0 >= FAIL and c >= FAIL:
                print('%s %-4s %.2f: does not speak tongued' % (inst, name(m), d), flush=True)
                continue
            if c > 0.9 * c0:  # (no real gain: the plain way stays)
                g, c = plain, c0
            else:
                prev = g
            rows.append(dict(key=m, dyn=d, g=g, cost=c, before=c0))
            print('%s %-4s %.2f: %5.0f -> %5.0f  %s' % (inst, name(m), d, c0, c, ashow(g)), flush=True)
    path = os.path.join(DATA, inst + '.json')
    with open(path) as f:
        data = json.load(f)
    data['attacks'] = rows
    data['attack_renders'] = S.n
    with open(path, 'w') as f:
        json.dump(data, f, indent=1)
    b, a = np.mean([r['before'] for r in rows]), np.mean([r['cost'] for r in rows])
    print('%s: %d starts, mean cost %.0f -> %.0f, %d changed, %d renders' % (
        inst, len(rows), b, a, sum(1 for r in rows if r['g'] != APLAIN), S.n))


def verify_attacks(inst, jobs, quick):
    """With the table built in, and without it: every note's start on all its occasions."""
    if INSTRUMENTS[inst]['kind'] in ('jet', 'bow'):
        return
    warm(inst)
    info = INSTRUMENTS[inst]
    cells = [(m, d) for m in range(info['lo'], info['hi'] + 1, 3 if quick else 1) for d in DYNS]
    for label, knobs in (('before', {'PJ_ATTACKS': 0}), ('after', None)):
        work = [(inst, m, d, knobs, occ, 'va%d' % i) for i, (m, d) in enumerate(cells) for occ in AOCC]
        rs = pmap(job_attack, work, jobs)
        worst = {}
        for (_, m, d, _, occ, _), r in zip(work, rs):
            if (m, d) not in worst or acost(r) > acost(worst[(m, d)]):
                worst[(m, d)] = r
        v = list(worst.values())
        oks = [r for r in v if r['ok']]
        print('%s starts %-6s: %d notes x dynamics, %d fail, %d crack, speaks median %.0f ms (90 %%: %.0f), cost mean %.0f' % (
            inst, label, len(v), len(v) - len(oks), sum(1 for r in oks if r['junk'] > 6),
            np.median([r['settle'] for r in oks] or [0]), np.percentile([r['settle'] for r in oks] or [0], 90),
            np.mean([min(FAIL, acost(r)) for r in v])))
        if label == 'after':
            for (m, d), r in sorted(worst.items(), key=lambda kv: -acost(kv[1]))[:8]:
                print('  %-4s dyn %.2f  %s' % (name(m), d, afmt(r)))


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


# ---- The trills' own way ----
# A trill is not a row of slurs: the embouchure is held between the two notes and the fingers
# alone move. One gesture for both directions, rehearsed as trills are played, each kind apart:
TRILL_KINDS = ['steady', 'accel', 'cresc', 'decresc', 'swell50', 'swell75', 'linked']
# (the three the search plays, standing for the rest: an even one, a quickening one, one that
# grows and dies away; every kind is rehearsed for the notes that need their own way, and measured)
TRILL_CORE = ['steady', 'accel', 'swell50']
TRILL_SPAN = [1, 2, 3, 4]


def trill_events(lo, iv, rate, kind, info):
    """The key events of a trill of this kind at `rate` notes per second, and what to listen for:
    [(start s, length s, key)] of the notes after the first two, and the note after (linked)."""
    T = 1.0 / rate
    n = 16
    durs = [T] * n
    expr = [None] * n
    if kind == 'accel':  # steady at first, then quickening towards the end: 1.5 T .. 0.75 T
        durs = [1.5 * T] * 5 + [1.5 * T * (0.5 ** ((k + 1) / 11.0)) for k in range(11)]
    elif kind == 'cresc':
        expr = [0.35 + 0.35 * k / (n - 1) for k in range(n)]
    elif kind == 'decresc':
        expr = [0.70 - 0.35 * k / (n - 1) for k in range(n)]
    elif kind in ('swell50', 'swell75'):  # an emphasis half-way, or three quarters of the way in
        peak = (0.5 if kind == 'swell50' else 0.75) * (n - 1)
        expr = [0.4 + 0.35 * max(0.0, 1 - abs(k - peak) / (peak if k < peak else (n - 1 - peak))) for k in range(n)]
    seq = 'expr %g on %d 64 wait 0.35' % (expr[0] if expr[0] is not None else 0.5, lo)
    t = 0.35
    cur = lo
    segs = []
    for k in range(n):
        nxt = lo + iv if cur == lo else lo
        if expr[k] is not None:
            seq += ' expr %g' % expr[k]
        seq += ' on %d 64 off %d wait %g' % (nxt, cur, durs[k])
        if k >= 2:
            segs.append((t, durs[k], nxt))
        t += durs[k]
        cur = nxt
    after = None
    if kind == 'linked':  # musically linked: the turn under the main note, and on into the next
        under = lo - 1 if lo - 1 >= info['lo'] else None
        goal = lo + iv + 2 if lo + iv + 2 <= info['hi'] else lo - 3
        for m in ([under, lo] if under is not None else [lo]):
            if m == cur:
                continue
            seq += ' on %d 64 off %d wait %g' % (m, cur, T)
            segs.append((t, T, m))
            t += T
            cur = m
        seq += ' on %d 64 off %d wait 0.4' % (goal, cur)
        after = (t, 0.4, goal)
        t += 0.4
        cur = goal
    seq += ' off %d' % cur
    return seq, segs, after


def trill_play(inst, lo, iv, rate, kind, knobs, tag):
    """A trill's numbers: the share of its notes that do not arrive, how much of each note goes
    to arriving, how unequal the two notes are (dB), how far from the pitch (c)."""
    info = INSTRUMENTS[inst]
    seq, segs, after = trill_events(lo, iv, rate, kind, info)
    fs, x = render(inst, seq, 0.5, knobs, tag=tag)
    keys = [m for _, _, m in segs] + ([after[2]] if after else [])
    t, f, cl, lev = track(x, fs, freq(min(keys)) / 1.35, min(freq(max(keys)) * 3.3, 0.45 * fs), hop=0.001)
    ref = np.median(lev[(t > 0.2) & (t < 0.33)]) if ((t > 0.2) & (t < 0.33)).any() else -200.0
    miss, arr, levs, cs = 0, [], [], []
    for (t0, dur, m) in segs:
        w = (t >= t0) & (t <= t0 + dur)
        if w.sum() < 3:
            continue
        c = cents(f[w], freq(m))
        ok = (np.abs(c) < 30) & (cl[w] > 0.7) & (lev[w] > ref - 10)
        tw = t[w] - t0
        late = (tw >= 0.4 * dur)
        hit = late.sum() > 0 and ok[late].mean() >= 0.6
        if not hit:
            miss += 1
            arr.append(1.0)
            levs.append(None)
            continue
        bad = np.nonzero(~ok)[0]
        arr.append(0.0 if len(bad) == 0 else min(1.0, tw[min(bad[-1] + 1, len(tw) - 1)] / dur))
        levs.append(float(np.median(lev[w][late & ok])))
        cs.append(float(np.mean(np.abs(c[late & ok]))))
    n = max(1, len(arr))
    imb = [abs(levs[k] - 0.5 * (levs[k - 1] + levs[k + 1])) for k in range(1, len(levs) - 1)
           if levs[k] is not None and levs[k - 1] is not None and levs[k + 1] is not None]
    r = dict(miss=miss / n, arr=float(np.mean(arr)) if arr else 1.0, imb=float(np.mean(imb)) if imb else 0.0,
             cents=float(np.mean(cs)) if cs else 0.0, link=0.0)
    if after:
        t0, dur, m = after
        w = (t >= t0) & (t <= t0 + dur)
        c = cents(f[w], freq(m))
        ok = (np.abs(c) < 25) & (cl[w] > 0.75) & (lev[w] > ref - 10)
        bad = np.nonzero(~ok)[0]
        tw = t[w] - t0
        r['link'] = 400.0 if (len(tw) == 0 or (len(bad) and bad[-1] >= len(tw) - 5)) else (0.0 if len(bad) == 0 else 1e3 * tw[min(bad[-1] + 1, len(tw) - 1)])
    return r


def tcost(r):
    return 600.0 * r['miss'] + 200.0 * r['arr'] + 12.0 * max(0.0, r['imb'] - 1.0) + 1.5 * max(0.0, r['cents'] - 5.0) + 0.5 * r['link']


def job_trillway(w):
    inst, lo, iv, rate, kind, knobs, tag = w
    r = trill_play(inst, lo, iv, rate, kind, knobs, tag)
    r['effort'] = sum(ms * abs(float((knobs or {}).get('PJ_SLURTRY_' + n.upper(), 0.0))) for n, ms in EFFORT_MS.items())
    return r


def job_trill_limit(w):
    """The fastest rate each kind still speaks at (its notes arriving: 85 % of them)."""
    inst, lo, iv, knobs, kinds, tag = w
    out = {}
    for kind in kinds:
        best = 0
        for rate in TRILL_RATES:
            if trill_play(inst, lo, iv, rate, kind, knobs, tag)['miss'] > 0.15:
                break
            best = rate
        out[kind] = best
    return out


class TrillSearcher(Searcher):
    def tcosts(self, gestures, cells, kinds):
        """cells: (lo, iv, rate). Mean over the cells of each gesture's cost across the kinds
        (the worst kind counts extra)."""
        work = [(gi, lo, iv, rate, kind) for gi in range(len(gestures)) for (lo, iv, rate) in cells for kind in kinds]
        keys = [(lo, iv, rate, kind, tuple(sorted(gestures[gi].items()))) for (gi, lo, iv, rate, kind) in work]
        todo = sorted(set(k for k in keys if k not in self.memo), key=repr)
        rs = pmap(job_trillway, [(self.inst, k[0], k[1], k[2], k[3], knobs_of(dict(k[4])), 'w%d' % i) for i, k in enumerate(todo)], self.jobs)
        for k, r in zip(todo, rs):
            self.memo[k] = tcost(r) + r['effort']
        self.n += len(todo)
        per = {}
        for w, k in zip(work, keys):
            per.setdefault((w[0], w[1], w[2]), []).append(self.memo[k])
        tot = [[] for _ in gestures]
        for (gi, lo, iv), v in per.items():
            tot[gi].append(0.7 * float(np.mean(v)) + 0.3 * float(np.max(v)))
        return [float(np.mean(t)) if t else 0.0 for t in tot]

    def tsearch(self, start, cells, kinds, scales):
        g = dict(PLAIN, **start)
        best = self.tcosts([g], cells, kinds)[0]
        for scale in scales:
            cand = moves(g, scale, self.inst)
            cs = self.tcosts(cand, cells, kinds)
            better = sorted([(c, i) for i, c in enumerate(cs) if c < best - 0.5])
            if not better:
                continue
            combo, seen = dict(g), set()
            for c, i in better:
                nm = [n for n in cand[i] if cand[i][n] != g[n]][0]
                if nm not in seen:
                    seen.add(nm)
                    combo[nm] = cand[i][nm]
            cc = self.tcosts([combo], cells, kinds)[0] if len(seen) > 1 else 1e9
            if cc < better[0][0]:
                g, best = combo, cc
            else:
                best, g = better[0][0], cand[better[0][1]]
        return g, best


def trillways(inst, jobs, quick):
    """The trills' own gestures, and how fast each kind of trill can go with them."""
    if INSTRUMENTS[inst]['kind'] in ('jet', 'bow'):
        print('%s: its trills are its player\'s own (not practised here)' % inst)
        return
    warm(inst)
    info = INSTRUMENTS[inst]
    path = os.path.join(DATA, inst + '.json')
    with open(path) as f:
        data = json.load(f)
    S = TrillSearcher(inst, jobs)
    quickway = {r['interval']: r['g'] for r in data['general'] if r.get('pre', SLOW) == QUICK and r['dyn'] == 0.5}
    # 1. How fast the trills go as quick slurs (the way before): the rehearsal's speed is a step
    #    beyond what the middle of the instrument manages.
    notes = [lo for lo in range(info['lo'], info['hi'] - 1, 4 if quick else 1)]
    cells0 = [(lo, iv) for iv in TRILL_SPAN for lo in notes if lo + iv <= info['hi']]

    def way0(iv):
        return dict(PLAIN, **quickway.get(iv, quickway.get(min(quickway, key=lambda k: abs(k - iv)), PLAIN) if quickway else PLAIN))
    lim0 = pmap(job_trill_limit, [(inst, lo, iv, knobs_of(way0(iv)), ['steady'], 'l%d' % i) for i, (lo, iv) in enumerate(cells0)], jobs)
    before = {c: l['steady'] for c, l in zip(cells0, lim0)}
    general = []
    for iv in TRILL_SPAN:
        mine = [(lo, i2) for (lo, i2) in cells0 if i2 == iv]
        if not mine:
            continue
        med = float(np.median([before[c] for c in mine]))
        rate = min(14, max(6, ([r for r in TRILL_RATES if r > med] or [14])[0]))
        cells = [(lo, iv, rate) for (lo, _) in mine[::(1 if quick else 3)]]
        g0 = way0(iv)
        starts = [g0, dict(PLAIN), dict(PLAIN, fingers=1.8), dict(g0, lip=0.0, dip=0.0, air=0.0), dict(g0, firm=0.5), dict(g0, firm=-0.5)]
        cs = S.tcosts(starts, cells, TRILL_CORE)
        k = int(np.argmin(cs))
        g, c = S.tsearch(starts[k], cells, TRILL_CORE, [1, 0.5] if quick else [1, 1, 0.5, 0.25])
        general.append(dict(interval=iv, rate=rate, g=g, cost=c, before=cs[0]))
        print('%s trill +%d at %d/s: %5.0f -> %5.0f  %s' % (inst, iv, rate, cs[0], c, show(g)), flush=True)
    gen = {r['interval']: r for r in general}
    # 2. Every trill of a semitone and a tone with the general way, every kind: the ones far worse
    #    than the others rehearse alone.
    own = []
    cells = [(lo, iv) for (lo, iv) in cells0 if iv <= 2 and iv in gen]
    S2 = TrillSearcher(inst, jobs)
    rs = pmap(job_trillway, [(inst, lo, iv, gen[iv]['rate'], kind, knobs_of(gen[iv]['g']), 'b%d' % i)
                             for i, ((lo, iv), kind) in enumerate([(c, k) for c in cells for k in TRILL_KINDS])], jobs)
    per = {}
    for ((lo, iv), kind), r in zip([(c, k) for c in cells for k in TRILL_KINDS], rs):
        per.setdefault((lo, iv), []).append(tcost(r))
    worst = {c: 0.7 * float(np.mean(v)) + 0.3 * float(np.max(v)) for c, v in per.items()}
    for iv in (1, 2):
        mine = [c for c in cells if c[1] == iv]
        if not mine:
            continue
        typical = float(np.median([worst[c] for c in mine]))
        for (lo, _) in mine:
            c0 = worst[(lo, iv)]
            if c0 <= max(1.5 * typical, typical + 40):
                continue
            S2.memo = {}
            near = [o['g'] for o in own if o['interval'] == iv and abs(o['lo'] - lo) <= 2][-1:]
            starts = [gen[iv]['g']] + near + [dict(PLAIN), dict(gen[iv]['g'], firm=0.5), dict(gen[iv]['g'], firm=-0.5),
                                              dict(gen[iv]['g'], vowel=1.0, vowelAt=0.5), dict(gen[iv]['g'], aperture=-0.3)]
            cs = S2.tcosts(starts, [(lo, iv, gen[iv]['rate'])], TRILL_KINDS)
            k = int(np.argmin(cs))
            g, c = S2.tsearch(starts[k], [(lo, iv, gen[iv]['rate'])], TRILL_KINDS, [1, 0.5] if quick else [1, 0.5, 0.25])
            if c < 0.85 * cs[0]:
                own.append(dict(lo=lo, interval=iv, g=g, cost=c, before=cs[0]))
            print('%s trill %-4s +%d: %5.0f -> %5.0f  %s' % (inst, name(lo), iv, cs[0], c, show(g)), flush=True)
    # 3. How fast each kind goes now, per note.
    ownway = {(o['lo'], o['interval']): o['g'] for o in own}
    lim = pmap(job_trill_limit, [(inst, lo, iv, knobs_of(ownway.get((lo, iv), gen[iv]['g'])), TRILL_KINDS, 'm%d' % i)
                                 for i, (lo, iv) in enumerate(cells0) if iv in gen], jobs)
    rows = [dict(lo=lo, interval=iv, rate=l['steady'], before=before[(lo, iv)], kinds=l)
            for (lo, iv), l in zip([c for c in cells0 if c[1] in gen], lim)]
    with open(path) as f:  # (read again: another practice of this instrument may have written since)
        data = json.load(f)
    data['trillways'] = dict(general=general, own=own)
    data['trills'] = rows
    data['trill_renders'] = S.n + S2.n
    with open(path, 'w') as f:
        json.dump(data, f, indent=1)
    for iv in TRILL_SPAN:
        v = [r for r in rows if r['interval'] == iv]
        if v:
            print('%s trill +%d, notes per second (median over the notes): before %g, now %s' % (
                inst, iv, np.median([r['before'] for r in v]),
                '  '.join('%s %g' % (k, np.median([r['kinds'][k] for r in v])) for k in TRILL_KINDS)))


# ---- The table ----
def cfloat(v):
    t = '%g' % v
    return t + ('f' if ('.' in t or 'e' in t) else '.0f')


def emit(write=True):
    """Rewrites src/slur_data.h from the json; write=False: only returns its text."""
    out = ['// Written by tools/slurs.py emit from tools/slurs/*.json: do not edit (src/slurs.h).',
           '// {instrument, the key left (-1: the general way), interval, dynamics, the speed (s the',
           '//  note left had sounded: 0.6 settled, 0.1 only just spoken; 0: a trill\'s own way, the key',
           '//  its lower note, the interval up),',
           '//  {fingers, lead, air, airT, lip, lipT, dip, dipT, tongue, floor,',
           '//   aperture, firm, focus, vowel, vowelAt, setT, fails}}',
           'static const SlurRow kSlurRows[] = {']
    tr = []
    at = []

    def row(inst, frm, r):
        g = r['g']
        return '    {"%s", %d, %d, %s, %s, {%s, %s}},' % (
            inst, frm, r['interval'], cfloat(r['dyn']), cfloat(r.get('pre', SLOW)), ', '.join(cfloat(g.get(n, PLAIN[n])) for n, _, _, _ in PARAMS),
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
        for r in sorted(d['own'], key=lambda r: (r['from'], r['interval'], r['dyn'], -r.get('pre', SLOW))):
            out.append(row(inst, r['from'], r))
        tw = d.get('trillways', {})
        for r in tw.get('general', []):
            out.append(row(inst, -1, dict(interval=r['interval'], dyn=0.5, pre=0.0, g=r['g'])))
        for r in sorted(tw.get('own', []), key=lambda r: (r['lo'], r['interval'])):
            out.append(row(inst, r['lo'], dict(interval=r['interval'], dyn=0.5, pre=0.0, g=r['g'])))
        for r in d.get('attacks', []):
            # (a start that still fails on several of its occasions is no start to trust: built
            # in, the horn's A2 at mezzo-forte did not speak at all. The player's own start stays.)
            if r['cost'] >= ASTILL:
                continue
            at.append('    {"%s", %d, %s, {%s}},' % (inst, r['key'], cfloat(r['dyn']),
                                                   ', '.join(cfloat(r['g'].get(n, APLAIN[n])) for n, _, _, _ in APARAMS)))
        for r in d.get('trills', []):
            tr.append('    {"%s", %d, %d, %s},' % (inst, r['lo'], r['interval'], cfloat(r['rate'])))
    out.append('    {"", 0, 0, 0, 0, {}},')
    out.append('};')
    out.append('// {instrument, key, dynamics, {release, rise, push, pushT, lip, lipT, aperture, firm, vowel, vowelAt, setT}}')
    out.append('static const AttackRow kAttackRows[] = {')
    out += at
    out.append('    {"", 0, 0, {}},')
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
    cells = [(a, a + s * iv, d, sp) for a in range(info['lo'], info['hi'] + 1, 3 if quick else 1)
             for iv in intervals(inst) for s in (1, -1) for d in DYNS for sp in SPEEDS if info['lo'] <= a + s * iv <= info['hi']]

    rs = pmap(job_both, [(inst, a, b, d, sp, None, 'v%d' % i) for i, (a, b, d, sp) in enumerate(cells)], jobs)
    rows = [dict(a=a, b=b, dyn=d, pre=sp, **r) for (a, b, d, sp), r in zip(cells, rs)]
    played = [x for x in rows if not x.get('skip')]
    fails = [x for x in played if not x['ok']]
    print('%s: %d slurs, %d fail' % (inst, len(played), len(fails)))
    for sp, label in ((SLOW, 'settled'), (QUICK, 'quick')):
        ps = [x for x in played if x['pre'] == sp]
        if ps:
            print('  %-7s %4d slurs, %3d fail, cost mean %.1f, settle median %.0f ms' % (
                label, len(ps), sum(1 for x in ps if not x['ok']), np.mean([min(FAIL, cost(x)) for x in ps]),
                np.median([x['settle'] for x in ps if x['ok']] or [0])))
    oks = [x for x in played if x['ok']]
    for k in ('settle', 'over', 'off', 'junk', 'dip', 'bump', 'hole', 'lstep'):
        v = np.array([abs(x[k]) for x in oks if k in x])
        if len(v):
            print('  %-6s median %6.1f  90 %% %6.1f  max %6.1f' % (k, np.median(v), np.percentile(v, 90), v.max()))
    print('  cost   mean %.1f' % np.mean([min(FAIL, cost(x)) for x in played]))
    for x in sorted(played, key=lambda x: -cost(x))[:20]:
        print('  %-4s -> %-4s dyn %.2f%s  %s' % (name(x['a']), name(x['b']), x['dyn'], ' quick' if x['pre'] < SLOW else '      ', fmt(x)))
    return rows


def audition(inst, out):
    """For the ear: the slurs, starts and trills the practice changed most, each the way before
    (the tables off) and then as practised, in one file; prints what is where."""
    warm(inst)
    with open(os.path.join(DATA, inst + '.json')) as f:
        d = json.load(f)
    info = INSTRUMENTS[inst]
    off = {'PJ_SLURS': 0, 'PJ_ATTACKS': 0, 'PJ_TRILLWAYS': 0}
    items = []
    own = sorted([o for o in d.get('own', []) if not o.get('fails') and 'before' in o], key=lambda o: o['cost'] - o['before'])
    for sp, label in ((SLOW, 'slur'), (QUICK, 'quick slur')):
        for o in [o for o in own if o.get('pre', SLOW) == sp][:4]:
            a, b = o['from'], o['from'] + o['interval']
            pre = 0.6 if sp >= SLOW else quick_pre(a)
            items.append(('%s %s -> %s, dyn %.2f' % (label, name(a), name(b), o['dyn']),
                          'on %d 64 wait %g on %d 64 off %d wait 0.6 off %d' % (a, pre, b, a, b), o['dyn']))
    for r in sorted([r for r in d.get('attacks', []) if 'before' in r], key=lambda r: r['cost'] - r['before'])[:4]:
        items.append(('start %s, dyn %.2f' % (name(r['key']), r['dyn']), 'on %d 64 wait 0.5 off %d' % (r['key'], r['key']), r['dyn']))
    tw = d.get('trillways', {}).get('general', [])
    mid = (info['lo'] + info['hi']) // 2
    for r in tw[:2]:
        for kind in ('steady', 'accel', 'swell75', 'linked'):
            seq, _, _ = trill_events(mid, r['interval'], r['rate'], kind, info)
            items.append(('trill %s +%d, %s, %d notes/s' % (name(mid), r['interval'], kind, r['rate']), seq, 0.5))
    parts, t, fs0 = [], 0.0, None
    for label, seq, dyn in items:
        for which, knobs in (('before', off), ('after', None)):
            fs, x = render(inst, seq, dyn, knobs, tag='aud')
            fs0 = fs0 or fs
            print('%7.2f s  %-6s %s' % (t, which, label))
            parts += [x, np.zeros(int(0.35 * fs))]
            t += len(x) / fs + 0.35
        parts.append(np.zeros(int(0.5 * fs0)))
        t += 0.5
    if parts:
        y = np.concatenate(parts)
        wavfile.write(out, fs0, (y / max(1e-9, np.abs(y).max()) * 0.8 * 32767).astype(np.int16))
        print('wrote %s (%.0f s)' % (out, t))


def main(argv):
    jobs, quick, out, wav = 12, False, None, None
    args = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == '-j':
            i += 1
            jobs = min(28, int(argv[i]))
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
    insts = args[1:] or [k for k, v in INSTRUMENTS.items() if not v.get('survey_only')]
    snapshot()
    if cmd == 'survey':
        survey(args[1], jobs, quick, out)
    elif cmd == 'one':
        knobs = dict(kv.split('=') for kv in args[5:])
        r = slur(args[1], int(args[2]), int(args[3]), float(args[4]), knobs, keep=wav)
        print('%s -> %s dyn %s: %s' % (name(int(args[2])), name(int(args[3])), args[4], fmt(r)))
    elif cmd == 'practice':
        for inst in insts:
            practice(inst, jobs, quick)
    elif cmd == 'attacks':
        for inst in insts:
            attacks(inst, jobs, quick)
    elif cmd == 'verify-attacks':
        for inst in insts:
            verify_attacks(inst, jobs, quick)
    elif cmd == 'trills':
        for inst in insts:
            trills(inst, jobs, quick)
    elif cmd == 'trillways':
        for inst in insts:
            trillways(inst, jobs, quick)
    elif cmd == 'emit':
        emit()
    elif cmd == 'audition':
        audition(args[1], out or (args[1] + '-audition.wav'))
    elif cmd == 'verify':
        for inst in insts:
            verify(inst, jobs, quick)
    elif cmd == 'recalibrate':
        # The whole process. The practice plays with the table out of the way (its own trial
        # gestures); the trills and the check play the rebuilt table.
        def make():
            subprocess.run(['make', '-C', ROOT, 'pajupilli'], check=True, stdout=subprocess.DEVNULL)
            snapshot()
            _warm.clear()
            global _pool
            if _pool is not None:  # (the workers remember which notes speak: of the old binary)
                _pool.shutdown()
                _pool = None
        for inst in insts:
            practice(inst, jobs, quick)
            attacks(inst, jobs, quick)
            trillways(inst, jobs, quick)
        emit()
        make()
        for inst in insts:
            verify(inst, jobs, quick)
            verify_attacks(inst, jobs, quick)
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
