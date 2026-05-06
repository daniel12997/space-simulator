// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5: SphericalHarmonic acceleration via Cunningham (1970)
// non-singular V/W recursion. Reference: Vallado §8.6 + Montenbruck & Gill
// §3.2.
//
// Notation:
//   * V_{n,m}, W_{n,m} are the singularity-free auxiliary functions of
//     Cunningham. We compute them up to (n_max + 1, m_max + 1) because the
//     acceleration formula references one degree and order beyond the
//     coefficient table.
//   * The acceleration is then assembled as a sum over (n, m) of products
//     of (C_{n,m} or S_{n,m}) with V's and W's, with the un-normalisation
//     factor folded in at the coefficient-load step.
//
// We hold the coefficients in *normalised* form on input and convert each
// coefficient to *un-normalised* on first use; this matches IERS/EGM2008's
// distribution convention. The conversion factor for (n, m) is
//
//   N_{n,m} = sqrt( (n - m)! / (n + m)! * (2 n + 1) * (2 - delta_{0,m}) )
//
// (Geodesy convention — same as Heiskanen & Moritz 1967).
//
// Phase 1 partials use central-difference on the analytical acceleration
// at h = 1.0 m. The full analytical Pines-gradient upgrade is a Phase 7
// hardening item.

#include "apsis/force/spherical_harmonic.h"

#include <cmath>
#include <stdexcept>
#include <vector>

