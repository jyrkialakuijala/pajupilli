#!/usr/bin/env python3
"""The synth leads' circuits against ngspice (research/synth_leads.md, section 6).

    tools/circuit.py parasitics
        Each parasitic alone (op-amp bandwidth, stray capacitance, a ceramic
        capacitor's voltage coefficient, leakage, wiring inductance, emitter
        resistance) against the clean core: pitch, ramp bend, harmonics.

    tools/circuit.py saw [-temp C] [-param NAME=VALUE ...]
        The sawtooth core: start-up from rest, then frequency against the
        control voltage (octaves around A4): tracking error per octave, the
        reset time, and the steady waveform's harmonics at A4.

The netlists are component level: a 2N3904 expo transistor, an op-amp
integrator, a comparator with hysteresis and a MOSFET reset switch. The
op-amps are macro-models (gain, a dominant pole, slew limit, rails): the
slew and the switch's on-resistance set the reset time, which bends the
pitch at the top of the range.
"""
import math
import os
import subprocess
import sys
import tempfile
from concurrent.futures import ThreadPoolExecutor

import numpy as np

VT25 = 0.025693  # kT/q at 25 C (V)

MODELS = """
.model Q2N3904 NPN(IS=6.734f XTI=3 EG=1.11 VAF=74.03 BF=416.4 NE=1.259 ISE=6.734f IKF=66.78m XTB=1.5
+ BR=.7371 NC=2 ISC=0 IKR=0 RC=1 CJC=3.638p MJC=.3085 VJC=.75 FC=.5 CJE=4.493p MJE=.2593 VJE=.75
+ TR=239.5n TF=301.2p ITF=.4 VTF=4 XTF=2 RB=10)
.model MSW NMOS(LEVEL=1 VTO=1.8 KP=0.05 LAMBDA=0.01 CGSO=20p CGDO=5p)
* Op-amp macro-model: a transconductance input stage limited to +-ISLEW (the
* slew rate is ISLEW / CC), a dominant-pole node that saturates near the
* rails (as a real second stage does: no wind-up), rail-limited output.
.subckt OPAMP inp inn out vcc vee PARAMS: GM=1m ISLEW=13u CC=1p ROUT=100
B1 0 n1 I = {ISLEW} * tanh({GM} / {ISLEW} * (V(inp) - V(inn)))
R1 n1 0 1G
C1 n1 0 {CC}
DH n1 hi DCLAMP
DL lo n1 DCLAMP
VH hi 0 13.0
VL lo 0 -13.0
.model DCLAMP D(IS=1e-12 N=1)
B2 n2 0 V = max(min(V(n1) * 1, V(vcc) - 1.5), V(vee) + 1.5)
R2 n2 out {ROUT}
.ends
"""


