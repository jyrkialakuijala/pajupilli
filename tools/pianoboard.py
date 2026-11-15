#!/usr/bin/env python3
"""The concert grand's soundboard as resonances (research/piano.md, section 4).

  - the board: spruce in the outline of a 2.74 m grand, 9 mm thick in the middle thinning to
    6 mm at the rim, stiff along the grain and soft across it; the grain runs along the long
    bridge; the rim holds the edge;
  - the ribs: 17 bars glued under it across the grain, adding bending stiffness and mass along
    their lines (the method of the double bass's bass bar, tools/bassbody.py);
  - the bridges: the long (treble) bridge and the bass bridge, mass and stiffness along theirs;
  - a grid of 2.5 cm, turned to the grain; its lowest 256 resonances (16 registers of 16), each
    with its shape at the 88 bridge points (where each key's strings stand) and how much air it
    moves over each of about 50 patches of the board (20 cm squares): the piano hears every
    patch after its own distance (src/piano.cc);
  - above the last of them, where the board's resonances overlap and none is heard alone
    (the waves caught between ribs), 96 broad resonances a 66th of an octave apart stand for
    the rest: the bridge there is nearly a plain resistance.

    tools/pianoboard.py          the resonances, the bridge's impedance by register
    tools/pianoboard.py -emit    and write src/pianoboard_data.h
"""
import math
import os
import sys

import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
D = 0.025                       # grid spacing (m)
RHO, E_L, E_R, G_LR = 380.0, 11e9, 0.86e9, 0.70e9   # spruce (E_L, rho: Chabassier et al.)
H_MID, H_EDGE = 0.009, 0.006    # thickness (m)
RIB_SPACING, RIB_W, RIB_H, RIB_E, RIB_RHO = 0.125, 0.025, 0.025, 10e9, 400.0
BRIDGE_W, BRIDGE_H, BRIDGE_E, BRIDGE_RHO = 0.036, 0.042, 12e9, 700.0
Z_BRIDGE = 2500.0               # the bridge's impedance where the plate's resonances end (kg/s)
ETA = 0.02                      # mean loss factor (Ege & Boutillon)
N_MODES, N_TAIL, TAIL_TOP = 256, 96, 10000.0
EVEN = 3.0                      # the plate's resonances above 1 kHz this much broader than computed (evenness)
TAIL_ETA = 0.08                 # the broad resonances' loss factor (their width, not the wood's loss)
RHO_AIR, C_AIR = 1.18, 345.0
BRIDGES = (range(1, 21), range(21, 89))   # the keys on the bass bridge, on the long bridge
BRIDGE_MIN = 0.07               # the top of the long bridge: no nearer the front edge than this (m)
RAD_FROM, RAD_FULL = 50.0, 120.0   # the board radiates fully from RAD_FULL up, rising from RAD_FROM (Hz)
PATCH = 0.20                    # the patches' size (m)

# The outline (m): x from the belly rail (the keyboard's end) to the tail, y from the spine
# (the straight bass side) across to the treble side. 2.12 m long: the longest plain string
# (1.90 m, fitted) ends on its bridge 29 cm before the tail's rim; nearer, the rim holds the
# bridge's end and the lowest plain strings lose their fundamentals.
OUTLINE = [(0, 0), (1.93, 0), (2.07, 0.08), (2.12, 0.22), (2.09, 0.38), (1.93, 0.52), (1.66, 0.62),
           (1.34, 0.75), (1.07, 0.92), (0.86, 1.12), (0.66, 1.3), (0.48, 1.4), (0.27, 1.43), (0,
           1.43)]


def speaking_length(key):
    """Speaking length (m) of key 1..88: the wound bass 2.01 m to 1.33 m; the plain strings as
    long as their stiffness, fitted to two recorded grands, says (src/piano.cc: 1.90 m at
    key 21, halving every 13 keys or so, 53 mm at the top)."""
    pts = [(1, 2.01), (20, 1.33), (21, 1.896), (25, 1.534), (28, 1.310), (31, 1.118), (34, 0.954), (37, 0.814),
           (40, 0.695), (46, 0.505), (52, 0.368), (58, 0.267), (64, 0.194), (70, 0.141), (76, 0.102),
           (82, 0.074), (88, 0.053)]
    ks = [p[0] for p in pts]
    return float(np.exp(np.interp(key, ks, np.log([p[1] for p in pts]))))


