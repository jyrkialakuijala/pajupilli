#!/usr/bin/env python3
"""Transfer-matrix (frequency-domain) model of the same flute, to check the
time-domain waveguide: bore from the cork, tone holes as shunt impedances,
embouchure window in series, foot with the unflanged radiation impedance.
Prints the resonances (admittance peaks seen from the window) per fingering."""
import sys, numpy as np
rho, c, nu = 1.18, 347.0, 1.56e-5
cork, foot = 0.0175, 0.6004
def diam(x):
    e = (x - cork) * 1e3
    if e < 0: d = 16.8 + 0.27 * (e + 17.5) / 17.5
    elif e < 113.5: d = 17.07 + 1.73 * e / 113.5
    elif e < 150.5: d = 18.8
    elif e < 154.5: d = 19.8
    else: d = 19.0
    return d * 1e-3
holes = [(200.7,7.4,1.6,13,0),(218.0,7.8,1.6,13,0),(234.4,7.0,1.9,13,0),(266.9,13.2,2.6,19,0),
 (286.1,13.2,2.2,19,0),(307.1,13.2,2.5,19,1),(330.3,13.2,2.4,19,1),(352.0,13.4,2.4,19,0),
 (352.5,13.4,2.5,19,0),(376.9,14.2,2.1,19,0),(402.6,14.2,2.3,19,1),(429.9,14.2,2.5,19,1),
 (458.7,14.2,2.4,19,1),(490.3,15.4,2.8,20.6,0),(524.3,15.4,2.8,20.6,0),(557.3,15.4,2.8,20.6,0)]
def tube(w, L, a, lossy=True):
    S = np.pi * a * a; k = w / c
    if lossy:  # viscothermal, same 1.47 lumping as the FDTD: Z per length = jw rho/S + kappa sqrt(jw)
        kap = 2.94 * rho * np.sqrt(nu) / (S * a)
        Zs = 1j * w * rho / S + kap * np.sqrt(1j * w); Ys = 1j * w * S / (rho * c * c)
        g = np.sqrt(Zs * Ys); Zc = np.sqrt(Zs / Ys)
    else:
        g = 1j * k; Zc = rho * c / S
    ch, sh = np.cosh(g * L), np.sinh(g * L)
    return np.array([[ch, Zc * sh], [sh / Zc, ch]])
def tin(b, d): return b * (0.82 - 0.193*d - 1.09*d*d + 1.27*d**3 - 0.71*d**4)
def hole_Z(w, h, x):
    xe, dd, padH, cup, perf = h
    b = dd * 0.5e-3; a = diam(x) / 2; S = np.pi * b * b; dl = b / a
    tm = b * dl * (1 + 0.207 * dl**3) / 8
    hp = padH * 1e-3; R = cup * 0.5e-3
    dpad = b * b * np.log(R / b) / (2 * hp)
    if perf:
        q, e = 3.4e-3, 3.6e-3
        de = 1.64*b/q - 0.15*b/R - 1.1 + e*b/(q*q)
        dpad /= 1 + 5 * de**-1.35 * (hp / b)**-0.2
    ts = 1.9e-3 + tin(b, dl) + tm + dpad
    L_r = rho * 0.6133 * b / S; R_r = 1.5 * rho * c / S
    Zr = 1j * w * L_r * R_r / (R_r + 1j * w * L_r)
    return 1j * w * rho * ts / S + Zr
def closed_Y(w, h, x):
    b = h[1] * 0.5e-3; return 1j * w * np.pi * b * b * 1.9e-3 / (rho * c * c)
def window_Z(w, unc=6e-3, extra=0.0):
    sw = unc * 11.6e-3; sbot = np.pi * 6.4e-3**2; rw = np.sqrt(sw / np.pi); a = diam(cork) / 2
    m = rho * (4.3e-3 / np.sqrt(sw * sbot) + tin(6.4e-3, 6.4e-3 / a) / sbot + extra / sw)
    L_r = rho * 0.82 * rw / sw; R_r = 1.35 * rho * c / sw
    return 1j * w * m + 1j * w * L_r * R_r / (R_r + 1j * w * L_r)
def Zin(w, pattern, unc=6e-3, extra=0.0, lossy=True):
    # from the foot upward to the window junction
    a = diam(cork + foot - 1e-3) / 2; S = np.pi * a * a; ka = w / c * a
    Z = rho * c / S * (0.25 * ka * ka + 1j * 0.6133 * ka)
    pos = sorted([(cork + h[0] * 1e-3, h, pattern[i] == 'o') for i, h in enumerate(holes)], key=lambda t: -t[0])
    x = cork + foot
    for (xh, h, op) in pos + [(cork, None, None)]:
        n = max(1, int((x - xh) / 5e-3))
        for j in range(n):  # short cylindrical pieces
            x1 = x - (x - xh) * (j + 1) / n; xm = x - (x - xh) * (j + 0.5) / n
            T = tube(w, (x - xh) / n, diam(xm) / 2, lossy)
            Z = (T[0,0] * Z + T[0,1]) / (T[1,0] * Z + T[1,1])
        x = xh
        if h is not None:
            Y = 1 / hole_Z(w, h, xh) if op else closed_Y(w, h, xh)
            Z = 1 / (1 / Z + Y)
    # the cork cavity in parallel at the window
    T = tube(w, cork, diam(cork / 2) / 2, lossy); Zcav = T[0,0] / T[1,0]
    Zb = 1 / (1 / Z + 1 / Zcav)
    return Zb + window_Z(w, unc, extra)
def peaks(pattern, fmax=2500, **kw):
    f = np.arange(100, fmax, 0.5); Y = np.abs(1 / Zin(2 * np.pi * f, pattern, **kw))
    i = np.where((Y[1:-1] > Y[:-2]) & (Y[1:-1] > Y[2:]))[0] + 1
    return f[i]
if __name__ == '__main__':
    import math
    pats = {60:"xxxxxxxxxxxxxxxx",62:"xxxxxxxxxxxxxxoo",64:"xxxxxxxxxxxxoooo",65:"xxxxxxxxxxxooooo",
            67:"xxxxxxxxxooooooo",69:"xxxxxxoxoooooooo",71:"xxxxoooxoooooooo",72:"xxxooooxoooooooo",73:"xxoooooxoooooooo"}
    extra = float(sys.argv[1]) * 1e-3 if len(sys.argv) > 1 else 0.0
    for m, p in pats.items():
        t = 440 * 2 ** ((m - 69) / 12); pk = [v for v in peaks(p, extra=extra) if v > 0.8 * t][:3]
        print(m, ' '.join(f'{1200*math.log2(v/(t*(i+1))):+4.0f}c' for i, v in enumerate(pk)))
