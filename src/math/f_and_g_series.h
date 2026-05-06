// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §7: Lagrange f-and-g series for Keplerian propagation. Used by
// the Encke wrapper to advance the reference orbit analytically.
//
// Reference: Battin §4.5 / Vallado §2.3 / Bate, Mueller, White §4.5.
// We use the universal-variable formulation with Stumpff functions C(z),
// S(z), iterating on the universal anomaly chi.
//
// Internal-only header — not part of the public API.

#pragma once

#include "apsis/frames/frame_tags.h"
#include "apsis/frames/state.h"

namespace apsis::math::fandg {

// Propagate a Keplerian orbit from `state0` (at t = 0) to t = `dt` seconds
// later, under central-body gravitation with parameter `mu`. Returns the
// state at t = dt. Uses universal-variable iteration; converges in
// typically 6-8 iterations to ~1e-12 m for typical LEO/GEO step sizes.
apsis::frames::State<apsis::frames::tags::ICRF>
propagate(const apsis::frames::State<apsis::frames::tags::ICRF>& state0, double dt, double mu);

}  // namespace apsis::math::fandg
