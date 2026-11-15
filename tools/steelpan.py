#!/usr/bin/env python3
"""A tenor steel pan made the way a pan maker makes one, then its modes.

An oil drum's head (steel, 1 mm, 570 mm) is sunk into a bowl (200 mm deep:
the steel stretches, thinner toward the middle), the note areas are marked
out (ellipses in three rings, the circle of fifths: Mayers' 1990 tenor, after
Kronman), flattened and backed into shallow domes, grooved round their edges,
and tuned: the tuner hammers each note (its two curvatures, the stress left
in it) until its first three modes ring 1 : 2 : 3 (the outer ring) or
1 : 2 : 4 (the middle ring; the inner ring 1 : 2, its third free).

Each note's vibration is a shallow shell's (Marguerre): bending (D grad^4 w)
and the membrane stiffness its curvature gives it (the stress function F:
grad^4 F / (E h) = -L(z, w), L the Monge-Ampere bracket), on a finite
difference grid over the note and a margin of the bowl around it (the bowl's
steeper curvature keeps the note's modes in the note: mode confinement,
Maloney 2010). The modes come out with their shapes: the stick's place on
each (the excitation), the volume each moves (the radiation), and the
quadratic coupling the shell's curvature gives them (Touze's coefficients,
the octave's bloom).

  python3 tools/steelpan.py            # tune all, write src/steelpan_data.h
  python3 tools/steelpan.py -note 62   # one note, verbose
"""
import argparse
import math
import sys

import numpy as np
import scipy.sparse as sp
import scipy.sparse.linalg as sla
from scipy.optimize import least_squares

E, NU, RHO = 200e9, 0.3, 7850.0
R_PAN, DEPTH, H_RIM = 0.285, 0.20, 1.0e-3
GROOVE = 0.008  # the edge's width (m): the steel turns from the bowl into the note


def hz(m):
    return 440 * 2 ** ((m - 69) / 12)


# --- The bowl ---------------------------------------------------------------
# A paraboloid z = DEPTH (r / R)^2, its surface positions by arc length s.
_r = np.linspace(0, R_PAN, 4001)
_s = np.concatenate([[0], np.cumsum(np.sqrt(1 + (2 * DEPTH * _r[1:] / R_PAN ** 2) ** 2) * np.diff(_r))])
S_RIM = _s[-1]


def r_of_s(s):
    return float(np.interp(s, _s, _r))


def bowl_curvatures(r):
    """The bowl's principal curvatures at radius r: along the radius
    (meridian) and around (hoop)."""
    zp = 2 * DEPTH * r / R_PAN ** 2
    zpp = 2 * DEPTH / R_PAN ** 2
    km = zpp / (1 + zp * zp) ** 1.5
    kh = zpp / math.sqrt(1 + zp * zp) if r < 1e-6 else zp / (r * math.sqrt(1 + zp * zp))
    return km, kh


def thickness(r):
    """Sinking stretches the steel: thinner toward the middle (~1 mm at the
    rim, ~0.55 at the centre; Monteil: ~0.5 mm at a note's centre)."""
    return H_RIM * (1 - 0.45 * (1 - (r / R_PAN) ** 2))


# --- The notes --------------------------------------------------------------
# A low-C tenor: C4..B4 outer, C5..B5 middle, C6..F#6 inner, each ring in the
# circle of fifths, the octaves one inside another. Sizes (radial x
# tangential, mm): Mayers' tenor (Kronman), fitted per ring as a power of the
# pitch; C4, C#4 enlarged.
FIFTHS = [0, 7, 2, 9, 4, 11, 6, 1, 8, 3, 10, 5]  # C G D A E B F# C# G# D# A# F


