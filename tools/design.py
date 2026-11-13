#!/usr/bin/env python3
"""Instrument design: a transfer-matrix model (TMM) of the instruments described
by a bore profile, tone holes and an excitation (src/designs.cc), mirroring the
waveguide's physics (src/tube.cc, src/flute.cc, src/reed.cc): the same hole,
pad, radiation, window and reed formulas.

As an instrument maker would, it places the tone holes (and the end of the
bore) so that each fingering's resonance falls on its note, by least squares
over every fingering at once (cross fingerings and the upper registers pull on
the same holes, as on a real instrument). Notes without a fingering of their
own (cross fingerings, half-holes, vents) are then found by search among a
player's plausible fingerings. Writes src/designs.cc.

    tools/design.py [NAME ...]          fit and write (default: all)
    tools/design.py -emit               write src/designs.cc from the saved fits
    tools/design.py -scale SRC DST K    DST as SRC's fit scaled by K (the baritone sax: the alto x2)
    tools/design.py -peaks NAME MIDI    the resonances of a written note's fingering
"""
import itertools, json, math, os, sys
import numpy as np

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, 'tools'))
from instruments import INSTRUMENTS  # noqa: E402

rho, c, nu, mu = 1.18, 349.5, 1.56e-5, 1.85e-5
KEMBR = 0.01           # PJ_EMBR: lip-slit loss in a flute's window
COURANT = 0.975
FINGER, NCLOSED, PERF = 1, 2, 4


def nfreq(m):
    return 440.0 * 2 ** ((m - 69) / 12)


def tin(b, d):
    """Inner end correction of a side hole (Dalmont / Nederveen), d = b / a."""
    return b * (0.82 - 0.193 * d - 1.09 * d * d + 1.27 * d ** 3 - 0.71 * d ** 4)


def t_series(b, a, t, closed):
    d = b / a
    th = math.tanh(1.84 * t / b)
    f = 1 / th if closed else th
    return -b * d * d / (1.78 * f + 0.940 + 0.540 * d + 0.285 * d * d)


def tube(w, L, a, Z):
    """A short lossy cylinder: load Z at its far end, impedance at its near end."""
    S = np.pi * a * a
    kap = 2.94 * rho * np.sqrt(nu) / (S * a)
    Zs = 1j * w * rho / S + kap * np.sqrt(1j * w)
    Ys = 1j * w * S / (rho * c * c)
    g = np.sqrt(Zs * Ys)
    Zc = np.sqrt(Zs / Ys)
    th = np.tanh(g * L)
    return Zc * (Z + Zc * th) / (Zc + Z * th)


