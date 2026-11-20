// The bowed instruments' body as its modes: how they are found (bodymodes.h).
#include "bodymodes.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstring>
#include <future>
#include <map>
#include <mutex>
#include <tuple>

#include "bassbody.h"
#include "knob.h"

namespace pajupilli {

namespace {

using cd = std::complex<double>;
using namespace bassdata;

// Householder tridiagonalisation and the implicit QL algorithm (after JAMA's tred2 and tql2,
// public domain): the eigenvalues d and eigenvectors of the symmetric n x n matrix V (row-major,
// overwritten: on return column k of V, V[i * n + k], is the k-th eigenvector).
void symmetricEigen(int n, std::vector<double>& V, std::vector<double>& d) {
  std::vector<double> e(n);
  d.assign(n, 0);
  auto at = [&](int i, int j) -> double& { return V[size_t(i) * n + j]; };
  for (int j = 0; j < n; ++j) d[j] = at(n - 1, j);
  for (int i = n - 1; i > 0; --i) {
    double scale = 0, h = 0;
    for (int k = 0; k < i; ++k) scale += std::fabs(d[k]);
    if (scale == 0) {
      e[i] = d[i - 1];
      for (int j = 0; j < i; ++j) {
        d[j] = at(i - 1, j);
        at(i, j) = 0;
        at(j, i) = 0;
      }
    } else {
      for (int k = 0; k < i; ++k) {
        d[k] /= scale;
        h += d[k] * d[k];
      }
      double f = d[i - 1], g = std::sqrt(h);
      if (f > 0) g = -g;
      e[i] = scale * g;
      h -= f * g;
      d[i - 1] = f - g;
      for (int j = 0; j < i; ++j) e[j] = 0;
      for (int j = 0; j < i; ++j) {
        f = d[j];
        at(j, i) = f;
        g = e[j] + at(j, j) * f;
        for (int k = j + 1; k <= i - 1; ++k) {
          g += at(k, j) * d[k];
          e[k] += at(k, j) * f;
        }
        e[j] = g;
      }
      f = 0;
      for (int j = 0; j < i; ++j) {
        e[j] /= h;
        f += e[j] * d[j];
      }
      const double hh = f / (h + h);
      for (int j = 0; j < i; ++j) e[j] -= hh * d[j];
      for (int j = 0; j < i; ++j) {
        f = d[j];
        g = e[j];
        for (int k = j; k <= i - 1; ++k) at(k, j) -= f * e[k] + g * d[k];
        d[j] = at(i - 1, j);
        at(i, j) = 0;
      }
    }
    d[i] = h;
  }
  for (int i = 0; i < n - 1; ++i) {
    at(n - 1, i) = at(i, i);
    at(i, i) = 1;
    const double h = d[i + 1];
    if (h != 0) {
      for (int k = 0; k <= i; ++k) d[k] = at(k, i + 1) / h;
      for (int j = 0; j <= i; ++j) {
        double g = 0;
        for (int k = 0; k <= i; ++k) g += at(k, i + 1) * at(k, j);
        for (int k = 0; k <= i; ++k) at(k, j) -= g * d[k];
      }
    }
    for (int k = 0; k <= i; ++k) at(k, i + 1) = 0;
  }
  for (int j = 0; j < n; ++j) {
    d[j] = at(n - 1, j);
    at(n - 1, j) = 0;
  }
  at(n - 1, n - 1) = 1;
  e[0] = 0;
  // (QL on the transpose: the rotations then run along contiguous rows)
  std::vector<double> W(size_t(n) * n);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) W[size_t(j) * n + i] = at(i, j);
  for (int i = 1; i < n; ++i) e[i - 1] = e[i];
  e[n - 1] = 0;
  double f = 0, tst1 = 0;
  const double eps = std::ldexp(1.0, -52);
  for (int l = 0; l < n; ++l) {
    tst1 = std::max(tst1, std::fabs(d[l]) + std::fabs(e[l]));
    int m = l;
    while (m < n) {
      if (std::fabs(e[m]) <= eps * tst1) break;
      ++m;
    }
    if (m > l) {
      for (int iter = 0; iter < 60; ++iter) {
        double g = d[l];
        double p = (d[l + 1] - g) / (2 * e[l]);
        double r = std::hypot(p, 1.0);
        if (p < 0) r = -r;
        d[l] = e[l] / (p + r);
        d[l + 1] = e[l] * (p + r);
        const double dl1 = d[l + 1];
        double h = g - d[l];
        for (int i = l + 2; i < n; ++i) d[i] -= h;
        f += h;
        p = d[m];
        double c = 1, c2 = c, c3 = c, s = 0, s2 = 0;
        const double el1 = e[l + 1];
        for (int i = m - 1; i >= l; --i) {
          c3 = c2;
          c2 = c;
          s2 = s;
          g = c * e[i];
          h = c * p;
          r = std::hypot(p, e[i]);
          e[i + 1] = s * r;
          s = e[i] / r;
          c = p / r;
          p = c * d[i] - s * g;
          d[i + 1] = h + s * (c * g + s * d[i]);
          double* wi = &W[size_t(i) * n];
          double* wi1 = &W[size_t(i + 1) * n];
          for (int k = 0; k < n; ++k) {
            const double t = wi1[k];
            wi1[k] = s * wi[k] + c * t;
            wi[k] = c * wi[k] - s * t;
          }
        }
        p = -s * s2 * c3 * el1 * e[l] / dl1;
        e[l] = s * p;
        d[l] = c * p;
        if (std::fabs(e[l]) <= eps * tst1) break;
      }
    }
    d[l] += f;
    e[l] = 0;
  }
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < n; ++j) at(i, j) = W[size_t(j) * n + i];
}

