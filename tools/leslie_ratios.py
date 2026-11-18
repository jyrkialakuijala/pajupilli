#!/usr/bin/env python3
"""The Leslie cabinets' speeds chosen away from small whole-number ratios.

Several rotors turning at speeds in a simple ratio (3:2, 6:5, 8:5 ...) fall into step: their
swirls line up again and again, a pattern that repeats. Speeds far from every simple ratio drift
past each other without ever repeating. This searches the speed step between the cabinets and the
first cabinet's horn-to-drum ratio for the set whose every pairwise ratio stays furthest from any
fraction p/q with q <= Q.

The measure: for a ratio x, d(x) = min over q <= Q (17) of q |ln(x / (p/q))| (p the nearest
numerator): its relative distance from the nearest simple fraction, the simpler the fraction the
wider the berth it wants (3 % from 4/5 as bad as 1 % from 15/17; nothing over 17 counted). The
set's score is its single worst cross ratio's d (the closest miss decides, no sum). Larger is
better.

The rotors (src/hammond.cc, src/leslie.cc): cabinet c's speed factor f_c (1, 1/step, 1/step^2,
step: the user's "17 % less, 17 % less again, 17 % more"); its horn at slow 0.80 f_c, fast 6.67 f_c;
its drum at the horn's speed over its ratio: the first cabinet's own (h, slow and fast), the others
the published 122's (1.20 slow, 1.17 fast). Slow and fast are scored apart (all the rotors run on
one switch).

  python3 tools/leslie_ratios.py [--q 10] [--free-pub] [--n 400000]
  python3 tools/leslie_ratios.py --four [--rotors]   (1.0 and three free factors, every cross ratio)

(Several cabinets turn at exactly their factors in the model: a lone cabinet's +-3 % pulley
spread would move every ratio by more than these optima differ.)
"""
import argparse
import math

import numpy as np

QMAX = 17
POWER = 1.0  # (the weight of the fraction's simplicity: q^POWER)


def diophantine(x, qmax=None):
    """x: an array of ratios. How far x stays from every simple fraction p/q with q <= qmax (17):
    min over those of q |ln(x / (p/q))|, the relative distance weighted by how simple the
    fraction is. 3 % from 4/5 counts as 0.15, as 1 % from 15/17 does; fractions over larger
    denominators (18/19, 31/35) are not counted at all. Only the closest one matters."""
    qmax = qmax or QMAX
    best = np.full(np.shape(x), np.inf)
    lx = np.log(x)
    for q in range(1, qmax + 1):
        p = np.maximum(np.round(x * q), 1)
        best = np.minimum(best, q ** POWER * np.abs(lx - np.log(p / q)))
    return best


def nearest(x):
    """The fraction (q <= QMAX) that decides x's score, and x's distance from it in percent."""
    q = min(range(1, QMAX + 1), key=lambda q: q ** POWER * abs(math.log(x / (max(round(x * q), 1) / q))))
    p = max(round(x * q), 1)
    return f'{x:.4f} ({100 * (x / (p / q) - 1):+.1f} % from {p}/{q})'


def rotors(step, h, rs, rf, mode):
    """The 8 rotors' speeds (arrays over the candidates): cabinets 1 .. 4 at speed factors
    1, 1/step, 1/step^2, step; horn, then drum."""
    f = [np.ones_like(step), 1 / step, 1 / step ** 2, step]
    horn0 = 0.80 if mode == 'slow' else 6.67
    r = rs if mode == 'slow' else rf
    out = []
    for c in range(4):
        horn = horn0 * f[c]
        out += [horn, horn / (h if c == 0 else r)]
    return out


def score(step, h, rs, rf, skip=()):
    """The worst pair's d over both modes; skip: pairs (mode, i, j) whose ratio is fixed."""
    worst = np.full(np.shape(step), np.inf)
    arg = [None] * np.size(step)
    for mode in ('slow', 'fast'):
        s = rotors(step, h, rs, rf, mode)
        for i in range(8):
            for j in range(i + 1, 8):
                if (mode, i, j) in skip:
                    continue
                x = np.maximum(s[i], s[j]) / np.minimum(s[i], s[j])
                worst = np.minimum(worst, diophantine(x))
    return worst


