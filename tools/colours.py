#!/usr/bin/env python3
"""The syllable exercise: which vocal colours a brass or reed player can put into which note.

A wind player says syllables into the instrument: "da-ta-da-te-ti-da". The consonant is the
tongue (a soft "d", a hard "t"); the vowel is the mouth, shaped by the tongue into a resonance
(src/performer.h: setSyllable; src/horn.cc, src/reed.cc: the tract). Not every colour carries on
every note: a vowel is heard only where the mouth's resonance meets the instrument's own strongly
enough to move the tone, and a consonant only where the note speaks fast enough to show it.

Here every note plays the syllables at three dynamics, and the tool listens to what changed
against the player's own plain start ("-"):

  vowel      the held tone's colour: how far the harmonics' levels moved (dB, rms over the
             harmonics within 40 dB of the strongest, the overall level taken out), the brightness
             (the spectral centroid, %), the level (dB), the pitch (cents)
  consonant  the start: how long until the note is there (ms), its bark (dB over the held level)

A colour "carries" when it can be heard and the note is still the note:
  a vowel      moves the harmonics by 1 dB rms or more (and twice what the same tone differs
               from itself on another start), the pitch by under 12 c, and the note speaks;
  d against t  the starts differ by 8 ms or more, or their barks by 2 dB.

  tools/colours.py exercise INST [-o FILE.json]   every note, the table by register and dynamics
  tools/colours.py line INST KEY [-w FILE.wav]    "da ta da te ti da" on one note, for the ear

Options: -j N workers (default 8), -quick (every third note).
Run as: python3 -I tools/colours.py ...
"""
import json
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import slurs as S  # noqa: E402

VOWELS = ['u', 'o', 'a', 'e', 'i']
# ("-": the player's own start; "d", "t": the consonant alone, the mouth open; then each with each vowel)
SYLLABLES = ['-', 'd', 't'] + ['d' + v for v in VOWELS] + ['t' + v for v in VOWELS]
HOLD = 0.6
DYNS = [0.3, 0.5, 0.75]
OUT = os.path.join(S.ROOT, 'tools', 'colours')


def spectrum(x, fs, f0):
    """The held tone's harmonics (dB) and its centroid (Hz), from 0.3 s before the key's release."""
    a, b = int((HOLD - 0.32) * fs), int((HOLD - 0.02) * fs)
    seg = x[a:b] * np.hanning(b - a)
    sp = np.abs(np.fft.rfft(seg, 1 << 16))
    fr = np.fft.rfftfreq(1 << 16, 1.0 / fs)
    hs = []
    for k in range(1, 25):
        if k * f0 > 0.45 * fs or k * f0 > 9000:
            break
        m = (fr > (k - 0.3) * f0) & (fr < (k + 0.3) * f0)
        hs.append(20 * np.log10(sp[m].max() + 1e-12))
    p = sp * sp
    band = fr < 10000
    return np.array(hs), float((fr[band] * p[band]).sum() / (p[band].sum() + 1e-30))


def play(inst, m, dyn, syl, tag, keep=None):
    seq = 'syl %s on %d 64 wait %g off %d' % (syl, m, HOLD, m)
    fs, x = S.render(inst, seq, dyn, None, keep, tag)
    r = S.analyse_attack(x, fs, m, 0.0, HOLD)
    if not r['ok']:
        return dict(ok=False, why=r['why'])
    f0 = S.freq(m) * 2 ** (r['pb'] / 1200)
    hs, cen = spectrum(x, fs, f0)
    return dict(ok=True, speak=r['settle'], bark=r['bump'], cents=r['pb'], level=r['Lb'], hs=hs.tolist(), centroid=cen)


def job(w):
    inst, m, dyn, tag = w
    return {syl: play(inst, m, dyn, syl, tag) for syl in SYLLABLES}


def colour(a, b):
    """How far apart two held tones are: dB rms over the harmonics that matter."""
    ha, hb = np.array(a['hs']), np.array(b['hs'])
    n = min(len(ha), len(hb))
    ha, hb = ha[:n], hb[:n]
    use = (ha > ha.max() - 40) | (hb > hb.max() - 40)
    d = (hb - ha)[use]
    return float(np.sqrt(np.mean((d - d.mean()) ** 2))) if use.sum() > 1 else 0.0


def judge(cell):
    """One note at one dynamics: what each syllable did, and which colours carry."""
    ref = cell['d']
    out = dict(ok=ref['ok'] and cell['-']['ok'], vowels={}, consonant=None, floor=0.0)
    if not out['ok']:
        return out
    # (the floor: how far the same tone is from itself on another start, the breath's noise)
    floor = max(colour(ref, cell['-']), colour(ref, cell['t']) if cell['t']['ok'] else 0.0)
    out['floor'] = floor
    for v in VOWELS:
        ds = [cell[c + v] for c in 'dt']
        if not all(d['ok'] for d in ds):
            out['vowels'][v] = dict(carries=False, breaks=True)
            continue
        d = ds[0]
        col = colour(ref, d)
        dc = d['cents'] - ref['cents']
        out['vowels'][v] = dict(carries=bool(col >= max(1.0, 2 * floor) and abs(dc) < 12), colour=col, cents=dc,
                                bright=100 * (d['centroid'] / ref['centroid'] - 1), level=d['level'] - ref['level'], breaks=False)
    d, t = cell['d'], cell['t']
    if d['ok'] and t['ok']:
        out['consonant'] = dict(carries=bool(abs(d['speak'] - t['speak']) >= 8 or abs(d['bark'] - t['bark']) >= 2),
                                dspeak=d['speak'], tspeak=t['speak'], dbark=d['bark'], tbark=t['bark'])
    return out


