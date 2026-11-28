#!/usr/bin/env python3
"""Velocity curves: how loud each key and instrument plays at each MIDI velocity, here and in
a sample-based General MIDI player (TiMidity++ with its sound font), A-weighted.

For each GM drum key (channel 10) or GM program (one note on channel 1), a MIDI file plays the
note at velocities 8, 16, ... 127, each alone (1.6 s apart), through `pajupilli -play -o` and
`timidity -Ow`; each note is measured as

  - LAFmax: the loudest A-weighted, fast (125 ms) level, dB;
  - LAeq:   the A-weighted mean level over its first 0.5 s, dB.

Absolute levels differ between the two players (gains, normalisation), so each curve is shown
relative to its own velocity-127 note: the shape and the dynamic range (127 against 16) are
what compare. Keys this program does not build come out silent here ("-").

    tools/velcurve.py drums                 every GM drum key 35..81
    tools/velcurve.py drums 38 42 51        some keys
    tools/velcurve.py programs              every GM program 1..128 (one key each)
    tools/velcurve.py programs 1 57 66      some programs (1-based)
    (-csv FILE: also write every number; -kit N: the drum kit's program, 1-based)
    tools/velcurve.py -emit-winds [57-62,65-68]  the winds (1-based programs) as whole parts at one
                                            velocity each (their -play reading): their rows only
    tools/velcurve.py -emit                 measure every program and drum key without the
                                            velocity gain (PJ_NOVELGAIN) and write
                                            src/velgain_data.h

The velocity gain (-play only): a sample player's loudness follows velocity as its curve says,
here the median of TiMidity++'s over every program and key, 33.3 log10(v / 127) dB; ours follows
from the physics (a hammer's speed, the breath) and is often much flatter (a reed: a few dB). Per
instrument, -emit writes the difference, 0 dB at velocity 90 (where tools/levels.py trims).
"""
import math
import re
import os
import struct
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

import numpy as np
from scipy.io import wavfile
from scipy.signal import bilinear, lfilter

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PAJUPILLI = os.path.join(ROOT, 'pajupilli')
TIMIDITY_CFG = '/etc/timidity/fluidr3_gm.cfg'
VELS = [8, 16, 24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 127]
GAP = 1.6  # s between notes
FS = 48000

DRUM_NAMES = {35: 'Acoustic Bass Drum', 36: 'Bass Drum 1', 37: 'Side Stick', 38: 'Acoustic Snare', 39: 'Hand Clap',
              40: 'Electric Snare', 41: 'Low Floor Tom', 42: 'Closed Hi-Hat', 43: 'High Floor Tom', 44: 'Pedal Hi-Hat',
              45: 'Low Tom', 46: 'Open Hi-Hat', 47: 'Low-Mid Tom', 48: 'Hi-Mid Tom', 49: 'Crash Cymbal 1', 50: 'High Tom',
              51: 'Ride Cymbal 1', 52: 'Chinese Cymbal', 53: 'Ride Bell', 54: 'Tambourine', 55: 'Splash Cymbal',
              56: 'Cowbell', 57: 'Crash Cymbal 2', 58: 'Vibraslap', 59: 'Ride Cymbal 2', 60: 'Hi Bongo', 61: 'Low Bongo',
              62: 'Mute Hi Conga', 63: 'Open Hi Conga', 64: 'Low Conga', 65: 'High Timbale', 66: 'Low Timbale',
              67: 'High Agogo', 68: 'Low Agogo', 69: 'Cabasa', 70: 'Maracas', 71: 'Short Whistle', 72: 'Long Whistle',
              73: 'Short Guiro', 74: 'Long Guiro', 75: 'Claves', 76: 'Hi Wood Block', 77: 'Low Wood Block',
              78: 'Mute Cuica', 79: 'Open Cuica', 80: 'Mute Triangle', 81: 'Open Triangle'}


