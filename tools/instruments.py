"""The designed instruments (tools/design.py turns these into src/designs.cc).

Each: a bore profile (mm from the top: the reed tip, or the top of the air
column at a window), tone holes (top to bottom), the fingerings that place the
holes ("core": note -> pattern, resonance, weight; fitted by least squares),
a generator of candidate fingerings for the other notes (cross fingerings,
half-holes, vents: the best in tune is kept), and the excitation.

Hole dimensions, bores and reeds are typical of the real instruments (sources
in the docstrings); the hole positions and the bore's length are fitted. The
keywork is simplified where a real instrument shares holes between notes in
ways the fingering charts cannot show without the full mechanism: a keyed
instrument here has a hole for each semitone of its lowest register (as the
saxophone nearly has), and the upper registers add a vent.
"""
import math

FINGER, NCLOSED, PERF = 1, 2, 4


def hole(name, x, d, t, padH=None, cup=None, flags=0, **kw):
    if flags & FINGER:
        padH = padH or 8.0   # a finger lifts ~8 mm
        cup = cup or 15.0    # a fingertip
    else:
        padH = padH or max(1.5, 0.3 * d)
        cup = cup or d + 5
    h = dict(name=name, x=x, d=d, t=t, padH=padH, cup=cup, flags=flags)
    h.update(kw)
    return h


def lin(pts, x):
    for (x0, y0), (x1, y1) in zip(pts, pts[1:]):
        if x <= x1:
            return y0 + (y1 - y0) * (x - x0) / (x1 - x0)
    return pts[-1][1]


def first_open(holes, k, vents=(), shut=()):
    """Pattern with every hole above hole k closed, k and below open (normally
    closed keys below stay closed), the vents open."""
    p = []
    for i, h in enumerate(holes):
        if h['name'] in vents:
            p.append('o')
        elif h['name'] in shut:
            p.append('x')
        elif i < k:
            p.append('x')
        elif i > k and h['flags'] & NCLOSED:
            p.append('x')
        else:
            p.append('o')
    return ''.join(p)


def all_closed(holes, vents=()):
    return ''.join('o' if h['name'] in vents else 'x' for h in holes)


NAMES = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B']


def midi(n):
    """'F#3' -> 54."""
    name, octave = n[:-1], int(n[-1])
    return 12 * (octave + 1) + NAMES.index(name)


