#!/usr/bin/env python3
"""A brush on a tom, as a small mechanism against the head's centre modes (research/toms.md 4.6).

The hand moves a target (down at v0; a tap turns up after 4 ms past the head, a dead stroke stops
4 mm into it); the handle (mh) follows it through the grip (a spring, or none: a relaxed throw); the
fan is a spring-damper (kf, zf) from the handle to the wire ends (mt), which meet the head through a
stiff Hunt-Crossley contact. The head: the two coupled (0,1)s (in phase: the fundamental), (0,2) and
(0,3), at its centre. Prints the contacts (start, length in ms), the rebound, the head's motion, and
how much of a head already ringing a stroke takes away.

    tools/brushbounce.py
"""
import math, sys
import numpy as np
from scipy.special import j1

def head(d_in, f_in, sigma=0.26, t60=(1.0, 0.8, 0.35, 0.25)):
    R = d_in * 0.0254 / 2
    M1 = sigma * math.pi * R * R  # x J1(j0n)^2 per mode
    j0 = [2.405, 5.520, 8.654]
    # (0,1) in-phase (both heads + air: about twice the mass), (0,1) out-of-phase (x1.08),
    # (0,2), (0,3) at the ideal ratios from the single head's (0,1) (= f_in / 0.9)
    f01 = f_in / 0.9
    modes = [(f_in, 2 * M1 * j1(j0[0]) ** 2, t60[0]), (1.08 * f01, 2 * M1 * j1(j0[0]) ** 2, t60[1]),
             (f01 * j0[1] / j0[0], M1 * j1(j0[1]) ** 2, t60[2]), (f01 * j0[2] / j0[0], M1 * j1(j0[2]) ** 2, t60[3])]
    return modes

def run(modes, v0=1.0, mh=0.02, kf=400.0, zf=0.3, mt=0.001, kc=2e6, grip=0.0, zg=0.5, hold=False,
        T=0.08, dt=2e-6, pre=None):
    """Hand: a target moving down at v0 then (tap) turning up after 8 ms, or (hold) stopping at the
    head; grip: the hand-handle spring (N/m; 0 = a free, relaxed throw). pre: the head's (0,1)
    velocity amplitude already ringing (m/s) and its phase."""
    n = len(modes)
    w = np.array([2 * math.pi * f for f, _, _ in modes]); M = np.array([m for _, m, _ in modes])
    g = np.array([6.91 / t for _, _, t in modes])  # amplitude decay rate (1/s)
    x = np.zeros(n); v = np.zeros(n)
    if pre:
        amp, ph = pre
        x[0] = amp / w[0] * math.sin(ph); v[0] = amp * math.cos(ph)
    gap = 2e-3
    yh = gap + 0.0; vh = -v0   # handle (above its rest length over the tips)
    yt = gap; vt = -v0          # tips plane
    L0 = 0.0
    cf = 2 * zf * math.sqrt(kf * mt)
    hand_y = yh; hand_v = -v0
    contacts = []; on = False; t0 = 0; maxpen = 0
    amp01 = 0; F_hist = []
    steps = int(T / dt)
    for i in range(steps):
        t = i * dt
        # hand target: moves down at v0 until it reaches the head (y 0) then hold or turn up
        if hold:
            hand_v = -v0 if hand_y > -0.004 else 0.0
        else:
            hand_v = -v0 if t < gap / v0 + 0.004 else v0
        hand_y += hand_v * dt
        y_head = float(np.sum(x))  # centre displacement (each shape 1 at centre)
        pen = y_head - yt
        Fc = 0.0
        if pen > 0:
            Fc = kc * pen ** 1.5 * (1 + 0.5 * (vt - 0) * 0) + 0.0
            Fc = max(0.0, kc * pen ** 1.5 * (1 - 0.3 * (vt - float(np.sum(v)))))  # Hunt-Crossley-ish
        # fan between handle and tips
        Ff = kf * (yh - yt - L0) + cf * (vh - vt)
        Fg = grip * (hand_y - yh) + (2 * zg * math.sqrt(grip * mh) if grip else 0) * (hand_v - vh)
        if grip == 0:  # a free throw: the handle follows the hand only until contact
            Fg = 0.0 if on or contacts else 1e4 * (hand_y - yh) + 50 * (hand_v - vh)
        at = (Ff + Fc) / mt
        ah = (Fg - Ff) / mh
        vt += at * dt; yt += vt * dt
        vh += ah * dt; yh += vh * dt
        a = -w * w * x - 2 * g * v - Fc / M
        v += a * dt; x += v * dt
        F_hist.append(Fc)
        if Fc > 0 and not on:
            on = True; t0 = t
        elif Fc == 0 and on:
            on = False; contacts.append((t0 * 1e3, (t - t0) * 1e3))
        amp01 = max(amp01, abs(x[0]))
    F = np.array(F_hist)
    imp = F.sum() * dt
    return contacts, vh, amp01, imp, F

