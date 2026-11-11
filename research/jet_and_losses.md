# Jet drive, player parameters, attacks, bore losses and radiation — research notes

Status: 2026-11-11. SI units. **[G]** marks my estimates or guesses, not published values.
**[D]** marks values I derived from published formulas. References are numbered; the list is at the end.

---

## 1. Jet-drive excitation model (time domain)

### 1.1 Equation chain (Verge/Fabre/de la Cuadra form, as used in [1][2])

| # | Quantity | Equation | Values / notes |
|---|---|---|---|
| a | Jet centre velocity | `Uj = sqrt(2 Pm / ρ)` (Bernoulli, steady) | ρ ≈ 1.2 (1.16 for warm breath at 30 °C). Pm = 200–1700 Pa → Uj ≈ 18–53 m/s (§2). Fletcher [3] fits his data better with an effective speed of ≈ 0.4–0.6·Bernoulli (jet slows through entrainment). Fold that into W or the θ calibration; Uj itself stays Bernoulli. |
| b | Unsteady jet velocity (optional) | `ρ (ℓc) dUj/dt = Pm − ½ ρ Uj²` | ℓc is the "channel" length. For lips it is a few mm **[G: 2–5 mm]**. The term limits how fast the jet can grow (it matters in attacks, [9]). |
| c | Convection (phase) velocity | `cp ≈ 0.4 Uj`. Range 0.3–0.5 Uj | [1][2][4]. de la Cuadra's visualisation gives cp/Uj ≈ 0.4 for a laminar jet and ≈ 0.3 for a turbulent one [4]. |
| d | Convective delay | `τ = W / cp ≈ W / (0.4 Uj)` | C4: W = 8 mm, Uj = 18.7 → τ ≈ 1.07 ms ≈ 51 samples. C7: ≈ 0.19 ms ≈ 9 samples [D]. |
| e | Initial perturbation at the flue/lip exit | `η0(t) = (h / Uj) · v_ac(t)` | Empirical fit by de la Cuadra [1][2]. v_ac is the transverse acoustic velocity in the window. |
| f | Deflection at the labium | `η(W,t) = (h / Uj) · v_ac(t − τ) · exp(αi W)` | [2] eq. 4 |
| g | Spatial growth rate | `αi ≈ 0.4 / h` [2]. `αi ≈ 0.3 / h` [1] (antisymmetric mode) | For a flute W/h ≈ 5–10, so exp(αi W) ≈ e^2…e^4 ≈ 7–55. The exponential holds only for a few h. Clamp the gain or let tanh saturate (§1.3). Rayleigh theory: αi depends on the Strouhal number St = ωb/Uj, with a maximum near St ≈ 0.25 [1]. A constant αi = 0.3–0.4/h is the standard simplification. |
| h | Bickley profile | `U(y) = Uj sech²((y − η)/b)`, `b = 2h/5` | [1][2] |
| i | Flow split (y axis pointing out of the pipe, labium at y0 from the jet axis) | `Qin = H ∫_{−∞}^{y0−η} U dy = H b Uj [1 + tanh((y0 − η)/b)]` | H is the jet/window width. Recorder model: y0 = 0.1 mm [1]. Flautas de chinos: 0.2 mm [2]. y0 sets the even/odd harmonic balance. y0 = 0 gives a symmetric split and weak even harmonics. |
| j | Dipole source distance | `δd = (4/π) · sqrt(2 h W)` | Verge [2][5]. h = 1 mm, W = 10 mm → 5.7 mm. "A sensitive parameter" [1]. |
| k | Source pressure across the window | `Δp_src = −(ρ δd / (W H)) dQin/dt = +(ρ δd b Uj / W) · d/dt tanh((η − y0)/b)` | [2] eqs. 6–9. Here W·H stands for the window area. For a flute use the uncovered embouchure-window area **[G]**. |
| l | Nonlinear (vortex-shedding) loss | `Δp_los = −(ρ/2) · v_ac·abs(v_ac) / αvc²`, `αvc ≈ 0.6` | [1][2]. v_ac is the window velocity, so Δp_los = −ρ Q·abs(Q)/(2 αvc² S_w²). |
| m | Total | `Δp = Δp_src + Δp_los` | Acts as a series pressure source in the window branch. |
| n | Symmetric mode (optional, vocal-tract coupling) | thickness perturbation `b' = b0 e^{αi(s) x}`, `cp(s) = 0.8 Uj`, `αi(s) = 0.2/h`, `b0/h = σ u'/Uj`, σ ∈ 0…1 | [1]. Skip it unless the mouth cavity is modelled. |