def bridge_point(key):
    """Where key 1..88's strings cross their bridge (m). Keys 1..20: the bass bridge (the
    bass strings run over the others, towards the middle of the tail). The others: the string
    starts at the agraffe, which stands before the board's front edge by the gap the hammers
    come up through (7 cm; less where the strings are shorter than twice that)."""
    L = speaking_length(key)
    if key <= 20:
        return 0.30 + 0.72 * L, 0.26 + (key - 1) * 0.019
    return max(BRIDGE_MIN, L - min(0.07, 0.45 * L)), 0.22 + (key - 21) / 67 * 1.08


def bridge_line(keys):
    """A bridge's line: through its keys' points, and on past the last one (8 cm: the bar does
    not end at its last string; ending there, the top of the long bridge would hang free)."""
    pts = [bridge_point(k) for k in keys]
    (x0, y0), (x1, y1) = pts[-2], pts[-1]
    d = math.hypot(x1 - x0, y1 - y0)
    return pts + [(x1 + 0.08 * (x1 - x0) / d, y1 + 0.08 * (y1 - y0) / d)]


GRAIN = math.atan2(bridge_point(88)[1] - bridge_point(21)[1], bridge_point(88)[0] - bridge_point(21)[0])


def to_grain(x, y):
    c, s = math.cos(GRAIN), math.sin(GRAIN)
    return c * x + s * y, -s * x + c * y


def from_grain(u, v):
    c, s = math.cos(GRAIN), math.sin(GRAIN)
    return c * u - s * v, s * u + c * v


def inside(x, y):
    n, hit = len(OUTLINE), False
    for a in range(n):
        (x0, y0), (x1, y1) = OUTLINE[a], OUTLINE[(a + 1) % n]
        if (y0 > y) != (y1 > y) and x < x0 + (y - y0) * (x1 - x0) / (y1 - y0):
            hit = not hit
    return hit


def edge_distance(x, y):
    best = 1e9
    n = len(OUTLINE)
    for a in range(n):
        (x0, y0), (x1, y1) = OUTLINE[a], OUTLINE[(a + 1) % n]
        dx, dy = x1 - x0, y1 - y0
        t = max(0.0, min(1.0, ((x - x0) * dx + (y - y0) * dy) / (dx * dx + dy * dy)))
        best = min(best, math.hypot(x - x0 - t * dx, y - y0 - t * dy))
    return best


