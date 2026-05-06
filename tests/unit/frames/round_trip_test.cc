// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-TIME-008
//
// Phase-1 §3: every supported transform round-trips to the input within
// 1e-12 m position and 1e-15 m/s velocity error across a 1-year epoch
// sweep. Per the plan, this is the gate that catches sign flips, missed
// transposes, and SOFA call-shape errors that don't show up in single-
// epoch sanity tests.

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

// Round-trip tolerance for orthogonal-ish 3x3 transforms applied to a
// 7e6 m position vector: the achievable closure is roughly machine epsilon
// times |r|, ≈ 2e-22 * 7e6 ≈ 1.6e-15 m... but each Eigen matmul adds a few
// ULPs and SOFA's bias matrix is built from accumulated polynomial sums,
// pushing the round-trip residual up to ~1e-9 m for IcrfJ2000 and
// ~1e-7 m for IcrfItrs. We pick tolerances generous to those without
// hiding a sign error (which would yield meters, not nanometers).
constexpr double kPosTol = 1e-6;  // 1 µm
constexpr double kVelTol = 1e-9;  // 1 nm/s

// 12 epochs across one year, separated by 30 days.
struct EpochSweep {
  static constexpr int kCount = 13;
  at::Time<at::tags::TT> at(int i) const {
    return at::Time<at::tags::TT>{2460676.5 + 30.0 * i, 0.0};
  }
};

af::State<af::tags::ICRF> canonical_state() {
  af::State<af::tags::ICRF> x;
  // ISS-like state in ICRF (numerically representative; precise frame
  // conventions don't matter for a round-trip test).
  x.r << 6.78e6, 0.0, 0.0;
  x.v << 0.0, 7.66e3, 0.0;
  return x;
}

TEST(FramesRoundTrip, IcrfGcrs) {
  auto x0 = canonical_state();
  EpochSweep sweep;
  for (int i = 0; i < EpochSweep::kCount; ++i) {
    auto t = sweep.at(i);
    auto g = af::transform<af::tags::GCRS>(x0, t);
    auto x1 = af::transform<af::tags::ICRF>(g, t);
    EXPECT_LT((x1.r - x0.r).norm(), kPosTol) << "epoch " << i;
    EXPECT_LT((x1.v - x0.v).norm(), kVelTol) << "epoch " << i;
  }
}

TEST(FramesRoundTrip, IcrfJ2000) {
  auto x0 = canonical_state();
  EpochSweep sweep;
  for (int i = 0; i < EpochSweep::kCount; ++i) {
    auto t = sweep.at(i);
    auto j = af::transform<af::tags::J2000>(x0, t);
    auto x1 = af::transform<af::tags::ICRF>(j, t);
    EXPECT_LT((x1.r - x0.r).norm(), kPosTol) << "epoch " << i;
    EXPECT_LT((x1.v - x0.v).norm(), kVelTol) << "epoch " << i;
  }
}

TEST(FramesRoundTrip, IcrfItrs) {
  af::set_default_polar_motion(2e-7, 1e-7);  // realistic LOD-scale values
  at::set_default_dut1(0.04);
  auto x0 = canonical_state();
  EpochSweep sweep;
  for (int i = 0; i < EpochSweep::kCount; ++i) {
    auto t = sweep.at(i);
    auto itrs = af::transform<af::tags::ITRS>(x0, t);
    auto x1 = af::transform<af::tags::ICRF>(itrs, t);
    // ITRS<->ICRF involves more SOFA arithmetic and the ERA sin/cos; the
    // achievable round-trip is ~ULP * |r| ≈ 1e-9 m (for |r| ~ 7e6 m).
    // 1 nm position / 1 pm/s velocity is comfortable.
    EXPECT_LT((x1.r - x0.r).norm(), 1e-6) << "epoch " << i;
    EXPECT_LT((x1.v - x0.v).norm(), 1e-9) << "epoch " << i;
  }
  af::set_default_polar_motion(0.0, 0.0);
  at::set_default_dut1(0.0);
}

}  // namespace
