// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5: `IForceModel` — the force-model seam, a.k.a. the Variational
// Equations contract (ADR-002, REQ-PHY-016, REQ-PHY-020).
//
// Every adapter SHALL provide:
//   * `acceleration(t, x)` — SI [m/s^2] in ICRF at TT epoch t and ICRF state x.
//   * `partials(t, x)`     — 3x6 Jacobian. Cols 0..2 = ∂a/∂r; cols 3..5 = ∂a/∂v.
//
// The 3x6 shape is per the plan §5 deliverable. Force models with no
// velocity dependence return zeros in cols 3..5; the conformance test
// validates this against central-difference perturbations.

#pragma once

#include "apsis/frames/frame_tags.h"
#include "apsis/frames/state.h"
#include "apsis/math/types.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace apsis::force {

class IForceModel {
 public:
  IForceModel() = default;
  IForceModel(const IForceModel&) = delete;
  IForceModel& operator=(const IForceModel&) = delete;
  IForceModel(IForceModel&&) = delete;
  IForceModel& operator=(IForceModel&&) = delete;
  virtual ~IForceModel() = default;

  // Acceleration in ICRF at TT epoch `t` for state `x` (ICRF, SI units).
  virtual apsis::math::Vec3 acceleration(
      apsis::time::Time<apsis::time::tags::TT> t,
      const apsis::frames::State<apsis::frames::tags::ICRF>& x) const = 0;

  // 3x6 partials: ∂a/∂[r; v] in the same units (1/s^2 for the position
  // columns; 1/s for the velocity columns when a velocity-dependent force
  // is present, zero otherwise in Phase 1 since drag/SRP are deferred).
  virtual apsis::math::Mat36 partials(
      apsis::time::Time<apsis::time::tags::TT> t,
      const apsis::frames::State<apsis::frames::tags::ICRF>& x) const = 0;
};

}  // namespace apsis::force
