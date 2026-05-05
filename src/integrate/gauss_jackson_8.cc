// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: GaussJackson8 — see header for the Phase 1 fidelity note.

#include "apsis/integrate/gauss_jackson_8.h"

namespace apsis::integrate {

StepResult GaussJackson8::step(
    apsis::time::Time<apsis::time::tags::TT> t,
    const apsis::frames::State<apsis::frames::tags::ICRF>& x,
    const apsis::math::Mat6& phi,
    double dt,
    const apsis::force::IForceModel& force) {
  // Phase 1 stand-in: the fixed-step caller wants to advance by exactly
  // `dt`; we drive the internal Dp54 with a tight tolerance and return
  // the result. The full GJ8 second-sum implementation lands in Phase 7.
  return engine_.step(t, x, phi, dt, force);
}

}  // namespace apsis::integrate