def main():
    drums = {'12in jazz 165': head(12, 165), '14in 110': head(14, 110), '16in loose 73': head(16, 73)}
    print('free (relaxed) throw, 1 m/s, handle 20 g, fan 400 N/m, tips 1 g')
    for name, m in drums.items():
        c, vh, a, imp, _ = run(m)
        print(f'  {name:15s} contacts (ms at, ms long): {[(round(s,1), round(l,1)) for s, l in c][:6]}  '
              f'rebound {vh:+.2f} m/s (e {vh/1.0:.2f})  (0,1) peak {a*1e3:.3f} mm  impulse {imp*1e3:.1f} mNs')
    print('stick for comparison: 40 g rigid-ish (fan 1e5 N/m, tips 5 g)')
    for name, m in drums.items():
        c, vh, a, imp, _ = run(m, mh=0.035, kf=2e5, mt=0.005, zf=0.05)
        print(f'  {name:15s} contacts: {[(round(s,1), round(l,1)) for s, l in c][:6]}  rebound {vh:+.2f}  '
              f'(0,1) peak {a*1e3:.3f} mm  impulse {imp*1e3:.1f} mNs')
    print('fan stiffness and handle mass (14in, free throw)')
    for kf in (150, 400, 1000):
        for mh in (0.01, 0.03):
            c, vh, a, imp, _ = run(drums['14in 110'], kf=kf, mh=mh)
            print(f'  kf {kf:5d} mh {mh*1e3:3.0f} g: contacts {[(round(s,1), round(l,1)) for s, l in c][:5]} rebound {vh:+.2f}  (0,1) {a*1e3:.3f} mm')
    print('firm grip (2000 N/m) vs held down (slap / dead stroke), 14in')
    for lab, kw in (('firm grip', dict(grip=2000)), ('held down', dict(grip=2000, hold=True))):
        c, vh, a, imp, _ = run(drums['14in 110'], **kw)
        print(f'  {lab:10s} contacts {[(round(s,1), round(l,1)) for s, l in c][:6]}  (0,1) {a*1e3:.3f} mm')
    print('onto a ringing head (14in, (0,1) at 0.1 m/s), by phase')
    for ph in (0, math.pi / 2, math.pi, 1.5 * math.pi):
        c, vh, a, imp, _ = run(drums['14in 110'], pre=(0.1, ph))
        print(f'  phase {ph:4.2f}: contacts {[(round(s,1), round(l,1)) for s, l in c][:6]} rebound {vh:+.2f}')


def _energy01(m, x, v):
    w = 2*math.pi*m[0][0]; M = m[0][1]
    return 0.5*M*(v*v + w*w*x*x)

