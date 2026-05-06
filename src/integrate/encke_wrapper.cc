// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §7: EnckeWrapper implementation.
//
// Approach (per the plan):
//   1. Maintain (x_ref0, t_ref0) — the reference epoch and state.
//   2. For each `step(t, x, phi, dt)`:
//      a. Compute x_kep_now    = fandg(x_ref0, t - t_ref0)
//                 x_kep_next   = fandg(x_ref0, t - t_ref0 + dt)
//      b. Define a deviation-force adapter that returns
//             a_dev(t', x_dev) = a_full(t', x_kep(t' from ref) + x_dev)
//                                - a_kep_at_x_kep
//         For Phase 1 short steps we approximate x_kep across the inner
//         integrator's stages via linear interpolation between
//         x_kep_now and x_kep_next; this is exact at the endpoints and
//         introduces O(dt^2) error in the deviation magnitude (NOT in the
//         output, which is x = x_kep + dev — the deviation's magnitude is
//         already small). For ISS-class arcs the approximation is
//         comfortably below the regression-test tolerance.
//      c. Step the inner integrator with the deviation-force on the
//         current deviation x - x_kep_now.
//      d. Add x_kep_next + dev_next to obtain the new full state.
//      e. If |dev_next.r| / |x_kep_next.r| > rectify_threshold, set
//         (x_ref0, t_ref0) = (x_full_next, t + dt); zero deviation.
//
// EnckeWrapper does NOT cache (x_ref0, t_ref0) across calls — Phase 1
// uses a per-step recompute starting from `x` itself; this matches the
// "per-call" semantics of IIntegrator and keeps Encke stateless. A
// future optimisation could cache the reference across calls when the
// caller signals it has not crossed the rectification threshold.

#include "apsis/integrate/encke_wrapper.h"

#include "apsis/force/iforce_model.h"

#include "../math/f_and_g_series.h"

namespace apsis::integrate {
namespace {

// Force adapter that subtracts a constant Kepler acceleration of a
// reference position-at-time from the inner force evaluation.
class DeviationForce final : public apsis::force::IForceModel {
 public:
  DeviationForce(const apsis::force::IForceModel& full,
                 const apsis::frames::State<apsis::frames::tags::ICRF>& x_ref0,
                 apsis::time::Time<apsis::time::tags::TT> t_ref0, double mu)
      : full_(full), x_ref0_(x_ref0), t_ref0_(t_ref0), mu_(mu) {}

  [[nodiscard]] apsis::math::Vec3
  acceleration(apsis::time::Time<apsis::time::tags::TT> t,
               const apsis::frames::State<apsis::frames::tags::ICRF>& x_dev) const override {
    // Reference position at this time, via f-and-g.
    const double kDtFromRef = (t - t_ref0_).seconds();
    auto x_kep = apsis::math::fandg::propagate(x_ref0_, kDtFromRef, mu_);
    apsis::frames::State<apsis::frames::tags::ICRF> x_full;
    x_full.r = x_kep.r + x_dev.r;
    x_full.v = x_kep.v + x_dev.v;
    const auto kAFull = full_.acceleration(t, x_full);
    // Kepler acceleration of the reference's position.
    const double kR3 = x_kep.r.norm();
    const apsis::math::Vec3 kAKep = -(mu_ / (kR3 * kR3 * kR3)) * x_kep.r;
    return kAFull - kAKep;
  }

  [[nodiscard]] apsis::math::Mat36
  partials(apsis::time::Time<apsis::time::tags::TT> t,
           const apsis::frames::State<apsis::frames::tags::ICRF>& x_dev) const override {
    // Partial wrt x_dev. The Kepler subtraction is a constant in x_dev,
    // so its partial vanishes; we just return the full-force partials at
    // x_full = x_kep + x_dev.
    const double kDtFromRef = (t - t_ref0_).seconds();
    auto x_kep = apsis::math::fandg::propagate(x_ref0_, kDtFromRef, mu_);
    apsis::frames::State<apsis::frames::tags::ICRF> x_full;
    x_full.r = x_kep.r + x_dev.r;
    x_full.v = x_kep.v + x_dev.v;
    return full_.partials(t, x_full);
  }

 private:
  const apsis::force::IForceModel& full_;
  apsis::frames::State<apsis::frames::tags::ICRF> x_ref0_;
  apsis::time::Time<apsis::time::tags::TT> t_ref0_;
  double mu_;
};

}  // namespace

StepResult EnckeWrapper::step(apsis::time::Time<apsis::time::tags::TT> t,
                              const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                              const apsis::math::Mat6& phi, double dt,
                              const apsis::force::IForceModel& force) {
  // Per-step rebase: the reference is `x` at `t`. Deviation starts at zero.
  apsis::frames::State<apsis::frames::tags::ICRF> dev0;  // r = v = 0
  DeviationForce dev_force(force, x, t, opts_.mu);
  auto inner_res = inner_->step(t, dev0, phi, dt, dev_force);

  // Reconstruct full state: x_kepler_at(t + dt) + deviation_at(t + dt).
  auto x_kep_next = apsis::math::fandg::propagate(x, inner_res.dt_actually_taken, opts_.mu);
  StepResult res;
  res.x.r = x_kep_next.r + inner_res.x.r;
  res.x.v = x_kep_next.v + inner_res.x.v;
  res.phi = inner_res.phi;
  res.dt_actually_taken = inner_res.dt_actually_taken;
  return res;
}

}  // namespace apsis::integrate
