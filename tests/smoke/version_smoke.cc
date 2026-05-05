// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-0 smoke test: confirms apsis::core links and apsis::version() returns
// a non-empty build-stamp. Replaced/extended in Phase 1.

#include <gtest/gtest.h>

#include "apsis/version.h"

TEST(VersionSmoke, NonEmpty) {
  EXPECT_FALSE(apsis::version().empty());
}

TEST(VersionSmoke, MatchesConstant) {
  EXPECT_EQ(apsis::version(), apsis::kVersionString);
}
