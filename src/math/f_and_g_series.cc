// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §7: f-and-g series via universal variables.

#include "f_and_g_series.h"

#include <cmath>

namespace apsis::math::fandg {
namespace {

// Stumpff functions C(z), S(z) — series form near z = 0, closed form
// elsewhere. Both formulas in Battin §4.4.5.
double stumpff_C(double z) {
  if (z > 1e-6) {
    const double sz = std::sqrt(z);
    return (1.0 - std::cos(sz)) / z;
  }
  if (z < -1e-6) {
    const double sz = std::sqrt(-z);
    return (1.0 - std::cosh(sz)) / z;
  }
  // z near zero: 1/2! - z/4! + z^2/6! - ...
  return 0.5 - z / 24.0 + z * z / 720.0;
}

double stumpff_S(double z) {
  if (z > 1e-6) {
    const double sz = std::sqrt(z);
    return (sz - std::sin(sz)) / (sz * sz * sz);
  }
  if (z < -1e-6) {
    const double sz = std::sqrt(-z);
    return (std::sinh(sz) - sz) / (sz * sz * sz);
  }
  // 1/3! - z/5! + z^2/7! - ...
  return 1.0 / 6.0 - z / 120.0 + z * z / 5040.0;
}

}  // namespace

apsis::frames::State<apsis::frames::tags::ICRF>
propagate(const apsis::frames::State<apsis::frames::tags::ICRF>& state0,
          double dt,
          double mu) {
  const auto& r0 = state0.r;
  const auto& v0 = state0.v;
  const double r0_norm = r0.norm();
  const double v0_norm_sq = v0.squaredNorm();
  const double sqrt_mu = std::sqrt(mu);

  // Specific orbital energy: alpha = -2 * energy / mu = 2/r - v^2/mu = 1/a.
  const double alpha = 2.0 / r0_norm - v0_norm_sq / mu;

  // Initial guess for universal anomaly chi (Vallado §2.3 algorithm 8).
  double chi = sqrt_mu * std::abs(alpha) * dt;
  if (alpha > 1e-9) {
    // Elliptic.
    chi = sqrt_mu * dt * alpha;
  }

  const double r0_dot_v0 = r0.dot(v0);
  // Newton-Raphson iteration on Kepler's equation in universal variables.
  for (int it = 0; it < 50; ++it) {
    const double psi = chi * chi * alpha;
    const double C = stumpff_C(psi);
    const double S = stumpff_S(psi);
    const double r = chi * chi * C
                     + (r0_dot_v0 / sqrt_mu) * chi * (1.0 - psi * S)
                     + r0_norm * (1.0 - psi * C);
    const double f_chi = (r0_dot_v0 / sqrt_mu) * chi * chi * C
                         + (1.0 - r0_norm * alpha) * chi * chi * chi * S
                         + r0_norm * chi
                         - sqrt_mu * dt;
    // Newton step: g(chi) = f_chi (already includes the -sqrt_mu*dt term);
    // g'(chi) = r. So dchi = -g(chi)/g'(chi).
    const double dchi = -f_chi / r;
    chi += dchi;
    if (std::abs(dchi) < 1e-12 * std::max(std::abs(chi), 1.0)) break;
  }

  const double psi = chi * chi * alpha;
  const double C = stumpff_C(psi);
  const double S = stumpff_S(psi);

  // Lagrange f, g, fdot, gdot (Vallado eqs 2-67, 2-68).
  const double f    = 1.0 - (chi * chi / r0_norm) * C;
  const double g    = dt - (1.0 / sqrt_mu) * chi * chi * chi * S;

  apsis::frames::State<apsis::frames::tags::ICRF> result;
  result.r = f * r0 + g * v0;
  const double r = result.r.norm();
  const double fdot = (sqrt_mu / (r * r0_norm)) * (psi * S - 1.0) * chi;
  const double gdot = 1.0 - (chi * chi / r) * C;
  result.v = fdot * r0 + gdot * v0;
  return result;
}

}  // namespace apsis::math::fandg