class Model:
    """One instrument: geometry in metres."""

    def __init__(self, spec):
        self.s = spec
        self.kind = spec['kind']
        self.set_end(spec['end'])
        self.closed = spec.get('closed', False)
        self.holes = [dict(h) for h in spec.get('holes', [])]
        self.fs = spec['fs']
        self.dx = c / (COURANT * self.fs)
        self.excite = spec.get('excite', 0.5 * self.dx * 1e3) * 1e-3

    def set_end(self, mm):
        self.end = mm * 1e-3
        if not hasattr(self, 'extra'):
            self.extra = {k: v for k, v, _, _ in self.s.get('extra', [])}
        pts = self.s['bore_fn'](mm, **self.extra) if 'bore_fn' in self.s else self.s['bore']
        self.bore_mm = pts
        self.bore = [(x * 1e-3, d * 1e-3) for x, d in pts]

    def diam(self, x):
        b = self.bore
        if x <= b[0][0]:
            return b[0][1]
        for (x0, d0), (x1, d1) in zip(b, b[1:]):
            if x <= x1:
                return d0 + (d1 - d0) * (x - x0) / max(1e-12, x1 - x0)
        return b[-1][1]

    # --- holes
    def hole_open_Z(self, w, h, frac=1.0):
        x, b = h['x'] * 1e-3, 0.5e-3 * h['d']
        t = h['t'] * 1e-3
        a = self.diam(x) / 2
        dl = b / a
        tm = b * dl * (1 + 0.207 * dl ** 3) / 8
        if h['flags'] & FINGER:
            be = b * math.sqrt(frac)
            S = np.pi * be * be
            ts = t + tin(be, be / a) + tm
            Lr = rho * 0.6133 * be / S
            Rr = 1.5 * rho * c / S
            Zr = 1j * w * Lr * Rr / (Rr + 1j * w * Lr)
            return 1j * w * rho * ts / S + Zr, (1 - frac) * np.pi * b * b * (t + tm - 0.05 * b)
        S = np.pi * b * b
        hp = h['padH'] * 1e-3 * frac
        R = 0.5e-3 * h['cup']
        lnr = math.log(R / b)
        dpad = b * b * lnr / (2 * hp)
        if h['flags'] & PERF:
            q, e = 3.4e-3, 3.6e-3
            de = 1.64 * b / q - 0.15 * b / R - 1.1 + e * b / (q * q)
            dpad /= 1 + 5 * de ** -1.35 * (hp / b) ** -0.2
        ts = t + tin(b, dl) + tm + dpad
        rs = 6 * mu * lnr / (np.pi * hp ** 3)
        Lr = rho * 0.6133 * b / S
        Rr = 1.5 * rho * c / S
        Zr = 1j * w * Lr * Rr / (Rr + 1j * w * Lr)
        return 1j * w * rho * ts / S + rs + Zr, 0.0

    def hole_closed_Y(self, w, h):
        x, b = h['x'] * 1e-3, 0.5e-3 * h['d']
        a = self.diam(x) / 2
        dl = b / a
        tm = b * dl * (1 + 0.207 * dl ** 3) / 8
        return 1j * w * np.pi * b * b * (h['t'] * 1e-3 + tm - 0.05 * b) / (rho * c * c)

    def Zbore(self, w, pattern, top=None):
        """Impedance looking down the bore from x = top (default: the excitation)."""
        top = self.excite if top is None else top
        end = self.end
        a = self.diam(end - 1e-4) / 2
        S = np.pi * a * a
        if self.closed:
            Z = 1e12 / S + 0 * w
        else:
            ka = w / c * a
            Z = rho * c / S * (0.25 * ka * ka + 1j * 0.6133 * ka)
        hs = sorted(enumerate(self.holes), key=lambda ih: -ih[1]['x'])
        x = end
        for item in hs + [None]:
            xh = item[1]['x'] * 1e-3 if item else top
            xh = max(xh, top)
            L = x - xh
            if L > 0:
                n = max(1, int(math.ceil(L / max(2e-3, 0.01 * xh))))
                for j in range(n):
                    xm = x - L * (j + 0.5) / n
                    Z = tube(w, L / n, self.diam(xm) / 2, Z)
            x = xh
            if item is None:
                break
            i, h = item
            ch = pattern[i] if i < len(pattern) else 'o'
            ah = self.diam(xh) / 2
            b = 0.5e-3 * h['d']
            if ch == 'x':
                Y = self.hole_closed_Y(w, h)
                ta = t_series(b, ah, h['t'] * 1e-3, True)
            else:
                frac = 1.0 if ch == 'o' else (0.25 if h['flags'] & FINGER else 0.5)
                Zh, vol = self.hole_open_Z(w, h, frac)
                Y = 1 / Zh + 1j * w * vol / (rho * c * c)
                ta = t_series(b, ah, h['t'] * 1e-3, False) if frac == 1 else 0
            Za = 1j * w * rho * ta / (np.pi * ah * ah)
            Z = Za / 2 + 1 / (1 / (Z + Za / 2) + Y)
        return Z

    # --- what the excitation sees
    def response(self, w, pattern, midi=None, cover=1.0):
        """|Y| at the window (flue) or |Z| at the reed: its peaks are the notes."""
        Z = self.Zbore(w, pattern)
        if self.kind in ('reed', 'dreed'):
            r = self.s['reed']
            # the reed: a damped mass-spring sweeping sr (its compliance below fr)
            wr = 2 * np.pi * r['fr']
            Y = 1 / Z + 1j * w * r['sr'] / r['k'] / (1 - (w / wr) ** 2 + 1j * r['qr'] * w / wr)
            return np.abs(1 / Y)
        j = self.s['jet']
        top = self.excite
        if top > 1e-4:  # the cavity above the window, in parallel
            ac = self.diam(top / 2) / 2
            Zcav = tube(w, top, ac, 1e12 / (np.pi * ac * ac) + 0 * w)
            Z = 1 / (1 / Z + 1 / Zcav)
        a = self.diam(top) / 2
        if self.kind == 'duct':
            sw = j['window'] * j['w'] * 1e-6
            rin = math.sqrt(sw / math.pi)
            sbot = sw
            extra = 0.0
        else:  # end-blown: the lips leave `unc` of the rim open (the player's schedule)
            fw = nfreq(midi - self.s['transpose'])
            t = min(max(math.log(fw / 261.6) / math.log(1760 / 261.6), 0.0), 1.2)
            unc = (6.0 - 2.5 * t) * 1e-3 * self.s['emb'] * cover
            sw = max(4e-6 * self.s['emb'] ** 2, unc * 2 * a)
            rin = a
            sbot = math.pi * a * a
            extra = 2e-3 * self.s['emb']  # (the player's face rule is the transverse flute's)
        rw = math.sqrt(sw / math.pi)
        m = rho * (j['chimney'] * 1e-3 / math.sqrt(sw * sbot) + tin(rin, rin / a) / sbot + extra / sw)
        Lr = rho * 0.82 * rw / sw
        Rr = 1.35 * rho * c / sw
        Zw = 1j * w * m + KEMBR * rho * c / sw + 1j * w * Lr * Rr / (Rr + 1j * w * Lr)
        return np.abs(1 / (Z + Zw))

    def peaks(self, pattern, fmin, fmax, midi=None, step=5.0):
        """Resonance peaks (Hz) between fmin and fmax, parabolic in log magnitude."""
        n = int(1200 * math.log2(fmax / fmin) / step) + 3
        f = fmin * 2 ** (np.arange(n) * step / 1200)
        y = np.log(self.response(2 * np.pi * f, pattern, midi))
        i = np.where((y[1:-1] > y[:-2]) & (y[1:-1] >= y[2:]))[0] + 1
        out = []
        for k in i:
            # prominent peaks only (3 dB over the troughs on both sides)
            lo = y[:k][::-1]
            lmin = y[k]
            for v in lo:
                if v > y[k]: break
                lmin = min(lmin, v)
            rmin = y[k]
            for v in y[k + 1:]:
                if v > y[k]: break
                rmin = min(rmin, v)
            if y[k] - max(lmin, rmin) < 0.35:
                continue
            d = 0.5 * (y[k - 1] - y[k + 1]) / (y[k - 1] - 2 * y[k] + y[k + 1])
            out.append(fmin * 2 ** ((k + d) * step / 1200))
        return out

    def resonance(self, pattern, n, target, midi=None):
        """The n-th resonance of a fingering, refined near the target (Hz)."""
        f1 = target / n
        pk = self.peaks(pattern, 0.6 * f1, target * 1.35, midi, 6.0)
        if len(pk) < n:
            return None
        f = pk[n - 1]
        fine = self.peaks(pattern, f * 2 ** (-30 / 1200), f * 2 ** (30 / 1200), midi, 0.5)
        if fine:
            f = min(fine, key=lambda v: abs(math.log(v / f)))
        return f