def key_for_program(p):
    """A typical key for GM program p (0-based): the middle of the family's usual range."""
    fam = p // 8
    special = {32: 40, 33: 40, 34: 40, 35: 40, 36: 40, 37: 40, 38: 40, 39: 40, 42: 48, 43: 40, 57: 52, 58: 40,
               67: 48, 70: 48, 71: 60, 72: 84, 73: 74, 116: 60, 117: 60, 118: 60}
    if p in special:
        return special[p]
    return {1: 72, 3: 52, 9: 72}.get(fam, 60)


def var(n):
    out = [n & 0x7f]
    n >>= 7
    while n:
        out.insert(0, (n & 0x7f) | 0x80)
        n >>= 7
    return bytes(out)


def midi_file(path, channel, program, key, hold, kit=None):
    """Notes at VELS, GAP apart, from t = 0 (TiMidity skips leading silence: the first note at 0)."""
    q, tempo = 480, 500000  # a beat = 0.5 s
    tick = lambda s: int(round(s / 0.5 * q))
    ev = [(0, b'\xff\x51\x03' + tempo.to_bytes(3, 'big'))]
    if channel == 9 and kit is not None:
        ev.append((0, bytes([0xc9, kit])))
    if channel != 9:
        ev.append((0, bytes([0xc0 | channel, program])))
    for i, v in enumerate(VELS):
        t = i * GAP
        ev += [(tick(t), bytes([0x90 | channel, key, v])), (tick(t + hold), bytes([0x80 | channel, key, 0]))]
    ev.append((tick(len(VELS) * GAP + 0.5), b'\xff\x2f\x00'))
    ev.sort(key=lambda e: e[0])
    body, last = b'', 0
    for t, d in ev:
        body += var(t - last) + d
        last = t
    with open(path, 'wb') as f:
        f.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, q) + b'MTrk' + struct.pack('>I', len(body)) + body)


def a_weight(x, fs):
    """IEC 61672 A-weighting, as a bilinear-transformed analog filter."""
    f1, f2, f3, f4 = 20.598997, 107.65265, 737.86223, 12194.217
    a1000 = 1.9997
    num = [(2 * math.pi * f4) ** 2 * 10 ** (a1000 / 20), 0, 0, 0, 0]
    den = np.polymul([1, 4 * math.pi * f4, (2 * math.pi * f4) ** 2], [1, 4 * math.pi * f1, (2 * math.pi * f1) ** 2])
    den = np.polymul(np.polymul(den, [1, 2 * math.pi * f3]), [1, 2 * math.pi * f2])
    b, a = bilinear(num, den, fs)
    return lfilter(b, a, x)


def levels(wav, n):
    """Per note (onsets on the GAP grid from the first sound): LAFmax and LAeq over 0.5 s, dB."""
    fs, x = wavfile.read(wav)
    x = x.astype(float)
    x = (x.mean(1) if x.ndim > 1 else x) / (32768.0 if np.abs(x).max() > 2 else 1.0)
    xa = a_weight(x, fs)
    t0 = 0.0  # (both players start the first note at 0: the quietest may not be found by its level)
    # fast time weighting: exponential, 125 ms, on the squared signal
    alpha = 1 - math.exp(-1 / (0.125 * fs))
    sq = lfilter([alpha], [1, alpha - 1], xa * xa)
    out = []
    for i in range(n):
        a, b = int((t0 + i * GAP) * fs), int((t0 + i * GAP + GAP - 0.05) * fs)
        if b > len(x):
            out.append((None, None))
            continue
        fmax = 10 * math.log10(sq[a:b].max() + 1e-20)
        eq = 10 * math.log10(np.mean(xa[a:a + int(0.5 * fs)] ** 2) + 1e-20)
        out.append((fmax, eq) if fmax > -100 else (None, None))
    return out


