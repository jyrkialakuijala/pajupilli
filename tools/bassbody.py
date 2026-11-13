#!/usr/bin/env python3
"""The double bass's body as a lumped model (research/double_bass.md):

  - the top plate, a 16 x 8 grid of masses in the outline of a bass (spruce: stiff along the
    grain, soft across it), its edge held by the ribs; thicker in the middle than at the edge
    (graduation); the f-holes as cuts that soften the plate across the grain beside the bridge
    (the "island" between them, where the bridge stands);
  - the bass bar: extra mass and bending stiffness along one line of the top, under the bridge's
    bass foot, highest under the bridge and tapering to its ends;
  - the back plate, the same grid (maple), graduated too;
  - the sound post: a rigid rod with mass between the top and the back, standing a little
    behind the treble foot (towards the tail): its two ends move together, and it passes
    whatever force that takes (here a spring stiff enough to be rigid; the real-time body
    holds the two ends together exactly);
  - the air inside: a column of 16 cells along the body (its own standing waves), each cell
    compressed by the plates over it; the air plugs in the f-holes open from the bridge's cell.

    tools/bassbody.py            the resonances, and what moves in each
    tools/bassbody.py -fit       scale the top's stiffness (its arching) and the f-holes so
                                 that the air and wood resonances land on their targets
    tools/bassbody.py -emit      fit, and write src/bassbody_data.h for the real-time body
                                 (src/bassbody.h): per plate, the stiffness as 13 neighbour
                                 coefficients per mass, rows of 16 (one AVX-512 register each)
    tools/bassbody.py -v1 ...    the first version (uniform plates, an untapered bar, the post
                                 a plain spring under the foot, one air pressure): to compare
"""
import os
import sys
import numpy as np
from scipy.linalg import eigh
from scipy.optimize import least_squares

RHO_AIR, C_AIR = 1.18, 349.5
NX, NY = 16, 8
LEN, WID = 1.11, 0.68          # body length, widest width (m)
DEPTH = 0.20                   # the ribs' height (m)
DX, DY = LEN / NX, WID / NY
A0_TARGET = 66.0
RATIO = (5 * (1 + 5 ** 0.5) / 2 + 12) / (3 * (1 + 5 ** 0.5) / 2 + 7)  # 1.69478: a noble number
T1_TARGET = A0_TARGET * RATIO
OFFSETS = [(0, 0), (1, 0), (-1, 0), (2, 0), (-2, 0), (0, 1), (0, -1), (0, 2), (0, -2),
           (1, 1), (1, -1), (-1, 1), (-1, -1)]  # (along the row, across rows)


def half_width(u):
    """The outline: half the body's width (m) at u = 0 (neck end) .. 1 (tail end)."""
    pts = [(0.0, 0.10), (0.06, 0.20), (0.18, 0.25), (0.30, 0.245), (0.40, 0.19), (0.50, 0.185),
           (0.60, 0.20), (0.70, 0.30), (0.82, 0.34), (0.93, 0.31), (1.0, 0.16)]
    return np.interp(u, [p[0] for p in pts], [p[1] for p in pts])


def mask():
    m = np.zeros((NX, NY), bool)
    for i in range(NX):
        hw = half_width((i + 0.5) / NX)
        for j in range(NY):
            y = (j + 0.5) * DY - WID / 2
            m[i, j] = abs(y) < hw
    return m


def graduation(m, h_mid, h_edge):
    """Thickness per node: h_edge next to the ribs, h_mid two or more nodes in."""
    h = np.zeros((NX + 2, NY + 2))
    mm = np.zeros((NX + 4, NY + 4), bool)
    mm[2:-2, 2:-2] = m
    for i in range(NX):
        for j in range(NY):
            if not m[i, j]:
                continue
            d = 3
            for r in (1, 2):
                if not mm[i + 2 - r:i + 3 + r, j + 2 - r:j + 3 + r].all():
                    d = r
                    break
            h[i + 1, j + 1] = h_edge + (h_mid - h_edge) * (d - 1) / 2
    # (outside the outline: the nearest inside value, for the curvatures that reach over the edge)
    for _ in range(2):
        g = h.copy()
        for i in range(NX + 2):
            for j in range(NY + 2):
                if h[i, j] == 0:
                    nb = [h[a, b] for a in range(max(0, i - 1), min(NX + 2, i + 2))
                          for b in range(max(0, j - 1), min(NY + 2, j + 2)) if h[a, b] > 0]
                    if nb:
                        g[i, j] = np.mean(nb)
        h = g
    h[h == 0] = h_edge
    return h