**Published parameter sets.** You can copy these directly (both are recorder-like, not transverse flute).

| Source | W | H | h | b | y0 | αi | cp | δd | αvc | ρ |
|---|---|---|---|---|---|---|---|---|---|---|
| Auvray, Ernoult, Terrien, Lagrée, Fabre, Vergez 2016 [1] (alto recorder) | 4 mm | 12 mm | 1 mm | 2h/5 | 0.1 mm | 0.3/h | 0.4 Uj | (4/π)√(2hW) | 0.6 | 1.2 |
| Terrien, Vergez, de la Cuadra, Fabre 2013 [2] (flauta de chinos) | 10 mm | 6 mm | 1 mm | 0.4 mm | 0.2 mm | 0.4/h = 400 m⁻¹ | 0.4 Uj | 5.7 mm | 0.6 | 1.2 |

For the transverse flute **[G, from §2]**: W = 4–8 mm (per note), H = 6–12 mm (the lip-opening width, not the
embouchure length), h = 0.4–1.0 mm, y0 = 0–0.3 mm. Start from y0 = 0.1 mm and αi = 0.35/h.

### 1.2 Sign conventions

The papers do not agree on signs. In Terrien/Verge [2], y points out of the pipe and Qin is the part of the jet
under the labium (inside). So η > 0 means "jet outward", and dQin/dt = −H Uj sech²(·) dη/dt.
In [2], v_ac and Δp are related by the pipe input admittance Y = V_ac/ΔP. For the loop to close, a positive
source pressure must drive flow the same way that a positive η0 is measured.

**Recommendation.** Write the window branch with Q_w > 0 = flow from the bore out through the window. Compute η from
v_w = Q_w/S_w. Then **verify the sign empirically.** With the right sign, the first register starts at
θ = W f1 / Uj ≈ 0.25 and keeps sounding down to θ ≈ 0.07 (recorder data, §2.3). The frequency also rises slightly with Uj.
With the wrong sign, the oscillation appears at about 2× (or ½×) the expected θ, or not at all. Expose a ±1 sign
constant `kJetSign` in the code.

### 1.3 Window / embouchure branch and stable discretisation

The embouchure hole is a side branch at grid point i_e. Its volume flow Q_w (positive outward) obeys

```
M_w dQ_w/dt = p_bore[i_e] − p_rad(Q_w) − s·K_j · dF/dt − (ρ / (2 αvc² S_w²)) · Q_w·abs(Q_w)
F(t)   = tanh((η(t) − y0)/b),  η(t) = (h/Uj) e^{αi W} · Q_w(t−τ)/S_w  (+ noise, §1.5)
K_j    = ρ δd b Uj / W_area_norm    (Pa·s),  s = ±1 (sign, §1.2)
M_w    = ρ ℓ_e / S_w,   S_w = (1 − coverage) · S_emb,   ℓ_e = t_ch + t_i + t_o
```

Embouchure geometry **[G / typical]**: S_emb ≈ 10 × 12 mm. Chimney t_ch ≈ 4–5 mm (wall plus lip plate).
Inner end correction t_i ≈ 0.8·r_e (Dalmont/Nederveen formula, §4.4). Outer end correction t_o ≈ 0.6–0.85·r_e,
where r_e = √(S_w/π). The lip and face partly flange the opening, and coverage increases t_o.
Coltman measured the uncovered embouchure width as 6 mm (C4) down to 3.5 mm (C7), so S_w shrinks by about 2× over the range [3].
The embouchure inertance strongly affects intonation. Treat ℓ_e as the per-note **tuning knob** (players do this by rolling the flute).