def render(tmp, name, channel, program, key, hold, kit, raw=False, timidity=True):
    """Both players' levels, and the instrument -play chose (None: silent)."""
    mid = os.path.join(tmp, name + '.mid')
    midi_file(mid, channel, program, key, hold, kit)
    ours, theirs = os.path.join(tmp, name + '-p.wav'), os.path.join(tmp, name + '-t.wav')
    env = dict(os.environ, PJ_PLAY_TAIL='0.2')
    if raw:
        env['PJ_NOVELGAIN'] = '1'
    r = subprocess.run([PAJUPILLI, '-play', mid, '-room', 'dry', '-o', ours], capture_output=True, text=True, env=env)
    m = re.search(r'(?:channel|drums)\s+\d+: (\S+) \(', r.stderr)
    if timidity:
        subprocess.run(['timidity', '-c', TIMIDITY_CFG, '-Ow', '-s', str(FS), '-o', theirs, mid], capture_output=True)
    res = []
    for w in (ours, theirs):
        res.append(levels(w, len(VELS)) if os.path.exists(w) else [(None, None)] * len(VELS))
    return res + [m.group(1) if m else None]


def curve(lv, k):
    """Levels relative to the velocity-127 note (k: 0 LAFmax, 1 LAeq)."""
    top = lv[-1][k]
    return [None if (v[k] is None or top is None) else v[k] - top for v in lv]


def fmt(c):
    return ' '.join('   -' if v is None else '%4.0f' % v for v in c)


GM_SLOPE = 33.3  # dB per decade of velocity: TiMidity++ with FluidR3, the median curve


def gain_curve(lv):
    """dB to add at each of VELS: GM's curve less ours, both from velocity 90 (None: unmeasured).
    Ours is smoothed first: a quadratic in log velocity through 0 dB at 90 (a pluck's or a soft
    wind attack's chance is not the instrument's curve)."""
    pts = [(math.log10(v / 90), x[0]) for v, x in zip(VELS, lv) if x[0] is not None]
    if len(pts) < 6:
        return None
    u = np.array([p[0] for p in pts])
    y = np.array([p[1] for p in pts])
    a = np.stack([np.ones_like(u), u, u * u], 1)
    c = np.linalg.lstsq(a, y, rcond=None)[0]
    fit = lambda w: c[1] * w + c[2] * w * w  # (from 90)
    return [max(-40.0, min(30.0, GM_SLOPE * math.log10(v / 90) - fit(math.log10(v / 90)))) for v in VELS]


def emit():
    jobs = [(f'p{p}', 0, p, key_for_program(p), 1.0) for p in range(128)]
    jobs += [(f'd{k}', 9, 0, k, 0.3) for k in range(35, 82)]
    per = {}
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(8) as ex:
        futs = [ex.submit(render, tmp, *j, None, True, False) for j in jobs]
        for j, fu in zip(jobs, futs):
            ours, _, inst = fu.result()
            g = gain_curve(ours) if inst else None
            if g is not None:
                per.setdefault(inst, []).append(g)
    lines = ['// Generated by tools/velcurve.py -emit: per instrument, the gain (dB) at each velocity that puts',
             '// its response on General MIDI\'s curve (0 dB at velocity 90).',
             '#pragma once', 'namespace pajupilli {', 'constexpr int kVelGainN = %d;' % len(VELS),
             'constexpr int kVelGainVels[kVelGainN] = {%s};' % ', '.join(map(str, VELS)),
             'struct VelGain { const char* name; float db[kVelGainN]; };', 'constexpr VelGain kVelGains[] = {']
    for inst in sorted(per):
        g = np.median(np.array(per[inst]), 0)  # (several programs or keys on one instrument)
        lines.append('    {"%s", {%s}},' % (inst, ', '.join('%.1f' % x for x in g)))
        print(f'{inst:24s} ' + ' '.join('%5.1f' % x for x in g))
    lines += ['};', '}  // namespace pajupilli', '']
    open(os.path.join(ROOT, 'src', 'velgain_data.h'), 'w').write('\n'.join(lines))
    print('wrote src/velgain_data.h')


