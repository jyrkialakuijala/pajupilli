#!/usr/bin/env python3
"""The concert harp's strings and soundboard as data (research/harp.md, sections 2 and 3).

The strings (2.1-2.3): 47, C1 to G7, numbered here from the bottom (0: C1 .. 46: G7):
  - lengths of the gut and nylon strings from the Elysian's table [WLA19 Table III], the wires'
    estimated (G2 1.10, C2 1.30, C1 1.50 m), in between by the logarithm;
  - the top nine (F6-G7) nylon, the next 26 (A2-E6) gut, the bottom twelve (C1-G2) steel wound
    with silvered copper: the gauges of the Elysian (gut and nylon) and Camac's wires;
  - every string tuned flat on its full length (all pedals up: C-flat major), except the three
    without discs (C1, D1, G7), tuned by hand to their naturals; the gut and nylon tensions from
    their mass and length at that pitch, the wires' from Galli's table (their mass from it);
  - the wires' steel cores (not found: 0.6 mm at G2 to 1.0 mm at C1, estimated) give their
    stiffness, inharmonicity and the stiffness along them.

The soundboard (3.1, 3.3, 3.4):
  - spruce, 1.42 m long, 0.50 m wide at the bass end narrowing to 0.10 m at the top, 11.5 mm
    thick at the bottom thinning to 2.25 mm [Waltham08], a 0.8 mm veneer on its face; the grain
    across the board (stiff across, soft along);
  - the string bar down the middle of its inside: Le Carrou's equivalent beam's stiffness (E 5.9
    GPa, J 38.9 cm^4 [LeCarrou09]: the board itself is soft along), thinning with the board; its
    mass that beam's (2.12 kg/m over the first metre) less the board's own, times BAR_MASS (2.3:
    so that 200 g on the board moves T1 and A0 nearly as Le Carrou measured, below);
  - each string tied through the bar at its own point on the centre line, 29 mm apart (34 of
    them over the metre from the foot to string 11 [LeCarrou09]), pulling at 35 degrees: its
    stiffness along itself and across it a spring at that point (the strung board's resonances
    higher than the bare one's [Wood22]);
  - its edges held by the box: from `edge` at the wide foot to clamped at the narrow top (the hold
    rising as the fourth power of the height);
  - a grid of 1 cm; its lowest resonances, each with its motion at the 47 points (towards the
    strings' plane: what the strings drive and feel; and its turning there, times the half
    thickness from the board's middle to its face, where the eyelet takes a string's sideways
    pull: what the strings' swing across their plane drives and feels; 10-15 dB under the swing
    towards the board, as Woodhouse measured the two plucks' partials);
  - the box's air (29 litres [LeCarrou07]) behind it, breathing through the five holes in the
    back (Le Carrou's areas, an end correction of 1.7 radii + 6 mm: 220 Hz were the walls rigid):
    the board's resonances and the air's, coupled. Three things fitted to three measurements
    [LeCarrou07]: the edges' hold at the foot (the board's first bending), the walls' give (the air
    alone) and how much of the board's sweep the box's air feels (the box's sides drawn in as the
    board bulges out: the coupling), so that T1 lies at 153.5 Hz (the air in the holes going the
    board's way), A0 at 172 Hz (against it), the air two thirds of A0 (a 2 cm chimney on the
    lowest hole lowered A0 3 Hz and T1 1.5). The result: the edges two thirds of the way to
    clamped ("quasi-clamped"), 200 g at the shaker lowering T1 6.3 Hz and A0 4.0 (measured 5 and
    3.5);
  - not the harp's own three lowest resonances, measured (61.5 Hz: the whole harp on its foot and
    the player's shoulder; 84.5 and 124.5 Hz: the box bending as a free beam): the one string
    measured, beside the third, decays as the board and the air alone make it (BODY, below);
  - above the grid's last resonances (2.4 kHz), where nothing was measured, broad ones a 40th of
    an octave apart to 10 kHz, each string's share random and alike for neighbours, as large as
    the grid's resonances had it there just below (the board's mobility at each point goes on as
    it was).

The sound: the board in patches (5 cm up it, each side of the bar; the box's sides drawn in beside
each row) and the five holes' air in the back, 25 cm behind: what each moves per unit of each
resonance, raised to full radiation from 120 Hz as the piano's board had to be (tools/pianoboard.py:
fitted to two recorded grands), each keeping where on the board it moves the air. The radiation
loss of each resonance from the same patches (sources in free air). src/harp.cc hears the patches
after their distances at two microphones.

    tools/harpboard.py          fit, solve and print
    tools/harpboard.py -emit    and write src/harpboard_data.h
    tools/harpboard.py -fitmass the scan behind BAR_MASS
"""
import math
import os
import sys

