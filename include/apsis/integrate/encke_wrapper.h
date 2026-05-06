// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §7: EnckeWrapper — composes any IIntegrator with Encke
// perturbation propagation per [[concepts/long-arc-state-conditioning]].
//
// Decomposition: x(t) = x_kepler(t; x0, t0) + delta_x(t).
//   - x_kepler(t) is propagated analytically via the f-and-g series under
//     a Keplerian central-body field with parameter mu.
//   - delta_x(t) is propagated numerically by the wrapped integrator under
//     the *deviation* dynamics
//        d(delta_x)/dt = a_full(t, x_kepler + delta_x) - a_kepler(x_kepler)
//
// Periodic rectification: when |delta_r| / |r_kepler| exceeds
// `rectify_threshold` (default 0.01 = 1%), the reference is reset to the
// current full state and delta_x = 0.

#pragma once

#include <memory>

#include "apsis/integrate/iintegrator.h"

namespace apsis::integrate {

class EnckeWrapper final : public IIntegrator {
 public:
  struct Options {
    double rectify_threshold = 0.01;
    double mu = 0.0;  // central-body GM. Required (no sensible default).
  };

  // Wraps `inner` (whose lifetime must outlast this wrapper). The wrapper
  // does not take ownership.
  EnckeWrapper(IIntegrator* inner, Options opts) noexcept : inner_(inner), opts_(opts) {}

  StepResult step(apsis::time::Time<apsis::time::tags::TT> t,
                  const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                  const apsis::math::Mat6& phi, double dt,
                  const apsis::force::IForceModel& force) override;

 private:
  IIntegrator* inner_;
  Options opts_;
};

}  // namespace apsis::integrate
