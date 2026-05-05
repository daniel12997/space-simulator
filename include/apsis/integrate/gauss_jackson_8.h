// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: GaussJackson8 — fixed-step multi-step integrator.
//
// IMPORTANT NOTE ON FIDELITY: ADR-009 commits to the Berry-Healy 2004 8th-
// order second-sum Gauss-Jackson algorithm with an adaptive-RK starter.
// For the Phase 1 timeline, this class ships with a fixed-step Dp54-
// equivalent substep loop (each `step()` call performs the requested `dt`
// as a single adaptive Dp54 step capped at the requested size). The ordinate-form
// GJ8 with second-sum accumulation is tracked as a Phase 7 hardening
// item. The seam (`IIntegrator`) and the per-call contract are unchanged
// at the upgrade.

#pragma once

#include "apsis/integrate/dp54.h"
#include "apsis/integrate/iintegrator.h"

namespace apsis::integrate {

class GaussJackson8 final : public IIntegrator {
 public:
  GaussJackson8() noexcept = default;

  StepResult step(apsis::time::Time<apsis::time::tags::TT> t,
                  const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                  const apsis::math::Mat6& phi,
                  double dt,
                  const apsis::force::IForceModel& force) override;

 private:
  // Internal fixed-step engine — Phase 1 stand-in (see header note).
  Dp54 engine_;
};

}  // namespace apsis::integrate
