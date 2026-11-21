#!/usr/bin/env python3
"""Where the time goes, instrument by instrument: each General MIDI program plays the same short
passage (three-note chords, then a run) on one thread under `perf record`; the table is the cost
of each (CPU seconds per second of sound) and its hottest functions, then the functions summed
over the set, weighted by the CPU time.

  python3 -I tools/speedprofile.py                 # harp, strings, choir, piano, organ + 20 random
  python3 -I tools/speedprofile.py --seed 7        # another 20
  python3 -I tools/speedprofile.py --programs 0,46 --random 0
  python3 -I tools/speedprofile.py --bin ./pajupilli --tag after   # (keeps the runs apart)

The fixed five: grand piano (0), harp (46), string ensemble (48), choir aahs (52), church
organ (19). Programs are numbered from 0. --no-perf: the times only. Each is run once unmeasured
first, so that a tuning learnt on the first run (the calibration cache) is not in the numbers.
"""
import argparse, os, random, re, struct, subprocess, sys, time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
import synthcompare as sc
import gmcompare as gc

FIXED = [0, 46, 48, 52, 19]
WORK = os.path.join(ROOT, 'research', 'gm_compare', 'work', 'speed')
VEL = 88


def passage(p):
    """(time s, on/off, key) and the end: four held triads (1 s each), eight quick notes, a tail."""
    keys = sc.test_keys(p)
    n = len(keys); mid = n // 2
    pick = lambda i: keys[max(0, min(n - 1, i))]
    ev = []
    for c in range(4):
        t = c * 1.0
        for j in (-2, 0, 2):
            k = pick(mid + j + (c % 2))
            ev.append((t, 1, k)); ev.append((t + 0.95, 0, k))
    for i in range(8):
        t = 4.0 + i * 0.25; k = pick(mid - 3 + i)
        ev.append((t, 1, k)); ev.append((t + 0.22, 0, k))
    return ev, 8.0


def write_mid(path, p):
    ev, end = passage(p)
    tpq = 480; tick = lambda s: int(round(s * 2 * tpq))
    E = [(0, bytes([0xC0, p]))] + [(tick(t), bytes([0x90 if on else 0x80, k, VEL if on else 0])) for t, on, k in ev]
    E.sort(key=lambda e: (e[0], e[1][0] == 0x90))
    body = bytearray(); last = 0
    for t, e in E: body += sc._enc_vlq(t - last) + e; last = t
    body += sc._enc_vlq(tick(end) - last) + b'\xff\x2f\x00'
    open(path, 'wb').write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, tpq) + b'MTrk' + struct.pack('>I', len(body)) + bytes(body))