// One degree of freedom of the grid: a plate's mass ('n': plate p, row j, column i), an air
// slug along the body ('s': side p, between cells i - 1 and i), across it ('c': cell i), a plug
// in an f-hole ('p': side p).
struct Dof {
  char type;
  int p, j, i;
};

std::shared_ptr<BodyModes> build(const BodyParams& P) {
  const BodyData& d = *P.data;
  const int S = d.substeps;
  // (the grid's numbers, rounded as it rounds them)
  const float dtF = float(1 / (P.fs * S));
  const double h = dtF;
  const float keepF = 1 - P.rate * dtF, keepAirF = 1 - P.air * dtF, keepSlugF = 1 - 4 * P.air * dtF;
  const double ratePlate = (1 - double(keepF)) / h, rateAir = (1 - double(keepAirF)) / h,
               rateSlug = (1 - double(keepSlugF)) / h;
  const float(*const Kc[2])[kNY][kNX] = {d.topK, d.backK};
  const float(*const invM[2])[kNX] = {d.topInvM, d.backInvM};
  const float dx = d.colX[1] - d.colX[0], dy = d.rowY[1] - d.rowY[0];
  const float crossArea = dx * d.depth;
  float crossInv[kNX];
  for (int i = 0; i < kNX; ++i) {
    int rows = 0;
    for (int j = 0; j < kNY; ++j) rows += d.topInvM[j][i] > 0;
    crossInv[i] = rows >= 2 ? d.slugInv * dx / (0.5f * rows * dy) : 0.0f;
  }
  const float holeY = 0.2f * kNY * dy;

  // --- The degrees of freedom, their masses (the inertia each force acts against) and rates.
  std::vector<Dof> dof;
  int nodeIdx[2][kNY][kNX];
  for (int p = 0; p < 2; ++p)
    for (int j = 0; j < kNY; ++j)
      for (int i = 0; i < kNX; ++i) {
        nodeIdx[p][j][i] = -1;
        if (invM[p][j][i] > 0) {
          nodeIdx[p][j][i] = int(dof.size());
          dof.push_back({'n', p, j, i});
        }
      }
  const int nPlate = int(dof.size());
  for (int s = 0; s < 2; ++s)
    for (int i = 1; i < kNX; ++i)
      if (d.slugArea[i - 1] > 0) dof.push_back({'s', s, 0, i});
  if (P.sides)
    for (int i = 0; i < kNX; ++i)
      if (crossInv[i] > 0) dof.push_back({'c', 0, 0, i});
  dof.push_back({'p', 0, 0, 0});
  dof.push_back({'p', 1, 0, 0});
  const int n = int(dof.size());
  std::vector<double> mu(n), rate(n);
  for (int k = 0; k < n; ++k) {
    const Dof& f = dof[k];
    switch (f.type) {
      case 'n': mu[k] = 1 / double(invM[f.p][f.j][f.i]); rate[k] = ratePlate; break;
      case 's': mu[k] = 0.5 * d.slugArea[f.i - 1] / d.slugInv; rate[k] = rateSlug; break;
      case 'c': mu[k] = crossArea / crossInv[f.i]; rate[k] = rateSlug; break;
      default: mu[k] = 0.5 / d.plugInvM; rate[k] = rateAir; break;
    }
  }
  // --- The stiffness: the plates' (13 neighbours) and the air's (each cell's pressure against
  // its volume: kappa g g^T, g the cell's volume per unit motion of each degree of freedom).
  std::vector<double> Kp(size_t(n) * n, 0.0);
  for (int k = 0; k < nPlate; ++k) {
    const Dof& f = dof[k];
    for (int o = 0; o < kOffsets; ++o) {
      const int jj = f.j + kOffJ[o], ii = f.i + kOffI[o];
      if (jj < 0 || jj >= kNY || ii < 0 || ii >= kNX || nodeIdx[f.p][jj][ii] < 0) continue;
      Kp[size_t(k) * n + nodeIdx[f.p][jj][ii]] += double(P.plateK) * Kc[f.p][o][f.j][f.i];
    }
  }
  const int nCell = P.sides ? 2 * kNX : kNX;
  std::vector<double> g(size_t(nCell) * n, 0.0), kappa(nCell);
  {
    std::vector<double> gs(size_t(2 * kNX) * n, 0.0);  // [side][cell]
    auto G = [&](int s, int i, int k) -> double& { return gs[(size_t(s) * kNX + i) * n + k]; };
    for (int k = 0; k < n; ++k) {
      const Dof& f = dof[k];
      if (f.type == 'n') {
        G(f.j >= kNY / 2, f.i, k) += d.nodeArea;
      } else if (f.type == 's') {
        const double A = d.slugArea[f.i - 1];
        G(f.p, f.i - 1, k) += 0.5 * A;
        G(f.p, f.i, k) -= 0.5 * A;
      } else if (f.type == 'c') {
        G(0, f.i, k) += crossArea;
        G(1, f.i, k) -= crossArea;
      } else {
        const int bi = d.bridgeI;
        if (d.holeCells >= 3) {
          G(f.p, bi - 1, k) += 0.125 * d.plugArea;
          G(f.p, bi, k) += 0.25 * d.plugArea;
          G(f.p, bi + 1, k) += 0.125 * d.plugArea;
        } else {
          G(f.p, bi, k) += 0.5 * d.plugArea;
        }
      }
    }
    if (P.sides) {
      g = gs;
      for (int s = 0; s < 2; ++s)
        for (int i = 0; i < kNX; ++i) kappa[s * kNX + i] = 2.0 * P.airKs * d.airK[i];
    } else {
      for (int i = 0; i < kNX; ++i) {
        for (int k = 0; k < n; ++k) g[size_t(i) * n + k] = gs[size_t(i) * n + k] + gs[(size_t(kNX) + i) * n + k];
        kappa[i] = double(P.airKs) * d.airK[i];
      }
    }
  }
  // --- The post: a (1 - behind) and b (behind) of the two nodes by the treble foot, top plus
  // back, stay at 0.
  std::vector<double> post(n, 0.0);
  {
    const double a = 1 - double(P.postBehind), b = P.postBehind;
    for (int p = 0; p < 2; ++p) {
      post[nodeIdx[p][d.trebleJ][d.bridgeI]] += a;
      if (b != 0) post[nodeIdx[p][d.trebleJ][d.bridgeI + 1]] += b;
    }
  }
  const int footB = nodeIdx[0][d.bassJ][d.bridgeI], footT = nodeIdx[0][d.trebleJ][d.bridgeI];

  // --- Mass-weighted coordinates (x = mu^-1/2 y), the post's direction removed by a
  // Householder reflection Q (its column e is the post's direction; the others span the rest).
  std::vector<double> isq(n);
  for (int k = 0; k < n; ++k) isq[k] = 1 / std::sqrt(mu[k]);
  std::vector<double> Km(size_t(n) * n);
  for (int r = 0; r < n; ++r)
    for (int c = 0; c < n; ++c) {
      double v = Kp[size_t(r) * n + c];
      Km[size_t(r) * n + c] = v;
    }
  for (int c2 = 0; c2 < nCell; ++c2) {
    const double* gc = &g[size_t(c2) * n];
    for (int r = 0; r < n; ++r) {
      if (gc[r] == 0) continue;
      const double kr = kappa[c2] * gc[r];
      for (int c = 0; c < n; ++c) Km[size_t(r) * n + c] += kr * gc[c];
    }
  }
  for (int r = 0; r < n; ++r)
    for (int c = 0; c < n; ++c) Km[size_t(r) * n + c] *= isq[r] * isq[c];
  std::vector<double> v(n);
  int e = 0;
  {
    double nrm = 0;
    for (int k = 0; k < n; ++k) {
      v[k] = post[k] * isq[k];
      nrm += v[k] * v[k];
      if (std::fabs(v[k]) > std::fabs(v[e])) e = k;
    }
    v[e] += (v[e] >= 0 ? 1 : -1) * std::sqrt(nrm);
  }
  double vv = 0;
  for (double x : v) vv += x * x;
  const double beta = 2 / vv;
  {
    std::vector<double> w(n, 0.0);
    double alpha = 0;
    for (int r = 0; r < n; ++r) {
      double s = 0;
      for (int c = 0; c < n; ++c) s += Km[size_t(r) * n + c] * v[c];
      w[r] = s;
      alpha += v[r] * s;
    }
    for (int r = 0; r < n; ++r)
      for (int c = 0; c < n; ++c)
        Km[size_t(r) * n + c] += -beta * v[r] * w[c] - beta * w[r] * v[c] + beta * beta * alpha * v[r] * v[c];
  }
  const int m0 = n - 1;
  std::vector<double> Kr(size_t(m0) * m0);
  for (int r = 0, rr = 0; r < n; ++r) {
    if (r == e) continue;
    for (int c = 0, cc = 0; c < n; ++c) {
      if (c == e) continue;
      Kr[size_t(rr) * m0 + cc++] = Km[size_t(r) * n + c];
    }
    ++rr;
  }
  std::vector<double> w2;
  symmetricEigen(m0, Kr, w2);
  // The mode shapes in the grid's own coordinates (U^T mu U = I), the air's circulations (the air
  // going round between the two halves, no cell's volume changed: no stiffness, nothing drives or
  // hears them) left out.
  std::vector<int> keepMode;
  const double wMin = 2 * M_PI * 1.0;
  for (int k = 0; k < m0; ++k)
    if (w2[k] > wMin * wMin) keepMode.push_back(k);
  const int m = int(keepMode.size());
  std::vector<double> U(size_t(n) * m);  // U[dof * m + mode]
  std::vector<double> om2(m);
  {
    std::vector<double> y(n);
    for (int q = 0; q < m; ++q) {
      const int k = keepMode[q];
      om2[q] = w2[k];
      for (int r = 0, rr = 0; r < n; ++r) y[r] = r == e ? 0.0 : Kr[size_t(rr++) * m0 + k];
      double vy = 0;
      for (int r = 0; r < n; ++r) vy += v[r] * y[r];
      for (int r = 0; r < n; ++r) U[size_t(r) * m + q] = (y[r] - beta * vy * v[r]) * isq[r];
    }
  }
  // --- The losses in the modes: Ch (all of them, -C v) and Cp (those inside the acceleration the
  // grid reports: not the decay rates its step applies after).
  //   C = diag(rate mu) + stiff Kp + airHf R Ka (R: the air's rows; the plates feel the pressure
  //   itself), U^T Kp U = Omega^2 - U^T Ka U.
  std::vector<double> GU(size_t(nCell) * m, 0.0), GRU(size_t(nCell) * m, 0.0);
  for (int c2 = 0; c2 < nCell; ++c2)
    for (int r = 0; r < n; ++r) {
      const double gr = g[size_t(c2) * n + r];
      if (gr == 0) continue;
      const double* u = &U[size_t(r) * m];
      double* o = &GU[size_t(c2) * m];
      double* oR = &GRU[size_t(c2) * m];
      for (int q = 0; q < m; ++q) o[q] += gr * u[q];
      if (r >= nPlate)
        for (int q = 0; q < m; ++q) oR[q] += gr * u[q];
    }
  std::vector<double> Ch(size_t(m) * m, 0.0), Cp(size_t(m) * m, 0.0);
  for (int a = 0; a < m; ++a)
    for (int b = 0; b < m; ++b) {
      double ka = 0, kr = 0;
      for (int c2 = 0; c2 < nCell; ++c2) {
        ka += kappa[c2] * GU[size_t(c2) * m + a] * GU[size_t(c2) * m + b];
        kr += kappa[c2] * GRU[size_t(c2) * m + a] * GU[size_t(c2) * m + b];
      }
      const double kp = (a == b ? om2[a] : 0.0) - ka;
      Cp[size_t(a) * m + b] = double(P.stiff) * kp + double(d.airHf) * kr;
      Ch[size_t(a) * m + b] = Cp[size_t(a) * m + b] + (a == b ? ratePlate : 0.0);
    }
  for (int r = nPlate; r < n; ++r) {
    const double x = (rate[r] - ratePlate) * mu[r];
    const double* u = &U[size_t(r) * m];
    for (int a = 0; a < m; ++a)
      for (int b = 0; b < m; ++b) Ch[size_t(a) * m + b] += x * u[a] * u[b];
  }
  // The feet (the modes' share of each foot's force) and the outputs (the accelerations' sums).
  std::vector<double> H(size_t(6) * n, 0.0);
  for (int k = 0; k < n; ++k) {
    const Dof& f = dof[k];
    double* h6 = &H[k];
    if (f.type == 'n') {
      const double A = d.nodeArea;
      h6[0] = A;
      if (f.p == 0) h6[2 * n] = A * d.rowY[f.j] + 0.5 * A * d.colX[f.i];
      h6[3 * n] = A * d.rowY[f.j];
      h6[4 * n] = A * d.colX[f.i];
      h6[5 * n] = (f.p == 0 ? 0.5 : -0.5) * double(d.depth) * A;
    } else if (f.type == 'p') {
      const double q = 0.5 * d.plugArea;
      h6[0] = q;
      h6[n] = q;
      h6[3 * n] = (f.p == 0 ? -holeY : holeY) * q;
      h6[4 * n] = d.colX[d.bridgeI] * q;
      h6[5 * n] = 0.5 * double(d.depth) * q;
    }
  }
  std::vector<double> HU(size_t(6) * m, 0.0), HUC(size_t(6) * m, 0.0), beta2(size_t(m) * 2);
  for (int o = 0; o < 6; ++o)
    for (int r = 0; r < n; ++r) {
      const double hr = H[size_t(o) * n + r];
      if (hr == 0) continue;
      for (int q = 0; q < m; ++q) HU[size_t(o) * m + q] += hr * U[size_t(r) * m + q];
    }
  for (int o = 0; o < 6; ++o)
    for (int l = 0; l < m; ++l) {
      const double x = HU[size_t(o) * m + l];
      for (int k = 0; k < m; ++k) HUC[size_t(o) * m + k] += x * Cp[size_t(l) * m + k];
    }
  for (int q = 0; q < m; ++q) {
    beta2[q * 2] = U[size_t(footB) * m + q];
    beta2[q * 2 + 1] = U[size_t(footT) * m + q];
  }
  double Dacc[6][2] = {};
  for (int o = 0; o < 6; ++o)
    for (int q = 0; q < m; ++q)
      for (int k = 0; k < 2; ++k) Dacc[o][k] += HU[size_t(o) * m + q] * beta2[q * 2 + k];

  // --- One step (of the grid's substeps) in the undamped modes: per mode (eta, zeta),
  //   zeta' = zeta + h (-w^2 eta - Ch zeta + beta u),  eta' = eta + h zeta'.
  // Each mode's own 2 x 2 step diagonalised (V_k, energy-normalised columns): T = V^-1 A V is
  // diagonal but for the losses' couplings between modes, -h Ch_kl (V_k^-1 [h 1]^T) (row 1 of V_l).
  const int N = 2 * m;
  std::vector<cd> T(size_t(N) * N, cd(0)), Bt(size_t(N) * 2), Ct(size_t(10) * N);
  std::vector<cd> pk(N), ql(N);  // [2k + a]
  for (int k = 0; k < m; ++k) {
    const double w2k = om2[k], ck = Ch[size_t(k) * m + k];
    const double a11 = 1 - h * h * w2k, a12 = h * (1 - h * ck), a21 = -h * w2k, a22 = 1 - h * ck;
    const double tr = a11 + a22, det = a11 * a22 - a12 * a21, disc = tr * tr / 4 - det;
    cd l1, l2;
    if (disc < 0) {
      l1 = cd(tr / 2, std::sqrt(-disc));
      l2 = std::conj(l1);
    } else {
      l1 = tr / 2 + std::sqrt(disc);
      l2 = tr / 2 - std::sqrt(disc);
    }
    cd V[2][2] = {{a12, a12}, {l1 - a11, l2 - a11}};
    for (int c = 0; c < 2; ++c) {
      const double nn = std::sqrt(w2k * std::norm(V[0][c]) + std::norm(V[1][c]));
      V[0][c] /= nn;
      V[1][c] /= nn;
    }
    const cd dt = V[0][0] * V[1][1] - V[0][1] * V[1][0];
    const cd Vi[2][2] = {{V[1][1] / dt, -V[0][1] / dt}, {-V[1][0] / dt, V[0][0] / dt}};
    T[size_t(2 * k) * N + 2 * k] = l1;
    T[size_t(2 * k + 1) * N + 2 * k + 1] = l2;
    for (int a = 0; a < 2; ++a) {
      pk[2 * k + a] = Vi[a][0] * h + Vi[a][1];
      ql[2 * k + a] = V[1][a];
      for (int in = 0; in < 2; ++in) Bt[size_t(2 * k + a) * 2 + in] = h * beta2[k * 2 + in] * pk[2 * k + a];
      // (per step: the accelerations from the state before it; the feet from the state after it)
      for (int o = 0; o < 6; ++o)
        Ct[size_t(o) * N + 2 * k + a] = -HU[size_t(o) * m + k] * w2k * V[0][a] - HUC[size_t(o) * m + k] * V[1][a];
      Ct[size_t(6) * N + 2 * k + a] = U[size_t(footB) * m + k] * V[0][a];
      Ct[size_t(7) * N + 2 * k + a] = U[size_t(footT) * m + k] * V[0][a];
      Ct[size_t(8) * N + 2 * k + a] = U[size_t(footB) * m + k] * V[1][a];
      Ct[size_t(9) * N + 2 * k + a] = U[size_t(footT) * m + k] * V[1][a];
    }
  }
  for (int k = 0; k < m; ++k)
    for (int l = 0; l < m; ++l) {
      if (k == l) continue;
      const double c = -h * Ch[size_t(k) * m + l];
      if (c == 0) continue;
      for (int a = 0; a < 2; ++a)
        for (int b = 0; b < 2; ++b) T[size_t(2 * k + a) * N + 2 * l + b] = c * pk[2 * k + a] * ql[2 * l + b];
    }
  // --- The couplings removed: 2 x 2 similarity transforms on every pair whose coupling is more
  // than tol of their poles' distance (largest first, sweep after sweep), carried to the feet's
  // shares (rows) and the outputs' (columns).
  static const double kTol = knob("PJ_BODY_TOL", 3e-3);
  double scale = 0;
  for (int i = 0; i < N; ++i) scale = std::max(scale, std::abs(T[size_t(i) * N + i]));
  const double floorAbs = 1e-14 * scale;
  struct Pair {
    double r;
    int i, j;
  };
  std::vector<Pair> pairs;
  for (int sweep = 0; sweep < 40; ++sweep) {
    pairs.clear();
    for (int i = 0; i < N; ++i) {
      const cd di = T[size_t(i) * N + i];
      for (int j = i + 1; j < N; ++j) {
        const double mx = std::max(std::abs(T[size_t(i) * N + j]), std::abs(T[size_t(j) * N + i]));
        if (mx <= floorAbs) continue;
        const double r = mx / std::max(1e-300, std::abs(di - T[size_t(j) * N + j]));
        if (r > kTol) pairs.push_back({r, i, j});
      }
    }
    if (pairs.empty()) break;
    std::sort(pairs.begin(), pairs.end(), [](const Pair& x, const Pair& y) { return x.r > y.r; });
    for (const Pair& pr : pairs) {
      const int i = pr.i, j = pr.j;
      const cd a = T[size_t(i) * N + i], b = T[size_t(i) * N + j], c = T[size_t(j) * N + i], dd = T[size_t(j) * N + j];
      const double mx = std::max(std::abs(b), std::abs(c));
      if (mx <= floorAbs || mx <= kTol * std::abs(a - dd)) continue;
      const cd de = 0.5 * (a - dd);
      cd disc = std::sqrt(de * de + b * c);
      if ((std::conj(de) * disc).real() < 0) disc = -disc;
      const cd den = de + disc;
      if (std::abs(den) < 1e-9 * mx) continue;  // (defective: leave it)
      const cd s21 = c / den, s12 = -b / den, idet = 1.0 / (1.0 - s12 * s21);
      for (int r = 0; r < N; ++r) {
        cd& ti = T[size_t(r) * N + i];
        cd& tj = T[size_t(r) * N + j];
        const cd x = ti, y = tj;
        ti = x + y * s21;
        tj = x * s12 + y;
      }
      for (int o = 0; o < 10; ++o) {
        cd& ti = Ct[size_t(o) * N + i];
        cd& tj = Ct[size_t(o) * N + j];
        const cd x = ti, y = tj;
        ti = x + y * s21;
        tj = x * s12 + y;
      }
      cd* ri = &T[size_t(i) * N];
      cd* rj = &T[size_t(j) * N];
      for (int col = 0; col < N; ++col) {
        const cd x = ri[col], y = rj[col];
        ri[col] = (x - s12 * y) * idet;
        rj[col] = (y - s21 * x) * idet;
      }
      for (int in = 0; in < 2; ++in) {
        const cd x = Bt[size_t(i) * 2 + in], y = Bt[size_t(j) * 2 + in];
        Bt[size_t(i) * 2 + in] = (x - s12 * y) * idet;
        Bt[size_t(j) * 2 + in] = (y - s21 * x) * idet;
      }
      T[size_t(i) * N + j] = T[size_t(j) * N + i] = 0;
    }
  }
  // The rest to first order: each mode's shares corrected by its weak couplings (X = E / (d_j -
  // d_i)), each pole to second order.
  std::vector<cd> lam(N);
  for (int i = 0; i < N; ++i) lam[i] = T[size_t(i) * N + i];
  {
    std::vector<cd> Bn(Bt), Cn(Ct), lamN(lam);
    for (int i = 0; i < N; ++i)
      for (int j = 0; j < N; ++j) {
        if (i == j) continue;
        const cd Eij = T[size_t(i) * N + j];
        if (std::abs(Eij) <= floorAbs * 0.1) continue;
        const cd dl = lam[j] - lam[i];
        if (std::abs(dl) <= 1e-12 * scale) continue;
        const cd X = Eij / dl;
        // Bt <- Bt - X Bt (row i gets -X_ij Bt_j); Ct <- Ct + Ct X (column j gets Ct_i X_ij)
        for (int in = 0; in < 2; ++in) Bn[size_t(i) * 2 + in] -= X * Bt[size_t(j) * 2 + in];
        for (int o = 0; o < 10; ++o) Cn[size_t(o) * N + j] += Ct[size_t(o) * N + i] * X;
        lamN[i] += Eij * T[size_t(j) * N + i] / (lam[i] - lam[j]);
      }
    Bt.swap(Bn);
    Ct.swap(Cn);
    lam.swap(lamN);
  }

  // --- A sample: S steps, the feet's forces held. The sides' and directions' low-passes (one
  // a step: side1 += A ((m - m_prev) / (h c) - side1), side += B (side1 - side); dip += A (...))
  // written as pure one-poles of m: side1 = h1 + be m_prev-step, h1' = (1 - A) h1 + ga m.
  const double A_ = double(float(1 - std::exp(-2 * M_PI * d.sideLo * dtF)));
  const double B_ = double(float(1 - std::exp(-2 * M_PI * d.sideHi * dtF)));
  const double al = 1 / (double(float(dtF * 349.5f))) * A_, be = al / (1 - A_), ga = -al * A_ / (1 - A_);
  double Pm[5][5] = {}, Wd[5][6] = {};
  Pm[0][0] = 1 - A_;
  Pm[1][0] = B_ * (1 - A_);
  Pm[1][1] = 1 - B_;
  Pm[2][2] = Pm[3][3] = Pm[4][4] = 1 - A_;
  Wd[0][2] = ga;
  Wd[1][2] = B_ * (ga + be);
  Wd[2][3] = Wd[3][4] = Wd[4][5] = ga;
  std::vector<cd> psi(size_t(5) * N);
  double Q[5][2] = {};
  for (int i = 0; i < N; ++i) {
    cd w[5];
    for (int r = 0; r < 5; ++r) {
      w[r] = 0;
      for (int o = 0; o < 6; ++o) w[r] += Wd[r][o] * Ct[size_t(o) * N + i];
    }
    cd ps[5];
    ps[0] = w[0] / (lam[i] - Pm[0][0]);
    ps[1] = (w[1] + Pm[1][0] * ps[0]) / (lam[i] - Pm[1][1]);
    for (int r = 2; r < 5; ++r) ps[r] = w[r] / (lam[i] - Pm[r][r]);
    for (int r = 0; r < 5; ++r) {
      psi[size_t(r) * N + i] = ps[r];
      for (int in = 0; in < 2; ++in) Q[r][in] -= (ps[r] * Bt[size_t(i) * 2 + in]).real();
    }
  }
  for (int r = 0; r < 5; ++r)
    for (int in = 0; in < 2; ++in)
      for (int o = 0; o < 6; ++o) Q[r][in] += Wd[r][o] * Dacc[o][in];
  auto out = std::make_shared<BodyModes>();
  // P^S, (sum P^k) Q
  {
    double Pk[5][5], acc[5][5] = {}, PS[5][5];
    for (int r = 0; r < 5; ++r)
      for (int c = 0; c < 5; ++c) Pk[r][c] = r == c;
    for (int k = 0; k < S; ++k) {
      for (int r = 0; r < 5; ++r)
        for (int c = 0; c < 5; ++c) acc[r][c] += Pk[r][c];
      double nx[5][5] = {};
      for (int r = 0; r < 5; ++r)
        for (int c = 0; c < 5; ++c)
          for (int t = 0; t < 5; ++t) nx[r][c] += Pm[r][t] * Pk[t][c];
      std::memcpy(Pk, nx, sizeof Pk);
    }
    std::memcpy(PS, Pk, sizeof PS);
    for (int r = 0; r < 5; ++r) {
      for (int c = 0; c < 5; ++c) out->ps[r][c] = float(PS[r][c]);
      for (int in = 0; in < 2; ++in) {
        double s = 0;
        for (int t = 0; t < 5; ++t) s += acc[r][t] * Q[t][in];
        out->qs[r][in] = float(s);
      }
    }
  }
  out->e[2][1] = 1;
  out->e[3][2] = out->e[4][3] = out->e[5][4] = 1;
  // Each mode over the sample: its pole^S, its drive (1 + l + .. + l^(S-1)) b, each output's share.
  std::vector<cd> L(N), bS(size_t(N) * 2), Wo(size_t(10) * N);
  double D[10][2] = {};
  for (int i = 0; i < N; ++i) {
    const cd l = lam[i];
    std::vector<cd> gk(S + 1), lk(S + 1);
    gk[0] = 0;
    lk[0] = 1;
    for (int k = 1; k <= S; ++k) {
      lk[k] = lk[k - 1] * l;
      gk[k] = gk[k - 1] + lk[k - 1];
    }
    L[i] = lk[S];
    const cd b0 = Bt[size_t(i) * 2], b1 = Bt[size_t(i) * 2 + 1];
    bS[size_t(i) * 2] = gk[S] * b0;
    bS[size_t(i) * 2 + 1] = gk[S] * b1;
    auto dsum = [&](int o, cd w) {  // the real part of w b, into D[o]
      D[o][0] += (w * b0).real();
      D[o][1] += (w * b1).real();
    };
    for (int o = 0; o < 2; ++o) {
      const cd c = Ct[size_t(o) * N + i];
      Wo[size_t(o) * N + i] = c * gk[S] / double(S);
      cd sg = 0;
      for (int k = 0; k < S; ++k) sg += gk[k];
      dsum(o, c * sg / double(S));
    }
    Wo[size_t(2) * N + i] = psi[size_t(1) * N + i] * lk[S];
    dsum(2, psi[size_t(1) * N + i] * gk[S]);
    for (int q = 0; q < 3; ++q) {
      const cd c = Ct[size_t(3 + q) * N + i], ps = psi[size_t(2 + q) * N + i];
      Wo[size_t(3 + q) * N + i] = ps * lk[S] + be * c * lk[S - 1];
      dsum(3 + q, ps * gk[S] + be * c * gk[S - 1]);
    }
    for (int q = 0; q < 4; ++q) {
      const cd c = Ct[size_t(6 + q) * N + i];
      Wo[size_t(6 + q) * N + i] = c * lk[S];
      dsum(6 + q, c * gk[S]);
    }
  }
  for (int in = 0; in < 2; ++in) {
    D[0][in] += Dacc[0][in];
    D[1][in] += Dacc[1][in];
    for (int q = 0; q < 3; ++q) D[3 + q][in] += be * Dacc[3 + q][in];
  }
  for (int o = 0; o < 10; ++o)
    for (int in = 0; in < 2; ++in) out->d[o][in] = float(D[o][in]);
  // --- Which to keep: one of each pair of complex poles (heard twice: re(w z) of both), the real
  // ones; those whose impulse response's energy in no output, from neither foot, reaches eps of
  // that output's are dropped.
  static const double kPrune = knob("PJ_BODY_PRUNE", 1e-7);
  std::vector<int> sel;
  std::vector<double> fac;
  for (int i = 0; i < N; ++i) {
    const double im = lam[i].imag(), ab = std::abs(lam[i]);
    if (std::fabs(im) <= 1e-9 * ab) {
      sel.push_back(i);
      fac.push_back(1);
    } else if (im > 0) {
      sel.push_back(i);
      fac.push_back(2);
    }
  }
  const int ns = int(sel.size());
  std::vector<double> cont(size_t(ns) * 20), tot(20, 0.0);
  for (int q = 0; q < ns; ++q) {
    const int i = sel[q];
    const double den = std::max(1e-30, 1 - std::norm(L[i]));
    for (int o = 0; o < 10; ++o)
      for (int in = 0; in < 2; ++in) {
        const double c = fac[q] * fac[q] * std::norm(Wo[size_t(o) * N + i]) * std::norm(bS[size_t(i) * 2 + in]) / den;
        cont[size_t(q) * 20 + o * 2 + in] = c;
        tot[o * 2 + in] += c;
      }
  }
  std::vector<int> kept;
  for (int q = 0; q < ns; ++q) {
    const int i = sel[q];
    if (!(std::abs(L[i]) < 1 - 1e-12)) continue;
    double mx = 0;
    for (int t = 0; t < 20; ++t) mx = std::max(mx, cont[size_t(q) * 20 + t] / std::max(1e-300, tot[t]));
    if (mx > kPrune) kept.push_back(q);
  }
  out->total = ns;
  out->modes = int(kept.size());
  const int ng = (out->modes + 15) / 16;
  out->groups.assign(ng, BodyModes::Group{});
  for (int k = 0; k < out->modes; ++k) {
    const int q = kept[k], i = sel[q];
    BodyModes::Group& G = out->groups[k / 16];
    const int l = k % 16;
    const bool real = fac[q] == 1;
    G.lr[l] = float(L[i].real());
    G.li[l] = real ? 0.0f : float(L[i].imag());
    G.b0r[l] = float(bS[size_t(i) * 2].real());
    G.b0i[l] = float(bS[size_t(i) * 2].imag());
    G.b1r[l] = float(bS[size_t(i) * 2 + 1].real());
    G.b1i[l] = float(bS[size_t(i) * 2 + 1].imag());
    for (int o = 0; o < 10; ++o) {
      // (a real pole's drive and shares may carry a rounding's worth of an imaginary part, a
      // factor common to both: kept, re(w z) is unchanged by it)
      const cd w = fac[q] * Wo[size_t(o) * N + i];
      G.wr[o][l] = float(w.real());
      G.wi[o][l] = float(w.imag());
    }
  }
  return out;
}

}  // namespace

bool BodyParams::operator<(const BodyParams& o) const {
  return std::tie(data, fs, rate, stiff, air, plateK, airKs, postBehind, sides) <
         std::tie(o.data, o.fs, o.rate, o.stiff, o.air, o.plateK, o.airKs, o.postBehind, o.sides);
}

std::shared_ptr<const BodyModes> bodyModes(const BodyParams& p) {
  // (one build per parameters, by whoever asks first; the others wait for it)
  static std::mutex mu;
  static std::map<BodyParams, std::shared_future<std::shared_ptr<const BodyModes>>> cache;
  std::unique_lock<std::mutex> lock(mu);
  auto it = cache.find(p);
  if (it != cache.end()) {
    std::shared_future<std::shared_ptr<const BodyModes>> f = it->second;
    lock.unlock();
    return f.get();
  }
  std::promise<std::shared_ptr<const BodyModes>> pr;
  cache.emplace(p, pr.get_future().share());
  lock.unlock();
  std::shared_ptr<const BodyModes> m = build(p);
  pr.set_value(m);
  return m;
}

}  // namespace pajupilli
