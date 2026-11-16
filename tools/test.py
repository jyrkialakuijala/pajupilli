#!/usr/bin/env python3
"""make test: fast checks that lock in the model's behaviour (PLAN.md step 1).

Run from the repo root after `make flute scalar`. Each check prints ok/FAIL;
exit status 1 if any check fails. "info" lines report but do not fail (they
become checks once the planned fix lands).
"""
import math, os, re, resource, subprocess, sys, tempfile, time

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FLUTE = os.path.join(ROOT, 'pajupilli')
SCALAR = os.path.join(ROOT, 'pajupilli-scalar')
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
        check('scalar parity', False, 'pajupilli-scalar missing (make scalar)')
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


def played_notes():
    """The oboe as live mode plays it (-note: calibrated, then a fresh attack at
    velocity 90): every note in its register and in tune. (-cal alone missed
    A4 sounding an octave up and E5 a twelfth up: the calibration listens to
    a short, softer attack.)"""
    from concurrent.futures import ThreadPoolExecutor
    notes = (58, 59, 62, 65, 69, 72, 74, 76, 79, 81, 83, 85)

    def one(m):
        out = run(['-inst', 'oboe', '-note', str(m), '-secs', '1.5']).stdout
        c = re.search(r'\(([+-]\d+) cents\)', out)
        return m, int(c.group(1)) if c else None
    with ThreadPoolExecutor(4) as ex:
        res = list(ex.map(one, notes))
    bad = [f'{m}: {c:+d} c' if c is not None else f'{m}: silent' for m, c in res if c is None or abs(c) > 10]
    check('oboe: notes played (-note) in their register and tune', not bad,
          f'{len(notes) - len(bad)}/{len(notes)} within 10 c' + (f'; {bad}' if bad else ''))