def worst_pair(step, h, rs, rf, skip=()):
    best = (math.inf, None)
    for mode in ('slow', 'fast'):
        s = rotors(np.array([step]), np.array([h]), np.array([rs]), np.array([rf]), mode)
        for i in range(8):
            for j in range(i + 1, 8):
                if (mode, i, j) in skip:
                    continue
                x = float(max(s[i][0], s[j][0]) / min(s[i][0], s[j][0]))
                d = float(diophantine(np.array([x]))[0])
                if d < best[0]:
                    best = (d, f'{mode}: {name(i)} / {name(j)} = {nearest(x)}')
    return best


def name(i):
    return f'cabinet {i // 2 + 1} {"horn" if i % 2 == 0 else "drum"}'


def fixed_pairs(free_pub):
    """The pairs whose ratio no searched value changes (nothing to choose there)."""
    a = rotors(np.array([1.17]), np.array([1.6]), np.array([1.2]), np.array([1.17]), 'slow'), \
        rotors(np.array([1.17]), np.array([1.6]), np.array([1.2]), np.array([1.17]), 'fast')
    b = rotors(np.array([1.21]), np.array([1.53]), np.array([1.2 if not free_pub else 1.16]), np.array([1.17 if not free_pub else 1.21]), 'slow'), \
        rotors(np.array([1.21]), np.array([1.53]), np.array([1.2 if not free_pub else 1.16]), np.array([1.17 if not free_pub else 1.21]), 'fast')
    out = set()
    for k, mode in enumerate(('slow', 'fast')):
        for i in range(8):
            for j in range(i + 1, 8):
                if abs(a[k][i][0] / a[k][j][0] - b[k][i][0] / b[k][j][0]) < 1e-12:
                    out.add((mode, i, j))
    return out


def four(rotors_too, n, rng, rs=1.20, rf=1.17, h=1.6, min_step=1.08):
    """Four speed factors, the first 1.0, the other three free in 0.6 .. 1.5: every cross ratio
    (each pair, either way) away from the simple fractions; rotors_too: all eight rotors' (each
    cabinet's horn and drum: the first's horn/drum h, the others' the published rs / rf) at slow and
    fast instead."""
    def sc(F):  # F: (n, 4)
        worst = np.full(len(F), np.inf)
        # (the cabinets apart: every cross ratio at least min_step, else the set is out)
        for i in range(4):
            for j in range(i + 1, 4):
                x = np.maximum(F[:, i], F[:, j]) / np.minimum(F[:, i], F[:, j])
                worst = np.where(x < min_step, -1.0, worst)
        if not rotors_too:
            for i in range(4):
                for j in range(i + 1, 4):
                    x = np.maximum(F[:, i], F[:, j]) / np.minimum(F[:, i], F[:, j])
                    worst = np.minimum(worst, diophantine(x))
            return worst
        for horn0, r in ((0.80, rs), (6.67, rf)):
            sp = []
            for c in range(4):
                sp += [horn0 * F[:, c], horn0 * F[:, c] / (h if c == 0 else r)]
            for i in range(8):
                for j in range(i + 1, 8):
                    if i // 2 == j // 2 and i // 2 > 0:
                        continue  # (a cabinet's own horn / drum: the published ratio, fixed)
                    if i // 2 == j // 2 == 0:
                        continue  # (the first's own: h, fixed here)
                    x = np.maximum(sp[i], sp[j]) / np.minimum(sp[i], sp[j])
                    worst = np.minimum(worst, diophantine(x))
        return worst
    F = np.column_stack([np.ones(n), rng.uniform(0.6, 1.5, (n, 3))])
    s_ = sc(F)
    top = F[np.argsort(-s_)[:200]]
    # (each of the best refined: small random steps kept when they help)
    for it in range(60):
        cand = top[:, None, :] + np.concatenate([np.zeros((len(top), 40, 1)), rng.normal(0, 0.004 * (1 - it / 60) + 1e-4, (len(top), 40, 3))], axis=2)
        cand = cand.reshape(-1, 4)
        cand[:, 1:] = np.clip(cand[:, 1:], 0.6, 1.5)
        cs = sc(cand).reshape(len(top), 40)
        best = cs.argmax(1)
        cur = sc(top)
        better = cs[np.arange(len(top)), best] > cur
        top[better] = cand.reshape(len(top), 40, 4)[np.arange(len(top)), best][better]
    s_ = sc(top)
    order = np.argsort(-s_)
    shown = []
    for k in order:
        f = np.sort(top[k])
        if all(np.abs(f - g).max() > 0.02 for g in shown):
            shown.append(f)
            pairs = []
            for i in range(4):
                for j in range(i + 1, 4):
                    pairs.append(nearest(max(f[i], f[j]) / min(f[i], f[j])))
            print(f'{"rotors" if rotors_too else "factors"}: ' + ', '.join(f'{v:.4f}' for v in f) + f'  (score {s_[k]:.4f})')
            print('    cross ratios: ' + ', '.join(pairs))
        if len(shown) == 5:
            break