def plate(m, e_l, e_r, g, h, rho, scale=1.0, bar=None, soft=None):
    """Stiffness and mass of an orthotropic plate on the masked grid (edge nodes outside the
    mask are held: the ribs). h: thickness, a number or a padded (NX+2, NY+2) array.
    bar: dict(row, e, rho, width, height: array over i) of a bar along x.
    soft: dict((i, j) -> factor) on the cross-grain and twist stiffness there (the f-holes)."""
    idx = -np.ones((NX + 2, NY + 2), int)
    idx[1:-1, 1:-1][m] = np.arange(m.sum())
    n = m.sum()
    if np.isscalar(h):
        h = np.full((NX + 2, NY + 2), h)
    soft = soft or {}
    K = np.zeros((n, n))
    M = np.zeros(n)
    for i in range(NX):
        for j in range(NY):
            if m[i, j]:
                M[idx[i + 1, j + 1]] = rho * h[i + 1, j + 1] * DX * DY
    area = DX * DY

    def add(stencil, coef):
        ids = [(idx[i, j], w) for i, j, w in stencil if idx[i, j] >= 0]
        for a, wa in ids:
            for b, wb in ids:
                K[a, b] += coef * area * wa * wb

    for i in range(1, NX + 1):
        for j in range(1, NY + 1):
            if idx[i, j] < 0 and not (idx[i - 1:i + 2, j - 1:j + 2] >= 0).any():
                continue
            h3 = h[i, j] ** 3 / 12
            dxx = scale * e_l * h3
            if bar is not None and j - 1 == bar['row'] and idx[i, j] >= 0 and bar['height'][i - 1] > 0:
                dxx += bar['e'] * bar['width'] * bar['height'][i - 1] ** 3 / 12 / DY
            dyy = scale * e_r * h3 * soft.get((i - 1, j - 1), 1.0)
            add([(i - 1, j, 1 / DX ** 2), (i, j, -2 / DX ** 2), (i + 1, j, 1 / DX ** 2)], dxx)
            add([(i, j - 1, 1 / DY ** 2), (i, j, -2 / DY ** 2), (i, j + 1, 1 / DY ** 2)], dyy)
    for i in range(0, NX + 1):  # twist, at the cell corners
        for j in range(0, NY + 1):
            c = 1 / (DX * DY)
            hh = np.mean([h[i, j], h[i + 1, j], h[i, j + 1], h[i + 1, j + 1]])
            f = min(soft.get((a, b), 1.0) for a in (i - 1, i) for b in (j - 1, j))
            add([(i, j, c), (i + 1, j, -c), (i, j + 1, -c), (i + 1, j + 1, c)],
                4 * scale * g * hh ** 3 / 12 * (0.5 + 0.5 * f))
    if bar is not None:
        for i in range(NX):
            if m[i, bar['row']] and bar['height'][i] > 0:
                M[idx[i + 1, bar['row'] + 1]] += bar['rho'] * bar['width'] * bar['height'][i] * DX
    return K, M, idx[1:-1, 1:-1]


DEFAULT = dict(h_top=0.0085, h_top_edge=0.0055, h_back=0.0080, h_back_edge=0.0055, arch_top=6.0,
               arch_back=15.0, bar_row=3, bridge_i=10, bass_j=3, treble_j=4, k_post=1e11,
               post_mass=0.026, post_behind=0.35, fhole_area=0.008, fhole_len=0.07, fhole_soft=0.15,
               v1=False)
