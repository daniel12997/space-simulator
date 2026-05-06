// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-TIME-005, REQ-TIME-006, REQ-TIME-008
//
// Phase-1 §3: per-pair transform sanity tests. These exercise basic
// behaviour: identity overloads are exact, bias is small but nonzero, the
// CIO pipeline rotates a vector to the right side of the planet at a known
// epoch.

#include <gtest/gtest.h>

#include <cmath>

#include "apsis/frames/state.h"
#include "apsis/frames/transform.h"
#include "apsis/time/convert.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace af = apsis::frames;
namespace at = apsis::time;

namespace {

at::Time<at::tags::TT> j2000_epoch() {
  // J2000.0 is JD 2451545.0 TT.
  return at::Time<at::tags::TT>{2451545.0, 0.0};
}

TEST(Transform, IcrfGcrsIdentity) {
  af::State<af::tags::ICRF> x;
  x.r << 7000e3, 0.0, 0.0;
  x.v << 0.0, 7500.0, 0.0;
  auto g = af::transform<af::tags::GCRS>(x, j2000_epoch());
  EXPECT_DOUBLE_EQ(g.r.x(), x.r.x());
  EXPECT_DOUBLE_EQ(g.v.y(), x.v.y());
}

TEST(Transform, IcrfJ2000Bias) {
  af::State<af::tags::ICRF> x;
  x.r << 7000e3, 0.0, 0.0;
  x.v << 0.0, 7500.0, 0.0;
  auto j = af::transform<af::tags::J2000>(x, j2000_epoch());
  // Bias is ~17 mas = ~ 8e-8 rad. At |r| = 7e6 m the displacement is
  // ~0.6 m. We assert the magnitude is well-bounded.
  const double dr = (j.r - x.r).norm();
  EXPECT_LT(dr, 1.0);   // < 1 m
  EXPECT_GT(dr, 1e-3);  // > 1 mm (it is ~0.6 m, must be nonzero)
}

TEST(Transform, IcrfItrsAtJ2000RotatesVector) {
  // No EOP loaded: dut1 = 0, polar motion = 0. ERA at J2000 is documented
  // ~ 280.46 deg + small. The test asserts only that |r| is preserved
  // (rotation is orthogonal) and that the result is not equal to the input
  // (rotation is non-trivial).
  af::set_default_polar_motion(0.0, 0.0);
  at::set_default_dut1(0.0);
  af::State<af::tags::ICRF> x;
  x.r << 7000e3, 0.0, 0.0;
  x.v << 0.0, 7500.0, 0.0;
  auto itrs = af::transform<af::tags::ITRS>(x, j2000_epoch());
  EXPECT_NEAR(itrs.r.norm(), x.r.norm(), 1e-6);
  EXPECT_GT((itrs.r - x.r).norm(), 1e3);  // significantly rotated
}

TEST(Transform, TemeStubThrows) {
  af::State<af::tags::TEME> x;
  EXPECT_THROW((void)af::transform<af::tags::ITRS>(x, j2000_epoch()), std::logic_error);
}

TEST(Transform, IdentitySameFrame) {
  af::State<af::tags::ICRF> x;
  x.r << 1.0, 2.0, 3.0;
  auto y = af::transform<af::tags::ICRF>(x, j2000_epoch());
  EXPECT_DOUBLE_EQ(y.r.x(), 1.0);
  EXPECT_DOUBLE_EQ(y.r.y(), 2.0);
  EXPECT_DOUBLE_EQ(y.r.z(), 3.0);
}

}  // namespace