**Discretisation tricks (48 kHz)**
1. **Do not differentiate tanh numerically.** Move the source term into the state:
   `Φ = M_w Q_w + s K_j F`, so `dΦ/dt = p_bore − p_rad − R_nl(Q_w) Q_w`. Then `Q_w = (Φ − s K_j F)/M_w`.
   F uses only *delayed* Q_w (τ ≥ 9 samples), so everything stays explicit and needs no Newton step. When K_j changes with Uj,
   this gives a small transient. It is acceptable **[G]**, or ramp Uj with a smoother of ≥ 1 ms.
2. **Nonlinear loss, semi-implicit and branch-free:**
   with `c_nl = ρ/(2 αvc² S_w²)`, evaluate the loss as `c_nl·abs(Q^n)·Q^{n+1}` (linearised around abs(Q^n)), giving
   `Q^{n+1} = (M_w Q^n + dt·(p_bore − p_rad) − s K_j (F^{n+1} − F^n)) / (M_w + dt·c_nl·abs(Q^n))`.
   This is unconditionally dissipative and equivalent to the Φ form of item 1.
3. **Delay τ:** keep a ring buffer of η0 (or v_w), ≥ 256 samples. Read it with linear or cubic-Lagrange interpolation at
   τ·fs, which is 9–55 samples. τ varies with Uj, so smooth Uj (one-pole, ~2–5 ms) to avoid zipper/Doppler artefacts.
4. **Gain clamp:** exp(αi W) can reach 50+. tanh saturates at |η − y0| ≳ 2b, which bounds the jet. Add no other clamps.
5. Connect the branch to the leapfrog the same way as the tone holes: Q_w leaves the continuity equation at i_e
   (`p[i_e] −= dt ρc²/(S dx) · Q_w`). Q_w is updated on the velocity half-step from p[i_e].
6. The DC jet flow H b Uj (1 + tanh(y0/b)) entering the bore can be ignored. If you inject it, DC-block the output (R13).

### 1.4 Radiation from the embouchure
The embouchure is an open radiating end. Model p_rad with the parallel-RL radiation load of §4.5
(R ≈ 1.35–1.5 ρc/S_w, L = ρ t_o/S_w). Its flow derivative dQ_w/dt goes into the output sum (R13).

### 1.5 Turbulence / breath noise
The literature has no settled model. These are common practices:
- **Into the jet (recommended).** Add band-limited noise to the transverse velocity driving η0:
  `v_eff = v_w + σ_n · Uj · n(t)`, with σ_n ≈ 0.005–0.03 **[G]**. The noise then passes through the jet delay, the e^{αi W} gain
  and the tanh, so it comes out shaped by the jet and the bore (shaped breath noise, as in real flutes).
  Turbulent jets (Re = Uj h/ν ≳ 2000; flutes reach Re up to 10⁴ [4]) carry velocity fluctuations of several % of Uj.
- **Into the source.** Add a pressure noise at the window, ∝ ρ Uj² · σ_p · n(t), with σ_p ≈ 1e-3–1e-2 **[G]**.
  This gives the "airy" hiss heard outside the bore.
- **Into the breath pressure.** STK Flute uses `breath = P(1 + 0.15·noise)` (noiseGain = 0.15, vibratoGain = 0.05) [6]. It is crude but robust.
- **Spectrum:** jet turbulence is broadband, with energy up to about St = f h/Uj ≈ 0.1–0.3, i.e. 2–10 kHz for flute jets **[G]**.
  Use white noise through a 1st/2nd-order band-pass, 500 Hz–8 kHz, with its centre scaled by Uj/h.
  Chafe's vortex-noise idea (amplitude-modulate the noise by the instantaneous flow to the labium, e.g. by sech²((η−y0)/b))
  gives pitch-synchronous "chiff" [G; Chafe, ICMC 1995].
