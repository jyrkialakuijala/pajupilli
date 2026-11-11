#!/usr/bin/env python3
"""make test: fast checks that lock in the model's behaviour (PLAN.md step 1).

Run from the repo root after `make flute scalar`. Each check prints ok/FAIL;
exit status 1 if any check fails. "info" lines report but do not fail (they
become checks once the planned fix lands).
"""
import math, os, re, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FLUTE = os.path.join(ROOT, 'flute')
SCALAR = os.path.join(ROOT, 'flute-scalar')
failures = []

def run(args, binary=FLUTE, env=None):
    e = dict(os.environ)
    e.update(env or {})
    return subprocess.run([binary] + args, capture_output=True, text=True, env=e, cwd=ROOT)

def check(name, ok, detail=''):
    print(f"{'ok  ' if ok else 'FAIL'}  {name}{': ' + detail if detail else ''}", flush=True)
    if not ok:
        failures.append(name)

def info(name, detail):
    print(f"info  {name}: {detail}", flush=True)

def vectorised():
    """Both per-sample loops of Bore::step use 512-bit FMAs."""
    dis = subprocess.run(['objdump', '-d', '--no-show-raw-insn', '-C', FLUTE],
                         capture_output=True, text=True).stdout
    m = re.search(r'<pajupilli::Bore::step\(.*?\)>:\n(.*?)\n\n', dis, re.S)
    if not m:
        check('Bore::step is AVX-512', False, 'symbol not found (inlined?)')
        return
    body = m.group(1)
    fma = len(re.findall(r'vf(?:n?m(?:add|sub))\w*\s+.*zmm', body))
    check('Bore::step is AVX-512', fma >= 8, f'{fma} zmm FMAs (the 8-vector loops stay rolled)')

def fuzz():
    for seed in (1, 2, 3):
        r = run(['-fuzz', '60', str(seed)])
        check(f'fuzz 60 s seed {seed}', r.returncode == 0, r.stdout.strip())

def nan_recovery():
    r = run(['-nantest'])
    check('NaN recovery', r.returncode == 0, r.stdout.strip())

def tuning():
    """Every note sounds in its register; C4..D#6 within 5 cents (3 after PLAN step 3.5), E6..C7 within 35."""
    r = run(['-cal'])
    bad, worst_lo, worst_hi = [], 0.0, 0.0
    for line in r.stdout.splitlines():
        m = re.match(r'calibrate: MIDI (\d+) (.*)', line)
        if not m:
            continue
        midi = int(m.group(1))
        if 'does not sound' in line:
            bad.append(f'{midi} silent/wrong register')
            continue
        c = float(re.search(r'([+-]\d+) cents', line).group(1))
        if midi <= 87:
            worst_lo = max(worst_lo, abs(c))
            if abs(c) > 5: bad.append(f'{midi} {c:+.0f}c')
        else:
            worst_hi = max(worst_hi, abs(c))
            if abs(c) > 35: bad.append(f'{midi} {c:+.0f}c')
    check('tuning and registers', not bad,
          f'worst {worst_lo:.0f} c (C4-D#6), {worst_hi:.0f} c (E6-C7)' + (f'; {bad}' if bad else ''))

def parity():
    """The portable build plays the same notes (pitch and level), R22."""
    if not os.path.exists(SCALAR):
        check('scalar parity', False, 'flute-scalar missing (make scalar)')
        return
    worst = ''
    ok = True
    for note in (60, 69, 81):
        a, b = run(['-note', str(note)]).stdout, run(['-note', str(note)], SCALAR).stdout
        fa, fb = [float(re.search(r'played ([\d.]+) Hz', t).group(1)) for t in (a, b)]
        ra, rb = [float(re.search(r'rms ([\d.]+)', t).group(1)) for t in (a, b)]
        dc = abs(1200 * math.log2(fa / fb))
        dr = abs(ra / rb - 1)
        ok &= dc < 1 and dr < 0.02
        worst += f' {note}: {dc:.2f} c, {100 * dr:.1f} %'
    check('scalar parity', ok, worst.strip())

def realtime():
    """Real-time margin: one 20 s note well under budget (R27: < 10 % of a core)."""
    t = time.perf_counter()
    run(['-note', '69', '-secs', '20'], env={'PJ_NOCAL': '1'})
    ns = (time.perf_counter() - t) / (20 * 48000) * 1e9
    check('real-time margin', ns < 400, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')

def passive_vs_tmm():
    """Waveguide vs transfer matrix, first resonance. Becomes a check (<= 3 c) after PLAN step 2."""
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import tmm
    pats = {60: "xxxxxxxxxxxxxxxx", 64: "xxxxxxxxxxxxoooo", 67: "xxxxxxxxxooooooo",
            69: "xxxxxxoxoooooooo", 72: "xxxooooxoooooooo"}
    out = []
    for m, p in pats.items():
        t = 440 * 2 ** ((m - 69) / 12)
        line = run(['-imp', str(m)]).stdout
        fd = [float(v) for v in line.split(':')[1].split('(')[0].split() if float(v) > 0.8 * t][0]
        tm = [v for v in tmm.peaks(p, extra=2e-3, fmax=1200) if v > 0.8 * t][0]
        out.append(f'{m}: {1200 * math.log2(fd / tm):+.1f} c')
    info('waveguide vs tmm.py (mode 1)', ', '.join(out))

if __name__ == '__main__':
    t0 = time.time()
    for f in (vectorised, nan_recovery, fuzz, tuning, parity, realtime, passive_vs_tmm):
        f()
    print(f"\n{len(failures)} failed, {time.time() - t0:.0f} s" + (f": {failures}" if failures else ''))
    sys.exit(1 if failures else 0)
