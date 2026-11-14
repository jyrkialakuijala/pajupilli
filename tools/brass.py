#!/usr/bin/env python3
"""Transfer-matrix (frequency-domain) model of the brass instruments in
src/brass.cc, to design their bores and check the waveguide (./flute -imp):
the mouthpiece (the cup a volume, the throat and backbore resolved), the bore
from the throat to the bell, the valve loops added in the cylindrical
section, the bell's radiation load as in src/bore.cc (Levine-Schwinger at low
ka: R parallel to the end-correction mass). Prints the input impedance peaks
seen by the lips, per valve combination, with the partial number and cents
against the harmonic series of the open horn's nominal fundamental.

    tools/brass.py [-inst horn] [-valves 0|2|1|12|23|13|123 ...] [-fit]

Mirrors src/brass.cc (kHornBore, the cup, the valve loops); keep the two in step.
"""
import math, sys
import numpy as np

rho, c, nu = 1.18, 349.5, 1.56e-5   # src/bore.h

def bessel_bell(x0, x1, d0, d1, eps):
    """Points (mm) of a Bessel horn r = B (xa - x)^-eps through (x0, d0), (x1, d1)."""
    # (xa - x0) / (xa - x1) = (d1 / d0)^(1 / eps)
    q = (d1 / d0) ** (1 / eps)
    xa = (q * x1 - x0) / (q - 1)
    pts = []
    for k in range(25):
        u = k / 24
        y = (xa - x1) * ((xa - x0) / (xa - x1)) ** (1 - u)  # denser near the mouth
        x = xa - y
        pts.append((x, d1 * ((xa - x1) / y) ** eps))
    return pts


def diam(profile, x):
    for (x0, d0), (x1, d1) in zip(profile, profile[1:]):
        if x <= x1:
            return d0 + (d1 - d0) * (x - x0) / max(1e-9, x1 - x0)
    return profile[-1][1]


def hermite_blend(f, xk, half, step=25.0):
    """Points (mm) of a smooth (C1) path replacing f around a kink at xk."""
    xa, xb = xk - half, xk + half
    da, db = f(xa), f(xb)
    sa, sb = f(xa) - f(xa - 1), f(xb + 1) - f(xb)
    n = max(2, int(round(2 * half / step)))
    pts = []
    for k in range(n + 1):
        t = k / n
        h00, h10 = 2 * t ** 3 - 3 * t ** 2 + 1, t ** 3 - 2 * t ** 2 + t
        h01, h11 = -2 * t ** 3 + 3 * t ** 2, t ** 3 - t ** 2
        pts.append((xa + t * (xb - xa), h00 * da + h10 * (xb - xa) * sa + h01 * db + h11 * (xb - xa) * sb))
    return pts


def horn_profile(L=3830.1, bell=439.1, eps=0.7306, dBellStart=68.7, dMouth=305.0, cone=(1919.3, 11.6),
                 blend=400.0):
    """The single F horn's bore, mm from the lips: throat, backbore, leadpipe,
    the cylindrical valve section, the long cone to the bell, a Bessel flare.
    The leadpipe and the cone meet the cylinder smoothly (Hermite blends over
    +-blend mm): a kink reflects, and every other partial came out 2-3 dB weaker.
    The flare (length, exponent, where it starts) and the cone's start are
    fitted for the most even partials 3..16, the length to put them on F1's
    series."""
    xb = L - bell
    base = [(0.0, 4.4), (11.6, 4.4), (60.0, 8.0), (700.0, 11.6), cone, (xb, dBellStart)]
    base += bessel_bell(xb, L, dBellStart, dMouth, eps)[1:]
    if not blend:
        return base
    f = lambda x: diam(base, x)
    out = []
    knots = [700.0, cone[0]]
    i = 0
    for x, d in base:
        while i < len(knots) and x > knots[i] - blend:
            out += hermite_blend(f, knots[i], blend)
            i += 1
        if out and x <= out[-1][0]:
            continue
        out.append((x, d))
    return out


HORN = dict(
    name='horn', transpose=-7, f0=43.654,  # F1: the open F horn's nominal fundamental
    profile=horn_profile(),
    cup=1.5e-6,          # cup volume (m^3), lumped at the lips
    throat=5.6,          # the throat starts here (mm): the cup is above it
    # Valves: junction on the bore (mm from the lips), the loop's added length (mm), bore (mm).
    valves={'1': (950.0, 477.8, 11.6), '2': (1000.0, 228.1, 11.6), '3': (1050.0, 741.0, 11.6)},
    semis={'1': 2, '2': 1, '3': 3, '4': 5},
)


