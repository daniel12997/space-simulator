// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §7: f-and-g series via universal variables.

#include "f_and_g_series.h"

#include <cmath>

namespace apsis::math::fandg {
namespace {

// Stumpff functions C(z), S(z) — series form near z = 0, closed form
// elsewhere. Both formulas in Battin §4.4.5. Names are prefixed (rather
// than suffixed) to avoid the cspice_seam.py false-positive on `_c(`.
double c_stumpff(double z) {
  if (z > 1e-6) {
    const double kSz = std::sqrt(z);
    return (1.0 - std::cos(kSz)) / z;
  }
  if (z < -1e-6) {
    const double kSz = std::sqrt(-z);
    return (1.0 - std::cosh(kSz)) / z;
  }
  // z near zero: 1/2! - z/4! + z^2/6! - ...
  return 0.5 - z / 24.0 + z * z / 720.0;
}

double s_stumpff(double z) {
  if (z > 1e-6) {
    const double kSz = std::sqrt(z);
    return (kSz - std::sin(kSz)) / (kSz * kSz * kSz);
  }
  if (z < -1e-6) {
    const double kSz = std::sqrt(-z);
    return (std::sinh(kSz) - kSz) / (kSz * kSz * kSz);
  }
  // 1/3! - z/5! + z^2/7! - ...
  return 1.0 / 6.0 - z / 120.0 + z * z / 5040.0;
}

}  // namespace

apsis::frames::State<apsis::frames::tags::ICRF>
propagate(const apsis::frames::State<apsis::frames::tags::ICRF>& state0, double dt, double mu) {
  const auto& r0 = state0.r;
  const auto& v0 = state0.v;
  const double kR0Norm = r0.norm();
  const double kV0NormSq = v0.squaredNorm();
  const double kSqrtMu = std::sqrt(mu);

  // Specific orbital energy: alpha = -2 * energy / mu = 2/r - v^2/mu = 1/a.
  const double kAlpha = 2.0 / kR0Norm - kV0NormSq / mu;

  // Initial guess for universal anomaly chi (Vallado §2.3 algorithm 8).
  double chi = kSqrtMu * std::abs(kAlpha) * dt;
  if (kAlpha > 1e-9) {
    // Elliptic.
    chi = kSqrtMu * dt * kAlpha;
  }

  const double kR0DotV0 = r0.dot(v0);
  // Newton-Raphson iteration on Kepler's equation in universal variables.
  for (int it = 0; it < 50; ++it) {
    const double kPsi = chi * chi * kAlpha;
    const double kC = c_stumpff(kPsi);
    const double kS = s_stumpff(kPsi);
    const double kR = chi * chi * kC + (kR0DotV0 / kSqrtMu) * chi * (1.0 - kPsi * kS) +
                      kR0Norm * (1.0 - kPsi * kC);
    const double kFChi = (kR0DotV0 / kSqrtMu) * chi * chi * kC +
                         (1.0 - kR0Norm * kAlpha) * chi * chi * chi * kS + kR0Norm * chi -
                         kSqrtMu * dt;
    // Newton step: g(chi) = kFChi (already includes the -kSqrtMu*dt term);
    // g'(chi) = kR. So dchi = -g(chi)/g'(chi).
    const double kDChi = -kFChi / kR;
    chi += kDChi;
    if (std::abs(kDChi) < 1e-12 * std::max(std::abs(chi), 1.0))
      break;
  }

  const double kPsi = chi * chi * kAlpha;
  const double kC = c_stumpff(kPsi);
  const double kS = s_stumpff(kPsi);

  // Lagrange f, g, fdot, gdot (Vallado eqs 2-67, 2-68).
  const double kF = 1.0 - (chi * chi / kR0Norm) * kC;
  const double kG = dt - (1.0 / kSqrtMu) * chi * chi * chi * kS;

  apsis::frames::State<apsis::frames::tags::ICRF> result;
  result.r = kF * r0 + kG * v0;
  const double kR = result.r.norm();
  const double kFDot = (kSqrtMu / (kR * kR0Norm)) * (kPsi * kS - 1.0) * chi;
  const double kGDot = 1.0 - (chi * chi / kR) * kC;
  result.v = kFDot * r0 + kGDot * v0;
  return result;
}

}  // namespace apsis::math::fandg
