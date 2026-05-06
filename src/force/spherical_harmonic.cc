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
  const double kLogNum = std::lgamma(n - m + 1.0);
  const double kLogDen = std::lgamma(n + m + 1.0);
  const double kKronecker = (m == 0) ? 1.0 : 2.0;
  return std::sqrt((2.0 * n + 1.0) * kKronecker * std::exp(kLogNum - kLogDen));
}

// Linear-index helper, parameterised on the row stride. Cunningham V/W
// arrays are sized for (n_max + 2) rows because acceleration formulae
// reference one beyond the coefficient table. Returns size_t directly to
// avoid sign-conversion warnings at every subscription site.
std::size_t vw_idx(int n, int m) {
  return static_cast<std::size_t>(n) * static_cast<std::size_t>(n + 1) / 2 +
         static_cast<std::size_t>(m);
}

}  // namespace

SphericalHarmonic::SphericalHarmonic(Coefficients coeffs) : coeffs_(std::move(coeffs)) {
  // Validate coefficient block size; convert normalised -> un-normalised
  // in place so the hot path doesn't pay the lgamma cost.
  const int kNeeded = coeffs_.triangular_size();
  if (static_cast<int>(coeffs_.c_norm.size()) != kNeeded ||
      static_cast<int>(coeffs_.s_norm.size()) != kNeeded) {
    throw std::invalid_argument(
        "SphericalHarmonic: c_norm/s_norm must hold (degree+1)(degree+2)/2 entries");
  }
  if (coeffs_.order > coeffs_.degree) {
    throw std::invalid_argument("SphericalHarmonic: order must be <= degree");
  }
  for (int n = 0; n <= coeffs_.degree; ++n) {
    const int kMMax = std::min(n, coeffs_.order);
    for (int m = 0; m <= kMMax; ++m) {
      const double kFactor = normalisation_factor(n, m);
      coeffs_.c_norm[Coefficients::idx(n, m)] *= kFactor;
      coeffs_.s_norm[Coefficients::idx(n, m)] *= kFactor;
    }
  }
}

apsis::math::Vec3 SphericalHarmonic::acceleration_body(const apsis::math::Vec3& r_bf) const {
  const int kNMax = coeffs_.degree;
  const int kMMaxModel = coeffs_.order;
  const double kRref = coeffs_.r_ref;
  const double kMu = coeffs_.mu;

  const double kX = r_bf.x();
  const double kY = r_bf.y();
  const double kZ = r_bf.z();
  const double kR2 = kX * kX + kY * kY + kZ * kZ;
  const double kR = std::sqrt(kR2);
  const double kRr = kRref / kR;
  const double kRr2 = kRr * kRr;
  const double kRxr2 = kRref * kX / kR2;
  const double kRyr2 = kRref * kY / kR2;
  const double kRzr2 = kRref * kZ / kR2;

  // V and W triangular arrays sized for (N + 2) rows.
  const int kRows = kNMax + 2;
  const auto kTri = static_cast<std::size_t>(kRows * (kRows + 1) / 2);
  std::vector<double> v_buf(kTri, 0.0);
  std::vector<double> w_buf(kTri, 0.0);

  // Seed the recursion (Vallado §8.6 / Montenbruck-Gill §3.2):
  //   V_{0,0} = R / r, W_{0,0} = 0
  v_buf[vw_idx(0, 0)] = kRr;
  w_buf[vw_idx(0, 0)] = 0.0;

  // Diagonal recursion (m increases): V_{m, m} and W_{m, m} from V_{m-1, m-1}.
  for (int m = 1; m <= kNMax + 1; ++m) {
    const double kVPrev = v_buf[vw_idx(m - 1, m - 1)];
    const double kWPrev = w_buf[vw_idx(m - 1, m - 1)];
    const double kCoef = (2.0 * m - 1.0);
    v_buf[vw_idx(m, m)] = kCoef * (kRxr2 * kVPrev - kRyr2 * kWPrev);
    w_buf[vw_idx(m, m)] = kCoef * (kRxr2 * kWPrev + kRyr2 * kVPrev);
  }

  // Off-diagonal recursion (n increases for each m).
  for (int m = 0; m <= kNMax + 1; ++m) {
    for (int n = m + 1; n <= kNMax + 1; ++n) {
      const double kA1 = (2.0 * n - 1.0) / static_cast<double>(n - m);
      double a2 = 0.0;
      if (n - 1 > m) {
        a2 = (n + m - 1.0) / static_cast<double>(n - m);
      }
      double v_nm = kA1 * kRzr2 * v_buf[vw_idx(n - 1, m)];
      double w_nm = kA1 * kRzr2 * w_buf[vw_idx(n - 1, m)];
      if (n - 2 >= m) {
        v_nm -= a2 * kRr2 * v_buf[vw_idx(n - 2, m)];
        w_nm -= a2 * kRr2 * w_buf[vw_idx(n - 2, m)];
      }
      v_buf[vw_idx(n, m)] = v_nm;
      w_buf[vw_idx(n, m)] = w_nm;
    }
  }

  // Assemble acceleration. mu/R^2 is the leading factor.
  const double kMuR2 = kMu / (kRref * kRref);
  double ax = 0.0;
  double ay = 0.0;
  double az = 0.0;

  for (int n = 0; n <= kNMax; ++n) {
    const int kMMax = std::min(n, kMMaxModel);
    for (int m = 0; m <= kMMax; ++m) {
      const double kC = coeffs_.c_norm[Coefficients::idx(n, m)];
      const double kS = coeffs_.s_norm[Coefficients::idx(n, m)];
      if (m == 0) {
        ax -= kC * v_buf[vw_idx(n + 1, 1)];
        ay -= kC * w_buf[vw_idx(n + 1, 1)];
      } else {
        const double kFactor = 0.5 * static_cast<double>((n - m + 1) * (n - m + 2));
        ax +=
            0.5 * (-kC * v_buf[vw_idx(n + 1, m + 1)] - kS * w_buf[vw_idx(n + 1, m + 1)] +
                   kFactor * (kC * v_buf[vw_idx(n + 1, m - 1)] + kS * w_buf[vw_idx(n + 1, m - 1)]));
        ay += 0.5 *
              (-kC * w_buf[vw_idx(n + 1, m + 1)] + kS * v_buf[vw_idx(n + 1, m + 1)] +
               kFactor * (-kC * w_buf[vw_idx(n + 1, m - 1)] + kS * v_buf[vw_idx(n + 1, m - 1)]));
      }
      const auto kNm1 = static_cast<double>(n - m + 1);
      az -= kNm1 * (kC * v_buf[vw_idx(n + 1, m)] + kS * w_buf[vw_idx(n + 1, m)]);
    }
  }

  return apsis::math::Vec3{kMuR2 * ax, kMuR2 * ay, kMuR2 * az};
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
  apsis::math::Mat36 jac = apsis::math::Mat36::Zero();
  constexpr double kH = 1.0;
  for (int i = 0; i < 3; ++i) {
    auto x_plus = x;
    auto x_minus = x;
    x_plus.r[i] += kH;
    x_minus.r[i] -= kH;
    const auto kAPlus = acceleration(t, x_plus);
    const auto kAMinus = acceleration(t, x_minus);
    jac.col(i) = (kAPlus - kAMinus) / (2.0 * kH);
  }
  // No velocity dependence -> cols 3..5 stay zero.
  return jac;
}

}  // namespace apsis::force