def main():
    global QMAX, POWER
    ap = argparse.ArgumentParser()
    ap.add_argument('--q', type=int, default=17, help='the largest denominator counted as simple')
    ap.add_argument('--free-pub', action='store_true', help='also choose the other cabinets\' horn/drum (1.15-1.22)')
    ap.add_argument('--n', type=int, default=400000, help='random candidates')
    ap.add_argument('--min-step', type=float, default=1.08, help='with --four: every cross ratio at least this (the cabinets apart)')
    ap.add_argument('--four', action='store_true', help='four free speed factors (1.0 and three more)')
    ap.add_argument('--rotors', action='store_true', help='with --four: score all eight rotors, not the factors alone')
    ap.add_argument('--power', type=float, default=1.0, help='the weight of a fraction\'s simplicity, q^power (smaller: the small fractions matter more)')
    a = ap.parse_args()
    QMAX = a.q
    POWER = a.power
    if a.four:
        four(a.rotors, a.n, np.random.default_rng(1), min_step=a.min_step)
        return
    skip = fixed_pairs(a.free_pub)
    for st in sorted(skip):
        print(f'(fixed, not scored: {st[0]} {name(st[1])} / {name(st[2])})')
    d, w = worst_pair(1.17, 1.6, 1.20, 1.17)
    print(f'now: step 1.17, horn/drum 1.6, others 1.20 / 1.17: worst {d:.4f} ({w})')
    rng = np.random.default_rng(1)
    step = rng.uniform(1.10, 1.25, a.n)
    h = rng.uniform(1.40, 1.70, a.n)
    rs = rng.uniform(1.15, 1.22, a.n) if a.free_pub else np.full(a.n, 1.20)
    rf = rng.uniform(1.15, 1.22, a.n) if a.free_pub else np.full(a.n, 1.17)
    sc = score(step, h, rs, rf, skip)
    order = np.argsort(-sc)
    seen = []
    for k in order:
        if all(abs(step[k] - s0) > 0.01 or abs(h[k] - h0) > 0.03 for s0, h0 in seen):
            seen.append((step[k], h[k]))
            d, w = worst_pair(step[k], h[k], rs[k], rf[k], skip)
            print(f'candidate {len(seen)}: step {step[k]:.4f} (down {1 / step[k]:.4f}), horn/drum {h[k]:.4f}, '
                  f'others {rs[k]:.4f} slow / {rf[k]:.4f} fast: worst {d:.4f} ({w})')
        if len(seen) == 6:
            break


if __name__ == '__main__':
    main()
