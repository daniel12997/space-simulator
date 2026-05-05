// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §2: SOFA-mediated implementations of `apsis::time::convert<>` for
// the supported scale graph (see include/apsis/time/convert.h for the graph).
//
// Conventions used here:
//   * "dta" in SOFA's UT1<->TAI naming = (UT1 - TAI) seconds.
//     Given that UT1 - UTC = dut1 and UTC - TAI ≈ -ΔAT (always negative),
//     we have dta = dut1 - ΔAT(UTC). The cleanest way to obtain ΔAT is to
//     route via UTC using `iauTaiutc` / `iauUtctai`; we therefore implement
//     TAI<->UT1 as TAI<->UTC<->UT1.
//   * iauTttdb / iauTdbtt take "dtr" = TDB - TT (seconds), itself a
//     periodic term computed via iauDtdb. For Phase 1 we evaluate iauDtdb
//     with the geocentric approximation (ut = 0, elong = 0, u = 0, v = 0)
//     per the plan's "geocentre approximation; Time<> API does not carry
//     observer position in v1".
//   * SOFA returns 0 on success; non-zero status flags edge cases (we
//     promote to assertion failures rather than letting them silently
//     return wrong instants).

#include "apsis/time/convert.h"

#include <atomic>
#include <cassert>

extern "C" {
#include "sofa.h"
}