def tube_step(w, L, a, Z):
    S = np.pi * a * a
    kap = 2.94 * rho * np.sqrt(nu) / (S * a)
    Zs = 1j * w * rho / S + kap * np.sqrt(1j * w); Ys = 1j * w * S / (rho * c * c)
    g = np.sqrt(Zs * Ys); Zc = np.sqrt(Zs / Ys)
    ch, sh = np.cosh(g * L), np.sinh(g * L)
    return (ch * Z + Zc * sh) / (sh / Zc * Z + ch)


def bell_load(w, a):
    S = np.pi * a * a
    L_r = rho * 0.6133 * a / S; R_r = 1.505 * rho * c / S
    return 1j * w * L_r * R_r / (R_r + 1j * w * L_r)


def Zin(spec, f, pressed='', step=2.0, loops=None, slide=0.0):
    """Input impedance at the lips (Pa s / m^3). pressed: the valves down
    ('13'); slide: the slide's extension (mm of added tube, in its cylinder)."""
    w = 2 * np.pi * np.asarray(f, float)
    prof = spec['profile']
    L = prof[-1][0]
    Z = bell_load(w, 0.5e-3 * prof[-1][1])
    # Insertions, bottom up: a pressed valve's loop (a cylinder) at its port;
    # the slide's extra tube in the middle of the slide.
    loops = loops or {}
    ins = [(spec['valves'][v][0], loops.get(v, spec['valves'][v][1]), spec['valves'][v][2]) for v in pressed]
    ins.sort(reverse=True)
    # The slide: every step between its ends stretched alike (as src/horn.cc
    # stretches its cells), adding `slide` mm of tube.
    st = 1.0
    if slide and 'slide' in spec:
        x0, x1 = spec['slide']
        st = 1 + slide / (x1 - x0)
    def run(xa, xb):  # the bore from xb down to xa (xa < xb)
        nonlocal Z
        cuts = [xa, xb]
        if st != 1:
            cuts += [x for x in spec['slide'] if xa < x < xb]
        cuts = sorted(cuts, reverse=True)
        for hi_, lo_ in zip(cuts, cuts[1:]):
            n = max(1, int(math.ceil((hi_ - lo_) / step)))
            k = st if st != 1 and spec['slide'][0] <= 0.5 * (hi_ + lo_) <= spec['slide'][1] else 1
            for j in range(n):
                xm = hi_ - (hi_ - lo_) * (j + 0.5) / n
                Z = tube_step(w, (hi_ - lo_) / n * k * 1e-3, 0.5e-3 * diam(prof, xm), Z)
    x = L
    for xs, add, dv in ins + [(spec['throat'], 0, 0)]:
        run(xs, x)
        x = xs
        if add:
            n = max(1, int(math.ceil(add / step)))
            for j in range(n):
                Z = tube_step(w, add / n * 1e-3, 0.5e-3 * dv, Z)
    # The cup in parallel at the lips.
    return 1 / (1 / Z + 1j * w * spec['cup'] / (rho * c * c))


def peaks(spec, pressed='', fmax=1400, **kw):
    f = np.arange(15, fmax, 0.25)
    Y = np.log(np.abs(Zin(spec, f, pressed, **kw)))
    i = np.where((Y[1:-1] > Y[:-2]) & (Y[1:-1] > Y[2:]))[0] + 1
    d = 0.5 * (Y[i - 1] - Y[i + 1]) / (Y[i - 1] - 2 * Y[i] + Y[i + 1])
    return f[i] + d * 0.25, Y[i]


def semis(spec, pressed):
    return sum(spec['semis'][v] for v in pressed)


def report(spec, pressed='', position=1, **kw):
    """Peaks: (partial, Hz, cents vs the nominal series, dB). position: the slide's."""
    s = semis(spec, pressed) + position - 1
    if position > 1:
        kw['slide'] = spec['slides'][position - 1]
    f0 = spec['f0'] * 2 ** (-s / 12)
    pk, lv = peaks(spec, pressed, fmax=spec.get('fmax', 1400), **kw)
    out = []
    for f, l in zip(pk, lv):
        n = max(1, round(f / f0))
        out.append((n, f, 1200 * math.log2(f / (n * f0)), 20 / math.log(10) * l))
    return out


def mean_err(r, lo, hi):
    return np.mean([cc for n, f, cc, l in r if lo <= n <= hi])


