// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// apsis::version — build-stamp string for the Apsis core library. Phase 0
// ships this and only this so the build system, linker, and smoke test can
// be exercised end-to-end before any domain code lands.

#pragma once

#include <string_view>

namespace apsis {

inline constexpr std::string_view kVersionString = "0.0.1+phase-0";

[[nodiscard]] std::string_view version() noexcept;

}  // namespace apsis
