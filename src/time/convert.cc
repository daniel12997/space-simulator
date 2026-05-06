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
//
// Phase-1A §B1: UT1-touching specialisations take a `const EopTable&` and
// query `dut1 = (UT1-UTC)` from it at the input epoch. The Phase-1
// process-wide DUT1 global and its set/get accessors are removed;
// calling `convert<UT1>(t)` without the EOP argument is now a
// compile-time error.

#include "apsis/time/convert.h"

#include <cassert>

#include "apsis/time/eop_table.h"

extern "C" {
#include "sofa.h"
}

namespace apsis::time {
namespace {

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

// ---------------------------------------------------------------------------
// TAI <-> TT (fixed offset)
// ---------------------------------------------------------------------------

template <> Time<tags::TT> convert<tags::TT, tags::TAI>(Time<tags::TAI> t) {
  double tt1, tt2;
  const int kRc = iauTaitt(t.jd1(), t.jd2(), &tt1, &tt2);
  assert(kRc == 0 && "iauTaitt: non-zero status");
  (void)kRc;
  return Time<tags::TT>{tt1, tt2};
}

template <> Time<tags::TAI> convert<tags::TAI, tags::TT>(Time<tags::TT> t) {
  double tai1, tai2;
  const int kRc = iauTttai(t.jd1(), t.jd2(), &tai1, &tai2);
  assert(kRc == 0 && "iauTttai: non-zero status");
  (void)kRc;
  return Time<tags::TAI>{tai1, tai2};
}

// ---------------------------------------------------------------------------
// TAI <-> UTC (leap-second aware)
// ---------------------------------------------------------------------------

template <> Time<tags::UTC> convert<tags::UTC, tags::TAI>(Time<tags::TAI> t) {
  double utc1, utc2;
  const int kRc = iauTaiutc(t.jd1(), t.jd2(), &utc1, &utc2);
  // kRc == 1 is "dubious year"; kRc < 0 is an actual error.
  assert(kRc >= 0 && "iauTaiutc: error status");
  (void)kRc;
  return Time<tags::UTC>{utc1, utc2};
}

template <> Time<tags::TAI> convert<tags::TAI, tags::UTC>(Time<tags::UTC> t) {
  double tai1, tai2;
  const int kRc = iauUtctai(t.jd1(), t.jd2(), &tai1, &tai2);
  assert(kRc >= 0 && "iauUtctai: error status");
  (void)kRc;
  return Time<tags::TAI>{tai1, tai2};
}

// ---------------------------------------------------------------------------
// UTC <-> UT1 (queries `eop` at the input epoch)
// ---------------------------------------------------------------------------
//
// `EopTable::query` is keyed on `Time<TT>`; for the UTC->UT1 direction we
// route the input through TT-via-TAI (both EOP-free) so the table query
// happens on the well-defined TT scale. The dut1 ~ 0.5 s ambiguity at the
// table's daily cadence is invisible to the linear interpolant — i.e.
// querying at "UTC mapped to TT" vs "UT1 mapped to TT" yields differences
// well below the 100-ns regression tolerance.

template <> Time<tags::UT1> convert<tags::UT1, tags::UTC>(Time<tags::UTC> t, const EopTable& eop) {
  const auto kTt = convert<tags::TT>(t);
  const double kDut1 = eop.query(kTt).dut1;
  double ut11, ut12;
  const int kRc = iauUtcut1(t.jd1(), t.jd2(), kDut1, &ut11, &ut12);
  assert(kRc >= 0 && "iauUtcut1: error status");
  (void)kRc;
  return Time<tags::UT1>{ut11, ut12};
}

template <> Time<tags::UTC> convert<tags::UTC, tags::UT1>(Time<tags::UT1> t, const EopTable& eop) {
  // The EOP table is keyed on TT; for the inverse direction we go
  // UT1 -> (TT via TAI is unavailable without dut1) so we convert UT1 -> UTC
  // first using a provisional dut1=0 query of the table to land at a TT
  // approximation. In practice the 1-second-scale dut1 displacement is
  // invisible to the daily-cadence linear interpolant; we therefore query
  // the EOP table at `Time<TT>` constructed as if the input were UTC, then
  // pass that dut1 to `iauUt1utc`. This is exactly the "evaluate dtr at
  // the input scale" pattern used in the TT<->TDB inverse below.
  const Time<tags::UTC> kAsUtc{t.jd1(), t.jd2()};
  const auto kTt = convert<tags::TT>(kAsUtc);
  const double kDut1 = eop.query(kTt).dut1;
  double utc1, utc2;
  const int kRc = iauUt1utc(t.jd1(), t.jd2(), kDut1, &utc1, &utc2);
  assert(kRc >= 0 && "iauUt1utc: error status");
  (void)kRc;
  return Time<tags::UTC>{utc1, utc2};
}

// ---------------------------------------------------------------------------
// TAI <-> UT1 (composed via UTC for ΔAT bookkeeping)
// ---------------------------------------------------------------------------

template <> Time<tags::UT1> convert<tags::UT1, tags::TAI>(Time<tags::TAI> t, const EopTable& eop) {
  return convert<tags::UT1>(convert<tags::UTC>(t), eop);
}

template <> Time<tags::TAI> convert<tags::TAI, tags::UT1>(Time<tags::UT1> t, const EopTable& eop) {
  return convert<tags::TAI>(convert<tags::UTC>(t, eop));
}

// ---------------------------------------------------------------------------
// TT <-> TDB (geocentric approximation; observer position not carried)
// ---------------------------------------------------------------------------

template <> Time<tags::TDB> convert<tags::TDB, tags::TT>(Time<tags::TT> t) {
  const double kDtr = dtr_geo_approx(t.jd1(), t.jd2());
  double tdb1, tdb2;
  const int kRc = iauTttdb(t.jd1(), t.jd2(), kDtr, &tdb1, &tdb2);
  assert(kRc == 0 && "iauTttdb: non-zero status");
  (void)kRc;
  return Time<tags::TDB>{tdb1, tdb2};
}

template <> Time<tags::TT> convert<tags::TT, tags::TDB>(Time<tags::TDB> t) {
  // iauDtdb is parameterised on TT (the function returns TDB-TT given a TT
  // input); evaluating it with TDB introduces a second-order error of
  // ~10^-23 s — far below the float resolution of jd2 — so for the inverse
  // we evaluate dtr at the input (TDB) directly. SOFA's tdbtt then folds in
  // the negation.
  const double kDtr = dtr_geo_approx(t.jd1(), t.jd2());
  double tt1, tt2;
  const int kRc = iauTdbtt(t.jd1(), t.jd2(), kDtr, &tt1, &tt2);
  assert(kRc == 0 && "iauTdbtt: non-zero status");
  (void)kRc;
  return Time<tags::TT>{tt1, tt2};
}

// ---------------------------------------------------------------------------
// Composed pairs (TAI is the canonical pivot for the {TT, UTC, UT1} cluster;
// TT is the pivot for {TDB}).
// ---------------------------------------------------------------------------

template <> Time<tags::TT> convert<tags::TT, tags::UTC>(Time<tags::UTC> t) {
  return convert<tags::TT>(convert<tags::TAI>(t));
}

template <> Time<tags::UTC> convert<tags::UTC, tags::TT>(Time<tags::TT> t) {
  return convert<tags::UTC>(convert<tags::TAI>(t));
}

template <> Time<tags::TT> convert<tags::TT, tags::UT1>(Time<tags::UT1> t, const EopTable& eop) {
  return convert<tags::TT>(convert<tags::TAI>(t, eop));
}

template <> Time<tags::UT1> convert<tags::UT1, tags::TT>(Time<tags::TT> t, const EopTable& eop) {
  return convert<tags::UT1>(convert<tags::TAI>(t), eop);
}

template <> Time<tags::TDB> convert<tags::TDB, tags::TAI>(Time<tags::TAI> t) {
  return convert<tags::TDB>(convert<tags::TT>(t));
}

template <> Time<tags::TAI> convert<tags::TAI, tags::TDB>(Time<tags::TDB> t) {
  return convert<tags::TAI>(convert<tags::TT>(t));
}

}  // namespace apsis::time