V1 = dict(DEFAULT, k_post=1.7e7, arch_back=6.0, h_top=0.007, h_top_edge=0.007, h_back=0.007, h_back_edge=0.007, post_mass=0.0,
          post_behind=0.0, fhole_soft=1.0, v1=True, volume=0.125)


def parts(p):
    """The plates' matrices and the layout."""
    m = mask()
    if p['v1']:
        bar_h = np.full(NX, 0.040)
    else:  # 40 mm under the bridge, tapering to 12 mm, over the middle three quarters
        bar_h = np.array([0.012 + 0.028 * max(0.0, 1 - abs(i - p['bridge_i']) / 8) if 2 <= i <= 14 else 0.0
                          for i in range(NX)])
    bar = dict(row=p['bar_row'], e=11e9, rho=420, width=0.025, height=bar_h)
    soft = {}
    if p['fhole_soft'] < 1:  # the f-holes: beside the bridge, at the rows outside the feet
        for i in range(p['bridge_i'] - 2, p['bridge_i'] + 2):
            soft[(i, p['bass_j'] - 1)] = p['fhole_soft']
            soft[(i, p['treble_j'] + 1)] = p['fhole_soft']
    ht = graduation(m, p['h_top'], p['h_top_edge'])
    hb = graduation(m, p['h_back'], p['h_back_edge'])
    kt, mt, it = plate(m, 11e9, 0.8e9, 0.7e9, ht, 420, p['arch_top'], bar, soft)
    kb, mb, ib = plate(m, 10e9, 1.3e9, 1.0e9, hb, 620, p['arch_back'])
    return m, (kt, mt, it), (kb, mb, ib)


def air(m, p):
    """The air cells along the body: volumes, the passages between them (area)."""
    width = m.sum(1) * DY
    vol = width * DEPTH * DX
    s = 0.5 * (width[:-1] + width[1:]) * DEPTH   # between cells i and i+1
    return vol, s


def build(p):
    """K, M (diagonal) of the whole body, and the layout.
    Displacements: top (n), back (n), [air slugs (NX-1)], the f-hole plug; plates outward +."""
    m, (kt, mt, it), (kb, mb, ib) = parts(p)
    n = m.sum()
    nslug = 0 if p['v1'] else NX - 1
    N = 2 * n + nslug + 1
    K = np.zeros((N, N))
    K[:n, :n] = kt
    K[n:2 * n, n:2 * n] = kb
    M = np.zeros(N)
    M[:n] = mt
    M[n:2 * n] = mb
    M[-1] = RHO_AIR * p['fhole_area'] * p['fhole_len']
    # The sound post: between a point of the top and the point of the back under it; the point
    # is post_behind of a node behind the treble foot (towards the tail).
    bi, tj = p['bridge_i'], p['treble_j']
    c = np.zeros(N)
    for (i, w) in ((bi, 1 - p['post_behind']), (bi + 1, p['post_behind'])):
        if w > 0:
            c[it[i, tj]] = w          # top inward = back outward: both outward-positive
            c[n + ib[i, tj]] = w
            M[it[i, tj]] += 0.5 * p['post_mass'] * w
            M[n + ib[i, tj]] += 0.5 * p['post_mass'] * w
    K += p['k_post'] * np.outer(c, c)
    area = DX * DY
    vol_all = np.zeros(N)
    vol_all[:2 * n] = area
    vol_all[-1] = p['fhole_area']
    if p['v1']:  # one pressure
        K += RHO_AIR * C_AIR ** 2 / p['volume'] * np.outer(vol_all, vol_all)
    else:
        vol, s = air(m, p)
        M[2 * n:2 * n + nslug] = RHO_AIR * s * DX
        for i in range(NX):  # cell i: the plates over it out, the slugs out of it, the plug
            a = np.zeros(N)
            for j in range(NY):
                if m[i, j]:
                    a[it[i, j]] = area
                    a[n + ib[i, j]] = area
            if i < NX - 1:
                a[2 * n + i] = s[i]       # slug i moves towards the tail: out of cell i
            if i > 0:
                a[2 * n + i - 1] = -s[i - 1]
            if i == bi:
                a[-1] = p['fhole_area']
            K += RHO_AIR * C_AIR ** 2 / vol[i] * np.outer(a, a)
    lay = dict(n=n, top=it, back=ib, bass_foot=it[bi, p['bass_j']], treble_foot=it[bi, tj], area=vol_all,
               nslug=nslug)
    return K, M, lay