def saw_netlist(cv_oct, p, temp=25.0, tstop=None, f_nom=440.0):
    """The sawtooth core at cv_oct octaves from A4 (the CV as the expo emitter's voltage)."""
    c = p.get('C', 1e-9)
    vtop = p.get('VTOP', 5.0)
    # Emitter voltage for f_nom at 25 C, then -VT ln2 per octave (18 mV/oct).
    i0 = c * vtop * f_nom
    is_ = 6.734e-15
    vbe0 = VT25 * math.log(i0 / is_)
    ve = -(vbe0 + cv_oct * VT25 * math.log(2))
    f_guess = f_nom * 2 ** cv_oct
    # Schmitt thresholds: th = (saw R4 + cmp R3) / (R3 + R4) against VREF, the
    # comparator's output at +-13.5 V.
    vlow, vsw = p.get('VLOW', 0.3), 13.5
    r3 = 100e3
    k = (vtop - vlow) / (2 * vsw)          # R3 / R4 ... from the hysteresis width
    r4 = r3 / k
    vref = 0.5 * (vtop + vlow) * r4 / (r3 + r4)
    # The integrator's op-amp: gain-bandwidth GBW (default: very fast) with CC = 1 pF.
    gm = 2 * math.pi * p.get('GBW', 160e6) * 1e-12
    xc = p.get('XC', 0.0)
    cline = (f"C1 n saw C='{c}*(1 - {xc}*(V(n,saw)/5)*(V(n,saw)/5))'" if xc else f"C1 n saw {c}")
    tstop = tstop or max(0.03, 12 / f_guess)
    step = min(1 / (f_guess * 400), 2e-6)
    return f"""* sawtooth core, cv {cv_oct:+.2f} oct
.options temp={temp} tnom=25 reltol=1e-4 abstol=1e-12 vntol=1e-6 method=gear
{MODELS}
VCC vcc 0 15
VEE vee 0 -15
* The CV: the expo transistor's emitter (base and collector at 0 V), through
* its bulk emitter resistance RE.
VE ve 0 {ve:.6f}
RE ve ve2 {p.get('RE', 1e-3):.6g}
Q1 n 0 ve2 Q2N3904
* Integrator: the collector current pulled from the summing node charges C
* (XC: a ceramic capacitor losing XC of its value at 5 V, ~ V^2).
XU1 0 n saw vcc vee OPAMP PARAMS: GM={gm:.6g} ISLEW=13u CC=1p
{cline}
* Parasitics at the summing node: stray capacitance, leakage.
CP n 0 {p.get('CPAR', 1e-15):.6g}
IL n 0 {p.get('ILEAK', 0):.6g}
* Reset switch across C, driven by the comparator (LSW: the loop's wiring).
LSW saw sw {p.get('LSW', 1e-12):.6g}
M1 sw g n n MSW W=1 L=1
* Comparator with hysteresis (a non-inverting Schmitt trigger): high when
* the ramp passes VTOP, low again when it is back under VLOW.
XU2 th ref cmp vcc vee OPAMP PARAMS: GM=10m ISLEW=1m CC=1p
R3 saw th {r3:.6g}
R4 cmp th {r4:.6g}
VREF ref 0 {vref:.6f}
* Gate drive: the comparator's swing clipped to 0..12 V.
R5 cmp g 1k
D1 g dd DMOD
VDD dd 0 12
D2 0 g DMOD
.model DMOD D(IS=1n N=1.8)
.tran {step:.3g} {tstop:.4g} 0 {step:.3g} uic
.control
run
wrdata {{OUT}} v(saw)
quit
.endc
.end
"""


def run_spice(netlist):
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, 'out.txt')
        cir = os.path.join(d, 'c.cir')
        open(cir, 'w').write(netlist.replace('{OUT}', out))
        r = subprocess.run(['ngspice', '-b', cir], capture_output=True, text=True, cwd=d)
        if not os.path.exists(out):
            raise RuntimeError(r.stdout[-2000:] + r.stderr[-2000:])
        a = np.loadtxt(out)
    return a[:, 0], a[:, 1]


