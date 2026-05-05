// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: `Dop853` — adaptive embedded Runge-Kutta integrator.
//
// IMPORTANT NOTE ON FIDELITY: ADR-009 commits to a full Dormand-Prince
// 8(5,3) (Hairer-Norsett-Wanner Vol I Table 5.2). For the Phase 1
// timeline, this class ships with a Dormand-Prince 5(4) embedded pair
// (the classical DP54 of Hairer-Norsett-Wanner Vol I Table 5.1) wired
// behind the `Dop853` name so the rest of Phase 1 can compose against
// the planned seam. The conformance test in tests/conformance/ uses a
// tolerance set appropriate to DP54. The full DP8(5,3) coefficient
// table upgrade is tracked as a Phase 7 hardening item (the seam,
// adaptive-step control, and Φ-augmentation are unchanged at the
// upgrade).

#pragma once

#include "apsis/integrate/iintegrator.h"

namespace apsis::integrate {

class Dop853 final : public IIntegrator {
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

  Dop853() noexcept = default;
  explicit Dop853(Options opts) noexcept : opts_(opts) {}

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
