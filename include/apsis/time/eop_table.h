// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §B1: `EopTable` — IERS Earth Orientation Parameters table threaded
// as a `const EopTable&` parameter through `convert<UT1, *>` and
// `transform<ITRS, ICRF>` (and inverses), replacing the Phase-1
// process-wide DUT1 + polar-motion globals.
//
// The motivating constraint is Phase 5 Monte Carlo determinism: globals are
// a footgun for concurrent trials and a hash-iteration-style nondeterminism
// surface. Threading the table through the API parameter at the small Phase 1
// call surface (~6 specialisations) is cheap now and forecloses a cleanup
// debt later. See ADR-008 (CSPICE/SOFA seam — the EOP feeds SOFA-mediated
// transforms) and ADR-010 (phantom-typed Time/State; the table is keyed on
// `Time<TT>`).
//
// Phase 1A scope: daily-cadence CSV, linear interpolation between rows.
// REQ-TIME-004 ("IERS Bulletin A polar motion and DUT1 over the mission
// interval") is the closing requirement; the live-fetch tooling against
// datacenter.iers.org remains a Phase 7 deliverable per
// `data/iers_eop_phase1.csv`'s disclaimer.

#pragma once

#include <filesystem>
#include <vector>

#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace apsis::time {

// EOP triple at a given epoch.
//
//   dut1     = UT1 - UTC, seconds. Bulletin A range is roughly [-1, +1] s.
//   polar_xp = polar motion x-coordinate, RADIANS (the CSV stores arcsec;
//              `load_from_csv` converts on ingest).
//   polar_yp = polar motion y-coordinate, RADIANS.
//
// All three are sub-microradian / sub-second IERS quantities; storing them
// as plain `double` is precision-overkill, which is what we want.
struct EopValues {
  double dut1{};
  double polar_xp{};
  double polar_yp{};
};

// One CSV row, post-conversion (xp/yp in radians, mjd_utc in days).
//
// Kept in the public header so `EopTable` is not pimpl'd; consumers
// (e.g. unit tests) may want to inspect `rows().size()` for sanity-check
// assertions. The struct is `private`-by-convention — out-of-class
// construction by anyone but `EopTable::load_from_csv` is undefined, but
// not actively prevented.
struct EopRow {
  double mjd_utc{};
  double dut1{};
  double polar_xp{};
  double polar_yp{};
};

class EopTable {
 public:
  // Construct from a CSV in the bundled-format documented in
  // `data/iers_eop_phase1.csv`:
  //
  //   mjd_utc,dut1_seconds,polar_xp_arcsec,polar_yp_arcsec
  //
  // Lines whose first non-whitespace character is `#` are treated as
  // comments and skipped; blank lines are also skipped. Rows must be sorted
  // by `mjd_utc` ascending; `load_from_csv` validates this and throws
  // `std::runtime_error` if the file is malformed, contains fewer than two
  // rows, or has out-of-order MJDs.
  //
  // `polar_xp` and `polar_yp` are converted from arcseconds to radians on
  // ingest so consumers (including SOFA's `iauPom00`) see radians directly.
  static EopTable load_from_csv(const std::filesystem::path& path);

  // Construct directly from a row list (for tests). Rows must be sorted by
  // `mjd_utc` ascending and contain at least two entries; otherwise this
  // throws `std::invalid_argument`.
  explicit EopTable(std::vector<EopRow> rows);

  // Query the table at a TT epoch. The TT is internally routed through
  // `convert<UTC>(tt)` to land on a UTC instant whose two-component
  // representation maps directly to MJD; the surrounding pair of rows is
  // located by `std::lower_bound` and the three EOP scalars are linearly
  // interpolated.
  //
  // Out-of-range epochs (before the first row or after the last) clamp to
  // the nearest endpoint's values rather than extrapolate; a noisy
  // extrapolated DUT1 outside the IERS-published window would silently
  // corrupt downstream UT1 conversions, and clamping is the more
  // conservative behaviour for a Phase 1 regression-test slice.
  [[nodiscard]] EopValues query(Time<tags::TT> t) const;

  // Row accessors. Useful for tests and for sanity-check assertions in
  // calling code (e.g. "the table covers my mission window").
  [[nodiscard]] const std::vector<EopRow>& rows() const noexcept { return rows_; }
  [[nodiscard]] std::size_t size() const noexcept { return rows_.size(); }

 private:
  std::vector<EopRow> rows_;
};

}  // namespace apsis::time
