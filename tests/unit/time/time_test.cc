// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-TIME-001, REQ-TIME-002, REQ-TIME-013
//
// Phase-1 §2: tests for `Time<Scale>`, `Duration`, and the SOFA-mediated
// `convert<>` graph. Round-trip tests assert that going A -> B -> A
// recovers the original instant to better than 100 ns (one part in 1e12 of
// a day at jd2 ~ 0.5 — well within IEEE double precision and SOFA's stated
// accuracy bounds).

#include <gtest/gtest.h>

#include <cmath>

#include "apsis/time/convert.h"
#include "apsis/time/duration.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace at = apsis::time;

namespace {

// Two-component absolute difference in seconds: returns the seconds-equivalent
// of the |a - b| difference without losing precision to an early sum.
double diff_seconds(double a1, double a2, double b1, double b2) {
  return ((a1 - b1) + (a2 - b2)) * at::kSecondsPerDay;
}

constexpr double kRoundTripTolSec = 1e-7;  // 100 ns

TEST(Duration, BasicArithmetic) {
  at::Duration d1{10.0};
  at::Duration d2{2.5};
  EXPECT_DOUBLE_EQ((d1 + d2).seconds(), 12.5);
  EXPECT_DOUBLE_EQ((d1 - d2).seconds(), 7.5);
  EXPECT_DOUBLE_EQ((-d1).seconds(), -10.0);
  EXPECT_TRUE(d2 < d1);
  EXPECT_TRUE(d1 == at::Duration{10.0});
}

TEST(Time, AddDurationScalePreserving) {
  // Pick a 2025-01-01 12:00:00 TT epoch as an integer-jd1 + 0.0-jd2 split.
  // Per SOFA convention, JD = 2460676.0 corresponds to 2024-12-31 12:00 TT;
  // the actual numeric date doesn't matter for arithmetic checks.
  at::Time<at::tags::TT> t0{2460676.0, 0.0};
  at::Time<at::tags::TT> t1 = t0 + at::Duration{60.0};
  EXPECT_NEAR((t1 - t0).seconds(), 60.0, 1e-12);
  EXPECT_DOUBLE_EQ(t1.jd1(), t0.jd1());
  EXPECT_NEAR(t1.jd2() - t0.jd2(), 60.0 / at::kSecondsPerDay, 1e-15);
}

TEST(Time, SubtractionYieldsDurationSameScale) {
  at::Time<at::tags::TAI> a{2460676.0, 0.5};
  at::Time<at::tags::TAI> b{2460676.0, 0.0};
  EXPECT_NEAR((a - b).seconds(), 0.5 * at::kSecondsPerDay, 1e-9);
}

TEST(Convert, TaiTtRoundTrip) {
  at::Time<at::tags::TAI> tai0{2460676.5, 0.0};
  auto tt = at::convert<at::tags::TT>(tai0);
  auto tai1 = at::convert<at::tags::TAI>(tt);
  EXPECT_LT(std::abs(diff_seconds(tai0.jd1(), tai0.jd2(), tai1.jd1(), tai1.jd2())),
            kRoundTripTolSec);
}

TEST(Convert, TaiUtcRoundTrip) {
  // 2025-01-01 00:00:00 TAI is well clear of any leap second. iauTaiutc /
  // iauUtctai will round-trip exactly modulo internal SOFA precision.
  at::Time<at::tags::TAI> tai0{2460676.5, 0.0};
  auto utc  = at::convert<at::tags::UTC>(tai0);
  auto tai1 = at::convert<at::tags::TAI>(utc);
  EXPECT_LT(std::abs(diff_seconds(tai0.jd1(), tai0.jd2(), tai1.jd1(), tai1.jd2())),
            kRoundTripTolSec);
}

TEST(Convert, UtcUt1RoundTrip) {
  at::set_default_dut1(0.04);  // arbitrary representative DUT1, ~40 ms
  at::Time<at::tags::UTC> utc0{2460676.5, 0.0};
  auto ut1  = at::convert<at::tags::UT1>(utc0);
  auto utc1 = at::convert<at::tags::UTC>(ut1);
  EXPECT_LT(std::abs(diff_seconds(utc0.jd1(), utc0.jd2(), utc1.jd1(), utc1.jd2())),
            kRoundTripTolSec);
  // UT1 - UTC should equal default_dut1 (modulo internal SOFA noise).
  const double diff = diff_seconds(ut1.jd1(), ut1.jd2(), utc0.jd1(), utc0.jd2());
  EXPECT_NEAR(diff, 0.04, 1e-9);
  at::set_default_dut1(0.0);
}

TEST(Convert, TtTdbRoundTrip) {
  at::Time<at::tags::TT> tt0{2460676.5, 0.0};
  auto tdb = at::convert<at::tags::TDB>(tt0);
  auto tt1 = at::convert<at::tags::TT>(tdb);
  // TT/TDB difference is ≤ ~1.6 ms periodic; round-trip must be MUCH tighter.
  EXPECT_LT(std::abs(diff_seconds(tt0.jd1(), tt0.jd2(), tt1.jd1(), tt1.jd2())),
            kRoundTripTolSec);
}

TEST(Convert, ComposedUtcToTtMatchesViaTai) {
  at::Time<at::tags::UTC> utc0{2460676.5, 0.0};
  auto tt_direct   = at::convert<at::tags::TT>(utc0);
  auto tt_via_tai  = at::convert<at::tags::TT>(at::convert<at::tags::TAI>(utc0));
  EXPECT_DOUBLE_EQ(tt_direct.jd1(), tt_via_tai.jd1());
  EXPECT_DOUBLE_EQ(tt_direct.jd2(), tt_via_tai.jd2());
}

TEST(Convert, IdentityIsNoop) {
  at::Time<at::tags::TT> tt0{2460676.5, 0.123456};
  auto tt1 = at::convert<at::tags::TT, at::tags::TT>(tt0);
  EXPECT_DOUBLE_EQ(tt1.jd1(), tt0.jd1());
  EXPECT_DOUBLE_EQ(tt1.jd2(), tt0.jd2());
}

TEST(Convert, TaiTtFixedOffset) {
  // TT - TAI = 32.184 s (fixed by IAU 1991 Resolution A4). To recover the
  // offset without losing precision to a single sum, we use the same
  // partition SOFA uses internally: iauTaitt fixes jd1 == tai.jd1(), and
  // packs the offset into jd2. So (tt.jd2() - tai.jd2()) * 86400 is the
  // offset in seconds at full precision.
  at::Time<at::tags::TAI> tai{2460676.5, 0.0};
  auto tt = at::convert<at::tags::TT>(tai);
  EXPECT_DOUBLE_EQ(tt.jd1(), tai.jd1());
  EXPECT_NEAR((tt.jd2() - tai.jd2()) * at::kSecondsPerDay, 32.184, 1e-9);
}

}  // namespace
