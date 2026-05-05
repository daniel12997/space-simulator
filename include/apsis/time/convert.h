// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §2: explicit per-pair `convert<To, From>(Time<From>) -> Time<To>`
// declarations. The supported graph is:
//
//     TAI <-> TT     (fixed offset 32.184 s; iauTaitt / iauTttai)
//     TAI <-> UTC    (leap-second aware; iauTaiutc / iauUtctai)
//     TAI <-> UT1    (UT1-UTC from EOP; iauTaiut1 / iauUt1tai)
//     TT  <-> TDB    (iauTttdb / iauTdbtt with geocentre approximation)
//
// All other pairs are obtained by composition (e.g. UTC -> TT goes via TAI).
// The composing convenience overloads are declared explicitly so the call
// site reads `convert<TT>(utc)` rather than spelling the chain.
//
// Pairs *not* declared (e.g. `convert<TDB, UTC>`) are deliberately omitted:
// either the caller composes through TAI explicitly, or — if they wrote a
// direct call — they get a link error rather than a silent SOFA chain
// glued together by the compiler. This is the project's "no silent
// fallback" rule applied to time.

#pragma once

#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace apsis::time {

// Primary template: declared but not defined. Any specialisation not
// provided below is a link error if instantiated.
template <class To, class From>
Time<To> convert(Time<From> t);

// Direct (SOFA-mediated) pairs.
template <>
Time<tags::TT> convert<tags::TT, tags::TAI>(Time<tags::TAI>);
template <>
Time<tags::TAI> convert<tags::TAI, tags::TT>(Time<tags::TT>);

template <>
Time<tags::UTC> convert<tags::UTC, tags::TAI>(Time<tags::TAI>);
template <>
Time<tags::TAI> convert<tags::TAI, tags::UTC>(Time<tags::UTC>);

template <>
Time<tags::UT1> convert<tags::UT1, tags::TAI>(Time<tags::TAI>);
template <>
Time<tags::TAI> convert<tags::TAI, tags::UT1>(Time<tags::UT1>);

template <>
Time<tags::TDB> convert<tags::TDB, tags::TT>(Time<tags::TT>);
template <>
Time<tags::TT> convert<tags::TT, tags::TDB>(Time<tags::TDB>);

// Composition pairs (declared so callers can write the natural form).
// Each is implemented by going through TAI (or TT, for the TDB family).
template <>
Time<tags::TT>  convert<tags::TT,  tags::UTC>(Time<tags::UTC>);
template <>
Time<tags::UTC> convert<tags::UTC, tags::TT> (Time<tags::TT>);

template <>
Time<tags::UT1> convert<tags::UT1, tags::UTC>(Time<tags::UTC>);
template <>
Time<tags::UTC> convert<tags::UTC, tags::UT1>(Time<tags::UT1>);

template <>
Time<tags::TT>  convert<tags::TT,  tags::UT1>(Time<tags::UT1>);
template <>
Time<tags::UT1> convert<tags::UT1, tags::TT> (Time<tags::TT>);

template <>
Time<tags::TDB> convert<tags::TDB, tags::TAI>(Time<tags::TAI>);
template <>
Time<tags::TAI> convert<tags::TAI, tags::TDB>(Time<tags::TDB>);

// Same-scale identity (not a no-op only because callers may use generic
// code paths that template over scales; declared inline for those sites).
template <>
inline Time<tags::TAI> convert<tags::TAI, tags::TAI>(Time<tags::TAI> t) { return t; }
template <>
inline Time<tags::TT>  convert<tags::TT,  tags::TT> (Time<tags::TT>  t) { return t; }
template <>
inline Time<tags::UTC> convert<tags::UTC, tags::UTC>(Time<tags::UTC> t) { return t; }
template <>
inline Time<tags::UT1> convert<tags::UT1, tags::UT1>(Time<tags::UT1> t) { return t; }
template <>
inline Time<tags::TDB> convert<tags::TDB, tags::TDB>(Time<tags::TDB> t) { return t; }

// EOP injection: per the plan, UT1-UTC is taken from a process-wide EOP
// table loaded once at startup. Phase 1 ships a frozen slice; callers may
// also override the table for tests.
//
// Setting a single scalar `dut1` (UT1-UTC, seconds) is sufficient for the
// regression tests in Phase 1, which all evaluate within the slice's
// flatness. A future "lookup by epoch" overload can be added without
// changing call sites that use the scalar form.
void set_default_dut1(double seconds) noexcept;
[[nodiscard]] double default_dut1() noexcept;

}  // namespace apsis::time