# A wind's part: -play's director reads a part's velocities as a line and accents (src/velline.h):
# the line goes into the part's level (its breath and PJ_DIRECTOR_WINDDB's gain), and the key is
# played at the part's own velocity plus the accent; the gain here is asked at that velocity. A
# whole part at one velocity is what it answers for, so a wind's own curve is measured that way:
# one file per velocity (no line to read), a few keys over the middle of its range, each twice.
WIND_KEYS = {56: (60, 67, 72), 57: (48, 55, 60), 58: (36, 43, 48), 59: (60, 67, 72), 60: (53, 60, 65),
             61: (55, 60, 67), 64: (66, 71, 78), 65: (61, 66, 73), 66: (54, 61, 66), 67: (43, 49, 54)}


def part_file(path, program, keys, v, hold=0.8, slot=1.6):
    q, tempo = 480, 500000
    tick = lambda s: int(round(s / 0.5 * q))
    ev = [(0, b'\xff\x51\x03' + tempo.to_bytes(3, 'big')), (0, bytes([0xc0, program]))]
    t = 0.0
    for _ in range(2):
        for k in keys:
            ev += [(tick(t), bytes([0x90, k, v])), (tick(t + hold), bytes([0x80, k, 0]))]
            t += slot
    ev.append((tick(t + 0.5), b'\xff\x2f\x00'))
    ev.sort(key=lambda e: e[0])
    body, last = b'', 0
    for tt, d in ev:
        body += var(tt - last) + d
        last = tt
    with open(path, 'wb') as f:
        f.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, q) + b'MTrk' + struct.pack('>I', len(body)) + body)
    return 2 * len(keys)


def part_level(tmp, program, keys, v, raw):
    """A part at velocity v: its notes' mean LAFmax (dB), and the instrument -play chose."""
    name = 'w%d_%d_%d' % (program, v, raw)
    mid, wav = os.path.join(tmp, name + '.mid'), os.path.join(tmp, name + '.wav')
    n = part_file(mid, program, keys, v)
    env = dict(os.environ, PJ_PLAY_TAIL='0.2', PJ_PLAY_VELMEDIAN='1000', PJ_THREADS='1')
    if raw:
        env['PJ_NOVELGAIN'] = '1'
    r = subprocess.run([PAJUPILLI, '-play', mid, '-room', 'dry', '-o', wav], capture_output=True, text=True, env=env)
    m = re.search(r'(?:channel|drums)\s+\d+: (\S+) \(', r.stderr)
    lv = levels(wav, n) if os.path.exists(wav) else []
    xs = [a for a, _ in lv if a is not None]
    return (float(np.mean(xs)) if xs else None), (m.group(1) if m else None)


def monotone(y):
    """The nearest non-decreasing sequence (pool adjacent violators), then a light 3-point smoothing."""
    blocks = [[float(v), 1] for v in y]
    i = 0
    while i < len(blocks) - 1:
        if blocks[i][0] > blocks[i + 1][0]:
            a, b = blocks[i], blocks[i + 1]
            blocks[i:i + 2] = [[(a[0] * a[1] + b[0] * b[1]) / (a[1] + b[1]), a[1] + b[1]]]
            i = max(0, i - 1)
        else:
            i += 1
    out = [v for v, w in blocks for _ in range(w)]
    sm = [out[0]] + [(out[k - 1] + 2 * out[k] + out[k + 1]) / 4 for k in range(1, len(out) - 1)] + [out[-1]]
    return sm


def wind_gain_curve(own):
    """dB to add at each of VELS: GM's curve less the part's own (monotone, from velocity 90)."""
    y = monotone(own)
    at90 = np.interp(math.log10(90), [math.log10(v) for v in VELS], y)
    return [max(-40.0, min(30.0, GM_SLOPE * math.log10(v / 90) - (o - at90))) for v, o in zip(VELS, y)]


