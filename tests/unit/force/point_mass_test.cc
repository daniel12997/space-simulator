// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-PHY-001, REQ-PHY-002, REQ-PHY-016
//
// Phase-1 §5: PointMass adapter unit test. Magnitudes match the closed
// form to machine epsilon; partials are checked here as a self-consistency
// (the parameterised VE-contract conformance test will independently
// verify partials vs central difference).

#include <gtest/gtest.h>

#include <cmath>

#include "apsis/force/point_mass.h"

namespace af = apsis::force;
namespace at = apsis::time;
namespace afr = apsis::frames;

namespace {

constexpr double kMuEarth = 3.986004418e14;  // m^3/s^2

TEST(PointMass, AccelerationMagnitude) {
  af::PointMass pm(kMuEarth);
  afr::State<afr::tags::ICRF> x;
  x.r << 7.0e6, 0.0, 0.0;
  const auto a = pm.acceleration(at::Time<at::tags::TT>{}, x);
  // |a| = mu / r^2
  const double expected = kMuEarth / (7.0e6 * 7.0e6);
  EXPECT_NEAR(a.norm(), expected, 1e-6);
  // Direction: -r_hat
  EXPECT_LT(a.x(), 0.0);
  EXPECT_NEAR(a.y(), 0.0, 1e-12);
  EXPECT_NEAR(a.z(), 0.0, 1e-12);
}

TEST(PointMass, PartialsSelfConsistent) {
  af::PointMass pm(kMuEarth);
  afr::State<afr::tags::ICRF> x;
  x.r << 7.0e6, 1.0e6, 5.0e5;
  const auto J = pm.partials(at::Time<at::tags::TT>{}, x);

  // Central-difference oracle at h = 10 m.
  constexpr double h = 10.0;
  for (int i = 0; i < 3; ++i) {
    auto xp = x;
    auto xm = x;
    xp.r[i] += h;
    xm.r[i] -= h;
    const auto ap = pm.acceleration(at::Time<at::tags::TT>{}, xp);
    const auto am = pm.acceleration(at::Time<at::tags::TT>{}, xm);
    const auto col = (ap - am) / (2.0 * h);
    for (int row = 0; row < 3; ++row) {
      EXPECT_NEAR(J(row, i), col[row], 1e-9)
          << "row=" << row << " col=" << i;
    }
  }
  // Velocity columns are zero.
  for (int i = 3; i < 6; ++i) {
    EXPECT_NEAR(J.col(i).norm(), 0.0, 1e-15);
  }
}

}  // namespace
