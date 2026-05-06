// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5 / Phase-1A §C: SphericalHarmonic acceleration via Cunningham
// (1970) non-singular V/W recursion. Reference: Vallado §8.6 + Montenbruck
// & Gill (M-G) §3.2.
//
// Notation:
//   * V_{n,m}, W_{n,m} are the singularity-free auxiliary functions of
//     Cunningham. The acceleration formula references one degree and one
//     order beyond the coefficient table (V/W up to row N+1); the
//     analytical Jacobian (Phase-1A §C2) needs one further row, so V/W
//     are computed up to row N+2.
//   * Acceleration is a sum over (n, m) of products of (C_{n,m} or
//     S_{n,m}) with V's and W's, with the un-normalisation factor folded
//     in at the coefficient-load step.
//   * Coefficients are held in *normalised* form on input and converted
//     to *un-normalised* on first use; this matches IERS / EGM2008's
//     distribution convention.
//
// Phase-1A §C1: this adapter is *body-fixed* (Earth-fixed for EGM2008).
// `acceleration()` and `partials_dadx()` accept ICRF input, rotate to
// body-fixed via the EOP-aware `apsis::frames::icrf_to_itrs_rotation`,
// evaluate Cunningham V/W in body-fixed, and rotate the acceleration
// (and conjugate the Jacobian) back to ICRF. The Phase-1 "body-fixed
// and ICRF aligned" shortcut is removed.
//
// Phase-1A §C2 (analytical gradient):
//
//   The acceleration assembly (M-G §3.2 / Vallado §8.6) expresses each
//   per-(n,m) term as a linear combination of V_{n+1, m'} and W_{n+1, m'}
//   for m' in {m-1, m, m+1}. The Cunningham identities for the partial
//   derivatives of V/W with respect to body-fixed Cartesian coordinates
//   (M-G eq 3.33) say
//
//     m = 0:
//       ∂V_{n,0}/∂x = -V_{n+1, 1}                       (units: 1/R_ref)
//       ∂V_{n,0}/∂y = -W_{n+1, 1}
//       ∂V_{n,0}/∂z = -(n+1) V_{n+1, 0}
//
//     m >= 1:
//       ∂V_{n,m}/∂x = 0.5 * [-V_{n+1, m+1} + (n-m+2)(n-m+1) V_{n+1, m-1}]
//       ∂V_{n,m}/∂y = 0.5 * [-W_{n+1, m+1} - (n-m+2)(n-m+1) W_{n+1, m-1}]
//       ∂V_{n,m}/∂z = -(n-m+1) V_{n+1, m}
//       ∂W_{n,m}/∂x = 0.5 * [-W_{n+1, m+1} + (n-m+2)(n-m+1) W_{n+1, m-1}]
//       ∂W_{n,m}/∂y = 0.5 * [ V_{n+1, m+1} + (n-m+2)(n-m+1) V_{n+1, m-1}]
//       ∂W_{n,m}/∂z = -(n-m+1) W_{n+1, m}
//
//   Each ∂/∂x_k carries an implicit `1/R_ref` factor (V/W are dimensionless
//   in (R_ref/r) units; one position derivative pulls out one `1/R_ref`).
//
//   The Jacobian ∂a_bf/∂r_bf is then computed by applying these identities
//   to the acceleration formula, term-by-term. The acceleration prefactor
//   `mu / R_ref^2` becomes `mu / R_ref^3` for the gradient.
//
// Cross-attribution: the canonical open-source reference is Orekit's
// `HolmesFeatherstoneAttractionModel.java` (Apache-2.0; same licence as
// Apsis); see [[wiki/sources/orekit-holmes-featherstone-impl]]. Orekit's
// implementation uses the Holmes-Featherstone (2002) normalised-Legendre
// formulation, which is mathematically equivalent to but stylistically
// different from Cunningham V/W. We chose the Cunningham gradient here
// because it stays inside the existing V/W skeleton (one extra row of
// recursion, identical assembly loop), avoiding a from-scratch Holmes-
// Featherstone port. M-G §3.2 + Vallado §8.6 are the textbook references;
// Orekit is the algorithm's open-source sibling that was studied while
// porting.

#include "apsis/force/spherical_harmonic.h"

#include <cmath>
#include <stdexcept>
#include <vector>

#include "apsis/frames/transform.h"
#include "apsis/time/eop_table.h"

