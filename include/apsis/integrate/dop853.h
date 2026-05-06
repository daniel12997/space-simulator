// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §D1: `Dop853` — Dormand-Prince 8(5,3) embedded adaptive
// Runge-Kutta. Hairer-Norsett-Wanner "Solving Ordinary Differential
// Equations I" (2nd ed.), Table 5.2; coefficients sourced from Hairer's
// official Fortran `dop853.f` (see `src/integrate/dop853_coeffs.h` for the
// transcription).
//
// Step controller is the **blended-error PI controller** described in
// Hairer Vol I §II.5: a 5th-order embedded estimate `err` and a 3rd-order
// embedded estimate `err2` are blended via `deno = err + 0.01 * err2`, and
// the step factor uses Lund stabilisation `(err)^(1/8 - 0.2*beta) /
// facold^beta`. Default `beta = 0.0` reduces to a plain I-controller; values
// up to 0.04 are recommended in §IV.2 for stiffer problems.
//
// The PI controller's `facold` (the previous accepted step's normalised
// error) is persisted on the `Dop853` instance across `step()` calls — it
// would otherwise reset on every entry and the integral term would lose
// memory. On the very first call we seed it with Hairer's dop853.f initial
// value of 1e-4.
//
// This adapter sits behind the `IIntegrator` seam alongside `Dp54`,
// `Yoshida4`, and (Phase 1A §D2) `GaussJackson8`. The Phi augmentation
// ride-along is unchanged from Dp54: dPhi/dt = A(t) * Phi is integrated at
// the same step but excluded from the error norm (per ADR-002 the Phi
// accuracy needs decouple from natural-state step control).

#pragma once

#include "apsis/integrate/iintegrator.h"

namespace apsis::integrate {

class Dop853 final : public IIntegrator {
 public:
  // Tolerances + step-size controller knobs. Defaults mirror Hairer's
  // dop853.f: SAFE=0.9, FAC1=0.333 (max grow = 1/FAC1 = 3.0), FAC2=6.0
  // (min shrink = 1/FAC2 = 1/6), BETA=0.0 (plain I-controller).
  struct Options {
    double rtol = 1e-13;
    double atol = 1e-9;
    // Initial step guess (s). If 0.0, an automatic estimate is used.
    double dt_initial = 0.0;
    // Step-size envelope.
    double dt_min = 1e-6;
    double dt_max = 3600.0;
    // Safety + grow/shrink + Lund-stabilisation exponent.
    double safety = 0.9;
    double max_grow = 3.0;          // Hairer 1/FAC1.
    double min_shrink = 1.0 / 6.0;  // Hairer 1/FAC2.
    double beta = 0.0;              // Lund-stabilisation exponent.
    int max_iters_per_step = 32;
  };

  Dop853() noexcept = default;
  explicit Dop853(Options opts) noexcept : opts_(opts) {}

  StepResult step(apsis::time::Time<apsis::time::tags::TT> t,
                  const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                  const apsis::math::Mat6& phi, double dt,
                  const apsis::force::IForceModel& force) override;

  [[nodiscard]] const Options& options() const noexcept { return opts_; }

  // Visible for testing the PI step controller. The Lund-stabilised PI
  // factor (Hairer Vol I §II.5) requires `facold` — the normalised error
  // estimate from the previous accepted step — to persist across step()
  // calls. Initialised to 1e-4 (Hairer's dop853.f convention for the very
  // first step). Read on rejected attempts; written on accepted steps.
  [[nodiscard]] double facold_for_test() const noexcept { return facold_; }

 private:
  Options opts_;
  double facold_ = 1e-4;
};

}  // namespace apsis::integrate