def cents(f, t):
    return 1200 * math.log2(f / t) if f else float('nan')


def target(spec, m):
    """Passive resonance a written note aims at: its sounding pitch, offset by
    how the instrument plays against its resonance (spec['offset'], cents)."""
    return nfreq(m + spec['transpose']) * 2 ** (spec.get('offset', 0) / 1200)


def errors(model, spec, notes, fixed_only=True):
    out = {}
    for m, (pat, n) in sorted(notes.items()):
        f = model.resonance(pat, n, target(spec, m), m + spec['transpose'])
        out[m] = cents(f, target(spec, m)) if f else float('nan')
    return out


def fit(spec, verbose=True):
    """Least squares on the hole positions (and the end) over the core fingerings."""
    from scipy.optimize import least_squares
    model = Model(spec)
    core = spec['core']
    movable = [i for i, h in enumerate(model.holes) if not h.get('fixed')]
    fit_end = spec.get('fit_end', True)
    keys = {'x': ['x'], 'd': ['d'], 'xd': ['x', 'd']}[spec.get('fit', 'x')]
    extras = spec.get('extra', [])
    var = [(i, k) for k in keys for i in movable]  # hole parameters

    def apply(p):
        for (i, k), v in zip(var, p):
            model.holes[i][k] = v
        for (name, _, _, _), v in zip(extras, p[len(var):]):
            model.extra[name] = v
        model.set_end(p[len(var) + len(extras)] if fit_end else model.end * 1e3)

    def res(p):
        apply(p)
        r = []
        for m, (pat, n, wgt) in core.items():
            t = target(spec, m)
            f = model.resonance(pat, n, t, m + spec['transpose'])
            r.append(wgt * (cents(f, t) if f else 300.0))
        if 'x' in keys:  # keep holes apart (a pad needs room)
            xs = sorted(model.holes[i]['x'] for i in movable)
            for a, b in zip(xs, xs[1:]):
                gap = b - a
                need = spec.get('gap', 8)
                r.append(0 if gap > need else 10 * (need - gap))
        return r

    x0, lb, ub = [], [], []
    for i, k in var:
        h = model.holes[i]
        x0.append(h[k])
        if k == 'x':
            lb.append(h.get('lo', 5))
            ub.append(h.get('hi', spec['end'] * 1.5))
        else:
            lb.append(h.get('dlo', 0.6 * h['d'] if spec.get('fit') == 'xd' else 1.5))
            ub.append(h.get('dhi', 1.4 * h['d'] if spec.get('fit') == 'xd' else 14))
    for name, v, lo, hi in extras:
        x0.append(v)
        lb.append(lo)
        ub.append(hi)
    if fit_end:
        x0.append(spec['end'])
        lb.append(0.5 * spec['end'])
        ub.append(1.6 * spec['end'])
    x0 = [min(max(v, l + 1e-3), u - 1e-3) for v, l, u in zip(x0, lb, ub)]
    sol = least_squares(res, x0, bounds=(lb, ub), x_scale=10.0, diff_step=1e-3, max_nfev=400)
    apply(sol.x)
    if verbose:
        rr = res(sol.x)[:len(core)]
        if extras:
            print('  extra:', {k: round(v, 3) for k, v in model.extra.items()})
        print(f"{spec['name']}: rms {np.sqrt(np.mean(np.square(rr))):.1f} c over {len(core)} fingerings, "
              f"worst {max(abs(v) for v in rr):.1f} c", flush=True)
    return model


