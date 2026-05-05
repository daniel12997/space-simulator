// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §3: TEME <-> ITRS — STUB. The full implementation belongs to
// Phase 2 alongside SGP4 (REQ-INT-005). Phase 1 ships the symbol so that
// downstream code that templates over frame pairs can name TEME without
// triggering a link error at unrelated build sites; calling either of these
// at runtime in Phase 1 is a programming error and asserts.
//
// The Phase 2 implementation will follow Vallado et al. 2006 §VI:
//   * For SGP4 output: r_itrs = R3(GMST_82) * r_teme  +  W * (...)
//   * Sidereal time via the equation-of-equinoxes form (NOT iauEra00),
//     because TEME is an equinox-based intermediate frame.

#include "apsis/frames/transform.h"

#include <stdexcept>

namespace apsis::frames {

template <>
State<tags::ITRS> transform<tags::ITRS, tags::TEME>(
    State<tags::TEME>, apsis::time::Time<apsis::time::tags::TT>) {
  throw std::logic_error(
      "transform<ITRS, TEME>: stubbed in Phase 1; full implementation lands "
      "in Phase 2 with SGP4. See src/frames/teme.cc.");
}

template <>
State<tags::TEME> transform<tags::TEME, tags::ITRS>(
    State<tags::ITRS>, apsis::time::Time<apsis::time::tags::TT>) {
  throw std::logic_error(
      "transform<TEME, ITRS>: stubbed in Phase 1; full implementation lands "
      "in Phase 2 with SGP4. See src/frames/teme.cc.");
}

}  // namespace apsis::frames