def fit_loops(spec):
    """Loop lengths that put partials lo..hi of each single valve on its semitones."""
    lo, hi = spec.get('fitPartials', (4, 12))
    res = {}
    for v in spec['valves']:
        th = spec['profile'][-1][0] * (2 ** (spec['semis'][v] / 12) - 1)  # a cylinder's share
        a, b = 0.6 * th, 1.5 * th
        for _ in range(40):
            mid = 0.5 * (a + b)
            if mean_err(report(spec, v, loops={v: mid}), lo, hi) > 0: a = mid
            else: b = mid
        res[v] = 0.5 * (a + b)
    return res


def fit_slide(spec):
    """The slide's extension (mm of tube) per position 1..7: partials lo..hi on its semitones."""
    lo, hi = spec.get('fitPartials', (2, 8))
    out = [0.0]
    for pos in range(2, 8):
        th = spec['profile'][-1][0] * (2 ** ((pos - 1) / 12) - 1)
        a, b = 0.6 * th, 1.6 * th
        for _ in range(40):
            mid = 0.5 * (a + b)
            spec['slides'][pos - 1] = mid
            if mean_err(report(spec, '', pos), lo, hi) > 0: a = mid
            else: b = mid
        out.append(0.5 * (a + b))
        spec['slides'][pos - 1] = out[-1]
    return out


# Smith-Watkins B-flat trumpet, Kelly Screamer mouthpiece (Bilbao & Chick 2013,
# Fig. 7, digitised: research/trumpet.md 1.3). x from the throat entry.
TRUMPET = dict(
    name='trumpet', transpose=-2, f0=116.54 / 2 ** (-0 / 12),  # B-flat 2's series (sounding)
    # (+44.9 mm at 400 mm: the main tuning slide pulled for warm, breathed air.)
    profile=[(0.0, 4.16), (1.9, 4.07), (6.0, 4.07), (42.4, 5.79), (47.4, 6.2), (52.5, 6.49), (57.2, 6.8), (62.3, 7.1), (67.3, 7.49), (77.4, 8.1), (77.9, 9.69), (78.4, 9.58), (78.9, 8.7), (81.2, 8.7), (86.3, 8.8), (88.2, 8.7), (150.5, 10.08), (200.4, 10.78), (250.4, 11.38), (275.4, 11.71), (308.3, 11.75), (400.0, 11.75), (761.4, 11.75), (776.2, 11.79), (876.5, 11.79), (976.4, 12.89), (1016.2, 13.8), (1066.2, 14.87), (1076.3, 15.3), (1116.5, 16.49), (1166.4, 18.29), (1176.2, 18.76), (1216.4, 20.51), (1277.4, 24.52), (1323.3, 29.94), (1338.8, 33.09), (1352.4, 36.57), (1362.5, 40.42), (1378.3, 49.37), (1383.3, 54.58), (1389.3, 60.29), (1393.8, 66.64), (1398.8, 73.67), (1402.9, 81.41), (1406.4, 89.98), (1410.8, 99.44), (1413.4, 109.9), (1416.5, 126.77)],
    cup=0.92e-6,
    throat=3.7,  # half a cell at 48 kHz
    valves={'1': (765.0, 182.8, 11.75), '2': (710.0, 89.1, 11.75), '3': (655.0, 281.6, 11.75)},
    ring={'13': 30.9, '123': 57.5},  # the 3rd slide out (mm of tube)
    semis={'1': 2, '2': 1, '3': 3},
    fitPartials=(3, 8), fmax=2000,
)