- Scale the noise level with Uj (velocity noise) or Uj² (pressure noise). Players control airiness with lip shape, so
  also expose a separate noise parameter (R10).

---

## 2. Player parameters (real flutists)

### 2.1 Measured relations (Fletcher 1975, 4 players [3]; consistent with Coltman 1966/68)
- Blowing pressure: **p ≈ 0.008·f mbar = 0.8·f Pa** (f in Hz). The maximum observed deviation is mostly within ±50%.
  Coltman's values sit at the low edge. Bouhuys's (750–1300 Pa for C4) are considered too high.
- Jet length (lip to edge): **l ≈ 1.8 + 100/√f mm**, all within ±20%. Players change W little with dynamics.
  The first 1.8 mm acts as a "dead length".
- Pianissimo→fortissimo: 2 players kept p constant and 2 changed p by about 2×. **All players enlarge the lip opening (jet
  cross-section) for forte.**
- Lip opening: an oval with axial ratio 10:1–20:1. Width ≈ 12 mm (= embouchure width) for loud low notes,
  ≈ 5 mm for soft high notes. So h ≈ 0.6–1.2 mm (loud, low) and 0.25–0.5 mm (soft, high) [D].
- Coverage: the uncovered embouchure width runs from 6 mm (C4) to 3.5 mm (C7) (Coltman, quoted in [3]).
- Vibrato: ≈ 5 Hz with ±10% blowing pressure. Mostly an amplitude/timbre modulation of upper partials; FM < 1% [3].
- Fletcher's own theory gives `l·p^(-1/2) ≈ const/f`, i.e. **θ = W f/Uj ≈ constant**.
- Novice players show longer pressure rise times and different W/h/p than experts (de la Cuadra et al., cited in [1]).

### 2.2 Per-note starting values [D from 2.1; ±30% is normal]

| Note (sounding) | f (Hz) | p (Pa) | W (mm) | Uj (m/s) | θ = W f/Uj | θ with W−1.8 mm | τ = W/0.4Uj (samples @48k) |
|---|---|---|---|---|---|---|---|
| C4 | 262 | 210 | 8.0 | 18.7 | 0.112 | 0.087 | 51 |
| G4 | 392 | 314 | 6.9 | 22.9 | 0.117 | 0.087 | 36 |
| C5 | 523 | 419 | 6.2 | 26.4 | 0.122 | 0.087 | 28 |
| G5 | 784 | 627 | 5.4 | 32.3 | 0.130 | 0.087 | 20 |
| C6 | 1047 | 837 | 4.9 | 37.4 | 0.137 | 0.087 | 16 |
| G6 | 1568 | 1254 | 4.3 | 45.7 | 0.148 | 0.087 | 11 |
| C7 | 2093 | 1674 | 4.0 | 52.8 | 0.158 | 0.087 | 9 |

Soft playing: p ×0.5–1.0, h and H smaller. Loud playing: p ×1–1.5, larger h·H. A 2nd-register note uses
the low-octave fingering at about 2× the pressure and a shorter W: C5 from C4 fingering has θ(f1) = 6.2·262/26.4·1e-3 ≈ 0.061,
which is below the regime-1 limit, so the flute jumps to regime 2 (§2.3).

### 2.3 Register thresholds and hysteresis (from recorder data — flute-specific numbers not found)
- Auvray et al. 2016 [1] (alto-recorder resonator, W = 4 mm, f1 ≈ 564 Hz): regime 1→2 at 695 Pa (Uj = 34 m/s, **θ1 = W f1/Uj ≈ 0.066**).
  The return 2→1 is at 534 Pa (Uj = 29.8 m/s, **θ1 ≈ 0.076**). Hysteresis ≈ 30% in pressure, ≈ 15% in Uj [D].
- Auvray, Fabre, Lagrée 2012 [7]: the linear theory gives frequency vs Uj well. The jet-drive model predicts the regime-1 onset
  threshold poorly but the **hysteresis of the higher-regime thresholds accurately**.