def modes(p):
    K, M, lay = build(p)
    w2, v = eigh(K, np.diag(M))
    f = np.sqrt(np.maximum(w2, 0)) / (2 * np.pi)
    return f, v, M, lay


def describe(p, count=18):
    f, v, M, lay = modes(p)
    n = lay['n']
    print(f"grid {NX} x {NY}: {n} masses per plate; node {1e3 * DX:.0f} x {1e3 * DY:.0f} mm; "
          f"top {1e3 * M[:n].sum():.0f} g with the bar, back {1e3 * M[n:2 * n].sum():.0f} g")
    print(' mode   Hz    energy share: top  back  air    volume pushed out (top, back, f-holes)   bass foot  treble foot')
    for k in range(count):
        e = M * v[:, k] ** 2
        e /= e.sum()
        vol = lay['area'] * v[:, k]
        vt, vb, va = vol[:n].sum(), vol[n:2 * n].sum(), vol[-1]
        s = max(abs(vt), abs(vb), abs(va))
        print(f'{k + 1:4d} {f[k]:7.1f}          {e[:n].sum():5.2f} {e[n:2 * n].sum():5.2f} {e[2 * n:].sum():5.2f}'
              f'      {vt / s:+5.2f} {vb / s:+5.2f} {va / s:+5.2f}'
              f'                  {v[lay["bass_foot"], k]:+7.2f}   {v[lay["treble_foot"], k]:+7.2f}')
    return f


def fit(p):
    """The plates' arching stiffness (top and back together, in their set proportion) and the
    f-holes' size, so the two lowest resonances are A0 and T1 at their targets."""
    ratio = p['arch_back'] / p['arch_top']

    def err(x):
        q = dict(p, arch_top=np.exp(x[0]), arch_back=ratio * np.exp(x[0]), fhole_area=p['fhole_area'] * np.exp(x[1]))
        f = modes(q)[0]
        return [np.log(f[0] / A0_TARGET), np.log(f[1] / T1_TARGET)]
    r = least_squares(err, [np.log(p['arch_top']), 0.0])
    return dict(p, arch_top=float(np.exp(r.x[0])), arch_back=float(ratio * np.exp(r.x[0])),
                fhole_area=float(p['fhole_area'] * np.exp(r.x[1])))