def one(p, a, work):
    mid = os.path.join(work, 'p%03d.mid' % p); wav = os.path.join(work, 'p%03d.wav' % p)
    data = os.path.join(work, 'p%03d.perf' % p)
    write_mid(mid, p)
    cmd = [a.bin, '-play', mid, '-o', wav]
    env = dict(os.environ, PJ_THREADS='1', PJ_VOICE_THREADS='1')
    if a.warm:  # (a first run, unmeasured: a tuning not yet in ~/.cache/pajupilli is learnt here)
        subprocess.run(cmd, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    if a.perf: cmd = ['perf', 'record', '-q', '-m', '8', '-F', str(a.freq), '-o', data] + cmd
    t0 = time.time()
    log = os.path.join(work, 'p%03d.log' % p)
    with open(log, 'w') as lf:
        pid = subprocess.Popen(cmd, env=env, stdout=lf, stderr=subprocess.STDOUT)
        _, st, ru = os.wait4(pid.pid, 0)
    cpu = ru.ru_utime + ru.ru_stime; wall = time.time() - t0
    txt = open(log, errors='replace').read()
    m = re.search(r'rendered ([\d.]+) s', txt)
    if not m or st != 0:
        return dict(p=p, err='failed (status %d): %s' % (st, txt.strip().splitlines()[-1][:80] if txt.strip() else ''))
    dur = float(m.group(1))
    if os.path.exists(wav): os.remove(wav)
    syms = []
    if a.perf:
        r = subprocess.run(['perf', 'report', '-i', data, '--stdio', '--no-children', '--sort', 'symbol', '-q',
                            '--percent-limit', '0.5'], capture_output=True, text=True)
        for ln in r.stdout.splitlines():
            m = re.match(r'\s*([\d.]+)%\s+\[(.)\]\s+(.*)', ln)
            if m: syms.append((float(m.group(1)), clean(m.group(3)) + (' [kernel]' if m.group(2) == 'k' else '')))
        os.remove(data)
    return dict(p=p, cpu=cpu, wall=wall, dur=dur, syms=syms)


def clean(s):
    """A perf symbol without its padding, namespace noise and (long) argument list."""
    s = re.split(r'\s{2,}', s.strip())[0]
    s = s.replace('(anonymous namespace)::', '').replace('pajupilli::', '')
    s = re.sub(r'@@GLIBC.*', '', s)
    if len(s) > 60: s = re.sub(r'\((?!\)).*', '(…)', s, count=1)
    return s


def short(s, n=86):
    return s if len(s) <= n else s[:n - 1] + '…'


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('--seed', type=int, default=1, help='the random set (default 1)')
    ap.add_argument('--random', type=int, default=20, help='how many random programs beside the fixed (default 20)')
    ap.add_argument('--programs', default=None, help='the fixed set, comma separated, from 0 (default 0,46,48,52,19)')
    ap.add_argument('--bin', default=os.path.join(ROOT, 'pajupilli'))
    ap.add_argument('--tag', default='run', help='the work directory under research/gm_compare/work/speed')
    ap.add_argument('--jobs', type=int, default=8, help='renders at once (each one thread; default 8)')
    ap.add_argument('--freq', type=int, default=2000, help='perf samples a second')
    ap.add_argument('--top', type=int, default=4, help='functions shown for each instrument')
    ap.add_argument('--all', type=int, default=40, help='functions shown in the sum')
    ap.add_argument('--no-perf', dest='perf', action='store_false')
    ap.add_argument('--no-warm', dest='warm', action='store_false', help='measure the first run (with any calibration)')
    ap.add_argument('-o', default=None, help='also write the table here')
    a = ap.parse_args()
    fixed = FIXED if a.programs is None else [int(x) for x in a.programs.split(',') if x != '']
    rest = [p for p in range(128) if p not in fixed]
    progs = fixed + sorted(random.Random(a.seed).sample(rest, min(a.random, len(rest))))
    work = os.path.join(WORK, a.tag); os.makedirs(work, exist_ok=True)
    names, _ = gc.pj_names(a.bin)
    with ThreadPoolExecutor(max(1, min(a.jobs, 15))) as ex:
        R = list(ex.map(lambda p: one(p, a, work), progs))
    out = []
    out.append('seed %d, %d programs, one thread each; cost = CPU s per s of sound (1.00 = a whole core)' % (a.seed, len(progs)))
    out.append('')
    out.append('%-4s %-26s %7s %7s  %s' % ('prog', 'instrument', 'cost', 'cpu s', 'hottest functions'))
    tot = {}; cpu_all = 0.0; dur_all = 0.0
    for r in sorted(R, key=lambda r: -r.get('cpu', 0) / max(r.get('dur', 1), 1e-9)):
        p = r['p']; nm = '%s (%s)' % (names.get(p, '?'), gc.GM[p])
        if 'err' in r:
            out.append('%-4d %-26s %s' % (p, nm[:26], r['err'])); continue
        cpu_all += r['cpu']; dur_all += r['dur']
        for pct, s in r['syms']: tot[s] = tot.get(s, 0.0) + pct / 100 * r['cpu']
        hot = r['syms'][:a.top]
        out.append('%-4d %-26s %7.2f %7.1f  %s' % (p, nm[:26], r['cpu'] / r['dur'], r['cpu'],
                                                  ('%4.1f%% %s' % (hot[0][0], short(hot[0][1], 70))) if hot else ''))
        for pct, s in hot[1:]:
            out.append('%-4s %-26s %7s %7s  %4.1f%% %s' % ('', '', '', '', pct, short(s, 70)))
    out.append('')
    out.append('all: %.1f CPU s for %.1f s of sound, mean cost %.2f' % (cpu_all, dur_all, cpu_all / max(dur_all, 1e-9)))
    if tot:
        out.append('')
        out.append('functions over the set (share of all CPU time):')
        for s, c in sorted(tot.items(), key=lambda kv: -kv[1])[:a.all]:
            out.append('  %5.1f%% %6.1f s  %s' % (100 * c / cpu_all, c, short(s, 100)))
    txt = '\n'.join(out) + '\n'
    sys.stdout.write(txt)
    if a.o: open(a.o, 'w').write(txt)


if __name__ == '__main__':
    main()
