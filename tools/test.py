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
    # The cello (-inst cello): the bass's construction at a cello's size (research/cello.md); its
    # open strings, stopped notes to A5, a harmonic.
    cents, clar = [], []
    for note in (38, 43, 50, 57, 64, 72, 81, 88):
        r = run(['-inst', 'cello', '-note', str(note), '-secs', '2'])
        c = re.search(r'\(([+-]?\d+) cents\) clarity ([0-9.]+)', r.stdout)
        cents.append(int(c.group(1)) if c else 999)
        clar.append(float(c.group(2)) if c else 0)
    check('cello: bowed notes in tune', max(abs(c) for c in cents) <= 10, f'cents {cents}')
    check('cello: bowed notes clear', min(clar) >= 0.7, f'clarity {clar}')
    r = run(['-inst', 'cello', '-fuzz', '10', '1'])
    check('fuzz 10 s (cello)', r.returncode == 0, r.stdout.strip())
    ns = cpu_time(['-inst', 'cello', '-note', '57', '-secs', '4'], env={'PJ_NOCAL': '1'}) / (4 * 48000) * 1e9
    check('real-time margin (cello)', ns < 5200, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
    # The viola (-inst viola): the cello's construction with a viola's strings, body and bridge
    # (research/viola.md); its open strings, stopped notes to E6, harmonics to E7.
    import numpy as np
    from scipy.io import wavfile
    cents, clar = [], []
    for note in (50, 55, 62, 69, 76, 84, 88, 93):
        r = run(['-inst', 'viola', '-note', str(note), '-secs', '2'])
        c = re.search(r'\(([+-]?\d+) cents\) clarity ([0-9.]+)', r.stdout)
        cents.append(int(c.group(1)) if c else 999)
        clar.append(float(c.group(2)) if c else 0)
    check('viola: bowed notes in tune', max(abs(c) for c in cents) <= 10, f'cents {cents}')
    check('viola: bowed notes clear', min(clar) >= 0.7, f'clarity {clar}')
    # Its two main resonances as heard (a force on the bridge's foot, the open strings silent): the
    # air (A0) 2-4 semitones above the open G, the main wood resonance near F#4, each about a
    # semitone wide or a little less (Q 15 .. 50).
    with tempfile.TemporaryDirectory() as d:
        dump = os.path.join(d, 'imp.bin')
        run(['-inst', 'viola', '-imp', '60'], env={'PJ_BASS_SYMP': '0', 'PJ_IMPOUT': '1', 'PJ_IMPDUMP': dump})
        q = np.fromfile(dump)
    X = np.abs(np.fft.rfft(q, 8 * len(q)))
    f = np.fft.rfftfreq(8 * len(q), 1 / 48000)
    def peak(lo, hi):
        m = (f > lo) & (f < hi)
        i = np.where(m)[0][np.argmax(X[m])]
        a, b = i, i
        while X[a] > X[i] / 2 ** 0.5: a -= 1
        while X[b] > X[i] / 2 ** 0.5: b += 1
        return f[i], f[i] / (f[b] - f[a])
    (fa, qa), (ft, qt) = peak(205, 245), peak(345, 390)
    check('viola: A0 and the main wood resonance, a semitone wide or less',
          205 < fa < 245 and 345 < ft < 390 and 15 <= qa <= 50 and 15 <= qt <= 50,
          f'A0 {fa:.0f} Hz Q {qa:.1f}, main {ft:.0f} Hz Q {qt:.1f}')
    # (A body whose main resonance does not rock the bridge, and a mode above it that does, robs
    # the notes whose second harmonic falls there of their fundamental: D#4 .. F#4.)
    worst = 99.0
    for note in (63, 64, 65, 66):
        with tempfile.TemporaryDirectory() as d:
            wav = os.path.join(d, 'n.wav')
            run(['-inst', 'viola', '-note', str(note), '-secs', '2', '-o', wav])
            fs, x = wavfile.read(wav)
        x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
        seg = x[int(0.6 * fs):int(1.6 * fs)]
        S = np.abs(np.fft.rfft(seg * np.hanning(len(seg))))
        g = np.fft.rfftfreq(len(seg), 1 / fs)
        f0 = 440 * 2 ** ((note - 69) / 12)
        lev = lambda h: 20 * np.log10(S[(g > 0.98 * h * f0) & (g < 1.02 * h * f0)].max())
        worst = min(worst, lev(1) - lev(2))
    check('viola: D#4 .. F#4 keep their fundamental', worst > -15, f'fundamental re second harmonic at worst {worst:+.1f} dB')
    # Pizzicato (a key let go within 100 ms): the nail's pluck, dying away.
    with tempfile.TemporaryDirectory() as d:
        wav = os.path.join(d, 'p.wav')
        run(['-inst', 'viola', '-seq', 'on 62 100 wait 0.05 off 62 wait 1.5', '-o', wav])
        fs, x = wavfile.read(wav)
    x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
    e = lambda t: 10 * math.log10(np.mean(x[int(t * fs):int((t + 0.1) * fs)] ** 2) + 1e-20)
    check('viola: a pizzicato dies away', e(0.1) - e(0.9) > 15, f'{e(0.1) - e(0.9):.1f} dB from 0.1 to 0.9 s')
    # The bow's turns (a long note: the hair runs out every second or so) leave the open C and the
    # cello's open D sounding: the fundamental dips for a moment, not a new note's 'buaaa' (held at
    # full weight through the turn the string sticks, and its fundamental comes back through a
    # double slip over ~1 s).
    for inst, note in (('viola', 48), ('cello', 50)):
        with tempfile.TemporaryDirectory() as d:
            wav = os.path.join(d, 't.wav')
            run(['-inst', inst, '-note', str(note), '-secs', '6', '-o', wav])
            fs, x = wavfile.read(wav)
        x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
        f0 = 440 * 2 ** ((note - 69) / 12)
        h1 = []
        for t in np.arange(0.5, 5.5, 0.01):
            seg = x[int(t * fs):int((t + 0.04) * fs)]
            S = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 16 * len(seg)))
            g = np.fft.rfftfreq(16 * len(seg), 1 / fs)
            h1.append(20 * np.log10(S[(g > 0.7 * f0) & (g < 1.3 * f0)].max() + 1e-9))
        down = int((np.array(h1) < np.median(h1) - 3).sum()) * 10
        check(f'{inst}: the bow turns gently', down < 450, f'fundamental more than 3 dB down {down} ms of 5 s')
    for seed in ('1', '2', '3'):
        r = run(['-inst', 'viola', '-fuzz', '10', seed])
        check(f'fuzz 10 s (viola, seed {seed})', r.returncode == 0 and 'ok' in r.stdout, r.stdout.strip())
    # (Its body steps twice per sample: a small body's grid rings up to ~16 kHz.)
    ns = cpu_time(['-inst', 'viola', '-note', '69', '-secs', '4'], env={'PJ_NOCAL': '1'}) / (4 * 48000) * 1e9
    check('real-time margin (viola)', ns < 6500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
    # The violin (-inst violin): the viola's construction at its size, the body fitted to recorded
    # violins; G3 .. C7 stopped, harmonics to A7.
    cents, clar = [], []
    for note in (57, 62, 69, 76, 83, 88, 93, 100):
        r = run(['-inst', 'violin', '-note', str(note), '-secs', '2'])
        c = re.search(r'\(([+-]?\d+) cents\) clarity ([0-9.]+)', r.stdout)
        cents.append(int(c.group(1)) if c else 999)
        clar.append(float(c.group(2)) if c else 0)
    check('violin: bowed notes in tune', max(abs(c) for c in cents) <= 10, f'cents {cents}')
    check('violin: bowed notes clear', min(clar) >= 0.7, f'clarity {clar}')
    # Its air resonance (A0) as heard, near the open D's fifth below (270 .. 290 Hz on violins).
    with tempfile.TemporaryDirectory() as d:
        dump = os.path.join(d, 'imp.bin')
        run(['-inst', 'violin', '-imp', '60'], env={'PJ_BASS_SYMP': '0', 'PJ_IMPOUT': '1', 'PJ_IMPDUMP': dump})
        q = np.fromfile(dump)
    X = np.abs(np.fft.rfft(q, 8 * len(q)))
    f = np.fft.rfftfreq(8 * len(q), 1 / 48000)
    fa, qa = peak(255, 305)
    check('violin: A0, a semitone wide or less', 255 < fa < 305 and 12 <= qa <= 60, f'A0 {fa:.0f} Hz Q {qa:.1f}')
    with tempfile.TemporaryDirectory() as d:
        wav = os.path.join(d, 'p.wav')
        run(['-inst', 'violin', '-seq', 'on 69 100 wait 0.05 off 69 wait 1.5', '-o', wav])
        fs, x = wavfile.read(wav)
    x = x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
    e = lambda t: 10 * np.log10(np.mean(x[int(t * fs):int((t + 0.05) * fs)] ** 2) + 1e-20)
    check('violin: a pizzicato dies away', e(0.1) - e(0.9) > 15, f'{e(0.1) - e(0.9):.1f} dB from 0.1 to 0.9 s')
    for seed in ('1', '2', '3'):
        r = run(['-inst', 'violin', '-fuzz', '10', seed])
        check(f'fuzz 10 s (violin, seed {seed})', r.returncode == 0 and 'ok' in r.stdout, r.stdout.strip())
    ns = cpu_time(['-inst', 'violin', '-note', '76', '-secs', '4'], env={'PJ_NOCAL': '1'}) / (4 * 48000) * 1e9
    check('real-time margin (violin)', ns < 8000, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')
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


def brush_swirl():
    """The brush swirled on a cymbal (key 84 held, src/cymbal.cc; research/jazz_drums.md 5.7): a
    sustained high swoosh whose brightness moves as the hand wanders between the edge and the bell
    (brighter towards the bell, washier at the edge); the brush lying on the plate damps a ringing
    ride a little, more when pressed harder. In -play with the brushes (drumkit.h planCymbalSwirls):
    the drummer swirls on a cymbal now and then, at least once and at most a few times, the same
    every run, and not on a cymbal the file is striking."""
    import struct
    import numpy as np
    from scipy.io import wavfile
    from scipy.signal import butter, sosfiltfilt
    def render(seq, env=None, inst='ride-cymbal'):
        wav = os.path.join(ROOT, 'tools', '.swirl.wav')
        run(['-inst', inst, '-seq', seq, '-o', wav], env=env)
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, (x.astype(float).mean(1) if x.ndim > 1 else x.astype(float))
    def centroid(x, fs, t0, t1):
        seg = x[int(t0 * fs):int(t1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
        f = np.fft.rfftfreq(len(seg), 1 / fs)
        return (sp * f).sum() / sp.sum()
    # Sustained: the 2-16 kHz band through 8 s of swirl, every half second near its median, the
    # last seconds as strong as the first; gone when the key is let go.
    fs, x = render('on 84 60 wait 8 off 84 wait 1')
    hi = sosfiltfilt(butter(4, [2000, 16000], 'bandpass', fs=fs, output='sos'), x)
    lev = [10 * math.log10(np.mean(hi[int(t * fs):int((t + 0.5) * fs)] ** 2) + 1e-30) for t in np.arange(0.5, 8, 0.5)]
    med = float(np.median(lev))
    after = 10 * math.log10(np.mean(hi[int(8.5 * fs):int(9 * fs)] ** 2) + 1e-30)
    drift = np.mean(lev[-4:]) - np.mean(lev[:4])
    check('brush swirl on the ride: a sustained high swoosh (2-16 kHz, half seconds over 8 s)',
          min(lev) > med - 12 and abs(drift) < 8 and after < med - 15,
          f'median {med:.1f} dB, lowest {min(lev) - med:+.1f}, last 2 s vs first {drift:+.1f}, after the release {after - med:+.1f}')
    cs = [centroid(x, fs, t, t + 0.5) for t in np.arange(0.5, 8, 0.5)]
    check('brush swirl on the ride: its brightness moves (the hand wandering)', max(cs) > 1.2 * min(cs),
          f'centroid {min(cs):.0f} .. {max(cs):.0f} Hz')
    # Towards the bell brighter, at the edge washier (more of it low).
    def place(at):
        fs, y = render('on 84 60 wait 5 off 84', {'PJ_CYM_SWIRLAT': str(at)})
        seg = y[fs:5 * fs]
        sp = np.abs(np.fft.rfft(seg)) ** 2
        f = np.fft.rfftfreq(len(seg), 1 / fs)
        low = sp[(f > 200) & (f < 1500)].sum() / sp[(f > 5000) & (f < 16000)].sum()
        return (sp * f).sum() / sp.sum(), 10 * math.log10(low)
    (cb, lb), (ce, le) = place(0.3), place(0.88)
    check('brush swirl on the ride: brighter near the bell, washier at the edge', cb > 1.25 * ce and le > lb + 6,
          f'centroid {cb:.0f} Hz near the bell, {ce:.0f} Hz at the edge; low (0.2-1.5 kHz) over high {lb:.1f} / {le:.1f} dB')
    # The brush's press damps a ringing ride (2 kHz, 0.7 s into the swirl): a light touch a little, a
    # heavier one more; the swirl itself well under that ring (so it is damping, not masking).
    sos = butter(4, [2000 / 1.26, 2000 * 1.26], 'bandpass', fs=fs, output='sos')
    band = lambda y: 10 * math.log10(np.mean(sosfiltfilt(sos, y)[int(1.0 * fs):int(1.3 * fs)] ** 2) + 1e-30)
    free = band(render('on 51 110 wait 2')[1])
    damp = {v: free - band(render(f'on 51 110 wait 0.3 on 84 {v} wait 1.7 off 84')[1]) for v in (30, 110)}
    alone = free - band(render('wait 0.3 on 84 110 wait 1.7 off 84')[1])
    check('brush swirl on the ride: damps the ring a little, more when pressed harder',
          2 < damp[30] < 10 and damp[110] > damp[30] + 3 and alone > 10,
          f'at 1 s, 2 kHz: light {damp[30]:.1f} dB, heavy {damp[110]:.1f} dB under the free ring (the swirl alone {alone:.1f} dB under)')
    # -play: a Brush kit file (program 40 on channel 10), 24 bars at 132 to the minute: brushed
    # snare and hi-hat foot on 2 and 4 throughout, the ride's time in bars 1-8 and 17-24, a quiet
    # passage between, a crash on bar 17.
    def vlq(n):
        b = [n & 127]
        n >>= 7
        while n:
            b.insert(0, (n & 127) | 128)
            n >>= 7
        return bytes(b)
    def song(path, ride_always=False):
        T, ev = 480, []
        def hit(t, k, v):
            ev.append((t, [0x99, k, v]))
            ev.append((t + 60, [0x89, k, 0]))
        for bar in range(24):
            quiet = 8 <= bar < 16
            for beat in range(4):
                t = (bar * 4 + beat) * T
                if beat in (1, 3):
                    hit(t, 38, 40 if quiet else 72)
                    hit(t, 44, 35 if quiet else 60)
                if not quiet or ride_always:
                    hit(t, 51, 45 if quiet else 80)
                    if beat in (1, 3):
                        hit(t + 2 * T // 3, 51, 40 if quiet else 60)
                    if beat == 0 and not quiet:
                        hit(t, 36, 50)
            if bar == 16:
                hit(bar * 4 * T, 49, 100)
        ev.sort(key=lambda e: e[0])
        trk, last = vlq(0) + bytes([0xFF, 0x51, 3]) + struct.pack('>I', 60000000 // 132)[1:] + vlq(0) + bytes([0xC9, 40]), 0
        for t, e in ev:
            trk += vlq(t - last) + bytes(e)
            last = t
        trk += b'\x00\xff\x2f\x00'
        with open(path, 'wb') as fh:
            fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, T) + b'MTrk' + struct.pack('>I', len(trk)) + trk)
    mid, wav = os.path.join(ROOT, 'tools', '.swirl.mid'), os.path.join(ROOT, 'tools', '.swirl-play.wav')
    def play(env=None):
        r = run(['-play', mid, '-o', wav], env=dict({'PJ_NOCAL': '1'}, **(env or {})))
        with open(wav, 'rb') as fh:
            audio = fh.read()
        return re.findall(r'\(the brushes: a swirl on the (\w+), bars (\d+)-(\d+)', r.stderr), audio
    song(mid)
    (p1, a1), (p2, a2) = play(), play()
    check('-play, Brush kit: the drummer swirls on a cymbal at least once, at most a few times', 1 <= len(p1) <= 3,
          '; '.join(f'{c} bars {b0}-{b1}' for c, b0, b1 in p1) or 'none')
    check('-play, Brush kit: the swirls fall in the quiet passage, two to eight bars, on the ride',
          bool(p1) and all(9 <= int(b0) and int(b1) <= 16 and 2 <= int(b1) - int(b0) + 1 <= 8 and c == 'ride' for c, b0, b1 in p1),
          '; '.join(f'{c} bars {b0}-{b1}' for c, b0, b1 in p1))
    check('-play, Brush kit: the same swirls every run (planned from the notes)', p1 == p2 and a1 == a2,
          'the plan and the audio identical' if a1 == a2 else 'differ')
    song(mid, ride_always=True)
    p3, _ = play()
    check('-play, Brush kit: never swirls on a cymbal the file is striking (the ride all through: on a crash)',
          all(c != 'ride' for c, _, _ in p3), '; '.join(f'{c} bars {b0}-{b1}' for c, b0, b1 in p3) or 'none')
    os.remove(mid)
    os.remove(wav)
    for inst, seed in (('ride-cymbal', '11'), ('crash-cymbal-2', '12'), ('brush-snare', '13')):
        r = run(['-inst', inst, '-fuzz', '10', seed])
        check(f'fuzz 10 s ({inst}, the swirl key among the keys)', r.returncode == 0, r.stdout.strip())
    setup = cpu_time(['-inst', 'ride-cymbal', '-seq', 'wait 0.01'], env={'PJ_NOCAL': '1'})  # (the plate's set-up, once)
    ns = (cpu_time(['-inst', 'ride-cymbal', '-seq', 'on 84 60 wait 10 off 84'], env={'PJ_NOCAL': '1'}) - setup) / (10 * 48000) * 1e9
    check('real-time margin (ride-cymbal, swirled for 10 s)', ns < 2500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


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


EXACT = {'PJ_ORGAN_DETUNE_HZ': '0', 'PJ_ORGAN_RANK_HZ': '0'}  # (the organ's tuning without its +-1/7 Hz life)


def organ():
    """The church organ's first ranks (src/organ.cc, src/organpipe.h, src/organbox.h; the Principal 8' an open
    pipe sounding from its mouth and its top, its bass in the façade): each pipe a jet
    driving its column's modes, calibrated in tune where it stands (its enclosure's field acting on it);
    the Schwellwerk's Gedackt 8' in two swell boxes (C side left, C# right), the Positiv's Rohrgedackt 8'
    (a chimney flute) at the rail, both together; they speak in time, with their tones (a stopped pipe's
    twelfth; the chimney's even harmonics and lifted 5th), stable, real time."""
    import numpy as np
    from scipy.io import wavfile
    for inst in ('gedackt-8', 'rohrgedackt-8', 'gedackt-rohrgedackt', 'principal-8'):
        cents = []
        for m in (36, 48, 60, 72, 84, 96):  # (the tuning's life, its +-1/7 Hz offsets, off: the calibration checked)
            c = re.search(r'\(([+-]\d+) cents\)', run(['-inst', inst, '-note', str(m), '-secs', '1'], env=EXACT).stdout)
            cents.append(int(c.group(1)) if c else 999)
        # (the swell box's air ~0.6 C warmer than the church: its Gedackt ~+2 c; each pipe +-0.5 c)
        check(f'organ: {inst} speaks in tune', all(abs(c) <= 5 for c in cents), ' '.join(f'{c:+d} c' for c in cents))
    def render(inst, m):
        wav = os.path.join(ROOT, 'tools', '.organ.wav')
        run(['-inst', inst, '-seq', f'on {m} 80 wait 1.2 off {m} wait 0.3', '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        return fs, x.astype(float).mean(1) if x.ndim > 1 else x.astype(float)
    def harmonics(fs, x, m):
        seg = x[int(0.7 * fs):int(1.1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 18))
        f = np.fft.rfftfreq(1 << 18, 1 / fs)
        f0 = 440 * 2 ** ((m - 69) / 12)
        h = [sp[(f > k * f0 * 0.98) & (f < k * f0 * 1.02)].max() for k in range(1, 7)]
        return [20 * math.log10(v / h[0]) for v in h]
    speech, ged, rohr, prin, pspeech = [], [], [], [], []
    for m in (36, 60):
        fs, x = render('principal-8', m)
        env = np.sqrt(np.convolve(x ** 2, np.ones(480) / 480, 'same'))
        pspeech.append(np.where(env > 0.708 * env[int(0.8 * fs):int(1.1 * fs)].mean())[0][0] / fs * 1e3)
        prin.append(harmonics(fs, x, m))
        fs, x = render('gedackt-8', m)
        env = np.sqrt(np.convolve(x ** 2, np.ones(480) / 480, 'same'))
        st = env[int(0.8 * fs):int(1.1 * fs)].mean()
        speech.append(np.where(env > 0.708 * st)[0][0] / fs * 1e3)
        ged.append(harmonics(fs, x, m))
        rohr.append(harmonics(*render('rohrgedackt-8', m), m))
    check('organ: it speaks in time (8\' C, middle C)', speech[0] < 300 and speech[1] < 150, f'{speech[0]:.0f} ms, {speech[1]:.0f} ms')
    check('organ: the Gedackt a stopped pipe (the twelfth over the octave)', all(h[2] > h[1] + 15 for h in ged),
          'twelfth ' + ', '.join(f'{h[2]:+.0f}' for h in ged) + ' dB, octave ' + ', '.join(f'{h[1]:+.0f}' for h in ged) + ' dB')
    check('organ: the Rohrgedackt\'s chimney (more octave and 5th than the Gedackt)',
          all(r[1] > g[1] + 5 and r[4] > g[4] + 5 for r, g in zip(rohr, ged)),
          'octave ' + ', '.join(f'{r[1]:+.0f}/{g[1]:+.0f}' for r, g in zip(rohr, ged)) + ', 5th ' +
          ', '.join(f'{r[4]:+.0f}/{g[4]:+.0f}' for r, g in zip(rohr, ged)) + ' dB (Rohr/Gedackt)')
    check('organ: the Principal an open pipe (the octave within 16 dB, 30 over the Gedackt\'s)',
          all(p[1] > -16 and p[1] > g[1] + 30 for p, g in zip(prin, ged)),
          'octave ' + ', '.join(f'{p[1]:+.0f}/{g[1]:+.0f}' for p, g in zip(prin, ged)) + ' dB (Principal/Gedackt), 3rd ' +
          ', '.join(f'{p[2]:+.0f}' for p in prin) + ', 4th ' + ', '.join(f'{p[3]:+.0f}' for p in prin) + ' dB')
    check('organ: the Principal speaks in time (8\' C ~100-250 ms, middle C ~30-90)',
          100 < pspeech[0] < 250 and 30 < pspeech[1] < 90, f'{pspeech[0]:.0f} ms, {pspeech[1]:.0f} ms')
    # The swell: the box closed (dynamics 0) quieter and darker than open (1).
    lv = []
    for d in ('0', '1'):
        wav = os.path.join(ROOT, 'tools', '.organ.wav')
        run(['-inst', 'gedackt-8', '-dyn', d, '-seq', 'on 72 80 wait 1.2 off 72 wait 0.3', '-o', wav])
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float)
        lv.append(10 * math.log10(np.mean(x[int(0.7 * fs):int(1.1 * fs)] ** 2) + 1e-20))
    check('organ: the swell box closes (quieter)', lv[0] < lv[1] - 6, f'closed {lv[0] - lv[1]:+.1f} dB')
    for inst in ('gedackt-8', 'gedackt-rohrgedackt', 'principal-8', 'organ'):
        r = run(['-inst', inst, '-fuzz', '10', '1'])
        check(f'fuzz 10 s ({inst})', r.returncode == 0, r.stdout.strip())
    run(['-inst', 'organ', '-note', '60', '-secs', '0.1'], env={'PJ_NOCAL': '1', 'PJ_ORGAN_STOPS': 'principal8,gedackt8,rohrgedackt8'})  # (the calibration cached first)
    ns = cpu_time(['-inst', 'organ', '-fuzz', '10', '1'], env={'PJ_NOCAL': '1', 'PJ_ORGAN_STOPS': 'principal8,gedackt8,rohrgedackt8'}) / (10 * 48000) * 1e9
    check('real-time margin (organ: three ranks, random playing)', ns < 8000, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')

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


def harmonica():
    """The chromatic harmonica (-inst harmonica; src/harmonica.cc; research/harmonica.md §10): 16 holes
    C3..D7 in tune; one breath direction at a time (never blow and draw reeds driven together); chords
    choose their breath by velocity and number; bends follow the pitch bend by ear; the slide
    cross-fades; a breath reversal leaves a gap, a hole change none; quick lines speak; the hands
    open brighter; CC1's hand tremolo; stable, real time."""
    import struct
    import numpy as np
    from scipy.io import wavfile
    still = {'PJ_HARP_HANDS': '0'}  # (the player's own hand moves off: measurable)
    wav = os.path.join(ROOT, 'tools', '.harmonica.wav')
    def load():
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float)
        return fs, x.mean(1) if x.ndim > 1 else x
    def render(seq, env=None):
        e = dict(still)
        e.update(env or {})
        r = run(['-inst', 'harmonica', '-seq', seq, '-o', wav], env=e)
        return (*load(), r.stderr)
    def peak_hz(x, fs, f0, t0, t1, lo=0.85, hi=1.06):
        seg = x[int(t0 * fs):int(t1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 20))
        f = np.fft.rfftfreq(1 << 20, 1 / fs)
        m = (f > f0 * lo) & (f < f0 * hi)
        return f[m][sp[m].argmax()]
    cents = []
    for m in (48, 60, 72, 84, 98):
        fs, x, _ = render(f'on {m} 80 wait 1.2 off {m}')
        f0 = 440 * 2 ** ((m - 69) / 12)
        cents.append(1200 * math.log2(peak_hz(x, fs, f0, 0.5, 1.15, 0.97, 1.03) / f0))
    check('harmonica: in tune over C3..D7 (C3 C4 C5 C6 D7)', all(abs(c) <= 15 for c in cents), ' '.join(f'{c:+.0f} c' for c in cents))
    def levels(x, fs, notes, t0, t1):
        seg = x[int(t0 * fs):int(t1 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 18))
        f = np.fft.rfftfreq(1 << 18, 1 / fs)
        ref = sp.max()
        return [20 * math.log10(sp[(f > 440 * 2 ** ((m - 69) / 12) * 0.99) & (f < 440 * 2 ** ((m - 69) / 12) * 1.01)].max() / ref) for m in notes]
    fs, x, err = render('on 60 100 on 62 40 wait 1.5 off 60 off 62', {'PJ_HARP_CHECK': '1'})
    c_, d_ = levels(x, fs, (60, 62), 0.5, 1.2)
    fs, x, _ = render('on 60 60 on 64 60 on 67 60 on 62 70 wait 1.5 off 60 off 64 off 67 off 62')
    ce, de, ee, ge = levels(x, fs, (60, 62, 64, 67), 0.5, 1.2)
    fs, x, _ = render('on 62 80 on 65 80 on 69 80 on 71 80 wait 1.5 off 62 off 65 off 69 off 71')
    draw = levels(x, fs, (62, 65, 69, 71), 0.5, 1.2)
    check('harmonica: chords take one breath (C4 v100 + D4 v40: blow; C E G v60 + D v70: blow, more notes; D F A B: draw)',
          c_ > -30 and d_ < -60 and min(ce, ee, ge) > -30 and de < -60 and min(draw) > -30,
          f'C {c_:.0f} D {d_:.0f} dB; C E G {ce:.0f} {ee:.0f} {ge:.0f}, D {de:.0f} dB; D F A B ' + ' '.join(f'{l:.0f}' for l in draw))
    r = run(['-inst', 'harmonica', '-fuzz', '10', '3'], env={'PJ_HARP_CHECK': '1'})
    viol = re.search(r'(\d+) samples with blow and draw', r.stderr)
    check('harmonica: never blow and draw reeds driven at once (fuzz 10 s, chords and reversals)',
          r.returncode == 0 and viol and viol.group(1) == '0', (viol.group(0) if viol else 'no count') + '; ' + r.stdout.strip())
    # Bends: a MIDI file, G4 bent -0.5 and -2 semitones.
    def vlq(n):
        b = [n & 127]
        n >>= 7
        while n:
            b.insert(0, (n & 127) | 128)
            n >>= 7
        return bytes(b)
    def midi(events, path):
        trk = b''.join(vlq(dt) + bytes(e) for dt, e in events) + b'\x00\xff\x2f\x00'
        with open(path, 'wb') as fh:
            fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(trk)) + trk)
    mid = os.path.join(ROOT, 'tools', '.harmonica.mid')
    midi([(0, [0xC0, 22]), (0, [0x90, 67, 80]), (960, [0xE0, 0, 0x30]), (960, [0xE0, 0, 0x00]), (960, [0xE0, 0, 0x40]),
          (960, [0xE0, 0, 0x60]), (960, [0x80, 67, 0])], mid)
    run(['-play', mid, '-o', wav], env=still)
    fs, x = load()
    f0 = 392.0
    b = [1200 * math.log2(peak_hz(x, fs, f0, t + 0.4, t + 0.95) / f0) for t in (1, 2, 3, 4)]
    up = b[3] - b[2]
    check('harmonica: bends follow the pitch bend by ear (-50 c, -200 c), up only a few cents (a softer breath)',
          abs(b[0] + 50) < 15 and abs(b[1] + 200) < 25 and 0 <= up < 15,
          f'{b[0]:+.0f} c, {b[1]:+.0f} c; up a semitone: {up:+.0f} c')
    # Transitions: the slide into G#4 (both rows partly open), a breath reversal G4 -> A4, a hole
    # change in one breath E4 -> G4.
    def dip(seq):
        fs, x, _ = render(seq)
        env = np.sqrt(np.convolve(x ** 2, np.ones(96) / 96, 'same'))
        st = np.median(env[int(0.4 * fs):int(0.75 * fs)])
        w = env[int(0.7 * fs):int(1.0 * fs)]
        return 20 * math.log10(w.min() / st), (w < st * 10 ** -0.5).sum() / fs * 1e3
    sd, _ = dip('on 67 80 wait 0.8 on 68 80 off 67 wait 0.8 off 68')
    rd, rms_ = dip('on 67 80 wait 0.8 on 69 80 off 67 wait 0.8 off 69')
    hd, hms = dip('on 64 80 wait 0.8 on 67 80 off 64 wait 0.8 off 67')
    check('harmonica: the slide cross-fades, a breath reversal leaves a gap, a hole change none',
          sd > -6 and 40 < rms_ < 130 and hms < 20, f'slide {sd:.1f} dB; reversal {rd:.0f} dB, {rms_:.0f} ms under -10 dB; hole change {hd:.1f} dB, {hms:.0f} ms')
    seq = ''.join(f'on {a} 80 wait 0.06 off {a} on {b_} 80 wait 0.06 off {b_} ' for a, b_ in [(67, 69)] * 6)
    fs, x, _ = render(seq + 'wait 0.2')
    lv = [20 * math.log10(np.sqrt(np.mean(x[int((i * 0.06 + 0.02) * fs):int((i * 0.06 + 0.06) * fs)] ** 2)) + 1e-9) for i in range(12)]
    check('harmonica: a quick blow/draw line speaks on every note (G4 A4 at 60 ms)', max(lv) - min(lv) < 10, ' '.join(f'{l:.0f}' for l in lv))
    # The hands: open, brighter.
    def cent(env):
        fs, x, _ = render('on 67 80 wait 1.5 off 67', env)
        seg = x[int(0.6 * fs):int(1.4 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
        f = np.fft.rfftfreq(len(seg), 1 / fs)
        return (sp * f).sum() / sp.sum(), 10 * math.log10(np.mean(seg ** 2))
    (cc, lc), (co, lo) = cent({}), cent({'PJ_HARP_CUPPED': '0.035', 'PJ_HARP_OPEN': '0.05'})
    check('harmonica: the hands opened, brighter', co > 1.2 * cc, f'centroid {cc:.0f} -> {co:.0f} Hz, level {lc:.1f} -> {lo:.1f} dB')
    # CC1: the hand tremolo, ~6.5 Hz, level and brightness.
    midi([(0, [0xC0, 22]), (0, [0xB0, 1, 80]), (0, [0x90, 67, 80]), (2880, [0x80, 67, 0])], mid)
    run(['-play', mid, '-o', wav], env=still)
    os.remove(mid)
    fs, x = load()
    seg = x[int(0.8 * fs):int(2.8 * fs)]
    hop = 240
    lev = np.array([10 * math.log10(np.mean(seg[i:i + 960] ** 2) + 1e-12) for i in range(0, len(seg) - 960, hop)])
    L = lev - lev.mean()
    sp = np.abs(np.fft.rfft(L * np.hanning(len(L)), 4096))
    ff = np.fft.rfftfreq(4096, hop / fs)
    m = (ff > 2) & (ff < 12)
    rate, pp = ff[m][sp[m].argmax()], np.percentile(lev, 95) - np.percentile(lev, 5)
    check('harmonica: CC1 flutters the hands (~6.5 Hz, the level)', 5.5 < rate < 7.5 and pp > 3, f'{rate:.2f} Hz, {pp:.1f} dB p-p')
    seq = 'on 60 80 on 64 80 on 67 80 on 72 80 wait 10 off 60 off 64 off 67 off 72'
    base = cpu_time(['-inst', 'harmonica', '-seq', 'wait 0.01', '-o', wav], env=still)
    ns = (cpu_time(['-inst', 'harmonica', '-seq', seq, '-o', wav], env=still) - base) / (10 * 48000) * 1e9
    os.remove(wav)
    check('real-time margin (harmonica: a four-note chord)', ns < 2500, f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def reed_organ():
    """The reed organ (-inst reed-organ; src/harmonium.cc; research/reed_organ.md §8): a pressure
    harmonium, its reeds in cells downstream, the cells out through the pallets; in tune; every key
    speaks at the softest wind; with Expression the feet's pressure is the dynamics (the tone
    brightening as it grows), without it the reservoir's steady wind; the Percussion hammer starts
    the bass quicker; the Voix céleste beats; stable, real time with Grand Jeu."""
    import numpy as np
    from scipy.io import wavfile
    wav = os.path.join(ROOT, 'tools', '.reedorgan.wav')
    def render(seq, env=None):
        run(['-inst', 'reed-organ', '-seq', seq, '-o', wav], env=env)
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float)
        return fs, x.mean(1) if x.ndim > 1 else x
    cents = []
    for m in (36, 48, 60, 72, 84, 96):
        c = re.search(r'\(([+-]\d+) cents\)', run(['-inst', 'reed-organ', '-note', str(m), '-secs', '1.5']).stdout)
        cents.append(int(c.group(1)) if c else 999)
    check('reed-organ: in tune (C2 .. C7)', all(abs(c) <= 10 for c in cents), ' '.join(f'{c:+d} c' for c in cents))
    soft = []
    for m in (36, 60, 96):
        fs, x = render(f'on {m} 1 wait 1.5 off {m}', {'PJ_HARM_PERC': '0'})
        soft.append(10 * math.log10(np.mean(x[int(0.8 * fs):int(1.4 * fs)] ** 2) + 1e-15))
    check('reed-organ: the softest key speaks (C2, C4, C7)', all(l > -75 for l in soft), ' '.join(f'{l:.0f} dB' for l in soft))
    def level(v, env):
        fs, x = render(f'on 60 {v} wait 1.5 off 60', env)
        seg = x[int(0.8 * fs):int(1.4 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)))) ** 2
        f = np.fft.rfftfreq(len(seg), 1 / fs)
        return 10 * math.log10(np.mean(seg ** 2)), (sp * f).sum() / sp.sum()
    (l1, c1), (l2, c2) = level(30, {'PJ_HARM_PERC': '0'}), level(127, {'PJ_HARM_PERC': '0'})
    (r1, _), (r2, _) = level(30, {'PJ_HARM_PERC': '0', 'PJ_HARM_EXPR': '0'}), level(127, {'PJ_HARM_PERC': '0', 'PJ_HARM_EXPR': '0'})
    check('reed-organ: Expression: the feet are the dynamics, brighter louder; the reservoir steady',
          l2 - l1 > 12 and c2 > 1.2 * c1 and abs(r2 - r1) < 1.5,
          f'v30 -> v127: {l2 - l1:+.1f} dB, centroid {c1:.0f} -> {c2:.0f} Hz; reservoir {r2 - r1:+.1f} dB')
    def onset(env):
        fs, x = render('on 48 80 wait 1.5 off 48', env)
        e = np.sqrt(np.convolve(x ** 2, np.ones(240) / 240, 'same'))
        st = e[int(1.0 * fs):int(1.4 * fs)].mean()
        return np.where(e > 0.7 * st)[0][0] / fs * 1e3
    plain, hammer = onset({'PJ_HARM_PERC': '0'}), onset({})
    check('reed-organ: the Percussion hammer starts C3 quicker', hammer < 0.85 * plain, f'{plain:.0f} ms -> {hammer:.0f} ms')
    fs, x = render('on 69 80 wait 4 off 69', {'PJ_HARM_REG': 'celeste', 'PJ_HARM_PERC': '0'})
    seg = x[int(0.5 * fs):int(3.9 * fs)]
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 21))
    f = np.fft.rfftfreq(1 << 21, 1 / fs)
    m = (f > 436) & (f < 446)
    pk = [f[m][i] for i in np.argsort(sp[m])[::-1] if True][:1]
    peaks = []
    mm = np.where(m)[0]
    for i in mm[1:-1]:
        if sp[i] > sp[i - 1] and sp[i] >= sp[i + 1] and sp[i] > 0.2 * sp[m].max():
            peaks.append(f[i])
    beat = (max(peaks) - min(peaks)) if len(peaks) >= 2 else 0
    check('reed-organ: the Voix céleste beats ~1.5 a second at A4', 1.0 < beat < 2.2, f'{beat:.2f} Hz ({len(peaks)} peaks)')
    r = run(['-inst', 'reed-organ', '-fuzz', '10', '1'])
    check('fuzz 10 s (reed-organ)', r.returncode == 0, r.stdout.strip())
    seq = 'on 48 100 on 55 100 on 60 100 on 64 100 on 67 100 on 72 100 on 76 100 on 79 100 wait 10 ' + \
          'off 48 off 55 off 60 off 64 off 67 off 72 off 76 off 79'
    env = {'PJ_HARM_REG': 'grandjeu'}
    base = cpu_time(['-inst', 'reed-organ', '-seq', 'wait 0.01', '-o', wav], env=env)
    ns = (cpu_time(['-inst', 'reed-organ', '-seq', seq, '-o', wav], env=env) - base) / (10 * 48000) * 1e9
    os.remove(wav)
    check('real-time margin (reed-organ: Grand Jeu, an eight-note chord, 32 reeds)', ns < 2500,
          f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def organ_stops():
    """Every stop of the organ built so far (src/organ.cc's kStops) speaks at its pitch on three keys (each
    rank's strongest peak near its harmonic of the key: 16' 1/2 .. 1 3/5' 5; the mixtures' ranks each at its
    composition), the smallest pipes too (oversampled, their jets deflected by the flow / their mouths)."""
    import numpy as np
    from scipy.io import wavfile
    stops = {'octave4': [2], 'quinte223': [3], 'superoctave2': [4], 'rohrflote8': [1], 'bourdon16': [0.5],
             'flute4': [2], 'nasard223': [3], 'waldflote2': [4], 'terz135': [5], 'posoctave4': [2],
             'gedacktflote4': [2], 'sesquialtera': [3, 5], 'subbass16': [0.5], 'octavbass8': [1], 'choralbass4': [2],
             'gamba8': [1], 'celeste8': [2 ** (6 / 1200)],
             'trompete8': [1], 'trompette8': [1], 'oboe8': [1], 'fagott16': [0.5], 'cromorne8': [1],
             'posaune16': [0.5], 'principal16': [0.5], 'fluteharm8': [1], 'trompete16': [0.5], 'untersatz32': [0.25],
             'principalbass16': [0.5], 'violonbass16': [0.5], 'pedtrompete8': [1],
             'salicional8': [1], 'blockflote4': [2], 'swprincipal8': [1], 'swoctave4': [2], 'posprincipal8': [1],
             'gemshorn8': [1], 'posoctave2': [4], 'dulcian16': [0.5], 'chamade8': [1], 'gedacktbass8': [1],
             'contraposaune32': [0.25], 'klarine4': [2]}
    mix = {'mixtur5': {36: [4, 6, 8, 12, 16], 60: [2, 3, 4, 6, 8], 96: [1, 1.5, 2, 3, 4]},
           'cymbel3': {36: [16, 24, 32], 60: [4, 6, 8], 96: [2, 3, 4]}, 'pedmixtur4': {36: [3, 4, 6, 8], 60: [3, 4, 6, 8]},
           'pleinjeu4': {36: [4, 6, 8, 12], 60: [2, 3, 4, 6], 96: [1, 1.5, 2, 3]},
           'scharf4': {36: [8, 12, 16, 24], 60: [4, 6, 8, 12], 96: [2, 3, 4, 6]},
           'quinte113': {36: [6], 60: [6], 84: [3]}, 'sifflote1': {36: [8], 60: [8], 84: [4]},
           'clairon4': {36: [2], 72: [2], 96: [1]}}  # (the Clairon breaks back at c''': its top octave at 8')
    wav = os.path.join(ROOT, 'tools', '.organ.wav')
    def peaks(stop, key, ratios):
        run(['-inst', 'organ', '-seq', f'on {key} 80 wait 2.0 off {key} wait 0.1', '-o', wav], env={'PJ_ORGAN_STOPS': stop, **EXACT})
        fs, x = wavfile.read(wav)
        x = x.astype(float)[int(0.7 * fs):int(2.0 * fs)]
        sp = np.abs(np.fft.rfft(x * np.hanning(len(x)), 1 << 20))
        f = np.fft.rfftfreq(1 << 20, 1 / fs)
        out = []
        for r in ratios:
            f0 = 440 * 2 ** ((key - 69) / 12) * r
            m = np.where((f > f0 * 0.985) & (f < f0 * 1.015))[0]
            i = m[sp[m].argmax()]
            a_, b_, c_ = np.log(sp[i - 1:i + 2] + 1e-30)  # (the peak between bins: a parabola through three)
            fp = f[i] + 0.5 * (a_ - c_) / (a_ - 2 * b_ + c_) * (f[1] - f[0])
            out.append(1200 * math.log2(fp / f0) if sp[i] > 1e-6 * sp.max() + 1e-12 else 999)
        return out
    bad = []
    for stop, ratios in stops.items():
        keys = (36, 48, 67) if stop in ('subbass16', 'octavbass8', 'choralbass4', 'posaune16', 'untersatz32',
                                         'principalbass16', 'violonbass16', 'pedtrompete8', 'gedacktbass8',
                                         'contraposaune32', 'klarine4') else (48, 72, 96) if stop == 'celeste8' else (36, 72, 96)
        for k in keys:
            c = peaks(stop, k, ratios)
            if any(abs(v) > 5 for v in c):
                bad.append(f'{stop}@{k}: ' + ' '.join(f'{v:+.0f}' for v in c))
    for stop, comp in mix.items():
        for k, ratios in comp.items():
            c = peaks(stop, k, ratios)
            if any(abs(v) > 5 for v in c):
                bad.append(f'{stop}@{k}: ' + ' '.join(f'{v:+.0f}' for v in c))
    # The reeds die away when their keys are let go (their resonators' sound does not feed back through
    # the enclosures; the gap's flow viscous near no pressure).
    sang = []
    for stop, k in (('posaune16', 38), ('oboe8', 40), ('trompete8', 36), ('cromorne8', 48), ('fagott16', 36)):
        run(['-inst', 'organ', '-seq', f'on {k} 80 wait 1.0 off {k} wait 0.6', '-o', wav], env={'PJ_ORGAN_STOPS': stop})
        fs, x = wavfile.read(wav)
        x = np.abs(x.astype(float))
        held, tail = x[int(0.5 * fs):int(0.9 * fs)].max(), x[int(1.45 * fs):].max()
        if tail > 0.3 * held:
            sang.append(f'{stop}@{k} {20 * math.log10(tail / held):+.0f} dB')
    check('organ: the reeds die away after their keys', not sang, ', '.join(sang) or 'all five')
    # The reeds' treble (C5-C7 on the manuals' 8', the same keys on the 16' and the 4'; the pedal's top
    # fifth): every key in tune, steady (pitch and level over 1.5 s), speaking within 150 ms, full (its
    # 2nd or 3rd harmonic within 12 dB of the fundamental: not a sine) and on an even line (the five
    # keys' K-weighted levels within 6 dB: a reed's flow per unit of its scale rose ~27 dB to the top).
    # (speaking within 150 ms, the pedal's within 200)
    from levels import k_weight
    rbad, worst = [], {}
    reeds = {'trompete8': 1, 'trompete16': 0.5, 'trompette8': 1, 'oboe8': 1, 'clairon4': 2, 'cromorne8': 1,
             'fagott16': 0.5, 'dulcian16': 0.5, 'chamade8': 1, 'klarine4': 2, 'posaune16': 0.5, 'pedtrompete8': 1,
             'contraposaune32': 0.25}
    for stop, r in reeds.items():
        pedal = stop in ('klarine4', 'posaune16', 'pedtrompete8', 'contraposaune32')
        lv = []
        for k in ((55, 58, 61, 64, 67) if pedal else (72, 78, 84, 90, 96)):
            run(['-inst', 'organ', '-seq', f'on {k} 80 wait 2.0 off {k} wait 0.1', '-o', wav], env={'PJ_ORGAN_STOPS': stop, **EXACT})
            fs, x = wavfile.read(wav)
            x = x.astype(float)
            f0 = 440 * 2 ** ((k - 69) / 12) * (r / 2 if stop == 'clairon4' and k >= 84 else r)
            seg = x[int(0.4 * fs):int(1.9 * fs)]
            lv.append(10 * math.log10(np.mean(k_weight(seg, fs) ** 2) + 1e-30))
            sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 20))
            f = np.fft.rfftfreq(1 << 20, 1 / fs)
            def pk(fc, w):
                m = np.where((f > fc * (1 - w)) & (f < fc * (1 + w)))[0]
                i = m[sp[m].argmax()]
                a_, b_, c_ = np.log(sp[i - 1:i + 2] + 1e-30)
                return sp[i], f[i] + 0.5 * (a_ - c_) / (a_ - 2 * b_ + c_) * (f[1] - f[0])
            a1, fp = pk(f0, 0.03)
            cents = 1200 * math.log2(fp / f0)
            full = max(20 * math.log10(pk(h * fp, 0.005)[0] / a1) for h in (2, 3))
            cs, ls = [], []  # (200 ms frames: the pitch and the level as it is held)
            n = int(0.2 * fs)
            for s0 in range(int(0.4 * fs), int(1.9 * fs) - n, n // 2):
                y = x[s0:s0 + n]
                Y = np.abs(np.fft.rfft(y * np.hanning(n), 1 << 18))
                ff = np.fft.rfftfreq(1 << 18, 1 / fs)
                m = np.where((ff > fp * 0.97) & (ff < fp * 1.03))[0]
                i = m[Y[m].argmax()]
                a_, b_, c_ = np.log(Y[i - 1:i + 2] + 1e-30)
                cs.append(1200 * math.log2((ff[i] + 0.5 * (a_ - c_) / (a_ - 2 * b_ + c_) * (ff[1] - ff[0])) / fp))
                ls.append(10 * math.log10(np.mean(y ** 2) + 1e-30))
            e = np.sqrt(np.convolve(x ** 2, np.ones(240) / 240, 'same'))
            speech = np.argmax(e > 0.708 * e[int(1.0 * fs):int(1.9 * fs)].mean()) / fs * 1e3
            w = worst.setdefault(stop, [0, 0, 0, 0, -99])
            w[0], w[1], w[2], w[3], w[4] = max(w[0], abs(cents)), max(w[1], np.std(cs)), max(w[2], np.std(ls)), max(w[3], speech), max(w[4], -full)
            if abs(cents) > 5 or np.std(cs) > 1 or np.std(ls) > 0.5 or speech > (200 if pedal else 150) or full < -12:
                rbad.append(f'{stop}@{k}: {cents:+.1f} c, sd {np.std(cs):.1f} c / {np.std(ls):.1f} dB, {speech:.0f} ms, H2/3 {full:+.0f} dB')
        if max(lv) - min(lv) > 6:
            rbad.append(f'{stop}: its treble\'s line {min(lv):.0f}..{max(lv):.0f} dB')
        worst[stop].append(max(lv) - min(lv))
    check('organ: the reeds\' treble in tune, steady, prompt, full and even (13 stops, each its top two octaves)', not rbad,
          '; '.join(rbad) or 'worst: ' + ', '.join(f'{s} {w[0]:.1f} c sd {w[1]:.2f} c/{w[2]:.2f} dB {w[3]:.0f} ms H2/3 {-w[4]:+.0f} line {w[5]:.1f} dB'
                                              for s, w in worst.items()))
    # The Gamba's middle (C3-C5, its frein harmonique): every other key prompt (to 30 % of its held level
    # within 70 ms: no hesitation; to 90 % within 180, on average within 125: it was ~160, the octave
    # leading, the fundamental stalled), its pitch within 5 c of its held one by 180 ms (the harmonics'
    # instantaneous frequency: the swell box's notch on one does not hide it), in tune (3 c), steady
    # (1 c, 0.5 dB), a string's (its 2nd or 3rd within 15 dB of the fundamental, as the box colours it).
    from scipy.signal import butter as _butter, sosfiltfilt as _sff, hilbert as _hilbert
    gbad, gt90, gw = [], [], [0, 0, 0, 0, 0, 0, -99]
    for k in range(48, 73, 2):
        run(['-inst', 'organ', '-seq', f'on {k} 80 wait 2.0 off {k} wait 0.1', '-o', wav], env={'PJ_ORGAN_STOPS': 'gamba8', **EXACT})
        fs, x = wavfile.read(wav)
        x = x.astype(float)
        f0 = 440 * 2 ** ((k - 69) / 12)
        seg = x[int(1.0 * fs):int(1.9 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 20))
        f = np.fft.rfftfreq(1 << 20, 1 / fs)
        def gpk(fc, w):
            m = np.where((f > fc * (1 - w)) & (f < fc * (1 + w)))[0]
            i = m[sp[m].argmax()]
            a_, b_, c_ = np.log(sp[i - 1:i + 2] + 1e-30)
            return sp[i], f[i] + 0.5 * (a_ - c_) / (a_ - 2 * b_ + c_) * (f[1] - f[0])
        a1, fp = gpk(f0, 0.03)
        cents = 1200 * math.log2(fp / f0)
        full = max(20 * math.log10(gpk(h * fp, 0.005)[0] / a1) for h in (2, 3))
        n = max(240, int(2 * fs / fp))
        e = np.sqrt(np.convolve(x ** 2, np.ones(n) / n, 'same'))
        held = e[int(1.0 * fs):int(1.9 * fs)].mean()
        t30, t90 = np.argmax(e > 0.3 * held) / fs * 1e3, np.argmax(e > 0.9 * held) / fs * 1e3
        lsd = np.std(20 * np.log10(e[int(0.5 * fs):int(1.9 * fs):n] / held))
        w = int(0.02 * fs)
        num = den = 0
        for h in range(1, 5):
            z = _hilbert(_sff(_butter(2, [(h - 0.2) * fp, (h + 0.2) * fp], 'bandpass', fs=fs, output='sos'), x[:int(2.0 * fs)]))
            ph = np.unwrap(np.angle(z))
            ak = np.abs(z)[w // 2:w // 2 + len(ph) - w] ** 2
            num, den = num + (ph[w:] - ph[:-w]) / (2 * np.pi * w / fs) / h * ak, den + ak
        c = 1200 * np.log2(np.maximum(num / den, 1) / fp)
        off = np.where(np.abs(c[:int(1.9 * fs) - w]) >= 5)[0]
        settle = (off[-1] + w / 2) / fs * 1e3 if len(off) else 0
        psd = np.std(c[int(0.5 * fs):int(1.9 * fs) - w])
        gt90.append(t90)
        gw = [max(gw[0], t30), max(gw[1], t90), max(gw[2], settle), max(gw[3], abs(cents)), max(gw[4], psd), max(gw[5], lsd), max(gw[6], -full)]
        if t30 > 70 or t90 > 180 or settle > 180 or abs(cents) > 3 or psd > 1 or lsd > 0.5 or full < -15:
            gbad.append(f'@{k}: 30 % {t30:.0f} ms, 90 % {t90:.0f} ms, pitch {settle:.0f} ms, {cents:+.1f} c sd {psd:.1f} c / {lsd:.2f} dB, H2/3 {full:+.0f} dB')
    if np.mean(gt90) > 125:
        gbad.append(f'90 % on average {np.mean(gt90):.0f} ms')
    check('organ: the Gamba\'s middle prompt, sure, in tune, steady and a string\'s (C3-C5, its frein harmonique)', not gbad,
          '; '.join(gbad) or f'worst: 30 % {gw[0]:.0f} ms, 90 % {gw[1]:.0f} ms (mean {np.mean(gt90):.0f}), pitch within 5 c {gw[2]:.0f} ms, '
                             f'{gw[3]:.1f} c sd {gw[4]:.2f} c/{gw[5]:.2f} dB, H2/3 {-gw[6]:+.0f} dB')
    # The small stops' chiff (from C6 up; research/church_organ.md §8a): the first overtone leading at the
    # attack (its level against the fundamental's over their steady ratio, while the note is within 20 dB of
    # its held level) on average over G5-F#6 by +2..+12 dB on an open pipe, +1..+8 on a stopped one (its
    # twelfth: before, the stopped 4' top's led by ~+14..+16 dB, up to +50, overblown), its peak at least
    # 10 dB under the held tone (proportionate), each key its own (neighbours' leads spread >= 0.5 dB) and
    # each press its own (C6 struck five times: >= 0.5 dB; before, the flutes' attacks alike within ~0.3).
    cbad, cinfo = [], []
    keys = list(range(79, 91)) + [84] * 5
    seq = ' '.join(f'on {k} 80 wait 0.6 off {k} wait 0.35' for k in keys)
    for stop, r, ov in (('waldflote2', 4, 2), ('superoctave2', 4, 2), ('gedacktflote4', 2, 3)):
        run(['-inst', 'organ', '-seq', seq, '-o', wav], env={'PJ_ORGAN_STOPS': stop, **EXACT})
        fs, x = wavfile.read(wav)
        x = x.astype(float)
        leads, peaks = [], []
        for i, k in enumerate(keys):
            f0 = 440 * 2 ** ((k - 69) / 12) * r
            n = int(np.clip(6 * fs / f0, 96, 2048))
            win = np.hanning(n) / np.hanning(n).sum()
            ph = 2j * np.pi * np.arange(n) / fs
            i0 = int(i * 0.95 * fs)
            def frame(a):
                y = x[max(a, 0):a + n] if a >= 0 else np.concatenate([np.zeros(-a), x[:a + n]])
                return [abs((y * win * np.exp(-ph * h * f0)).sum()) ** 2 for h in (1, ov)] + [np.mean(y ** 2)]
            held = np.mean([frame(i0 + int(0.3 * fs) + j * 24) for j in range(500)], 0)
            att = np.array([frame(i0 - n // 2 + j * 24) for j in range(300)])
            db = lambda v: 10 * np.log10(v + 1e-30)
            rel = db(att[:, 1]) - db(att[:, 0]) - (db(held[1]) - db(held[0]))
            loud = db(att[:, 2]) - db(held[2]) > -20
            leads.append(rel[loud].max() if loud.any() else 0)
            peaks.append(db(att[:, 1].max()) - db(held[2]))
        main, rep = leads[:12], leads[12:]
        nb, ps = np.std(np.diff(main)) / np.sqrt(2), np.std(rep)
        cinfo.append(f'{stop} lead {np.mean(main):+.1f} dB (peak {max(peaks):+.0f} dB re the tone), keys sd {nb:.1f}, presses sd {ps:.2f} dB')
        lo, hi = (1, 8) if ov == 3 else (2, 12)
        if not (lo < np.mean(main) < hi and max(peaks) < -10 and nb >= 0.5 and ps >= 0.5):
            cbad.append(cinfo[-1])
    check('organ: the small stops\' chiff present, proportionate, varying key to key and press to press', not cbad,
          '; '.join(cbad or cinfo))
    # The crescendo roller (-inst organ-crescendo; K1 / -dyn): silence at 0, then each dynamic from ppp to
    # fff louder than the one before. Loudness as BS.1770 (K-weighted mean square: as heard, the upper
    # stops' brightness counted), power-averaged over three chords (C, Eb, G major) held 4 s: the unisons
    # of pipes a fraction of a Hz apart beat slowly, so one chord over 2 s reads +-0.5-1 dB by chance.
    from levels import k_weight
    chords = ((48, 55, 60, 64), (51, 58, 63, 67), (43, 50, 55, 59))
    lv = []
    for d in ('0.02', '0.155', '0.275', '0.395', '0.515', '0.635', '0.755', '0.875', '0.995'):
        ms = []
        for ch in chords[:1] if d == '0.02' else chords:
            # (in the interval table's tuning: the registrations' levels, not the tuning's; by the spectra a
            # few unisons beat differently, the 3 chords' level moving a few tenths of a dB)
            run(['-inst', 'organ-crescendo', '-dyn', d, '-seq', ' '.join(f'on {k} 80' for k in ch) + ' wait 4 ' +
                 ' '.join(f'off {k}' for k in ch) + ' wait 0.1', '-o', wav], env={'PJ_JI_SPECTRAL': '0'})
            fs, x = wavfile.read(wav)
            x = x.astype(float)[int(0.8 * fs):int(3.9 * fs)]
            ms.append(np.mean(k_weight(x, fs) ** 2))
        lv.append(10 * math.log10(np.mean(ms) + 1e-30))
    steps = np.diff(lv[1:])
    # (every step louder; from mf to f, f to ff and ff to fff each at least +1 dB, fff +1.2 over ff and +3
    # over f: the plenum's mixtures, the 8' reed chorus, then the swell's reed battery, the 16' and 4' reeds,
    # the 32' and the chamade, each reed regulated against the Principal 8')
    check('organ: the crescendo roller, silence to fff', lv[0] < -150 and all(steps > 0.5) and all(steps[4:] >= 1.0)
          and lv[-1] - lv[-2] >= 1.2 and lv[-1] - lv[-3] >= 3 and lv[-1] - lv[1] > 15,
          f'ppp {lv[1]:.0f} dB, then ' + ' '.join(f'{v:+.1f}' for v in steps) + f' (fff {lv[-1] - lv[1]:+.0f} dB over ppp)')
    # No clicks as pipes start and stop in a texture: three overlapping lines (a trio's), the highs checked
    # for bursts over the median.
    from scipy.signal import butter, sosfilt
    seq, t = [], 0.0
    for i in range(40):
        a1, a2, a3 = 60 + (i * 5) % 12, 55 + (i * 7) % 10, 36 + (i * 3) % 12
        seq.append(f'on {a1} 80 on {a2} 80' + (f' on {a3} 80' if i % 2 == 0 else '') + ' wait 0.2')
        seq.append(f'off {a1} off {a2}' + (f' off {a3}' if i % 2 == 0 else ''))
    run(['-inst', 'organ', '-seq', ' '.join(seq) + ' wait 0.5', '-o', wav], env={'PJ_ORGAN_STOPS': 'rohrflote8,octave4,rohrgedackt8,gedacktflote4,subbass16,octavbass8'})
    fs, y = wavfile.read(wav)
    hp = sosfilt(butter(4, 8000, 'highpass', fs=fs, output='sos'), y.astype(float))
    e = np.sqrt(np.convolve(hp ** 2, np.ones(48) / 48, 'same'))
    worst = 20 * math.log10(e.max() / (np.median(e) + 1e-12))
    check('organ: no clicks as pipes start in a texture (the head start smooth)', worst < 30, f'worst high burst {worst:.0f} dB over the median')
    # The pajupilli intonation (PJ_ORGAN_JI, on by default): a held C major triad. By the interval table
    # (PJ_JI_SPECTRAL=0) its third just (386 c), its fifth just (702 c); by the spectra (the default) a
    # cornet's third just (its Terz's partials meet the E's), a Gedackt's near equal temperament (its odd
    # partials meet nothing there: nothing beats), the fifths just within 2.5 c.
    def triad(stops, spectral):
        run(['-inst', 'organ', '-seq', 'on 48 80 on 52 80 on 55 80 wait 2.5 off 48 off 52 off 55', '-o', wav],
            env={'PJ_ORGAN_STOPS': stops, 'PJ_JI_SPECTRAL': spectral, **EXACT})
        fs, y = wavfile.read(wav)
        seg = y.astype(float)[int(1.2 * fs):int(2.4 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 21))
        f = np.fft.rfftfreq(1 << 21, 1 / fs)
        def pk(f0):
            m = np.where((f > f0 * 0.98) & (f < f0 * 1.02))[0]
            i = m[sp[m].argmax()]
            a_, b_, c_ = np.log(sp[i - 1:i + 2])
            return f[i] + 0.5 * (a_ - c_) / (a_ - 2 * b_ + c_) * (f[1] - f[0])
        c, e, g = (pk(440 * 2 ** ((k - 69) / 12)) for k in (48, 52, 55))
        return 1200 * math.log2(e / c), 1200 * math.log2(g / c)
    third, fifth = triad('principal8', '0')
    check('organ: the pajupilli intonation by the table (a triad: the third and the fifth just)', abs(third - 386.3) < 3 and abs(fifth - 702.0) < 2,
          f'third {third:.1f} c, fifth {fifth:.1f} c')
    t1, f1 = triad('gedackt8,flute4,nasard223,waldflote2,terz135', '1')
    t2, f2 = triad('gedackt8', '1')
    check('organ: the pajupilli intonation by the spectra (a cornet\'s third just, a Gedackt\'s near equal)',
          abs(t1 - 386.3) < 4 and abs(t2 - 400) < 4 and abs(f1 - 702.0) < 2.5 and abs(f2 - 702.0) < 2.5,
          f'cornet third {t1:.1f} c, fifth {f1:.1f} c; Gedackt third {t2:.1f} c, fifth {f2:.1f} c')
    # The 88-key keyboard in zones (-inst organ-88): A0-B2 the pedal (an octave up: C1 plays the 16' C), C3-E4
    # the Schwellwerk, F4 up the Hauptwerk / Positiv.
    got = []
    for k, f in ((24, 32.7), (52, 164.8), (72, 523.3)):
        out = run(['-inst', 'organ-88', '-dyn', '0.6', '-note', str(k), '-secs', '0.8']).stdout
        m = re.search(r'played ([0-9.]+) Hz', out)
        got.append((k, f, float(m.group(1)) if m else 0))
    check('organ: the 88-key zones (pedal, swell, Hauptwerk)', all(abs(1200 * math.log2(max(g, 1) / f)) < 10 for _, f, g in got),
          ', '.join(f'key {k}: {g:.1f} Hz' for k, _, g in got))
    # The wind (src/organwind.h): one stop, one note barely moves its chest; the full organ (every stop)
    # with a 10-note chord held sags it ~5-12 % (its flues ~4 c flat, as the user chose).
    def lowest(stops, keys, hold):
        seq = ' '.join(f'on {k} 80' for k in keys) + f' wait {hold} ' + ' '.join(f'off {k}' for k in keys) + ' wait 0.1'
        r = run(['-inst', 'organ', '-seq', seq, '-o', wav], env={'PJ_ORGAN_STOPS': stops, 'PJ_WIND_LOG': '1'})
        m = re.search(r'lowest chest ([0-9.]+)', r.stderr)
        return float(m.group(1)) if m else 0
    every = ','.join(list(stops) + list(mix) + ['principal8', 'gedackt8', 'rohrgedackt8'])
    one, full = lowest('gedackt8', [60], 1.0), lowest(every, [36, 48, 55, 60, 64, 67, 72, 76, 79, 84], 4.0)
    # The tremulant (a beater working the swell's regulator): the chest swings ~10-20 % p-p at ~5.5 Hz.
    r = run(['-inst', 'organ', '-seq', 'on 72 80 wait 2 off 72 wait 0.1', '-o', wav], env={'PJ_ORGAN_STOPS': 'gedackt8,tremsw', 'PJ_WIND_LOG': '1'})
    m = re.search(r'lowest chest ([0-9.]+)', r.stderr)
    trem = 1 - float(m.group(1)) if m else 0
    check('organ: the tremulant swings the wind', 0.04 < trem < 0.12, f'the swell chest down to -{100 * trem:.1f} %')
    check('organ: the wind holds one note (< 0.5 % sag) and sags the full organ 5-12 %',
          one > 0.995 and 0.88 < full < 0.95, f'one note {100 * (1 - one):.2f} %, full organ {100 * (1 - full):.1f} %')
    if os.path.exists(wav):
        os.remove(wav)
    check('organ: every stop speaks at its pitch (cents within 5)', not bad, '; '.join(bad) or f'{len(stops) + len(mix)} stops')


def hammond():
    """The Hammond B-3 and its Leslie (-inst hammond, drawbar-organ, percussive-organ, rock-organ;
    src/hammond.cc, src/leslie.cc, src/hammondplan.cc; research/hammond_b3.md, leslie.md,
    hammond_character.md): the wheels geared as built (A 440 exact, C -0.58 c), phase-locked (a
    chord the same whenever it is played), each with its flaws (sidebands at its rotation rate);
    a key click never the same twice; single-trigger percussion; the Leslie's horn at its speed;
    a MIDI file's hands, pedals and drawbars planned; stable, real time."""
    import struct
    import numpy as np
    from scipy.io import wavfile
    from scipy.signal import butter, lfilter
    wav = os.path.join(ROOT, 'tools', '.hammond.wav')
    def render(seq, env=None, inst='hammond'):
        run(['-inst', inst, '-seq', seq, '-o', wav], env=env)
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float)
        return fs, x.mean(1) if x.ndim > 1 else x
    dry = {'PJ_HAMMOND_UPPER': '008000000', 'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_LESLIE': 'off', 'PJ_HAMMOND_DIRT': '0'}
    fs, x = render('on 69 100 on 84 100 wait 8 off 69 off 84', dict(dry, PJ_HAMMOND_MAINS='0', PJ_HAMMOND_FLUTTER='0'))  # (the gears alone)
    seg = x[int(1 * fs):int(7.5 * fs)]
    N = 1 << 21
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), N))
    f = np.fft.rfftfreq(N, 1 / fs)
    def pk(hz, w=0.3):
        m = (f > hz - w) & (f < hz + w)
        return 20 * math.log10(sp[m].max() + 1e-12), f[m][sp[m].argmax()]
    (la, fa), (lc, fc) = pk(440), pk(20 * 85 / 104 * 64)
    ca, cc = 1200 * math.log2(fa / 440), 1200 * math.log2(fc / 1046.502)
    check('hammond: the wheels geared as built (A4 exact, C6 -0.58 c)', abs(ca) < 0.1 and abs(cc + 0.58) < 0.15, f'A4 {ca:+.2f} c, C6 {cc:+.2f} c')
    side = [pk(440 - 27.5)[0] - la, pk(440 + 27.5)[0] - la]
    check('hammond: each wheel\'s flaws: sidebands at its rotation rate (A4 +-27.5 Hz)', all(-45 < s_ < -20 for s_ in side),
          ', '.join(f'{s_:.0f} dB' for s_ in side))
    # Phase lock: the same chord begun at two moments sounds the same at the same moment after.
    a = render('wait 0.1 on 60 100 on 64 100 on 67 100 wait 1.2 off 60 off 64 off 67', dry)[1]
    b = render('wait 0.37 on 60 100 on 64 100 on 67 100 wait 0.93 off 60 off 64 off 67', dry)[1]
    wa, wb = a[int(1.0 * fs):int(1.2 * fs)], b[int(1.0 * fs):int(1.2 * fs)]
    diff = 20 * math.log10(np.sqrt(np.mean((wa - wb) ** 2)) / np.sqrt(np.mean(wa ** 2)) + 1e-12)
    check('hammond: one generator, phase-locked (a chord the same whenever begun)', diff < -40, f'difference {diff:.0f} dB')
    # The organic flaws: each wheel's flutter on its coupling (a few tenths of a cent), the grid's
    # slow drift (under a cent), a floor of hum and leakage under it all.
    from scipy.signal import hilbert, filtfilt, butter as _bt
    fs, x = render('on 96 100 wait 6 off 96 wait 2', dict(dry, PJ_HAMMOND_MAINS='0'))
    bb, aa = _bt(2, [(2092.3 - 25) / (fs / 2), (2092.3 + 25) / (fs / 2)], 'band')
    an = hilbert(filtfilt(bb, aa, x[int(0.5 * fs):int(5.5 * fs)]))
    inst = np.convolve(np.diff(np.unwrap(np.angle(an))) * fs / (2 * np.pi), np.ones(480) / 480, 'same')[2000:-2000]
    flutter = (1200 * np.log2(inst / 2092.3)).std()
    floor = 10 * math.log10(np.mean(x[int(6.6 * fs):int(7.9 * fs)] ** 2) / np.mean(x[int(1 * fs):int(5 * fs)] ** 2))
    fs, x = render('on 69 100 wait 30 off 69', dict(dry, PJ_HAMMOND_FLUTTER='0'))
    bb, aa = _bt(2, [434 / (fs / 2), 446 / (fs / 2)], 'band')
    an = hilbert(filtfilt(bb, aa, x[int(1 * fs):int(29 * fs)]))
    inst = np.convolve(np.diff(np.unwrap(np.angle(an))) * fs / (2 * np.pi), np.ones(4800) / 4800, 'same')[9600:-9600]
    drift = 1200 * np.log2(inst[::4800] / 440)
    check('hammond: organic flaws: the wheels\' flutter, the grid\'s drift, a floor of hum and leakage',
          0.1 < flutter < 1.0 and 0.1 < drift.max() - drift.min() < 2.5 and -75 < floor < -40,
          f'flutter {flutter:.2f} c rms; drift {drift.min():+.2f}..{drift.max():+.2f} c; floor {floor:.0f} dB under a note')
    # The click: above 5 kHz in a press's first 5 ms, louder than the tone, never twice the same.
    seq = ''.join('on 60 100 wait 0.4 off 60 wait 0.2 ' for _ in range(5))
    fs, x = render(seq, {'PJ_HAMMOND_UPPER': '888888888', 'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_LESLIE': 'off'})
    bb, aa = butter(4, 5000 / (fs / 2), 'high')
    h = lfilter(bb, aa, x)
    ons = [h[int(i * 0.6 * fs):int(i * 0.6 * fs) + 240] for i in range(5)]
    over = [10 * math.log10(np.mean(o ** 2) / (np.mean(h[int((i * 0.6 + 0.2) * fs):int((i * 0.6 + 0.2) * fs) + 240] ** 2) + 1e-15)) for i, o in enumerate(ons)]
    corr = max(abs(np.corrcoef(ons[0], o)[0, 1]) for o in ons[1:])
    check('hammond: the key click, never the same twice', min(over) > 6 and corr < 0.9, f'{min(over):.0f}..{max(over):.0f} dB over the tone; likeness at most {corr:.2f}')
    # Velocity as the key's speed: a hard key slams the contacts (a sharp, louder click), a soft one
    # lays them on (a soft click); the percussion's ping by its touch.
    clicks = []
    for v in (30, 127):
        seq_ = ''.join(f'on 60 {v} wait 0.4 off 60 wait 0.2 ' for _ in range(5))
        fs, x = render(seq_, {'PJ_HAMMOND_UPPER': '888888888', 'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_LESLIE': 'off', 'PJ_HAMMOND_DIRT': '0'})
        hh = lfilter(*butter(4, 5000 / (fs / 2), 'high'), x)
        clicks.append(np.mean([10 * math.log10(np.mean(hh[int(i * 0.6 * fs):int(i * 0.6 * fs) + 240] ** 2) /
                                               (np.mean(hh[int((i * 0.6 + 0.2) * fs):int((i * 0.6 + 0.2) * fs) + 240] ** 2) + 1e-15)) for i in range(5)]))
    pings = []
    for v in (30, 127):
        fs, x = render(f'on 60 {v} wait 1 off 60', {'PJ_HAMMOND_PERC': '3f', 'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_LESLIE': 'off'})
        pings.append(20 * math.log10(np.abs(lfilter(*butter(2, [700 / (fs / 2), 900 / (fs / 2)], 'band'), x)[:int(0.1 * fs)]).max()))
    check('hammond: velocity as the key\'s speed (the click, the percussion\'s touch)',
          clicks[1] - clicks[0] > 6 and 2 < pings[1] - pings[0] < 8,
          f'click {clicks[0]:.0f} -> {clicks[1]:.0f} dB over the tone; ping {pings[1] - pings[0]:+.1f} dB (velocity 30 -> 127)')
    from scipy.signal import hilbert, sosfiltfilt
    # The Leslies: four cabinets by default, each its parts off (+-7 %), their speeds the factors
    # away from simple ratios (the centre 1.172, left 1.081, right 0.953, back 0.827): the second
    # (left) 0.922 x the first (centre), with the published horn/drum ratio (1.20 on slow).
    r_ = run(['-inst', 'hammond', '-seq', 'on 60 90 wait 0.3 off 60', '-o', wav], env={'PJ_LESLIE_LOG': '1', 'PJ_LESLIE_LOG2': '1'})
    os.remove(wav)
    c1 = [list(map(float, l_.split()[2::2])) for l_ in r_.stderr.splitlines() if l_.startswith('leslie t')]
    c2 = [list(map(float, l_.split()[2::2])) for l_ in r_.stderr.splitlines() if l_.startswith('leslie2 t')]
    sp_, ra_ = c2[0][1] / c1[0][1], c2[0][1] / c2[0][2]
    check('hammond: several Leslies, each its own (the second 0.92 x the first, the published horn/drum ratio)',
          0.89 < sp_ < 0.95 and 1.10 < ra_ < 1.30,
          f'the second\'s horn {sp_:.2f} x the first\'s; its horn/drum {ra_:.2f} (the first\'s {c1[0][1] / c1[0][2]:.2f})')
    # Each Leslie its own mix of the manuals: the upper mostly in the centre one, the lower in the
    # left and right, the pedals in the one behind (three tracks: upper 0-2 s, lower 3-5 s, pedals
    # 6-8 s; each cabinet's input level logged).
    def vlq_(n):
        b_ = [n & 0x7f]
        n >>= 7
        while n:
            b_.append(0x80 | (n & 0x7f))
            n >>= 7
        return bytes(reversed(b_))
    def trk(ch, notes):
        ev2, out2, last2 = [(0, [0xC0 | ch, 16])], b'', 0
        for st, p_, ln in notes:
            ev2 += [(st, [0x90 | ch, p_, 90]), (st + ln, [0x80 | ch, p_, 0])]
        ev2.sort(key=lambda e2: (e2[0], e2[1][0] & 0xf0 == 0x90))
        for tk, e2 in ev2:
            out2 += vlq_(tk - last2) + bytes(e2)
            last2 = tk
        out2 += b'\x00\xff\x2f\x00'
        return b'MTrk' + struct.pack('>I', len(out2)) + out2
    mid_ = os.path.join(ROOT, 'tools', '.hammond_mix.mid')
    with open(mid_, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 1, 3, 480) + trk(0, [(0, 76, 1920), (7680, 76, 480)]) +
                 trk(1, [(2880, 60, 1920), (2880, 64, 1920)]) + trk(2, [(5760, 36, 1920)]))
    r_ = run(['-play', mid_, '-o', wav], env={'PJ_LESLIE_MIXLOG': '1'})
    os.remove(mid_)
    if os.path.exists(wav):
        os.remove(wav)
    lv = {int(m_.group(1)): [float(v_) for v_ in re.findall(r'(-?[\d.]+) dB', m_.group(2))]
          for m_ in re.finditer(r'leslie inputs t (\d+):((?: -?[\d.]+ dB)+)', r_.stderr)}
    ok_ = all(k_ in lv and len(lv[k_]) == 4 for k_ in (2, 5, 8))
    if ok_:
        up_, lo_, pe_ = lv[2], lv[5], lv[8]
        ok_ = (up_[0] > max(up_[1:]) + 10 and min(lo_[1], lo_[2]) > max(lo_[0], lo_[3]) + 10 and abs(lo_[1] - lo_[2]) < 1
               and pe_[3] > max(pe_[:3]) + 10)
    check('hammond: each Leslie its own mix (upper: the centre; lower: left and right; pedals: the one behind)', ok_,
          ' | '.join(f'{n_}: ' + ' '.join(f'{v_:.0f}' for v_ in lv.get(k_, [])) for n_, k_ in (('upper', 2), ('lower', 5), ('pedals', 8))) + ' dB (centre, left, right, back)')
    # The valve stage: a triode's 2nd-order curve (C4 and E4 held: their difference tone, 68 Hz,
    # appears only through it).
    dts = []
    for v_ in ('0', '1'):
        run(['-inst', 'hammond', '-seq', 'on 60 100 on 64 100 wait 3 off 60 off 64', '-room', 'dry', '-o', wav],
            env={'PJ_HAMMOND_UPPER': '008000000', 'PJ_HAMMOND_LESLIE': 'off', 'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_PERC': 'off', 'PJ_HAMMOND_VALVE': v_})
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float)
        x = x.mean(1) if x.ndim > 1 else x
        seg = x[int(0.5 * fs):int(2.5 * fs)]
        sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg))))
        f = np.fft.rfftfreq(len(seg), 1 / fs)
        pk = lambda fr: sp[(f > fr - 1) & (f < fr + 1)].max()
        dts.append(20 * math.log10(pk(329.63 - 261.63) / pk(261.63)))
    check('hammond: the valve stage\'s even-order distortion (C4 + E4: their difference tone)',
          dts[1] > dts[0] + 15 and dts[1] < -25, f'68 Hz {dts[0]:.0f} dB without, {dts[1]:.0f} dB with the valve')
    # The machined wheels' overtones: an unfiltered wheel (C3's 8', wheel 25) carries its 2nd, 3rd
    # and 5th at ~-30..-36 dB; the 5th of middle C beats against the tempered 1 3/5' wheel (E6,
    # ~14 c sharp of it) at ~10.4 Hz.
    def wheel_render(m_, upper, extra=None):
        env_ = {'PJ_HAMMOND_UPPER': upper, 'PJ_HAMMOND_LESLIE': 'off', 'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_PERC': 'off', 'PJ_HAMMOND_DIRT': '0'}
        env_.update(extra or {})
        run(['-inst', 'hammond', '-seq', f'on {m_} 100 wait 4 off {m_}', '-room', 'dry', '-o', wav], env=env_)
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float)
        return fs, x.mean(1) if x.ndim > 1 else x
    fs, x = wheel_render(48, '008000000', {'PJ_HAMMOND_BRIGHT': '0'})
    seg = x[int(0.5 * fs):int(2.5 * fs)]
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg))))
    f = np.fft.rfftfreq(len(seg), 1 / fs)
    pk = lambda fr: sp[(f > fr * 0.995) & (f < fr * 1.005)].max()
    f0 = 440 * 2 ** ((48 - 69) / 12)
    hs = [20 * math.log10(pk(k_ * f0) / pk(f0)) for k_ in (2, 3, 5)]
    fs, x = wheel_render(60, '008000800')
    f0 = 440 * 2 ** ((60 - 69) / 12)
    beat = f0 * 2 ** (28 / 12) - 5 * f0
    y = sosfiltfilt(butter(4, [5 * f0 * 0.96, f0 * 2 ** (28 / 12) * 1.04], 'band', fs=fs, output='sos'), x)[int(0.5 * fs):int(3.5 * fs)]
    e_ = np.abs(hilbert(y))
    e_ = e_ / e_.mean()
    sp = np.abs(np.fft.rfft((e_ - 1) * np.hanning(len(e_)))) * 4 / len(e_)
    f = np.fft.rfftfreq(len(e_), 1 / fs)
    swing = sp[(f > beat - 0.4) & (f < beat + 0.4)].max()
    check('hammond: the machined wheels\' overtones (and their beating against the tempered drawbars)',
          all(-42 < h_ < -24 for h_ in hs) and swing > 0.01,
          f'C3 wheel: 2nd {hs[0]:.0f}, 3rd {hs[1]:.0f}, 5th {hs[2]:.0f} dB; middle C\'s 5th against the 1 3/5\': a {beat:.1f} Hz beat, +-{100 * swing:.1f} %')
    # Each wheel's own subharmonics: its rotation rate and that rate's harmonics, far under the tone
    # (C3's wheel 25: 8 teeth, turning 16.35 Hz: lines at 16.35 .. 114.5 Hz, -35..-60 dB).
    fs, x = wheel_render(48, '008000000', {'PJ_HAMMOND_BRIGHT': '0'})
    seg = x[int(0.5 * fs):int(3.5 * fs)]
    sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg))))
    f = np.fft.rfftfreq(len(seg), 1 / fs)
    f0 = 440 * 2 ** ((48 - 69) / 12)
    pk = lambda fr: sp[(f > fr - 0.6) & (f < fr + 0.6)].max()
    subs = [20 * math.log10(pk(k_ * f0 / 8) / pk(f0)) for k_ in range(1, 8)]
    check('hammond: each wheel\'s subharmonics (its rotation rate and that rate\'s harmonics, far under the tone)',
          all(-65 < d_ < -30 for d_ in subs),
          'C3 wheel at 16.35 x 1..7 Hz: ' + ' '.join(f'{d_:.0f}' for d_ in subs) + ' dB')
    # The two wheels on one shaft (four octaves apart: C2's wheel 13 and C6's 61, C3's 25 and C5's
    # 73) share its wobble and its drive's errors, and no other shaft does: their level swells and
    # pitch wobbles move together; C2 and C3 (two shafts of one gear ratio) apart.
    from scipy.signal import hilbert, sosfiltfilt
    fm_, am_ = {}, {}
    for m_ in (36, 84, 48, 72):
        run(['-inst', 'hammond', '-seq', f'on {m_} 100 wait 6 off {m_}', '-room', 'dry', '-o', wav],
            env={'PJ_HAMMOND_UPPER': '008000000', 'PJ_HAMMOND_LESLIE': 'off', 'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_DIRT': '0', 'PJ_HAMMOND_PERC': 'off',
                 'PJ_HAMMOND_SUBS': '-200', 'PJ_HAMMOND_VALVE': '0', 'PJ_HAMMOND_MICRO': '-200', 'PJ_HAMMOND_MICROMOD': '0'})
        # (the subharmonics next to the tone read as a pitch wobble of their own; the valve and its
        # microphonics modulate every wheel alike: the generator measured alone)
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float)
        x = x.mean(1) if x.ndim > 1 else x
        f0 = 440 * 2 ** ((m_ - 69) / 12)
        y = sosfiltfilt(butter(2, [f0 * 0.8, f0 * 1.2], 'band', fs=fs, output='sos'), x)[int(1 * fs):int(5.5 * fs)]
        a_ = hilbert(y)
        c_ = 1200 * np.log2(np.maximum(np.diff(np.unwrap(np.angle(a_))) * fs / (2 * np.pi), 1) / f0)
        band_ = butter(2, [2, 40], 'band', fs=fs, output='sos')
        c_ = sosfiltfilt(butter(2, [18, 22], 'bandstop', fs=fs, output='sos'), sosfiltfilt(band_, c_))  # (the drive shaft's 20 Hz: every wheel's)
        e_ = np.abs(a_)
        e_ = sosfiltfilt(band_, e_ / e_.mean())
        fm_[m_], am_[m_] = c_[fs:-fs][::48], e_[fs:-fs][::48]
    cc_ = lambda u, w: abs(np.corrcoef(u, w)[0, 1])
    one = (cc_(am_[36], am_[84]) + cc_(am_[48], am_[72])) / 2, (cc_(fm_[36], fm_[84]) + cc_(fm_[48], fm_[72])) / 2
    two = cc_(am_[36], am_[48]), cc_(fm_[36], fm_[48])
    check('hammond: the two wheels on a shaft share its wobble and drive errors (no other shaft does)',
          one[0] > 0.6 and two[0] < 0.4 and cc_(fm_[36], fm_[84]) > two[1] + 0.2,
          f'level swell together {one[0]:.2f} on a shaft, {two[0]:.2f} across; pitch wobble C2-C6 {cc_(fm_[36], fm_[84]):.2f} / C2-C3 {two[1]:.2f}')
    # Percussion: the 3rd's fast ping; legato does not strike it again.
    fs, x = render('on 60 100 wait 1.5 off 60 wait 0.3 on 62 100 wait 0.5 on 64 100 off 62 wait 1 off 64',
                   {'PJ_HAMMOND_PERC': '3f', 'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_LESLIE': 'off'})
    bb, aa = butter(2, [700 / (fs / 2), 900 / (fs / 2)], 'band')
    e = np.sqrt(np.convolve(lfilter(bb, aa, x) ** 2, np.ones(480) / 480, 'same'))
    p0 = e[:int(0.1 * fs)].max()
    t20 = np.where(e[int(0.02 * fs):int(1.5 * fs)] < 0.1 * p0)[0]
    t20 = (t20[0] / fs + 0.02) * 1e3 if len(t20) else 9999
    bb, aa = butter(2, [820 / (fs / 2), 1000 / (fs / 2)], 'band')
    e2 = np.sqrt(np.convolve(lfilter(bb, aa, x) ** 2, np.ones(480) / 480, 'same'))
    rest, legato = e2[int(1.8 * fs):int(2.0 * fs)].max(), e2[int(2.3 * fs):int(2.5 * fs)].max()
    check('hammond: percussion (3rd, fast), single-trigger', 100 < t20 < 300 and legato < 0.3 * rest,
          f'to -20 dB in {t20:.0f} ms; legato ping {20 * math.log10(legato / rest + 1e-12):.0f} dB under one after a rest')
    # The Leslie fast: the horn's swirl at its speed.
    fs, x = render('on 72 100 wait 4 off 72', {'PJ_HAMMOND_LESLIE': 'fast'})
    bb, aa = butter(2, [1000 / (fs / 2), 1200 / (fs / 2)], 'band')
    env = np.sqrt(np.convolve(lfilter(bb, aa, x[int(1 * fs):int(4 * fs)]) ** 2, np.ones(240) / 240, 'same'))
    L = 20 * np.log10(env + 1e-9)
    L -= L.mean()
    sp = np.abs(np.fft.rfft(L * np.hanning(len(L)), 1 << 18))
    f = np.fft.rfftfreq(1 << 18, 1 / fs)
    m = (f > 2) & (f < 12)
    rate = f[m][sp[m].argmax()]
    check('hammond: the Leslie\'s horn fast (~6.7 Hz)', 6.3 < rate < 7.1, f'{rate:.2f} Hz, {np.percentile(L, 95) - np.percentile(L, 5):.0f} dB p-p')
    # The plan: one track with a tune, chords under it and a bass.
    def vlq(n):
        b_ = [n & 127]
        n >>= 7
        while n:
            b_.insert(0, (n & 127) | 128)
            n >>= 7
        return bytes(b_)
    ev = [(0, [0xC0, 16])]
    for i, m_ in enumerate([72, 74, 76, 77, 79, 77, 76, 74] * 2):
        on = i * 480
        ev += [(on, [0x90, m_, 90]), (on + 460, [0x80, m_, 0])]
        if i % 2 == 0:
            for c_ in (60, 64, 67):
                ev += [(on, [0x90, c_, 70]), (on + 940, [0x80, c_, 0])]
            b_ = 36 if (i // 2) % 2 == 0 else 43
            ev += [(on, [0x90, b_, 80]), (on + 920, [0x80, b_, 0])]
    ev.sort(key=lambda e_: (e_[0], e_[1][0] == 0x90))
    trk, last = b'', 0
    for tk, e_ in ev:
        trk += vlq(tk - last) + bytes(e_)
        last = tk
    trk += b'\x00\xff\x2f\x00'
    mid = os.path.join(ROOT, 'tools', '.hammond.mid')
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(trk)) + trk)
    r = run(['-play', mid, '-o', wav])
    os.remove(mid)
    if os.path.exists(wav):
        os.remove(wav)
    got = re.search(r'notes: (\d+) upper, (\d+) lower, (\d+) pedals', r.stderr)
    counts = tuple(int(g) for g in got.groups()) if got else None
    check('hammond: a MIDI file planned: the tune upper, the chords lower, the bass the pedals', counts == (16, 24, 8),
          f'{counts} (upper, lower, pedals)')
    # Three tracks (an organist's three staves) with the Leslie on CC82: the pedals and two
    # manuals, the file's Leslie.
    def track(evs):
        evs = sorted(evs, key=lambda e_: (e_[0], e_[1][0] & 0xf0 == 0x90))
        out, last = b'', 0
        for t, e_ in evs:
            out += vlq(t - last) + bytes(e_)
            last = t
        out += b'\x00\xff\x2f\x00'
        return b'MTrk' + struct.pack('>I', len(out)) + out
    t0 = [(0, [0xC0, 16]), (0, [0xB0, 82, 100]), (7680, [0xB0, 82, 60])]
    tu, tl, tp = [], [], []
    for i in range(16):
        m_ = [72, 74, 76, 77][i % 4]
        tu += [(i * 480, [0x90, m_, 90]), (i * 480 + 460, [0x80, m_, 0])]
        if i % 4 == 0:
            for c_ in (55, 60, 64):
                tl += [(i * 480, [0x90, c_, 70]), (i * 480 + 1900, [0x80, c_, 0])]
            b_ = [36, 41, 43, 36][(i // 4) % 4]
            tp += [(i * 480, [0x90, b_, 80]), (i * 480 + 1890, [0x80, b_, 0])]
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 1, 4, 480) + track(t0) + track(tu) + track(tl) + track(tp))
    r = run(['-play', mid, '-o', wav])
    got = re.search(r'track 1: (\w+); channel \d+ track 2: (\w+); channel \d+ track 3: (\w+);.*Leslie (.*)', r.stderr)
    check('hammond: three tracks: two manuals and the pedals; the file\'s Leslie (CC82)',
          got is not None and got.groups()[:3] == ('upper', 'lower', 'pedals') and got.group(4).startswith("the file"),
          ' / '.join(got.groups()) if got else r.stderr[-200:])
    # The Leslie's three parts at the microphones (fast, a C6 wheel; a full chord): the Doppler
    # (pitch), the tremolo (level), the cabinet's reflections (each partial swinging its own way).
    from scipy.signal import hilbert, filtfilt
    def held(notes, env):
        ev_ = [(0, [0xC0, 16])] + [(0, [0x90, n_, 100]) for n_ in notes] + [(4800, [0x80, notes[0], 0])] + [(0, [0x80, n_, 0]) for n_ in notes[1:]]
        trk_ = b''.join(vlq(d) + bytes(e_) for d, e_ in ev_) + b'\x00\xff\x2f\x00'
        with open(mid, 'wb') as fh:
            fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 960) + b'MTrk' + struct.pack('>I', len(trk_)) + trk_)
        run(['-play', mid, '-room', 'dry', '-o', wav], env=env)
        fs_, y = wavfile.read(wav)
        os.remove(wav)
        return fs_, y.astype(float)
    fast = {'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_PERC': 'off', 'PJ_HAMMOND_LESLIE': 'fast'}
    fs, y = held([84], dict(fast, PJ_HAMMOND_UPPER='008000000'))
    bb, aa = butter(2, [950 / (fs / 2), 1150 / (fs / 2)], 'band')
    an = hilbert(filtfilt(bb, aa, y[int(0.6 * fs):int(2.4 * fs), 0]))
    inst = np.convolve(np.diff(np.unwrap(np.angle(an))) * fs / (2 * np.pi), np.ones(96) / 96, 'same')[500:-500]
    cents = 1200 * np.log2(np.clip(inst, 500, 2000) / 1046.15)
    dop = np.percentile(cents, 95) - np.percentile(cents, 5)
    am = 20 * np.log10(np.abs(an)[500:-500] + 1e-9)
    trem = np.percentile(am, 95) - np.percentile(am, 5)
    fs, y = held([72, 76, 79, 84], dict(fast, PJ_HAMMOND_UPPER='888888888'))
    win, hop = 2048, int(fs / 6.67 / 16)
    fr = np.array([20 * np.log10(np.abs(np.fft.rfft(y[i:i + win, 0] * np.hanning(win))) + 1e-9)
                   for i in range(int(0.6 * fs), int(2.4 * fs) - win, hop)])
    hz = [440 * 2 ** ((m_ - 69) / 12) * k for m_ in (72, 76, 79, 84) for k in (1, 2, 3, 4, 6, 8)]
    idx = sorted(set(int(round(h_ / (fs / win))) for h_ in hz if 900 < h_ < 7000))
    dev = fr[:, idx] - fr[:, idx].mean(0)
    corr = np.corrcoef(dev.T)[np.triu_indices(len(idx), 1)].mean()
    check('hammond: the Leslie\'s Doppler, tremolo and reflections',
          50 < dop < 110 and trem > 8 and corr < 0.6,
          f'pitch {dop:.0f} c p-p, level {trem:.0f} dB p-p; partials swinging alike {corr:.2f}')
    # The harmonic percussion planned: on for a detached, moving tune; off for a legato one; for a
    # long legato middle switched off and back (two changes); a short one not worth a switch.
    def piece(parts):
        ev_, t = [(0, [0xC0, 16])], 0
        for kind, bars in parts:
            for i in range(bars * 8 if kind == 'det' else bars * 2):
                m_ = [72, 74, 76, 79, 77, 74, 72, 71][i % 8]
                if kind == 'det':
                    ev_ += [(t, [0x90, m_, 90]), (t + 180, [0x80, m_, 0])]
                    t += 240
                else:
                    ev_ += [(t, [0x90, m_, 70]), (t + 990, [0x80, m_, 0])]
                    t += 960
        ev_.sort(key=lambda e_: (e_[0], e_[1][0] & 0xf0 == 0x90))
        out, last = b'', 0
        for tk, e_ in ev_:
            out += vlq(tk - last) + bytes(e_)
            last = tk
        out += b'\x00\xff\x2f\x00'
        with open(mid, 'wb') as fh:
            fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(out)) + out)
        r_ = run(['-play', mid, '-o', wav])
        if os.path.exists(wav):
            os.remove(wav)
        got_ = re.search(r'pedals; percussion ([^;]*);', r_.stderr)
        return got_.group(1) if got_ else '?'
    # Played low, registered higher: a 4' melody around C3-E3 (302800000), the 888 two octaves up for
    # one an octave lower: no mud.
    def low_tune(base):
        ev_, t = [(0, [0xC0, 16])], 0
        for i in range(32):
            m_ = base + [0, 2, 4, 5, 7, 5, 4, 2][i % 8]
            ev_ += [(t, [0x90, m_, 90]), (t + 200, [0x80, m_, 0])]
            t += 240
        out, last = b'', 0
        for tk, e_ in ev_:
            out += vlq(tk - last) + bytes(e_)
            last = tk
        out += b'\x00\xff\x2f\x00'
        with open(mid, 'wb') as fh:
            fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(out)) + out)
        r_ = run(['-play', mid, '-o', wav])
        if os.path.exists(wav):
            os.remove(wav)
        got_ = re.search(r'upper (\d{9})', r_.stderr)
        return got_.group(1) if got_ else '?'
    # Routing to a division (hammond-upper-manual, -lower-manual, -pedal): a piano and a bass
    # remapped onto one Hammond, the tune on the upper, the bass on the pedals; and the general
    # pauses re-registering (a drawbar out at each, two pauses here).
    ev_ = [(0, [0xC0, 0]), (0, [0xC1, 32])]
    for i in range(24):
        if 8 <= i < 10 or 16 <= i < 18:
            continue  # (two general pauses)
        m_ = [72, 74, 76, 77][i % 4]
        ev_ += [(i * 480, [0x90, m_, 90]), (i * 480 + 460, [0x80, m_, 0])]
        if i % 2 == 0:
            b_ = [36, 43][(i // 2) % 2]
            ev_ += [(i * 480, [0x91, b_, 90]), (i * 480 + 460, [0x81, b_, 0])]
    ev_.sort(key=lambda e_: (e_[0], e_[1][0] & 0xf0 == 0x90))
    out, last = b'', 0
    for tk, e_ in ev_:
        out += vlq(tk - last) + bytes(e_)
        last = tk
    out += b'\x00\xff\x2f\x00'
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(out)) + out)
    r_ = run(['-play', mid, '-reinstrument', 'grand-piano=hammond-upper-manual,double-bass=hammond-pedal', '-o', wav])
    if os.path.exists(wav):
        os.remove(wav)
    got_ = re.search(r'notes: (\d+) upper, (\d+) lower, (\d+) pedals', r_.stderr)
    reg_ = re.search(r're-registered at (\d+) pause', r_.stderr)
    parts_ = len(re.findall(r'hammond plan', r_.stderr))
    check('hammond: routed to its divisions (one organ), re-registered at the general pauses',
          got_ is not None and got_.groups() == ('20', '0', '10') and parts_ == 1 and reg_ is not None and reg_.group(1) == '2',
          (' '.join(got_.groups()) if got_ else '?') + f' (upper lower pedals), {parts_} organ, ' + (reg_.group(0) if reg_ else 'no re-registration'))
    # A piano part reinstrumented onto the Hammond: its sustain pedal (CC64) is its damper, not the
    # Leslie's switch: the Leslie planned, not thrown at every pedal change.
    ev_ = [(0, [0xC0, 0])]
    for i in range(32):
        m_ = [72, 74, 76, 77][i % 4]
        ev_ += [(i * 480, [0x90, m_, 90]), (i * 480 + 460, [0x80, m_, 0]), (i * 480 + 10, [0xB0, 64, 127]), (i * 480 + 400, [0xB0, 64, 0])]
    ev_.sort(key=lambda e_: (e_[0], e_[1][0] & 0xf0 == 0x90))
    out, last = b'', 0
    for tk, e_ in ev_:
        out += vlq(tk - last) + bytes(e_)
        last = tk
    out += b'\x00\xff\x2f\x00'
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(out)) + out)
    r_ = run(['-play', mid, '-reinstrument', 'grand-piano=hammond', '-o', os.devnull])
    les_ = re.search(r'Leslie (the file\'s|planned[^;]*)', r_.stderr)
    check('hammond: a reinstrumented piano\'s sustain pedal is not the Leslie\'s switch',
          les_ is not None and les_.group(1).startswith('planned'), les_.group(0) if les_ else r_.stderr[-200:])
    # (the jazz 888 with the percussion: its 1' out too, silent while the percussion takes its busbar)
    regs = [low_tune(62), low_tune(48), low_tune(36)]
    check('hammond: a low tune registered higher (no mud)', regs == ['888000008', '302800008', '203808088'], ' / '.join(regs))
    plans = [piece([('det', 24)]), piece([('leg', 24)]), piece([('det', 16), ('leg', 16), ('det', 16)]),
             piece([('det', 30), ('leg', 2), ('det', 16)])]
    check('hammond: the percussion planned (detached on, legato off, a long legato middle switched, a short one not)',
          plans == ['on (3rd, fast, soft)', 'off', 'on (3rd, fast, soft), off for bars 17-32', 'on (3rd, fast, soft)'], ' | '.join(plans))
    # The lower manual as the percussion's stand-in (008880080): a detached jazz line with the
    # percussion, the left hand comping chords round C4 over a pedal bass; with the left hand's own
    # bass (no pedals, chords low) not.
    def comp(pedals):
        ev_ = [(0, [0xC0, 0]), (0, [0xC1, 2]), (0, [0xC2, 32])]
        for i in range(24 * 8):
            m_ = [72, 74, 76, 79, 77, 74, 72, 71][i % 8]
            ev_ += [(i * 240, [0x90, m_, 90]), (i * 240 + 180, [0x80, m_, 0])]
        for b_ in range(48):
            for p_ in ([60, 64, 67] if b_ % 2 == 0 else [62, 65, 69]) if pedals else ([36, 43, 48] if b_ % 2 == 0 else [38, 45, 50]):
                ev_ += [(b_ * 960, [0x91, p_, 70]), (b_ * 960 + 900, [0x81, p_, 0])]
            if pedals:
                ev_ += [(b_ * 960, [0x92, [36, 43][b_ % 2], 80]), (b_ * 960 + 900, [0x82, [36, 43][b_ % 2], 0])]
        ev_.sort(key=lambda e_: (e_[0], e_[1][0] & 0xf0 == 0x90))
        out, last = b'', 0
        for tk, e_ in ev_:
            out += vlq(tk - last) + bytes(e_)
            last = tk
        out += b'\x00\xff\x2f\x00'
        with open(mid, 'wb') as fh:
            fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(out)) + out)
        r_ = run(['-play', mid, '-reinstrument', 'grand-piano=hammond-upper-manual,electric-grand=hammond-lower-manual,double-bass=hammond-pedal',
                  '-o', os.devnull])
        got_ = re.search(r'lower (\d{9})', r_.stderr)
        return got_.group(1) if got_ else '?', 'stand-in' in r_.stderr
    # A soft accompaniment (velocity 45 against the tune's 120, the same register): no whisper
    # 11+ dB under, 6 dB under on the hollow 8' (008060400's shape), its vibrato off.
    ev_ = [(0, [0xC0, 0]), (0, [0xC1, 2])]
    for i in range(96):
        m_ = [72, 74, 76, 79, 77, 74, 72, 71][i % 8]
        ev_ += [(i * 240, [0x90, m_, 120]), (i * 240 + 220, [0x80, m_, 0])]
    for i in range(48):
        m_ = [64, 65, 67, 65][i % 4]
        ev_ += [(i * 480, [0x91, m_, 45]), (i * 480 + 460, [0x81, m_, 0])]
    ev_.sort(key=lambda e_: (e_[0], e_[1][0] & 0xf0 == 0x90))
    out, last = b'', 0
    for tk, e_ in ev_:
        out += vlq(tk - last) + bytes(e_)
        last = tk
    out += b'\x00\xff\x2f\x00'
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(out)) + out)
    r_ = run(['-play', mid, '-reinstrument', 'grand-piano=hammond-upper-manual,electric-grand=hammond-lower-manual', '-o', os.devnull])
    low_ = re.search(r'lower (\d{9})', r_.stderr)
    low_ = low_.group(1) if low_ else '?'
    check('hammond: a soft accompaniment set apart by colour (the hollow 8\', 6 dB under, vibrato off), not left a whisper',
          'set apart by colour' in r_.stderr and low_[:2] == '00' and low_[2] != '0' and low_[3] == '0' and low_[5] == '0'
          and low_[7:] == '00' and 'upper only' in r_.stderr, low_)
    a_, b_ = comp(True), comp(False)
    shape = lambda s_: ''.join('x' if c_ != '0' else '0' for c_ in s_)
    check('hammond: the lower comping over the pedals on 008880080 (the percussion\'s stand-in), the left hand\'s bass not',
          a_[1] and shape(a_[0]) == '00xxx00x0' and not b_[1], f'{a_[0]} / {b_[0]}')
    # The rotors' slow PI speed control: slow to fast at 1 s, back at 6 s (CC82): the horn up in
    # ~1 s, the drum in ~4 s, both down again, neither far past its speed.
    ev_ = [(0, [0xC0, 16]), (0, [0xB0, 82, 60]), (0, [0x90, 72, 100]), (1920, [0xB0, 82, 127]), (9600, [0xB0, 82, 60]),
           (9600, [0x80, 72, 0])]
    trk_ = b''.join(vlq(d) + bytes(e_) for d, e_ in ev_) + b'\x00\xff\x2f\x00'
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 960) + b'MTrk' + struct.pack('>I', len(trk_)) + trk_)
    r_ = run(['-play', mid, '-room', 'dry', '-o', wav], env={'PJ_LESLIE_LOG': '1', 'PJ_LESLIE_COUNT': '1'})  # (one cabinet: its plain speeds)
    if os.path.exists(wav):
        os.remove(wav)
    rows = [list(map(float, l_.split()[2::2])) for l_ in r_.stderr.splitlines() if l_.startswith('leslie t')]
    def first(col, t0, cond):
        for row in rows:
            if row[0] >= t0 and cond(row[col]):
                return row[0] - t0
        return 99
    hu, du = first(1, 1.0, lambda v: v > 0.9 * 6.6), first(2, 1.0, lambda v: v > 0.9 * 4.17)  # (the drum's own motor: 1.6 x slower)
    hd, dd = first(1, 6.0, lambda v: v < 1.2), first(2, 6.0, lambda v: v < 0.7)
    hmax = max(r__[1] for r__ in rows if 1 < r__[0] < 6)
    hmin = min(r__[1] for r__ in rows if r__[0] > 6)
    check('hammond: the Leslie\'s rotors by a slow PI control (the horn ~1 s, the drum ~4 s, little overshoot)',
          0.5 < hu < 1.5 and 2.5 < du < 6 and 0.5 < hd < 2 and 2.5 < dd < 7 and hmax < 7.2 and hmin > 0.4,
          f'up: horn {hu:.1f} s, drum {du:.1f} s; down: horn {hd:.1f} s, drum {dd:.1f} s; horn {hmin:.2f}..{hmax:.2f} Hz')
    # A nudge of the slow Leslie in the middle of a long held note (a 4 s D5 at 2..6 s): the horn
    # swells a little faster around its middle, then slow again.
    ev_, t = [(0, [0xC0, 16])], 0
    for m_ in (67, 69, 71, 72):
        ev_ += [(t, [0x90, m_, 90]), (t + 460, [0x80, m_, 0])]
        t += 480
    ev_ += [(t, [0x90, 74, 90]), (t + 8 * 480 - 20, [0x80, 74, 0])]
    t += 8 * 480
    for m_ in (72, 71, 69, 67):
        ev_ += [(t, [0x90, m_, 90]), (t + 460, [0x80, m_, 0])]
        t += 480
    ev_.sort(key=lambda e_: (e_[0], e_[1][0] & 0xf0 == 0x90))
    out, last = b'', 0
    for tk, e_ in ev_:
        out += vlq(tk - last) + bytes(e_)
        last = tk
    out += b'\x00\xff\x2f\x00'
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(out)) + out)
    r_ = run(['-play', mid, '-room', 'dry', '-o', wav], env={'PJ_LESLIE_LOG': '1', 'PJ_HAMMOND_FASTSHARE': '0'})
    if os.path.exists(wav):
        os.remove(wav)
    rows = [list(map(float, l_.split()[2::2])) for l_ in r_.stderr.splitlines() if l_.startswith('leslie t')]
    peak = max((row_[1], row_[0]) for row_ in rows)
    after = [row_[1] for row_ in rows if 6.0 <= row_[0] <= 7.0]
    check('hammond: the slow Leslie nudged on a long note (swells round its middle, slow again)',
          'nudged on 1 long note' in r_.stderr and 1.3 < peak[0] < 3 and 3.5 < peak[1] < 5.5 and max(after) < 1.0,
          f'horn up to {peak[0]:.2f} Hz at {peak[1]:.1f} s (the note 2..6 s), back to {max(after):.2f} Hz')
    # The swell on a long struck chord (C major held 2..8 s), an organist's way: the switch to fast
    # just before the chord is struck, back to slow in its middle: the horn up, then slow again
    # within the chord.
    ev_, t = [(0, [0xC0, 16])], 0
    for m_ in (67, 69, 71, 72):
        ev_ += [(t, [0x90, m_, 90]), (t + 460, [0x80, m_, 0])]
        t += 480
    for m_ in (60, 64, 67):
        ev_ += [(t, [0x90, m_, 90]), (t + 12 * 480 - 20, [0x80, m_, 0])]
    t += 12 * 480
    for m_ in (72, 71, 69, 67):
        ev_ += [(t, [0x90, m_, 90]), (t + 460, [0x80, m_, 0])]
        t += 480
    ev_.sort(key=lambda e_: (e_[0], e_[1][0] & 0xf0 == 0x90))
    out, last = b'', 0
    for tk, e_ in ev_:
        out += vlq(tk - last) + bytes(e_)
        last = tk
    out += b'\x00\xff\x2f\x00'
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 480) + b'MTrk' + struct.pack('>I', len(out)) + out)
    r_ = run(['-play', mid, '-room', 'dry', '-o', wav], env={'PJ_LESLIE_LOG': '1', 'PJ_HAMMOND_FASTSHARE': '0', 'PJ_HAMMOND_FLICK': '1'})
    if os.path.exists(wav):
        os.remove(wav)
    rows = [list(map(float, l_.split()[2::2])) for l_ in r_.stderr.splitlines() if l_.startswith('leslie t')]
    peak = max((row_[1], row_[0]) for row_ in rows)
    end = [row_[1] for row_ in rows if 7.0 <= row_[0] <= 8.0]
    check('hammond: the fast switch on a long struck chord (just before it, slow again in its middle: the horn swells up and settles)',
          'flicked fast on 1 held chord' in r_.stderr and 5 < peak[0] < 8 and 2 < peak[1] < 6 and max(end) < 1.2,
          f'horn up to {peak[0]:.2f} Hz at {peak[1]:.1f} s (the chord 2..8 s), {max(end):.2f} Hz by its end')
    def write_mid(ev_, tpq=480):
        ev_ = sorted(ev_, key=lambda e_: (e_[0], e_[1][0] & 0xf0 == 0x90))
        out, last = b'', 0
        for tk, e_ in ev_:
            out += vlq(tk - last) + bytes(e_)
            last = tk
        out += b'\x00\xff\x2f\x00'
        with open(mid, 'wb') as fh:
            fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, tpq) + b'MTrk' + struct.pack('>I', len(out)) + out)
    # A trill (D5-E5, 12 notes 0.0625 s apart, all at velocity 80) after a few slow notes: planned
    # with a rising touch, from about half its velocity to 1.3 x (the key clicks follow the touch:
    # the velocity check above).
    ev_, t = [(0, [0xC0, 16])], 0
    for m_ in (67, 69, 71, 72):
        ev_ += [(t, [0x90, m_, 80]), (t + 460, [0x80, m_, 0])]
        t += 480
    for k_ in range(12):
        ev_ += [(t, [0x90, 74 + 2 * (k_ & 1), 80]), (t + 58, [0x80, 74 + 2 * (k_ & 1), 0])]
        t += 60
    ev_ += [(t, [0x90, 74, 80]), (t + 900, [0x80, 74, 0])]
    write_mid(ev_)
    r_ = run(['-play', mid, '-room', 'dry', '-o', wav])
    if os.path.exists(wav):
        os.remove(wav)
    m = re.search(r'(\d+) trills? with a rising touch \(the first (\d+) -> (\d+)\)', r_.stderr)
    check('hammond: a trill planned with a rising touch (its key clicks sharpening)',
          m is not None and m.group(1) == '1' and 30 <= int(m.group(2)) <= 50 and 95 <= int(m.group(3)) <= 110,
          m.group(0) if m else 'no trill planned')
    # Harmonic complexity: dense chords with semitones and tritones and close low intervals plan a
    # single foundation; plain triads keep the 888.
    plans = []
    for chords in (((48, 52, 55, 60), (53, 57, 60, 65), (55, 59, 62, 67)),
                   ((48, 52, 58, 61, 63), (53, 57, 63, 66, 68), (55, 56, 59, 65, 68))):
        ev_, t = [(0, [0xC0, 16])], 0
        for _ in range(4):
            for c_ in chords:
                for m_ in c_:
                    ev_ += [(t, [0x90, m_, 80]), (t + 900, [0x80, m_, 0])]
                t += 960
        write_mid(ev_)
        r_ = run(['-play', mid, '-room', 'dry', '-o', wav])
        if os.path.exists(wav):
            os.remove(wav)
        m = re.search(r'upper (\d{9})', r_.stderr)
        plans.append((m.group(1) if m else '?', 'single foundation' in r_.stderr))
    check('hammond: a harmonically complex piece planned on a single foundation (no 16\', 5 1/3\' mud)',
          not plans[0][1] and plans[1][1] and plans[1][0][:2] == '00',
          f'triads: upper {plans[0][0]}; clusters: upper {plans[1][0]}')
    # Online: a low cluster held (C3 D3 Eb3) eases the 16' and 5 1/3' back, its sub-bass falling.
    fs, x = render('on 48 90 on 50 90 on 51 90 wait 4 off 48 off 50 off 51',
                   {'PJ_HAMMOND_UPPER': '888000000', 'PJ_HAMMOND_LESLIE': 'off', 'PJ_HAMMOND_VIB': 'off'})
    def sub(a, b):
        sp = np.abs(np.fft.rfft(x[int(a * fs):int(b * fs)])) ** 2
        f = np.fft.rfftfreq(int(b * fs) - int(a * fs), 1 / fs)
        return 10 * math.log10(sp[(f > 40) & (f < 120)].sum() / sp[(f > 40) & (f < 4000)].sum())
    s0, s1 = sub(0, 0.3), sub(2, 4)
    check('hammond: muddiness corrected online (a low cluster: the 16\' and 5 1/3\' eased back)',
          s1 < s0 - 4, f'the sub-bass share {s0:.1f} dB as struck -> {s1:.1f} dB held')
    # The Leslie's own flaws: the horn's wind at speed, the speed relay's click, a loose panel's
    # buzz under a loud bass.
    ev_ = [(0, [0xC0, 16]), (0, [0xB0, 82, 60]), (5760, [0xB0, 82, 127]), (7680, [0x90, 84, 1]), (10, [0x80, 84, 0])]
    trk_ = b''.join(vlq(d) + bytes(e_) for d, e_ in ev_) + b'\x00\xff\x2f\x00'
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 960) + b'MTrk' + struct.pack('>I', len(trk_)) + trk_)
    run(['-play', mid, '-room', 'dry', '-o', wav], env={'PJ_HAMMOND_HUM': '0', 'PJ_LESLIE_RELAY': '40'})  # (the relay at its measured level)
    fs, y = wavfile.read(wav)
    os.remove(wav)
    y = y.astype(float).mean(1)
    bb, aa = butter(2, [1500 / (fs / 2), 5000 / (fs / 2)], 'band')
    w = filtfilt(bb, aa, y)
    wind = 10 * math.log10(np.mean(w[int(5.5 * fs):int(7.5 * fs)] ** 2) / np.mean(w[int(1 * fs):int(2.8 * fs)] ** 2))
    hp = butter(4, 1500 / (fs / 2), 'high')
    e = np.abs(filtfilt(*hp, y))
    relay = 20 * math.log10(e[int(2.95 * fs):int(3.1 * fs)].max() / e[int(2.5 * fs):int(2.9 * fs)].max())
    rock = {'PJ_HAMMOND_PEDAL': '88', 'PJ_HAMMOND_UPPER': '888800000', 'PJ_HAMMOND_DIRT': '0', 'PJ_LESLIE_NOISE': '0'}
    seq = 'on 28 127 on 40 127 on 52 127 on 55 127 wait 3 off 28 off 40 off 52 off 55'
    a_ = render(seq, rock, 'rock-organ')[1]
    b2 = render(seq, dict(rock, PJ_LESLIE_RATTLEGAP='100'), 'rock-organ')[1]
    hi = filtfilt(*butter(4, 2000 / (fs / 2), 'high'), a_ - b2)
    rattle = 10 * math.log10(np.mean(hi ** 2) / np.mean(filtfilt(*butter(4, 2000 / (fs / 2), 'high'), b2) ** 2) + 1e-30)
    check('hammond: the Leslie\'s flaws: the horn\'s wind, the relay\'s click, a loose panel\'s buzz',
          wind > 4 and relay > 15 and -50 < rattle < -15,
          f'wind fast +{wind:.0f} dB over slow; relay {relay:.0f} dB over the floor; the buzz {rattle:.0f} dB under the chord\'s highs')
    # In a room the Leslie radiates into it (its rays round the cabinet, each wall its own swirl,
    # the room's late field fed all round): a fuller, wider room than through close microphones.
    ev_ = [(0, [0xC0, 16]), (0, [0xB0, 82, 127])] + [(0, [0x90, m_, 100]) for m_ in (60, 64, 67, 72)] + \
          [(5760, [0x80, 60, 0])] + [(0, [0x80, m_, 0]) for m_ in (64, 67, 72)]
    trk_ = b''.join(vlq(d) + bytes(e_) for d, e_ in ev_) + b'\x00\xff\x2f\x00'
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 0, 1, 960) + b'MTrk' + struct.pack('>I', len(trk_)) + trk_)
    res = {}
    for mode in ('close', 'room'):
        run(['-play', mid, '-o', wav], env={'PJ_LESLIE_MICS': mode})
        fs, y = wavfile.read(wav)
        os.remove(wav)
        y = y.astype(float)
        held, tail = y[int(1 * fs):int(2.9 * fs)], y[int(3.1 * fs):int(4.5 * fs)]
        res[mode] = (10 * math.log10(np.mean(tail ** 2) / np.mean(held ** 2)), np.corrcoef(held[:, 0], held[:, 1])[0, 1])
    check('hammond: the Leslie in the room (a full tail, at least the close microphones\', the ears apart)',
          res['room'][0] > -20 and res['room'][0] >= res['close'][0] and res['room'][1] < 0.85,
          f'tail {res["room"][0]:.0f} dB (close mics {res["close"][0]:.0f}); left-right correlation {res["room"][1]:.2f}')
    os.remove(mid)
    # The drawbar knobs (live K1-K4; PJ_HAMMOND_KNOBS): K1 through sixteen classics, soft to hard,
    # each exact at its place; K2 hollow .. full, K3 dark .. bright, K4 light .. deep; their
    # middles leave K1's registration as it is.
    def bars_at(k):
        r_ = run(['-inst', 'hammond', '-seq', 'wait 0.01', '-o', os.devnull], env={'PJ_HAMMOND_KNOBS': ','.join(f'{v:.5f}' for v in k)})
        got_ = re.findall(r'drawbars ((?:\d\.\d ?){9})', r_.stderr)
        return [int(round(float(v))) for v in got_[-1].split()] if got_ else None
    stations = ['008000000', '008400000', '808000000', '838000000', '868000000', '688600000', '888000000', '888000008',
                '888400000', '888600000', '888800000', '888808000', '888880000', '888888000', '888888880', '888888888']
    exact = sum(bars_at([i / 15, 0.5, 0.5, 0.5]) == [int(c_) for c_ in st] for i, st in enumerate(stations))
    jazz = 6 / 15
    hollow, full = bars_at([jazz, 0, 0.5, 0.5]), bars_at([jazz, 1, 0.5, 0.5])
    bright, light = bars_at([jazz, 0.5, 1, 0.5]), bars_at([jazz, 0.5, 0.5, 0])
    ok = (exact == 16 and hollow[3] == hollow[5] == hollow[8] == 0 and hollow[4] > 0 and sum(full) > 30 and bright[8] > 0
          and light[0] == 0)
    # The sliders glide: K1 turned from the jazz 888 to the 8' alone at 1 s, the 16' fades over a few
    # tenths of a second, never by a step.
    from scipy.signal import hilbert as _hb
    fs, x = render('on 60 100 wait 3 off 60', {'PJ_HAMMOND_KNOBS2': '1.0,0,0.5,0.5,0.5', 'PJ_HAMMOND_VIB': 'off',
                                               'PJ_HAMMOND_LESLIE': 'off', 'PJ_HAMMOND_PERC': 'off'})
    bb, aa = butter(2, [125 / (fs / 2), 137 / (fs / 2)], 'band')
    e16 = np.convolve(np.abs(_hb(filtfilt(bb, aa, x))), np.ones(480) / 480, 'same')
    L16 = 20 * np.log10(e16 + 1e-12) - 20 * math.log10(e16[int(0.9 * fs)])
    t20 = (np.where(L16[int(1.0 * fs):] < -20)[0][0]) / fs
    seg = L16[int(1.0 * fs):int(1.0 * fs + t20 * fs)][::480]
    step = np.abs(np.diff(seg)).max()
    # The lower manual follows the upper (K1): the accompaniment, softer and darker.
    def lower_at(k):
        r_ = run(['-inst', 'hammond', '-seq', 'wait 0.01', '-o', os.devnull], env={'PJ_HAMMOND_KNOBS': ','.join(f'{v:.5f}' for v in k)})
        got_ = re.findall(r'drawbars ((?:\d\.\d ?){9}).*lower ((?:\d\.\d ?){9})', r_.stderr)
        return ([float(v) for v in got_[-1][0].split()], [float(v) for v in got_[-1][1].split()]) if got_ else (None, None)
    gainof = lambda bars: sum(0 if b_ <= 0 else (10 ** (-3 * (8 - b_) / 20) if b_ >= 1 else b_ * 10 ** (-21 / 20)) for b_ in bars)
    pairs = [lower_at([i / 15, 0.5, 0.5, 0.5]) for i in (3, 6, 10, 15)]
    darker = all(lo_[d] <= up_[d] for up_, lo_ in pairs for d in range(9)) and all(gainof(lo_) < gainof(up_) for up_, lo_ in pairs)
    top = max(max(lo_[5:]) for _, lo_ in pairs)
    check('hammond: the lower manual follows the upper: softer, darker (ballad, jazz, gospel, full)', darker and top <= 2,
          ' | '.join(''.join(str(int(round(v))) for v in up_) + ' -> ' + ''.join(str(int(round(v))) for v in lo_) for up_, lo_ in pairs))
    # The pedals follow too: the 16' by the upper's loudness, the 8' by its brightness.
    def pedal_at(k):
        r_ = run(['-inst', 'hammond', '-seq', 'wait 0.01', '-o', os.devnull], env={'PJ_HAMMOND_KNOBS': ','.join(f'{v:.5f}' for v in k)})
        got_ = re.findall(r'pedal (\d\.\d) (\d\.\d)', r_.stderr)
        return (float(got_[-1][0]), float(got_[-1][1])) if got_ else (None, None)
    whisper, jazz_, full_ = pedal_at([0, .5, .5, .5]), pedal_at([6 / 15, .5, .5, .5]), pedal_at([1, .5, .5, .5])
    check('hammond: the pedals follow the upper (soft: less 16\'; bright: more 8\')',
          whisper[0] < jazz_[0] and jazz_[1] < full_[1] and jazz_[1] <= 3,
          f'whisper {whisper[0]:.1f}/{whisper[1]:.1f}, jazz {jazz_[0]:.1f}/{jazz_[1]:.1f}, full {full_[0]:.1f}/{full_[1]:.1f}')
    check('hammond: the drawbars glide (a PI-controlled slider, no steps)', 0.2 < t20 < 1.0 and step < 4,
          f'the 16\' to -20 dB in {t20 * 1e3:.0f} ms, at most {step:.1f} dB in 10 ms')
    check('hammond: the drawbar knobs: K1 through 16 classics, K2 hollow/full, K3 bright, K4 light',
          ok, f'{exact} of 16 stations exact; from the jazz 888: hollow {"".join(map(str, hollow))}, full {"".join(map(str, full))}, '
              f'bright {"".join(map(str, bright))}, light {"".join(map(str, light))}')
    # The ending (a quick tune, then C major held 6 s at the end): the Leslie switched up as the chord
    # is struck (fast from ~1 s in) and slow again at 4/5 of it (4.8 s), the rotors still winding down
    # at the release; the
    # expression swelled; two drawbars pulled out through it.
    def tune(ev_, t, n, prog_vel=90, gap=60):
        for i in range(n):
            m_ = [72, 74, 76, 79, 77, 74, 72, 71][i % 8]
            ev_ += [(t, [0x90, m_, prog_vel]), (t + 240 - gap, [0x80, m_, 0])]
            t += 240
        return t
    ev_ = [(0, [0xC0, 16])]
    t = tune(ev_, 0, 32)
    for m_ in (60, 64, 67, 72):
        ev_ += [(t, [0x90, m_, 90]), (t + 12 * 480, [0x80, m_, 0])]
    write_mid(ev_)
    r_ = run(['-play', mid, '-room', 'dry', '-o', wav], env={'PJ_LESLIE_LOG': '1', 'PJ_HAMMOND_FASTSHARE': '0', 'PJ_HAMMOND_ENDLESLIE': '1'})
    if os.path.exists(wav):
        os.remove(wav)
    rows = [list(map(float, l_.split()[2::2])) for l_ in r_.stderr.splitlines() if l_.startswith('leslie t')]
    c0 = 32 * 0.25  # (the chord's start, s)
    peak = max((row_[1], row_[0]) for row_ in rows if row_[0] >= c0)
    atEnd = [row_[1] for row_ in rows if c0 + 5.6 <= row_[0] <= c0 + 6.0]
    m = re.search(r'the ending: ([^;\n]*)', r_.stderr)
    check('hammond: the ending planned: the Leslie up and winding down, a swell, drawbars out',
          m is not None and 'the Leslie up and winding down' in m.group(1) and 'a swell' in m.group(1) and '2 drawbars out' in m.group(1)
          and peak[0] > 3.5 and c0 + 1 < peak[1] < c0 + 5.0 and atEnd and 0.9 < max(atEnd) < peak[0] - 1,
          (m.group(1) if m else 'no ending') + f'; horn up to {peak[0]:.2f} Hz at {peak[1] - c0:.1f} s into the chord, '
          f'{max(atEnd) if atEnd else 0:.2f} Hz at its release')
    # The rock organ's ending with the run switch off (PJ_HAMMOND_DIVE=1): the chord sinking and
    # fading as the generator coasts down (dry, no Leslie: the wheels' own pitch).
    ev_ = [(0, [0xC0, 18]), (0, [0xB0, 11, 110])]  # (its own expression: no planned swell)
    t = tune(ev_, 0, 16)
    for m_ in (48, 60):
        ev_ += [(t, [0x90, m_, 100]), (t + 8 * 480, [0x80, m_, 0])]
    write_mid(ev_)
    r_ = run(['-play', mid, '-room', 'dry', '-o', wav], env={'PJ_HAMMOND_DIVE': '1', 'PJ_HAMMOND_LESLIE': 'off', 'PJ_HAMMOND_VIB': 'off',
                                                          'PJ_HAMMOND_UPPER': '008000000', 'PJ_HAMMOND_GLISS': '0'})
    fs, y = wavfile.read(wav)
    os.remove(wav)
    y = y.astype(float)[:, 0]
    c0 = 16 * 0.25
    def pitch_lvl(t0):
        seg = y[int(t0 * fs):int((t0 + 0.25) * fs)] * np.hanning(int(0.25 * fs))
        sp = np.abs(np.fft.rfft(seg, 1 << 18))
        hz = np.fft.rfftfreq(1 << 18, 1 / fs)
        band = (hz > 150) & (hz < 290)
        k = np.argmax(sp * band)
        return hz[k], 20 * np.log10(sp[k] + 1e-9)
    f1, l1 = pitch_lvl(c0 + 0.5)
    f2, l2 = pitch_lvl(c0 + 3.6)
    drop = 1200 * math.log2(f1 / f2)
    check('hammond: the rock organ\'s ending: the run switch off, the chord sinking and fading',
          'the run switch off' in r_.stderr and 300 < drop < 1200 and l1 - l2 > 1,
          f'C4 {f1:.1f} Hz -> {f2:.1f} Hz ({drop:.0f} c down) over 1.6 s, {l1 - l2:.1f} dB softer')
    # Glissandos (the rock organ: a phrase, a short general pause, a phrase): swept up the white
    # keys into the music resuming; the short pause re-registered by the preset key.
    ev_ = [(0, [0xC0, 18])]
    t = tune(ev_, 0, 16)
    t += 240  # (a pause of 0.5 s: a preset key's)
    t = tune(ev_, t, 16)
    t += 1200  # (1.3 s: drawbars pulled)
    t = tune(ev_, t, 16)
    write_mid(ev_)
    r_ = run(['-play', mid, '-o', os.devnull], env={'PJ_HAMMOND_ENDING': '0'})
    g_ = re.search(r'(a glissando|two glissandos) up the white keys', r_.stderr)
    k_ = re.search(r're-registered at (\d+) pauses? \((\d+) by the preset key\)', r_.stderr)
    check('hammond: glissandos into the music resuming (the rock organ); a short pause\'s preset key',
          g_ is not None and k_ is not None and k_.group(1) == '2' and k_.group(2) == '1',
          (g_.group(0) if g_ else 'no glissando') + '; ' + (k_.group(0) if k_ else 'no preset key'))
    # The percussion's touch: a detached jazz line with a legato move into each bar's first note:
    # those notes detached before (the next pings), the other legato moves left as written.
    ev_, t = [(0, [0xC0, 16])], 0
    for i in range(64):
        m_ = [72, 74, 76, 79, 77, 74, 72, 71][i % 8]
        legato = i % 8 == 7 or i % 8 == 3  # (into the bar's first note, and into its middle)
        ev_ += [(t, [0x90, m_, 90]), (t + (260 if legato else 180), [0x80, m_, 0])]
        t += 240
    write_mid(ev_)
    r_ = run(['-play', mid, '-o', os.devnull], env={'PJ_HAMMOND_ENDING': '0'})
    d_ = re.search(r'(\d+) accents? detached to strike the percussion', r_.stderr)
    check('hammond: the percussion\'s touch: legato into the bar\'s first note detached (the next pings)',
          'percussion on (3rd, fast, soft)' in r_.stderr and d_ is not None and 4 <= int(d_.group(1)) <= 8,
          d_.group(0) if d_ else 'none detached')
    # The vibrato tabs: three staves in counterpoint (a single line in each, plain harmony) played
    # straight; a lower manual carrying the bass without a pedal line, its tab off.
    tu, tl, tp = [], [], []
    for i in range(48):
        tu += [(i * 240, [0x90, [72, 74, 76, 77, 79, 77, 76, 74][i % 8], 90]), (i * 240 + 220, [0x80, [72, 74, 76, 77, 79, 77, 76, 74][i % 8], 0])]
        if i % 2 == 0:
            n_ = [60, 59, 57, 55][(i // 2) % 4]
            tl += [(i * 240, [0x90, n_, 80]), (i * 240 + 460, [0x80, n_, 0])]
        if i % 4 == 0:
            n_ = [36, 43, 41, 43][(i // 4) % 4]
            tp += [(i * 240, [0x90, n_, 80]), (i * 240 + 940, [0x80, n_, 0])]
    with open(mid, 'wb') as fh:
        fh.write(b'MThd' + struct.pack('>IHHH', 6, 1, 4, 480) + track([(0, [0xC0, 16])]) + track(tu) + track(tl) + track(tp))
    r_ = run(['-play', mid, '-o', os.devnull], env={'PJ_HAMMOND_ENDING': '0'})
    straight = 'the vibrato off (counterpoint' in r_.stderr and 'vibrato C3 (tabs off)' in r_.stderr
    ev_ = [(0, [0xC0, 16])]
    for i in range(32):
        m_ = [72, 74, 76, 77][i % 4]
        ev_ += [(i * 480, [0x90, m_, 90]), (i * 480 + 460, [0x80, m_, 0])]
        if i % 2 == 0:  # (the left hand: a bass note with the tune, a chord off the beat)
            b_ = [48, 50, 52, 50][(i // 2) % 4]
            ev_ += [(i * 480, [0x90, b_, 80]), (i * 480 + 470, [0x80, b_, 0])]
        else:
            for c_ in (55, 59):
                ev_ += [(i * 480, [0x90, c_, 70]), (i * 480 + 460, [0x80, c_, 0])]
    write_mid(ev_)
    r_ = run(['-play', mid, '-o', os.devnull], env={'PJ_HAMMOND_ENDING': '0'})
    bass = re.search(r'the lower\'s vibrato off|vibrato C3 lower only|vibrato C3 upper only', r_.stderr)
    check('hammond: the vibrato tabs planned (counterpoint straight; the lower carrying the bass without it)',
          straight and 'the lower\'s vibrato off' in r_.stderr and 'C3 upper only' in r_.stderr,
          f'counterpoint {"straight" if straight else "not straight"}; ' + (bass.group(0) if bass else 'the lower\'s tab on'))
    # The horn's own response (a fixed filter: its flare's cutoff, its mouth's reflection's ripple,
    # its driver's top): a tone on the 8' alone through the slow Leslie, with it against without it
    # (PJ_LESLIE_HORNEQ=0): +3 dB at the first ripple's peak (B5), under it by the dip (F6), nothing
    # at C5 (the drum's).
    def horn_db(k, eq):
        run(['-inst', 'hammond', '-room', 'dry', '-seq', f'on {k} 100 wait 4 off {k}', '-o', wav],
            env={'PJ_LESLIE_HORNEQ': eq, 'PJ_HAMMOND_UPPER': '008000000', 'PJ_HAMMOND_VIB': 'off', 'PJ_HAMMOND_PERC': 'off',
                 'PJ_HAMMOND_LESLIE': 'slow'})
        fs_, y_ = wavfile.read(wav)
        os.remove(wav)
        y_ = y_.astype(float)
        y_ = y_[:, 0] if y_.ndim > 1 else y_
        a_ = y_[int(0.8 * fs_):int(3.8 * fs_)]
        sp = np.abs(np.fft.rfft(a_ * np.hanning(len(a_)))) ** 2
        hz = np.fft.rfftfreq(len(a_), 1 / fs_)
        f_ = 440 * 2 ** ((k - 69) / 12)
        return 10 * math.log10(sp[(hz > 0.97 * f_) & (hz < 1.03 * f_)].sum() + 1e-20)
    resp = {k: horn_db(k, '1') - horn_db(k, '0') for k in (72, 83, 89)}
    check('hammond: the Leslie horn\'s own response (its cutoff, its mouth\'s ripple)',
          abs(resp[72]) < 1 and 2 < resp[83] < 4.5 and resp[83] - resp[89] > 2.5,
          ', '.join(f'{440 * 2 ** ((k - 69) / 12):.0f} Hz {v:+.1f} dB' for k, v in resp.items()))
    r = run(['-inst', 'hammond', '-fuzz', '10', '1'])
    check('fuzz 10 s (hammond)', r.returncode == 0, r.stdout.strip())
    seq = 'on 48 100 on 60 100 on 64 100 on 67 100 on 72 100 wait 10 off 48 off 60 off 64 off 67 off 72'
    for count, limit in (('1', 2500), ('4', 5000)):  # (one Leslie; the four of the default)
        e_ = dict(os.environ, PJ_LESLIE_COUNT=count)
        base = cpu_time(['-inst', 'hammond', '-seq', 'wait 0.01', '-o', wav], env=e_)
        ns = (cpu_time(['-inst', 'hammond', '-seq', seq, '-o', wav], env=e_) - base) / (10 * 48000) * 1e9
        os.remove(wav)
        check(f'real-time margin (hammond: 91 wheels, a five-note chord, {count} Leslie{"s" if count != "1" else ""})', ns < limit,
              f'{ns:.0f} ns/sample ({ns / 20833 * 100:.1f} % of a core)')


def honky_tonk():
    """The honky-tonk (-inst honky-tonk, General MIDI 4; src/pianoinst.cc; research/piano_variants.md
    3.3-3.4): the upright, every string of a key its own ~1.5 Hz off, worn hammers: a held note's
    fundamental splits into lines about a hertz or two apart (the upright's is one), at its pitch."""
    import numpy as np
    from scipy.io import wavfile
    wav = os.path.join(ROOT, 'tools', '.honky.wav')
    width, pitch = {}, 0
    for inst in ('upright-piano', 'honky-tonk'):
        ws = []
        for m in (69, 72):
            run(['-inst', inst, '-seq', f'on {m} 80 wait 4 off {m}', '-o', wav])
            fs, x = wavfile.read(wav)
            os.remove(wav)
            x = x.astype(float)
            x = x.mean(1) if x.ndim > 1 else x
            seg = x[int(0.3 * fs):int(3.8 * fs)]
            sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 1 << 21))
            f = np.fft.rfftfreq(1 << 21, 1 / fs)
            f0 = 440 * 2 ** ((m - 69) / 12)
            w = (f > f0 - 6) & (f < f0 + 6)
            band = f[w][sp[w] > 0.25 * sp[w].max()]
            ws.append(band.max() - band.min())
            if inst == 'honky-tonk' and m == 69:
                pitch = 1200 * math.log2(f[w][sp[w].argmax()] / f0)
        width[inst] = np.mean(ws)
    check('honky-tonk: each string its own (the fundamental split into lines ~1-3 Hz apart), at its pitch',
          width['honky-tonk'] > 1.0 and width['honky-tonk'] > 2 * width['upright-piano'] and abs(pitch) < 15,
          f'the fundamental\'s lines over {width["upright-piano"]:.2f} Hz (upright) -> {width["honky-tonk"]:.2f} Hz; A4 {pitch:+.1f} c')
    # The tacks: each key's hammer meets one or two of its strings with metal first, unevenly: the
    # attacks' brightness differs from key to key.
    seq_ = ''.join(f'on {m} 80 wait 0.5 off {m} ' for m in (60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79))
    def shares(env):
        run(['-inst', 'honky-tonk', '-seq', seq_, '-o', wav], env=dict(os.environ, **env))
        fs, x = wavfile.read(wav)
        os.remove(wav)
        x = x.astype(float)
        x = x.mean(1) if x.ndim > 1 else x
        out = []
        for i in range(12):
            seg = x[int(i * 0.5 * fs):int(i * 0.5 * fs) + int(0.03 * fs)]
            sp = np.abs(np.fft.rfft(seg * np.hanning(len(seg)), 4096)) ** 2
            f = np.fft.rfftfreq(4096, 1 / fs)
            out.append(10 * math.log10(sp[(f > 2000) & (f < 6000)].sum() / sp[(f > 100) & (f < 1000)].sum()))
        return np.array(out)
    dk = shares({}) - shares({'PJ_HONKY_TACK': '0', 'PJ_HONKY_TACKGAP': '0'})
    check('honky-tonk: the tacks (each key\'s attack its own: one or two strings met by metal first)',
          dk.std() > 2 and dk.mean() > 1,
          f'attack 2-6 kHz {dk.mean():+.1f} dB on average, key to key sd {dk.std():.1f} dB')
    for seed in (1, 2, 3):
        r = run(['-inst', 'honky-tonk', '-fuzz', '10', str(seed)])
        m = re.search(r'peak ([0-9.e+]+)', r.stdout)
        check(f'fuzz 10 s (honky-tonk, seed {seed})', r.returncode == 0 and m and float(m.group(1)) < 5, r.stdout.strip())

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
              realtime, passive_vs_tmm, woodwinds, played_notes, brass, leads, strings, modals, electric_grand, upright, clavinet, jazz_kick, toms, cymbals, brush_swirl, gm2_extras, hand_drums, metal_perc, scrapers, shakers, stick_snare, clap_claves_surdo, celesta, harpsichord, guitars, bagpipe_drones, organ, organ_stops, accordions, harmonica, reed_organ, hammond, honky_tonk):
        f()
    print(f"\n{len(failures)} failed, {time.time() - t0:.0f} s" + (f": {failures}" if failures else ''))
    sys.exit(1 if failures else 0)
