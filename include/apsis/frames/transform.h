// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §3: explicit `transform<To, From>` declarations covering every
// frame pair Phase 1 needs.
//
// Closed graph:
//   ICRF <-> GCRS    (identity in v1; documented placeholder)
//   ICRF <-> J2000   (frame bias; iauPmat06)
//   ICRF <-> ITRS    (CIO pipeline; iauC2t06a / iauPom00 + ERA + polar motion)
//   TEME <-> ITRS    (stubbed in Phase 1; throws — full implementation Phase 2)
//
// Phase-1A §B1: ICRF<->ITRS specialisations take a `const EopTable&` and
// query (xp, yp, dut1) from it at the input epoch. The Phase-1
// process-wide polar-motion globals and their set/get accessors are
// removed; calling `transform<ITRS, ICRF>(state, t)` without the EOP
// argument is now a compile-time error. The non-EOP pairs (GCRS, J2000,
// TEME, identity) keep the original two-argument signature.

#pragma once

#include "apsis/frames/frame_tags.h"
#include "apsis/frames/state.h"
#include "apsis/math/types.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace apsis::time {
class EopTable;  // fwd-decl; full definition in apsis/time/eop_table.h
}  // namespace apsis::time

namespace apsis::frames {

// Phase-1A §C1: expose the bare ICRF -> ITRS rotation matrix (used by force
// models that operate in body-fixed coordinates and need to apply the
// rotation themselves rather than transforming a full kinematic state).
// `R = W * R3(ERA) * Q^T` per the CEO pipeline; orthogonal so its inverse
// is its transpose. The same `EopTable` query that drives
// `transform<ITRS, ICRF>(...)` is used here, so the matrix is consistent
// with a state transform at the same epoch.
[[nodiscard]] apsis::math::Mat3 icrf_to_itrs_rotation(apsis::time::Time<apsis::time::tags::TT> tt,
                                                      const apsis::time::EopTable& eop);

// Primary template (EOP-free). Specialisations: GCRS<->ICRF, J2000<->ICRF,
// TEME<->ITRS, and the same-frame identity overloads below.
template <class To, class From>
State<To> transform(const State<From>& x, apsis::time::Time<apsis::time::tags::TT> t);

// Primary template (EOP variant). Specialisations: ITRS<->ICRF.
// Per Phase 1A §B1, the EOP table is threaded through here rather than
// pulled from a global.
template <class To, class From>
State<To> transform(const State<From>& x, apsis::time::Time<apsis::time::tags::TT> t,
                    const apsis::time::EopTable& eop);

// ICRF <-> GCRS: identity in v1 (SOFA's CIO pipeline absorbs the
// centre-of-mass offset; for the Phase 1 force models this is exact).
template <>
State<tags::GCRS> transform<tags::GCRS, tags::ICRF>(const State<tags::ICRF>&,
                                                    apsis::time::Time<apsis::time::tags::TT>);
template <>
State<tags::ICRF> transform<tags::ICRF, tags::GCRS>(const State<tags::GCRS>&,
                                                    apsis::time::Time<apsis::time::tags::TT>);

// ICRF <-> J2000: frame bias only (constant ~17 mas; iauPmat06 with
// epoch = J2000 returns the bias matrix).
template <>
State<tags::J2000> transform<tags::J2000, tags::ICRF>(const State<tags::ICRF>&,
                                                      apsis::time::Time<apsis::time::tags::TT>);
template <>
State<tags::ICRF> transform<tags::ICRF, tags::J2000>(const State<tags::J2000>&,
                                                     apsis::time::Time<apsis::time::tags::TT>);

// ICRF <-> ITRS: full CIO pipeline (EOP variant).
template <>
State<tags::ITRS> transform<tags::ITRS, tags::ICRF>(const State<tags::ICRF>&,
                                                    apsis::time::Time<apsis::time::tags::TT>,
                                                    const apsis::time::EopTable&);
template <>
State<tags::ICRF> transform<tags::ICRF, tags::ITRS>(const State<tags::ITRS>&,
                                                    apsis::time::Time<apsis::time::tags::TT>,
                                                    const apsis::time::EopTable&);

// TEME <-> ITRS: stubbed in Phase 1 (used by SGP4 in Phase 2). EOP-free
// (the stub throws unconditionally).
template <>
State<tags::ITRS> transform<tags::ITRS, tags::TEME>(const State<tags::TEME>&,
                                                    apsis::time::Time<apsis::time::tags::TT>);
template <>
State<tags::TEME> transform<tags::TEME, tags::ITRS>(const State<tags::ITRS>&,
                                                    apsis::time::Time<apsis::time::tags::TT>);

// Same-frame identity overloads (header-inline for hot generic paths).
// EOP-free: identity transforms don't need the table.
template <>
inline State<tags::ICRF>
transform<tags::ICRF, tags::ICRF>(const State<tags::ICRF>& x,
                                  apsis::time::Time<apsis::time::tags::TT>) {
  return x;
}
template <>
inline State<tags::J2000>
transform<tags::J2000, tags::J2000>(const State<tags::J2000>& x,
                                    apsis::time::Time<apsis::time::tags::TT>) {
  return x;
}
template <>
inline State<tags::ITRS>
transform<tags::ITRS, tags::ITRS>(const State<tags::ITRS>& x,
                                  apsis::time::Time<apsis::time::tags::TT>) {
  return x;
}
template <>
inline State<tags::GCRS>
transform<tags::GCRS, tags::GCRS>(const State<tags::GCRS>& x,
                                  apsis::time::Time<apsis::time::tags::TT>) {
  return x;
}
template <>
inline State<tags::TEME>
transform<tags::TEME, tags::TEME>(const State<tags::TEME>& x,
                                  apsis::time::Time<apsis::time::tags::TT>) {
  return x;
}

}  // namespace apsis::frames