def nm(m):
    return NAMES[m % 12] + str(m // 12 - 1)


# ---------------------------------------------------------------------------
# Clarinet in Bb (Boehm), written E3..G6. A cylindrical bore closed at the reed:
# odd harmonics, the second register a twelfth up (register key).
# Bore 14.6 mm, mouthpiece ~ its volume, bell flaring to 60 mm (Nederveen 1969;
# Dalmont, Gazengel, Kergomard & Ollivier 1995: the clarinet's reed and mouthpiece).

def clarinet():
    names = ['reg', 'A#4', 'A4', 'G#4', 'G4', 'F#4', 'F4', 'E4', 'D#4', 'D4', 'C#4', 'C4',
             'B3', 'A#3', 'A3', 'G#3', 'G3', 'F#3', 'F3']
    fingers = {'G4', 'F4', 'E4', 'D4', 'C4', 'A#3', 'A3'}
    keys_nc = {'reg', 'A#4', 'A4', 'G#4', 'F#4', 'D#4', 'C#4', 'B3', 'G#3'}
    holes = []
    for i, n in enumerate(names):
        if n == 'reg':
            holes.append(hole('register', 140, 3.0, 13.0, padH=1.2, cup=6, flags=NCLOSED, lo=100, hi=200))
            continue
        m = midi(n)
        u = (70 - m) / (70 - 53)  # 0 at the throat .. 1 at F3
        d = 5.0 + 7.5 * u if n not in fingers else 6.5 + 4 * u
        t = 7.0 - 3 * u
        flags = (FINGER if n in fingers else 0) | (NCLOSED if n in keys_nc else 0)
        # start: a quarter-wave from the reed, lengthened by the bell's correction
        x0 = 349.5e3 / (4 * 440 * 2 ** ((m - 2 - 69) / 12)) - 25
        holes.append(hole(n, x0, d, t, flags=flags))

    def bore(end, mp=1.0):
        return [(0, 9.0 * mp), (12, 12.5 * mp), (30, 14.0 * mp), (60, 14.6), (end - 75, 14.8), (end - 50, 17.5),
                (end - 30, 23.0), (end - 12, 36.0), (end, 60.0)]
    core = {}
    for k, h in enumerate(holes):
        if k == 0:
            continue
        core[midi(h['name'])] = (first_open(holes, k), 1, 1.0)
    core[midi('E3')] = (all_closed(holes), 1, 1.0)
    for m in range(midi('E3'), midi('F4') + 1):  # clarion: the chalumeau + register key, a twelfth up
        pat, _, _ = core[m]
        core[m + 19] = (pat[:0] + 'o' + pat[1:], 2, 0.7)

    def cand(m, notes):
        # Altissimo: the third resonance of a chalumeau fingering, the register
        # key open, maybe a finger hole half open as a second vent.
        out = []
        for b in range(midi('E3'), midi('A#4') + 1):
            base = core[b][0]
            for vent in (None, 4, 6, 7):
                p = list('o' + base[1:])
                cost = 0
                if vent is not None:
                    if p[vent] == 'o':
                        continue
                    p[vent] = 'h'
                    cost = 3
                out.append((''.join(p), 3, cost))
        return out
    return dict(
        name='clarinet', title='Clarinet in Bb (Boehm), written E3..C6', family='clarinet',
        kind='reed', transpose=-2, lo=midi('E3'), hi=midi('C6'), fs=48000, gain=0.5, offset=25,
        end=665, bore_fn=bore, holes=holes, core=core, candidates=cand, gap=9, extra=[('mp', 1.0, 0.7, 1.3)],
        doc="""A cylindrical bore closed at the reed: odd harmonics; the register key
        opens the second register a twelfth up; the altissimo is the third resonance.""",
        reed=dict(width=12e-3, h0=0.42e-3, k=8e6, fr=3000, qr=0.35, sr=1.2e-4, press=1600, pressHi=2100, vib=-1),
    )



def chromatic(holes, lo_note, vents_for=lambda m: (), n_for=lambda m: 1):
    """A keyed instrument's first register: hole k sounds its note when it is
    the first open hole; the lowest note has every hole closed."""
    core = {midi(lo_note): (all_closed(holes), 1, 1.0)}
    for k, h in enumerate(holes):
        try:
            m = midi(h['name'])
        except (ValueError, IndexError):
            continue
        core[m] = (first_open(holes, k), 1, 1.0)
    return core


def with_vent(holes, pat, vent):
    i = [h['name'] for h in holes].index(vent)
    return pat[:i] + 'o' + pat[i + 1:]


# ---------------------------------------------------------------------------
# Saxophones: a cone (half-angle ~1.6 deg) from the neck to the bell, the
# mouthpiece standing in for the missing apex (its volume ~ the cone's);
# one tone hole per semitone from low Bb to C#5, two octave vents (body, neck)
# switched by the octave key, palm keys D6..F#6. Large holes (0.6 of the bore)
# with pads (Nederveen 1969; Benade 1976; Chen, Smith & Wolfe 2009 on the
# saxophone's resonances). The family is the alto scaled: soprano x2/3, tenor
# x4/3, baritone x2 (the real ones are nearly geometrically similar).

def sax(name, s, transpose, fs, reed, gain, title, top=None):
    top = top or midi('D#6')  # (the palm keys above: their first mode wins, see PLAN)
    slope = 0.0555
    neck_x, neck_d = 62 * s, 12.0 * s
    apex = neck_x - neck_d / slope  # where the cone would end (mm, negative)

    def bore(end, mp=1.0):
        dend = neck_d + slope * (end - 40 * s - neck_x)
        return [(0, 15 * s * mp), (25 * s, 17 * s * mp), (50 * s, 14 * s * mp), (neck_x, neck_d),
                (end - 40 * s, dend), (end, dend * 1.3)]
    names = ['neck', 'F#6', 'F6', 'E6', 'D#6', 'D6', 'body', 'C#5', 'C5', 'B4', 'A#4', 'A4', 'G#4',
             'G4', 'F#4', 'F4', 'E4', 'D#4', 'D4', 'C#4', 'C4', 'B3']
    nc = {'neck', 'body', 'F#6', 'F6', 'E6', 'D#6', 'D6', 'A#4', 'G#4', 'D#4', 'C#4', 'C5', 'F#4'}

    def xs(m):  # first guess: where an open end would sound m (sounding)
        f = 440 * 2 ** ((m + transpose - 69) / 12)
        return 349.5e3 / (2 * f) + apex - 35 * s
    holes = []
    for n in names:
        if n in ('neck', 'body'):
            x0 = 0.42 * xs(midi('D4')) if n == 'neck' else 0.5 * xs(midi('C#5'))
            # Each vent near the pressure node of the second mode it serves (half
            # the sounding length from the cone's apex): the neck's for A5..C#6,
            # the body's for D5..G#5.
            lo, hi = ((110 * s, 190 * s) if n == 'neck' else (200 * s, 330 * s))
            holes.append(hole(n, 0.5 * (lo + hi), (2.6 if n == 'neck' else 3.2) * s, 5 * s, padH=1.5 * s, cup=8 * s,
                              flags=NCLOSED, lo=lo, hi=hi))
            continue
        m = midi(n)
        palm = m >= midi('D6')
        x0 = xs(m - 12) if palm else xs(m)
        bd = neck_d + slope * (x0 - neck_x)
        d = (0.45 if palm else 0.62) * bd
        kw = dict(lo=200 * s, hi=xs(midi('C#5'))) if palm else {}
        holes.append(hole(n, x0, max(d, 6 * s), 4 * s, flags=NCLOSED if n in nc else 0, **kw))
    core = chromatic(holes, 'A#3')
    for m in range(midi('D5'), midi('C#6') + 1):
        pat = core[m - 12][0]
        core[m] = (with_vent(holes, pat, 'body' if m <= midi('G#5') else 'neck'), 2, 0.8)
    for n in ('D6', 'D#6', 'E6', 'F6', 'F#6'):  # palm keys: their own hole, the neck vent, mode 2
        k = names.index(n)
        core[midi(n)] = (first_open(holes, k, vents=('neck',)), 2, 0.6)
    return dict(name=name, title=title, family='sax', kind='reed', transpose=transpose, lo=midi('A#3'),
                hi=top, fs=fs, gain=gain, offset=15, end=xs(midi('A#3')) + 40 * s, bore_fn=bore,
                holes=holes, core=core, gap=10 * s, scale=s, extra=[('mp', 1.0, 0.6, 1.6)],
                doc="""A conical bore: all harmonics, the octave key opens a vent (body or neck)
                for the second register; palm keys open their own holes for D6..F#6.""",
                reed=reed)


# ---------------------------------------------------------------------------
# Oboe (conservatoire system, simplified): a narrow cone from the staple
# (4.6 mm) to ~15.5 mm, the bell; the double reed's own cavity (2-3 mm) and
# staple stand for the missing apex. Small holes in a thick wall; two octave
# vents. Bore after Nederveen 1969 / Almeida et al. 2007 (the oboe reed).

def oboe():
    names = ['oct2', 'oct1', 'C#5', 'C5', 'B4', 'A#4', 'A4', 'G#4', 'G4', 'F#4', 'F4', 'E4', 'D#4', 'D4',
             'C#4', 'C4', 'B3']
    fingers = {'C5', 'B4', 'A4', 'G4', 'F#4', 'E4'}
    nc = {'oct2', 'oct1', 'C#5', 'A#4', 'G#4', 'F4', 'D#4', 'C#4'}

    def bore(end, top=1.0):
        return [(0, 1.8 * top), (10, 2.2 * top), (25, 2.8 * top), (72, 4.6), (end - 60, 15.5), (end - 40, 16.5),
                (end - 20, 22.0), (end, 36.0)]
    apex = -147

    def xs(m):
        return 349.5e3 / (2 * 440 * 2 ** ((m - 69) / 12)) + apex - 30
    holes = []
    for n in names:
        if n.startswith('oct'):
            lo, hi = (100, 170) if n == 'oct2' else (160, 240)
            holes.append(hole(n, 0.5 * (lo + hi), 1.1, 12, padH=1.0, cup=5, flags=NCLOSED, lo=lo, hi=hi))
            continue
        m = midi(n)
        u = (73 - m) / 14
        d = 3.2 + 5.0 * u
        holes.append(hole(n, xs(m), d, 8.5 - 3.5 * u, flags=(FINGER if n in fingers else 0) | (NCLOSED if n in nc else 0)))
    core = chromatic(holes, 'A#3')
    for m in range(midi('D5'), midi('C#6') + 1):
        core[m] = (with_vent(holes, core[m - 12][0], 'oct1' if m <= midi('G#5') else 'oct2'), 2, 0.8)

    def cand(m, notes):
        out = []
        for b in range(midi('A#3'), midi('C#5') + 1):
            for n in (3, 4):
                if abs(b + 12 * math.log2(n) - m) > 1.5:
                    continue
                for vs in ((), ('oct1',), ('oct2',), ('oct1', 'oct2')):
                    p = core[b][0]
                    for v in vs:
                        p = with_vent(holes, p, v)
                    out.append((p, n, len(vs)))
        return out
    return dict(name='oboe', title='Oboe, Bb3..C#6', family='oboe', kind='dreed', transpose=0, lo=midi('A#3'),
                hi=midi('C#6'), fs=48000, gain=0.35, offset=20, end=650, bore_fn=bore, holes=holes, core=core,
                candidates=cand, gap=7, extra=[('top', 2.4, 1.2, 4.0)],
                doc="""A narrow cone and a double reed; octave vents for the second register,
                the third register by search among vented fingerings.""",
                reed=dict(width=7e-3, h0=0.25e-3, k=4.8e7, fr=3000, qr=0.3, sr=3e-5, psi=0, sd=4.5e-6,
                          press=5000, pressHi=7000, vib=0))


# ---------------------------------------------------------------------------
# Bassoon (Heckel, simplified): a 2.5 m cone folded in two, from the bocal
# (4 -> 8.5 mm) to ~40 mm at the bell; small holes drilled obliquely through
# the thick wing and boot joints (long chimneys); flick-key vents for the
# second register. After Nederveen 1969 and Lehman / Fransson on the bassoon.

def bassoon():
    names = ['vent2', 'vent1', 'F3', 'E3', 'D#3', 'D3', 'C#3', 'C3', 'B2', 'A#2', 'A2', 'G#2', 'G2', 'F#2',
             'F2', 'E2', 'D#2', 'D2', 'C#2', 'C2', 'B1']
    fingers = {'F3', 'E3', 'D3', 'C3', 'B2', 'A2'}
    nc = {'vent2', 'vent1', 'D#3', 'C#3', 'A#2', 'G#2', 'F#2', 'D#2', 'C#2'}
    slope = 0.0143

    def bd(x):
        return 8.5 + slope * (x - 355) if x > 355 else 4.0 + 4.5 * (x - 55) / 300

    def bore(end, top=1.0):
        return [(0, 3.0 * top), (55, 4.0 * top), (355, 8.5), (end - 100, bd(end - 100)), (end, bd(end - 100) * 1.08)]
    apex = -212

    def xs(m):
        return 349.5e3 / (2 * 440 * 2 ** ((m - 69) / 12)) + apex - 40
    holes = []
    for n in names:
        if n.startswith('vent'):
            lo, hi = (250, 500) if n == 'vent2' else (500, 800)
            holes.append(hole(n, 0.5 * (lo + hi), 2.0, 10, padH=1.2, cup=7, flags=NCLOSED, lo=lo, hi=hi))
            continue
        m = midi(n)
        x0 = xs(m)
        t = 25 if x0 < 900 else 14 if x0 < 1500 else 6
        holes.append(hole(n, x0, min(16, max(4.5, 0.5 * bd(x0))), t,
                          flags=(FINGER if n in fingers else 0) | (NCLOSED if n in nc else 0)))
    core = chromatic(holes, 'A#1')
    for m in range(midi('F#3'), midi('F4') + 1):
        core[m] = (with_vent(holes, core[m - 12][0], 'vent1' if m <= midi('A#3') else 'vent2'), 2, 0.8)
    return dict(name='bassoon', title='Bassoon, Bb1..Eb4', family='bassoon', kind='dreed', transpose=0,
                lo=midi('A#1'), hi=midi('D#4'), fs=24000, gain=0.6, offset=20, end=xs(midi('A#1')) + 60, bore_fn=bore,
                holes=holes, core=core, gap=10, extra=[('top', 2.0, 0.8, 4.0)],
                doc="""A long narrow cone and a double reed; small holes with long oblique
                chimneys; flick-key vents for the second register.""",
                reed=dict(width=13e-3, h0=0.6e-3, k=1e7, fr=1100, qr=0.35, sr=6e-5, psi=0, sd=1.3e-5,
                          press=2500, pressHi=3800, vib=0))


def finger_cands(holes, core, m, notes, thumb=None, max_cross=2, half=True, ns=(1, 2, 3)):
    """Fingerings a player tries: a closed prefix, then open holes, with up to
    max_cross holes closed below the first open one (cross fingerings) and
    the first open hole perhaps half-holed; `thumb` (index) may be x / o / h."""
    nh = len(holes)
    out = []
    fingers = [i for i in range(nh) if i != thumb]
    for k in range(len(fingers) + 1):
        below = fingers[k:]
        for nx in range(0, max_cross + 1):
            for cross in itertools_combinations(below[1:], nx):
                for hf in ((False, True) if half and k < len(fingers) else (False,)):
                    p = ['x'] * nh
                    for i in below:
                        p[i] = 'x' if i in cross else 'o'
                    if hf:
                        p[below[0]] = 'h'
                    for tc in (('x', 'o', 'h') if thumb is not None else (None,)):
                        if tc:
                            p[thumb] = tc
                        pat = ''.join(p)
                        for n in ns:
                            cost = 2 * nx + (2 if hf else 0) + (1 if tc == 'h' else 0) + 3 * (n - 1)
                            out.append((pat, n, cost))
    return out


def itertools_combinations(seq, n):
    import itertools
    return itertools.combinations(seq, n)


# ---------------------------------------------------------------------------
# Duduk in A: a cylindrical bore of apricot wood (~11 -> 12.5 mm) and a large
# flat double reed (the ghamish, ~ 20 mm wide, soft): closed at the reed, it
# sounds an octave lower than its length suggests; eight finger holes and a
# thumb hole; semitones by half-holing, cross fingerings and the lips (the
# duduk's pitch bends a long way with the lips).

def duduk():
    names = ['thumb', '1', '2', '3', '4', '5', '6', '7', '8']
    holes = [hole(n, 0, 6.0 if n == 'thumb' else 6.8, 5.5, flags=FINGER) for n in names]
    seq = ['C#5', 'B4', 'A4', 'G#4', 'F#4', 'E4', 'D4', 'C#4', 'B3']  # opened from the bottom
    for i, n in enumerate(seq):
        f = 440 * 2 ** ((midi(n) - 69) / 12)
        holes[i]['x'] = 349.5e3 / (4 * f) - 20
    core = {midi('A3'): ('x' * 9, 1, 1.0)}
    for i, n in enumerate(seq):  # hole i is the first open (the thumb last)
        core[midi(n)] = ('x' * i + 'o' * (9 - i), 1, 1.0)

    def bore(end):
        return [(0, 4.5), (30, 6.5), (90, 11.0), (end, 12.5)]

    def cand(m, notes):
        return finger_cands(holes, core, m, notes, thumb=None, ns=(1,))
    return dict(name='duduk', title='Duduk in A, A3..C#5', family='duduk', kind='dreed', transpose=0,
                lo=midi('A3'), hi=midi('C#5'), fs=48000, gain=0.3, offset=35, end=380, bore_fn=bore, holes=holes,
                core=core, candidates=cand, gap=12,
                doc="""A cylindrical bore and a large soft double reed; diatonic finger holes,
                the semitones by half-holing and cross fingerings (and the lips).""",
                reed=dict(width=18e-3, h0=0.5e-3, k=5e6, fr=900, qr=0.5, sr=8e-5, psi=0, sd=2e-5,
                          press=1100, pressHi=1500, vib=0.6))


# ---------------------------------------------------------------------------
# Treble shawm in D (Renaissance): a wide cone and a flared bell, a large
# double reed played with the lips against the pirouette (not on the reed:
# little damping), seven finger holes. Loud. Second register by overblowing.

def shawm():
    names = ['1', '2', '3', '4', '5', '6', '7']
    holes = [hole(n, 0, 7.0, 6.0, flags=FINGER) for n in names]
    seq = ['C#5', 'B4', 'A4', 'G4', 'F#4', 'E4']
    apex = -57

    def xs(m):
        return 349.5e3 / (2 * 440 * 2 ** ((m - 69) / 12)) + apex - 40
    holes[0]['x'] = xs(midi('D5'))
    for i, n in enumerate(seq):
        holes[i + 1]['x'] = xs(midi(n))
    core = {midi('D4'): ('x' * 7, 1, 1.0)}
    for i, n in enumerate(['D5'] + seq):
        core[midi(n)] = ('x' * i + 'o' * (7 - i), 1, 1.0)

    # (A cone running to the reed with a small reed, which fixes the octave on
    # paper, played 100-270 c flat and unsteady: the second register is open.)
    def bore(end, top=1.0):
        return [(0, 3.0 * top), (30, 4.5 * top), (60, 5.0), (end - 80, 28.0), (end - 40, 38.0), (end, 80.0)]

    def cand(m, notes):
        return finger_cands(holes, core, m, notes, ns=(1,))
    return dict(name='shawm', title='Treble shawm in D, D4..D5', family='shawm', kind='dreed', transpose=0,
                lo=midi('D4'), hi=midi('D5'), fs=48000, gain=0.2, offset=40, end=xs(midi('D4')) + 80, bore_fn=bore,
                holes=holes, core=core, candidates=cand, gap=12, extra=[('top', 1.0, 0.5, 2.0)],
                doc="""A wide cone, a flared bell and a large double reed behind a pirouette
                (the lips barely damp it); seven finger holes.""",
                reed=dict(width=12e-3, h0=0.6e-3, k=1e7, fr=2500, qr=0.12, sr=5e-5, psi=0, sd=1.6e-5,
                          press=2800, pressHi=3800, vib=0))


# ---------------------------------------------------------------------------
# Alto recorder in F (Baroque fingering): a duct (windway ~1.1 x 12 mm), the
# window and labium at the top, a reverse-conical bore (19 -> 12 mm), a thumb
# hole and seven finger holes, the lowest two double. The thumb half-holed
# opens the second register.

def recorder():
    names = ['T', '1', '2', '3', '4', '5', '6a', '6b', '7a', '7b']
    ds = [6.0, 6.6, 6.4, 5.6, 6.4, 5.8, 4.8, 3.4, 4.2, 3.0]
    holes = [hole(n, 0, d, 5.0, flags=FINGER) for n, d in zip(names, ds)]

    def bore(end):
        f = [(0, 19.0), (0.12, 19.0), (0.3, 17.6), (0.5, 15.6), (0.7, 13.6), (0.88, 12.2), (0.96, 12.0), (1.0, 12.6)]
        return [(u * end, d) for u, d in f]
    pos = {'T': 'C5', '1': 'E5', '2': 'D5', '3': 'C5', '4': 'B4', '5': 'A4', '6a': 'G4', '6b': 'G4',
           '7a': 'F#4', '7b': 'F#4'}
    for h in holes:
        f = 440 * 2 ** ((midi(pos[h['name']]) - 69) / 12)
        h['x'] = 349.5e3 / (2 * f) - 45 + (6 if h['name'].endswith('b') else 0)
    holes[0]['x'] = holes[3]['x'] - 30  # the thumb is above hole 1 on the back
    core = {
        midi('F4'): ('xxxxxxxxxx', 1, 1.0), midi('G4'): ('xxxxxxxxoo', 1, 1.0), midi('A4'): ('xxxxxxoooo', 1, 1.0),
        midi('A#4'): ('xxxxxoxxxx', 1, 0.6), midi('C5'): ('xxxxoooooo', 1, 1.0), midi('D5'): ('xxxooooooo', 1, 1.0),
        midi('E5'): ('xxoooooooo', 1, 1.0), midi('F5'): ('xoxooooooo', 1, 0.6), midi('G5'): ('ooxooooooo', 1, 0.6),
        midi('A5'): ('hxxxxxoooo', 2, 0.6), midi('C6'): ('hxxxoooooo', 2, 0.6), midi('D6'): ('hxxooooooo', 2, 0.6),
        midi('F#4'): ('xxxxxxxxxo', 1, 0.6), midi('G#4'): ('xxxxxxxooo', 1, 0.6),
    }

    def cand(m, notes):
        out = []
        for pat, n, cost in finger_cands(holes, core, m, notes, thumb=0, max_cross=2, half=False, ns=(1, 2, 3)):
            # the double holes: a finger covers both, or only the larger (a half hole)
            if pat[6] == 'o' and pat[7] == 'x' or pat[8] == 'o' and pat[9] == 'x':
                continue
            out.append((pat, n, cost))
        return out
    return dict(name='recorder', title='Alto recorder in F (Baroque fingering), F4..E6', family='recorder',
                kind='duct', transpose=0, lo=midi('F4'), hi=midi('E6'), fs=48000, gain=1.0, end=440, bore_fn=bore,
                holes=holes, core=core, candidates=cand, gap=9, emb=1.0, fit='xd',
                doc="""A duct forms the jet; a reverse-conical bore; the thumb half-holed opens
                the second register. The breath sets the pitch: louder is sharper.""",
                jet=dict(h=1.1, width=12, w=3.0, window=12, chimney=2.0, channel=55, y0=0.2, press=250, pressHi=1500))


# ---------------------------------------------------------------------------
# Shakuhachi 1.8 (in D): end-blown; the lower lip covers part of the open top
# (the utaguchi's edge), whose coverage bends the pitch a semitone or more
# (meri / kari); four front holes and a thumb hole; a bore narrowing from
# 20 mm to ~16.5 mm and widening again to the root end.

def shakuhachi():
    names = ['5', '4', '3', '2', '1']  # top to bottom: the thumb hole (back), then 4 .. 1
    holes = [hole(n, 0, 10.0 if n != '5' else 9.5, 6.0, flags=FINGER) for n in names]
    seq = {'1': 'F4', '2': 'G4', '3': 'A4', '4': 'C5', '5': 'D5'}
    for h in holes:
        f = 440 * 2 ** ((midi(seq[h['name']]) - 69) / 12)
        h['x'] = 349.5e3 / (2 * f) - 40
    core = {midi('D4'): ('xxxxx', 1, 1.0), midi('F4'): ('xxxxo', 1, 1.0), midi('G4'): ('xxxoo', 1, 1.0),
            midi('A4'): ('xxooo', 1, 1.0), midi('C5'): ('xoooo', 1, 1.0),
            midi('D5'): ('xxxxx', 2, 0.7), midi('F5'): ('xxxxo', 2, 0.7), midi('G5'): ('xxxoo', 2, 0.7),
            midi('A5'): ('xxooo', 2, 0.7), midi('C6'): ('xoooo', 2, 0.7)}

    def bore(end, narrow=16.5, top=20.5, root=21.0):
        f = [(0, top), (0.11, 0.5 * (top + 19.5)), (0.37, 0.5 * (narrow + 18.5)), (0.55, narrow),
             (0.73, 0.5 * (narrow + 17.5)), (0.88, 0.5 * (root + 17)), (1.0, root)]
        return [(u * end, d) for u, d in f]

    def cand(m, notes):
        out = []
        for p in itertools_product('xoh', 5):
            pat = ''.join(p)
            nh = pat.count('h')
            if nh > 1:
                continue
            for n in (1, 2, 3):
                out.append((pat, n, 2 * nh + 3 * (n - 1) + sum(1 for a, b in zip(pat, pat[1:]) if a == 'o' and b == 'x')))
        return out
    return dict(name='shakuhachi', title='Shakuhachi 1.8 (in D), D4..C#6', family='shakuhachi', kind='end',
                transpose=0, lo=midi('D4'), hi=midi('C#6'), fs=48000, gain=1.0, end=545, bore_fn=bore, holes=holes,
                core=core, candidates=cand, gap=15, emb=1.3, fit='xd', extra=[('narrow', 16.5, 12.0, 20.0), ('top', 20.5, 16.0, 24.0), ('root', 21.0, 15.0, 28.0)],
                doc="""End-blown: the lips cover part of the top's rim, which bends the pitch
                (meri / kari); four front holes, a thumb hole.""",
                jet=dict(chimney=1.0))


def itertools_product(a, n):
    import itertools
    return itertools.product(a, repeat=n)


# ---------------------------------------------------------------------------
# Alto C ocarina, 12 holes: a vessel (a Helmholtz resonator: the pitch is set
# by the open holes' total area, not where they are), a duct, sounding A4..F6.
# The holes in the order a rising scale opens them (a standard 12-hole chart):
# the two small sub-holes beside the middle fingers' holes (all closed: A4;
# the right one open: B4; both open: C5), the right hand's four from the little
# finger up (D5..G5), the left hand's four (A5..D6; C6: every front hole but
# L1 open), then the left thumb (E6) and the right thumb (F6: every hole open).
# The player's pitch is the breath: low notes soft, high notes much harder.

def ocarina():
    names = ['sub-R', 'sub-L', 'R4', 'R3', 'R2', 'R1', 'L4', 'L3', 'L2', 'L1', 'thumb-L', 'thumb-R']
    holes = [hole(n, 10 + 4 * i, 4.0 + 0.5 * i, 4.0, flags=FINGER) for i, n in enumerate(names)]
    seq = ['A4', 'B4', 'C5', 'D5', 'E5', 'F5', 'G5', 'A5', 'B5', 'C6', 'D6', 'E6', 'F6']
    core = {midi(n): ('o' * i + 'x' * (12 - i), 1, 1.0) for i, n in enumerate(seq)}

    def cand(m, notes):
        out = []
        base = [core[k][0] for k in (m - 1, m + 1) if k in core]
        for b in base:
            for i in range(12):
                for j in range(i, 12):
                    p = list(b)
                    p[i] = 'o' if p[i] == 'x' else 'x'
                    if j != i:
                        p[j] = 'o' if p[j] == 'x' else 'x'
                    out.append((''.join(p), 1, 2 if j == i else 4))
                    pp = list(b)
                    pp[i] = 'h'
                    out.append((''.join(pp), 1, 2))
        return out
    return dict(name='ocarina', title='Alto C ocarina (12 holes), A4..F6', family='ocarina', kind='duct',
                transpose=0, lo=midi('A4'), hi=midi('F6'), fs=48000, gain=0.5, end=62, fit='d', closed=True,
                bore=[(0, 36.0), (1000, 36.0)], holes=holes, core=core, candidates=cand, emb=1.0,
                doc="""A vessel flute: the pitch follows the open holes' total area (Helmholtz);
                a duct forms the jet.""",
                jet=dict(h=0.9, width=10, w=4.5, window=10, chimney=3.0, channel=20, y0=0.15, press=320, pressHi=1300))


# ---------------------------------------------------------------------------
# Pan flute (nai), chromatic: one stopped pipe per note, blown across its rim;
# the lips cover part of the top (tilting the pipe bends the pitch).

def panflute():
    lo, hi = midi('G4'), midi('C7')

    def dia(m):
        return 15.0 * (6.5 / 15.0) ** ((m - lo) / (hi - lo))

    def pipes(spec, Model):
        out = []
        for m in range(lo, hi + 1):
            d = dia(m)
            t = 440 * 2 ** ((m - 69) / 12)
            exc = 0.5 * 349.5 / (0.975 * spec['fs']) * 1e3
            a, b = 5.0, 349.5e3 / (4 * t) + 5
            for _ in range(40):
                L = 0.5 * (a + b)
                sp = dict(spec, bore=[(0, d), (2000, d)], end=exc + L, closed=True, holes=[], excite=exc)
                sp.pop('bore_fn', None)
                f = Model(sp).resonance('', 1, t, m)
                if f is None or f > t:
                    a = L
                else:
                    b = L
            out.append((m, 0.5 * (a + b), d))
        return out
    return dict(name='panflute', title='Pan flute (nai), chromatic, G4..C7', family='panflute', kind='end',
                transpose=0, lo=lo, hi=hi, fs=96000, gain=1.0, end=200, closed=True,
                bore=[(0, 15.0), (1000, 15.0)], holes=[], core={}, pipes_fn=pipes, emb=0.8,
                doc="""One stopped pipe per note, blown across the rim.""", jet=dict(chimney=0.5))


INSTRUMENTS = {}
for f in (clarinet, oboe, bassoon, duduk, shawm, recorder, shakuhachi, ocarina, panflute,
          lambda: sax('soprano-sax', 2 / 3, -2, 48000, dict(width=11e-3, h0=0.8e-3, k=4.5e6, fr=2400, qr=0.3,
                      sr=4.3e-5, press=1700, pressHi=2300, vib=1), 0.5, 'Soprano saxophone in Bb, written Bb3..D6', midi('D6')),
          lambda: sax('alto-sax', 1.0, -9, 48000, dict(width=13e-3, h0=0.8e-3, k=4e6, fr=1900, qr=0.3,
                      sr=6e-5, press=1500, pressHi=2100, vib=1), 0.5, 'Alto saxophone in Eb, written Bb3..D#6'),
          lambda: sax('tenor-sax', 4 / 3, -14, 36000, dict(width=15e-3, h0=0.85e-3, k=3.5e6, fr=1500, qr=0.3,
                      sr=8e-5, press=1400, pressHi=2000, vib=1), 0.4, 'Tenor saxophone in Bb, written Bb3..D#6'),
          # (the baritone: tools/design.py -scale alto-sax baritone-sax 2; its reed's
          # compliance sr / k, part of the cone's apex volume, x 2^3 of the alto's)
          lambda: sax('baritone-sax', 2.0, -21, 24000, dict(width=17e-3, h0=0.9e-3, k=3e6, fr=1150, qr=0.3,
                      sr=3.6e-4, press=1300, pressHi=1900, vib=1), 0.45, 'Baritone saxophone in Eb, written Bb3..D#6'),
          ):
    s = f()
    INSTRUMENTS[s['name']] = s