# patch run to also return the head state over time
def run2(modes, **kw):
    import types
    n = len(modes)
    T = kw.pop('T', 0.08); dt = 2e-6
    v0 = kw.get('v0', 1.0); mh = kw.get('mh', 0.02); kf = kw.get('kf', 400.0); zf = kw.get('zf', 0.3)
    mt = kw.get('mt', 0.001); kc = 2e6; grip = kw.get('grip', 0.0); hold = kw.get('hold', False); pre = kw.get('pre')
    w = np.array([2*math.pi*f for f,_,_ in modes]); M = np.array([m for _,m,_ in modes]); g = np.array([6.91/t for _,_,t in modes])
    x = np.zeros(n); v = np.zeros(n)
    if pre: x[0] = pre[0]/w[0]*math.sin(pre[1]); v[0] = pre[0]*math.cos(pre[1])
    gap = 2e-3; yh = gap; vh = -v0; yt = gap; vt = -v0; hand_y = yh
    cf = 2*zf*math.sqrt(kf*mt); on = False; contacts = []; t0 = 0; Ehist = []; started = False
    for i in range(int(T/dt)):
        t = i*dt
        hand_v = (-v0 if hand_y > -0.004 else 0.0) if hold else (-v0 if t < gap/v0 + kw.get('turn', 0.004) else kw.get('up', v0))
        hand_y += hand_v*dt
        yhd = float(np.sum(x)); pen = yhd - yt; Fc = 0.0
        if pen > 0: Fc = max(0.0, kc*pen**1.5*(1 - 0.3*(vt - float(np.sum(v)))))
        Ff = kf*(yh - yt) + cf*(vh - vt)
        if grip: Fg = grip*(hand_y - yh) + 2*0.5*math.sqrt(grip*mh)*(hand_v - vh)
        else: Fg = 0.0 if (on or contacts) else 1e4*(hand_y - yh) + 50*(hand_v - vh)
        vt += (Ff + Fc)/mt*dt; yt += vt*dt; vh += (Fg - Ff)/mh*dt; yh += vh*dt
        v += (-w*w*x - 2*g*v - Fc/M)*dt; x += v*dt
        if Fc > 0 and not on: on = True; t0 = t
        elif Fc == 0 and on: on = False; contacts.append((round(t0*1e3,1), round((t-t0)*1e3,1)))
        if i % 50 == 0: Ehist.append((t, 0.5*M[0]*(v[0]**2 + w[0]**2*x[0]**2)))
    return contacts, vh, Ehist


def ring():
    m14 = head(14, 110); m12 = head(12, 165); m16 = head(16, 73)
    print('fan loss (zf) -> rebound, 14in, free throw 1 m/s')
    for zf in (0.3, 0.8, 1.5, 3.0):
        c, vh, E = run2(m14, zf=zf)
        print(f'  zf {zf:3.1f}: rebound e {vh:+.2f}, contacts {c[:6]}')
    print('a stick hit already ringing (0,1) at 0.5 m/s; then a brush; (0,1) energy just before vs 30 ms after the brush leaves, vs no brush')
    for name, m in (('12in', m12), ('14in', m14), ('16in', m16)):
        for lab, kw in (('tap, free', dict(zf=1.5)), ('dead stroke (held)', dict(zf=1.5, grip=2000, hold=True))):
            for ph in (0.0, math.pi):
                c, vh, E = run2(m, pre=(0.5, ph), T=0.06, **kw)
                c0, _, E0 = run2(m, pre=(0.5, ph), T=0.06, v0=1e-9, zf=1.5)  # brush never arrives
                e_end = E[-1][1]; e0_end = E0[-1][1]
                print(f'  {name} {lab:20s} phase {ph:.1f}: (0,1) energy at 60 ms vs no brush {10*math.log10(e_end/e0_end):+5.1f} dB  contacts {c[:4]}')


def pullout():
    """The tom stroke as played (research/toms.md 4.5): the hand lifts as the brush lands (a firm
    wrist, turning up at contact); the brush springs off, the head rings on. Per velocity: the
    contacts, and the (0,1) left ringing at 60 ms against a free head struck the same way."""
    print('pull-out strokes (grip 2000 N/m, the hand turning up as the brush lands)')
    for name, m in (('12in', head(12, 165)), ('14in', head(14, 110)), ('16in', head(16, 73))):
        for v0 in (0.2, 0.5, 1.0, 2.0):
            c, vh, E = run2(m, v0=v0, zf=1.5, grip=2000, turn=0.0, up=1.0, T=0.06)
            tot = sum(l for _, l in c)
            print(f'  {name} {v0:3.1f} m/s: contacts {c[:4]}  in contact {tot:4.1f} ms  rebound {vh:+.2f} m/s  '
                  f'(0,1) energy at 60 ms {10 * math.log10(E[-1][1] + 1e-30):6.1f} dB')


if __name__ == '__main__':
    main()
    ring()
    pullout()