def exercise(inst, jobs, quick, out):
    S.warm(inst)
    info = S.INSTRUMENTS[inst]
    cells = [(m, d) for m in range(info['lo'], info['hi'] + 1, 3 if quick else 1) for d in DYNS]
    rs = S.pmap(job, [(inst, m, d, 'c%d' % i) for i, (m, d) in enumerate(cells)], jobs)
    rows = [dict(key=m, dyn=d, **judge(r)) for (m, d), r in zip(cells, rs)]
    os.makedirs(OUT, exist_ok=True)
    path = out or os.path.join(OUT, inst + '.json')
    with open(path, 'w') as f:
        json.dump(dict(inst=inst, rows=rows), f, indent=1)
    # The table: by register (thirds of the range) and dynamics, the share of notes each colour
    # carries on, and how strongly.
    span = info['hi'] - info['lo'] + 1
    regs = [('low', info['lo'], info['lo'] + span // 3), ('middle', info['lo'] + span // 3, info['lo'] + 2 * span // 3),
            ('high', info['lo'] + 2 * span // 3, info['hi'] + 1)]
    print('%s: the colours that carry (share of the notes; the vowels\' colour in dB rms, median)' % inst)
    print('  %-7s %-5s %s   d/t' % ('', 'dyn', '   '.join('%-12s' % v for v in VOWELS)))
    for name, lo, hi in regs:
        for d in DYNS:
            rr = [r for r in rows if lo <= r['key'] < hi and r['dyn'] == d and r['ok']]
            if not rr:
                continue
            cols = []
            for v in VOWELS:
                vs = [r['vowels'][v] for r in rr]
                share = np.mean([x['carries'] for x in vs])
                med = np.median([x['colour'] for x in vs if not x['breaks']] or [0])
                brk = sum(x['breaks'] for x in vs)
                cols.append('%3.0f%% %4.1f dB%s' % (100 * share, med, '!' if brk else ' '))
            cs = [r['consonant'] for r in rr if r['consonant']]
            print('  %-7s %-5.2f %s   %3.0f%%' % (name, d, '   '.join(cols), 100 * np.mean([c['carries'] for c in cs]) if cs else 0))
    print('  (! : the vowel breaks some notes there: they do not speak, or leave the pitch)')
    print('  the same tone against itself on another start: %.2f dB (median)' % np.median([r['floor'] for r in rows if r['ok']] or [0]))
    ok = [r for r in rows if r['ok']]
    for v in VOWELS:
        vs = [(r['key'], r['dyn'], r['vowels'][v]) for r in ok if not r['vowels'][v]['breaks']]
        if vs:
            print('  %s: brightness %+.1f %% (median), level %+.1f dB, pitch %+.1f c; strongest on %s' % (
                v, np.median([x['bright'] for _, _, x in vs]), np.median([x['level'] for _, _, x in vs]),
                np.median([x['cents'] for _, _, x in vs]),
                ', '.join('%s (%.1f dB)' % (S.name(k), x['colour']) for k, _, x in sorted(vs, key=lambda t: -t[2]['colour'])[:3])))
    cs = [r['consonant'] for r in ok if r['consonant']]
    if cs:
        print('  d: speaks in %.0f ms, bark %.1f dB; t: %.0f ms, %.1f dB (medians)' % (
            np.median([c['dspeak'] for c in cs]), np.median([c['dbark'] for c in cs]),
            np.median([c['tspeak'] for c in cs]), np.median([c['tbark'] for c in cs])))
    print('wrote %s' % path)


def line(inst, key, wav):
    """da ta da te ti da on one note."""
    seq = ' '.join('syl %s on %d 64 wait 0.45 off %d wait 0.08' % (s, key, key) for s in ['da', 'ta', 'da', 'te', 'ti', 'da'])
    S.render(inst, seq, 0.5, None, wav, 'line')
    print('wrote %s: da ta da te ti da on %s' % (wav, S.name(key)))


def main(argv):
    jobs, quick, out, wav, args = 8, False, None, None, []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == '-j':
            i += 1
            jobs = int(argv[i])
        elif a == '-quick':
            quick = True
        elif a in ('-o', '-w'):
            i += 1
            out = wav = argv[i]
        else:
            args.append(a)
        i += 1
    if len(args) < 2:
        print(__doc__)
        return 1
    S.snapshot()
    if args[0] == 'exercise':
        for inst in args[1:]:
            exercise(inst, jobs, quick, out)
    elif args[0] == 'line':
        line(args[1], int(args[2]), wav or '%s-syllables.wav' % args[1])
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