- Typical sounding window [D]: regime n sounds while θ_n = W f_n/Uj stays within ≈ 0.07–0.25. The "centre" is
  ≈ 0.1–0.15 (flutists sit at 0.11–0.16 by Bernoulli Uj, table 2.2). Fletcher [3] notes players blow "just short of
  overblowing" to enrich the harmonics.
- Overblowing to register 3 (12th) or 4 from low fingerings: θ1 ≈ 0.04 / 0.03 [D]. In practice the 3rd octave uses
  cross fingerings, so θ relative to the sounding f stays near 0.12–0.16.
- Edge-tone/jet-only frequency rises ∝ Uj/W, so faster jets select higher resonances [8].

---

## 3. Attack transients / tonguing

| Item | Value | Source |
|---|---|---|
| Organ pipe (flute-like) attack, three pallet speeds | transient duration 160 / 45 / 26 ms; static threshold (125 Pa) crossed at 40 / 9 / 4 ms | Castellengo, ISMA 2004 [9] |
| Fast rise | mouth-tone/edge-tone components and higher regimes appear in the transient; "higher frequencies can be produced when high pressures are achieved in a very short time, as during tonguing" | [9] |
| Flute model studies | transient onset ≈ 10–30 ms; novices use longer mouth-pressure rise times than experts; for the shortest rises jet growth is limited by channel inertia | French flute-model study (CFA 2018 / de la Cuadra, Ernoult) via search summary [10] — not verified in full text |
| Tongued attack, flute **[G]** | mouth pressure 10–90% rise ≈ 5–20 ms (tongue release); slight overshoot ≈ 10–30% for an accented attack, decaying over 30–50 ms | estimate consistent with [9][10] and clarinet tonguing data (UNSW) |
| STK Flute | ADSR attack 5 ms, decay 10 ms | [6] |

**Modelling.** Gate Pm with a shaped ramp: raised-cosine or exponential, τ ≈ 3–15 ms, optional overshoot.
Also apply the jet inertia equation (1.1b), so Uj lags Pm by ~1 ms. While Uj is small at the start, θ is large, so
the jet first excites mouth tones and higher partials ("chiff"). This happens naturally if Uj is ramped. Add a noise burst
(about 2× the breath-noise level for 10–30 ms **[G]**) to mimic the turbulent release of the tongue ("t"/"d" consonants).
Slurs change only the fingering (R12). Hysteresis (§2.3) keeps the register.

---

## 4. Bore losses and radiation

### 4.1 Viscothermal attenuation (Keefe 1984 [11]; Benade)
r_v = a·sqrt(ωρ/μ), μ = 1.85e-5 Pa·s (1.87e-5 at 30 °C), Pr = 0.71, γ = 1.4.
```
α(ω)    = (ω/c) · (1.045/r_v + 1.080/r_v² + 0.750/r_v³)          [Np/m]
v_ph/c  ≈ 1 − 1.045/r_v − …        (1.045 = (1 + (γ−1)/√Pr)/√2)
Short form: α ≈ 3.0e-5 · √f / a     (a in m)                    [Benade / Fletcher & Rossing]
```
Series (viscous) and shunt (thermal) parts per unit length, leading order:
```
Z = jωρ/S · [1 + √2(1−j)/r_v],       Y = jωS/(ρc²) · [1 + (γ−1)√2(1−j)/(√Pr r_v)]
```
The viscous term is ≈ 68% of α and the thermal term ≈ 32%.

**a = 9.5 mm (19 mm bore), computed [D]:**

| f (Hz) | 100 | 262 | 523 | 1000 | 2000 | 4000 | 8000 |
|---|---|---|---|---|---|---|---|
| r_v | 61 | 98 | 139 | 192 | 271 | 384 | 542 |
| α (Np/m) | 0.032 | 0.051 | 0.072 | 0.099 | 0.140 | 0.198 | 0.280 |
| wall-loss-only Q = k/2α | 29 | 46 | 66 | 91 | 129 | 183 | 259 |
| phase-velocity drop | 1.7% | 1.06% | 0.75% | 0.54% | 0.39% | 0.27% | 0.19% |

