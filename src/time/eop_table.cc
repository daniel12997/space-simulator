// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §B1: `EopTable` implementation.
//
// CSV parser is hand-rolled (not Boost.Tokenizer / Abseil / etc.) because the
// dependency graph at this Phase 1 stage is intentionally minimal — SOFA,
// CSPICE, Eigen, GoogleTest — and the format is trivial: four
// comma-separated doubles per non-comment line. The parser is liberal about
// whitespace and strict about column count.
//
// Interpolation: linear between the two surrounding rows on the UTC MJD
// timeline. Daily-cadence input (the bundled slice samples ~91-day
// intervals; real Bulletin A is daily) is sufficient for Phase 1A
// regression tests. Out-of-range epochs clamp to the nearest endpoint
// rather than extrapolate — see header comment for rationale.

#include "apsis/time/eop_table.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "apsis/time/convert.h"

namespace apsis::time {
namespace {

// Arcseconds -> radians conversion (1 arcsec = π / (180 * 3600) radians).
// Spelled out as a constant so the conversion site below reads as a single
// scalar multiply rather than a chained-π/180/3600 expression.
constexpr double kArcsecToRad = 4.8481368110953599358991e-6;

// MJD - JD offset: JD = MJD + 2400000.5.
constexpr double kMjdOffsetJd = 2400000.5;

// Strip leading/trailing whitespace from a string view; return the trimmed
// substring. Used by the CSV parser to be liberal about column padding.
std::string trim(const std::string& s) {
  const auto first = s.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = s.find_last_not_of(" \t\r\n");
  return s.substr(first, last - first + 1);
}

// Split a CSV row into trimmed string fields. The bundled Phase 1 EOP CSV
// has no embedded commas / quoted fields; a simple `std::getline(', ')` on
// a stringstream is therefore sufficient.
std::vector<std::string> split_csv_row(const std::string& line) {
  std::vector<std::string> fields;
  std::stringstream ss(line);
  std::string field;
  while (std::getline(ss, field, ',')) {
    fields.push_back(trim(field));
  }
  return fields;
}

// Parse one EOP row from a four-field CSV record. Throws
// `std::runtime_error` with the offending line number embedded if the
// fields don't all parse as doubles or the count is wrong.
EopRow parse_row(const std::vector<std::string>& fields, std::size_t line_no,
                 const std::filesystem::path& path) {
  if (fields.size() != 4) {
    std::ostringstream oss;
    oss << "EopTable::load_from_csv: " << path.string() << ":" << line_no
        << ": expected 4 fields (mjd_utc,dut1_s,xp_arcsec,yp_arcsec), got " << fields.size();
    throw std::runtime_error(oss.str());
  }
  EopRow row;
  try {
    row.mjd_utc = std::stod(fields.at(0));
    row.dut1 = std::stod(fields.at(1));
    row.polar_xp = std::stod(fields.at(2)) * kArcsecToRad;
    row.polar_yp = std::stod(fields.at(3)) * kArcsecToRad;
  } catch (const std::exception& e) {
    std::ostringstream oss;
    oss << "EopTable::load_from_csv: " << path.string() << ":" << line_no
        << ": failed to parse field as double (" << e.what() << ")";
    throw std::runtime_error(oss.str());
  }
  return row;
}

}  // namespace

EopTable::EopTable(std::vector<EopRow> rows) : rows_(std::move(rows)) {
  if (rows_.size() < 2) {
    throw std::invalid_argument("EopTable: need at least two rows for interpolation");
  }
  for (std::size_t i = 1; i < rows_.size(); ++i) {
    if (!(rows_.at(i).mjd_utc > rows_.at(i - 1).mjd_utc)) {
      throw std::invalid_argument("EopTable: rows must be sorted strictly ascending by mjd_utc");
    }
  }
}

EopTable EopTable::load_from_csv(const std::filesystem::path& path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("EopTable::load_from_csv: cannot open " + path.string());
  }
  std::vector<EopRow> rows;
  std::string line;
  std::size_t line_no = 0;
  while (std::getline(in, line)) {
    ++line_no;
    const std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() == '#') {
      continue;  // comment / blank
    }
    const auto fields = split_csv_row(trimmed);
    rows.push_back(parse_row(fields, line_no, path));
  }
  if (rows.size() < 2) {
    throw std::runtime_error("EopTable::load_from_csv: " + path.string() +
                             ": fewer than two non-comment rows");
  }
  // Validate ascending order and convert to the canonical EopTable invariant.
  // We construct via the public ctor so `rows_` invariant checking lives in
  // exactly one place.
  return EopTable(std::move(rows));
}

EopValues EopTable::query(Time<tags::TT> t) const {
  // TT -> UTC -> MJD. The `convert<UTC, TT>` path goes through TAI and is
  // EOP-free (UTC<->TAI is leap-second only). At the two-component
  // precision SOFA returns, jd1+jd2-kMjdOffsetJd lands on the right MJD to
  // ~1 ULP of the day — orders of magnitude tighter than the linear-
  // interpolation residual the table itself contributes.
  const auto utc = convert<tags::UTC>(t);
  const double mjd = (utc.jd1() - kMjdOffsetJd) + utc.jd2();

  // Out-of-range clamp: see header comment for rationale.
  if (mjd <= rows_.front().mjd_utc) {
    return {rows_.front().dut1, rows_.front().polar_xp, rows_.front().polar_yp};
  }
  if (mjd >= rows_.back().mjd_utc) {
    return {rows_.back().dut1, rows_.back().polar_xp, rows_.back().polar_yp};
  }

  // Bracket: find the first row whose mjd_utc > mjd; the previous row is
  // the lower bracket. `lower_bound` over a comparator on `mjd_utc`
  // returns the first row with mjd_utc >= mjd; we want strict-greater so
  // the bracket pair is (it - 1, it) for an equality hit too (which gives
  // alpha = 0 and returns the exact row's values).
  const auto it = std::lower_bound(rows_.begin(), rows_.end(), mjd,
                                   [](const EopRow& r, double m) { return r.mjd_utc < m; });
  // After the upper-clamp above, `it` is in (rows_.begin(), rows_.end()).
  const auto& hi = *it;
  const auto& lo = *(it - 1);
  const double span = hi.mjd_utc - lo.mjd_utc;
  // Defensive: the ctor invariant guarantees span > 0, but a checked
  // division costs nothing and surfaces accidental mis-construction.
  if (!(span > 0.0)) {
    throw std::logic_error("EopTable::query: zero-span bracket (table invariant violated)");
  }
  const double alpha = (mjd - lo.mjd_utc) / span;
  return {
      lo.dut1 + alpha * (hi.dut1 - lo.dut1),
      lo.polar_xp + alpha * (hi.polar_xp - lo.polar_xp),
      lo.polar_yp + alpha * (hi.polar_yp - lo.polar_yp),
  };
}

}  // namespace apsis::time