# The brass (src/brass.cc): the share of the written range that must sound in
# its partial after calibration. (2026-11-13: horn 34/37; F2, F#2, G2, the
# lowest valved partials 2 and 3, do not speak; G#2..B2 are 21..39 c off and
# C#4, D4 14..16 c: 6 of 34 over 10 c.)
BRASS = {'horn': 0.9, 'trumpet': 0.95, 'muted-trumpet': 0.95, 'trombone': 0.95, 'tuba': 0.95}
lo_steady = {'horn': 45, 'trumpet': 55, 'muted-trumpet': 55, 'trombone': 41, 'tuba': 37}  # (above the notes that do not speak)
hi_steady = {'horn': 78, 'trumpet': 83, 'muted-trumpet': 83, 'trombone': 78, 'tuba': 66}


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
        for m in range(lo_steady[inst], hi_steady[inst], 3):
            for d in ('0.3', '0.6', '0.9'):
                t = run(['-inst', inst, '-note', str(m), '-secs', '2', '-dyn', d]).stdout
                ps = float(re.search(r'pitch wobble ([\d.]+)', t).group(1))
                lw = float(re.search(r'level wobble ([\d.]+)', t).group(1))
                if ps > 5 or lw > 25:
                    bad.append(f'{m}@{d}: {ps:.0f} c {lw:.0f} %')
        check(f'{inst}: held notes steady', len(bad) <= 1,
              f'pitch within 5 c rms, level within 25 % from 0.4 s, MIDI {lo_steady[inst]}..{hi_steady[inst] - 1} x 3 dynamics'
              + (f'; {bad}' if bad else ''))
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
        r = run(['-inst', inst, '-nantest'])
        check(f'NaN recovery ({inst})', r.returncode == 0, r.stdout.strip())
        ns = cpu_time(['-inst', inst, '-note', '65', '-secs', '10'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
        check(f'real-time margin ({inst})', ns < 800, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import brass as tmmb
    # name, sounding MIDI for -imp, [(fingering for the waveguide, valves, slide position for brass.py)], partials
    cases = [('horn', 53, [('ooo', '', 1), ('xxx', '123', 1)], (2, 12)),
             ('trumpet', 60, [('ooo', '', 1), ('xxo', '12', 1)], (3, 10)),  # (partial 2: a broad peak, the receiver's sub-cell gap moves it 15..30 c)
             ('trombone', 46, [('1o', '', 1), ('7o', '', 7), ('1x', 'F', 1)], (2, 10)),
             ('tuba', 36, [('oooo', '', 1), ('ooox', '4', 1)], (2, 10))]
    for name, midi, fings, (plo, phi) in cases:
        worst, out = 0, []
        for pat, combo, pos in fings:
            line = run(['-inst', name, '-imp', str(midi)], env={'PJ_NOCAL': '1', 'PJ_IMPHOLES': pat}).stdout
            got = [float(v) for v in line.split(':')[1].split('(')[0].split()]
            ref = [(n, f) for n, f, c, l in tmmb.report(tmmb.INSTS[name], combo, pos) if plo <= n <= phi]
            errs = [1200 * math.log2(min(got, key=lambda g: abs(math.log(g / f))) / f) for n, f in ref]
            w = max(abs(e) for e in errs)
            worst = max(worst, w)
            out.append(f'{pat} worst {w:.1f} c')
        # (The horn's partials 3..6 turn back in the flare, which 11 mm cells
        # resolve coarsely: up to 13 c there; Bilbao & Chick's FDTD met measured
        # horns within 1 %.)
        check(f'{name} waveguide vs brass.py (partials {plo}..{phi})', worst <= 15, ', '.join(out))


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

def leads():
    """The synth leads and the synth families (src/lead.cc): after the autotune
    every note in tune (modulo the octave: a locked fifth, an octave divider or
    formants make the period longer than the note's); lead-fifths' fifth locked
    at exactly 3:2; random playing never blows up; NaN recovery; real time."""
    # Worst error allowed (c): the detuned pairs (charang, the strings) and the
    # quarter-frequency period of bass+lead measure wider.
    slack = {'lead-charang': 9, 'lead-bass+lead': 14, 'synth-strings-1': 10, 'synth-strings-2': 8,
             'lead-voice': 6, 'synth-voice': 6, 'pad-new-age': 8, 'pad-warm': 7, 'pad-choir': 8, 'pad-sweep': 8,
             'pad-halo': 5, 'pad-polysynth': 5}
    insts = ['lead-saw', 'lead-square', 'lead-calliope', 'lead-chiff', 'lead-charang', 'lead-voice',
             'lead-fifths', 'lead-bass+lead', 'synth-bass-1', 'synth-bass-2', 'synth-strings-1',
             'synth-strings-2', 'synth-voice', 'synth-brass-1', 'synth-brass-2', 'melodic-tom', 'synth-drum',
             'pad-new-age', 'pad-warm', 'pad-polysynth', 'pad-choir', 'pad-bowed', 'pad-metallic', 'pad-halo',
             'pad-sweep', 'fx-crystal']
    # The bell (new age) and the pipes (metallic) are inharmonic, and the
    # halo's per-key formants can put a pair on the 2nd and 3rd harmonics
    # (the tracker then reads a fifth up): their cores' tuning is read with
    # those parts out (PJ_INHARM=0).
    inharm = {'pad-new-age', 'pad-metallic', 'pad-halo'}
    for inst in insts:
        if inst == 'fx-crystal':
            crystal()
        else:
            tuned(inst, slack, inharm)
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
        r = run(['-inst', inst, '-nantest'])
        check(f'NaN recovery ({inst})', r.returncode == 0, r.stdout.strip())
        ns = cpu_time(['-inst', inst, '-note', '57', '-secs', '4'], env={'PJ_NOCAL': '1'}) / (4 * 48000) * 1e9
        check(f'real-time margin ({inst})', ns < 800, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def wav_mono(path):
    """A float32 or int16 WAV as a list of floats (channels averaged)."""
    import struct
    b = open(path, 'rb').read()
    i, ch, bits = 12, 1, 16
    while i < len(b):
        cid, n = b[i:i + 4], struct.unpack('<I', b[i + 4:i + 8])[0]
        if cid == b'fmt ':
            ch, bits = struct.unpack('<H', b[i + 10:i + 12])[0], struct.unpack('<H', b[i + 22:i + 24])[0]
        if cid == b'data':
            d = b[i + 8:i + 8 + n]
            x = struct.unpack(f'<{n // 4}f', d) if bits == 32 else [v / 32768 for v in struct.unpack(f'<{n // 2}h', d)]
            return [sum(x[k:k + ch]) / ch for k in range(0, len(x) - ch + 1, ch)]
        i += 8 + n + (n & 1)
    return []


def crystal():
    """fx-crystal (a struck torus plate, tools/glass.py): its pitch is its first mode (the
    autocorrelation tracker of -scale is fooled by the beating cluster 1, 1.022, 1.164): a
    strike's spectrum near the note, scanned in 1 c steps (Goertzel over 0.5 s), peaks on it."""
    import tempfile
    worst = 0
    for note in (48, 60, 72):
        path = os.path.join(tempfile.gettempdir(), f'pj_crystal_{os.getpid()}.wav')
        run(['-inst', 'fx-crystal', '-note', str(note), '-secs', '1', '-o', path])
        x = wav_mono(path)[2400:2400 + 24000]
        os.remove(path)
        f0 = 440 * 2 ** ((note - 69) / 12)
        best, bc = -1, 0
        for c in range(-40, 41):
            w = 2 * math.pi * f0 * 2 ** (c / 1200) / 48000
            k, s1, s2 = 2 * math.cos(w), 0.0, 0.0
            for v in x:
                s1, s2 = v + k * s1 - s2, s1
            p = s1 * s1 + s2 * s2 - k * s1 * s2
            if p > best:
                best, bc = p, c
        worst = max(worst, abs(bc))
    check('fx-crystal: the first mode on the note', worst <= 3, f'worst {worst} c (C3, C4, C5)')


def tuned(inst, slack, inharm):
    """One synth's tuning over its range (-scale), modulo the octave; lead-fifths' lock."""
    out = run(['-inst', inst, '-scale'], env={'PJ_INHARM': '0'} if inst in inharm else None).stdout
    cs = [int(m) for m in re.findall(r'^\S+\s+\S+\s+\S+\s+([+-]\d+)', out, re.M)]
    mod = [((c + 600) % 1200) - 600 for c in cs]
    worst = max((abs(c) for c in mod), default=99)
    check(f'{inst}: tuned by its autotune', len(cs) >= 38 and worst <= slack.get(inst, 3),
          f'{len(cs)} notes, worst {worst} c (modulo the octave)')
    if inst == 'lead-fifths':
        # Locked at 3:2 the pair repeats every two of the note's periods: an
        # octave under it, exactly. Unlocked, it beats and measures off.
        locked = sum(1 for c in cs if abs(c + 1200) <= 1)
        check('lead-fifths: the fifth locked at 3:2', locked == len(cs) and cs,
              f'{locked}/{len(cs)} notes at the pair\'s period (-1200 c) within 1 c')


def strings():
    """The plucked double bass (src/bass.cc, src/bassbody.cc; research/double_bass.md): the body's
    design puts its air and wood resonances on their targets; notes over the four strings play
    in tune; random playing never blows up; it runs within its share of a core (25 %)."""
    out = subprocess.run([sys.executable, os.path.join(ROOT, 'tools', 'bassbody.py'), '-fit'],
                         capture_output=True, text=True).stdout
    m = re.search(r'lowest two: ([\d.]+) and ([\d.]+) Hz, ratio ([\d.]+)', out)
    ok = bool(m) and abs(float(m.group(1)) - 66.0) < 0.1 and abs(float(m.group(3)) - 1.69478) < 0.002
    check('double-bass: body resonances on their targets', ok,
          f'air {m.group(1)} Hz, wood {m.group(2)} Hz, ratio {m.group(3)}' if m else out[-200:])
    cents = []
    for note in (28, 33, 38, 43, 36, 45, 50, 55):
        r = run(['-inst', 'double-bass', '-note', str(note), '-secs', '1.2'])
        c = re.search(r'\(([+-]\d+) cents\)', r.stdout)
        cents.append(int(c.group(1)) if c else 999)
    check('double-bass: notes in tune', max(abs(c) for c in cents) <= 10, f'cents {cents}')
    r = run(['-inst', 'double-bass', '-fuzz', '10', '1'])
    check('fuzz 10 s (double-bass)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'double-bass', '-note', '45', '-secs', '4'], env={'PJ_NOCAL': '1'}) / (4 * 48000) * 1e9
    check('real-time margin (double-bass)', ns < 5200, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
    # The same bass under the bow (-inst contrabass): the string is caught once a period (a clear
    # pitch, in tune) on every string, low and high and in the harmonics; a slur moves the pitch
    # under one bow.
    cents, clar = [], []
    for note in (28, 33, 38, 43, 36, 47, 52, 57, 64, 67, 74, 79, 84):
        r = run(['-inst', 'contrabass', '-note', str(note), '-secs', '2'])
        c = re.search(r'\(([+-]\d+) cents\) clarity ([\d.-]+)', r.stdout)
        cents.append(int(c.group(1)) if c else 999)
        clar.append(float(c.group(2)) if c else 0)
    check('contrabass: bowed notes in tune', max(abs(c) for c in cents) <= 10, f'cents {cents}')
    check('contrabass: bowed notes clear', min(clar) >= 0.7, f'clarity {clar}')
    r = run(['-inst', 'contrabass', '-slur', '45', '48'])
    c = re.search(r'mean ([+-]\d+) cents', r.stdout)
    check('contrabass: a slur arrives in tune', bool(c) and abs(int(c.group(1))) <= 12, r.stdout.strip()[-120:])
    r = run(['-inst', 'contrabass', '-fuzz', '10', '1'])
    check('fuzz 10 s (contrabass)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'contrabass', '-note', '45', '-secs', '4'], env={'PJ_NOCAL': '1'}) / (4 * 48000) * 1e9
    check('real-time margin (contrabass)', ns < 5200, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
    # The concert grand (-inst grand-piano): every octave speaks at its pitch (read a little
    # sharp: the stiff wires' partials lie above the harmonics), no blow-up, and real time
    # both for one note and with the pedal down (all 243 strings free).
    cents, clar = [], []
    for note in (24, 36, 48, 60, 72, 84, 96):
        r = run(['-inst', 'grand-piano', '-note', str(note), '-secs', '1.5'])
        c = re.search(r'\(([+-]\d+) cents\) clarity ([\d.-]+)', r.stdout)
        cents.append(int(c.group(1)) if c else 999)
        clar.append(float(c.group(2)) if c else 0)
    # (Modulo the octave: under a bass note the blow leaves a soundboard resonance ringing for a
    # moment, and when it lies near 2.5 times the note the tracker reads an octave low: C2 does.)
    check('grand-piano: notes at their pitch', max(abs(((c + 600) % 1200) - 600) for c in cents) <= 15, f'cents {cents}')
    # (C1's many partials lie well off the harmonics, the stiffer the wire the higher: it reads less clear.)
    check('grand-piano: notes clear', clar[0] >= 0.7 and min(clar[1:]) >= 0.8, f'clarity {clar}')
    r = run(['-inst', 'grand-piano', '-fuzz', '10', '1'])
    check('fuzz 10 s (grand-piano)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'grand-piano', '-note', '60', '-secs', '4'], env={'PJ_NOCAL': '1'}) / (4 * 48000) * 1e9
    check('real-time margin (grand-piano)', ns < 4000, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
    ns = cpu_time(['-inst', 'grand-piano', '-note', '60', '-secs', '4'], env={'PJ_NOCAL': '1', 'PJ_PIANO_PEDAL': '1'}) / (4 * 48000) * 1e9
    check('real-time margin (grand-piano, pedal down)', ns < 10400, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


# The ethnic and percussive families on the modal engine (src/modal.cc,
# src/modalinst.cc): no blow-ups, NaN recovery (the key struck again), real
# time, and the pitched ones in tune (three notes of their range).
MODALS = ['sitar', 'banjo', 'shamisen', 'koto', 'kalimba', 'fiddle', 'tinkle-bell', 'agogo', 'steel-drums',
          'woodblock', 'taiko', 'reverse-cymbal', 'vibraphone',
          # the sound effects (src/sfx.cc)
          'fret-noise', 'breath-noise', 'seashore', 'bird-tweet', 'telephone', 'helicopter', 'applause', 'gunshot']
# (Modulo the octave: a banjo's or a shamisen's fundamental is weak, a pan's
# octave blooms over it; the taiko's pitch is not clear enough to track.)
MODAL_PITCHED = {'sitar': (50, 62, 74), 'banjo': (55, 64, 76), 'shamisen': (52, 62, 66), 'koto': (55, 62, 74),
                 'kalimba': (60, 72, 84), 'fiddle': (60, 69, 81), 'steel-drums': (62, 72, 84),
                 'tinkle-bell': (55, 72, 84), 'vibraphone': (55, 72, 84)}


def struck_cents(inst, midi, harmonics=8):
    """A plucked or struck note's tuning (cents): rendered, its first 0.4 s
    (a banjo's note is nearly gone by the time -note listens), the best fit of
    a harmonic series within +-100 c of the target (a sum of log magnitudes
    over harmonics 1..8: a weak fundamental or a strong octave do not fool it)."""
    import numpy as np
    from scipy.io import wavfile
    with tempfile.TemporaryDirectory() as d:
        path = os.path.join(d, 'n.wav')
        run(['-inst', inst, '-note', str(midi), '-secs', '1', '-o', path])
        fs, x = wavfile.read(path)
    x = x.astype(float)
    if x.ndim > 1:
        x = x.mean(1)
    seg = x[int(0.03 * fs):int(0.43 * fs)]
    if np.sqrt(np.mean(seg ** 2)) < 1e-5:
        return None
    X = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 17)) + 1e-12
    df = fs / (1 << 17)
    f0 = 440 * 2 ** ((midi - 69) / 12)
    best, bc = -1e30, 0
    for c in range(-100, 101):
        f = f0 * 2 ** (c / 1200)
        sc = sum(np.log(X[int(round(k * f / df))]) for k in range(1, harmonics + 1) if k * f < fs / 2.2)
        if sc > best:
            best, bc = sc, c
    return bc


def modals():
    for inst in MODALS:
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        peak = float(re.search(r'peak ([\d.]+)', r.stdout).group(1))
        # (A peak under 8: a single hardest hit peaks near 1, but ten seconds of
        # random keys on a sitar's twenty ringing strings add up.)
        check(f'fuzz 10 s ({inst})', r.returncode == 0 and peak < 8, r.stdout.strip())
        r = run(['-inst', inst, '-nantest'])
        check(f'NaN recovery ({inst})', r.returncode == 0, r.stdout.strip())
        ns = cpu_time(['-inst', inst, '-note', '64', '-secs', '10']) / (10 * 48000) * 1e9
        check(f'real-time margin ({inst})', ns < 1000, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
        if inst in MODAL_PITCHED:
            # (A kalimba's first overtone, ~5.9 x, would pull a harmonic fit flat: its fundamental alone.)
            out = [struck_cents(inst, m, 1 if inst == 'kalimba' else 8) for m in MODAL_PITCHED[inst]]
            bad = [c for c in out if c is None or abs(c) > 15]
            check(f'{inst}: in tune', not bad, ' '.join('silent' if c is None else f'{c:+d} c' for c in out))


def electric_grand():
    """The CP-80 (src/cp80.cc: the grand's strings on a rigid bridge, read by
    piezos, then its circuit): no blow-ups, three notes in tune, real time
    with the pedal down."""
    r = run(['-inst', 'electric-grand', '-fuzz', '10', '1'])
    peak = float(re.search(r'peak ([\d.]+)', r.stdout).group(1))
    check('fuzz 10 s (electric-grand)', r.returncode == 0 and peak < 8, r.stdout.strip())
    out = [struck_cents('electric-grand', m, 1) for m in (48, 60, 72)]
    bad = [c for c in out if c is None or abs(c) > 15]
    check('electric-grand: in tune', not bad, ' '.join('silent' if c is None else f'{c:+d} c' for c in out))
    ns = cpu_time(['-inst', 'electric-grand', '-note', '60', '-secs', '4'], env={'PJ_PIANO_PEDAL': '1'}) / (4 * 48000) * 1e9
    # (About 0.45 x the grand's work, whose limit is 10400 ns: 4.7 us under a load of 27.)
    check('real-time margin (electric-grand, pedal down)', ns < 6000, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def upright():
    """The upright piano (-inst upright-piano, src/upright.cc): the grand's checks on its own scale
    and board: every octave at its pitch (modulo the octave, as the grand), clear, no blow-up,
    real time."""
    cents, clar = [], []
    for note in (24, 36, 48, 60, 72, 84, 96):
        r = run(['-inst', 'upright-piano', '-note', str(note), '-secs', '1.5'])
        c = re.search(r'\(([+-]\d+) cents\) clarity ([\d.-]+)', r.stdout)
        cents.append(int(c.group(1)) if c else 999)
        clar.append(float(c.group(2)) if c else 0)
    # C1 by its partials (tools/pianofit.py: a stiff string's series fitted), not the tracker:
    # the small board passes little of its first few partials, and the tracker can read a sixth
    # off. A short scale's bass is stretched flat: within 40 c.
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    import pianofit
    wav = os.path.join(ROOT, 'tools', '.upright-c1.wav')
    run(['-inst', 'upright-piano', '-note', '24', '-secs', '2', '-o', wav])
    fs, x = pianofit.load(wav)
    os.remove(wav)
    n = pianofit.measure_file(x, fs, 24, 0.0, 1.5)
    cents[0] = round(1200 * math.log2(n['f0'] / 32.703))
    check('upright-piano: notes at their pitch', abs(cents[0]) <= 40 and max(abs(((c + 600) % 1200) - 600) for c in cents[1:]) <= 15,
          f'cents {cents} (C1 by its partials)')
    check('upright-piano: notes clear', clar[0] >= 0.6 and min(clar[1:]) >= 0.8, f'clarity {clar}')
    r = run(['-inst', 'upright-piano', '-fuzz', '10', '1'])
    check('fuzz 10 s (upright-piano)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'upright-piano', '-note', '60', '-secs', '4'], env={'PJ_NOCAL': '1', 'PJ_PIANO_PEDAL': '1'}) / (4 * 48000) * 1e9
    check('real-time margin (upright-piano, pedal down)', ns < 10400, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def celesta():
    """The chromatic percussion. The celesta (-inst celesta, src/bars.cc): in tune, the damper
    stops a note, no blow-up, real time; then the others."""
    out = [struck_cents('celesta', m, 1) for m in (48, 60, 72, 84, 96)]
    bad = [c for c in out if c is None or abs(c) > 8]
    check('celesta: in tune', not bad, ' '.join('silent' if c is None else f'{c:+d} c' for c in out))
    import numpy as np
    from scipy.io import wavfile
    def level(seq, t0, t1):
        wav = os.path.join(ROOT, 'tools', '.celesta.wav')
        run(['-inst', 'celesta', '-seq', seq, '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
        return 10 * math.log10(np.mean(x[int(t0 * fs):int(t1 * fs)] ** 2) + 1e-20)
    held = level('on 72 80 wait 1.2 off 72', 0.9, 1.1)
    let = level('on 72 80 wait 0.3 off 72 wait 0.9', 0.9, 1.1)
    check('celesta: the damper stops a note', held - let > 25, f'{held - let:.0f} dB under the held note 0.6 s after the key is let go')
    r = run(['-inst', 'celesta', '-fuzz', '10', '1'])
    check('fuzz 10 s (celesta)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'celesta', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (celesta, random playing)', ns < 2500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
    # The rest of the chromatic percussion (src/bars.cc, src/dulcimer.cc): in tune (the
    # tubular bells have no mode at the pitch heard: their fourth is an octave over it), no
    # blow-up, real time.
    for inst, notes in (('glockenspiel', (79, 91, 103)), ('marimba', (36, 60, 84)), ('xylophone', (65, 89)),
                        ('music-box', (60, 84)), ('dulcimer', (55, 67, 79))):
        out = [struck_cents(inst, m, 1) for m in notes]
        bad = [c for c in out if c is None or abs(c) > 10]
        check(f'{inst}: in tune', not bad, ' '.join('silent' if c is None else f'{c:+d} c' for c in out))
    out = [struck_cents('tubular-bells', m + 12, 1) for m in (60, 72)]
    for i, m in enumerate((60, 72)):  # (played a key and listened for an octave up)
        wav = os.path.join(ROOT, 'tools', '.bells.wav')
        run(['-inst', 'tubular-bells', '-seq', f'on {m} 80 wait 1.0', '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
        sp = np.abs(np.fft.rfft(x[int(0.05 * fs):int(0.9 * fs)] * np.hanning(int(0.9 * fs) - int(0.05 * fs)), 1 << 18))
        fr = np.fft.rfftfreq(1 << 18, 1 / fs)
        f4 = 2 * 440 * 2 ** ((m - 69) / 12)
        band = (fr > f4 * 0.97) & (fr < f4 * 1.03)
        out[i] = round(1200 * math.log2(fr[band][np.argmax(sp[band])] / f4))
    check('tubular-bells: the fourth mode an octave over the note', all(abs(c) <= 5 for c in out), ' '.join(f'{c:+d} c' for c in out))
    for inst in ('glockenspiel', 'marimba', 'xylophone', 'tubular-bells', 'music-box', 'dulcimer'):
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'marimba', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (marimba, random playing)', ns < 2500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def harpsichord():
    """The harpsichord (src/harpsichord.cc, on the piano's engine): in tune, the 4' an octave up
    and the 16' an octave down, the damper stops a note, no blow-up, real time."""
    import numpy as np
    from scipy.io import wavfile
    out = [struck_cents('harpsichord-8', m, 4) for m in (36, 48, 60, 72, 84)]
    bad = [c for c in out if c is None or abs(c) > 8]
    check('harpsichord: the 8\' in tune', not bad, ' '.join('silent' if c is None else f'{c:+d} c' for c in out))
    # Tuned just on C (alone): the third 14 cents under equal temperament's, the fifth 2 over.
    third, fifth = struck_cents('harpsichord-8', 64, 4), struck_cents('harpsichord-8', 67, 4)
    check('harpsichord: just intonation on C', third is not None and fifth is not None and abs(third + 14) <= 3 and abs(fifth - 2) <= 3,
          f'E4 {third:+d} c, G4 {fifth:+d} c against equal temperament')
    def strongest(stops, lo, hi):
        wav = os.path.join(ROOT, 'tools', '.hpsi.wav')
        run(['-inst', 'harpsichord-8', '-seq', 'on 60 80 wait 1.0 off 60 wait 0.6', '-o', wav], env={'PJ_HPSI_STOPS': stops})
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
        sp = np.abs(np.fft.rfft(x[int(0.05 * fs):int(0.9 * fs)] * np.hanning(int(0.9 * fs) - int(0.05 * fs)), 1 << 17))
        fr = np.fft.rfftfreq(1 << 17, 1 / fs)
        band = (fr > lo) & (fr < hi)
        late = 10 * math.log10(np.mean(x[int(1.3 * fs):int(1.5 * fs)] ** 2) + 1e-20) - 10 * math.log10(np.mean(x[int(0.7 * fs):int(0.9 * fs)] ** 2) + 1e-20)
        return fr[band][np.argmax(sp[band])], late
    f4, _ = strongest('4', 100, 700)
    f16, _ = strongest('16', 100, 700)
    f8, late = strongest('8', 100, 700)
    check('harpsichord: the registers\' pitches at middle C', abs(f8 - 261.6) < 3 and abs(f4 - 523.3) < 5 and abs(f16 - 130.8) < 2,
          f"8' {f8:.1f} Hz, 4' {f4:.1f} Hz, 16' {f16:.1f} Hz")
    check('harpsichord: the damper stops a note', late < -20, f'{late:.0f} dB 0.4 s after the key is let go')
    for inst in ('harpsichord', 'harpsichord-full'):
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'harpsichord-full', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (harpsichord, all four choirs, random playing)', ns < 16000, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def guitars():
    """The classical guitar (-inst nylon-guitar, src/guitar.cc): in tune on open and fretted
    strings, a released note is damped, the strings not played ring along, no blow-up, real time."""
    import numpy as np
    from scipy.io import wavfile
    out = [struck_cents('nylon-guitar', m, 4) for m in (40, 45, 50, 55, 59, 64, 69, 76, 83)]
    bad = [c for c in out if c is None or abs(c) > 8]
    check('nylon-guitar: in tune', not bad, ' '.join('silent' if c is None else f'{c:+d} c' for c in out))
    def level(seq, t0, t1, lo=0, hi=1e9):
        wav = os.path.join(ROOT, 'tools', '.guitar.wav')
        run(['-inst', 'nylon-guitar', '-seq', seq, '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
        seg = x[int(t0 * fs):int(t1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
        fr = np.fft.rfftfreq(len(seg), 1 / fs)
        return 10 * math.log10(sp[(fr >= lo) & (fr < hi)].sum() + 1e-20)
    # (B-flat 3: a note the open strings do not ring along with.)
    held = level('on 58 80 wait 1.2 off 58', 0.9, 1.1)
    let = level('on 58 80 wait 0.3 off 58 wait 0.9', 0.9, 1.1)
    check('nylon-guitar: a released note is damped', held - let > 20, f'{held - let:.0f} dB under the held note 0.6 s after the key is let go')
    # A4 on the first string, let go: the open A string (two octaves under) rings on at 440 Hz and
    # its other partials, though it was never plucked.
    symp = level('on 69 100 wait 0.5 off 69 wait 1.0', 1.1, 1.4, 425, 455)
    floor = level('on 70 100 wait 0.5 off 70 wait 1.0', 1.1, 1.4, 425, 455)
    check('nylon-guitar: the open strings ring along', symp - floor > 6, f'{symp - floor:.0f} dB more at 440 Hz after an A than after a B-flat')
    r = run(['-inst', 'nylon-guitar', '-fuzz', '10', '1'])
    check('fuzz 10 s (nylon-guitar)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'nylon-guitar', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (nylon-guitar, random playing)', ns < 2500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def clavinet():
    """The Clavinet (-inst clavinet, src/clavinet.cc): in tune, no blow-up, real time, and the
    release three semitones down (the tangent lets the whole string go: research/clavinet.md 5)."""
    out = [struck_cents('clavinet', m, 1) for m in (40, 52, 64, 76)]
    bad = [c for c in out if c is None or abs(c) > 15]
    check('clavinet: in tune', not bad, ' '.join('silent' if c is None else f'{c:+d} c' for c in out))
    r = run(['-inst', 'clavinet', '-fuzz', '10', '1'])
    check('fuzz 10 s (clavinet)', r.returncode == 0, r.stdout.strip())
    wav = os.path.join(ROOT, 'tools', '.clav-release.wav')
    run(['-inst', 'clavinet', '-seq', 'on 52 90 wait 0.5 off 52 wait 0.3', '-o', wav])
    import numpy as np
    from scipy.io import wavfile
    fs, x = wavfile.read(wav)
    os.remove(wav)
    x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
    seg = x[int(0.505 * fs):int(0.62 * fs)]
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 17))
    f = np.fft.rfftfreq(1 << 17, 1 / fs)
    band = (f > 100) & (f < 200)
    drop = 1200 * math.log2(164.81 / f[band][np.argmax(sp[band])])
    check('clavinet: the release drops three semitones', abs(drop - 300) < 25, f'E3 released: {drop:.0f} c down')
    for inst in ('clavinet-e7', 'clavinet-c'):
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    # The wah and the envelope filter: stable at the ends of their sweeps.
    for env in ({'PJ_CLAV_WAH': '0'}, {'PJ_CLAV_WAH': '1'}, {'PJ_CLAV_ENV': '1'}):
        r = run(['-inst', 'clavinet', '-fuzz', '5', '2'], env=env)
        check(f'fuzz 5 s (clavinet, {list(env.items())[0][0]}={list(env.values())[0]})', r.returncode == 0, r.stdout.strip())
    # Pressed harder: the held note a few cents up.
    pc = []
    for press in ('0', '10'):
        c = re.search(r'\(([+-]\d+) cents\)', run(['-inst', 'clavinet', '-note', '52'], env={'PJ_CLAV_PRESS': press}).stdout)
        pc.append(int(c.group(1)) if c else -999)
    check('clavinet: pressing bends the note up', 7 <= pc[1] - pc[0] <= 13, f'{pc[1] - pc[0]} c for 10 c pressed')
    # A key struck again soon after its release: the old note goes on under the new one (the
    # tangent clamps a swinging string; only what is beyond the anvil dies in the yarn), not cut.
    def clav(seq):
        wav = os.path.join(ROOT, 'tools', '.clav-repeat.wav')
        run(['-inst', 'clavinet', '-seq', seq, '-o', wav])
        fs, y = wavfile.read(wav)
        os.remove(wav)
        return fs, y.astype(float).mean(1) if y.ndim > 1 else y.astype(float)
    fs, a = clav('on 60 100 wait 0.3 off 60 wait 0.03 on 60 100 wait 0.3 off 60 wait 0.2')
    _, b = clav('wait 0.33 on 60 100 wait 0.3 off 60 wait 0.2')
    n = min(len(a), len(b))
    w = slice(int(0.35 * fs), int(0.39 * fs))
    old = 10 * math.log10(np.mean((a[:n] - b[:n])[w] ** 2) / np.mean(b[:n][w] ** 2) + 1e-20)
    check('clavinet: a key struck again keeps the old note under the new', old > -15,
          f'the old note 20-60 ms after the re-strike: {old:+.1f} dB against the new')
    ns = cpu_time(['-inst', 'clavinet', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (clavinet, random playing)', ns < 2500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def jazz_kick():
    """The jazz bass drum (-inst jazz-kick, src/kick.cc; research/kick.md): its note, a hard stroke's
    glide and none on a feathered one, General MIDI's range over velocity, the buried beater's short
    thud, no blow-up, real time."""
    import numpy as np
    from scipy.io import wavfile
    def render(vel, env=None, inst='jazz-kick'):
        wav = os.path.join(ROOT, 'tools', '.kick.wav')
        run(['-inst', inst, '-seq', f'on 36 {vel} wait 1.5', '-o', wav], env=env)
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, (x.astype(float).mean(1) if x.ndim > 1 else x.astype(float))
    def pitch(fs, x, t, lo=40, hi=110):
        seg = x[int(t * fs):int((t + 0.12) * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 18))
        f = np.fft.rfftfreq(1 << 18, 1 / fs)
        b = (f > lo) & (f < hi)
        return f[b][np.argmax(sp[b])]
    fs, x = render(60)
    f0 = pitch(fs, x, 0.4)
    check('jazz-kick: its note (18", tuned up)', 64 < f0 < 78, f'{f0:.1f} Hz')
    glide = {}
    for v in (44, 127):
        fs, x = render(v)
        glide[v] = 1200 * math.log2(pitch(fs, x, 0.0) / pitch(fs, x, 0.6))
    check('jazz-kick: a hard stroke starts sharp, a feathered one does not',
          30 < glide[127] < 200 and abs(glide[44]) < 6, f'v127 {glide[127]:+.0f} c, v44 {glide[44]:+.0f} c')
    def lafmax(x):
        a = 1 - math.exp(-1 / (0.125 * fs))
        e, m = 0.0, 0.0
        for v in x[:int(0.5 * fs)] ** 2:
            e += a * (v - e)
            m = max(m, e)
        return 10 * math.log10(m + 1e-20)
    rng = lafmax(render(127)[1]) - lafmax(render(16)[1])
    check('jazz-kick: velocity 16 .. 127 on the GM curve (~30 dB)', 24 < rng < 38, f'{rng:.1f} dB (unweighted fast max)')
    e = lambda x: 10 * math.log10(np.mean(x[int(0.2 * fs):int(0.25 * fs)] ** 2) + 1e-20)
    open_, buried = e(render(90)[1]), e(render(90, {'PJ_KICK_BURY': '1'})[1])
    check('jazz-kick: the buried beater damps the drum', open_ - buried > 8, f'at 0.2 s {open_ - buried:.1f} dB under the open stroke')
    for inst, seed in (('jazz-kick', '1'), ('jazz-kick-20', '2')):
        r = run(['-inst', inst, '-fuzz', '10', seed])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    r = run(['-inst', 'jazz-kick', '-fuzz', '10', '3'], env={'PJ_KICK_BURY': '1'})
    check('fuzz 10 s (jazz-kick, buried)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'jazz-kick', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (jazz-kick, random playing)', ns < 1500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def metal_perc():
    """The triangle and the cowbell (-inst triangle, cowbell, src/metalperc.cc) and the agogo's
    partials (src/modalinst.cc; research/triangle_cowbell.md): the open triangle rings, the muted one
    and a choke die fast, the cowbell's and agogo's partials where recorded ones have them, General
    MIDI's range over velocity, no blow-up, real time."""
    import numpy as np
    from scipy.io import wavfile
    from scipy.signal import butter, sosfiltfilt
    def render(inst, seq):
        wav = os.path.join(ROOT, 'tools', '.metal.wav')
        run(['-inst', inst, '-seq', seq, '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, (x.astype(float).mean(1) if x.ndim > 1 else x.astype(float))
    def band(fs, x, lo, hi, t0, t1):
        y = sosfiltfilt(butter(4, [lo, hi], 'bandpass', fs=fs, output='sos'), x)
        return 10 * math.log10(np.mean(y[int(t0 * fs):int(t1 * fs)] ** 2) + 1e-30)
    fs, x = render('triangle', 'on 81 100 wait 3 off 81')
    t60 = 60 * 1.0 / max(1e-3, band(fs, x, 2000, 6000, 0.2, 0.4) - band(fs, x, 2000, 6000, 1.2, 1.4))
    check('triangle: the open stroke rings (2 - 6 kHz)', 1.5 < t60 < 6, f'T60 {t60:.1f} s')
    fs, m = render('triangle', 'on 80 100 wait 1.5 off 80')
    t60m = 60 * 0.4 / max(1e-3, band(fs, m, 2000, 6000, 0.05, 0.15) - band(fs, m, 2000, 6000, 0.45, 0.55))
    check('triangle: the muted stroke (80) dies fast', t60m < 0.8, f'T60 {t60m:.2f} s')
    fs, c = render('triangle', 'on 81 100 wait 1.0 off 81 on 80 1 wait 1.0 off 80')
    choke = band(fs, x, 1000, 12000, 1.4, 1.5) - band(fs, c, 1000, 12000, 1.4, 1.5)
    check('triangle: a muted key chokes the ringing one', choke > 25, f'0.4 s later {choke:.0f} dB under ringing free')
    def peak(x, lo, hi):
        seg = x[int(0.01 * fs):int(0.01 * fs) + (1 << 15)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 16))
        f = np.fft.rfftfreq(1 << 16, 1 / fs)
        b = (f > lo) & (f < hi)
        return f[b][np.argmax(sp[b])], 20 * math.log10(sp[b].max() / sp.max())
    fs, x = render('cowbell', 'on 56 100 wait 1 off 56')
    f1, _ = peak(x, 300, 800)
    f2, l2 = peak(x, 2300, 2700)
    check("cowbell: its note and the walls' partial (as recorded: 472 Hz; 2.49 kHz at -20 dB)",
          abs(f1 - 472) < 10 and abs(f2 - 2490) < 40 and -30 < l2 < -10, f'{f1:.0f} Hz; {f2:.0f} Hz at {l2:+.0f} dB')
    fs, x = render('agogo', 'on 83 100 wait 1 off 83')
    f5, l5 = peak(x, 4800, 5300)
    check("agogo: the recorded bells' strong partial at 5.12 x", abs(f5 / 988 - 5.12) < 0.08 and l5 > -25,
          f'{f5 / 988:.2f} x at {l5:+.0f} dB')
    def lafmax(x):
        a = 1 - math.exp(-1 / (0.125 * fs))
        e, mx = 0.0, 0.0
        for v in x[:int(0.4 * fs)] ** 2:
            e += a * (v - e)
            mx = max(mx, e)
        return 10 * math.log10(mx + 1e-20)
    for inst, key in (('triangle', 81), ('cowbell', 56)):
        rng = lafmax(render(inst, f'on {key} 127 wait 0.5')[1]) - lafmax(render(inst, f'on {key} 16 wait 0.5')[1])
        check(f'{inst}: velocity 16 .. 127 on the GM curve (~30 dB)', 24 < rng < 38, f'{rng:.1f} dB (unweighted fast max)')
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
        ns = cpu_time(['-inst', inst, '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
        check(f'real-time margin ({inst}, random playing)', ns < 1000, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def toms():
    """The jazz toms (-inst tom-10 .. tom-16-low, src/tom.cc; research/toms.md): each drum on its
    note, a hard stick stroke's glide and none on a feathered or brushed one, the brush off the head
    at once (the drum rings on as after a stick), General MIDI's range over velocity, no blow-up,
    real time."""
    import numpy as np
    from scipy.io import wavfile
    from scipy.signal import lfilter
    def render(inst, key, vel):
        wav = os.path.join(ROOT, 'tools', '.tom.wav')
        run(['-inst', inst, '-seq', f'on {key} {vel} wait 1.2', '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, (x.astype(float).mean(1) if x.ndim > 1 else x.astype(float))
    def pitch(fs, x, t0, t1, lo, hi):
        seg = x[int(t0 * fs):int(t1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 19))
        f = np.fft.rfftfreq(1 << 19, 1 / fs)
        b = (f > lo) & (f < hi)
        return f[b][np.argmax(sp[b])]
    notes = {'tom-10': 196.0, 'tom-12': 164.8, 'tom-13': 146.8, 'tom-14': 110.0, 'tom-16': 92.5, 'tom-16-low': 73.4}
    out = []
    for inst, f0 in notes.items():
        fs, x = render(inst, 45, 60)
        out.append(pitch(fs, x, 0.4, 0.9, 0.7 * f0, 1.3 * f0) / f0 - 1)
    check('toms: each drum on its note', all(abs(d) < 0.04 for d in out), ' '.join(f'{100 * d:+.1f} %' for d in out))
    g = {}
    for lab, key, vel in (('ff', 45, 127), ('feathered', 45, 30), ('brushed', 69, 90)):
        fs, x = render('tom-14', key, vel)
        g[lab] = 1200 * math.log2(pitch(fs, x, 0.01, 0.11, 100, 160) / pitch(fs, x, 0.5, 0.9, 100, 120))
    check('toms: a hard stick stroke starts sharp, a feathered or brushed one does not',
          g['ff'] > 20 and abs(g['feathered']) < 8 and abs(g['brushed']) < 8,
          f"14\": ff {g['ff']:+.0f} c, feathered {g['feathered']:+.0f} c, brushed {g['brushed']:+.0f} c")
    def decay(x, fs):  # dB from 0.1-0.15 s to 0.4-0.45 s
        e = lambda a, b: 10 * math.log10(np.mean(x[int(a * fs):int(b * fs)] ** 2) + 1e-20)
        return e(0.1, 0.15) - e(0.4, 0.45)
    ds, db = decay(*render('tom-14', 45, 90)[::-1]), decay(*render('tom-14', 69, 90)[::-1])
    check('toms: the brush jumps off and the drum rings on', abs(db - ds) < 4, f'decay 0.1 .. 0.45 s: stick {ds:.1f} dB, brush {db:.1f} dB')
    def lafmax(fs, x):
        a = 1 - math.exp(-1 / (0.125 * fs))
        return 10 * math.log10(lfilter([a], [1, a - 1], x * x)[:int(0.6 * fs)].max() + 1e-20)
    rng = lafmax(*render('tom-14', 45, 127)) - lafmax(*render('tom-14', 45, 16))
    check('toms: velocity 16 .. 127 on the GM curve (~30 dB)', 24 < rng < 38, f'{rng:.1f} dB (unweighted fast max)')
    for inst, seed in (('tom-12', '1'), ('tom-16-low', '2')):
        r = run(['-inst', inst, '-fuzz', '10', seed])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'tom-14', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (tom-14, random playing)', ns < 1500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def cymbals():
    """The cymbals (-inst ride-cymbal ... china-cymbal, src/cymbal.cc; research/cymbals_built.md): the
    ride's band decays near a recorded ride's, the wash building under ride time, the bell pitched
    and below the bow's brightness, General MIDI's range over velocity (stick and brush), the brush
    quieter than the stick, a choke, no blow-up, real time."""
    import numpy as np
    from scipy.io import wavfile
    from scipy.signal import butter, sosfiltfilt
    def render(seq, inst='ride-cymbal', env=None):
        wav = os.path.join(ROOT, 'tools', '.cym.wav')
        run(['-inst', inst, '-seq', seq, '-o', wav], env=env)
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, (x.astype(float).mean(1) if x.ndim > 1 else x.astype(float))
    def band(x, fs, fc):
        return sosfiltfilt(butter(4, [fc / 2 ** (1 / 6), fc * 2 ** (1 / 6)], 'bandpass', fs=fs, output='sos'), x)
    def t60(x, fs, fc):
        y = band(x, fs, fc)[int(0.15 * fs):] ** 2
        edc = np.cumsum(y[::-1])[::-1]
        edc = 10 * np.log10(edc / edc[0] + 1e-30)
        t = np.arange(len(edc)) / fs
        m = (edc < -5) & (edc > -25)
        return -60 / np.polyfit(t[m], edc[m], 1)[0]
    fs, x = render('on 51 90 wait 7')
    got = [t60(x, fs, fc) for fc in (1000, 4000, 8000, 12500)]
    want = [3.9, 2.6, 1.65, 1.06]  # a recorded ride (FluidR3)
    check('ride-cymbal: band decays near a recorded ride\'s (within 25 %)', all(abs(g / w - 1) < 0.25 for g, w in zip(got, want)),
          ' '.join(f'{fc}: {g:.1f} s ({w})' for fc, g, w in zip((1000, 4000, 8000, 12500), got, want)))
    b = 60 / 200
    seq, last = '', 0.0
    hits = [(bar * 4 + off) * b for bar in range(4) for off in (0, 1, 1.67, 2, 3, 3.67)]
    for h in hits:
        seq += f'wait {h - last:.4f} on 51 80 '
        last = h
    fs, x = render(seq + 'wait 1')
    y = band(x, fs, 1000) ** 2
    pre = lambda t: np.mean(y[int((t - 0.05) * fs):int(t * fs)])
    build = 10 * math.log10(np.mean([pre(h) for h in hits[-6:]]) / np.mean([pre(h) for h in hits[1:3]]))
    check('ride-cymbal: ride time builds a wash (1 kHz, bar 4 over bar 1)', build > 2, f'{build:+.1f} dB')
    def centroid(x, fs, t0, t1):
        seg = x[int(t0 * fs):int(t1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
        f = np.fft.rfftfreq(len(seg), 1 / fs)
        return (sp * f).sum() / sp.sum()
    cb = centroid(render('on 53 90 wait 1.5')[1], fs, 0.3, 1.0)
    cw = centroid(render('on 51 90 wait 1.5')[1], fs, 0.3, 1.0)
    check('ride-cymbal: the bell rings lower and clearer than the bow', cb < 0.6 * cw, f'centroid 0.3-1 s: bell {cb:.0f} Hz, bow {cw:.0f} Hz')
    def lafmax(x):
        a = 1 - math.exp(-1 / (0.125 * fs))
        from scipy.signal import lfilter
        e = lfilter([a], [1, a - 1], x[:int(0.6 * fs)] ** 2)
        return 10 * math.log10(e.max() + 1e-20)
    for inst, key in (('ride-cymbal', 51), ('ride-cymbal', 75), ('crash-cymbal', 49)):
        rng = lafmax(render(f'on {key} 127 wait 0.8', inst)[1]) - lafmax(render(f'on {key} 16 wait 0.8', inst)[1])
        check(f'{inst} key {key}: velocity 16 .. 127 on the GM curve (~30 dB)', 22 < rng < 40, f'{rng:.1f} dB (unweighted fast max)')
    quiet = lafmax(render('on 51 80 wait 0.8')[1]) - lafmax(render('on 75 80 wait 0.8')[1])
    check('ride-cymbal: a brush stroke is quieter than a stick stroke', 6 < quiet < 25, f'{quiet:.1f} dB under the stick at velocity 80')
    fs, a = render('on 49 110 wait 1.0 off 49 wait 1.5', 'crash-cymbal', {'PJ_CYM_NOTEOFF': '1'})
    _, b2 = render('on 49 110 wait 1.0 off 49 wait 1.5', 'crash-cymbal')
    e = lambda x: 10 * math.log10(np.mean(x[int(1.4 * fs):int(1.6 * fs)] ** 2) + 1e-20)
    check('crash-cymbal: choked by the hand', e(b2) - e(a) > 20, f'0.4 s after the grab {e(b2) - e(a):.1f} dB under the free ring')
    for inst, seed, env in (('ride-cymbal', '1', None), ('ride-cymbal-2', '2', None), ('crash-cymbal', '3', None),
                            ('crash-cymbal-2', '4', None), ('splash-cymbal', '5', {'PJ_CYM_BRUSH': '1'}),
                            ('china-cymbal', '6', None), ('ride-cymbal', '7', {'PJ_CYM_BRUSH': '1'})):
        r = run(['-inst', inst, '-fuzz', '10', seed], env=env)
        check(f'fuzz 10 s ({inst}{", brushes" if env else ""})', r.returncode == 0, r.stdout.strip())
    for inst, env in (('ride-cymbal', None), ('ride-cymbal', {'PJ_CYM_BRUSH': '1'}), ('crash-cymbal-2', None)):
        ns = cpu_time(['-inst', inst, '-fuzz', '10', '1'], env=dict({'PJ_NOCAL': '1'}, **(env or {}))) / (10 * 48000) * 1e9
        check(f'real-time margin ({inst}{", brushes" if env else ""}, random playing)', ns < 2500,
              f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def gm2_extras():
    """The small sounds below 35 (-inst perc-extras, src/percextras.cc; research/gm2_extras.md): every
    key sounds; the sticks ring at their free-free modes; about General MIDI's 30 dB from velocity 16
    to 127; no blow-up; real time."""
    import numpy as np
    from scipy.io import wavfile
    def render(key, vel):
        wav = os.path.join(ROOT, 'tools', '.percx.wav')
        run(['-inst', 'perc-extras', '-seq', f'on {key} {vel} wait 0.6', '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
        return fs, x
    keys = (20, 21, 23, 24, 26, 27, 28, 29, 30, 31, 32, 33, 34)
    silent = [k for k in keys if np.abs(render(k, 100)[1]).max() < 1e-4]
    check('perc-extras: every key sounds', not silent, f'silent: {silent}' if silent else f'{len(keys)} keys')
    fs, x = render(31, 100)
    n = int(0.2 * fs)
    sp = np.abs(np.fft.rfft(x[:n] * np.hanning(n), 1 << 16))
    f = np.fft.rfftfreq(1 << 16, 1 / fs)
    found = []
    for want in (330, 910, 1780, 2950):  # a 5A hickory stick, free-free
        band = (f > want * 0.93) & (f < want * 1.07)
        out = (f > want * 0.8) & (f < want * 1.2) & ~band
        found.append(sp[band].max() > 3 * np.median(sp[out]))
    check('perc-extras: the sticks ring at their bending modes', all(found), f'330/910/1780/2950 Hz: {found}')
    rng = []
    for k in (31, 34):
        a, b = render(k, 16)[1], render(k, 127)[1]
        rng.append(20 * math.log10(np.sqrt(np.mean(b ** 2)) / np.sqrt(np.mean(a ** 2))))
    check('perc-extras: velocity 16 to 127', all(24 <= r <= 38 for r in rng),
          f'sticks {rng[0]:.1f} dB, metronome bell {rng[1]:.1f} dB')
    r = run(['-inst', 'perc-extras', '-fuzz', '10', '3'])
    check('fuzz 10 s (perc-extras)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'perc-extras', '-fuzz', '10', '3'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (perc-extras, random playing)', ns < 500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def hand_drums():
    """The hand drums (-inst conga, bongo, timbale; src/handdrum.cc, research/hand_drums.md): each drum's
    note, the closed conga tone shorter than the open, the conga's barrel heard near 100 Hz, General
    MIDI's range over velocity, no blow-up (the implicit stick on the light timbale head), real time."""
    import numpy as np
    from scipy.io import wavfile
    def render(inst, key, vel=100):
        wav = os.path.join(ROOT, 'tools', '.hand.wav')
        run(['-inst', inst, '-seq', f'on {key} {vel} wait 1.0', '-o', wav], env={'PJ_NOCAL': '1'})
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, (x.astype(float).mean(1) if x.ndim > 1 else x.astype(float))
    def spectrum(fs, x, t0=0.01, t1=0.3):
        seg = x[int(t0 * fs):int(t1 * fs)]
        return np.fft.rfftfreq(1 << 17, 1 / fs), np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 17))
    out = []
    for inst, key, hz in (('conga', 63, 262), ('conga', 64, 196), ('bongo', 60, 523), ('bongo', 61, 392),
                          ('timbale', 65, 330), ('timbale', 66, 247)):
        f, sp = spectrum(*render(inst, key))
        b = (f > hz / 1.15) & (f < hz * 1.15)
        out.append(round(1200 * math.log2(f[b][np.argmax(sp[b])] / hz)))
    check('hand drums: each note on its tuning', all(abs(c) < 40 for c in out), ' '.join(f'{c:+d} c' for c in out))
    e = lambda x, fs: 10 * math.log10(np.mean(x[int(0.1 * fs):int(0.15 * fs)] ** 2) + 1e-20)
    fs, o = render('conga', 63)
    _, m = render('conga', 62)
    check('conga: the closed tone (62) dies faster than the open (63)', e(o, fs) - e(m, fs) > 8,
          f'at 0.1 s {e(o, fs) - e(m, fs):.1f} dB under')
    f, sp = spectrum(*render('conga', 64))
    pipe = 20 * math.log10(sp[(f > 80) & (f < 120)].max() / sp[(f > 60) & (f < 3000)].max())
    check('conga: its barrel sings near 100 Hz', pipe > -30, f'{pipe:.1f} dB against the strongest peak')
    def lafmax(x, fs):
        a = 1 - math.exp(-1 / (0.125 * fs))
        y = np.zeros(1)
        from scipy.signal import lfilter
        e2 = lfilter([a], [1, a - 1], x[:int(0.5 * fs)] ** 2)
        return 10 * math.log10(e2.max() + 1e-20)
    rng = []
    for inst, key in (('conga', 63), ('bongo', 60), ('timbale', 65)):
        fs, hi = render(inst, key, 127)
        _, lo = render(inst, key, 16)
        rng.append(lafmax(hi, fs) - lafmax(lo, fs))
    check('hand drums: velocity 16 .. 127 on the GM curve (~30 dB)', all(22 < r < 40 for r in rng),
          ' '.join(f'{r:.1f}' for r in rng) + ' dB (conga, bongo, timbale)')
    for inst, seed in (('conga', '1'), ('bongo', '2'), ('timbale', '3')):
        r = run(['-inst', inst, '-fuzz', '10', seed])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    ns = max(cpu_time(['-inst', inst, '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) for inst in ('conga', 'bongo', 'timbale')) / (10 * 48000) * 1e9
    check('real-time margin (hand drums, random playing)', ns < 2500, f'{ns:.0f} ns/sample at most ({ns / 20833 * 100:.1f} % of a core)')


def bagpipe_drones():
    """The bagpipe's drones (src/drone.h): a key below the chanter (MIDI 67..81) is a drone pipe tuned
    to it, from the bag, while held (through -play: a General MIDI file, program 110)."""
    import struct
    import numpy as np
    from scipy.io import wavfile
    def var(n):
        out = [n & 0x7f]
        n >>= 7
        while n:
            out.insert(0, (n & 0x7f) | 0x80)
            n >>= 7
        return bytes(out)
    ev = [(0, b'\xff\x51\x03' + (500000).to_bytes(3, 'big')), (0, bytes([0xc0, 109]))]
    ev += [(0.2, bytes([0x90, 45, 90])), (0.2, bytes([0x90, 57, 90])), (3.2, bytes([0x80, 45, 0])), (3.2, bytes([0x80, 57, 0]))]
    ev.append((4.0, b'\xff\x2f\x00'))
    body, last = b'', 0
    for t, d in ev:
        tk = int(round(t / 0.5 * 480))
        body += var(tk - last) + d
        last = tk
    mid, wav = os.path.join(ROOT, 'tools', '.drone.mid'), os.path.join(ROOT, 'tools', '.drone.wav')
    open(mid, 'wb').write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(body)) + body)
    run(['-play', mid, '-room', 'dry', '-o', wav], env={'PJ_PLAY_TAIL': '0.3'})
    fs, x = wavfile.read(wav)
    os.remove(mid)
    os.remove(wav)
    x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
    seg = x[int(0.8 * fs):int(2.8 * fs)]
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 18))
    f = np.fft.rfftfreq(1 << 18, 1 / fs)
    cents = []
    for want in (110.0, 220.0):
        m = (f > want * 0.95) & (f < want * 1.05)
        cents.append(1200 * math.log2(f[m][sp[m].argmax()] / want))
    after = np.sqrt(np.mean(x[int(3.5 * fs):int(3.9 * fs)] ** 2)) / (np.sqrt(np.mean(seg ** 2)) + 1e-12)
    check('bagpipe: keys below the chanter are drones in tune (A2, A3)', all(abs(c) < 10 for c in cents) and np.abs(seg).max() > 0,
          f'{cents[0]:+.1f} c, {cents[1]:+.1f} c; after the keys are up {20 * math.log10(after + 1e-12):.0f} dB')


def organ():
    """The church organ's first rank (-inst gedackt-8, src/organ.cc, src/organpipe.h): each pipe a jet
    driving the stopped column's odd modes, calibrated in tune; it speaks in time (8' C ~150 ms, middle C
    ~90), its tone a stopped pipe's (the twelfth over the octave), stable, real time."""
    import numpy as np
    from scipy.io import wavfile
    cents = []
    for m in (36, 48, 60, 72, 84, 96):
        c = re.search(r'\(([+-]\d+) cents\)', run(['-inst', 'gedackt-8', '-note', str(m), '-secs', '1']).stdout)
        cents.append(int(c.group(1)) if c else 999)
    check('organ: the Gedackt\'s pipes speak in tune', all(abs(c) <= 3 for c in cents), ' '.join(f'{c:+d} c' for c in cents))
    def render(m):
        wav = os.path.join(ROOT, 'tools', '.organ.wav')
        run(['-inst', 'gedackt-8', '-seq', f'on {m} 80 wait 1.2 off {m} wait 0.3', '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
    speech, twelfth, octave = [], [], []
    for m in (36, 60):
        fs, x = render(m)
        env = np.sqrt(np.convolve(x ** 2, np.ones(480) / 480, 'same'))
        st = env[int(0.8 * fs):int(1.1 * fs)].mean()
        speech.append(np.where(env > 0.708 * st)[0][0] / fs * 1e3)
        seg = x[int(0.7 * fs):int(1.1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 18))
        f = np.fft.rfftfreq(1 << 18, 1 / fs)
        f0 = 440 * 2 ** ((m - 69) / 12)
        h = lambda k: sp[(f > k * f0 * 0.98) & (f < k * f0 * 1.02)].max()
        twelfth.append(20 * math.log10(h(3) / h(1)))
        octave.append(20 * math.log10(h(2) / h(1)))
    check('organ: it speaks in time (8\' C, middle C)', speech[0] < 300 and speech[1] < 150, f'{speech[0]:.0f} ms, {speech[1]:.0f} ms')
    check('organ: a stopped pipe\'s tone (the twelfth over the octave)', all(t > o + 15 for t, o in zip(twelfth, octave)),
          'twelfth ' + ', '.join(f'{t:+.0f}' for t in twelfth) + ' dB, octave ' + ', '.join(f'{o:+.0f}' for o in octave) + ' dB')
    r = run(['-inst', 'gedackt-8', '-fuzz', '10', '1'])
    check('fuzz 10 s (gedackt-8)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'gedackt-8', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (gedackt-8, random playing)', ns < 2500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def accordions():
    """The accordion and the bandoneón (-inst accordion, bandoneon; src/accordion.cc, src/freereed.h;
    research/free_reeds.md): free reeds (the minimal model of Millot & Baumann) tuned in at
    mezzo-forte; the accordion's musette beating, the bandoneón's octave pair; a reed's quick start
    and a pallet's quick stop; the bellows turned under a long chord (a short gap); pitch bend
    pulls the pitch down, as a choked pallet does; stable, real time."""
    import struct
    import numpy as np
    from scipy.io import wavfile
    cents = []
    for inst, notes in (('accordion', (36, 60, 84)), ('bandoneon', (48, 69, 90))):
        for m in notes:
            c = re.search(r'\(([+-]\d+) cents\)', run(['-inst', inst, '-note', str(m), '-secs', '1.5']).stdout)
            cents.append(int(c.group(1)) if c else 999)
    check('accordions: the reeds speak in tune (accordion C2 C4 C6, bandoneon C3 A4 F#6)',
          all(abs(c) <= 8 for c in cents), ' '.join(f'{c:+d} c' for c in cents))
    def render(inst, seq, name='.accordion.wav'):
        wav = os.path.join(ROOT, 'tools', name)
        run(['-inst', inst, '-seq', seq, '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
    # The musette: three reeds at A4, the outer two ~3.5 beats a second from the middle one.
    fs, x = render('accordion', 'on 69 90 wait 2.5 off 69 wait 0.3')
    seg = x[int(0.5 * fs):int(2.4 * fs)]
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 20))
    f = np.fft.rfftfreq(1 << 20, 1 / fs)
    peak = lambda hz: sp[(f > hz - 1) & (f < hz + 1)].max()
    mid = peak(440)
    outer = [20 * math.log10(peak(440 + d) / mid) for d in (-3.5, 3.5)]
    dip = 20 * math.log10(sp[(f > 437.8) & (f < 438.2)].max() / mid)
    check('accordion: the musette (reeds 3.5 Hz either side of A4)', all(o > -12 for o in outer) and dip < min(outer) - 6,
          ', '.join(f'{o:+.0f} dB' for o in outer) + f'; between {dip:+.0f} dB')
    # The start and the stop.
    for inst in ('accordion', 'bandoneon'):
        fs, x = render(inst, 'on 69 90 wait 1.5 off 69 wait 0.5')
        env = np.sqrt(np.convolve(x ** 2, np.ones(240) / 240, 'same'))
        st = env[int(1.0 * fs):int(1.4 * fs)].mean()
        on = np.where(env > 0.7 * st)[0][0] / fs * 1e3
        off = np.where(env[int(1.5 * fs):] < 0.01 * st)[0]
        off = off[0] / fs * 1e3 if len(off) else 999
        check(f'{inst}: A4 speaks and stops in time', on < 90 and off < 120, f'{on:.0f} ms to speak, {off:.0f} ms to -40 dB')
    # The bellows: turned within ~8.5 s of a held chord, a gap of 10-80 ms.
    fs, x = render('accordion', 'on 48 80 on 60 80 on 64 80 on 67 80 wait 9 off 48 off 60 off 64 off 67 wait 0.3')
    env = np.sqrt(np.convolve(x ** 2, np.ones(480) / 480, 'same'))
    body = env[int(0.5 * fs):int(9 * fs)]
    low = (body < 0.3 * np.median(body)).sum() / fs * 1e3
    check('accordion: the bellows turned under a long chord', 10 < low < 80, f'{low:.0f} ms under -10 dB')
    # Pitch bend: the bandoneón's A4, bend -0.5 (a choked pallet, more bellows) a little flat, quieter.
    def vlq(n):
        b = [n & 127]
        n >>= 7
        while n:
            b.insert(0, (n & 127) | 128)
            n >>= 7
        return bytes(b)
    trk = b''.join([vlq(0) + bytes([0xC0, 23]), vlq(0) + bytes([0x90, 69, 90]), vlq(1440) + bytes([0xE0, 0, 0x20]),
                    vlq(1440) + bytes([0x80, 69, 0])]) + b'\x00\xff\x2f\x00'
    mid_path = os.path.join(ROOT, 'tools', '.bend.mid')
    with open(mid_path, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(trk)) + trk)
    wav = os.path.join(ROOT, 'tools', '.bend.wav')
    run(['-play', mid_path, '-o', wav])
    os.remove(mid_path)
    fs, x = wavfile.read(wav)
    os.remove(wav)
    x = x.astype(float).mean(1)
    def pitch(t0, t1):
        seg = x[int(t0 * fs):int(t1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 19))
        f = np.fft.rfftfreq(1 << 19, 1 / fs)
        m = (f > 400) & (f < 460)
        return f[m][sp[m].argmax()], np.sqrt(np.mean(seg ** 2))
    (f0, l0), (f1, l1) = pitch(0.6, 1.4), pitch(2.1, 2.9)
    bend, drop = 1200 * math.log2(f1 / f0), 20 * math.log10(l1 / l0)
    check('bandoneon: pitch bend pulls the reeds down (EMS21: 10-35 c)', -60 < bend < -8 and drop < 0,
          f'{bend:+.0f} c, {drop:+.1f} dB')
    for inst in ('accordion', 'bandoneon'):
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    seq = 'on 36 80 on 60 80 on 64 80 on 67 80 on 72 80 wait 10 off 36 off 60 off 64 off 67 off 72'
    wav = os.path.join(ROOT, 'tools', '.accordion.wav')
    base = cpu_time(['-inst', 'accordion', '-seq', 'wait 0.01', '-o', wav])
    ns = (cpu_time(['-inst', 'accordion', '-seq', seq, '-o', wav]) - base) / (10 * 48000) * 1e9
    os.remove(wav)
    check('real-time margin (accordion: a bass and a four-note musette chord, 14 reeds)', ns < 2500,
          f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def scrapers():
    """The guiro, cuica, vibraslap and pea whistle (src/scrapers.cc; research/scrapers_friction.md):
    the guiro's clicks at the stroke's speed over the notch spacing, the cuica speaking (stick-slip)
    and its thumb's glide, the mute an octave up, the whistle's pitch and the pea's trill, the
    vibraslap's rattle about a second, velocity on the GM curve, no blow-up, real time."""
    import numpy as np
    from scipy.io import wavfile
    from scipy.signal import butter, sosfiltfilt, lfilter
    def render(inst, seq):
        wav = os.path.join(ROOT, 'tools', '.scraper.wav')
        run(['-inst', inst, '-seq', seq, '-o', wav])
        fs, y = wavfile.read(wav)
        os.remove(wav)
        return fs, (y.astype(float).mean(1) if y.ndim > 1 else y.astype(float))
    def lafmax(x, fs):
        a = 1 - math.exp(-1 / (0.125 * fs))
        return 10 * math.log10(lfilter([a], [1, a - 1], x * x).max() + 1e-20)
    # Guiro: at the stroke's middle (its fastest) the clicks repeat at v / lambda (4 mm notches).
    rates = []
    for vel in (40, 100):
        fs, x = render('guiro', f'on 74 {vel} wait 0.6')
        v = vel / 127
        T = 0.24 * (1.8 - 1.2 * v)
        vpk = math.pi * 0.15 / (2 * T)
        t0 = np.argmax(np.abs(x) > 0.02 * np.abs(x).max()) / fs
        hp = sosfiltfilt(butter(4, [2000, 9000], 'bandpass', fs=fs, output='sos'), x)
        env = sosfiltfilt(butter(2, 2000, fs=fs, output='sos'), np.abs(hp))
        seg = env[int((t0 + T / 2 - 0.02) * fs):int((t0 + T / 2 + 0.02) * fs)]
        seg = seg - seg.mean()
        ac = np.correlate(seg, seg, 'full')[len(seg) - 1:]
        lo, hi = int(fs / 2000), int(fs / 60)
        k = lo + int(np.argmax(ac[lo:hi]))
        rates.append((fs / k, vpk / 0.004))
    check('guiro: clicks at the stroke speed over the notch spacing', all(abs(r / e - 1) < 0.1 for r, e in rates),
          ', '.join(f'{r:.0f} /s for {e:.0f}' for r, e in rates))
    # Cuica: the open stroke speaks (a pitch through it) and the thumb glides it up; the mute an
    # octave over the open's start.
    def pitches(x, fs, t1, t2):
        out = []
        for t in np.arange(t1, t2, 0.04):
            seg = x[int(t * fs):int((t + 0.03) * fs)]
            seg = seg - seg.mean()
            if np.sqrt(np.mean(seg ** 2)) < 1e-4:
                continue
            ac = np.correlate(seg, seg, 'full')[len(seg) - 1:]
            lo, hi = int(fs / 1500), int(fs / 150)
            out.append(fs / (lo + int(np.argmax(ac[lo:hi]))))
        return out
    fs, x = render('cuica', 'on 79 100 wait 0.6')
    po = pitches(x, fs, 0.02, 0.45)
    fs, x = render('cuica', 'on 78 100 wait 0.4')
    pm = pitches(x, fs, 0.02, 0.12)
    glide = 1200 * math.log2(np.median(po[-3:]) / np.median(po[:3])) if len(po) >= 6 else 0
    octave = 1200 * math.log2(np.median(pm) / np.median(po[:3])) if pm and po else 0
    check('cuica: speaks, the thumb glides the open stroke up', len(po) >= 8 and 300 < glide < 1000,
          f'{np.median(po[:3]):.0f} Hz to {np.median(po[-3:]):.0f} Hz ({glide:+.0f} c)')
    check('cuica: the mute an octave up', 1000 < octave < 1400, f'{octave:+.0f} c over the open')
    # Pea whistle: its pitch and the pea's trill.
    fs, x = render('pea-whistle', 'on 72 100 wait 0.6')
    seg = x[int(0.1 * fs):int(0.4 * fs)]
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg))))
    f = np.fft.rfftfreq(len(seg), 1 / fs)
    f0 = f[np.argmax(sp * (f > 1000) * (f < 6000))]
    env = sosfiltfilt(butter(2, 200, fs=fs, output='sos'), np.abs(seg))
    e = env - env.mean()
    se = np.abs(np.fft.rfft(e * np.hanning(len(e)), 1 << 16))
    fe = np.fft.rfftfreq(1 << 16, 1 / fs)
    trill = fe[np.argmax(se * (fe > 8) * (fe < 120))]
    check('pea whistle: pitch and the pea\'s trill', 3000 < f0 < 3600 and 20 < trill < 60, f'{f0:.0f} Hz, trill {trill:.0f} Hz')
    # Vibraslap: the rattle lasts about a second (to -30 dB).
    fs, x = render('vibraslap', 'on 58 100 wait 2.5')
    hp = sosfiltfilt(butter(4, [2000, 9000], 'bandpass', fs=fs, output='sos'), x)
    lev = 10 * np.log10(sosfiltfilt(butter(2, 20, fs=fs, output='sos'), hp * hp).clip(1e-20))
    last = np.where(lev > lev.max() - 30)[0][-1] / fs
    check('vibraslap: the rattle dies over about a second', 0.7 < last < 2.2, f'{last:.2f} s to -30 dB')
    # Velocity: 16 against 127, A-weighting left out (the shape is what counts here).
    rng = []
    for inst, key, wait in (('guiro', 74, 0.6), ('guiro', 73, 0.3), ('cuica', 79, 0.6), ('cuica', 78, 0.4),
                            ('vibraslap', 58, 2.0), ('pea-whistle', 72, 0.7)):
        fs, lo_ = render(inst, f'on {key} 16 wait {wait}')
        _, hi_ = render(inst, f'on {key} 127 wait {wait}')
        rng.append(lafmax(hi_, fs) - lafmax(lo_, fs))
    check('scrapers: velocity 16 .. 127 near the GM curve (~30 dB)', all(18 < r < 40 for r in rng),
          ' '.join(f'{r:.1f}' for r in rng) + ' dB (guiro long, short, cuica open, mute, vibraslap, whistle)')
    for inst, seed in (('guiro', '1'), ('cuica', '2'), ('vibraslap', '3'), ('pea-whistle', '4')):
        r = run(['-inst', inst, '-fuzz', '10', seed])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    ns = max(cpu_time(['-inst', inst, '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'})
             for inst in ('guiro', 'cuica', 'vibraslap', 'pea-whistle')) / (10 * 48000) * 1e9
    check('real-time margin (scrapers, random playing)', ns < 2500, f'{ns:.0f} ns/sample at most ({ns / 20833 * 100:.1f} % of a core)')

def shakers():
    """Maracas, shaker, cabasa, tambourine, jingle bell (src/shaker.cc; research/shakers.md): the
    maraca's chick and its rattle, each body's brightness, no two strokes alike, the cabasa's twist
    held as long as its key, velocity on the GM curve, no blow-up, real time."""
    import numpy as np
    from scipy.io import wavfile
    from scipy.signal import lfilter
    def render(inst, seq):
        wav = os.path.join(ROOT, 'tools', '.shaker.wav')
        run(['-inst', inst, '-seq', seq, '-o', wav])
        fs, y = wavfile.read(wav)
        os.remove(wav)
        return fs, (y.astype(float).mean(1) if y.ndim > 1 else y.astype(float))
    def db(x):
        return 10 * math.log10(np.mean(x * x) + 1e-20)
    def lafmax(x, fs):
        a = 1 - math.exp(-1 / (0.125 * fs))
        return 10 * math.log10(lfilter([a], [1, a - 1], x * x).max() + 1e-20)
    # The maraca: the chick at once, a rattle under it to ~80 ms, then quiet.
    fs, x = render('maracas', 'on 70 100 wait 0.06 off 70 wait 0.6')
    ms = lambda a, b: x[int(a * fs / 1000):int(b * fs / 1000)]
    chick, rattle, after = db(ms(0, 8)), db(ms(20, 70)), db(ms(200, 300))
    check('maracas: the chick, then the rattle', -38 < rattle - chick < -8 and after - chick < -50,
          f'rattle 20-70 ms {rattle - chick:+.1f} dB, 200-300 ms {after - chick:+.1f} dB, against the chick')
    # Brightness: each body's spectral centre.
    want = {'maracas': (70, 2500, 5500), 'shaker': (82, 3500, 7500), 'cabasa': (69, 5000, 12000),
            'tambourine': (54, 4500, 8500), 'jingle-bell': (83, 4000, 8500)}
    cs = []
    ok = True
    for inst, (key, lo, hi) in want.items():
        fs, y = render(inst, f'on {key} 100 wait 0.08 off {key} wait 0.5')
        sp = np.abs(np.fft.rfft(y[:int(0.2 * fs)])) ** 2
        f = np.fft.rfftfreq(int(0.2 * fs), 1 / fs)
        c = (sp * f).sum() / sp.sum()
        cs.append(f'{inst} {c:.0f}')
        ok &= lo < c < hi
    check('shakers: each body\'s brightness (spectral centre, Hz)', ok, ', '.join(cs))
    # No two strokes alike; velocity: about General MIDI's 30 dB from 16 to 127.
    rs, rng = [], []
    for inst, (key, _, _) in want.items():
        fs, y = render(inst, f'on {key} 100 wait 0.06 off {key} wait 0.44 ' * 3)
        a = y[:int(0.15 * fs)]
        rs.append(abs(np.corrcoef(a, y[int(1.0 * fs):int(1.0 * fs) + len(a)])[0, 1]))
        fs, y = render(inst, ' '.join(f'on {key} {v} wait 0.06 off {key} wait 0.74' for v in (16, 16, 127, 127)))
        seg = lambda i: y[int(i * 0.8 * fs):int((i + 1) * 0.8 * fs)]
        lo, hi = [10 * math.log10(np.mean([10 ** (lafmax(seg(i), fs) / 10) for i in p])) for p in ((0, 1), (2, 3))]
        rng.append(hi - lo)
    check('shakers: no two strokes alike', max(rs) < 0.6, 'correlations ' + ' '.join(f'{r:.2f}' for r in rs))
    check('shakers: velocity 16 to 127 on the GM curve', all(22 < r < 38 for r in rng),
          ' '.join(f'{r:.1f}' for r in rng) + ' dB (maracas, shaker, cabasa, tambourine, jingle bell)')
    # The cabasa's twist lasts as long as its key, then eases off.
    fs, y = render('cabasa', 'on 69 80 wait 0.6 off 69 wait 0.5')
    seg = lambda a, b: db(y[int(a * fs):int(b * fs)])
    top = seg(0.0, 0.1)
    held, gone = seg(0.3, 0.55) - top, seg(0.75, 0.9) - top
    check('cabasa: held as long as its key', held > -16 and gone < -40, f'0.3-0.55 s {held:+.1f} dB, 0.75-0.9 s {gone:+.1f} dB')
    for inst, seed in (('maracas', '1'), ('shaker', '2'), ('cabasa', '3'), ('tambourine', '4'), ('jingle-bell', '5')):
        r = run(['-inst', inst, '-fuzz', '10', seed])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    ns = max(cpu_time(['-inst', inst, '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'})
             for inst in ('maracas', 'tambourine')) / (10 * 48000) * 1e9
    check('real-time margin (shakers, random playing)', ns < 2500, f'{ns:.0f} ns/sample at most ({ns / 20833 * 100:.1f} % of a core)')


def stick_snare():
    """The snare with sticks (-inst stick-snare, src/sticksnare.cc; research/stick_snare.md): every
    stroke sounds; General MIDI's ~30 dB over velocity; the wires answer (the high band is theirs);
    the open rim shot louder than a stroke; the buzz roll a continuous run of impacts that stops
    when the key is let go; no blow-up; real time."""
    import numpy as np
    from scipy.io import wavfile
    def render(seq, env=None):
        wav = os.path.join(ROOT, 'tools', '.ssnare.wav')
        run(['-inst', 'stick-snare', '-seq', seq, '-o', wav], env=env)
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
    sys.path.insert(0, os.path.join(ROOT, 'tools'))
    from velcurve import a_weight
    def laf(x, fs):  # A-weighted, the 125 ms rms's peak (as velcurve.py's LAFmax)
        xa = a_weight(x, fs)
        e = np.convolve(xa * xa, np.ones(int(0.125 * fs)) / (0.125 * fs), 'same')
        return 10 * math.log10(e.max() + 1e-20)
    def hf(x, fs, t0, t1):
        seg = x[int(t0 * fs):int(t1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
        f = np.fft.rfftfreq(len(seg), 1 / fs)
        return 10 * math.log10(sp[(f > 2000) & (f < 8000)].sum() + 1e-20)
    silent = [k for k in (36, 37, 38, 39, 40, 42) if np.abs(render(f'on {k} 90 wait 0.4 off {k} wait 0.2')[1]).max() < 1e-4]
    check('stick-snare: every stroke sounds', not silent, f'silent: {silent}' if silent else '36 37 38 39 40 42')
    fs, a = render('on 38 16 wait 0.8')
    _, b = render('on 38 127 wait 0.8')
    rng = laf(b, fs) - laf(a, fs)
    check('stick-snare: velocity 16 to 127', 22 <= rng <= 40, f'{rng:.1f} dB')
    _, w = render('on 38 90 wait 0.6')
    _, nw = render('on 38 90 wait 0.6', env={'PJ_SNARE_WIRES': '0'})
    d = hf(w, fs, 0.03, 0.2) - hf(nw, fs, 0.03, 0.2)
    check('stick-snare: the wires answer the stroke (2-8 kHz, 30-200 ms)', d > 10, f'{d:+.1f} dB with the wires')
    _, s = render('on 38 100 wait 0.6')
    _, r = render('on 39 100 wait 0.6')
    rs = laf(r, fs) - laf(s, fs)
    check('stick-snare: the open rim shot is louder than a stroke', rs > 3, f'{rs:+.1f} dB')
    _, x = render('on 36 90 wait 1.5 off 36 wait 0.6')
    env = np.sqrt(np.convolve(x * x, np.ones(960) / 960, 'same'))
    held = env[int(0.3 * fs):int(1.4 * fs)]
    after = env[int(1.9 * fs):int(2.05 * fs)].max()
    even = 20 * math.log10(np.percentile(held, 95) / max(np.percentile(held, 5), 1e-12))
    stop = 20 * math.log10(after / max(np.median(held), 1e-12))
    check('stick-snare: the roll is continuous, and stops when let go', even < 20 and stop < -30,
          f'level spread while held {even:.1f} dB (5..95 %), {stop:.0f} dB 0.4 s after the release')
    r = run(['-inst', 'stick-snare', '-fuzz', '10', '1'])
    check('fuzz 10 s (stick-snare)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'stick-snare', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'}) / (10 * 48000) * 1e9
    check('real-time margin (stick-snare, random playing)', ns < 2500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')

def clap_claves_surdo():
    """The hand clap, claves and wood blocks, surdo and bell tree (src/clap.cc, woodperc.cc, surdo.cc,
    belltree.cc; research/clap_claves_surdo.md): each where it should sound, the clap and the wood
    blocks a little different every stroke, the surdo's mute, the bell tree's falling run, General
    MIDI's ~30 dB over velocity, no blow-up, real time."""
    import numpy as np
    from scipy.io import wavfile
    def render(inst, seq):
        wav = os.path.join(ROOT, 'tools', '.ccs.wav')
        run(['-inst', inst, '-seq', seq, '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
        return fs, x
    def spec(x, fs, t0, t1):
        seg = x[int(t0 * fs):int(t1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 16)) ** 2
        return np.fft.rfftfreq(1 << 16, 1 / fs), sp
    def strongest(x, fs, lo, hi, t0=0.0, t1=0.3):
        f, sp = spec(x, fs, t0, t1)
        m = (f > lo) & (f < hi)
        return f[m][np.argmax(sp[m])]
    def centre(x, fs, t0, t1):
        f, sp = spec(x, fs, t0, t1)
        return (f * sp).sum() / sp.sum()
    def laf(x, fs):  # A-weighted would be the tool's; the fast peak level suffices here
        a = math.exp(-1 / (0.125 * fs))
        e, best = 0.0, 0.0
        for v in x[::4] ** 2:
            e = a * e + (1 - a) * v
            best = max(best, e)
        return 10 * math.log10(best + 1e-20)
    # The clap: its centre where a hand clap's is (~1-4 kHz, the cupped palms' pitch), and no two alike.
    fs, x = render('hand-clap', ' '.join('on 39 100 wait 0.4' for _ in range(4)))
    segs = [x[int(i * 0.4 * fs):int(i * 0.4 * fs) + int(0.08 * fs)] for i in range(4)]
    cen = np.mean([centre(s, fs, 0, 0.08) for s in segs])
    cc = max(np.max(np.abs(np.correlate(segs[i] / np.linalg.norm(segs[i]), segs[j] / np.linalg.norm(segs[j]), 'full')))
             for i in range(4) for j in range(i + 1, 4))
    check('hand-clap: a hand clap\'s brightness, every clap its own', 1200 < cen < 4500 and cc < 0.7,
          f'spectral centre {cen:.0f} Hz; the most alike two of four: correlation {cc:.2f}')
    # The claves: their ring at ~2.45 kHz.
    fs, x = render('claves', 'on 75 100 wait 0.6')
    f = strongest(x, fs, 1000, 6000)
    check('claves: they ring at ~2.45 kHz', 2300 < f < 2600, f'{f:.0f} Hz')
    # The wood blocks: on their notes; struck roughly the same place, never quite the same stroke.
    fs, x = render('wood-blocks', 'on 76 100 wait 0.5 on 77 100 wait 0.5')
    hi, lo = strongest(x, fs, 300, 3000, 0, 0.4), strongest(x, fs, 300, 3000, 0.5, 0.9)
    seq = ' '.join('on 76 100 wait 0.5' for _ in range(8))
    fs, x = render('wood-blocks', seq)
    lv = []
    for i in range(8):
        f2, sp = spec(x, fs, i * 0.5, i * 0.5 + 0.3)
        band = lambda c: 10 * math.log10(sp[(f2 > c * 0.985) & (f2 < c * 1.015)].max() + 1e-20)
        lv.append([band(1638) - band(746), band(672) - band(746)])
    sd = float(np.mean(np.std(np.array(lv), 0)))
    check('wood-blocks: on their notes, each stroke a little different', abs(hi - 746) < 25 and abs(lo - 502) < 20 and sd > 0.5,
          f'high {hi:.0f} Hz, low {lo:.0f} Hz; partials\' balance varies {sd:.1f} dB stroke to stroke')
    # The surdo: its low note; the hand's mute.
    fs, a = render('surdo', 'on 87 100 wait 1')
    _, b = render('surdo', 'on 86 100 wait 1')
    f0 = strongest(a, fs, 40, 100, 0.05, 0.8)
    lev = lambda x, t: 10 * math.log10(np.mean(x[int(t * fs):int((t + 0.05) * fs)] ** 2) + 1e-20)
    mute = lev(a, 0.2) - lev(b, 0.2)
    check('surdo: its low note, and the hand mutes it', 55 < f0 < 70 and mute > 15, f'{f0:.1f} Hz; muted {mute:.0f} dB under open at 0.2 s')
    # The bell tree: the beater drawn down the stack: high bells first.
    fs, x = render('bell-tree', 'on 84 100 wait 1.5')
    c0, c1 = centre(x, fs, 0.0, 0.06), centre(x, fs, 0.3, 0.36)
    check('bell-tree: a falling run', c0 > 1.3 * c1, f'spectral centre {c0:.0f} Hz at the start, {c1:.0f} Hz at 0.3 s')
    # About General MIDI's 30 dB from velocity 16 to 127.
    rng = {}
    for inst, k in (('hand-clap', 39), ('claves', 75), ('wood-blocks', 76), ('surdo', 87), ('bell-tree', 84)):
        rng[inst] = laf(render(inst, f'on {k} 127 wait 1.2')[1], fs) - laf(render(inst, f'on {k} 16 wait 1.2')[1], fs)
    check('clap, claves, blocks, surdo, bell tree: velocity 16 to 127', all(22 <= v <= 40 for v in rng.values()),
          ', '.join(f'{k} {v:.1f} dB' for k, v in rng.items()))
    for inst, seed in (('hand-clap', '1'), ('claves', '2'), ('wood-blocks', '3'), ('surdo', '4'), ('bell-tree', '5')):
        r = run(['-inst', inst, '-fuzz', '10', seed])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    ns = max(cpu_time(['-inst', inst, '-fuzz', '10', '1'], env={'PJ_NOCAL': '1'})
             for inst in ('hand-clap', 'claves', 'wood-blocks', 'surdo', 'bell-tree')) / (10 * 48000) * 1e9
    check('real-time margin (clap, claves, blocks, surdo, bell tree)', ns < 2500, f'{ns:.0f} ns/sample at most ({ns / 20833 * 100:.1f} % of a core)')


if __name__ == '__main__':
    t0 = time.time()
    for f in (vectorised, nan_recovery, fuzz, tuning, slur_stability, register_map, family, polyphony, room, parity,
              realtime, passive_vs_tmm, woodwinds, played_notes, brass, leads, strings, modals, electric_grand, upright, clavinet, jazz_kick, toms, cymbals, gm2_extras, hand_drums, metal_perc, scrapers, shakers, stick_snare, clap_claves_surdo, celesta, harpsichord, guitars, bagpipe_drones, organ, accordions):
        f()
    print(f"\n{len(failures)} failed, {time.time() - t0:.0f} s" + (f": {failures}" if failures else ''))
    sys.exit(1 if failures else 0)
