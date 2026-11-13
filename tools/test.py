#!/usr/bin/env python3
"""make test: fast checks that lock in the model's behaviour (PLAN.md step 1).

Run from the repo root after `make flute scalar`. Each check prints ok/FAIL;
exit status 1 if any check fails. "info" lines report but do not fail (they
become checks once the planned fix lands).
"""
import math, os, re, resource, subprocess, sys, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FLUTE = os.path.join(ROOT, 'flute')
SCALAR = os.path.join(ROOT, 'flute-scalar')
failures = []
# The flute family: name, sounding minus written pitch, written range.
FAMILY = (('flute-c', 0, 60, 96), ('piccolo', 12, 62, 95), ('alto', -5, 60, 88), ('bass', -12, 60, 86))

def run(args, binary=FLUTE, env=None):
    e = dict(os.environ)
    e.update(env or {})
    return subprocess.run([binary] + args, capture_output=True, text=True, env=e, cwd=ROOT)

def cpu_time(args, env=None):
    """CPU seconds of one run (not wall time: a busy machine does not fail the check)."""
    before = resource.getrusage(resource.RUSAGE_CHILDREN)
    run(args, env=env)
    after = resource.getrusage(resource.RUSAGE_CHILDREN)
    return (after.ru_utime + after.ru_stime) - (before.ru_utime + before.ru_stime)

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

def tuning(inst='flute', tr=0):
    """Every note sounds in its register; C4..C#6 within 3 cents on the flute (5 on the others),
    D6..C7 within 40 (written pitches on the piccolo, alto and bass flutes). D6 and D#6 are
    third-octave fingerings (their 3rd and 4th modes): with the physical geometry (PLAN step 2)
    and the player's ranges (step 3.5) the alto's D#6 is +21 c, a model error, logged."""
    r = run(['-inst', inst, '-cal'])
    bad, worst_lo, worst_hi = [], 0.0, 0.0
    for line in r.stdout.splitlines():
        m = re.match(r'calibrate: MIDI (\d+) (.*)', line)
        if not m:
            continue
        midi = int(m.group(1)) - tr
        if 'does not sound' in line:
            bad.append(f'{midi} silent/wrong register')
            continue
        c = float(re.search(r'([+-]\d+) cents', line).group(1))
        if midi <= 85:
            worst_lo = max(worst_lo, abs(c))
            if abs(c) > (3 if inst == 'flute' else 5): bad.append(f'{midi} {c:+.0f}c')
        else:
            worst_hi = max(worst_hi, abs(c))
            if abs(c) > 40: bad.append(f'{midi} {c:+.0f}c')
    check('tuning and registers' + ('' if inst == 'flute' else f' ({inst})'), not bad,
          f'worst {worst_lo:.0f} c (C4-C#6), {worst_hi:.0f} c (D6-C7)' + (f'; {bad}' if bad else ''))

def slur_measure(a, b, dyn, inst='flute'):
    line = run(['-inst', inst, '-slur', str(a), str(b), '-dyn', str(dyn)]).stdout
    m = re.search(r'wobble ([\d.]+) cents, level [\d.]+ wobble (\d+) %, other register (\d+)/', line)
    return (float(m.group(1)), float(m.group(2)), int(m.group(3))) if m else None

def slur_stability():
    """Slurs settle on a steady note in the right register: no pumping (a ~7 Hz
    loop between the jet's aim and the window flow made C5 warble after a soft
    slur), and upward octave slurs leave the low mode (D5 stayed on A4)."""
    bad, worst = [], (0.0, 0.0)
    for a, b in ((71, 72), (69, 72), (74, 72), (60, 72), (76, 78), (78, 79), (81, 79), (79, 81),
                 (60, 74), (62, 74), (67, 74), (71, 74), (77, 79), (83, 84), (76, 77), (72, 84),
                 (84, 86), (79, 86), (74, 86)):
        for dyn in (0.2, 0.5, 0.8):
            r = slur_measure(a, b, dyn)
            if r is None:
                bad.append(f'{a}->{b} {dyn}: no output')
                continue
            w, lw, other = r
            worst = (max(worst[0], w), max(worst[1], lw))
            if w > 3 or lw > 25 or other:
                bad.append(f'{a}->{b} dyn {dyn}: {w:.1f} c, {lw:.0f} %, {other} frames off')
    check('slur stability', not bad,
          f'worst wobble {worst[0]:.1f} cents, level {worst[1]:.0f} %' + (f'; {bad}' if bad else ''))

