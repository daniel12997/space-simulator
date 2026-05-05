// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: `Dp54` — Dormand-Prince 5(4) embedded adaptive Runge-Kutta
// (Hairer-Nørsett-Wanner "Solving Ordinary Differential Equations I" 2nd
// ed., Table 5.1). Adaptive step via the embedded 4th-order solution and
// a standard PI controller.
//
// Phase 1 status: this is the integrator that ships behind the
// IIntegrator seam. ADR-009 commits to a full DOP853 (Hairer Vol I Table
// 5.2) as the Phase-1 adaptive RK; Phase 1 ships DP5(4) as a
// coefficient-table stand-in to land the seam, the Phi augmentation, and
// the conformance gate within the schedule. Per ADR-009's "Phase 1
// Implementation Note", the upgrade to DP8(5,3) is a Phase 7 hardening
// item — the seam, adaptive-step control, and Phi-augmentation are
// unchanged at the upgrade. (Naming `Dp54` for what shipped is the
// honest name; the previous `Dop853` alias was renamed to remove the
// load-bearing lie.)

#pragma once

#include "apsis/integrate/iintegrator.h"

namespace apsis::integrate {

class Dp54 final : public IIntegrator {
 public:
  // rtol / atol per the embedded-error scaled-norm criterion (Hairer
  // Vol I §II.4). Adaptive step uses the standard PI step controller.
  struct Options {
    double rtol = 1e-13;
    double atol = 1e-9;
    // Initial step guess (s). If 0.0, an automatic estimate is used.
    double dt_initial = 0.0;
    // Step-size envelope.
    double dt_min = 1e-6;
    double dt_max = 3600.0;
    // Safety + growth/shrink limits for the controller.
    double safety = 0.9;
    double max_grow = 5.0;
    double min_shrink = 0.1;
    int max_iters_per_step = 32;
  };

  Dp54() noexcept = default;
  explicit Dp54(Options opts) noexcept : opts_(opts) {}

  StepResult step(apsis::time::Time<apsis::time::tags::TT> t,
                  const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                  const apsis::math::Mat6& phi,
                  double dt,
                  const apsis::force::IForceModel& force) override;

  [[nodiscard]] const Options& options() const noexcept { return opts_; }

 private:
  Options opts_;
};

}  // namespace apsis::integrate
