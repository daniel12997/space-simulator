// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-ARCH-007
//
// Phase-1 §4: SpiceEphemeris concurrency contract — exercises the
// process-wide CSPICE mutex (ADR-008). Two SpiceEphemeris instances
// hammer the seam from two threads at once; if the lock is per-instance
// rather than process-wide, CSPICE's global error stack will tear and
// either spuriously surface failures into one thread from the other or
// trip the address sanitiser on the run-under-sanitiser CI job.
//
// The test never asserts a positive query result (no kernels are loaded);
// each `state()` call is expected to throw std::runtime_error from the
// no-data path. The contract under test is "two instances calling CSPICE
// concurrently do not corrupt each other's seen error state and do not
// deadlock". A per-instance mutex would NOT have caught this.

#include <gtest/gtest.h>

#include <atomic>
#include <stdexcept>
#include <thread>
#include <vector>

#include "apsis/ephemeris/spice_ephemeris.h"

namespace {

TEST(SpiceEphemerisConcurrency, TwoInstancesShareGlobalLock) {
  // Two independent ephemeris instances. With a per-instance mutex (the
  // ADR-008 violation we are testing against) each thread would proceed
  // into CSPICE in parallel and corrupt the shared error stack.
  apsis::ephemeris::SpiceEphemeris a({});
  apsis::ephemeris::SpiceEphemeris b({});

  constexpr int kIters = 200;
  std::atomic<int> a_throws{0};
  std::atomic<int> b_throws{0};

  auto hammer = [](apsis::ephemeris::SpiceEphemeris& e, std::atomic<int>& throws) {
    for (int i = 0; i < kIters; ++i) {
      apsis::time::Time<apsis::time::tags::TDB> t{2451545.0, 0.0};
      try {
        // No kernels loaded -> SPK lookup fails -> seam throws. The
        // contract is: every iteration sees a clean failure path
        // independently of the other thread's interleaving.
        (void)e.state(/*Sun=*/10, t);
      } catch (const std::runtime_error&) {
        throws.fetch_add(1, std::memory_order_relaxed);
      }
    }
  };

  std::thread t1(hammer, std::ref(a), std::ref(a_throws));
  std::thread t2(hammer, std::ref(b), std::ref(b_throws));
  t1.join();
  t2.join();

  // Every iteration must have thrown — if the lock failed to serialise,
  // one thread could observe the other's reset_c() between failed_c()
  // and getmsg_c(), losing the throw.
  EXPECT_EQ(a_throws.load(), kIters);
  EXPECT_EQ(b_throws.load(), kIters);
}

}  // namespace
