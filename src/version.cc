// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0

#include "apsis/version.h"

namespace apsis {

std::string_view version() noexcept {
  return kVersionString;
}

}  // namespace apsis
