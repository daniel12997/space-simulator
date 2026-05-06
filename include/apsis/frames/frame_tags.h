// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §3: empty phantom tag types for the supported reference frames.
// `State<Frame>` carries its frame at the type level; cross-frame mixing is
// a compile error.

#pragma once

namespace apsis::frames::tags {

// International Celestial Reference Frame (ICRF3).
struct ICRF {};

// Geocentric Celestial Reference System. Identical to ICRF up to a
// negligible centre-of-mass offset for v1; SOFA does not currently
// distinguish them and Apsis treats GCRS<->ICRF as identity.
struct GCRS {};

// International Terrestrial Reference System (ITRS) — Earth-fixed.
struct ITRS {};

// J2000 mean equator and equinox. Distinct from ICRF by the constant
// ~17 mas frame bias (REQ-TIME-005).
struct J2000 {};

// True-Equator Mean-Equinox of date — required by SGP4 in Phase 2; stubbed
// here so forward-compat declarations exist.
struct TEME {};

// Per-body fixed frame, parameterised on the central body. The body type
// is itself a tag (e.g. `bodies::Earth`); Phase 1 only uses Earth's body-fixed
// frame indirectly via `ITRS`. The `BodyFixed` template is reserved for the
// generalisation and not used by Phase 1 transforms.
template <class Body> struct BodyFixed {};

}  // namespace apsis::frames::tags