namespace apsis::force {
namespace {

// Compute the geodesy-normalisation factor N_{n,m} as defined in the file
// header. Returns the multiplicative factor such that C_unnorm = N * C_norm.
double normalisation_factor(int n, int m) {
  // Use lgamma to avoid overflow for n up to ~50.
  // (n - m)! / (n + m)! = exp(lgamma(n - m + 1) - lgamma(n + m + 1))
  const double kLogNum = std::lgamma(n - m + 1.0);
  const double kLogDen = std::lgamma(n + m + 1.0);
  const double kKronecker = (m == 0) ? 1.0 : 2.0;
  return std::sqrt((2.0 * n + 1.0) * kKronecker * std::exp(kLogNum - kLogDen));
}

// Linear-index helper for the V/W triangular tables. Returns size_t directly
// to avoid sign-conversion warnings at every subscription site. Valid only
// for `0 <= m <= n`; reads outside that triangle are *mathematically* zero
// (V_{n,m}, W_{n,m} vanish for m > n), so callers that may stray outside the
// triangle MUST go through `vw_at` instead.
std::size_t vw_idx(int n, int m) {
  return static_cast<std::size_t>(n) * static_cast<std::size_t>(n + 1) / 2 +
         static_cast<std::size_t>(m);
}

// Triangular-aware accessor: returns 0 for (n, m) outside the valid
// triangle (m < 0 or m > n). Callers that touch the gradient code paths
// use this because the partial-derivative recursions reference V_{n+1, m+2}
// terms which fall outside the m <= n triangle for sectorial coefficients.
double vw_at(const std::vector<double>& tab, int n, int m) {
  if (m < 0 || m > n) {
    return 0.0;
  }
  return tab[vw_idx(n, m)];
}

// Compute the Cunningham V/W tables in body-fixed coordinates up to row
// `rows` (i.e. n in [0, rows-1], m in [0, n]). Caller passes the body-fixed
// position and the reference radius.
//
// Returns the two tables (V, W) as flat row-major triangular arrays.
struct VW {
  std::vector<double> v;
  std::vector<double> w;
};

VW compute_vw(const apsis::math::Vec3& r_bf, double r_ref, int rows) {
  const double kX = r_bf.x();
  const double kY = r_bf.y();
  const double kZ = r_bf.z();
  const double kR2 = kX * kX + kY * kY + kZ * kZ;
  const double kR = std::sqrt(kR2);
  const double kRr = r_ref / kR;
  const double kRr2 = kRr * kRr;
  const double kRxr2 = r_ref * kX / kR2;
  const double kRyr2 = r_ref * kY / kR2;
  const double kRzr2 = r_ref * kZ / kR2;

  const auto kTri = static_cast<std::size_t>(rows * (rows + 1) / 2);
  VW out{std::vector<double>(kTri, 0.0), std::vector<double>(kTri, 0.0)};

  // Seed (Vallado §8.6 / M-G §3.2):
  //   V_{0,0} = R_ref / r, W_{0,0} = 0.
  out.v[vw_idx(0, 0)] = kRr;
  out.w[vw_idx(0, 0)] = 0.0;

  // Diagonal recursion (m increases): V_{m,m}, W_{m,m} from V_{m-1,m-1},
  // W_{m-1,m-1}.
  for (int m = 1; m < rows; ++m) {
    const double kVPrev = out.v[vw_idx(m - 1, m - 1)];
    const double kWPrev = out.w[vw_idx(m - 1, m - 1)];
    const double kCoef = (2.0 * m - 1.0);
    out.v[vw_idx(m, m)] = kCoef * (kRxr2 * kVPrev - kRyr2 * kWPrev);
    out.w[vw_idx(m, m)] = kCoef * (kRxr2 * kWPrev + kRyr2 * kVPrev);
  }

  // Off-diagonal recursion (n increases for each m).
  for (int m = 0; m < rows; ++m) {
    for (int n = m + 1; n < rows; ++n) {
      const double kA1 = (2.0 * n - 1.0) / static_cast<double>(n - m);
      double a2 = 0.0;
      if (n - 1 > m) {
        a2 = (n + m - 1.0) / static_cast<double>(n - m);
      }
      double v_nm = kA1 * kRzr2 * out.v[vw_idx(n - 1, m)];
      double w_nm = kA1 * kRzr2 * out.w[vw_idx(n - 1, m)];
      if (n - 2 >= m) {
        v_nm -= a2 * kRr2 * out.v[vw_idx(n - 2, m)];
        w_nm -= a2 * kRr2 * out.w[vw_idx(n - 2, m)];
      }
      out.v[vw_idx(n, m)] = v_nm;
      out.w[vw_idx(n, m)] = w_nm;
    }
  }

  return out;
}

}  // namespace

SphericalHarmonic::SphericalHarmonic(Coefficients coeffs, const apsis::time::EopTable& eop)
    : coeffs_(std::move(coeffs)), eop_(&eop) {
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

  // Acceleration assembly references V/W up to row N+1 (one beyond the
  // coefficient table). Compute kNMax + 2 rows so the gradient code can
  // share the same table.
  const VW kVW = compute_vw(r_bf, kRref, kNMax + 2);
  const auto& v_buf = kVW.v;
  const auto& w_buf = kVW.w;

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

// ----------------------------------------------------------------------------
// Phase-1A §C2: analytical Jacobian via Cunningham gradient.
//
// The body-fixed Jacobian J = ∂a_bf/∂r_bf is computed by substituting the
// V/W partial-derivative identities into the acceleration formula. The
// V/W table is the same one acceleration_body computes (up to row N+2).
//
// For an acceleration term of the form `a_x_term = α V_{n+1, m'}`, the
// chain rule gives `∂a_x_term/∂x = α * (∂V_{n+1, m'}/∂x) / R_ref`. The
// V/W partial-derivative identities (M-G eq 3.33) are coded inline as
// `d_v_dx(n', m', V, W)` etc., where `n' = n + 1` is the index appearing
// in the acceleration formula. Each helper returns the partial in units
// of `1/R_ref`; the assembly multiplies by `mu / R_ref^3`.
//
// The implementation deliberately *parallels* the acceleration loop above:
// every (n, m) term that contributes to `a` contributes three Jacobian
// columns to J. Keeping the structure isomorphic makes the mapping easy
// to audit against the acceleration code (and against M-G eq 3.33).
// ----------------------------------------------------------------------------
namespace {

// Partial of V_{n', m'} w.r.t. x (in units of 1/R_ref). `n'` is the row
// index appearing in the acceleration formula; the derivative reads V/W
// at row n'+1 (so the V/W table must be sized for at least `n' + 2` rows).
//
// All reads go through `vw_at` because the m+1 reads can stray outside the
// `m <= n` triangle for sectorial coefficients (m = n), where V/W are
// mathematically zero. Direct `vw_idx` access in those cases would alias
// onto a different (n'', m'') entry.
double d_v_dx(int np, int mp, const std::vector<double>& v, const std::vector<double>& /*w*/) {
  if (mp == 0) {
    return -vw_at(v, np + 1, 1);
  }
  // (n' - m' + 2)(n' - m' + 1)
  const auto kF = static_cast<double>((np - mp + 2) * (np - mp + 1));
  return 0.5 * (-vw_at(v, np + 1, mp + 1) + kF * vw_at(v, np + 1, mp - 1));
}

double d_v_dy(int np, int mp, const std::vector<double>& /*v*/, const std::vector<double>& w) {
  if (mp == 0) {
    return -vw_at(w, np + 1, 1);
  }
  const auto kF = static_cast<double>((np - mp + 2) * (np - mp + 1));
  return 0.5 * (-vw_at(w, np + 1, mp + 1) - kF * vw_at(w, np + 1, mp - 1));
}

double d_v_dz(int np, int mp, const std::vector<double>& v, const std::vector<double>& /*w*/) {
  // ∂V_{n,m}/∂z = -(n - m + 1) V_{n+1, m}.
  const auto kF = static_cast<double>(np - mp + 1);
  return -kF * vw_at(v, np + 1, mp);
}

double d_w_dx(int np, int mp, const std::vector<double>& /*v*/, const std::vector<double>& w) {
  if (mp == 0) {
    // W_{n,0} ≡ 0, so its derivative is 0.
    return 0.0;
  }
  const auto kF = static_cast<double>((np - mp + 2) * (np - mp + 1));
  return 0.5 * (-vw_at(w, np + 1, mp + 1) + kF * vw_at(w, np + 1, mp - 1));
}

double d_w_dy(int np, int mp, const std::vector<double>& v, const std::vector<double>& /*w*/) {
  if (mp == 0) {
    return 0.0;
  }
  const auto kF = static_cast<double>((np - mp + 2) * (np - mp + 1));
  return 0.5 * (vw_at(v, np + 1, mp + 1) + kF * vw_at(v, np + 1, mp - 1));
}

double d_w_dz(int np, int mp, const std::vector<double>& /*v*/, const std::vector<double>& w) {
  const auto kF = static_cast<double>(np - mp + 1);
  return -kF * vw_at(w, np + 1, mp);
}

}  // namespace

apsis::math::Mat3 SphericalHarmonic::partials_body(const apsis::math::Vec3& r_bf) const {
  const int kNMax = coeffs_.degree;
  const int kMMaxModel = coeffs_.order;
  const double kRref = coeffs_.r_ref;
  const double kMu = coeffs_.mu;

  // Compute V/W up to row N+2 — one extra row beyond what acceleration
  // needs, because each Jacobian element reads V/W at (n+2, m'').
  const VW kVW = compute_vw(r_bf, kRref, kNMax + 3);
  const auto& v_buf = kVW.v;
  const auto& w_buf = kVW.w;

  // Jacobian assembly. Every (n, m) coefficient term contributes the same
  // shape of expression to a_x, a_y, a_z — and applying ∂/∂x_k to that
  // expression by linearity inserts d_v_dxk and d_w_dxk for every V_{n+1,*}
  // and W_{n+1,*} that appears.
  //
  // For each output row i ∈ {x, y, z} and each input column k ∈ {x, y, z}
  // we accumulate a sum that is structurally identical to the acceleration
  // sum, but with V_{n+1,*} -> dV_{n+1,*}/dx_k (and similarly W).
  apsis::math::Mat3 j_bf = apsis::math::Mat3::Zero();

  // The acceleration's leading factor is mu / R_ref^2; each ∂/∂x_k brings
  // in a `1/R_ref` factor (V/W are dimensionless functions of R_ref/r), so
  // the Jacobian's leading factor is mu / R_ref^3.
  const double kMuR3 = kMu / (kRref * kRref * kRref);

  for (int n = 0; n <= kNMax; ++n) {
    const int kMMax = std::min(n, kMMaxModel);
    for (int m = 0; m <= kMMax; ++m) {
      const double kC = coeffs_.c_norm[Coefficients::idx(n, m)];
      const double kS = coeffs_.s_norm[Coefficients::idx(n, m)];

      // We need ∂(a_x_term)/∂x_k, ∂(a_y_term)/∂x_k, ∂(a_z_term)/∂x_k for
      // k ∈ {x, y, z}. Each of those is the acceleration assembly with
      // the V_{n+1,*}, W_{n+1,*} entries replaced by their k-th partial.
      // Loop over k and reuse the assembly.
      for (int k = 0; k < 3; ++k) {
        // Pick the V/W partial functions for this k.
        // kNp = n + 1 (the row appearing in the acceleration formula).
        const int kNp = n + 1;
        double dvm1{};
        double dvm{};
        double dvp1{};
        double dwm1{};
        double dwm{};
        double dwp1{};
        if (k == 0) {  // x
          if (m >= 1) {
            dvm1 = d_v_dx(kNp, m - 1, v_buf, w_buf);
            dwm1 = d_w_dx(kNp, m - 1, v_buf, w_buf);
          }
          dvm = d_v_dx(kNp, m, v_buf, w_buf);
          dwm = d_w_dx(kNp, m, v_buf, w_buf);
          dvp1 = d_v_dx(kNp, m + 1, v_buf, w_buf);
          dwp1 = d_w_dx(kNp, m + 1, v_buf, w_buf);
        } else if (k == 1) {  // y
          if (m >= 1) {
            dvm1 = d_v_dy(kNp, m - 1, v_buf, w_buf);
            dwm1 = d_w_dy(kNp, m - 1, v_buf, w_buf);
          }
          dvm = d_v_dy(kNp, m, v_buf, w_buf);
          dwm = d_w_dy(kNp, m, v_buf, w_buf);
          dvp1 = d_v_dy(kNp, m + 1, v_buf, w_buf);
          dwp1 = d_w_dy(kNp, m + 1, v_buf, w_buf);
        } else {  // z
          if (m >= 1) {
            dvm1 = d_v_dz(kNp, m - 1, v_buf, w_buf);
            dwm1 = d_w_dz(kNp, m - 1, v_buf, w_buf);
          }
          dvm = d_v_dz(kNp, m, v_buf, w_buf);
          dwm = d_w_dz(kNp, m, v_buf, w_buf);
          dvp1 = d_v_dz(kNp, m + 1, v_buf, w_buf);
          dwp1 = d_w_dz(kNp, m + 1, v_buf, w_buf);
        }

        double dax = 0.0;
        double day = 0.0;
        double daz = 0.0;
        if (m == 0) {
          // a_x_n0 = -C V_{n+1, 1};  a_y_n0 = -C W_{n+1, 1}.
          // The helpers are indexed by m' (the entry being differentiated),
          // not by the (n, m) coefficient loop variable. When m == 0, the
          // V_{n+1, 1} in the acceleration formula is at m'=1, not at the
          // m=0 helper index, so re-evaluate explicitly.
          double dv_np_1{};
          double dw_np_1{};
          if (k == 0) {
            dv_np_1 = d_v_dx(kNp, 1, v_buf, w_buf);
            dw_np_1 = d_w_dx(kNp, 1, v_buf, w_buf);
          } else if (k == 1) {
            dv_np_1 = d_v_dy(kNp, 1, v_buf, w_buf);
            dw_np_1 = d_w_dy(kNp, 1, v_buf, w_buf);
          } else {
            dv_np_1 = d_v_dz(kNp, 1, v_buf, w_buf);
            dw_np_1 = d_w_dz(kNp, 1, v_buf, w_buf);
          }
          dax = -kC * dv_np_1;
          day = -kC * dw_np_1;
        } else {
          const double kFactor = 0.5 * static_cast<double>((n - m + 1) * (n - m + 2));
          // a_x_nm = 0.5 [-C V_{n+1,m+1} - S W_{n+1,m+1}
          //             + kFactor * (C V_{n+1,m-1} + S W_{n+1,m-1})]
          // ∂a_x_nm/∂xk = 0.5 [-C dV_{n+1,m+1}/dxk - S dW_{n+1,m+1}/dxk
          //              + kFactor * (C dV_{n+1,m-1}/dxk + S dW_{n+1,m-1}/dxk)]
          dax = 0.5 * (-kC * dvp1 - kS * dwp1 + kFactor * (kC * dvm1 + kS * dwm1));
          day = 0.5 * (-kC * dwp1 + kS * dvp1 + kFactor * (-kC * dwm1 + kS * dvm1));
        }
        // a_z_nm = -(n-m+1) (C V_{n+1,m} + S W_{n+1,m})
        // ∂a_z_nm/∂xk = -(n-m+1) (C dV_{n+1,m}/dxk + S dW_{n+1,m}/dxk)
        const auto kNm1 = static_cast<double>(n - m + 1);
        daz = -kNm1 * (kC * dvm + kS * dwm);

        j_bf(0, k) += dax;
        j_bf(1, k) += day;
        j_bf(2, k) += daz;
      }
    }
  }

  return kMuR3 * j_bf;
}

apsis::math::Vec3
SphericalHarmonic::acceleration(apsis::time::Time<apsis::time::tags::TT> t,
                                const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  // Phase-1A §C1: rotate ICRF position to body-fixed (ITRS), evaluate
  // Cunningham V/W in body-fixed, rotate the resulting acceleration back
  // to ICRF. The rotation matrix is orthogonal so its inverse is its
  // transpose.
  const apsis::math::Mat3 kR = apsis::frames::icrf_to_itrs_rotation(t, *eop_);
  const apsis::math::Vec3 kRBf = kR * x.r;
  const apsis::math::Vec3 kABf = acceleration_body(kRBf);
  return kR.transpose() * kABf;
}

apsis::math::Mat36
SphericalHarmonic::partials_dadx(apsis::time::Time<apsis::time::tags::TT> t,
                                 const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  // Phase-1A §C2: analytical Cunningham gradient in body-fixed. Conjugate
  // by the body-fixed rotation to obtain the ICRF Jacobian:
  //   J_icrf = R_BF<-ICRF^T · J_bf · R_BF<-ICRF.
  // No velocity dependence -> cols 3..5 stay zero.
  const apsis::math::Mat3 kR = apsis::frames::icrf_to_itrs_rotation(t, *eop_);
  const apsis::math::Vec3 kRBf = kR * x.r;
  const apsis::math::Mat3 kJBf = partials_body(kRBf);
  apsis::math::Mat36 jac = apsis::math::Mat36::Zero();
  jac.block<3, 3>(0, 0) = kR.transpose() * kJBf * kR;
  return jac;
}

}  // namespace apsis::force
