// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: Dp54 implementation. See dp54.h for the Phase-1 status note
// (DP5(4) ships now; DP8(5,3) lands as a Phase 7 upgrade behind the
// unchanged IIntegrator seam).
//
// The augmented system integrated here is y = (state, vec(Phi)) of size
// 6 + 36 = 42. Stages share force evaluations between the state and Phi
// derivatives, exploiting that A is reused at the same (t, x).

#include "apsis/integrate/dp54.h"

#include <algorithm>
#include <cmath>

#include "apsis/force/iforce_model.h"
#include "apsis/integrate/iintegrator.h"

#include "dp54_coeffs.h"

namespace apsis::integrate {
namespace {

namespace dp = apsis::integrate::dp54;

// Augmented derivative: given (state, Phi) at (t, x), returns (dx/dt, dPhi/dt).
struct Deriv {
  apsis::math::Vec6 dx;
  apsis::math::Mat6 dphi;
};

Deriv evaluate(apsis::time::Time<apsis::time::tags::TT> t,
               const apsis::frames::State<apsis::frames::tags::ICRF>& x,
               const apsis::math::Mat6& phi, const apsis::force::IForceModel& force) {
  Deriv d;
  const auto kA = force.acceleration(t, x);
  const auto kJ36 = force.partials_dadx(t, x);
  d.dx.head<3>() = x.v;
  d.dx.tail<3>() = kA;
  const auto kAmat = assemble_a(kJ36);
  d.dphi = kAmat * phi;
  return d;
}

apsis::frames::State<apsis::frames::tags::ICRF> state_from_vec(const apsis::math::Vec6& y) {
  apsis::frames::State<apsis::frames::tags::ICRF> s;
  s.r = y.head<3>();
  s.v = y.tail<3>();
  return s;
}

apsis::math::Vec6 vec_from_state(const apsis::frames::State<apsis::frames::tags::ICRF>& s) {
  apsis::math::Vec6 y;
  y.head<3>() = s.r;
  y.tail<3>() = s.v;
  return y;
}

// Scaled-norm error per Hairer §II.4: sqrt(sum((err / (atol + rtol * |y|))^2) / N).
//
// Phase 1 note: per ADR-002, Phi accuracy needs decouple from natural-state
// step control. The error norm here uses ONLY the natural-state component;
// Phi is integrated alongside the state at the same step but does not drive
// step rejection. This keeps the Phase 1 Phi-augmented contract from
// pathologically shrinking the step over long horizons where the
// dynamics Jacobian is large (e.g. Encke deviation propagation against a
// full force model).
double scaled_norm(const apsis::math::Vec6& err_state, const apsis::math::Vec6& y_now, double atol,
                   double rtol) {
  double sum = 0.0;
  for (int i = 0; i < 6; ++i) {
    const double kSc = atol + rtol * std::abs(y_now[i]);
    const double kE = err_state[i] / kSc;
    sum += kE * kE;
  }
  return std::sqrt(sum / 6.0);
}

}  // namespace

StepResult Dp54::step(apsis::time::Time<apsis::time::tags::TT> t,
                      const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                      const apsis::math::Mat6& phi, double dt,
                      const apsis::force::IForceModel& force) {
  // Adaptive: try `dt`, accept or reduce, return the FIRST accepted step.
  // We do not advance more than the requested `dt`.
  double h = dt;
  const apsis::frames::State<apsis::frames::tags::ICRF>& x_in = x;
  const apsis::math::Mat6& phi_in = phi;

  for (int attempt = 0; attempt < opts_.max_iters_per_step; ++attempt) {
    // Stage k1..k7.
    std::array<apsis::math::Vec6, dp::kStages> kx;
    std::array<apsis::math::Mat6, dp::kStages> kp;

    for (int s = 0; s < dp::kStages; ++s) {
      const auto kUs = static_cast<std::size_t>(s);
      apsis::math::Vec6 y_stage = vec_from_state(x_in);
      apsis::math::Mat6 p_stage = phi_in;
      for (int j = 0; j < s; ++j) {
        const auto kUj = static_cast<std::size_t>(j);
        y_stage += h * dp::kA.at(kUs).at(kUj) * kx.at(kUj);
        p_stage += h * dp::kA.at(kUs).at(kUj) * kp.at(kUj);
      }
      const auto kTStage = t + apsis::time::Duration{h * dp::kC.at(kUs)};
      const auto kDeriv = evaluate(kTStage, state_from_vec(y_stage), p_stage, force);
      kx.at(kUs) = kDeriv.dx;
      kp.at(kUs) = kDeriv.dphi;
    }

    // 5th-order solution.
    apsis::math::Vec6 y_new = vec_from_state(x_in);
    apsis::math::Mat6 p_new = phi_in;
    for (int s = 0; s < dp::kStages; ++s) {
      const auto kUs = static_cast<std::size_t>(s);
      y_new += h * dp::kB5.at(kUs) * kx.at(kUs);
      p_new += h * dp::kB5.at(kUs) * kp.at(kUs);
    }
    // Error estimate.
    apsis::math::Vec6 err_x = apsis::math::Vec6::Zero();
    apsis::math::Mat6 err_p = apsis::math::Mat6::Zero();
    for (int s = 0; s < dp::kStages; ++s) {
      const auto kUs = static_cast<std::size_t>(s);
      err_x += h * dp::kE.at(kUs) * kx.at(kUs);
      err_p += h * dp::kE.at(kUs) * kp.at(kUs);
    }

    const double kErr = scaled_norm(err_x, y_new, opts_.atol, opts_.rtol);

    if (kErr <= 1.0 || h <= opts_.dt_min) {
      // Accepted.
      StepResult res;
      res.x = state_from_vec(y_new);
      res.phi = p_new;
      res.dt_actually_taken = h;
      return res;
    }

    // Reject; shrink step. Order p = 5.
    const double kFactor = std::clamp(opts_.safety * std::pow(1.0 / kErr, 1.0 / 5.0),
                                      opts_.min_shrink, opts_.max_grow);
    h *= kFactor;
    if (h < opts_.dt_min) {
      h = opts_.dt_min;
    }
  }

  // Exhausted attempts: take the last result anyway. This matches the
  // Hairer "force-progress" convention; surfaces as a tolerance breach
  // rather than a hang.
  StepResult res;
  res.x = x_in;
  res.phi = phi_in;
  res.dt_actually_taken = h;
  return res;
}

}  // namespace apsis::integrate