def note_layout():
    notes = {}
    for ring, (lo, hi, n) in enumerate([(60, 71, 12), (72, 83, 12), (84, 90, 7)]):
        for m in range(lo, hi + 1):
            f = hz(m)
            if ring == 0:
                L, W = 150 * (hz(62) / f) ** 0.35, 155 * (hz(62) / f) ** 0.95
                if m < 62:
                    L, W = L * 1.05, W * 1.0
            elif ring == 1:
                L, W = 100 * (hz(74) / f) ** 0.85, 70 * (hz(74) / f) ** 0.5
            else:
                L, W = 59 * (hz(85) / f) ** 0.2, 48 * (hz(85) / f) ** 0.35
            pc = m % 12
            k = FIFTHS.index(pc)
            n_ring = 12 if ring < 2 else 7
            ang = 2 * math.pi * (k if ring < 2 else (FIFTHS.index(pc) % 12)) / n_ring
            notes[m] = dict(ring=ring, L=L / 1000, W=W / 1000, ang=ang)
    # Radial positions (arc length from the centre): the outer ring by the
    # rim, the middle inside it, the inner inside that.
    for m, d in notes.items():
        if d['ring'] == 0:
            d['s'] = S_RIM - 0.02 - d['L'] / 2
        elif d['ring'] == 1:
            d['s'] = S_RIM - 0.02 - 0.17 - 0.008 - d['L'] / 2
        else:
            d['s'] = 0.05
        d['r'] = r_of_s(d['s'])
    return notes


# --- The shell on a grid ----------------------------------------------------
def d1(n, h):
    return sp.diags([-1, 1], [-1, 1], shape=(n, n)) / (2 * h)


def d2(n, h):
    return sp.diags([1, -2, 1], [-1, 0, 1], shape=(n, n)) / (h * h)