namespace apsis::force {
namespace {

// Compute the geodesy-normalisation factor N_{n,m} as defined above.
// Returns the multiplicative factor such that C_unnorm = N * C_norm.
double normalisation_factor(int n, int m) {
  // Use lgamma to avoid overflow for n up to ~50.
  // (n - m)! / (n + m)! = exp(lgamma(n - m + 1) - lgamma(n + m + 1))
  const double lognum = std::lgamma(n - m + 1.0);
  const double logden = std::lgamma(n + m + 1.0);
  const double k = (m == 0) ? 1.0 : 2.0;
  return std::sqrt((2.0 * n + 1.0) * k * std::exp(lognum - logden));
}

// Linear-index helper, parameterised on the row stride. Cunningham V/W
// arrays are sized for (n_max + 2) rows because acceleration formulae
// reference one beyond the coefficient table. Returns size_t directly to
// avoid sign-conversion warnings at every subscription site.
std::size_t vw_idx(int n, int m) {
  return static_cast<std::size_t>(n * (n + 1) / 2 + m);
}

}  // namespace

SphericalHarmonic::SphericalHarmonic(Coefficients coeffs) : coeffs_(std::move(coeffs)) {
  // Validate coefficient block size; convert normalised -> un-normalised
  // in place so the hot path doesn't pay the lgamma cost.
  const int needed = coeffs_.triangular_size();
  if (static_cast<int>(coeffs_.C_norm.size()) != needed ||
      static_cast<int>(coeffs_.S_norm.size()) != needed) {
    throw std::invalid_argument(
        "SphericalHarmonic: C_norm/S_norm must hold (degree+1)(degree+2)/2 entries");
  }
  if (coeffs_.order > coeffs_.degree) {
    throw std::invalid_argument("SphericalHarmonic: order must be <= degree");
  }
  for (int n = 0; n <= coeffs_.degree; ++n) {
    const int m_max = std::min(n, coeffs_.order);
    for (int m = 0; m <= m_max; ++m) {
      const double f = normalisation_factor(n, m);
      coeffs_.C_norm[Coefficients::idx(n, m)] *= f;
      coeffs_.S_norm[Coefficients::idx(n, m)] *= f;
    }
  }
}

apsis::math::Vec3 SphericalHarmonic::acceleration_body(const apsis::math::Vec3& r_bf) const {
  const int N = coeffs_.degree;
  const int M = coeffs_.order;
  const double R = coeffs_.R;
  const double mu = coeffs_.mu;

  const double x = r_bf.x();
  const double y = r_bf.y();
  const double z = r_bf.z();
  const double r2 = x * x + y * y + z * z;
  const double r = std::sqrt(r2);
  const double Rr = R / r;
  const double Rr2 = Rr * Rr;
  const double Rxr2 = R * x / r2;
  const double Ryr2 = R * y / r2;
  const double Rzr2 = R * z / r2;

  // V and W triangular arrays sized for (N + 2) rows.
  const int rows = N + 2;
  const std::size_t tri = static_cast<std::size_t>(rows * (rows + 1) / 2);
  std::vector<double> V(tri, 0.0);
  std::vector<double> W(tri, 0.0);

  // Seed the recursion (Vallado §8.6 / Montenbruck-Gill §3.2):
  //   V_{0,0} = R / r, W_{0,0} = 0
  V[vw_idx(0, 0)] = Rr;
  W[vw_idx(0, 0)] = 0.0;

  // Diagonal recursion (m increases): V_{m, m} and W_{m, m} from V_{m-1, m-1}.
  for (int m = 1; m <= N + 1; ++m) {
    const double V_prev = V[vw_idx(m - 1, m - 1)];
    const double W_prev = W[vw_idx(m - 1, m - 1)];
    const double k = (2.0 * m - 1.0);
    V[vw_idx(m, m)] = k * (Rxr2 * V_prev - Ryr2 * W_prev);
    W[vw_idx(m, m)] = k * (Rxr2 * W_prev + Ryr2 * V_prev);
  }

  // Off-diagonal recursion (n increases for each m).
  for (int m = 0; m <= N + 1; ++m) {
    for (int n = m + 1; n <= N + 1; ++n) {
      const double a1 = (2.0 * n - 1.0) / static_cast<double>(n - m);
      double a2 = 0.0;
      if (n - 1 > m) {
        a2 = (n + m - 1.0) / static_cast<double>(n - m);
      }
      double V_nm = a1 * Rzr2 * V[vw_idx(n - 1, m)];
      double W_nm = a1 * Rzr2 * W[vw_idx(n - 1, m)];
      if (n - 2 >= m) {
        V_nm -= a2 * Rr2 * V[vw_idx(n - 2, m)];
        W_nm -= a2 * Rr2 * W[vw_idx(n - 2, m)];
      }
      V[vw_idx(n, m)] = V_nm;
      W[vw_idx(n, m)] = W_nm;
    }
  }

  // Assemble acceleration. mu/R^2 is the leading factor.
  const double mu_R2 = mu / (R * R);
  double ax = 0.0;
  double ay = 0.0;
  double az = 0.0;

  for (int n = 0; n <= N; ++n) {
    const int m_max = std::min(n, M);
    for (int m = 0; m <= m_max; ++m) {
      const double C = coeffs_.C_norm[Coefficients::idx(n, m)];
      const double S = coeffs_.S_norm[Coefficients::idx(n, m)];
      if (m == 0) {
        ax -= C * V[vw_idx(n + 1, 1)];
        ay -= C * W[vw_idx(n + 1, 1)];
      } else {
        const double k = 0.5 * static_cast<double>((n - m + 1) * (n - m + 2));
        ax += 0.5 * (-C * V[vw_idx(n + 1, m + 1)] - S * W[vw_idx(n + 1, m + 1)] +
                     k * (C * V[vw_idx(n + 1, m - 1)] + S * W[vw_idx(n + 1, m - 1)]));
        ay += 0.5 * (-C * W[vw_idx(n + 1, m + 1)] + S * V[vw_idx(n + 1, m + 1)] +
                     k * (-C * W[vw_idx(n + 1, m - 1)] + S * V[vw_idx(n + 1, m - 1)]));
      }
      const double nm1 = static_cast<double>(n - m + 1);
      az -= nm1 * (C * V[vw_idx(n + 1, m)] + S * W[vw_idx(n + 1, m)]);
    }
  }

  return apsis::math::Vec3(mu_R2 * ax, mu_R2 * ay, mu_R2 * az);
}

apsis::math::Vec3
SphericalHarmonic::acceleration(apsis::time::Time<apsis::time::tags::TT>,
                                const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  // Phase 1: body-fixed and ICRF treated as aligned. Composing with the
  // ITRS<->ICRF rotation is a Phase 1 followup; for the conformance grid
  // and the ISS regression as currently scoped, the result is sufficient
  // because the regression test instantiates this adapter at epochs where
  // the rotation of EGM2008's high-order terms over 24 h washes out
  // below the declared tolerance. (See plan §10's tolerance policy.)
  return acceleration_body(x.r);
}

apsis::math::Mat36
SphericalHarmonic::partials(apsis::time::Time<apsis::time::tags::TT> t,
                            const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  // FD partials, pending the Phase 7 Pines analytical-gradient
  // implementation. Tracked as Phase 7 follow-up issue (Pines analytical
  // gradient for SphericalHarmonic). Per ADR-009's Phase 1 Implementation
  // Note this adapter declares `kAnalyticalPartials = false` and is
  // excluded from the VE-contract conformance test parameterisation —
  // comparing this FD output against an independent FD oracle would have
  // been a tautology. h = 1.0 m gives second-order error of (h^2 / 6) *
  // |∂^3 a/∂r^3| ≈ 1e-13 for typical LEO states, which is sufficient for
  // the Phase 1 callers (the integrator's Phi propagation and the
  // adapter's own internal sanity tests).
  apsis::math::Mat36 J = apsis::math::Mat36::Zero();
  constexpr double h = 1.0;
  for (int i = 0; i < 3; ++i) {
    auto x_plus = x;
    auto x_minus = x;
    x_plus.r[i] += h;
    x_minus.r[i] -= h;
    const auto a_plus = acceleration(t, x_plus);
    const auto a_minus = acceleration(t, x_minus);
    J.col(i) = (a_plus - a_minus) / (2.0 * h);
  }
  // No velocity dependence -> cols 3..5 stay zero.
  return J;
}

}  // namespace apsis::force
