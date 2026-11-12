#!/usr/bin/env python3
"""Transfer-matrix (frequency-domain) model of the same instruments, to check the
time-domain waveguide: bore from the cork, tone holes as shunt impedances with
their series (negative) inertance, the embouchure window in series, the foot
with the unflanged radiation impedance. Prints the resonances (admittance peaks
seen from the window) per fingering.

Mirrors src/flute.cc (Instrument, kSpec, holeBranch, embouchureGeometry); keep
the two in step.

    tools/tmm.py [-inst NAME] [-extra MM] [MIDI...]
"""
import math, sys
import numpy as np

rho, c, nu = 1.18, 349.5, 1.56e-5   # air in the flute: 30 C, humid, exhaled CO2 (src/bore.h)
CORK, EMB_LONG, EMB_RIN, EMB_CHIMNEY, HOLE_T = 0.0175, 11.6e-3, 6.4e-3, 4.3e-3, 1.9e-3
EMBR = 0.01   # PJ_EMBR: lip-slit loss in the window

# name: transpose, scale, bore, emb, foot (concert mm), dFoot, bFoot, head (mm)
INSTRUMENTS = {
    'flute':   (0, 1.0, 1.0, 1.0, 633.3, False, True, 0),
    'flute-c': (0, 1.0, 1.0, 1.0, 600.4, False, False, 0),
    'piccolo': (12, 0.5, 0.55, 0.7071, 528.0, True, False, -5),
    'alto':    (-5, 1.3348, 1.37, 1.1553, 600.4, False, False, 15),
    'bass':    (-12, 2.0, 1.84, 1.4142, 600.4, False, False, 30),
}

# x from the embouchure centre (mm), diameter, key opening, cup diameter (mm), French
SPEC = [(200.7, 7.4, 1.6, 13, 0), (218.0, 7.8, 1.6, 13, 0), (234.4, 7.0, 1.9, 13, 0),
        (266.9, 13.2, 2.6, 19, 0), (286.1, 13.2, 2.2, 19, 0), (307.1, 13.2, 2.5, 19, 1),
        (330.3, 13.2, 2.4, 19, 1), (352.0, 13.4, 2.4, 19, 0), (352.5, 13.4, 2.5, 19, 0),
        (376.9, 14.2, 2.1, 19, 0), (402.6, 14.2, 2.3, 19, 1), (429.9, 14.2, 2.5, 19, 1),
        (458.7, 14.2, 2.4, 19, 1), (490.3, 15.4, 2.8, 20.6, 0), (524.3, 15.4, 2.8, 20.6, 0),
        (557.3, 15.4, 2.8, 20.6, 0), (592.3, 15.4, 2.8, 20.6, 0)]

# Written fingerings (src/player.cc), MIDI 59..96.
FINGERINGS = {
    59: "xxxxxxxxxxxxxxxxx", 60: "xxxxxxxxxxxxxxxx", 61: "xxxxxxxxxxxxxxxo", 62: "xxxxxxxxxxxxxxoo",
    63: "xxxxxxxxxxxxxooo", 64: "xxxxxxxxxxxxoooo", 65: "xxxxxxxxxxxooooo", 66: "xxxxxxxxxxooxooo",
    67: "xxxxxxxxxooooooo", 68: "xxxxxxxoxooooooo", 69: "xxxxxxoxoooooooo", 70: "xxxxxooxoooooooo",
    71: "xxxxoooxoooooooo", 72: "xxxooooxoooooooo", 73: "xxoooooxoooooooo"}


def flute_bore(e):
    """Concert flute bore diameter (mm) at e mm from the embouchure centre."""
    if e < 0: return 16.8 + (17.07 - 16.8) * (e + 17.5) / 17.5
    if e < 113.5: return 17.07 + (18.8 - 17.07) * e / 113.5
    if e < 150.5: return 18.8
    if e < 154.5: return 19.8
    return 19.0


