// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: `IIntegrator` interface — propagates the augmented system
// (state, Phi) by a single dt step. ADR-009 commits to three adapters
// behind this seam: an adaptive RK (full DOP853 in the steady state, with
// `Dp54` shipped in Phase 1 as a stand-in coefficient table — see ADR-009
// Phase 1 Implementation Note), `Yoshida4` (symplectic), and
// `GaussJackson8` (multi-step; deferred to Phase 7).
//
// State convention: ICRF, SI units, with Phi as the 6x6 state-transition
// matrix d(state(t+dt))/d(state(t0)). Caller seeds Phi = I when starting
// a fresh segment; subsequent steps maintain Phi by integrating
// dPhi/dt = A(t) * Phi where A is the 6x6 Jacobian assembled from
// force.partials_dadx():
//
//     A = [[ 0      I    ]
//          [ da/dr  da/dv ]]
//
// In Phase 1 da/dv == 0 for every adapter (drag/SRP deferred), so A's
// bottom-right block is zero.

#pragma once

#include "apsis/force/iforce_model.h"
#include "apsis/frames/frame_tags.h"
#include "apsis/frames/state.h"
#include "apsis/math/types.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace apsis::integrate {

struct StepResult {
  apsis::frames::State<apsis::frames::tags::ICRF> x;
  apsis::math::Mat6 phi;
  double dt_actually_taken = 0.0;  // == dt for fixed-step adapters
};

class IIntegrator {
 public:
  IIntegrator() = default;
  IIntegrator(const IIntegrator&) = delete;
  IIntegrator& operator=(const IIntegrator&) = delete;
  IIntegrator(IIntegrator&&) = delete;
  IIntegrator& operator=(IIntegrator&&) = delete;
  virtual ~IIntegrator() = default;

  virtual StepResult step(apsis::time::Time<apsis::time::tags::TT> t,
                          const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                          const apsis::math::Mat6& phi, double dt,
                          const apsis::force::IForceModel& force) = 0;
};

// Helper: assemble the 6x6 dynamics Jacobian A from a 3x6 force partial.
[[nodiscard]] inline apsis::math::Mat6 assemble_a(const apsis::math::Mat36& dadx) {
  apsis::math::Mat6 a = apsis::math::Mat6::Zero();
  // Top-right: dr/dt = v -> identity in the position-rate-vs-velocity block.
  a.block<3, 3>(0, 3).setIdentity();
  // Bottom-left: dv/dt = a -> da/dr.
  a.block<3, 3>(3, 0) = dadx.block<3, 3>(0, 0);
  // Bottom-right: da/dv (zero in Phase 1).
  a.block<3, 3>(3, 3) = dadx.block<3, 3>(0, 3);
  return a;
}

}  // namespace apsis::integrate