SLURS = ((71, 72), (69, 72), (74, 72), (60, 72), (76, 78), (78, 79), (81, 79), (79, 81),
         (60, 74), (62, 74), (67, 74), (71, 74), (77, 79), (83, 84), (76, 77), (72, 84),
         (84, 86), (79, 86), (74, 86))

def family():
    """The C-foot flute, piccolo, alto and bass flutes: tuning, the flute's slur checks (written
    pitches, within each range), stability and NaN recovery."""
    for inst, tr, lo, hi in FAMILY:
        tuning(inst, tr)
        bad, n = [], 0
        for a, b in SLURS:
            if not (lo <= a <= hi and lo <= b <= hi):
                continue
            for dyn in (0.2, 0.5, 0.8):
                r = slur_measure(a + tr, b + tr, dyn, inst)
                n += 1
                if r is None:
                    bad.append(f'{a}->{b} {dyn}: no output')
                elif r[0] > 3 or r[1] > 25 or r[2]:
                    bad.append(f'{a}->{b} dyn {dyn}: {r[0]:.1f} c, {r[1]:.0f} %, {r[2]} frames off')
        check(f'slur stability ({inst})', not bad, f'{n} slurs' + (f'; {bad}' if bad else ''))
        r = run(['-inst', inst, '-fuzz', '20', '1'])
        check(f'fuzz 20 s ({inst})', r.returncode == 0, r.stdout.strip())
        r = run(['-inst', inst, '-nantest'])
        check(f'NaN recovery ({inst})', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'piccolo', '-note', '93', '-secs', '20'], env={'PJ_NOCAL': '1'}) / (20 * 48000) * 1e9
    check('real-time margin (piccolo, 96 kHz)', ns < 800, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')

def polyphony():
    """-poly: a slurred line stays on one flute, a held note under a melody gets a
    second, three-note chords a third, a rolled four-note chord (keys 60 ms apart,
    held) a fourth; no state resets."""
    r = run(['-poly', '4', '-polydemo'], env={'PJ_FLUTESONLY': '1'})  # (four flutes, not the flute choir)
    err = r.stderr
    marks = [float(m.group(1)) for m in re.finditer(r'([\d.]+) s  ', err)]
    # "playing: 2 x flute" -> 2
    seq = [str(sum(int(m.group(1) or 1) for m in re.finditer(r'(?:(\d+) x )?[a-z]+', w)))
           for w in re.findall(r'^playing: (.*)', err, re.M)]
    ok = r.returncode == 0 and seq == ['1', '2', '3', '4'] and 'moves to flute' in err and 'state resets: 0' in err
    # Just intonation: the last C-E-G triad (before the rolled chord) puts E ~14 c low, G ~2 c high.
    tunes = [t for t in re.findall(r'tune: (.*)', err) if sorted(re.findall(r'([A-G]#?)\d', t)) == ['C', 'E', 'G']]
    cents = dict(re.findall(r'([A-G]#?)\d ([+-]\d+)', tunes[-1])) if tunes else {}
    ji = bool(cents) and -16 <= int(cents['E']) - int(cents['C']) <= -10 and 0 <= int(cents['G']) - int(cents['C']) <= 5
    check('polyphony (-poly)', ok and ji, 'flutes ' + ' -> '.join(seq) +
          '; C-E-G tuned ' + ' '.join(f'{k}{v}' for k, v in sorted(cents.items())) + ('' if ok else ': ' + err.strip()[-200:]))

def room():
    """The room (-room): an instrument gives back far less than the room's sound brings it (a small
    passive scatterer: no energy is made); the ensemble deals chords low to high; the hall dies away."""
    worst = []
    for inst in ('flute', 'piccolo', 'bass'):
        vals = [float(v) for v in re.findall(r'radiates ([\d.]+)', run(['-inst', inst, '-scatter']).stdout)]
        worst.append((inst, max(vals) if vals else 9))
    check('room: instruments are passive', all(w < 0.03 for _, w in worst),
          ', '.join(f'{i} {w:.4f} Pa/Pa' for i, w in worst))
    # Diffuse: after 100 ms the room is even left/right (1.5 dB) and uncorrelated (0.3) for a
    # player at the side or the centre; room and direct sound about equal at the listener.
    for rm in ('studio', 'hall', 'church'):
        out = run(['-roomimp', rm]).stdout
        rd = [float(v) for v in re.findall(r'room / direct ([+-][\d.]+)', out)]
        late = [(float(a), float(c)) for t0, a, c in re.findall(r'(\d+)-\s*\d+ ms: left/right ([+-][\d.]+) dB, correlation ([+-][\d.]+)', out) if int(t0) >= 100]
        ok = rd and all(-3 <= v <= 10 for v in rd) and late and all(abs(a) <= 1.5 and abs(c) <= 0.3 for a, c in late)
        # Early reflections mirror the stage: the left wall's in the left channel, the right's in the right.
        side = [(w, float(v)) for w, v in re.findall(r'(left|right) wall \d+ ms, right - left ([+-][\d.]+) dB', out)]
        ok = ok and side and all((v <= -6) if w == 'left' else (v >= 6) for w, v in side)
        check(f'room is diffuse ({rm})', bool(ok), f'room/direct {rd} dB, side walls {side[:2]}, late L/R and correlation {late}')
    wav = os.path.join(ROOT, 'tools', '.ens-test.wav')
    r = run(['-ensemble', '-polydemo', '-o', wav])
    err = r.stderr
    rank = {'bass': 0, 'alto': 1, 'flute': 2, 'piccolo': 3}
    bad = []
    for d in re.findall(r'deal: (.*)', err):
        parts = [p.strip().split(' ') for p in d.split(',')]
        if len(parts) < 5:
            continue
        notes = sorted((int(p[0][-1]) * 12 + 'C C# D D# E F F# G G# A A# B'.split().index(p[0][:-1]), rank[p[1]]) for p in parts)
        if [n[1] for n in notes] != sorted(n[1] for n in notes):
            bad.append(d)
    # Every note within its instrument's range (no octave folding); the low pair moves the bass.
    span = {'bass': (48, 74), 'alto': (55, 83), 'flute': (59, 96), 'piccolo': (74, 107)}
    names = 'C C# D D# E F F# G G# A A# B'.split()
    for m in re.finditer(r'([A-G]#?)(\d) (bass|alto|flute|piccolo)', err):
        n = 12 * (int(m.group(2)) + 1) + names.index(m.group(1))
        if not span[m.group(3)][0] <= n <= span[m.group(3)][1]:
            bad.append(m.group(0))
    ok = r.returncode == 0 and 'state resets: 0' in err and 'playing: bass, alto, 2 x flute, piccolo' in err and not bad and 'deal:' in err
    ok &= 'move: E3 to bass' in err
    tail = ''
    if os.path.exists(wav):
        import struct
        d = open(wav, 'rb').read()
        x = struct.unpack('<%df' % ((len(d) - 44) // 4), d[44:44 + 4 * ((len(d) - 44) // 4)])
        n = len(x) // 2
        def lvl(a, b):
            seg = x[2 * int(a * 48000):2 * int(b * 48000)]
            return math.sqrt(sum(v * v for v in seg) / max(1, len(seg)))
        t = n / 48000
        db = 20 * math.log10(max(lvl(t - 1, t - 0.5), 1e-9) / lvl(t - 6, t - 3))
        tail = f'; hall tail {db:.0f} dB after 2 s'
        ok &= db < -50
        os.remove(wav)
    check('ensemble in a hall (-ensemble)', ok, ('voicing low to high, in range' if not bad else f'crossed / out of range: {bad}') + tail +
          ('' if r.returncode == 0 and 'state resets: 0' in err else ': ' + err.strip()[-200:]))

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
    ns = cpu_time(['-note', '69', '-secs', '20'], env={'PJ_NOCAL': '1'}) / (20 * 48000) * 1e9
    check('real-time margin', ns < 400, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')

def register_map():
    """A slow breath ramp (x0.25 .. x3 and back, -ramp): between x0.5 and x2 of the normal
    breath only the fundamental sounds (or nothing, below x0.7), up and down (PLAN step 3: the jet used to sound the
    octave below its threshold, and soft breath made C4 jump up to C5)."""
    bad = []
    for m in (60, 62, 65, 67, 69, 72):
        for line in run(['-ramp', str(m)]).stdout.splitlines():
            for lab, a, b in re.findall(r'(\S+) \[([\d.]+)-([\d.]+)\]', line):
                lo, hi = sorted((float(a), float(b)))
                if hi < 0.5 or lo > 2.0:
                    continue
                if lab == '-' and hi < 0.7:
                    continue  # below its threshold: silence is fine
                if lab in '-?' or not 0.85 <= float(lab) <= 1.15:
                    bad.append(f'{line.split(":")[0]} {lab} at x{lo:.2f}-{hi:.2f}')
    check('register map (-ramp)', not bad, 'C4 D4 F4 G4 A4 C5: the fundamental from x0.5 to x2' + (f'; {bad}' if bad else ''))

# The designed instruments (tools/instruments.py): the written range's share
# that must sound in its register after calibration (the rest is logged).
# (2026-11-13: recorder 21/24, ocarina 21/21, shakuhachi 23/24, pan flute 30/30, clarinet 32/33,
# soprano sax 28/29, alto 30/30, tenor 30/30, oboe 26/28, bassoon 30/30, duduk 16/17;
# 2026-11-13: English horn 27/28, whistle 19/20, bottles 25/25, shehnai 19/20, bagpipe 14/15.)
WOODWINDS = {'recorder': 0.85, 'ocarina': 0.95, 'shakuhachi': 0.9, 'panflute': 1.0, 'clarinet': 0.9,
             'soprano-sax': 0.9, 'alto-sax': 0.95, 'tenor-sax': 0.95, 'baritone-sax': 0.9, 'oboe': 0.9,
             'bassoon': 0.95, 'duduk': 0.9, 'shawm': 0.75, 'english-horn': 0.9, 'whistle': 0.9, 'bottle': 1.0,
             'shehnai': 0.9, 'bagpipe': 0.9}
# The share of the sounding notes allowed over 10 c (the shawm's cross fingerings: 3 of 13).
WOODWINDS_OFF = {'shawm': 0.25, 'shehnai': 0.35, 'bagpipe': 0.25}  # (the shehnai's half-holed komal notes, the chanter's chromatic notes)


def woodwinds():
    """Each designed instrument: the calibration finds most of its notes, in tune; random
    playing never blows up; it recovers from a NaN; the longest bores run in real time."""
    for inst, need in WOODWINDS.items():
        out = run(['-inst', inst, '-cal']).stdout
        lines = re.findall(r'calibrate: MIDI (\d+) (.*)', out)
        silent = [m for m, t in lines if 'does not sound' in t]
        cents = [float(re.search(r'([+-]\d+) cents', t).group(1)) for m, t in lines if 'cents' in t]
        share = 1 - len(silent) / max(1, len(lines))
        off = [c for c in cents if abs(c) > 10]
        check(f'{inst}: notes sound and tune', share >= need and len(off) <= WOODWINDS_OFF.get(inst, 0.2) * len(cents),
              f'{len(lines) - len(silent)}/{len(lines)} sound (need {need:.0%}), {len(off)} over 10 c'
              + (f'; silent {silent}' if silent else ''))
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
        r = run(['-inst', inst, '-nantest'])
        check(f'NaN recovery ({inst})', r.returncode == 0, r.stdout.strip())
    for inst, note in (('bassoon', 46), ('baritone-sax', 50), ('clarinet', 60)):
        ns = cpu_time(['-inst', inst, '-note', str(note), '-secs', '10'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
        check(f'real-time margin ({inst})', ns < 800, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


# The brass (src/brass.cc): the share of the written range that must sound in
# its partial after calibration. (2026-11-13: horn 34/37; F2, F#2, G2, the
# lowest valved partials 2 and 3, do not speak; G#2..B2 are 21..39 c off and
# C#4, D4 14..16 c: 6 of 34 over 10 c.)
BRASS = {'horn': 0.9}
lo_steady = {'horn': 45}  # (above the notes that do not speak)


def brass():
    """Each brass instrument: the calibration finds its notes, in tune; random playing
    (valves, slurs across partials) never blows up; NaN recovery; real time; and the
    waveguide's resonances (bore, cup, valve loops) against tools/brass.py's transfer
    matrices, partials 2..12, open and with all three valves."""
    for inst, need in BRASS.items():
        out = run(['-inst', inst, '-cal']).stdout
        lines = re.findall(r'calibrate: MIDI (\d+) (.*)', out)
        silent = [m for m, t in lines if 'does not sound' in t]
        cents = [float(re.search(r'([+-]\d+) cents', t).group(1)) for m, t in lines if 'cents' in t]
        share = 1 - len(silent) / max(1, len(lines))
        off = [c for c in cents if abs(c) > 10]
        check(f'{inst}: notes sound and tune', share >= need and len(off) <= 0.2 * len(cents),
              f'{len(lines) - len(silent)}/{len(lines)} sound (need {need:.0%}), {len(off)} over 10 c'
              + (f'; silent {silent}' if silent else ''))
        # Steady notes: a held note's pitch and level do not wander or beat
        # (a slot can hold two states; the player keeps clear of the jump).
        bad = []
        for m in range(lo_steady[inst], 78, 3):
            for d in ('0.3', '0.6', '0.9'):
                t = run(['-inst', inst, '-note', str(m), '-secs', '2', '-dyn', d]).stdout
                ps = float(re.search(r'pitch wobble ([\d.]+)', t).group(1))
                lw = float(re.search(r'level wobble ([\d.]+)', t).group(1))
                if ps > 5 or lw > 25:
                    bad.append(f'{m}@{d}: {ps:.0f} c {lw:.0f} %')
        check(f'{inst}: held notes steady', len(bad) <= 1,
              f'pitch within 5 c rms, level within 25 % from 0.4 s, MIDI {lo_steady[inst]}..77 x 3 dynamics'
              + (f'; {bad}' if bad else ''))
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
        r = run(['-inst', inst, '-nantest'])
        check(f'NaN recovery ({inst})', r.returncode == 0, r.stdout.strip())
        ns = cpu_time(['-inst', inst, '-note', '65', '-secs', '10'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
        check(f'real-time margin ({inst})', ns < 800, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import brass as tmmb
    worst, out = 0, []
    for combo in ('', '123'):
        pat = ''.join('x' if str(k) in combo else 'o' for k in (1, 2, 3))
        line = run(['-inst', 'horn', '-imp', '53'], env={'PJ_NOCAL': '1', 'PJ_IMPHOLES': pat}).stdout
        got = [float(v) for v in line.split(':')[1].split('(')[0].split()]
        ref = [(n, f) for n, f, c, l in tmmb.report(tmmb.HORN, combo) if 2 <= n <= 12]
        errs = [1200 * math.log2(min(got, key=lambda g: abs(math.log(g / f))) / f) for n, f in ref]
        w = max(abs(e) for e in errs)
        worst = max(worst, w)
        out.append(f'{combo or "open"} worst {w:.1f} c')
    # (Partials 3..6 turn back in the flare, which 11 mm cells resolve coarsely: up to
    # 13 c there, the rest within 4; Bilbao & Chick's FDTD met measured horns within 1 %.)
    check('horn waveguide vs brass.py (partials 2..12)', worst <= 15, ', '.join(out))


def passive_vs_tmm():
    """Waveguide vs transfer matrix, first resonance (PLAN step 2: the grid placed on the
    embouchure, a lumped open end, the holes' position and series corrections fitted)."""
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import tmm
    pats = {60: "xxxxxxxxxxxxxxxx", 64: "xxxxxxxxxxxxoooo", 67: "xxxxxxxxxooooooo",
            69: "xxxxxxoxoooooooo", 72: "xxxooooxoooooooo"}
    out = []
    for m, p in pats.items():
        t = 440 * 2 ** ((m - 69) / 12)
        line = run(['-inst', 'flute-c', '-imp', str(m)]).stdout  # tmm.py has the C foot
        fd = [float(v) for v in line.split(':')[1].split('(')[0].split() if float(v) > 0.8 * t][0]
        tm = [v for v in tmm.peaks(p, fmax=1200) if v > 0.8 * t][0]
        out.append(f'{m}: {1200 * math.log2(fd / tm):+.1f} c')
    worst = max(abs(float(o.split()[1])) for o in out)
    check('waveguide vs tmm.py (mode 1)', worst <= 3, ', '.join(out))

if __name__ == '__main__':
    t0 = time.time()
    for f in (vectorised, nan_recovery, fuzz, tuning, slur_stability, register_map, family, polyphony, room, parity,
              realtime, passive_vs_tmm, woodwinds, brass):
        f()
    print(f"\n{len(failures)} failed, {time.time() - t0:.0f} s" + (f": {failures}" if failures else ''))
    sys.exit(1 if failures else 0)
