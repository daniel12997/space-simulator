// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §2: NEGATIVE compile test. CMake's try_compile invokes this file
// with EXPECT_FAIL semantics: the build is expected to FAIL because TAI and
// UTC `Time<>` instantiations have no mixed-scale operators. If this ever
// compiles, the type-level safety pillar of ADR-003 / ADR-010 has been
// silently lifted and the surrounding gate fails the configure step.

#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

int main() {
  apsis::time::Time<apsis::time::tags::TAI> tai;
  apsis::time::Time<apsis::time::tags::UTC> utc;
  // The next line MUST NOT compile.
  auto delta = tai - utc;
  (void)delta;
  return 0;
}
