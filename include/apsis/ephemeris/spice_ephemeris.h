// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §4: `SpiceEphemeris` — the only place in the codebase that calls
// CSPICE. Per ADR-008, every CSPICE call is wrapped in one process-wide
// std::mutex held by this class. The cspice_seam.py CI lint enforces that
// no other source file calls any `*_c(` API.

#pragma once

#include <mutex>
#include <string>
#include <vector>

#include "apsis/ephemeris/iephemeris.h"

namespace apsis::ephemeris {

class SpiceEphemeris final : public IEphemeris {
 public:
  // Loads each path via furnsh_c (under the seam mutex). Throws
  // std::runtime_error if SPICE's error stack is non-empty after the
  // last furnsh_c — defensively converting CSPICE's "set error then
  // continue" model into a C++ exception at the seam boundary.
  explicit SpiceEphemeris(const std::vector<std::string>& kernel_paths);

  // Calls kclear_c under the lock to release loaded kernels.
  ~SpiceEphemeris() override;

  apsis::frames::State<apsis::frames::tags::ICRF>
  state(int body_naif_id,
        apsis::time::Time<apsis::time::tags::TDB> t) const override;

 private:
  // Mutex guarding ALL CSPICE state. Mutable because `state()` is logically
  // const — CSPICE has thread-unsafe internals but our caller-visible API
  // is read-only.
  mutable std::mutex spice_mutex_;
};

}  // namespace apsis::ephemeris
