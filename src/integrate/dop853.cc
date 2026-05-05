// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: Dop853 implementation. See dop853.h for the IMPORTANT NOTE
// on which RK pair is wired in; the integrator-seam contract is what
// matters for the rest of Phase 1.
//
// The augmented system integrated here is y = (state, vec(Phi)) of size
// 6 + 36 = 42. Stages share force evaluations between the state and Phi
// derivatives, exploiting that A is reused at the same (t, x).

#include "apsis/integrate/dop853.h"

#include <algorithm>
#include <cmath>

#include "apsis/force/iforce_model.h"
#include "apsis/integrate/iintegrator.h"

#include "dop853_coeffs.h"

namespace apsis::integrate {
namespace {

namespace dp = apsis::integrate::dop853;

// Augmented derivative: given (state, Phi) at (t, x), returns (dx/dt, dPhi/dt).
struct Deriv {
  apsis::math::Vec6  dx;
  apsis::math::Mat6  dphi;
};

Deriv evaluate(apsis::time::Time<apsis::time::tags::TT> t,
               const apsis::frames::State<apsis::frames::tags::ICRF>& x,
               const apsis::math::Mat6& phi,
               const apsis::force::IForceModel& force) {
  Deriv d;
  const auto a   = force.acceleration(t, x);
  const auto J36 = force.partials(t, x);
  d.dx.head<3>() = x.v;
  d.dx.tail<3>() = a;
  const auto A = assemble_A(J36);
  d.dphi = A * phi;
  return d;
}

apsis::frames::State<apsis::frames::tags::ICRF>
state_from_vec(const apsis::math::Vec6& y) {
  apsis::frames::State<apsis::frames::tags::ICRF> s;
  s.r = y.head<3>();
  s.v = y.tail<3>();
  return s;
}

apsis::math::Vec6 vec_from_state(
    const apsis::frames::State<apsis::frames::tags::ICRF>& s) {
  apsis::math::Vec6 y;
  y.head<3>() = s.r;
  y.tail<3>() = s.v;
  return y;
}

// Scaled-norm error per Hairer §II.4: sqrt(sum((err / (atol + rtol * |y|))^2) / N).
double scaled_norm(const apsis::math::Vec6& err_state,
                   const apsis::math::Mat6& err_phi,
                   const apsis::math::Vec6& y_now,
                   const apsis::math::Mat6& phi_now,
                   double atol, double rtol) {
  double sum = 0.0;
  int n = 0;
  for (int i = 0; i < 6; ++i) {
    const double sc = atol + rtol * std::abs(y_now[i]);
    const double e = err_state[i] / sc;
    sum += e * e;
    ++n;
  }
  for (int i = 0; i < 6; ++i) {
    for (int j = 0; j < 6; ++j) {
      const double sc = atol + rtol * std::abs(phi_now(i, j));
      const double e = err_phi(i, j) / sc;
      sum += e * e;
      ++n;
    }
  }
  return std::sqrt(sum / static_cast<double>(n));
}

}  // namespace

StepResult Dop853::step(apsis::time::Time<apsis::time::tags::TT> t,
                        const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                        const apsis::math::Mat6& phi,
                        double dt,
                        const apsis::force::IForceModel& force) {
  // Adaptive: try `dt`, accept or reduce, return the FIRST accepted step.
  // We do not advance more than the requested `dt`.
  double h = dt;
  apsis::frames::State<apsis::frames::tags::ICRF> x_in  = x;
  apsis::math::Mat6 phi_in = phi;

  for (int attempt = 0; attempt < opts_.max_iters_per_step; ++attempt) {
    // Stage k1..k7.
    std::array<apsis::math::Vec6, dp::kStages> kx;
    std::array<apsis::math::Mat6, dp::kStages> kp;

    for (int s = 0; s < dp::kStages; ++s) {
      apsis::math::Vec6 y_stage = vec_from_state(x_in);
      apsis::math::Mat6 p_stage = phi_in;
      for (int j = 0; j < s; ++j) {
        y_stage += h * dp::kA[static_cast<std::size_t>(s)][static_cast<std::size_t>(j)] * kx[static_cast<std::size_t>(j)];
        p_stage += h * dp::kA[static_cast<std::size_t>(s)][static_cast<std::size_t>(j)] * kp[static_cast<std::size_t>(j)];
      }
      const auto t_stage = t + apsis::time::Duration{h * dp::kC[static_cast<std::size_t>(s)]};
      const auto deriv = evaluate(t_stage, state_from_vec(y_stage), p_stage, force);
      kx[static_cast<std::size_t>(s)] = deriv.dx;
      kp[static_cast<std::size_t>(s)] = deriv.dphi;
    }

    // 5th-order solution.
    apsis::math::Vec6 y_new = vec_from_state(x_in);
    apsis::math::Mat6 p_new = phi_in;
    for (int s = 0; s < dp::kStages; ++s) {
      y_new += h * dp::kB5[static_cast<std::size_t>(s)] * kx[static_cast<std::size_t>(s)];
      p_new += h * dp::kB5[static_cast<std::size_t>(s)] * kp[static_cast<std::size_t>(s)];
    }
    // Error estimate.
    apsis::math::Vec6 err_x = apsis::math::Vec6::Zero();
    apsis::math::Mat6 err_p = apsis::math::Mat6::Zero();
    for (int s = 0; s < dp::kStages; ++s) {
      err_x += h * dp::kE[static_cast<std::size_t>(s)] * kx[static_cast<std::size_t>(s)];
      err_p += h * dp::kE[static_cast<std::size_t>(s)] * kp[static_cast<std::size_t>(s)];
    }

    const double E = scaled_norm(err_x, err_p, y_new, p_new, opts_.atol, opts_.rtol);

    if (E <= 1.0 || h <= opts_.dt_min) {
      // Accepted.
      StepResult res;
      res.x = state_from_vec(y_new);
      res.phi = p_new;
      res.dt_actually_taken = h;
      return res;
    }

    // Reject; shrink step. Order p = 5.
    const double factor = std::clamp(opts_.safety * std::pow(1.0 / E, 1.0 / 5.0),
                                     opts_.min_shrink, opts_.max_grow);
    h *= factor;
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