def tin(b, d):
    """Inner end correction of a side hole (Dalmont / Nederveen), d = b / a."""
    return b * (0.82 - 0.193 * d - 1.09 * d * d + 1.27 * d ** 3 - 0.71 * d ** 4)


def t_series(b, a, t, closed):
    """Dubos et al. 1999 (eq. 74): the hole's series length t_a (negative), in bore units."""
    d = b / a
    th = math.tanh(1.84 * t / b)
    f = 1 / th if closed else th
    return -b * d * d / (1.78 * f + 0.940 + 0.540 * d + 0.285 * d * d)


class Instrument:
    def __init__(self, name='flute-c'):
        tr, self.scale, self.bore, self.emb, foot, dfoot, bfoot, head = INSTRUMENTS[name]
        self.name, self.transpose = name, tr
        self.head = head * 1e-3
        self.cork = CORK * self.scale
        self.foot = foot * 1e-3 * self.scale - self.head      # embouchure centre to the open end
        self.holeT = HOLE_T * self.bore
        self.holes = []   # (x from the cork face, b, padH, padR, perforated, index)
        for i, (x, d, ph, cup, perf) in enumerate(SPEC):
            if (dfoot and i >= 14) or (i == 16 and not bfoot):
                continue
            self.holes.append((self.cork + x * 1e-3 * self.scale - self.head, 0.5e-3 * d * self.bore,
                               ph * 1e-3 * self.bore, 0.5e-3 * cup * self.bore, perf, i))

    def diam(self, x):
        return self.bore * 1e-3 * flute_bore((x - self.cork) / self.scale * 1e3)

    def hole_series(self, h, closed):
        x, b, *_ = h
        return t_series(b, self.diam(x) / 2, self.holeT, closed)

    def hole_Z(self, w, h):
        x, b, padH, R, perf, _ = h
        a = self.diam(x) / 2; S = np.pi * b * b; dl = b / a
        tm = b * dl * (1 + 0.207 * dl ** 3) / 8
        dpad = b * b * np.log(R / b) / (2 * padH)
        if perf:   # Dalmont 2001
            q, e = 3.4e-3, 3.6e-3
            de = 1.64 * b / q - 0.15 * b / R - 1.1 + e * b / (q * q)
            dpad /= 1 + 5 * de ** -1.35 * (padH / b) ** -0.2
        ts = self.holeT + tin(b, dl) + tm + dpad
        L_r = rho * 0.6133 * b / S; R_r = 1.5 * rho * c / S
        Zr = 1j * w * L_r * R_r / (R_r + 1j * w * L_r)
        return 1j * w * rho * ts / S + Zr

    def closed_Y(self, w, h):
        x, b, *_ = h
        a = self.diam(x) / 2; dl = b / a
        tm = b * dl * (1 + 0.207 * dl ** 3) / 8
        return 1j * w * np.pi * b * b * (self.holeT + tm - 0.05 * b) / (rho * c * c)

    def window_Z(self, w, unc=6e-3, extra=2e-3):
        el = EMB_LONG * self.emb; rin = EMB_RIN * self.emb
        sw = max(4e-6 * self.emb ** 2, unc * el); sbot = np.pi * rin * rin
        rw = np.sqrt(sw / np.pi); a = self.diam(self.cork) / 2
        m = rho * (EMB_CHIMNEY * self.emb / np.sqrt(sw * sbot) + tin(rin, rin / a) / sbot
                   + extra * self.emb / sw)
        L_r = rho * 0.82 * rw / sw; R_r = 1.35 * rho * c / sw
        return 1j * w * m + EMBR * rho * c / sw + 1j * w * L_r * R_r / (R_r + 1j * w * L_r)

    def Zin(self, w, pattern, unc=6e-3, extra=2e-3, series=True):
        """Impedance seen by the jet at the window. A fingering without the 17th
        character leaves the B foot's C hole open."""
        end = self.cork + self.foot
        a = self.diam(end - 1e-4) / 2; S = np.pi * a * a; ka = w / c * a
        Z = rho * c / S * (0.25 * ka * ka + 1j * 0.6133 * ka)
        hs = sorted(self.holes, key=lambda h: -h[0])
        x = end
        for h in hs + [None]:
            xh = h[0] if h else self.cork
            n = max(1, int(np.ceil((x - xh) / 4e-3)))
            for j in range(n):
                xm = x - (x - xh) * (j + 0.5) / n
                Z = tube_step(w, (x - xh) / n, self.diam(xm) / 2, Z)
            x = xh
            if h is None:
                break
            op = h[5] >= len(pattern) or pattern[h[5]] == 'o'
            Y = 1 / self.hole_Z(w, h) if op else self.closed_Y(w, h)
            Za = 1j * w * rho * self.hole_series(h, not op) / (np.pi * (self.diam(xh) / 2) ** 2) if series else 0
            Z = Za / 2 + 1 / (1 / (Z + Za / 2) + Y)
        # the cork cavity in parallel at the window
        Zcav = cavity_Z(w, self.cork, self.diam(self.cork / 2) / 2)
        return 1 / (1 / Z + 1 / Zcav) + self.window_Z(w, unc, extra)

    def peaks(self, pattern, fmin=None, fmax=None, **kw):
        """Admittance peaks (Hz), parabolic in log magnitude."""
        fmin = fmin or 80 / self.scale
        fmax = fmax or 2500 / self.scale
        f = np.arange(fmin, fmax, 0.25 / self.scale)
        Y = np.log(np.abs(1 / self.Zin(2 * np.pi * f, pattern, **kw)))
        i = np.where((Y[1:-1] > Y[:-2]) & (Y[1:-1] > Y[2:]))[0] + 1
        d = 0.5 * (Y[i - 1] - Y[i + 1]) / (Y[i - 1] - 2 * Y[i] + Y[i + 1])
        return f[i] + d * (f[1] - f[0])