class Patch:
    """A note and its margin of bowl. x along the pan's radius, y around it."""

    def __init__(self, note, nodes=100, margin=0.025, stretch_max=1.35):
        self.note = note
        self.a0, self.b0 = note['L'] / 2, note['W'] / 2
        # (One grid for every shape the tuner tries: room for the note
        # stretched either way.)
        s = math.sqrt(stretch_max)
        X, Y = self.a0 * s + margin, self.b0 * s + margin
        h = 2 * max(X, Y) / nodes
        nx, ny = int(2 * X / h) + 1, int(2 * Y / h) + 1
        self.h, self.nx, self.ny = h, nx, ny
        x = (np.arange(nx) - (nx - 1) / 2) * h
        y = (np.arange(ny) - (ny - 1) / 2) * h
        self.x, self.y = np.meshgrid(x, y, indexing='ij')
        self.km, self.kh = bowl_curvatures(note['r'])
        self.r = note['r'] + self.x  # (the radius at each point: near enough)
        Ix, Iy = sp.identity(nx), sp.identity(ny)
        self.Dx = sp.kron(d1(nx, h), Iy).tocsr()
        self.Dy = sp.kron(Ix, d1(ny, h)).tocsr()
        self.Dxx = sp.kron(d2(nx, h), Iy).tocsr()
        self.Dyy = sp.kron(Ix, d2(ny, h)).tocsr()
        self.Dxy = (self.Dx @ self.Dy).tocsr()
        # Rows where the stencils fit; w clamped two nodes in from the edge
        # (the bowl beyond is far stiffer), F free (the edge immovable).
        ii, jj = np.meshgrid(np.arange(nx), np.arange(ny), indexing='ij')
        inner1 = ((ii >= 1) & (ii < nx - 1) & (jj >= 1) & (jj < ny - 1)).ravel()
        inner2 = ((ii >= 2) & (ii < nx - 2) & (jj >= 2) & (jj < ny - 2)).ravel()
        self.rows = sp.diags(inner1.astype(float))
        self.wfree = np.flatnonzero(inner2)
        self.ffree = np.flatnonzero(inner1)
        self.dA = h * h
        self.set_shape(1.0)

    def set_shape(self, stretch):
        """The note's ellipse, its area kept, stretched along the radius by
        stretch: the area marked, the steel thinned along its edge."""
        a, b = self.a0 * math.sqrt(stretch), self.b0 / math.sqrt(stretch)
        self.a, self.b = a, b
        self.rho = np.sqrt((self.x / a) ** 2 + (self.y / b) ** 2)
        # The note's area: 1 inside, 0 outside, over the edge's bend
        # (GROOVE wide: the steel turns from the bowl into the note).
        u = (self.rho - 1) * min(a, b) / (0.5 * GROOVE)
        self.m = 0.5 * (1 - np.tanh(u))
        hh = thickness(np.abs(self.r)) * (1 - 0.15 * np.exp(-u * u))  # the groove: a punched line, thinner
        self.hh = hh
        hh = hh.ravel()
        self.Dw = E * hh ** 3 / (12 * (1 - NU * NU))
        self.C = 1 / (E * hh)
        self.mass = RHO * hh * self.dA

    def surface(self, kx, ky):
        """The bowl, the note flattened into it and backed into a dome of
        curvatures kx, ky (1/m; positive: convex toward the player, against
        the bowl's): z and its second derivatives (analytic in the note and
        the bowl, blended over the groove)."""
        zb = 0.5 * (self.km * self.x ** 2 + self.kh * self.y ** 2)
        zn = -0.5 * (kx * self.x ** 2 + ky * self.y ** 2)
        zb0 = 0.5 * (self.km * self.a ** 2 + self.kh * self.b ** 2) / 2  # the bowl's mean height on the note's edge
        zn0 = -0.5 * (kx * self.a ** 2 + ky * self.b ** 2) / 2
        z = (1 - self.m) * zb + self.m * (zb0 + zn - zn0)
        return z.ravel()

    def operators(self, kx, ky, n0, n1=None):
        z = self.surface(kx, ky)
        zxx, zyy, zxy = self.Dxx @ z, self.Dyy @ z, self.Dxy @ z
        R = self.rows
        dA = self.dA

        def form(Wt, a1, a2, s):
            return (a1.T @ R @ sp.diags(Wt) @ a2) * (dA * s)

        Lap = self.Dxx + self.Dyy
        Dw = self.Dw
        Kb = form(Dw, Lap, Lap, 1) - (1 - NU) * (form(Dw, self.Dxx, self.Dyy, 1) + form(Dw, self.Dyy, self.Dxx, 1)
                                                - 2 * form(Dw, self.Dxy, self.Dxy, 1))
        # The tuner's stress (N/m, negative: compression) in the note: n0
        # along the pan's radius, n1 around it (his hammer's direction).
        n1 = n0 if n1 is None else n1
        Kn = form(n0 * self.m.ravel(), self.Dx, self.Dx, 1) + form(n1 * self.m.ravel(), self.Dy, self.Dy, 1)
        C = self.C
        A = form(C, Lap, Lap, 1) - (1 + NU) * (form(C, self.Dxx, self.Dyy, 1) + form(C, self.Dyy, self.Dxx, 1)
                                               - 2 * form(C, self.Dxy, self.Dxy, 1))
        Lz = (sp.diags(zxx) @ self.Dyy + sp.diags(zyy) @ self.Dxx - 2 * sp.diags(zxy) @ self.Dxy)
        B = (R @ Lz) * dA
        wf, ff = self.wfree, self.ffree
        Kb = (Kb + Kn)[wf][:, wf]
        A = A[ff][:, ff]
        A = A + sp.identity(A.shape[0]) * (1e-9 * A.diagonal().mean())
        B = B[ff][:, wf]
        M = sp.diags(self.mass[wf])
        return Kb.tocsc(), A.tocsc(), B.tocsc(), M.tocsc(), z

    def modes(self, kx, ky, n0, n1=None, k=8, sigma=None):
        Kb, A, B, M, z = self.operators(kx, ky, n0, n1)
        nw = Kb.shape[0]
        if sigma is None:
            sigma = (2 * math.pi * 0.5 * self.note.get('f', 300)) ** 2
        S = sp.bmat([[Kb - sigma * M, B.T], [B, -A]]).tocsc()
        lu = sla.splu(S)
        zero = np.zeros(A.shape[0])

        def opinv(v):
            return lu.solve(np.concatenate([v, zero]))[:nw]

        Alu = sla.splu(A)
        Keff = sla.LinearOperator((nw, nw), matvec=lambda v: Kb @ v + B.T @ Alu.solve(B @ v))
        OP = sla.LinearOperator((nw, nw), matvec=opinv)
        vals, vecs = sla.eigsh(Keff, k=k, M=M, sigma=sigma, OPinv=OP, which='LM')
        order = np.argsort(vals)
        vals, vecs = vals[order], vecs[:, order]
        f = np.sqrt(np.maximum(vals, 0)) / (2 * math.pi)
        # Mass-normalised: phi^T M phi = 1.
        for i in range(vecs.shape[1]):
            vecs[:, i] /= math.sqrt(vecs[:, i] @ (M @ vecs[:, i]))
        self.last = (Kb, A, B, M, z, Alu)
        return f, vecs

    def full(self, v):
        out = np.zeros(self.nx * self.ny)
        out[self.wfree] = v
        return out