def emit(p, path):
    m, (kt, mt, it), (kb, mb, ib) = parts(p)

    def grids(K, M, idx):
        c = np.zeros((len(OFFSETS), NY, NX))
        inv = np.zeros((NY, NX))
        used = np.zeros_like(K, bool)
        for i in range(NX):
            for j in range(NY):
                a = idx[i, j]
                if a < 0:
                    continue
                inv[j, i] = 1 / M[a]
                for o, (di, dj) in enumerate(OFFSETS):
                    i2, j2 = i + di, j + dj
                    if 0 <= i2 < NX and 0 <= j2 < NY and idx[i2, j2] >= 0:
                        c[o, j, i] = K[a, idx[i2, j2]]
                        used[a, idx[i2, j2]] = True
        assert np.abs(K[~used]).max() < 1e-6 * np.abs(K).max(), 'stiffness outside the 13 neighbours'
        return c, inv

    def lit(x):  # a float literal (always with a point or an exponent)
        t = f'{x:.9g}'
        return (t if ('.' in t or 'e' in t) else t + '.0') + 'f'

    def arr(a):
        return '{' + ', '.join(lit(x) for x in a) + '}'

    # The post's mass is in the plates' masses.
    bi, tj = p['bridge_i'], p['treble_j']
    for (i, w) in ((bi, 1 - p['post_behind']), (bi + 1, p['post_behind'])):
        if w > 0:
            mt[it[i, tj]] += 0.5 * p['post_mass'] * w
            mb[ib[i, tj]] += 0.5 * p['post_mass'] * w
    vol, s = air(m, p)
    out = ['// Generated by tools/bassbody.py -emit: the double bass body (research/double_bass.md).',
           '// Rows of 16 along the body (neck to tail), 8 rows across (bass side first).',
           '#pragma once', 'namespace pajupilli {', 'namespace bassdata {',
           f'constexpr int kNX = {NX}, kNY = {NY}, kOffsets = {len(OFFSETS)};',
           'constexpr int kOffI[kOffsets] = ' + '{' + ', '.join(str(o[0]) for o in OFFSETS) + '};',
           'constexpr int kOffJ[kOffsets] = ' + '{' + ', '.join(str(o[1]) for o in OFFSETS) + '};']
    for name, (K, M, idx) in (('Top', (kt, mt, it)), ('Back', (kb, mb, ib))):
        c, inv = grids(K, M, idx)
        out.append(f'constexpr float k{name}K[kOffsets][kNY][kNX] = {{')
        for o in range(len(OFFSETS)):
            out.append('  {' + ', '.join(arr(c[o, j]) for j in range(NY)) + '},')
        out.append('};')
        out.append(f'constexpr float k{name}InvM[kNY][kNX] = {{' + ', '.join(arr(inv[j]) for j in range(NY)) + '};')
    out += [f'constexpr float kNodeArea = {lit(DX * DY)};',
            '// Where the masses are: each row\'s place across the body (m, bass side negative), each',
            '// column\'s along it (m, from the middle; the tail positive).',
            'constexpr float kRowY[kNY] = ' + arr([(j + 0.5) * DY - WID / 2 for j in range(NY)]) + ';',
            'constexpr float kColX[kNX] = ' + arr([(i + 0.5) * DX - LEN / 2 for i in range(NX)]) + ';',
            '// The air: 16 cells along the body. Pressure per volume (rho c^2 / V), the passages',
            '// between cells (area, m^2; the last: none), the slugs\' 1 / (rho dx).',
            'constexpr float kAirK[kNX] = ' + arr(RHO_AIR * C_AIR ** 2 / vol) + ';',
            'constexpr float kSlugArea[kNX] = ' + arr(list(s) + [0.0]) + ';',
            f'constexpr float kSlugInv = {lit(1 / (RHO_AIR * DX))};',
            f'constexpr float kPlugArea = {lit(p["fhole_area"])}, kPlugInvM = {lit(1 / (RHO_AIR * p["fhole_area"] * p["fhole_len"]))};',
            f'constexpr float kPostBehind = {lit(p["post_behind"])};  // the rigid post: this share of a node behind the foot',
            f'constexpr int kBridgeI = {bi}, kBassJ = {p["bass_j"]}, kTrebleJ = {tj};',
            f'constexpr float kA0 = {lit(A0_TARGET)}, kT1 = {lit(T1_TARGET)};',
            '}  // namespace bassdata', '}  // namespace pajupilli', '']
    open(path, 'w').write('\n'.join(out))
    print('wrote', path)


if __name__ == '__main__':
    p = dict(V1 if '-v1' in sys.argv else DEFAULT)
    if '-fit' in sys.argv or '-emit' in sys.argv:
        p = fit(p)
        print(f"fitted: arching stiffness top x{p['arch_top']:.2f}, back x{p['arch_back']:.2f}, f-holes {1e4 * p['fhole_area']:.1f} cm2 "
              f"(targets {A0_TARGET:.2f} and {T1_TARGET:.2f} Hz, ratio {RATIO:.5f})")
    f = describe(p)
    print(f'lowest two: {f[0]:.2f} and {f[1]:.2f} Hz, ratio {f[1] / f[0]:.4f}')
    if '-emit' in sys.argv:
        assert not p['v1']
        emit(p, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'src', 'bassbody_data.h'))