def tube_step(w, L, a, Zload):
    """A short lossy cylinder (viscothermal, thermal lumped into the series term as
    in the waveguide: Z per length jw rho / S + kappa sqrt(jw))."""
    S = np.pi * a * a
    kap = 2.94 * rho * np.sqrt(nu) / (S * a)
    Zs = 1j * w * rho / S + kap * np.sqrt(1j * w); Ys = 1j * w * S / (rho * c * c)
    g = np.sqrt(Zs * Ys); Zc = np.sqrt(Zs / Ys)
    ch, sh = np.cosh(g * L), np.sinh(g * L)
    return (ch * Zload + Zc * sh) / (sh / Zc * Zload + ch)


def cavity_Z(w, L, a):
    S = np.pi * a * a
    kap = 2.94 * rho * np.sqrt(nu) / (S * a)
    Zs = 1j * w * rho / S + kap * np.sqrt(1j * w); Ys = 1j * w * S / (rho * c * c)
    g = np.sqrt(Zs * Ys); Zc = np.sqrt(Zs / Ys)
    return Zc / np.tanh(g * L)


def peaks(pattern, fmax=2500, extra=2e-3, inst='flute-c', **kw):
    """Back-compatible helper (C-foot flute)."""
    return Instrument(inst).peaks(pattern, fmax=fmax, extra=extra, **kw)


if __name__ == '__main__':
    args = sys.argv[1:]
    name, extra, notes = 'flute-c', 2.0, []
    while args:
        a = args.pop(0)
        if a == '-inst': name = args.pop(0)
        elif a == '-extra': extra = float(args.pop(0))
        else: notes.append(int(a))
    inst = Instrument(name)
    for m in notes or sorted(FINGERINGS):
        p = FINGERINGS[m]
        t = 440 * 2 ** ((m + inst.transpose - 69) / 12)
        pk = [v for v in inst.peaks(p, extra=extra * 1e-3) if v > 0.8 * t][:3]
        print(m, ' '.join(f'{v:.2f} ({1200 * math.log2(v / (t * (i + 1))):+.1f}c)' for i, v in enumerate(pk)))