**Tuning impact:** the dispersion lowers C4-region resonances by ≈ 18 cents and C6 by ≈ 9 cents relative to a lossless bore.
A loss model that includes only the resistive part will mistune by that amount. Either use a model that includes
the reactive part (4.2 B) or retune with ℓ_e.
Per grid cell (dx = 7.3 mm): α·dx at 1 kHz ≈ 7e-4 Np, a 0.07% amplitude loss per cell.

### 4.2 Time-domain options for the leapfrog (branch-free, per-point coefficients)

| Option | Description | Cost/point | Accuracy | Verdict |
|---|---|---|---|---|
| A. Constant R, G | `ρ/S ∂u/∂t = −∂p/∂x − R u`, `S/(ρc²) ∂p/∂t = −∂u/∂x − G p`. Trapezoidal (centered) discretisation → passive | 1 mul each | α ∝ const: correct at one frequency only. Over-damps high harmonics if fitted at f1, and vice versa | Starter only |
| B. **Diffusive (fractional) representation, Bilbao & Harrison 2016 [12]; Bilbao, Harrison, Kergomard, Lombard, Vergez 2015 [13]** | Series impedance √(jω) part ≈ R0 + Σ_k (R_k jω)/(jω + ω_k) (an RL/RC ladder); shunt likewise. Each section is one extra state per point, updated with trapezoidal rule → **strictly passive, explicit, no branches**. 2–4 sections fitted over 50 Hz–10 kHz give α within a few % and the dispersion too | +2–4 states, +2 FMAs each | Matches √f attenuation **and** the phase-velocity correction | **Recommended** |
| C. Lumped loss filter at end(s) (waveguide style, e.g. Välimäki/Smith loop filter) | one low-order IIR per reflection | cheap | Wrong once tone holes split the bore. Not natural in FDTD | No |
| D. Single first-order section + R0 | Option B with K = 1 | +1 state | ±20% α over 200 Hz–5 kHz **[G]** | Good cheap compromise |

Implementation sketch for B (series side, per point i, SIMD over i):
```
// u update with loss:  (ρ/S)(u⁺−u⁻)/dt = −δx p − R0 ū − Σ_k ψ_k ,   ū = (u⁺+u⁻)/2
// each section k:  ψ_k is the pressure drop of an R_k ‖ L_k branch, dψ_k/dt = −ω_k ψ_k + R_k du/dt
// trapezoid: ψ_k⁺ = a_k ψ_k⁻ + g_k (u⁺ − u⁻),  a_k = (1−ω_k dt/2)/(1+ω_k dt/2), g_k = R_k/(1+ω_k dt/2)
// substitute → u⁺ solved in closed form: one divide-free update with precomputed per-point coeffs.
```
Area variation (Webster) enters through S_i and a_i = √(S_i/π): R0, R_k ∝ 1/(S a) and the shunt terms ∝ S/a.
Precompute them per point. Fit (R_k, ω_k) once for a = 1 m and scale them.
To avoid two sets of states, lump thermal losses into the series path by scaling R by ≈ 1/0.68 ≈ 1.47
(attenuation then remains correct; the dispersion error is small) **[G: acceptable simplification]**.

### 4.3 Unflanged open end (Levine–Schwinger; Silva et al. 2009 [14])
- End correction δ = 0.6133 a (low ka). |R| ≈ 1 − (ka)²/2.
- Z_r/Z_c ≈ (ka)²/4 + j 0.6133 ka. Flanged: (ka)²/2 + j 0.8216 ka.
- Causal rational form (unflanged, valid to ka ≈ 2) [14]:
  `R(ω) = −(1 − 0.167 jka) / (1 − 1.393 jka + 0.457 (jka)²)`,
  `Z_r = (1.226 jka + 0.457 (ka)²) / (2 − 1.560 jka − 0.457 (ka)²)`.
