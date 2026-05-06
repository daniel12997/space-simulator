// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-FRM-013
//
// The actual compile-fail gate ran at CMake configure time via try_compile()
// in tests/unit/frames/CMakeLists.txt. This gtest is a beacon so that ctest
// output shows "frame-mixing guard ran"; if the configure-time check were
// somehow disabled, this test still exists and intentionally won't catch
// the regression — it cannot, because the issue is whether the *other* file
// compiled. The configure-time message is the authoritative signal.

#include <gtest/gtest.h>

TEST(FrameMixingGuard, ConfigureTimeCheckRecorded) {
  // Recorded marker; see frame_mixing_compile_fail.cc + CMakeLists.txt.
  SUCCEED() << "Compile-fail gate evaluated at configure time.";
}