def emit_winds(programs):
    """Measure the winds' parts (programs 0-based) without the velocity gain and write their rows of
    src/velgain_data.h, the other rows kept."""
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(12) as ex:
        futs = {(p, v): ex.submit(part_level, tmp, p, WIND_KEYS.get(p, (key_for_program(p) - 5, key_for_program(p),
                                                                       key_for_program(p) + 7)), v, True)
                for p in programs for v in VELS}
        per = {}
        for p in programs:
            res = [futs[(p, v)].result() for v in VELS]
            inst = res[0][1]
            if inst is None or any(r[0] is None for r in res):
                print(f'GM {p + 1}: not measured')
                continue
            own = [r[0] for r in res]
            g = wind_gain_curve(own)
            per[inst] = g
            at90 = np.interp(90, VELS, own)
            print(f'{inst:16s} own  ' + ' '.join('%5.1f' % (o - at90) for o in own))
            print(f'{"":16s} gain ' + ' '.join('%5.1f' % x for x in g))
    path = os.path.join(ROOT, 'src', 'velgain_data.h')
    lines = open(path).read().split('\n')
    for inst, g in per.items():
        row = '    {"%s", {%s}},  // (velcurve.py -emit-winds: a part at one velocity)' % (inst, ', '.join('%.1f' % x for x in g))
        k = [i for i, l in enumerate(lines) if l.strip().startswith('{"%s",' % inst)]
        if k:
            lines[k[0]] = row
        else:
            end = max(i for i, l in enumerate(lines) if l.strip().startswith('{"'))
            lines.insert(end + 1, row)
    open(path, 'w').write('\n'.join(lines))
    print('wrote src/velgain_data.h (%d rows)' % len(per))


def main():
    args = sys.argv[1:]
    if '-emit' in args:
        return emit()
    if '-emit-winds' in args:
        i = args.index('-emit-winds')
        sel = args[i + 1] if len(args) > i + 1 else '57-62,65-68'
        progs = []
        for part in sel.split(','):
            a, _, b = part.partition('-')
            progs += list(range(int(a) - 1, int(b or a)))
        return emit_winds(progs)
    csv = None
    kit = None
    if '-csv' in args:
        i = args.index('-csv')
        csv = args[i + 1]
        del args[i:i + 2]
    if '-kit' in args:
        i = args.index('-kit')
        kit = int(args[i + 1]) - 1
        del args[i:i + 2]
    what = args[0] if args else 'drums'
    sel = [int(a) for a in args[1:]]
    jobs = []
    if what == 'drums':
        for k in sel or range(35, 82):
            jobs.append((f'd{k}', 9, 0, k, 0.3, f'{k} {DRUM_NAMES.get(k, "")}'))
    else:
        for p in [s - 1 for s in sel] or range(128):
            k = key_for_program(p)
            jobs.append((f'p{p}', 0, p, k, 1.0, f'GM {p + 1:3d} key {k}'))
    rows = []
    with tempfile.TemporaryDirectory() as tmp, ThreadPoolExecutor(8) as ex:
        futs = [ex.submit(render, tmp, j[0], j[1], j[2], j[3], j[4], kit) for j in jobs]
        for j, fu in zip(jobs, futs):
            ours, theirs, _ = fu.result()
            rows.append((j[5], ours, theirs))
    print('A-weighted level relative to velocity 127 (dB), LAFmax; velocities: ' + ' '.join('%4d' % v for v in VELS))
    for name, ours, theirs in rows:
        co, ct = curve(ours, 0), curve(theirs, 0)
        rng = lambda c: (c[1] if c[1] is not None else float('nan'))
        print(f'{name:28s} pajupilli {fmt(co)}   range 16..127 {-rng(co):5.1f} dB')
        print(f'{"":28s} timidity  {fmt(ct)}   range 16..127 {-rng(ct):5.1f} dB')
        if ours[-1][0] is not None and theirs[-1][0] is not None:
            print(f'{"":28s} level at 127: pajupilli {ours[-1][0]:6.1f}, timidity {theirs[-1][0]:6.1f} dBFS(A)')
    if csv:
        with open(csv, 'w') as f:
            f.write('item,player,velocity,LAFmax,LAeq\n')
            for name, ours, theirs in rows:
                for pl, lv in (('pajupilli', ours), ('timidity', theirs)):
                    for v, (a, b) in zip(VELS, lv):
                        f.write(f'"{name}",{pl},{v},{"" if a is None else "%.2f" % a},{"" if b is None else "%.2f" % b}\n')


if __name__ == '__main__':
    main()