- At the foot: the planned 40-point spherical horn (R3) provides radiation intrinsically. The end correction of
  ≈ 0.61a ≈ 5.8 mm is then not guaranteed. Check the reflection of the joined bore/horn against the formula above.
  If it differs, put a lumped parallel RL at the foot instead **[G]**.

### 4.4 Open and closed tone holes (lumped branch at nearest grid point)
Hole radius b, bore radius a, δ = b/a, chimney t. Main references: Dalmont et al. 2002; Lefebvre & Scavone 2012 [15]; Nederveen.
- Series inertance: `M_h = ρ t_e / S_h`, with `t_e = t + t_i + t_o`. Inner correction
  `t_i ≈ b (0.82 − 0.193δ − 1.09δ² + 1.27δ³ − 0.71δ⁴)` (Dalmont/Nederveen).
  Outer correction: unflanged 0.61b; flanged 0.82b. A hole on a pipe wall is ≈ 0.7b. **With a pad at height H_p above the
  hole, t_o grows**: use Lefebvre & Scavone's [15] fit, or ≈ 0.7b·(1 + 0.3 b/H_p) **[G]**.
- The small negative series inertance (t_a ≈ −0.3…−0.4 b δ²) can be neglected at first.
- Closed hole: compliance `C_h = S_h t / (ρc²)` added to the local cell volume (R7).
- Openness o ∈ 0…1 (R8): interpolate the effective t_o (pad height) and S_h·o. Use 1/M_h → 0 when closed, so code needs no branches.
- Radiation resistance: parallel R_r ‖ L_o with `L_o = ρ t_o/S_h` and `R_r = ρ c δ_o² /(S_h β b²)`.
  This gives R_r ≈ 1.5 ρc/S_h (unflanged values) or 1.35 ρc/S_h (flanged) [D: from matching Re Z = β(kb)² ρc/S at low f].
  It is branch-free: one extra state (flow through L_o) per hole. Trapezoid discretisation keeps it passive.
- Nonlinear hole loss at high amplitude: `Δp = ρ/(2 C²) v·abs(v)`, C ≈ 0.6–0.8 [G; Dalmont 2002 "linear and nonlinear behaviour"]. Same semi-implicit trick as §1.3.

---

## 5. Parameter tables to start from

| Model | What to copy | Ref |
|---|---|---|
| Auvray et al. 2016 recorder | full jet table (§1.1), modal admittance ω1 = 3547 rad/s, Q ≈ 1/ε ≈ 25–35, thresholds 695/534 Pa | [1] |
| Terrien et al. 2013 | jet table (§1.1), Uj range 30–40 m/s, both time-domain and continuation | [2] |
| Fletcher 1975 | player p(f), W(f), lip ellipse, vibrato | [3] |
| STK Flute (Cook) | jetRatio 0.32 (jet delay = 0.32 × bore delay; usable range 0.08–0.56), jet nonlinearity x − x³, jetReflection 0.5, endReflection 0.5, noiseGain 0.15, vibratoGain 0.05, loop LPF pole 0.7 − 0.1·22050/fs, ADSR 5 ms / 10 ms | [6] |
| Bilbao & Harrison 2016 | passive FDTD viscothermal scheme for variable-area tubes | [12] |
| Silva et al. 2009 | radiation rational fits | [14] |

**Suggested initial set for pajupilli [G, combining the above]:** ρ = 1.2, c = 347, αi = 0.35/h, cp = 0.4 Uj,
b = 0.4 h, h = 0.8 mm (mf), H = 9 mm, y0 = 0.1 mm, δd = (4/π)√(2hW), αvc = 0.6, W and p from table 2.2,
embouchure S_w = (1 − 0.35)·1.2e-4 m², ℓ_e ≈ 10–14 mm (tune), Uj slew 3 ms, noise σ_n = 0.01, attack ramp 8 ms.

---

