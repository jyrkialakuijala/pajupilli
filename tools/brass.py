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


def Zin(spec, f, pressed='', step=2.0, loops=None):
    """Input impedance at the lips (Pa s / m^3)."""
    w = 2 * np.pi * np.asarray(f, float)
    prof = spec['profile']
    L = prof[-1][0]
    Z = bell_load(w, 0.5e-3 * prof[-1][1])
    # Valve junctions, bottom up: a pressed valve inserts its loop (a cylinder) there.
    loops = loops or {}
    ins = sorted(((spec['valves'][v][0], v) for v in pressed), reverse=True)
    x = L
    stops = [xv for xv, _ in ins] + [spec['throat']]
    vi = 0
    for xs in stops:
        n = max(1, int(math.ceil((x - xs) / step)))
        for j in range(n):
            xm = x - (x - xs) * (j + 0.5) / n
            Z = tube_step(w, (x - xs) / n * 1e-3, 0.5e-3 * diam(prof, xm), Z)
        x = xs
        if vi < len(ins):
            v = ins[vi][1]
            xv, add, dv = spec['valves'][v]
            add = loops.get(v, add)
            n = max(1, int(math.ceil(add / step)))
            for j in range(n):
                Z = tube_step(w, add / n * 1e-3, 0.5e-3 * dv, Z)
            vi += 1
    # The cup in parallel at the lips.
    return 1 / (1 / Z + 1j * w * spec['cup'] / (rho * c * c))


def peaks(spec, pressed='', fmax=1400, **kw):
    f = np.arange(20, fmax, 0.25)
    Y = np.log(np.abs(Zin(spec, f, pressed, **kw)))
    i = np.where((Y[1:-1] > Y[:-2]) & (Y[1:-1] > Y[2:]))[0] + 1
    d = 0.5 * (Y[i - 1] - Y[i + 1]) / (Y[i - 1] - 2 * Y[i] + Y[i + 1])
    return f[i] + d * 0.25, Y[i]


SEMIS = {'': 0, '2': 1, '1': 2, '12': 3, '3': 3, '23': 4, '13': 5, '123': 6}


def report(spec, pressed='', **kw):
    f0 = spec['f0'] * 2 ** (-SEMIS[pressed] / 12)
    pk, lv = peaks(spec, pressed, **kw)
    out = []
    for f, l in zip(pk, lv):
        n = max(1, round(f / f0))
        out.append((n, f, 1200 * math.log2(f / (n * f0)), 20 / math.log(10) * l))
    return out


def fit_loops(spec):
    """Loop lengths that put partials 4..12 of each single valve on its semitones."""
    res = {}
    for v, semis in (('2', 1), ('1', 2), ('3', 3)):
        th = spec['profile'][-1][0] * (2 ** (semis / 12) - 1)  # a cylinder's share
        lo, hi = 0.7 * th, 1.4 * th
        for _ in range(40):
            mid = 0.5 * (lo + hi)
            r = report(spec, v, loops={v: mid})
            err = np.mean([cc for n, f, cc, l in r if 4 <= n <= 12])
            if err > 0: lo = mid
            else: hi = mid
        res[v] = 0.5 * (lo + hi)
    return res


if __name__ == '__main__':
    args = sys.argv[1:]
    combos = []
    fit = False
    while args:
        a = args.pop(0)
        if a == '-valves': combos.append(args.pop(0).replace('0', ''))
        elif a == '-fit': fit = True
    spec = HORN
    if fit:
        lp = fit_loops(spec)
        print('loops (mm):', {k: round(v, 1) for k, v in lp.items()})
        for v, add in lp.items():
            xv, _, dv = spec['valves'][v]
            spec['valves'][v] = (xv, add, dv)
    for p in combos or ['']:
        r = report(spec, p)
        print(f'valves {p or "0"}: ' + '  '.join(f'{n}:{f:.1f}({cc:+.0f}c,{l:.0f}dB)' for n, f, cc, l in r))
