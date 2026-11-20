#!/usr/bin/env python3
"""The attack matrix: every note of a horn or a reed started in many ways, and the state it
settles in. A wind instrument can hold more than one state on a fingering (the note, its octave
or twelfth, a partial beside it, a state a semitone or two flat), and which one sounds depends
on how the note was reached; the calibration plays each note from silence only.

Each note: tongued from silence with keys of 30, 64, 100 and 127, struck again after 60 ms, and
slurred from an octave, a fifth and a whole tone above and below. The pitch over the last 0.5 s
of 1.2 s, in cents from the note: "wrong" is more than 50 cents off or silent.

    tools/attacks.py [-v] [-j N] [INSTRUMENT ...] [PJ_KNOB=VALUE ...]

Default: the brass and the reeds. -v lists every start that is wrong or more than 10 cents off.
"""
import concurrent.futures, math, os, re, subprocess, sys, tempfile
import numpy as np
from scipy.io import wavfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
P = os.path.join(ROOT, 'pajupilli')
ALL = ('trumpet muted-trumpet trombone tuba horn soprano-sax alto-sax tenor-sax baritone-sax oboe english-horn '
       'bassoon clarinet duduk shawm bagpipe shehnai').split()
NAMES = 'C C# D D# E F F# G G# A Bb B'.split()


def pitch(x, fs, f0):
    """The first strong autocorrelation peak within 2.7x of f0 (Hz), and its height."""
    x=x-x.mean(); n=len(x)
    if np.sqrt(np.mean(x*x))<3e-4: return None,0
    X=np.fft.rfft(x*np.hanning(n),2*n); r=np.fft.irfft(np.abs(X)**2)[:n]
    r/=r[0]+1e-30
    a,b=max(2,int(fs/(f0*2.7))),min(n-2,int(fs/(f0/2.7)))
    mx=r[a:b].max()
    for k in range(a,b):
        if r[k]>0.88*mx and r[k]>=r[k-1] and r[k]>=r[k+1]:
            y0,y1,y2=r[k-1],r[k],r[k+1]; d=0.5*(y0-y2)/(y0-2*y1+y2+1e-30)
            return fs/(k+d),mx
    return None,mx


def matrix(inst, env):
    cal = subprocess.run([P, '-inst', inst, '-cal'], capture_output=True, text=True, env=env)
    notes = [int(x) for x in re.findall(r'MIDI (\d+)', cal.stdout + cal.stderr)]
    lo, hi = min(notes), max(notes)
    rows = []
    with tempfile.TemporaryDirectory() as d:
        wav = os.path.join(d, 'a.wav')
        for m in range(lo, hi + 1):
            f0 = 440 * 2 ** ((m - 69) / 12)
            cases = [(f't{v}', f'on {m} {v}, wait 1.2, off {m}, wait 0.1', 0.7) for v in (30, 64, 100, 127)]
            cases.append(('re', f'on {m} 80, wait 0.4, off {m}, wait 0.06, on {m} 80, wait 1.2, off {m}, wait 0.1', 1.16))
            for dd in (-12, 12, -7, 7, -2, 2):
                a = m + dd
                if lo <= a <= hi:
                    cases.append((f's{dd:+d}', f'on {a} 80, wait 0.5, on {m} 64, off {a}, wait 1.2, off {m}, wait 0.1', 1.2))
            row = []
            for name, seq, t0 in cases:
                subprocess.run([P, '-inst', inst, '-seq', seq, '-o', wav], capture_output=True, env=env)
                fs, x = wavfile.read(wav)
                isint = x.dtype.kind == 'i'
                x = x.astype(float)
                if x.ndim > 1:
                    x = x.mean(1)
                if isint:
                    x /= 32768.0
                p, _ = pitch(x[int(t0 * fs):int((t0 + 0.5) * fs)], fs, f0)
                row.append((name, None if p is None else round(1200 * math.log2(p / f0))))
            rows.append((m, row))
    return inst, rows


if __name__ == '__main__':
    args = sys.argv[1:]
    verbose = '-v' in args
    jobs = 12
    if '-j' in args:
        jobs = int(args[args.index('-j') + 1])
        del args[args.index('-j'):args.index('-j') + 2]
    env = dict(os.environ)
    insts = []
    for a in args:
        if a == '-v':
            continue
        if '=' in a:
            k, v = a.split('=', 1)
            env[k] = v
        else:
            insts.append(a)
    total = 0
    with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
        for inst, rows in ex.map(lambda i: matrix(i, env), insts or ALL):
            n = wrong = off = 0
            lines = []
            for m, row in rows:
                bad = []
                for name, c in row:
                    n += 1
                    if c is None or abs(c) > 50:
                        wrong += 1
                        bad.append(f'{name}:{"--" if c is None else format(c, "+d")}')
                    elif abs(c) > 10:
                        off += 1
                        bad.append(f'{name}:{c:+d}')
                if bad:
                    lines.append(f'   {NAMES[m % 12]}{m // 12 - 1} ({m}) ' + ' '.join(bad))
            total += wrong
            print(f'{inst}: {n} starts, {wrong} in a wrong state, {off} more than 10 cents off')
            if verbose:
                print('\n'.join(lines))
    print(f'{total} wrong')