namespace apsis::time {
namespace {

// Process-wide UT1-UTC scalar. `std::atomic<double>` guarantees lock-free
// reads on the platforms Apsis targets (x86_64 / aarch64); the value is set
// once at startup from the EOP slice and read in hot conversion paths.
std::atomic<double> g_default_dut1{0.0};

// `iauDtdb` with the geocentre approximation. Per the plan, Phase 1's
// `Time<>` API does not carry observer position; the location-dependent
// (Lense-Thirring + observer's gravitational potential) term is therefore
// dropped. The residual error vs a barycentric observer is ≤ 1.6 ms which
// matters for relativistic-strict ranging but not for any Phase 1
// regression case.
double dtr_geo_approx(double tt1, double tt2) {
  return iauDtdb(tt1, tt2, /*ut=*/0.0, /*elong=*/0.0, /*u=*/0.0, /*v=*/0.0);
}

}  // namespace

void set_default_dut1(double seconds) noexcept {
  g_default_dut1.store(seconds, std::memory_order_relaxed);
}

double default_dut1() noexcept {
  return g_default_dut1.load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// TAI <-> TT (fixed offset)
// ---------------------------------------------------------------------------

template <>
Time<tags::TT> convert<tags::TT, tags::TAI>(Time<tags::TAI> t) {
  double tt1, tt2;
  const int rc = iauTaitt(t.jd1(), t.jd2(), &tt1, &tt2);
  assert(rc == 0 && "iauTaitt: non-zero status");
  (void)rc;
  return Time<tags::TT>{tt1, tt2};
}

template <>
Time<tags::TAI> convert<tags::TAI, tags::TT>(Time<tags::TT> t) {
  double tai1, tai2;
  const int rc = iauTttai(t.jd1(), t.jd2(), &tai1, &tai2);
  assert(rc == 0 && "iauTttai: non-zero status");
  (void)rc;
  return Time<tags::TAI>{tai1, tai2};
}

// ---------------------------------------------------------------------------
// TAI <-> UTC (leap-second aware)
// ---------------------------------------------------------------------------

template <>
Time<tags::UTC> convert<tags::UTC, tags::TAI>(Time<tags::TAI> t) {
  double utc1, utc2;
  const int rc = iauTaiutc(t.jd1(), t.jd2(), &utc1, &utc2);
  // rc == 1 is "dubious year"; rc < 0 is an actual error.
  assert(rc >= 0 && "iauTaiutc: error status");
  (void)rc;
  return Time<tags::UTC>{utc1, utc2};
}

template <>
Time<tags::TAI> convert<tags::TAI, tags::UTC>(Time<tags::UTC> t) {
  double tai1, tai2;
  const int rc = iauUtctai(t.jd1(), t.jd2(), &tai1, &tai2);
  assert(rc >= 0 && "iauUtctai: error status");
  (void)rc;
  return Time<tags::TAI>{tai1, tai2};
}

// ---------------------------------------------------------------------------
// UTC <-> UT1 (uses default_dut1)
// ---------------------------------------------------------------------------

template <>
Time<tags::UT1> convert<tags::UT1, tags::UTC>(Time<tags::UTC> t) {
  double ut11, ut12;
  const int rc = iauUtcut1(t.jd1(), t.jd2(), default_dut1(), &ut11, &ut12);
  assert(rc >= 0 && "iauUtcut1: error status");
  (void)rc;
  return Time<tags::UT1>{ut11, ut12};
}

template <>
Time<tags::UTC> convert<tags::UTC, tags::UT1>(Time<tags::UT1> t) {
  double utc1, utc2;
  const int rc = iauUt1utc(t.jd1(), t.jd2(), default_dut1(), &utc1, &utc2);
  assert(rc >= 0 && "iauUt1utc: error status");
  (void)rc;
  return Time<tags::UTC>{utc1, utc2};
}

// ---------------------------------------------------------------------------
// TAI <-> UT1 (composed via UTC for ΔAT bookkeeping)
// ---------------------------------------------------------------------------

template <>
Time<tags::UT1> convert<tags::UT1, tags::TAI>(Time<tags::TAI> t) {
  return convert<tags::UT1>(convert<tags::UTC>(t));
}

template <>
Time<tags::TAI> convert<tags::TAI, tags::UT1>(Time<tags::UT1> t) {
  return convert<tags::TAI>(convert<tags::UTC>(t));
}

// ---------------------------------------------------------------------------
// TT <-> TDB (geocentric approximation; observer position not carried)
// ---------------------------------------------------------------------------

template <>
Time<tags::TDB> convert<tags::TDB, tags::TT>(Time<tags::TT> t) {
  const double dtr = dtr_geo_approx(t.jd1(), t.jd2());
  double tdb1, tdb2;
  const int rc = iauTttdb(t.jd1(), t.jd2(), dtr, &tdb1, &tdb2);
  assert(rc == 0 && "iauTttdb: non-zero status");
  (void)rc;
  return Time<tags::TDB>{tdb1, tdb2};
}

template <>
Time<tags::TT> convert<tags::TT, tags::TDB>(Time<tags::TDB> t) {
  // iauDtdb is parameterised on TT (the function returns TDB-TT given a TT
  // input); evaluating it with TDB introduces a second-order error of
  // ~10^-23 s — far below the float resolution of jd2 — so for the inverse
  // we evaluate dtr at the input (TDB) directly. SOFA's tdbtt then folds in
  // the negation.
  const double dtr = dtr_geo_approx(t.jd1(), t.jd2());
  double tt1, tt2;
  const int rc = iauTdbtt(t.jd1(), t.jd2(), dtr, &tt1, &tt2);
  assert(rc == 0 && "iauTdbtt: non-zero status");
  (void)rc;
  return Time<tags::TT>{tt1, tt2};
}

// ---------------------------------------------------------------------------
// Composed pairs (TAI is the canonical pivot for the {TT, UTC, UT1} cluster;
// TT is the pivot for {TDB}).
// ---------------------------------------------------------------------------

template <>
Time<tags::TT> convert<tags::TT, tags::UTC>(Time<tags::UTC> t) {
  return convert<tags::TT>(convert<tags::TAI>(t));
}

template <>
Time<tags::UTC> convert<tags::UTC, tags::TT>(Time<tags::TT> t) {
  return convert<tags::UTC>(convert<tags::TAI>(t));
}

template <>
Time<tags::TT> convert<tags::TT, tags::UT1>(Time<tags::UT1> t) {
  return convert<tags::TT>(convert<tags::TAI>(t));
}

template <>
Time<tags::UT1> convert<tags::UT1, tags::TT>(Time<tags::TT> t) {
  return convert<tags::UT1>(convert<tags::TAI>(t));
}

template <>
Time<tags::TDB> convert<tags::TDB, tags::TAI>(Time<tags::TAI> t) {
  return convert<tags::TDB>(convert<tags::TT>(t));
}

template <>
Time<tags::TAI> convert<tags::TAI, tags::TDB>(Time<tags::TDB> t) {
  return convert<tags::TAI>(convert<tags::TT>(t));
}

}  // namespace apsis::time
