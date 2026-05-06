// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §2: empty phantom tag types for the supported time scales.
// Per ADR-010 (and ADR-003 by reference), `Time<Scale>` carries its scale at
// the type level. The tag types live in their own header so consumers that
// only need to *name* a scale (e.g. function signatures) don't drag in the
// `Time` template, the conversion graph, or `<chrono>`-class dependencies.

#pragma once

namespace apsis::time::tags {

// IAU/CCSDS time scales supported in Phase 1.
//
// Closed graph from the user-facing API: TAI <-> {TT, UTC, UT1};
// TT <-> TDB. Any other path is an explicit composition of the above
// (see `apsis::time::convert<>`).
struct TAI {};
struct TT {};
struct UTC {};
struct UT1 {};
struct TDB {};

// TCG / TCB are deliberately *not* included in Phase 1. ADR-003 leaves
// them as an open item for relativistic-strict use cases; there is no
// regression case in Phase 1 that requires them and adding them later is
// purely additive (new tag struct + new convert<> specialisations).

}  // namespace apsis::time::tags