def tune(m, note, verbose=False):
    """The tuner: the note's shape (its ellipse stretched, the area kept),
    its dome (curvatures kx along the pan's radius, ky around it; positive:
    convex toward the player) and the stress his hammering leaves in it (n0
    along, n1 across; N/m) until its first modes ring 1 : 2 : 3 (outer),
    1 : 2 : 4 (middle) or 1 : 2 (inner). From a few shapes (a dome, a
    saddle either way), the best."""
    f1 = hz(m)
    note['f'] = f1
    target3 = {0: 3.0, 1: 4.0, 2: None}[note['ring']]
    p = Patch(note, nodes=72)
    sig = (2 * math.pi * 0.7 * f1) ** 2
    scale = p.km + p.kh

    def resid(q):
        ls, kx, ky, n0, n1 = q
        p.set_shape(math.exp(ls))
        f, _ = p.modes(kx, ky, n0, n1, k=4, sigma=sig)
        r = [3 * math.log(f[0] / f1), f[1] / f[0] - 2]
        r.append((f[2] / f[0] - target3) * 0.7 if target3 else 0.0)
        # (The least hammering that does it.)
        r += [0.02 * ls, 0.002 * kx / scale, 0.002 * ky / scale, 0.002 * n0 / 1e4, 0.002 * n1 / 1e4]
        if verbose:
            print(f'   stretch {math.exp(ls):6.3f} k {kx:7.3f} {ky:7.3f} n {n0:8.1f} {n1:8.1f}: '
                  + ' '.join(f'{v:7.1f}' for v in f), flush=True)
        return r

    best = None
    for q0, nfev in (([0, scale, -scale, 3e4, -5e3], 150), ([0, 0.3 * scale, 0.3 * scale, 1e3, 1e3], 60)):
        if best is not None and best.cost < 1e-4:
            break
        res = least_squares(resid, q0, x_scale=[0.1, scale, scale, 1e4, 1e4], diff_step=1e-3, max_nfev=nfev,
                            bounds=([math.log(1 / 1.35), -20 * scale, -20 * scale, -2e5, -2e5],
                                    [math.log(1.35), 20 * scale, 20 * scale, 2e5, 2e5]))
        if verbose:
            print(' start', q0, '->', list(res.x), 'cost', res.cost, flush=True)
        if best is None or res.cost < best.cost:
            best = res
    q = list(best.x)
    p.set_shape(math.exp(q[0]))
    f, v = p.modes(*q[1:], k=10, sigma=sig)
    return p, q, f, v


# --- What the program needs of each note -----------------------------------
Q_MODES = [110, 250, 330] + [200] * 7  # Monteil (G3: 110, 250), Maloney's soprano pan (100-300)
N_OUT = 10