def search(model, spec, m, cands, keep=None):
    """The best fingering for written note m among candidates (pattern, n)."""
    t = target(spec, m)
    best = None
    for pat, n, cost in cands:
        f = model.resonance(pat, n, t, m + spec['transpose'])
        if not f:
            continue
        e = abs(cents(f, t)) + cost
        if best is None or e < best[0]:
            best = (e, pat, n, cents(f, t))
    return best


def complete(model, spec):
    """Every written note lo..hi: a core fingering, or the best candidate."""
    notes = {}
    for m in range(spec['lo'], spec['hi'] + 1):
        if m in spec['core']:
            pat, n, _ = spec['core'][m]
            notes[m] = (pat, n)
    gen = spec.get('candidates')
    for m in range(spec['lo'], spec['hi'] + 1):
        if m in notes:
            continue
        best = search(model, spec, m, gen(m, notes)) if gen else None
        if best is None:
            raise SystemExit(f"{spec['name']}: no fingering for {m}")
        notes[m] = (best[1], best[2])
    return notes


# --- C++ output

def cpp_name(name):
    return 'k' + ''.join(p.capitalize() for p in name.replace('-', ' ').split())


def emit(results):
    out = ['// Generated by tools/design.py from tools/instruments.py: do not edit.',
           '// The instruments described by their geometry, fingerings and excitation',
           '// (the flute family is in src/flute.cc).',
           '#include "instrument.h"', '', 'namespace pajupilli {', 'namespace {', '']
    entries = []
    for spec, model, notes, errs in results:
        cn = cpp_name(spec['name'])
        out.append(f"// {spec['title']}")
        for line in spec.get('doc', '').strip().splitlines():
            out.append('// ' + line.strip())
        out.append(f"const BorePoint {cn}Bore[] = {{")
        out.append('    ' + ', '.join(f'{{{x:.1f}, {d:.2f}}}' for x, d in model.bore_mm) + '};')
        if model.holes:
            out.append(f"const HoleDef {cn}Holes[] = {{")
            for i, h in enumerate(model.holes):
                fl = ' | '.join(n for b, n in ((FINGER, 'kFingerHole'), (NCLOSED, 'kNormallyClosed'), (PERF, 'kPerforated'))
                                if h['flags'] & b) or '0'
                out.append(f"    {{{h['x']:.1f}, {h['d']:.2f}, {h['t']:.1f}, {h['padH']:.1f}, {h['cup']:.1f}, {fl}}},"
                           f"  // {i + 1} {h.get('name', '')}")
            out.append('};')
        out.append(f"const NoteInfo {cn}Notes[] = {{")
        for m in range(spec['lo'], spec['hi'] + 1):
            pat, n = notes[m]
            e = errs.get(m, float('nan'))
            out.append(f'    {{"{pat}", {n}}},  // {m} {note_name(m)} ({e:+.0f} c passive)')
        out.append('};')
        if spec.get('pipes'):
            out.append(f"const PipeDef {cn}Pipes[] = {{")
            out.append('    ' + ',\n    '.join('{' + ', '.join([str(p[0])] + [f'{v:.2f}' for v in p[1:]]) + '}'
                                                for p in spec['pipes']) + '};')
        r = spec.get('reed', {})
        j = spec.get('jet', {})
        reed = (f"{{{r.get('width', 0):g}, {r.get('h0', 0):g}, {r.get('k', 0):g}, {r.get('fr', 0):g}, "
                f"{r.get('qr', 0):g}, {r.get('sr', 0):g}, {r.get('psi', 0):g}, {r.get('sd', 0):g}, "
                f"{r.get('press', 0):g}, {r.get('pressHi', 0):g}, {r.get('vib', 0):g}, "
                f"{r.get('meetAt', 1):g}, {r.get('contact', 0):g}}}")
        jet = (f"{{{j.get('h', 0):g}, {j.get('width', 0):g}, {j.get('w', 0):g}, {j.get('window', 0):g}, "
               f"{j.get('chimney', 0):g}, {j.get('channel', 0):g}, {j.get('y0', 0):g}, {j.get('press', 0):g}, "
               f"{j.get('pressHi', 0):g}}}")
        jet_si = (f"{{{j.get('h', 0) * 1e-3:g}, {j.get('width', 0) * 1e-3:g}, {j.get('w', 0) * 1e-3:g}, "
                  f"{j.get('window', 0) * 1e-3:g}, {j.get('chimney', 0) * 1e-3:g}, {j.get('channel', 0) * 1e-3:g}, "
                  f"{j.get('y0', 0) * 1e-3:g}, {j.get('press', 0):g}, {j.get('pressHi', 0):g}}}")
        del jet
        holes = f"{cn}Holes, {len(model.holes)}" if model.holes else "nullptr, 0"
        pipes = f"{cn}Pipes, {len(spec['pipes'])}" if spec.get('pipes') else "nullptr, 0"
        out.append(f"const Design {cn}Design = {{{cn}Bore, {len(model.bore_mm)}, {holes}, {cn}Notes, "
                   f"{spec.get('excite', 0.5 * model.dx * 1e3):.2f}, {model.end * 1e3:.1f}, "
                   f"{'true' if model.closed else 'false'}, {pipes}, {reed}, {jet_si}}};")
        out.append('')
        kind = {'duct': 'kDuct', 'end': 'kEndBlown', 'reed': 'kReed', 'dreed': 'kDoubleReed'}[spec['kind']]
        entries.append(f"    {{\"{spec['name']}\", {spec['transpose']}, {spec['lo']}, {spec['hi']}, {spec.get('scale', 1):g}, "
                       f"1, {spec.get('emb', 1):g}, {spec['fs']}, 0, false, false, 0, {spec.get('gain', 1):g}, "
                       f"Kind::{kind}, &{cn}Design, \"{spec.get('family', spec['name'])}\"}},")
    out += ['}  // namespace', '', 'extern const Instrument kDesigned[] = {'] + entries + ['};',
            f'extern const int kNumDesigned = {len(entries)};', '', '}  // namespace pajupilli', '']
    return '\n'.join(out)


