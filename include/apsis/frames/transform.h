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
// Polar motion (xp, yp) is taken from the EOP scalar set
// (`set_default_polar_motion`); UT1-UTC from `apsis::time::default_dut1`.

#pragma once

#include "apsis/frames/frame_tags.h"
#include "apsis/frames/state.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace apsis::frames {

template <class To, class From>
State<To> transform(const State<From>& x, apsis::time::Time<apsis::time::tags::TT> t);

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

// ICRF <-> ITRS: full CIO pipeline.
template <>
State<tags::ITRS> transform<tags::ITRS, tags::ICRF>(const State<tags::ICRF>&,
                                                    apsis::time::Time<apsis::time::tags::TT>);
template <>
State<tags::ICRF> transform<tags::ICRF, tags::ITRS>(const State<tags::ITRS>&,
                                                    apsis::time::Time<apsis::time::tags::TT>);

// TEME <-> ITRS: stubbed in Phase 1 (used by SGP4 in Phase 2).
template <>
State<tags::ITRS> transform<tags::ITRS, tags::TEME>(const State<tags::TEME>&,
                                                    apsis::time::Time<apsis::time::tags::TT>);
template <>
State<tags::TEME> transform<tags::TEME, tags::ITRS>(const State<tags::ITRS>&,
                                                    apsis::time::Time<apsis::time::tags::TT>);

// Same-frame identity overloads (header-inline for hot generic paths).
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

// Polar motion injection. Loaded once from the EOP slice; tests may override.
// (xp, yp) in radians.
void set_default_polar_motion(double xp_rad, double yp_rad) noexcept;
[[nodiscard]] double default_polar_motion_xp() noexcept;
[[nodiscard]] double default_polar_motion_yp() noexcept;

}  // namespace apsis::frames
