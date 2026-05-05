// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-ARCH-007
//
// Phase-1 §4: SpiceEphemeris seam tests. Verifies the lifecycle contract
// without depending on a kernel file: empty-kernel construction succeeds,
// querying without a kernel produces a runtime_error rather than aborting,
// and a bad kernel path is reported as a runtime_error from the constructor.
//
// The richer test that loads `data/de440_phase1.bsp` and queries Earth's
// state lives in tests/regression/jpl_de_roundtrip.cc (deliverable §9).

#include <gtest/gtest.h>

#include <stdexcept>
#include <vector>

#include "apsis/ephemeris/spice_ephemeris.h"

namespace {

TEST(SpiceEphemeris, EmptyKernelListConstructs) {
  // No kernel paths: constructor should set up CSPICE error mode and
  // return cleanly. Subsequent queries will fail because no body data is
  // available, but the lifecycle is sound.
  apsis::ephemeris::SpiceEphemeris ephem({});
  SUCCEED();
}

TEST(SpiceEphemeris, BadKernelPathThrows) {
  EXPECT_THROW(
      apsis::ephemeris::SpiceEphemeris ephem({"/nonexistent/no/such/kernel.bsp"}),
      std::runtime_error);
}

TEST(SpiceEphemeris, QueryWithoutKernelThrows) {
  apsis::ephemeris::SpiceEphemeris ephem({});
  apsis::time::Time<apsis::time::tags::TDB> t{2451545.0, 0.0};
  // Body 10 = Sun, but no kernel loaded -> SPK lookup fails -> we surface
  // it as runtime_error.
  EXPECT_THROW((void)ephem.state(/*Sun=*/10, t), std::runtime_error);
}

}  // namespace