def bracket(P, a, b):
    """L(a, b) = a_xx b_yy + a_yy b_xx - 2 a_xy b_xy on the grid."""
    return (P.Dxx @ a) * (P.Dyy @ b) + (P.Dyy @ a) * (P.Dxx @ b) - 2 * (P.Dxy @ a) * (P.Dxy @ b)


def note_data(m):
    """Tune note m, then its modes for the program: frequencies (x the
    fundamental), decays, the stick's place on each (mass-normalised: per
    sqrt(kg)), the radiation, and the shell's quadratic coupling of the
    first three (the resonant terms of 1 : 2 : 3 or 1 : 2 : 4)."""
    note = dict(note_layout()[m])
    P, q, f, V = tune(m, note)
    f1 = hz(m)
    Kb, A, B, M, z, Alu = P.last
    phis = [P.full(V[:, k]) for k in range(N_OUT)]
    rows = np.asarray(P.rows.diagonal())
    dA = P.dA
    # The stick: a third of the way out from the centre (toward the rim and
    # aside: every one of the first modes moves there).
    xs, ys = 0.3 * P.a, 0.2 * P.b
    i0 = int(np.argmin((P.x.ravel() - xs) ** 2 + (P.y.ravel() - ys) ** 2))
    s = f1 / f[0]  # (tuned on its grid to within a hair: exactly in tune)
    out = dict(m=m, ring=note['ring'], ang=note['ang'], r=note['r'], q=q, raw=list(f[:N_OUT]))
    out['ratio'] = [float(f[k] / f[0]) for k in range(N_OUT)]
    out['t60'] = [float(6.9078 * Q_MODES[k] / (math.pi * f[k] * s)) for k in range(N_OUT)]
    out['exc'] = [float(ph[i0]) for ph in phis]
    rad = []
    for k, ph in enumerate(phis):
        fk = f[k] * s
        vol = float(np.sum(ph) * dA)
        dx, dy = float(np.sum(ph * P.x.ravel()) * dA), float(np.sum(ph * P.y.ravel()) * dA)
        kk = 2 * math.pi * fk / 343
        mag = math.sqrt(vol * vol + (kk * kk) * (dx * dx + dy * dy) / 3)
        # Pressure ~ d/dt of the volume (x f), the unbaffled pan's front and
        # back cancelling below ~277 Hz (Kronman: its 80 cm path).
        rad.append(mag * (fk / f1) * fk / math.sqrt(fk * fk + 277.0 ** 2) * (1 if vol >= 0 else -1))
    out['rad'] = rad
    # Quadratic coupling: Q_p = sum_ij q_i q_j G[p][i][j], with
    # G = int F1_j L(phi_p, phi_i) + int F2_ij L(z, phi_p) (A F1 = -B phi,
    # A F2 = -dA R L(phi_i, phi_j) / 2).
    ff = P.ffree
    F1 = []
    for j in range(3):
        Fj = np.zeros(P.nx * P.ny)
        Fj[ff] = -Alu.solve(B @ V[:, j])
        F1.append(Fj)
    Bphi = [B @ V[:, p] for p in range(3)]
    G = np.zeros((3, 3, 3))
    for i in range(3):
        for j in range(3):
            F2 = -Alu.solve((dA * rows * 0.5 * bracket(P, phis[i], phis[j]))[ff])
            for p in range(3):
                g1 = float(np.sum(rows * F1[j] * bracket(P, phis[p], phis[i])) * dA)
                g2 = float(Bphi[p] @ F2)
                G[p, i, j] = g1 + g2
    # The resonant terms (the engine: x_p'' += -g x_i x_j), scaled with the
    # frequencies (s^2: the stiffness).
    if note['ring'] == 0:
        terms = [(0, 0, 1), (0, 1, 2), (1, 0, 0), (1, 0, 2), (2, 0, 1)]
    else:
        terms = [(0, 0, 1), (1, 0, 0), (1, 1, 2), (2, 1, 1)]
    out['quad'] = []
    for p_, i, j in terms:
        g = G[p_, i, i] if i == j else G[p_, i, j] + G[p_, j, i]
        out['quad'].append((p_, i, j, float(-g * s * s)))
    print(f"note {m}: q {[round(v, 3) for v in q]}, ratios " + ' '.join(f'{r:.3f}' for r in out['ratio'][:6]), flush=True)
    return out


