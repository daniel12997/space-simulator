// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-TIME-004, REQ-TIME-008
//
// Phase-1A §B1: tests for `EopTable::load_from_csv` and `query`.
//
// Coverage:
//   * Loading the bundled `data/iers_eop_phase1.csv` succeeds and returns
//     the expected number of rows (40 data rows post-comment-stripping).
//   * `query` at an exact-row epoch returns that row's values to <1 µas
//     (polar) / <1 µs (DUT1) per the plan's Verify section.
//   * `query` between rows returns the linear interpolant.
//   * Out-of-range queries clamp to the nearest endpoint.
//   * Malformed CSV rejection: bad column count, non-numeric fields,
//     out-of-order rows, and fewer-than-two rows all throw.

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "apsis/time/convert.h"
#include "apsis/time/eop_table.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace at = apsis::time;

namespace {

#ifndef APSIS_DATA_DIR
#error "APSIS_DATA_DIR must be defined"
#endif

constexpr double kArcsecToRad = 4.8481368110953599358991e-6;

// "Known row" from the bundled `data/iers_eop_phase1.csv`:
//   MJD 60676,-0.038,0.040,0.330  (dut1 s, xp arcsec, yp arcsec)
// We construct a TT epoch that maps back to UTC MJD 60676.0 by composing
// the leap-second offset and the TT-TAI offset. UTC JD 2460676.5 +
// (37 + 32.184)s = 2460676.5 + 0.000800740... days.
constexpr double kKnownMjdUtc = 60676.0;
constexpr double kKnownDut1 = -0.038;            // seconds
constexpr double kKnownXpArcsec = 0.040;
constexpr double kKnownYpArcsec = 0.330;

at::Time<at::tags::TT> tt_at_mjd_utc(double mjd_utc) {
  // Round-trip via SOFA: build a UTC instant at the requested MJD, then
  // convert to TT. This avoids hard-coding the leap-second count and is
  // the same path `query` uses internally to map TT back to MJD.
  const double jd_utc1 = mjd_utc + 2400000.5;
  return at::convert<at::tags::TT>(at::Time<at::tags::UTC>{jd_utc1, 0.0});
}

TEST(EopTable, LoadBundledCsv) {
  const std::filesystem::path csv =
      std::filesystem::path(APSIS_DATA_DIR) / "iers_eop_phase1.csv";
  ASSERT_TRUE(std::filesystem::exists(csv)) << "missing " << csv;
  auto eop = at::EopTable::load_from_csv(csv);
  // 41 data rows (lines 16..56 of the bundled CSV; lines 1..15 are
  // comments and the trailing blank line is skipped).
  EXPECT_EQ(eop.size(), 41u);
  // First row: MJD 60676.
  EXPECT_DOUBLE_EQ(eop.rows().front().mjd_utc, 60676.0);
  EXPECT_DOUBLE_EQ(eop.rows().front().dut1, -0.038);
  // Arcseconds-to-radians conversion happened on load.
  EXPECT_NEAR(eop.rows().front().polar_xp, 0.040 * kArcsecToRad, 1e-15);
  EXPECT_NEAR(eop.rows().front().polar_yp, 0.330 * kArcsecToRad, 1e-15);
}

TEST(EopTable, QueryAtKnownRow) {
  const std::filesystem::path csv =
      std::filesystem::path(APSIS_DATA_DIR) / "iers_eop_phase1.csv";
  auto eop = at::EopTable::load_from_csv(csv);

  const auto tt = tt_at_mjd_utc(kKnownMjdUtc);
  const at::EopValues v = eop.query(tt);

  // Plan tolerance: <1 µs DUT1, <1 µas polar.
  EXPECT_NEAR(v.dut1, kKnownDut1, 1e-6);
  EXPECT_NEAR(v.polar_xp, kKnownXpArcsec * kArcsecToRad, 1e-6 * kArcsecToRad);
  EXPECT_NEAR(v.polar_yp, kKnownYpArcsec * kArcsecToRad, 1e-6 * kArcsecToRad);
}

TEST(EopTable, QueryInterpolatesLinearly) {
  // Two-row synthetic table. Pick an MJD halfway between rows; expect the
  // arithmetic mean.
  std::vector<at::EopRow> rows{
      {/*mjd=*/60000.0, /*dut1=*/-0.10, /*xp=*/0.10, /*yp=*/0.20},
      {/*mjd=*/60100.0, /*dut1=*/0.10, /*xp=*/0.30, /*yp=*/0.40},
  };
  at::EopTable eop(std::move(rows));
  const auto tt = tt_at_mjd_utc(60050.0);
  const at::EopValues v = eop.query(tt);
  EXPECT_NEAR(v.dut1, 0.0, 1e-9);
  EXPECT_NEAR(v.polar_xp, 0.20, 1e-9);
  EXPECT_NEAR(v.polar_yp, 0.30, 1e-9);
}

TEST(EopTable, QueryClampsBelowRange) {
  std::vector<at::EopRow> rows{
      {60000.0, 0.05, 1e-7, 2e-7},
      {60100.0, 0.06, 1.5e-7, 2.5e-7},
  };
  at::EopTable eop(std::move(rows));
  const auto tt = tt_at_mjd_utc(50000.0);  // far before
  const at::EopValues v = eop.query(tt);
  EXPECT_DOUBLE_EQ(v.dut1, 0.05);
  EXPECT_DOUBLE_EQ(v.polar_xp, 1e-7);
  EXPECT_DOUBLE_EQ(v.polar_yp, 2e-7);
}

TEST(EopTable, QueryClampsAboveRange) {
  std::vector<at::EopRow> rows{
      {60000.0, 0.05, 1e-7, 2e-7},
      {60100.0, 0.06, 1.5e-7, 2.5e-7},
  };
  at::EopTable eop(std::move(rows));
  const auto tt = tt_at_mjd_utc(70000.0);  // far after
  const at::EopValues v = eop.query(tt);
  EXPECT_DOUBLE_EQ(v.dut1, 0.06);
  EXPECT_DOUBLE_EQ(v.polar_xp, 1.5e-7);
  EXPECT_DOUBLE_EQ(v.polar_yp, 2.5e-7);
}

TEST(EopTable, RejectsTooFewRows) {
  std::vector<at::EopRow> rows{{60000.0, 0.0, 0.0, 0.0}};
  EXPECT_THROW(at::EopTable(std::move(rows)), std::invalid_argument);
}

TEST(EopTable, RejectsOutOfOrderRows) {
  std::vector<at::EopRow> rows{
      {60100.0, 0.0, 0.0, 0.0},
      {60000.0, 0.0, 0.0, 0.0},
  };
  EXPECT_THROW(at::EopTable(std::move(rows)), std::invalid_argument);
}

// Helper: write a string to a temp CSV path, return the path. Cleaned up
// at end of TEST scope via RAII; we leak the file otherwise (the temp
// directory churns naturally).
std::filesystem::path write_temp_csv(const std::string& body) {
  const auto path = std::filesystem::temp_directory_path() /
                    ("apsis_eop_test_" + std::to_string(::getpid()) + ".csv");
  std::ofstream out(path);
  out << body;
  return path;
}

TEST(EopTable, LoadRejectsBadColumnCount) {
  const auto path = write_temp_csv("# comment\n60000,0.0,0.0\n60100,0.0,0.0\n");
  EXPECT_THROW(at::EopTable::load_from_csv(path), std::runtime_error);
  std::filesystem::remove(path);
}

TEST(EopTable, LoadRejectsNonNumeric) {
  const auto path = write_temp_csv("60000,abc,0.0,0.0\n60100,0.0,0.0,0.0\n");
  EXPECT_THROW(at::EopTable::load_from_csv(path), std::runtime_error);
  std::filesystem::remove(path);
}

TEST(EopTable, LoadRejectsTooFewRows) {
  const auto path = write_temp_csv("# only header\n60000,0.0,0.0,0.0\n");
  EXPECT_THROW(at::EopTable::load_from_csv(path), std::runtime_error);
  std::filesystem::remove(path);
}

TEST(EopTable, LoadRejectsMissingFile) {
  EXPECT_THROW(
      at::EopTable::load_from_csv("/nonexistent/path/eop.csv"),
      std::runtime_error);
}

}  // namespace