def build():
    """The grid (turned to the grain: u along it, v across), stiffness and mass."""
    corners = [to_grain(x, y) for x, y in OUTLINE]
    u0, u1 = min(c[0] for c in corners), max(c[0] for c in corners)
    v0, v1 = min(c[1] for c in corners), max(c[1] for c in corners)
    nu, nv = int((u1 - u0) / D) + 3, int((v1 - v0) / D) + 3
    idx = -np.ones((nu + 2, nv + 2), int)
    h = np.full((nu + 2, nv + 2), H_EDGE)
    pos = []
    for i in range(nu):
        for j in range(nv):
            x, y = from_grain(u0 + (i - 0.5) * D, v0 + (j - 0.5) * D)
            if inside(x, y) and edge_distance(x, y) > 0.4 * D:
                idx[i + 1, j + 1] = len(pos)
                pos.append((x, y))
                h[i + 1, j + 1] = H_EDGE + (H_MID - H_EDGE) * min(1.0, edge_distance(x, y) / 0.35)
    n = len(pos)
    pos = np.array(pos)
    M = np.zeros(n)
    K = np.zeros((n, n))
    area = D * D
    for i in range(1, nu + 1):
        for j in range(1, nv + 1):
            if idx[i, j] >= 0:
                M[idx[i, j]] = RHO * h[i, j] * area

    def add(stencil, coef):
        ids = [(idx[i, j], w) for i, j, w in stencil if idx[i, j] >= 0]
        for a, wa in ids:
            for b, wb in ids:
                K[a, b] += coef * area * wa * wb

    # The ribs: every fifth column of the grid (12.5 cm), across the grain.
    rib_cols = set(range(3, nu + 1, int(round(RIB_SPACING / D))))
    rib_ei = RIB_E * RIB_W * RIB_H ** 3 / 6    # (glued to the board: stiffer than the bar alone)
    # The bridges: the grid points nearest their lines.
    on_bridge = {}
    for keys in BRIDGES:
        pts = bridge_line(keys)
        for a in range(len(pts) - 1):
            (x0, y0), (x1, y1) = pts[a], pts[a + 1]
            steps = max(1, int(math.hypot(x1 - x0, y1 - y0) / (0.5 * D)))
            for t in range(steps):
                x, y = x0 + (x1 - x0) * t / steps, y0 + (y1 - y0) * t / steps
                u, v = to_grain(x, y)
                i, j = int(round((u - u0) / D + 0.5)) + 1, int(round((v - v0) / D + 0.5)) + 1
                if idx[i, j] >= 0:
                    on_bridge[(i, j)] = on_bridge.get((i, j), 0) + math.hypot(x1 - x0, y1 - y0) / steps
    bridge_ei = BRIDGE_E * BRIDGE_W * BRIDGE_H ** 3 / 6

    def weights(x, y):
        u, v = to_grain(x, y)
        fi, fj = (u - u0) / D + 0.5, (v - v0) / D + 0.5
        i, j = int(math.floor(fi)), int(math.floor(fj))
        a, b = fi - i, fj - j
        return [(i + 1, j + 1, (1 - a) * (1 - b)), (i + 2, j + 1, a * (1 - b)), (i + 1, j + 2, (1 - a) * b),
                (i + 2, j + 2, a * b)]

    # The bridges bend as beams along their own lines (wherever those run against the grain):
    # the curvature at each step of D along the line, from the grid points around it.
    for keys in BRIDGES:
        pts = np.array(bridge_line(keys))
        seg = np.hypot(*np.diff(pts, axis=0).T)
        along = np.concatenate([[0], np.cumsum(seg)])
        ss = np.arange(0, along[-1], D)
        line = [(np.interp(t, along, pts[:, 0]), np.interp(t, along, pts[:, 1])) for t in ss]
        for a in range(1, len(line) - 1):
            st = {}
            for (x, y), c in ((line[a - 1], 1.0), (line[a], -2.0), (line[a + 1], 1.0)):
                for i, j, w in weights(x, y):
                    st[(i, j)] = st.get((i, j), 0) + c * w / D ** 2
            add([(i, j, w) for (i, j), w in st.items()], bridge_ei / D)
    for i in range(1, nu + 1):
        for j in range(1, nv + 1):
            if idx[i, j] < 0 and not (idx[i - 1:i + 2, j - 1:j + 2] >= 0).any():
                continue
            h3 = h[i, j] ** 3 / 12
            dxx = E_L * h3
            dyy = E_R * h3
            if i in rib_cols:
                dyy += rib_ei / D
                if idx[i, j] >= 0:
                    M[idx[i, j]] += RIB_RHO * RIB_W * RIB_H * D
            if (i, j) in on_bridge:
                M[idx[i, j]] += BRIDGE_RHO * BRIDGE_W * BRIDGE_H * on_bridge[(i, j)]
            add([(i - 1, j, 1 / D ** 2), (i, j, -2 / D ** 2), (i + 1, j, 1 / D ** 2)], dxx)
            add([(i, j - 1, 1 / D ** 2), (i, j, -2 / D ** 2), (i, j + 1, 1 / D ** 2)], dyy)
    for i in range(0, nu + 1):  # twist, at the cell corners
        for j in range(0, nv + 1):
            c = 1 / (D * D)
            hh = np.mean([h[i, j], h[i + 1, j], h[i, j + 1], h[i + 1, j + 1]])
            add([(i, j, c), (i + 1, j, -c), (i, j + 1, -c), (i + 1, j + 1, c)], 4 * G_LR * hh ** 3 / 12)
    return K, M, pos, idx, (u0, v0)


def at_point(shapes, idx, origin, x, y):
    """The modes' shapes at a point: between the four grid points around it."""
    u, v = to_grain(x, y)
    fi, fj = (u - origin[0]) / D + 0.5, (v - origin[1]) / D + 0.5
    i, j = int(math.floor(fi)), int(math.floor(fj))
    a, b = fi - i, fj - j
    out = np.zeros(shapes.shape[1])
    for di, dj, w in ((0, 0, (1 - a) * (1 - b)), (1, 0, a * (1 - b)), (0, 1, (1 - a) * b), (1, 1, a * b)):
        k = idx[i + di + 1, j + dj + 1]
        if k >= 0:
            out += w * shapes[k]
    return out