def write_header(data, path):
    L = ['// Generated by tools/steelpan.py: a tenor pan made, its notes tuned, their',
         '// modes (C4..F#6). Per note: the fundamental (Hz), its ring, its place',
         '// (angle, radius m), then per mode: frequency x the fundamental, t60 (s),',
         '// the shape where the stick strikes (1/sqrt(kg)), the radiation; the',
         '// quadratic coupling terms (x_p\'\' += -g x_i x_j; 1/(s^2 m sqrt(kg))).',
         '#pragma once', '', 'namespace pajupilli {', '',
         'struct PanNote {', '  float f1;', '  int ring;', '  float ang, r;',
         f'  float ratio[{N_OUT}], t60[{N_OUT}], exc[{N_OUT}], rad[{N_OUT}];', '  int nq;',
         '  struct { int p, i, j; float g; } quad[5];', '};', '',
         f'constexpr int kPanNotes = {len(data)}, kPanModes = {N_OUT};', 'const PanNote kPan[kPanNotes] = {']
    fmt = lambda v: ', '.join(f'{x:.6g}f' for x in v)
    for d in data:
        q = ', '.join(f'{{{p}, {i}, {j}, {g:.6g}f}}' for p, i, j, g in d['quad'])
        L.append(f"    {{{hz(d['m']):.3f}f, {d['ring']}, {d['ang']:.4f}f, {d['r']:.4f}f,")
        L.append(f"     {{{fmt(d['ratio'])}}},")
        L.append(f"     {{{fmt(d['t60'])}}},")
        L.append(f"     {{{fmt(d['exc'])}}},")
        L.append(f"     {{{fmt(d['rad'])}}},")
        L.append(f"     {len(d['quad'])}, {{{q}}}}},")
    L += ['};', '', '}  // namespace pajupilli', '']
    open(path, 'w').write('\n'.join(L))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('-note', type=int, default=None)
    ap.add_argument('-o', default='src/steelpan_data.h')
    args = ap.parse_args()
    notes = note_layout()
    if args.note is None:
        import json
        from multiprocessing import Pool
        with Pool(min(32, len(notes))) as pool:
            data = pool.map(note_data, sorted(notes))
        json.dump(data, open('/tmp/steelpan_data.json', 'w'))
        write_header(data, args.o)
        for d in data:
            print(d['m'], ' '.join(f'{r:.3f}' for r in d['ratio']), ' quad', ['%.3g' % g for *_, g in d['quad']])
        return
    if args.note is not None:
        d = notes[args.note]
        p = Patch(dict(d, f=hz(args.note)))
        print(f"note {args.note} ({hz(args.note):.1f} Hz): ring {d['ring']}, {d['L']*1000:.0f} x {d['W']*1000:.0f} mm, "
              f"r {d['r']*1000:.0f} mm, bowl curvatures {p.km:.2f} {p.kh:.2f} /m, h {thickness(d['r'])*1000:.2f} mm, "
              f"grid {p.nx} x {p.ny}")
        for kx, ky in [(0, 0), (-p.km, -p.kh)]:
            f, _ = p.modes(kx, ky, 0, k=6, sigma=1.0)
            print(f'  dome {kx:.2f} {ky:.2f}: ' + ' '.join(f'{v:.1f}' for v in f))
        P, q, f, v = tune(args.note, dict(d), verbose=True)
        print('tuned', q, ' '.join(f'{x / f[0]:.3f}' for x in f))


if __name__ == '__main__':
    main()
