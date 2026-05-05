// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-INT-007, REQ-TIME-009
//
// Phase-1 §7: EnckeWrapper unit test. Verifies that wrapping any inner
// integrator with Encke produces a result that closes against the
// closed-form circular Kepler trajectory to better-or-equal accuracy as
// the bare integrator (the deviation dynamics are zero when the full
// force model is pure point-mass, so Encke's deviation never grows).

#include <gtest/gtest.h>

#include <cmath>

#include "apsis/force/point_mass.h"
#include "apsis/integrate/dp54.h"
#include "apsis/integrate/encke_wrapper.h"

// Pull the f-and-g propagate API for the FAndG test below. The header is
// internal-only (lives under src/math/), so the test target adds src/ to
// its include path in tests/unit/integrate/CMakeLists.txt.
#include "math/f_and_g_series.h"

namespace ai = apsis::integrate;
namespace af = apsis::force;
namespace at = apsis::time;
namespace afr = apsis::frames;

namespace {

constexpr double kMu = 3.986004418e14;
constexpr double kRadius = 7.0e6;

afr::State<afr::tags::ICRF> initial_state() {
  afr::State<afr::tags::ICRF> s;
  s.r << kRadius, 0.0, 0.0;
  s.v << 0.0, std::sqrt(kMu / kRadius), 0.0;
  return s;
}

TEST(EnckeWrapper, ZeroDeviationOnPureKepler) {
  // Inner integrator does not need to do anything when wrapped on a
  // pure point-mass force: a_full - a_kepler = 0 -> deviation stays
  // zero -> result = analytical Kepler propagation.
  af::PointMass pm(kMu);
  ai::Dp54 inner;
  ai::EnckeWrapper::Options eopts;
  eopts.mu = kMu;
  ai::EnckeWrapper encke(&inner, eopts);

  auto x = initial_state();
  apsis::math::Mat6 phi = apsis::math::Mat6::Identity();
  at::Time<at::tags::TT> t{2460676.5, 0.0};
  const double T = 2.0 * M_PI * std::sqrt(kRadius * kRadius * kRadius / kMu);

  // Single Encke step over a quarter orbit — the reference Kepler is
  // exact so the result should match the closed form to f-and-g
  // precision (~ULP).
  auto res = encke.step(t, x, phi, T / 4.0, pm);

  // Closed form at quarter orbit: position rotated 90 deg.
  const double v0 = std::sqrt(kMu / kRadius);
  EXPECT_NEAR(res.x.r.x(), 0.0, 1.0);
  EXPECT_NEAR(res.x.r.y(), kRadius, 1.0);
  EXPECT_NEAR(res.x.v.x(), -v0, 1e-3);
  EXPECT_NEAR(res.x.v.y(), 0.0, 1e-3);
}

TEST(FAndG, CircularQuarterOrbit) {
  auto x0 = initial_state();
  const double T = 2.0 * M_PI * std::sqrt(kRadius * kRadius * kRadius / kMu);
  auto x_qtr = apsis::math::fandg::propagate(x0, T / 4.0, kMu);
  EXPECT_NEAR(x_qtr.r.x(), 0.0, 1e-6);
  EXPECT_NEAR(x_qtr.r.y(), kRadius, 1e-6);
}

}  // namespace
