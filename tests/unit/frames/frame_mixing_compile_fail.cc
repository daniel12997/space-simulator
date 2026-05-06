// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-PHY-007
//
// Phase-1A §A2: NEGATIVE compile test. CMake's try_compile invokes this
// file with EXPECT_FAIL semantics: the build is expected to FAIL because
// the explicit instantiation of `transform<ICRF, ICRF>` cannot be passed a
// `State<ITRS>&` argument — the From-tag mismatch is a deduction failure.
// If this ever compiles, the type-level frame-mixing safety pillar of
// ADR-010 has been silently lifted and the surrounding gate fails the
// configure step.

#include "apsis/frames/frame_tags.h"
#include "apsis/frames/state.h"
#include "apsis/frames/transform.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

int main() {
  apsis::frames::State<apsis::frames::tags::ITRS> state_in_itrs;
  apsis::time::Time<apsis::time::tags::TT> t;
  // The next line MUST NOT compile: explicit `From = ICRF` does not match
  // the actual argument type `State<ITRS>`. Mixing frames is only
  // permitted via an explicit `transform<>` between matching tags.
  auto bad = apsis::frames::transform<apsis::frames::tags::ICRF,
                                      apsis::frames::tags::ICRF>(state_in_itrs, t);
  (void)bad;
  return 0;
}