def solve():
    K, M, pos, idx, origin = build()
    s = 1 / np.sqrt(M)
    w2, vec = np.linalg.eigh(K * s[:, None] * s[None, :])
    f = np.sqrt(np.maximum(w2[:N_MODES], 0)) / (2 * math.pi)
    shapes = vec[:, :N_MODES] * s[:, None]     # mass-normalised: sum m phi^2 = 1
    phi = np.array([at_point(shapes, idx, origin, *bridge_point(k)) for k in range(1, 89)])  # [key][mode]
    # The patches: squares of the board; how much air each resonance moves over each
    # (the integral of its shape there, m^2 per unit of the resonance's motion).
    cell = {}
    for k, (x, y) in enumerate(pos):
        cell.setdefault((int(x // PATCH), int(y // PATCH)), []).append(k)
    cells = sorted(cell)
    # (A sliver at the rim joins its largest neighbour.)
    for c in list(cells):
        if len(cell[c]) * D * D < 0.25 * PATCH * PATCH:
            nb = [d for d in cells if d != c and abs(d[0] - c[0]) + abs(d[1] - c[1]) == 1]
            if nb:
                cell[max(nb, key=lambda d: len(cell[d]))] += cell.pop(c)
                cells.remove(c)
    patch_pos = np.array([pos[cell[c]].mean(axis=0) for c in cells])
    patch_vol = np.array([shapes[cell[c]].sum(axis=0) * D * D for c in cells])   # [patch][mode]
    # Where the bending waves are as long as the air's (coincidence) and above, a plate
    # radiates fully, by waves shorter than these patches, which average them away. There each
    # resonance's air motion is brought up to full radiation (volume velocity
    # c sqrt(2 pi S / M) / omega for the whole board's area and mass), keeping where on the
    # board it moves. From where: fitted. A bare plate of this spruce would reach it near
    # 1.3 kHz; with full radiation only from there the keys from A4 to C6 came out 5 dB under
    # the middle and too bright, against two recorded grands (research/piano.md, 11): the
    # ribbed board radiates fully from about 120 Hz, rising from 50.
    full = C_AIR * math.sqrt(2 * math.pi * len(M) * D * D / M.sum()) / (2 * math.pi * np.maximum(f, 1.0))
    have = np.sqrt(np.sum(patch_vol ** 2, axis=0))
    share = np.clip(np.log(np.maximum(f, 1.0) / RAD_FROM) / np.log(RAD_FULL / RAD_FROM), 0.0, 1.0)
    for lo, hi in ((0, 300), (300, 700), (700, 1400), (1400, 2400)):
        sel = (f >= lo) & (f < hi)
        if sel.any():
            print(f'  radiation {lo}-{hi} Hz: the patches carry {np.median(have[sel] / full[sel]):.2f} of full radiation')
    patch_vol = patch_vol * np.maximum(1.0, share * full / np.maximum(have, 1e-12))[None, :]
    key_patch = [int(np.argmin(np.hypot(*(patch_pos - np.array(bridge_point(k))).T))) for k in range(1, 89)]
    return dict(f=f, phi=phi, patch_pos=patch_pos, patch_vol=patch_vol, key_patch=key_patch, mass=M.sum(),
                area=len(M) * D * D, n=len(M))


def tail(r):
    """Above the last resonance: broad ones, evenly spaced in pitch; each key's share of each
    is random, neighbouring keys alike (the waves between ribs reach a few keys along the
    bridge). The bridge's impedance there: where the plate's own resonances end it is what
    is measured on a bridge near a rib (Z_BRIDGE; 1000-2000 kg/s and more: Giordano, Nakamura,
    as reviewed by Ege & Boutillon), and from there it rises with frequency, as a mass: the
    board's waves grow shorter than the bridge is wide and cannot take it along. That rise is
    fitted: on two recorded grands the partials at 4-8 kHz of the upper keys die at 35-40 dB/s,
    half of what a bridge of 1500 kg/s there would take from them (research/piano.md, 11)."""
    rng = np.random.default_rng(88)
    f0 = r['f'][-1]
    ratio = (TAIL_TOP / f0) ** (1 / N_TAIL)
    f = f0 * ratio ** (np.arange(N_TAIL) + 0.75)
    phi = np.zeros((88, N_TAIL))
    vol = np.zeros((len(r['patch_pos']), N_TAIL))
    for k in range(N_TAIL):
        spacing = f[k] * (ratio - 1)
        z = Z_BRIDGE * f[k] / f0
        size = math.sqrt(4 * spacing / z)             # mean phi^2 = 4 * spacing * conductance
        reach = 3.0                                   # keys
        noise = rng.standard_normal(88 + 40)
        kern = np.exp(-0.5 * (np.arange(-20, 21) / reach) ** 2)
        smooth = np.convolve(noise, kern, 'valid')[:88]
        phi[:, k] = size * smooth / np.sqrt(np.mean(smooth ** 2))
        # Its sound: a strip between two ribs (0.125 m x 0.6 m, 0.6 kg) radiating fully (above
        # the board's coincidence), as the air moved by the patches under the keys it belongs
        # to: volume velocity = c sqrt(2 pi S / M) / omega per unit of its motion, in all.
        w = 2 * math.pi * f[k]
        for key in range(88):
            vol[r['key_patch'][key], k] += phi[key, k]
        vol[:, k] *= C_AIR * math.sqrt(2 * math.pi * 0.075 / 0.6) / w / math.sqrt(np.sum(vol[:, k] ** 2))
    return f, phi, vol


def loss_factors(f, pos, vol):
    """Each resonance's loss factor: the wood's (ETA) and what it gives to the air. The patches
    as sources in a wall: power = rho w^2 / (4 pi c) * sum_ij Q_i Q_j sin(k r_ij) / (k r_ij), over
    the resonance's energy (1/2 at unit motion). The lowest few, moving the whole board one
    way, lose more to the air than to the wood: broad, shallow resonances at the bridge."""
    r = np.hypot(pos[:, None, 0] - pos[None, :, 0], pos[:, None, 1] - pos[None, :, 1])
    out = np.empty(len(f))
    for m in range(len(f)):
        w = 2 * math.pi * max(f[m], 1.0)
        out[m] = ETA + RHO_AIR * w / (2 * math.pi * C_AIR) * (vol[:, m] @ np.sinc(w / C_AIR * r / math.pi) @ vol[:, m])
    return out


def impedance(f, phi, freqs, eta=ETA):
    """|force / velocity| at each key's bridge point (kg/s), at freqs."""
    w, wm = 2 * math.pi * np.asarray(freqs)[:, None, None], 2 * math.pi * f[None, None, :]
    y = np.sum(phi[None, :, :] ** 2 * 1j * w / (wm ** 2 - w ** 2 + 1j * eta * wm * w), axis=2)
    return 1 / np.abs(y)


def conductance(f, phi, freqs, eta=ETA):
    """Re(velocity / force) at each key's bridge point (s/kg): what a string loses to."""
    w, wm = 2 * math.pi * np.asarray(freqs)[:, None, None], 2 * math.pi * f[None, None, :]
    return np.real(np.sum(phi[None, :, :] ** 2 * 1j * w / (wm ** 2 - w ** 2 + 1j * eta * wm * w), axis=2))


def main():
    r = solve()
    tf, tphi, tvol = tail(r)
    f = np.concatenate([r['f'], tf])
    phi = np.concatenate([r['phi'], tphi], axis=1)
    vol = np.concatenate([r['patch_vol'], tvol], axis=1)
    eta = loss_factors(f, r['patch_pos'], vol)
    # The broad ones above the plate's own each stand for some seven real resonances: made
    # as broad as those together (or single notes of the top octaves find peaks and gaps
    # between them that the real board, its resonances overlapping tenfold, has not), and
    # moving as much more air, so that the same share of what the strings give them is heard.
    broad = np.arange(len(f)) >= N_MODES
    widen = np.where(broad, np.maximum(TAIL_ETA / eta, 1.0), 1.0)
    # The plate's own resonances from the middle up, too (x1 below 250 Hz rising to x EVEN at
    # 1 kHz): as computed they stand out singly, and a loud note of the upper register, nearly
    # all fundamental, came out 4 dB over its neighbours on one key and 14 dB under on another.
    # A fine grand is even from key to key there (its bridge's mobility is smooth above some
    # hundreds of Hz: more resonances than this plate has, each broader). The low ones, which
    # give the bass and tenor their body, stay as sharp as computed.
    widen = np.where(broad, widen, 1.0 + (EVEN - 1.0) * np.clip(np.log(np.maximum(f, 1.0) / 250.0) / np.log(4.0), 0.0, 1.0))
    vol = vol * np.sqrt(widen)[None, :]
    eta = eta * widen
    print(f"board: {r['area']:.2f} m^2, {r['mass']:.1f} kg with ribs and bridges, {r['n']} grid points, "
          f"grain {math.degrees(GRAIN):.0f} deg, {len(r['patch_pos'])} patches")
    print('lowest resonances (Hz):', ' '.join(f'{x:.0f}' for x in r['f'][:12]))
    print('their loss factors:    ', ' '.join(f'{x:.3f}' for x in eta[:12]), f' (at 500 Hz {np.median(eta[(f > 400) & (f < 600)]):.3f},'
          f' 2 kHz {np.median(eta[(f > 1600) & (f < 2400)]):.3f}, 8 kHz {np.median(eta[f > 7000]):.3f})')
    print(f"resonance 256: {r['f'][-1]:.0f} Hz; one per {(r['f'][200] - r['f'][50]) / 150:.1f} Hz between 51 and 201")
    bands = [(50, 100), (100, 300), (300, 1000), (1000, 2500), (2500, 5000), (5000, 9000)]
    zz = impedance(f, phi, np.geomspace(40, 400, 400), eta)
    print('the deepest dip of |Z| between 40 and 400 Hz (kg/s): ' + ' '.join(f'key {k}: {zz[:, k - 1].min():.0f}' for k in (1, 16, 25, 31, 40)))
    print('bridge impedance (kg/s): the median of |Z| over the band; then 1 / the mean conductance')
    print('  band (Hz)      A0     C2     C4     C6     C8        A0     C2     C4     C6     C8')
    for lo, hi in bands:
        z = impedance(f, phi, np.geomspace(lo, hi, 60), eta)
        g = conductance(f, phi, np.geomspace(lo, hi, 60), eta)
        print(f'  {lo:5d}-{hi:5d} ' + ' '.join(f'{np.median(z[:, k - 1]):6.0f}' for k in (1, 16, 40, 64, 88))
              + '    ' + ' '.join(f'{1 / np.mean(g[:, k - 1]):6.0f}' for k in (1, 16, 40, 64, 88)))
    if '-emit' in sys.argv:
        n = len(f)
        out = ['// Generated by tools/pianoboard.py -emit: the grand\'s soundboard as resonances.',
               '#pragma once', 'namespace pajupilli {', 'namespace pianoboard {',
               f'constexpr int kModes = {n};  // the first {N_MODES}: the plate\'s own; the rest: the overlapping ones above',
               'constexpr int kKeys = 88;', f"constexpr int kPatches = {len(r['patch_pos'])};", f'constexpr float kEta = {ETA}f;  // the wood\'s loss factor']

        def arr(name, a, per=8):
            a = np.asarray(a).ravel()
            out.append(f'alignas(64) constexpr float {name}[{len(a)}] = {{')
            for i in range(0, len(a), per):
                out.append('    ' + ' '.join(f'{x:.6e}f,' for x in a[i:i + per]))
            out.append('};')

        arr('kFreq', f)
        arr('kLoss', eta)               # [mode]: loss factor, the wood's and the air's
        arr('kPhi', phi)                # [key][mode]: velocity at the key's bridge point per unit modal velocity
        arr('kPatchPos', r['patch_pos'])  # [patch]: x from the belly rail, y from the spine (m)
        arr('kPatchVol', vol)           # [patch][mode]: the air it moves there (m^2 per unit motion)
        out += ['}  // namespace pianoboard', '}  // namespace pajupilli', '']
        open(os.path.join(ROOT, 'src', 'pianoboard_data.h'), 'w').write('\n'.join(out))
        print('wrote src/pianoboard_data.h')


if __name__ == '__main__':
    main()
