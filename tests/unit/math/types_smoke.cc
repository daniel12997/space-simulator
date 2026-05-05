// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: none
//
// Phase-1 §1: smoke test for apsis::math aliases. Confirms the alias header
// compiles, that the canonical shapes zero-init, and that the basic ops we
// rely on (cross product, normalisation, quaternion rotation) behave as
// expected for the Eigen instantiations we expose.

#include <gtest/gtest.h>

#include "apsis/math/types.h"

namespace {

constexpr double kEps = 1e-12;

TEST(MathTypesSmoke, ZeroInit) {
  apsis::math::Vec3 v3;
  v3.setZero();
  EXPECT_DOUBLE_EQ(v3.norm(), 0.0);

  apsis::math::Vec6 v6;
  v6.setZero();
  EXPECT_DOUBLE_EQ(v6.norm(), 0.0);

  apsis::math::Mat3 m3 = apsis::math::Mat3::Identity();
  EXPECT_DOUBLE_EQ(m3.trace(), 3.0);

  apsis::math::Mat6 m6 = apsis::math::Mat6::Identity();
  EXPECT_DOUBLE_EQ(m6.trace(), 6.0);

  apsis::math::Mat36 m36 = apsis::math::Mat36::Zero();
  EXPECT_DOUBLE_EQ(m36.norm(), 0.0);
}

TEST(MathTypesSmoke, CrossProduct) {
  const apsis::math::Vec3 ex(1.0, 0.0, 0.0);
  const apsis::math::Vec3 ey(0.0, 1.0, 0.0);
  const apsis::math::Vec3 ez = ex.cross(ey);
  EXPECT_NEAR(ez.x(), 0.0, kEps);
  EXPECT_NEAR(ez.y(), 0.0, kEps);
  EXPECT_NEAR(ez.z(), 1.0, kEps);
}

TEST(MathTypesSmoke, Normalize) {
  apsis::math::Vec3 v(3.0, 4.0, 0.0);
  v.normalize();
  EXPECT_NEAR(v.norm(), 1.0, kEps);
}

TEST(MathTypesSmoke, QuatRotate) {
  // 90 deg about +z: (1,0,0) -> (0,1,0)
  const apsis::math::Quat q(Eigen::AngleAxisd(M_PI / 2.0, apsis::math::Vec3::UnitZ()));
  const apsis::math::Vec3 ex(1.0, 0.0, 0.0);
  const apsis::math::Vec3 r = q * ex;
  EXPECT_NEAR(r.x(), 0.0, kEps);
  EXPECT_NEAR(r.y(), 1.0, kEps);
  EXPECT_NEAR(r.z(), 0.0, kEps);
}

}  // namespace