import numpy as np
import scipy.sparse as sp
import scipy.sparse.linalg as sla

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# ---- The strings -------------------------------------------------------------------------------
N_STR = 47
LETTER_SEMI = [0, 2, 4, 5, 7, 9, 11]
RHO_GUT, E_GUT, ETA_GUT = 1320.0, 6.0e9, 0.04          # [WLA19, Wood22]
RHO_NYLON, ETA_NYLON_E2 = 1070.0, 0.25e9               # nylon: E = 4.5 + 39 sigma GPa, E'' 0.25 GPa
E_STEEL = 2.0e11
THETA = math.radians(35.0)                              # the strings' angle to the board [Wood22, VSL]


def natural_midi(i):
    return 12 * (i // 7 + 2) + LETTER_SEMI[i % 7]


def by_note(i, table):
    """Interpolate a {string: value} table by the logarithm over the string number."""
    ks = sorted(table)
    return float(np.exp(np.interp(i, ks, np.log([table[k] for k in ks]))))


# Lengths (m) at the strings of the tables (string number from C1 = 0).
LENGTH = {46: 0.073, 45: 0.081, 44: 0.087, 42: 0.103, 39: 0.133, 35: 0.182, 32: 0.231, 28: 0.315, 26: 0.367,
          25: 0.397, 21: 0.550, 18: 0.714, 14: 0.965, 12: 1.067, 11: 1.10, 7: 1.30, 0: 1.50}
GUT_D = {46: 0.41, 45: 0.43, 44: 0.45, 42: 0.50, 39: 0.64, 35: 0.76, 32: 0.88, 28: 1.05, 26: 1.15, 25: 1.20,
         21: 1.45, 18: 1.75, 14: 2.00, 12: 2.20}
NYLON_D = {46: 0.40, 45: 0.41, 44: 0.43, 42: 0.50, 39: 0.66, 35: 0.79}
WIRE_D = [2.80, 2.70, 2.54, 2.45, 2.30, 2.20, 2.10, 2.00, 1.92, 1.82, 1.73, 1.60]   # C1 .. G2 (mm) [CamacChart]
WIRE_T = [364, 368, 349, 356, 410, 436, 444, 475, 515, 560, 463, 491]              # C1 .. G2 (N) [Galli]
DISCLESS = (0, 1, 46)                                    # C1, D1, G7: tuned by hand


def strings():
    out = []
    for i in range(N_STR):
        L = by_note(i, LENGTH)
        nat = natural_midi(i)
        f_nat = 440 * 2 ** ((nat - 69) / 12)
        f = f_nat if i in DISCLESS else f_nat * 2 ** (-1 / 12)    # the full length's pitch
        if i <= 11:
            mat, d = 'wire', WIRE_D[i] * 1e-3
            T = WIRE_T[i]
            mu = T / (2 * L * f) ** 2
            core = (1.0 - 0.4 * i / 11) * 1e-3
            EA = E_STEEL * math.pi * core ** 2 / 4
            EI = E_STEEL * math.pi * core ** 4 / 64
            etaE = 0.0
        else:
            nylon = i >= 38
            mat = 'nylon' if nylon else 'gut'
            d = (by_note(i, NYLON_D) if nylon else by_note(i, GUT_D)) * 1e-3
            rho = RHO_NYLON if nylon else RHO_GUT
            A = math.pi * d * d / 4
            mu = rho * A
            T = mu * (2 * L * f) ** 2
            E = (4.5e9 + 39 * (T / A)) if nylon else E_GUT
            EA = E * A
            EI = E * math.pi * d ** 4 / 64
            etaE = (ETA_NYLON_E2 / E) if nylon else ETA_GUT
        B = math.pi ** 2 * EI / (T * L * L)
        out.append(dict(i=i, midi=nat, L=L, d=d, mu=mu, T=T, EA=EA, B=B, f=f, mat=mat, etaE=etaE,
                        rhoEff=mu / (math.pi * d * d / 4)))
    return out


# ---- The board ---------------------------------------------------------------------------------
D = 0.01                                 # grid (m)
LB, W0, W1 = 1.42, 0.50, 0.10            # length; width at the foot and at the top (m)
H0, H1 = 0.0115, 0.00225                 # thickness (m)
RHO, E_ACROSS, E_ALONG, G_LR = 400.0, 11e9, 0.86e9, 0.70e9   # spruce, the grain across the board
BAR_MU_EQ = 2.12                         # kg/m: Le Carrou's equivalent beam, board and bar
# x the bar's mass so taken. Le Carrou's 200 g on the board lowered T1 by 5 Hz and A0 by 3.5: as
# taken (x1), the board here moved twice as much (8 and 9.6 Hz). A heavier bar (the board's own
# mass at the bass end, its stiffeners) brings it nearer: x2.3, the heaviest with which the board
# still fits T1 and A0 (its edges then held two thirds of the way to clamped), gives 6.3 and 4.0 Hz
# (tools/harpboard.py -fitmass: the scan).
BAR_MASS = [2.3]
BAR_EI = 5.9e9 * 38.9e-8                 # N m^2: its stiffness (E 5.9 GPa, J 38.9 cm^4), all the bar's (the board is soft along)
X0, PITCH = 0.035, 0.029                 # C1's point and the spacing up the bar (m)
ETA_WOOD, ETA_AIR, ETA_BODY = 0.02, 0.02, 0.05
V_BOX, RHO_AIR, C_AIR = 0.029, 1.2, 343.0
HOLES = [63e-4, 76e-4, 97e-4, 114e-4, 135e-4]   # top (1) to bottom (5) [LeCarrou07]
HOLE_X = [1.20, 0.98, 0.76, 0.54, 0.32]         # their height on the box's back (m) [E]
DEPTH = 0.25                                    # the board in front of the holes (m) [E]
T1, A0, SHARE_A0 = 153.5, 172.0, 2 / 3
# The harp's own three lowest resonances (61.5, 84.5, 124.5 Hz [LeCarrou07]: the whole harp on its
# foot, the box bending) as (Hz, mass at the bar (kg), shape): left out (an empty list). The one
# string measured, C-flat 3 (123.6 Hz, 1 Hz under the third), decays in 10.2 s; the board and the
# air alone make that 10.8 s: they hardly move the string bar.
BODY = []
N_SOLVE, TOP = 220, 2400.0
EDGE_RISE = 4.0                          # the edges' hold: from `edge` at the foot to clamped at the top, as u^4
BAR_TAPER = 1.0                          # the bar's mass and stiffness as the board's thickness to this power
VENEER = 0.0008                          # m: the face's veneer, its grain along the board [M]
TAIL_TOP, TAIL_PER_OCT = 10000.0, 40
TAIL_ETA = 0.06
PATCH = 0.05                             # the patches' length up the board (m)
RAD_FROM, RAD_FULL = 50.0, 120.0         # the board radiates fully from RAD_FULL up, rising from RAD_FROM (Hz): the piano's


def width(x):
    return W0 + (W1 - W0) * x / LB


def thick(x):
    return H0 + (H1 - H0) * x / LB


def edge_hold(edge, x):
    u = min(max(x / LB, 0.0), 1.0)
    return edge + (1 - edge) * u ** EDGE_RISE


def attach(i):
    return X0 + PITCH * i


def build(bar_ei_scale, strs, edge=1.0):
    """Stiffness and mass on the grid (j = 0 the centre line)."""
    ni = int(LB / D)
    nj = int(W0 / 2 / D) + 2
    idx = -np.ones((ni + 4, 2 * nj + 5), int)
    pos = []
    for i in range(ni):
        x = (i + 0.5) * D
        for j in range(-nj, nj + 1):
            y = j * D
            if abs(y) <= width(x) / 2 - 0.5 * D:
                idx[i + 2, j + nj + 2] = len(pos)
                pos.append((x, y))
    n = len(pos)
    pos = np.array(pos)
    M = RHO * (thick(pos[:, 0]) + VENEER) * D * D
    rows, cols, vals = [], [], []

    def add(stencil, coef):
        ids = [(idx[a, b], w) for a, b, w in stencil if idx[a, b] >= 0]
        for p, wp in ids:
            for q, wq in ids:
                rows.append(p)
                cols.append(q)
                vals.append(coef * D * D * wp * wq)

    # The board itself: bending along (x, soft: across the grain) and across (y, stiff), twist.
    for a in range(1, ni + 3):
        for b in range(1, 2 * nj + 4):
            if idx[a, b] < 0 and not (idx[a - 1:a + 2, b - 1:b + 2] >= 0).any():
                continue
            x = (a - 2 + 0.5) * D
            h3 = thick(min(max(x, 0), LB)) ** 3 / 12
            # (centred outside the board: the edge's hold against turning, 1 clamped .. 0 hinged;
            # `edge` at the foot, tightening towards the narrow top as EDGE_RISE says)
            r = 1.0 if idx[a, b] >= 0 else edge_hold(edge, x)
            # (the veneer on the face, its grain along the board: E_ACROSS x its thickness x its
            # distance from the board's middle squared)
            hv = thick(min(max(x, 0), LB))
            dxx = E_ALONG * h3 + E_ACROSS * VENEER * (0.5 * (hv + VENEER)) ** 2
            add([(a - 1, b, 1 / D ** 2), (a, b, -2 / D ** 2), (a + 1, b, 1 / D ** 2)], r * dxx)
            add([(a, b - 1, 1 / D ** 2), (a, b, -2 / D ** 2), (a, b + 1, 1 / D ** 2)], r * E_ACROSS * h3)
    for a in range(1, ni + 3):
        for b in range(1, 2 * nj + 4):
            hh = thick(min(max((a - 2 + 1) * D, 0), LB))
            c = 1 / (D * D)
            add([(a, b, c), (a + 1, b, -c), (a, b + 1, -c), (a + 1, b + 1, c)], 4 * G_LR * hh ** 3 / 12)
    # The bar: on the centre column; its mass the equivalent beam's less the board's (over the
    # first metre, on average), thinning as the board does; its stiffness as the board's cube.
    xs = np.linspace(0, 1.0, 101)
    board_mu = RHO * np.mean(thick(xs) * width(xs))
    bar_mu0 = BAR_MASS[0] * (BAR_MU_EQ - board_mu) / np.mean((thick(xs) / H0) ** BAR_TAPER)
    jc = nj + 2
    for a in range(2, ni + 2):
        k = idx[a, jc]
        if k < 0:
            continue
        x = pos[k, 0]
        M[k] += bar_mu0 * (thick(x) / H0) ** BAR_TAPER * D
        ei = bar_ei_scale * (thick(x) / H0) ** BAR_TAPER / np.mean((thick(xs) / H0) ** BAR_TAPER)
        add([(a - 1, jc, 1 / D ** 2), (a, jc, -2 / D ** 2), (a + 1, jc, 1 / D ** 2)], ei / D)
    K = sp.coo_matrix((vals, (rows, cols)), shape=(n, n)).tocsr()
    # The strings at their points: along themselves (EA / L) and across (T / L), towards the board.
    kd = np.zeros(n)
    for s in strs:
        k = idx[int(attach(s['i']) / D) + 2, jc]
        kd[k] += s['EA'] / s['L'] * math.sin(THETA) ** 2 + s['T'] / s['L'] * math.cos(THETA) ** 2
    K = K + sp.diags(kd)
    return K, M, pos, idx, jc, bar_mu0


def point(shapes, idx, jc, x, dj=0):
    fi = x / D - 0.5
    i = int(math.floor(fi))
    a = fi - i
    out = 0
    for di, w in ((0, 1 - a), (1, a)):
        k = idx[i + di + 2, jc + dj]
        if k >= 0:
            out = out + w * shapes[k]
    return out


def plate_modes(edge, strs, n=N_SOLVE):
    K, M, pos, idx, jc, bar_mu0 = build(BAR_EI, strs, edge)
    w2, vec = sla.eigsh(K, k=n, M=sp.diags(M), sigma=0, which='LM')
    o = np.argsort(w2)
    w2, vec = w2[o], vec[:, o]
    vec = vec / np.sqrt(np.sum(M[:, None] * vec ** 2, axis=0))[None, :]   # mass-normalised
    return dict(w2=w2, vec=vec, pos=pos, idx=idx, jc=jc, M=M, bar_mu0=bar_mu0)


def air():
    req = [math.sqrt(s / math.pi) for s in HOLES]
    s_over_l = [s / (0.006 + 1.7 * r) for s, r in zip(HOLES, req)]
    return sum(s_over_l), [x / sum(s_over_l) for x in s_over_l]


def couple(pm, wall, cv=1.0):
    """The board's resonances with the box's air: the modes' volumes into one cavity whose
    stiffness is rho c^2 / V (less by the walls' give: x 1 / (1 + wall)), and the air in the
    holes a mass rho / sum(S / L). Returns frequencies, the board's share of each new mode (in the
    plate's modes' basis), and the air's."""
    sl, _ = air()
    vol = cv * pm['vec'].sum(axis=0) * D * D                 # each plate mode's volume, as the box's air feels it
    kc = RHO_AIR * C_AIR ** 2 / V_BOX / (1 + wall)
    ma = RHO_AIR / sl
    n = len(pm['w2'])
    K = np.zeros((n + 1, n + 1))
    K[:n, :n] = np.diag(pm['w2']) + kc * np.outer(vol, vol)
    K[:n, n] = K[n, :n] = kc * vol / math.sqrt(ma)
    K[n, n] = kc / ma
    w2, v = np.linalg.eigh(K)
    v_air = v[n, :] / math.sqrt(ma)                          # the holes' volume per unit of each mode
    return np.sqrt(np.maximum(w2, 0)) / (2 * math.pi), v[:n, :], v_air, vol


def breathing(pm):
    """The board's first bending: of its lowest few resonances the one that sweeps most volume."""
    vol = np.abs(pm['vec'][:, :6].sum(axis=0))
    return int(np.argmax(vol))


def pair(pm, wall, cv=1.0):
    """T1 and A0 of this board with this give of the walls: the two new modes made most of the
    board's first bending and the air (the lower T1, the upper A0), and the air's share of A0."""
    f, vb, va, vol = couple(pm, wall, cv)
    sl = air()[0]
    share = vb[breathing(pm), :] ** 2 + va ** 2 * RHO_AIR / sl
    two = sorted(np.argsort(share)[-2:])
    return f[two[0]], f[two[1]], va[two[1]] ** 2 * RHO_AIR / sl


def mass_test(pm, wall, cv):
    """How far 200 g on the board between the A-flat 2 and B-flat 2 strings lowers T1 and A0
    (Le Carrou: 5 and 3.5 Hz): -1/2 dm phi^2 f, phi the new modes' motion there."""
    f, vb, va, vol = couple(pm, wall, cv)
    x = 0.5 * (attach(12) + attach(13))
    at = point(pm['vec'], pm['idx'], pm['jc'], x) @ vb
    sl = air()[0]
    share = vb[breathing(pm), :] ** 2 + va ** 2 * RHO_AIR / sl
    two = sorted(np.argsort(share)[-2:])
    return [0.5 * 0.2 * at[k] ** 2 * f[k] for k in two]


def fit_mass(strs):
    """The scan behind BAR_MASS: each bar mass, the fit, and what 200 g does."""
    for b in (1.0, 1.4, 1.7, 2.0, 2.3, 2.6):
        BAR_MASS[0] = b
        edge, wall, cv = fit(strs)
        d = mass_test(plate_modes(edge, strs, 40), wall, cv)
        print(f'  bar x{b:.1f}: edges {edge:.2f}, walls {wall:.2f}, breathing {cv:.2f}; 200 g lowers T1 {d[0]:.1f} Hz, A0 {d[1]:.1f} Hz')


def fit(strs):
    """The edges' hold, the walls' give and the box's breathing that put T1 and A0 where they
    were measured, with the air two thirds of A0 (a 2 cm chimney on the lowest hole lowered A0
    by 3 Hz and T1 by 1.5 [LeCarrou07]). The board's first bending and the air as a pair: from
    the two frequencies and the share, the board alone (X), the air alone (Y) and their coupling
    (K) follow; X sets the edges, Y the walls, K the share of the board's sweep the box's air
    feels (the rest taken up by the box's sides, drawn in as the board bulges out)."""
    sl = air()[0]
    ma = RHO_AIR / sl
    want = [T1, A0]
    edge, wall, cv = 0.5, 0.5, 1.0
    for it in range(4):
        l1, l2 = (2 * math.pi * want[0]) ** 2, (2 * math.pi * want[1]) ** 2
        sa = SHARE_A0
        X_plus_Z = l1 * sa + l2 * (1 - sa)
        Y = l1 * (1 - sa) + l2 * sa
        Kba = math.sqrt(sa * (1 - sa)) * (l2 - l1)
        kc = Y * ma
        wall = RHO_AIR * C_AIR ** 2 / V_BOX / kc - 1
        # (the board's first mode alone: X = X_plus_Z - Z, Z = kc v^2 cv^2 = Kba^2 / Y)
        X = X_plus_Z - Kba ** 2 / Y
        lo, hi = 0.0, 1.0
        for _ in range(16):
            mid = 0.5 * (lo + hi)
            pm = plate_modes(mid, strs, 12)
            if pm['w2'][breathing(pm)] < X:
                lo = mid
            else:
                hi = mid
        edge = 0.5 * (lo + hi)
        pm = plate_modes(edge, strs, 40)
        v1 = abs(pm['vec'][:, breathing(pm)].sum() * D * D)
        cv = Kba / (kc * v1 / math.sqrt(ma))
        t1, a0, sh = pair(pm, wall, cv)
        if it == 3: print(f'  fit {it}: edges {edge:.3f}, walls {wall:.2f}, breathing {cv:.3f}: T1 {t1:.1f}, A0 {a0:.1f}, air in A0 {sh:.2f}')
        # (the other modes shift the pair a little: aim off by as much)
        want = [want[0] * T1 / t1, want[1] * A0 / a0]
    return edge, wall, cv


def body_shape(kind, x):
    u = x / LB
    if kind == 'rock':
        return u
    beta = 4.730 if kind == 'bend1' else 7.853
    sig = (math.cosh(beta) - math.cos(beta)) / (math.sinh(beta) - math.sin(beta))
    z = beta * u
    return (math.cosh(z) + math.cos(z)) - sig * (math.sinh(z) + math.sin(z))


def main():
    strs = strings()
    print('strings (from the bottom): note, length, diameter, tension, B, material')
    for s in strs[::4] + [strs[-1]]:
        print(f"  {s['i']:2d} midi {s['midi']:3d}  L {s['L']:.3f} m  d {s['d'] * 1e3:.2f} mm  T {s['T']:5.0f} N  "
              f"B {s['B']:.1e}  {s['mat']}")
    print(f"total pull {sum(s['T'] for s in strs) / 1000:.1f} kN")
    if '-fitmass' in sys.argv:
        fit_mass(strs)
        return
    edge, wall, cv = fit(strs)
    pm = plate_modes(edge, strs)
    print('200 g at the shaker lowers T1 and A0 by', ' and '.join(f'{d:.1f}' for d in mass_test(pm, wall, cv)), 'Hz (measured 5 and 3.5)')
    t1, a0, _ = pair(pm, wall, cv)
    print(f"bar: {pm['bar_mu0']:.2f} kg/m at the foot; the edges' hold {edge:.2f} (1 clamped); the walls give "
          f"{wall:.2f} x the air's stiffness; the box's air feels {cv:.2f} of the board's sweep; T1 {t1:.1f} Hz, A0 {a0:.1f} Hz")
    f, vb, va, vol = couple(pm, wall, cv)
    vec = pm['vec'] @ vb                                    # the coupled modes' shapes on the board
    keep = f < TOP
    f, vec, va = f[keep], vec[:, keep], va[keep]
    vb = vb[:, keep]
    n = len(f)
    idx, jc, pos = pm['idx'], pm['jc'], pm['pos']
    phi = np.array([point(vec, idx, jc, attach(i)) for i in range(N_STR)])                  # [string][mode]
    dwdy = np.array([(point(vec, idx, jc, attach(i), 1) - point(vec, idx, jc, attach(i), -1)) / (2 * D)
                     for i in range(N_STR)])
    side = np.array([0.5 * thick(attach(i)) for i in range(N_STR)])[:, None] * dwdy
    # The sound: the board in patches (5 cm up the board, each side of the bar), the box's sides
    # drawn in by (1 - cv) of each row's sweep beside it; the holes' air in the back, DEPTH behind.
    _, hole_share = air()
    rows = int(math.ceil(LB / PATCH))
    ppos, pvol = [], []
    for r in range(rows):
        inrow = (pos[:, 0] >= r * PATCH) & (pos[:, 0] < (r + 1) * PATCH)
        if not inrow.any():
            continue
        row = vec[inrow].sum(axis=0) * D * D
        for half in (-1, 1):
            sel = inrow & ((pos[:, 1] < 0) if half < 0 else (pos[:, 1] >= 0))
            if not sel.any():
                continue
            v = vec[sel].sum(axis=0) * D * D - 0.5 * (1 - cv) * row
            ppos.append((pos[sel, 0].mean(), pos[sel, 1].mean(), 0.0))
            pvol.append(v)
    board_patches = len(ppos)
    for h, x in zip(hole_share, HOLE_X):
        ppos.append((x, 0.0, -DEPTH))
        pvol.append(h * va)
    ppos, pvol = np.array(ppos), np.array(pvol)          # [patch][3], [patch][mode]
    # The patches carry what the grid's modes move, but the board radiates more than that above
    # its lowest resonances: the piano's board, made the same way (tools/pianoboard.py), had to
    # be brought up to full radiation (volume velocity c sqrt(2 pi S / M) / omega per unit of the
    # mode, S and M the board's) from about 120 Hz, rising from 50, to match two recorded grands.
    # The same here, each mode keeping where on the board (and the holes) it moves the air.
    area0, mass0 = len(pos) * D * D, pm['M'].sum()
    full = C_AIR * math.sqrt(2 * math.pi * area0 / mass0) / (2 * math.pi * np.maximum(f, 1.0))
    have = np.sqrt(np.sum(pvol ** 2, axis=0))
    share = np.clip(np.log(np.maximum(f, 1.0) / RAD_FROM) / np.log(RAD_FULL / RAD_FROM), 0.0, 1.0)
    for lo_, hi_ in ((100, 300), (300, 1000), (1000, 2400)):
        sel = (f >= lo_) & (f < hi_)
        if sel.any():
            print(f'  radiation {lo_}-{hi_} Hz: the patches carry {np.median(have[sel] / full[sel]):.2f} of full radiation')
    pvol = pvol * np.maximum(1.0, share * full / np.maximum(have, 1e-12))[None, :]
    # Losses: the wood's, the air's in the holes (by the air's share), and radiation (the patches
    # as sources in free air: rho w / 4 pi c sum_ij Q_i Q_j sin(k r_ij) / (k r_ij)).
    air_share = va ** 2 * RHO_AIR / air()[0]
    w = 2 * math.pi * f
    rr = np.linalg.norm(ppos[:, None, :] - ppos[None, :, :], axis=2)
    rad_eta = np.array([RHO_AIR * w[m] / (4 * math.pi * C_AIR) * (pvol[:, m] @ np.sinc(w[m] / C_AIR * rr / math.pi) @ pvol[:, m])
                        for m in range(n)])
    eta = ETA_WOOD * (1 - air_share) + ETA_AIR * air_share + rad_eta
    # The harp's own three, measured.
    bf, bphi, bside, beta_ = [], [], [], []
    for fr, mass, kind in BODY:
        shp = np.array([body_shape(kind, attach(i)) for i in range(N_STR)])
        shp = shp / np.max(np.abs(shp)) / math.sqrt(mass)
        bf.append(fr), bphi.append(shp), bside.append(np.zeros(N_STR)), beta_.append(ETA_BODY)
    # The broad ones above: each string's share random, alike for neighbours, as large as the
    # grid's resonances had it just below; radiating fully (above the board's coincidence: volume
    # velocity c sqrt(2 pi S / M) / omega per unit motion, S and M the board's), from the patches
    # under the strings it belongs to.
    rng = np.random.default_rng(47)
    f_t = f[-1] * 2 ** ((np.arange(int(TAIL_PER_OCT * math.log2(TAIL_TOP / f[-1]))) + 0.75) / TAIL_PER_OCT)
    top = f > 0.6 * f[-1]
    density = (phi[:, top] ** 2).sum(axis=1) / (f[-1] - 0.6 * f[-1])          # [string]: each point's own
    side_density = np.mean(side[:, top] ** 2, axis=0).sum() / (f[-1] - 0.6 * f[-1])
    area, mass = len(pos) * D * D, pm['M'].sum()
    near = [int(np.argmin(np.abs(ppos[:board_patches, 0] - attach(i)) + 10 * (ppos[:board_patches, 1] < 0))) for i in range(N_STR)]
    tf, tphi, tside, tvol = [], [], [], []
    for k, fr in enumerate(f_t):
        spacing = fr * (2 ** (1 / TAIL_PER_OCT) - 1)
        noise = rng.standard_normal(N_STR + 20)
        kern = np.exp(-0.5 * (np.arange(-10, 11) / 2.5) ** 2)
        sm = np.convolve(noise, kern, 'valid')[:N_STR]
        sm = sm / np.sqrt(np.mean(sm ** 2))
        tphi.append(sm * np.sqrt(density * spacing))
        tside.append(rng.standard_normal(N_STR) * math.sqrt(side_density * spacing) * 0.7)
        v = np.zeros(len(ppos))
        for i in range(N_STR):
            share = rng.uniform(0.2, 0.8)
            v[near[i]] += share * sm[i]
            v[near[i] - 1] += (1 - share) * sm[i] * (1 if rng.uniform() < 0.5 else -1)   # (the other side of the bar)
        v *= C_AIR * math.sqrt(2 * math.pi * area / mass) / (2 * math.pi * fr) / np.linalg.norm(v)
        tf.append(fr), tvol.append(v)
    allf = np.concatenate([bf, f, tf])
    allphi = np.concatenate([np.array(bphi).reshape(len(BODY), N_STR).T, phi, np.array(tphi).T], axis=1)
    allside = np.concatenate([np.array(bside).reshape(len(BODY), N_STR).T, side, np.array(tside).T], axis=1)
    alleta = np.concatenate([beta_, eta, np.full(len(tf), TAIL_ETA)])
    allvol = np.concatenate([np.zeros((len(ppos), len(BODY))), pvol, np.array(tvol).T], axis=1)   # [patch][mode]
    print(f'{board_patches} patches of the board and {len(HOLES)} holes; radiation loss factors at T1 {rad_eta[0]:.3f}, A0 {rad_eta[1]:.3f}, '
          f'median 300-1000 Hz {np.median(rad_eta[(f > 300) & (f < 1000)]):.4f}, 1-2.4 kHz {np.median(rad_eta[f > 1000]):.4f}')
    print(f'{n} resonances of the board and air under {TOP:.0f} Hz, {len(BODY)} of the harp, {len(tf)} broad above')
    print('lowest (Hz):', ' '.join(f'{x:.0f}' for x in f[:16]))
    print('loss factors:', ' '.join(f'{x:.3f}' for x in eta[:16]))
    print('air shares:  ', ' '.join(f'{x:.2f}' for x in air_share[:16]))
    # Where each resonance moves the bar most, against the string tuned there.
    print('the bar: each resonance\'s largest motion (string at it, that string\'s fundamental):')
    for m in range(min(n, 24)):
        a = np.abs(phi[:, m])
        if a.max() < 0.2 * np.abs(phi).max():
            continue
        s = int(np.argmax(a))
        print(f'  {f[m]:6.0f} Hz  string {s:2d} ({strs[s]["f"]:.0f} Hz flat)  |phi| {a.max():.2f}  side {np.abs(side[:, m]).max():.3f}')
    # The driving-point mobility on the bar (m/s/N) at a few strings, as Waltham measured its shape.
    for i in (5, 12, 21, 28, 35, 42):
        fr = np.geomspace(50, 4000, 600)
        wv = 2 * math.pi * fr[:, None]
        wm = 2 * math.pi * allf[None, :]
        y = np.sum(allphi[i][None, :] ** 2 * 1j * wv / (wm ** 2 - wv ** 2 + 1j * alleta * wm * wv), axis=1)
        pk = fr[np.argmax(np.abs(y))]
        print(f'  string {i:2d}: mobility at its own f0 {np.abs(y[np.argmin(np.abs(fr - strs[i]["f"]))]):.4f} m/s/N, '
              f'peak {np.abs(y).max():.3f} at {pk:.0f} Hz, median {np.median(np.abs(y)):.4f}')
    if '-emit' in sys.argv:
        nm = len(allf)
        out = ['// Generated by tools/harpboard.py -emit: the concert harp\'s strings and soundboard.',
               '#pragma once', 'namespace pajupilli {', 'namespace harpboard {',
               f'constexpr int kStrings = {N_STR};',
               f'constexpr int kModes = {nm};  // the harp\'s own {len(BODY)}, the board and air\'s {n}, {len(tf)} broad ones above',
               f'constexpr int kBodyModes = {len(BODY)};  // (the first: the harp\'s own)',
               f'constexpr float kTheta = {THETA:.6f}f;  // the strings\' angle to the board (rad)',
               f'constexpr float kAttachX[kStrings] = {{{", ".join(f"{attach(i):.4f}f" for i in range(N_STR))}}};']

        def arr(name, a, per=8, fmt='{:.6e}f'):
            a = np.asarray(a).ravel()
            out.append(f'alignas(64) constexpr float {name}[{len(a)}] = {{')
            for i in range(0, len(a), per):
                out.append('    ' + ' '.join(fmt.format(x) + ',' for x in a[i:i + per]))
            out.append('};')

        # The strings: full length (m), mass (kg/m), tension (N), stiffness along (EA, N),
        # inharmonicity B, diameter (m), the material's own loss in bending, its density (the
        # whole section's), the full length's pitch (Hz), material (0 wire, 1 gut, 2 nylon).
        arr('kLength', [s['L'] for s in strs])
        arr('kMu', [s['mu'] for s in strs])
        arr('kTension', [s['T'] for s in strs])
        arr('kEA', [s['EA'] for s in strs])
        arr('kB', [s['B'] for s in strs])
        arr('kDiameter', [s['d'] for s in strs])
        arr('kEtaE', [s['etaE'] for s in strs])
        arr('kRhoEff', [s['rhoEff'] for s in strs])
        arr('kFull', [s['f'] for s in strs])
        out.append('constexpr int kMaterial[kStrings] = {' + ', '.join(str({'wire': 0, 'gut': 1, 'nylon': 2}[s['mat']]) for s in strs) + '};')
        arr('kFreq', allf)
        arr('kLoss', alleta)
        arr('kPhi', allphi)          # [string][mode]: the bar's motion towards the strings per unit of the mode (mass-normalised)
        arr('kSide', allside)        # [string][mode]: its turning there x the arm: across the strings' plane
        out.append(f'constexpr int kPatches = {len(ppos)};  // {board_patches} of the board, then the holes')
        arr('kPatchPos', ppos)       # [patch][3]: up the board from its foot, across from the bar, out of its face (m)
        arr('kPatchVol', allvol)     # [patch][mode]: the air it moves per unit of the mode (m^3)
        out += ['}  // namespace harpboard', '}  // namespace pajupilli', '']
        open(os.path.join(ROOT, 'src', 'harpboard_data.h'), 'w').write('\n'.join(out))
        print('wrote src/harpboard_data.h')


if __name__ == '__main__':
    main()