# CC tuba, 4 valves (research/tuba.md 2.2: a bore fitted to the measured
# resonance pattern of a Besson BE994, transposed to C; the shape partly a guess).
TUBA = dict(
    name='tuba', transpose=0, f0=32.703,
    profile=[(0.0, 8.30), (14.0, 8.30), (90.0, 13.00), (534.7, 18.12), (558.7, 18.38), (582.7, 18.62), (606.7, 18.82),
             (630.7, 19.00), (654.7, 19.15), (678.7, 19.28), (702.7, 19.38), (726.7, 19.44), (750.7, 19.49), (774.7, 19.50),
             (1211.0, 19.50), (1235.0, 19.52), (1259.0, 19.58), (1283.0, 19.67), (1307.0, 19.81), (1331.0, 19.98),
             (1355.0, 20.19), (1379.0, 20.44), (1403.0, 20.73), (1427.0, 21.05), (1451.0, 21.41), (3423.6, 52.89),
             (3447.6, 53.37), (3471.6, 54.07), (3495.6, 54.96), (3519.6, 56.07), (3543.6, 57.38), (3567.6, 58.90),
             (3591.6, 60.62), (3615.6, 62.55), (3639.6, 64.69), (3663.6, 67.03), (4259.9, 127.79), (4364.6, 134.91),
             (4456.0, 142.44), (4535.8, 150.38), (4605.4, 158.77), (4666.3, 167.62), (4719.4, 176.97), (4765.8, 186.84),
             (4806.3, 197.25), (4841.7, 208.25), (4872.5, 219.87), (4899.5, 232.13), (4923.0, 245.07), (4943.6, 258.74),
             (4961.5, 273.17), (4977.2, 288.40), (4990.9, 304.48), (5002.8, 321.46), (5013.3, 339.39), (5022.4, 358.31),
             (5030.3, 378.29), (5037.3, 399.39), (5043.3, 421.66), (5048.6, 445.18), (5053.2, 470.00)],
    cup=9.0e-6, throat=9.0,  # half a cell at 20 kHz
    valves={'1': (900.0, 638.6, 19.5), '2': (800.0, 300.8, 19.5), '3': (1000.0, 999.1, 19.5), '4': (1100.0, 1804.6, 19.5)},
    semis={'1': 2, '2': 1, '3': 3, '4': 5},
    fitPartials=(2, 10), fmax=600,
)

# Tenor trombone with F attachment: the Conn 8H (Bilbao & Chick 2013, Fig. 7;
# research/trombone.md), x from the throat, the tuning slide pulled 75 mm.
# The slide (62.6..1567.4 mm) stretches; 'slides': mm of tube per position.
TROMBONE = dict(
    name='trombone', transpose=0, f0=58.27,
    profile=[(0.0, 8.0), (2.5, 7.23), (17.5, 7.69), (22.5, 8.08), (27.5, 8.37), (32.5, 8.89), (42.5, 9.87), (47.5, 10.63), (52.6, 12.72), (57.6, 12.76), (62.6, 13.6), (662.5, 13.6), (737.7, 14.79), (892.3, 13.6), (1567.4, 13.6), (1639.4, 13.84), (1667.7, 14.23), (1720.0, 14.33), (1843.2, 14.43), (1878.3, 14.48), (1979.8, 15.16), (2180.2, 19.38), (2187.0, 19.38), (2289.1, 21.46), (2376.8, 24.44), (2389.3, 25.5), (2476.3, 29.98), (2488.9, 31.04), (2522.0, 33.07), (2562.1, 36.62), (2589.0, 39.0), (2600.3, 40.4), (2632.3, 44.75), (2676.1, 54.47), (2689.2, 60.53), (2705.5, 66.79), (2716.8, 73.69), (2726.2, 81.31), (2734.3, 90.03), (2743.1, 99.34), (2749.3, 109.61), (2755.6, 121.36), (2761.8, 133.91), (2771.8, 164.17), (2777.5, 181.15), (2781.8, 199.87), (2783.7, 213.74)],
    cup=9.0e-6, throat=5.6,
    valves={'F': (1673.0, 966.7, 13.9)}, semis={'F': 5},
    slide=(62.6, 1567.4), slides=[0, 171.9, 354.3, 546.8, 748.9, 960.8, 1183.9],
    fitPartials=(3, 8),
)

INSTS = {'horn': HORN, 'trumpet': TRUMPET, 'tuba': TUBA, 'trombone': TROMBONE}


if __name__ == '__main__':
    args = sys.argv[1:]
    combos, positions = [], []
    fit = False
    name = 'horn'
    while args:
        a = args.pop(0)
        if a == '-valves': combos.append(args.pop(0).replace('0', ''))
        elif a == '-pos': positions.append(int(args.pop(0)))
        elif a == '-inst': name = args.pop(0)
        elif a == '-fit': fit = True
    spec = INSTS[name]
    if fit:
        if spec['valves']:
            lp = fit_loops(spec)
            print('loops (mm):', {k: round(v, 1) for k, v in lp.items()})
            for v, add in lp.items():
                xv, _, dv = spec['valves'][v]
                spec['valves'][v] = (xv, add, dv)
        if 'slides' in spec:
            print('slide (mm):', [round(v, 1) for v in fit_slide(spec)])
    for pos in positions or [1]:
        for p in combos or ['']:
            r = report(spec, p, pos)
            print(f'pos {pos} valves {p or "0"}: ' + '  '.join(f'{n}:{f:.1f}({cc:+.0f}c,{l:.0f}dB)' for n, f, cc, l in r))
