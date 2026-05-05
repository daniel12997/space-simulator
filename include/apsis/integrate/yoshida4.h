// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: Yoshida4 — fourth-order symplectic composition of velocity-
// Verlet (Yoshida 1990, "Construction of higher order symplectic integrators").
// Fixed-step.
//
// State Phi is integrated alongside the natural state via the same
// composition coefficients applied to the linear ODE dPhi/dt = A * Phi
// at the same intermediate times. This is not strictly symplectic for
// the linear-ODE portion (Phi is not Hamiltonian), but tracks the
// natural state's evolution which is what callers consume.

#pragma once

#include "apsis/integrate/iintegrator.h"

namespace apsis::integrate {

class Yoshida4 final : public IIntegrator {
 public:
  Yoshida4() noexcept = default;

  StepResult step(apsis::time::Time<apsis::time::tags::TT> t,
                  const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                  const apsis::math::Mat6& phi,
                  double dt,
                  const apsis::force::IForceModel& force) override;
};

}  // namespace apsis::integrate
