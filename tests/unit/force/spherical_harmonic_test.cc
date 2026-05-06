// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-PHY-003, REQ-PHY-004, REQ-PHY-016
//
// Phase-1 §5: SphericalHarmonic adapter unit test. Verifies:
//   * The deg=0/order=0 limit (only C_{0,0} = 1) reproduces point-mass
//     acceleration to machine epsilon.
//   * The J2-only case (C_{2,0} = -J2 normalised) gives a J2 acceleration
//     of the right sign and order of magnitude on a polar position.

#include <gtest/gtest.h>

#include <cmath>

#include "apsis/force/spherical_harmonic.h"

namespace af = apsis::force;
namespace at = apsis::time;
namespace afr = apsis::frames;

namespace {

constexpr double kMuEarth = 3.986004418e14;
constexpr double kReEarth = 6378136.3;         // m
constexpr double kJ2 = 1.0826266835531513e-3;  // EGM2008 zonal J2

af::SphericalHarmonic::Coefficients make_point_mass() {
  af::SphericalHarmonic::Coefficients c;
  c.degree = 0;
  c.order = 0;
  c.mu = kMuEarth;
  c.R = kReEarth;
  // (degree+1)(degree+2)/2 = 1 entry
  c.C_norm = {1.0};
  c.S_norm = {0.0};
  return c;
}

af::SphericalHarmonic::Coefficients make_j2_only() {
  af::SphericalHarmonic::Coefficients c;
  c.degree = 2;
  c.order = 0;
  c.mu = kMuEarth;
  c.R = kReEarth;
  // Triangular size = 6 entries: (0,0), (1,0), (1,1), (2,0), (2,1), (2,2)
  c.C_norm.assign(6, 0.0);
  c.S_norm.assign(6, 0.0);
  c.C_norm[0] = 1.0;  // C_{0,0}
  // C_{2,0} normalised: -J2 / sqrt(5).
  c.C_norm[3] = -kJ2 / std::sqrt(5.0);
  return c;
}

TEST(SphericalHarmonic, DegreeZeroEqualsPointMass) {
  af::SphericalHarmonic sh(make_point_mass());
  afr::State<afr::tags::ICRF> x;
  x.r << 7.0e6, 1.0e6, 5.0e5;
  const auto a = sh.acceleration(at::Time<at::tags::TT>{}, x);
  // Reference: point-mass.
  const double r = x.r.norm();
  const auto a_ref = -(kMuEarth / (r * r * r)) * x.r;
  EXPECT_NEAR((a - a_ref).norm() / a_ref.norm(), 0.0, 1e-12);
}

TEST(SphericalHarmonic, J2OnlySignAndMagnitude) {
  af::SphericalHarmonic sh(make_j2_only());
  // Polar position: r = (0, 0, R + 400 km). J2 perturbation should be
  // ~ -J2 * (3/2) * mu/r^2 * (R/r)^2 along +z (yes, J2 acceleration at the
  // pole points outward — the bulge is at the equator, so the polar
  // observer is "above" less mass).
  afr::State<afr::tags::ICRF> x;
  x.r << 0.0, 0.0, kReEarth + 400e3;
  const auto a = sh.acceleration(at::Time<at::tags::TT>{}, x);
  // Subtract central component to isolate J2:
  af::SphericalHarmonic pm(make_point_mass());
  const auto a_pm = pm.acceleration(at::Time<at::tags::TT>{}, x);
  const auto a_j2 = a - a_pm;
  // J2 perturbation at the pole: a_z = -(3 J2) (mu/r^2) (R/r)^2 e_z, since
  // the spherical-harmonic factor (3 sin^2(lat) - 1) at the pole is 2 and
  // the leading factor is 3/2. The acceleration points OUTWARD at the
  // pole (away from the equatorial bulge).
  const double r = x.r.norm();
  const double mag_ref = 3.0 * kJ2 * (kMuEarth / (r * r)) * std::pow(kReEarth / r, 2);
  EXPECT_NEAR(a_j2.norm(), mag_ref, mag_ref * 5e-2) << "actual=" << a_j2.transpose();
  // Direction: +z (outward at north pole).
  EXPECT_GT(a_j2.z(), 0.0);
}

TEST(SphericalHarmonic, PartialsSelfConsistent) {
  af::SphericalHarmonic sh(make_j2_only());
  afr::State<afr::tags::ICRF> x;
  x.r << 7.0e6, 1.0e6, 5.0e5;
  const auto J = sh.partials(at::Time<at::tags::TT>{}, x);
  // Independent oracle at h = 10 m (adapter uses h = 1 m internally).
  constexpr double h = 10.0;
  for (int i = 0; i < 3; ++i) {
    auto xp = x;
    auto xm = x;
    xp.r[i] += h;
    xm.r[i] -= h;
    const auto ap = sh.acceleration(at::Time<at::tags::TT>{}, xp);
    const auto am = sh.acceleration(at::Time<at::tags::TT>{}, xm);
    const auto col = (ap - am) / (2.0 * h);
    for (int row = 0; row < 3; ++row) {
      EXPECT_NEAR(J(row, i), col[row], 1e-7) << "row=" << row << " col=" << i;
    }
  }
}

}  // namespace