def measure_saw(t, v):
    """Resets (fast falls), the steady frequency, start-up, reset time, harmonics."""
    vmax = np.max(v)
    # A reset: v falls through mid-level.
    mid = 0.5 * vmax
    falls = np.where((v[:-1] > mid) & (v[1:] <= mid))[0]
    tf = t[falls] + (v[falls] - mid) / (v[falls] - v[falls + 1]) * (t[falls + 1] - t[falls])
    if len(tf) < 4:
        return None
    per = np.diff(tf)
    f = 1 / np.median(per[len(per) // 2:])
    # Start-up: the first reset (the ramp from rest reaching the threshold).
    startup = tf[0]
    # Reset time: from the peak to the trough around the last reset.
    k = falls[-2]
    w = (t > t[k] - 0.2 / f) & (t < t[k] + 0.2 / f)
    tt, vv = t[w], v[w]
    i_hi, i_lo = np.argmax(vv), np.argmin(vv)
    reset = tt[i_lo] - tt[i_hi] if i_lo > i_hi else float('nan')
    # Harmonics over the last 4 periods, resampled uniformly.
    t0, t1 = tf[-5], tf[-1]
    n = 4096
    tu = np.linspace(t0, t1, n, endpoint=False)
    vu = np.interp(tu, t, v)
    spec = np.abs(np.fft.rfft(vu - vu.mean())) / n
    h = [spec[4 * k] for k in range(1, 21)]
    return dict(f=f, startup=startup, reset=reset, vmax=vmax, h=h)


def saw(args):
    temp = 25.0
    p = {}
    i = 0
    while i < len(args):
        if args[i] == '-temp':
            temp = float(args[i + 1]); i += 2
        elif args[i] == '-param':
            k, v = args[i + 1].split('='); p[k] = float(v); i += 2
        else:
            i += 1
    octs = [-3, -2, -1, 0, 1, 2, 3]

    def one(o):
        t, v = run_spice(saw_netlist(o, p, temp))
        return o, measure_saw(t, v)
    with ThreadPoolExecutor(4) as ex:  # (a few at a time: the machine is shared)
        res = list(ex.map(one, octs))
    print(f"sawtooth core, {temp:g} C, params {p or 'default'}")
    print(" oct   target Hz    played Hz   cents   start-up ms   reset us   peak V")
    for o, m in res:
        ft = 440 * 2 ** o
        if not m:
            print(f"{o:+4d} {ft:10.1f}   (does not oscillate)")
            continue
        print(f"{o:+4d} {ft:10.1f} {m['f']:12.2f} {1200 * math.log2(m['f'] / ft):+7.1f} {1e3 * m['startup']:12.2f}"
              f" {1e6 * m['reset']:10.2f} {m['vmax']:8.2f}")
    m = dict(res)[0]
    if m:
        h1 = m['h'][0]
        print("A4 harmonics (dB re the fundamental; an ideal saw: -20 log10 n):")
        print('  ' + ' '.join(f"{20 * math.log10(x / h1 + 1e-12):+.1f}" for x in m['h'][:12]))
        print('  ' + ' '.join(f"{-20 * math.log10(n):+.1f}" for n in range(1, 13)))


def ramp_shape(t, v):
    """How far one full ramp bends from a straight line (share of the span),
    inside it (5 % of a period clear of each reset)."""
    vmax = np.max(v)
    mid = 0.5 * vmax
    falls = np.where((v[:-1] > mid) & (v[1:] <= mid))[0]
    if len(falls) < 3:
        return float('nan')
    ta, tb = t[falls[-3]], t[falls[-2]]
    per = tb - ta
    w = (t > ta + 0.05 * per) & (t < tb - 0.05 * per)
    tt, vv = t[w], v[w]
    k = np.polyfit(tt, vv, 1)
    return np.max(np.abs(vv - np.polyval(k, tt))) / (np.max(vv) - np.min(vv))


def parasitics(args):
    """Each parasitic alone against the clean core: pitch, harmonics, ramp shape."""
    variants = [('clean', {}),
                ('op-amp GBW 3 MHz (TL072)', {'GBW': 3e6}),
                ('stray C 20 pF at the summing node', {'CPAR': 20e-12, 'GBW': 3e6}),
                ('ceramic C: -10 % at 5 V', {'XC': 0.10}),
                ('leakage 2 nA', {'ILEAK': 2e-9}),
                ('reset loop 50 nH', {'LSW': 50e-9}),
                ('expo RE 2 ohm', {'RE': 2.0})]
    octs = [-3, 0, 3]
    jobs = [(name, pp, o) for name, pp in variants for o in octs]

    def one(j):
        name, pp, o = j
        t, v = run_spice(saw_netlist(o, pp))
        return name, o, measure_saw(t, v), ramp_shape(t, v)
    with ThreadPoolExecutor(4) as ex:
        res = list(ex.map(one, jobs))
    base = {o: m for name, o, m, _ in res if name == 'clean'}
    print('variant                              oct   cents vs clean   ramp bend %   harmonics 2..8 vs clean (dB)')
    for name, o, m, rb in res:
        if not m:
            print(f'{name:36s} {o:+d}   (no oscillation)')
            continue
        b = base[o]
        dc = 1200 * math.log2(m['f'] / b['f'])
        dh = [20 * math.log10((m['h'][k] / m['h'][0]) / (b['h'][k] / b['h'][0])) for k in range(1, 8)]
        print(f'{name:36s} {o:+d}   {dc:+8.2f}        {100 * rb:6.3f}      ' + ' '.join(f'{x:+.2f}' for x in dh))


if __name__ == '__main__':
    if len(sys.argv) > 1 and sys.argv[1] == 'saw':
        saw(sys.argv[2:])
    elif len(sys.argv) > 1 and sys.argv[1] == 'parasitics':
        parasitics(sys.argv[2:])
    else:
        print(__doc__)