def note_name(m):
    return ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'][m % 12] + str(m // 12 - 1)


CACHE = os.path.join(ROOT, 'tools', 'designs')  # one fit per instrument (json)


def load_saved():
    out = {}
    if os.path.isdir(CACHE):
        for fn in sorted(os.listdir(CACHE)):
            if fn.endswith('.json'):
                out[fn[:-5]] = json.load(open(os.path.join(CACHE, fn)))
    return out


def design(spec, verbose=True):
    if spec.get('pipes_fn'):  # a pan flute: each pipe cut to its note
        spec['pipes'] = spec['pipes_fn'](spec, Model)
        notes = {m: ('', 1) for m in range(spec['lo'], spec['hi'] + 1)}
        return Model(spec), notes, {m: 0.0 for m in notes}
    model = fit(spec, verbose) if spec.get('core') and model_has_holes(spec) else fit_end_only(spec, verbose)
    notes = complete(model, spec)
    errs = errors(model, spec, notes)
    return model, notes, errs


def model_has_holes(spec):
    return bool(spec.get('holes'))


def fit_end_only(spec, verbose):
    model = Model(spec)
    return model


def main():
    args = sys.argv[1:]
    if args and args[0] == '-grid':  # the waveguide (./flute -imp) against this TMM, per note
        import subprocess
        from concurrent.futures import ThreadPoolExecutor
        spec = INSTRUMENTS[args[1]]
        sv = load_saved()[spec['name']]
        model = Model(spec)
        for h, x, d in zip(model.holes, sv['x'], sv['d']):
            h['x'], h['d'] = x, d
        model.extra.update(sv.get('extra', {}))
        model.set_end(sv['end'])
        env = dict(os.environ)
        for kv in args[2:]:
            k, v = kv.split('=')
            env[k] = v

        def one(m):
            pat, n = sv['notes'][str(m)]
            t = target(spec, m)
            ref = model.resonance(pat, n, t, m + spec['transpose'])
            e = dict(env, PJ_IMPHOLES=pat)
            out = subprocess.run([os.path.join(ROOT, 'flute'), '-inst', spec['name'], '-imp', str(m + spec['transpose'])],
                                 capture_output=True, text=True, env=e).stdout
            got = [float(v) for v in out.split(':', 1)[1].split('(')[0].split()] if ':' in out else []
            g = min(got, key=lambda v: abs(math.log(v / ref))) if got and ref else float('nan')
            return m, pat, n, cents(g, ref) if ref else float('nan')
        with ThreadPoolExecutor(16) as ex:
            res = list(ex.map(one, range(spec['lo'], spec['hi'] + 1)))
        for m, pat, n, e in res:
            print(f'{m} {note_name(m):4} {pat} n{n} waveguide - tmm {e:+.1f} c')
        print('rms', np.sqrt(np.nanmean([r[3] ** 2 for r in res])))
        return
    if args and args[0] == '-scale':  # a family member: another's fit scaled (lengths, diameters)
        src, dst, k = args[1], args[2], float(args[3])
        a = load_saved()[src]
        spec = INSTRUMENTS[dst]
        b = {'x': [v * k for v in a['x']], 'd': [v * k for v in a['d']], 'end': a['end'] * k,
             'extra': a.get('extra', {}), 'notes': a['notes'], 'errs': {}, 'pipes': None}
        model = Model(spec)
        for h, x, d in zip(model.holes, b['x'], b['d']):
            h['x'], h['d'] = x, d
        model.extra.update(b['extra'])
        model.set_end(b['end'])
        for m, (pat, n) in b['notes'].items():
            t = target(spec, int(m))
            f = model.resonance(pat, n, t, int(m) + spec['transpose'])
            b['errs'][m] = cents(f, t) if f else float('nan')
        json.dump(b, open(os.path.join(CACHE, dst + '.json'), 'w'), indent=0)
        print(f'{dst} = {src} x {k}: passive rms {np.sqrt(np.nanmean([e * e for e in b["errs"].values()])):.1f} c')
        args = ['-emit']
    if args and args[0] == '-peaks':
        spec = INSTRUMENTS[args[1]]
        saved = load_saved()[spec['name']]
        model = Model(spec)
        for h, x, d in zip(model.holes, saved['x'], saved['d']):
            h['x'], h['d'] = x, d
        model.extra.update(saved.get('extra', {}))
        model.set_end(saved['end'])
        m = int(args[2])
        pat, n = saved['notes'][str(m)]
        t = target(spec, m)
        print(pat, n, ' '.join(f'{f:.1f}' for f in model.peaks(pat, 0.3 * t / n, 4 * t, m + spec['transpose'])),
              f'(target {t:.1f})')
        return
    names = [a for a in args if not a.startswith('-')] or list(INSTRUMENTS)
    if '-emit' in args:  # only write src/designs.cc from the saved fits (tools/designs.json)
        names = []
    saved = load_saved()
    os.makedirs(CACHE, exist_ok=True)
    for name in names:
        spec = INSTRUMENTS[name]
        model, notes, errs = design(spec)
        bad = {m: round(e) for m, e in errs.items() if not abs(e) < 15}
        print(f"  {name}: passive errors over the range rms "
              f"{np.sqrt(np.nanmean(np.square(list(errs.values())))):.1f} c; over 15 c: {bad}", flush=True)
        saved[name] = {'x': [h['x'] for h in model.holes], 'd': [h['d'] for h in model.holes], 'end': model.end * 1e3,
                       'extra': getattr(model, 'extra', {}),
                       'notes': {str(m): v for m, v in notes.items()}, 'errs': {str(m): e for m, e in errs.items()},
                       'pipes': spec.get('pipes')}
        json.dump(saved[name], open(os.path.join(CACHE, name + '.json'), 'w'), indent=0)
    results = []
    for name, spec in INSTRUMENTS.items():
        if name not in saved:
            continue
        sv = saved[name]
        if sv.get('pipes'):
            spec['pipes'] = sv['pipes']
        model = Model(spec)
        for h, x, d in zip(model.holes, sv['x'], sv['d']):
            h['x'], h['d'] = x, d
        model.extra.update(sv.get('extra', {}))
        model.set_end(sv['end'])
        notes = {int(k): tuple(v) for k, v in sv['notes'].items()}
        errs = {int(k): v for k, v in sv['errs'].items()}
        results.append((spec, model, notes, errs))
    open(os.path.join(ROOT, 'src', 'designs.cc'), 'w').write(emit(results))


if __name__ == '__main__':
    main()