## References
1. R. Auvray, A. Ernoult, S. Terrien, P.-Y. Lagrée, B. Fabre, C. Vergez, "Effect of changing the vocal tract shape on the sound production of the recorder: an experimental and theoretical study", arXiv:1601.05545 (2016) (modified jet-drive model, Table 1). https://arxiv.org/pdf/1601.05545
2. S. Terrien, C. Vergez, P. de la Cuadra, B. Fabre, "Is the jet-drive flute model able to produce modulated sonorities like flautas de chinos?", SMAC 2013, arXiv:1402.3143 (eqs. 1–10, Table 2). https://arxiv.org/pdf/1402.3143
3. N. H. Fletcher, "Acoustical correlates of flute performance technique", JASA 57(1), 233–237 (1975). https://openresearch-repository.anu.edu.au/bitstream/1885/125582/1/Acoustical%20correlates%20of%20flute%20performance%20technique.pdf
4. P. de la Cuadra, C. Vergez, B. Fabre, "Visualization and analysis of jet oscillation under transverse acoustic perturbation", J. Flow Vis. Image Proc. 2007; arXiv:0811.0333. https://ar5iv.arxiv.org/html/0811.0333
5. M.-P. Verge, A. Hirschberg, R. Caussé, "Sound production in recorderlike instruments II: a simulation model", JASA 101(5) 1997; M.-P. Verge, PhD thesis, TU Eindhoven 1995 (δd formula).
6. P. Cook / G. Scavone, STK `Flute.cpp`. https://github.com/thestk/stk/blob/master/src/Flute.cpp
7. R. Auvray, B. Fabre, P.-Y. Lagrée, "Regime change and oscillation thresholds in recorder-like instruments", JASA 131(2), 1574–1585 (2012), doi:10.1121/1.3672815. https://hal.sorbonne-universite.fr/hal-01444566 (abstract only verified)
8. Acoustics Today, Spring 2018, p. 57 (flute jet speed vs. resonance selection). https://acousticstoday.org/issues/2018AT/Spring2018/files/basic-html/page57.html
9. M. Castellengo et al., "Flute-like instrument transients: an analytical study of mouth-tone production versus pressure rise time", ISMA 2004. https://www.lam.jussieu.fr/Membres/Castellengo/publications/2004c-Transient_Pressure%20rise%20time-ISMA.pdf
10. CFA 2018 flute transient study (search summary only, not opened): https://www.conforg.fr/cfa2018/output_directory2/data/articles/000044.pdf ; A. Ernoult PhD thesis (UPMC 2016) https://hal.sorbonne-universite.fr/tel-01430674v1/file/these-augustin-ernoult.pdf (blocked by bot-check; not read)
11. D. H. Keefe, "Acoustical wave propagation in cylindrical ducts: transmission line parameter approximations for isothermal and nonisothermal boundary conditions", JASA 75(1), 58–62 (1984).
12. S. Bilbao, R. Harrison, "Passive time-domain numerical models of viscothermal wave propagation in acoustic tubes of variable cross section", JASA 140(1), 728–740 (2016). https://www.pubmed.gov/27475194/
13. S. Bilbao, R. Harrison, J. Kergomard, B. Lombard, C. Vergez, "Passive models of viscothermal wave propagation in acoustic tubes", JASA 138(2) EL (2015). https://hal.archives-ouvertes.fr/hal-01151253
14. F. Silva, Ph. Guillemain, J. Kergomard, B. Mallaroni, A. N. Norris, "Approximation formulae for the acoustic radiation impedance of a cylindrical pipe", JSV 322, 255–263 (2009). https://arxiv.org/abs/0811.3625
15. A. Lefebvre, G. P. Scavone, "Characterization of woodwind instrument toneholes with the finite element method", JASA 131(4), 3153–3163 (2012); J.-P. Dalmont et al., "Experimental determination of the equivalent circuit of an open side hole: linear and non linear behaviour", Acta Acustica 88 (2002).

Not obtained (paywalled or blocked): de la Cuadra PhD thesis 2005 (CCRMA) full text; Coltman 1968; Fabre & Hirschberg reviews;
Wolfe group flute impedance papers. The numbers above do not depend on them, but the flute-specific θ thresholds would.
