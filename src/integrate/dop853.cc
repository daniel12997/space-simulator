// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §D1: `Dop853` implementation. See dop853.h and dop853_coeffs.h
// for the references; the structure below mirrors Hairer's dop853.f exactly
// (the 12-stage block, the blended err / err2 norm, and the Lund-stabilised
// PI step factor) translated into the same augmented (state, Phi) pattern
// already used by Dp54.

#include "apsis/integrate/dop853.h"

#include <algorithm>
#include <array>
#include <cmath>

#include "apsis/force/iforce_model.h"
#include "apsis/integrate/iintegrator.h"

#include "dop853_coeffs.h"

namespace apsis::integrate {
namespace {

namespace dp = apsis::integrate::dop853;

// Augmented derivative: (dx/dt, dPhi/dt).
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

// Hairer dop853.f lines ~654–677: blended error norm.
//
//   sk_i        = atol + rtol * max(|y_i|, |y1_i|)
//   sum_err_sq  = sum_i ((er . k)_i / sk_i)^2          (5th-order estimate)
//   sum_err2_sq = sum_i (((b.k) - bhh1*k1 - bhh2*k9 - bhh3*k12)_i / sk_i)^2
//   deno        = sum_err_sq + 0.01 * sum_err2_sq
//   ERR         = |h| * sqrt(sum_err_sq / (N * deno))
//
// Phi is excluded from the error norm per ADR-002's "Phi accuracy decouples
// from natural-state step control" rule. Returns the dimensionless ERR;
// caller multiplies by |h|.
double blended_error(const std::array<apsis::math::Vec6, dp::kStages>& kx,
                     const apsis::math::Vec6& y_old, const apsis::math::Vec6& y_new, double h,
                     double atol, double rtol) {
  double sum_err = 0.0;
  double sum_err2 = 0.0;
  for (int i = 0; i < 6; ++i) {
    const double kSk = atol + rtol * std::max(std::abs(y_old[i]), std::abs(y_new[i]));

    double er_dot_k = 0.0;
    double b_dot_k = 0.0;
    for (std::size_t s = 0; s < dp::kStages; ++s) {
      er_dot_k += dp::kEr.at(s) * kx.at(s)[i];
      b_dot_k += dp::kB.at(s) * kx.at(s)[i];
    }
    const double kErrI = er_dot_k / kSk;
    sum_err += kErrI * kErrI;

    // 3rd-order estimate uses (b.k) minus the bhh-weighted subset of stages.
    // Hairer's `k4 = b.k` then ERR2 := k4 - bhh1*k1 - bhh2*k9 - bhh3*k12
    // (`k3last` in the Fortran reuses slot K3 after the 12th-stage call).
    const double kErr2I =
        (b_dot_k - dp::kBhh1 * kx.at(0)[i] - dp::kBhh2 * kx.at(8)[i] - dp::kBhh3 * kx.at(11)[i]) /
        kSk;
    sum_err2 += kErr2I * kErr2I;
  }

  const double kDeno = sum_err + 0.01 * sum_err2;
  if (kDeno <= 0.0) {
    return 0.0;
  }
  const double kN = 6.0;
  return std::abs(h) * std::sqrt(sum_err / (kN * kDeno));
}

}  // namespace

StepResult Dop853::step(apsis::time::Time<apsis::time::tags::TT> t,
                        const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                        const apsis::math::Mat6& phi, double dt,
                        const apsis::force::IForceModel& force) {
  double h = dt;
  const apsis::frames::State<apsis::frames::tags::ICRF>& x_in = x;
  const apsis::math::Mat6& phi_in = phi;

  // Lund stabilisation memory — `facold_` is the previous accepted step's
  // normalised error (instance member, persists across step() calls). Hairer
  // §II.5 / dop853.f seeds the very first step with 1e-4 (the constructor
  // initialiser handles that) and then carries the accepted-step `err` value
  // forward thereafter, so the PI controller's "I" term has memory across
  // both internal retries within one step() call AND across successive
  // step() calls in a longer integration.
  const double kExpo1 = (1.0 / 8.0) - opts_.beta * 0.2;

  for (int attempt = 0; attempt < opts_.max_iters_per_step; ++attempt) {
    std::array<apsis::math::Vec6, dp::kStages> kx;
    std::array<apsis::math::Mat6, dp::kStages> kp;

    // 12-stage Butcher block.
    constexpr std::size_t kInnerLen = std::tuple_size_v<std::decay_t<decltype(dp::kA[0])>>;
    for (std::size_t s = 0; s < dp::kStages; ++s) {
      apsis::math::Vec6 y_stage = vec_from_state(x_in);
      apsis::math::Mat6 p_stage = phi_in;
      for (std::size_t j = 0; j < s && j < kInnerLen; ++j) {
        const double kAij = dp::kA.at(s).at(j);
        if (kAij != 0.0) {
          y_stage += h * kAij * kx.at(j);
          p_stage += h * kAij * kp.at(j);
        }
      }
      const auto kTStage = t + apsis::time::Duration{h * dp::kC.at(s)};
      const auto kDeriv = evaluate(kTStage, state_from_vec(y_stage), p_stage, force);
      kx.at(s) = kDeriv.dx;
      kp.at(s) = kDeriv.dphi;
    }

    // 8th-order solution from b.k.
    apsis::math::Vec6 y_new = vec_from_state(x_in);
    apsis::math::Mat6 p_new = phi_in;
    for (std::size_t s = 0; s < dp::kStages; ++s) {
      const double kBs = dp::kB.at(s);
      if (kBs != 0.0) {
        y_new += h * kBs * kx.at(s);
        p_new += h * kBs * kp.at(s);
      }
    }

    const auto kY0 = vec_from_state(x_in);
    const double kErr = blended_error(kx, kY0, y_new, h, opts_.atol, opts_.rtol);

    if (kErr <= 1.0 || h <= opts_.dt_min) {
      // Accept. Persist the accepted-step error as `facold_` for the next
      // step's PI factor. Hairer's `MAX(ERR, 1.0E-4)` clamp prevents a
      // spuriously tiny accepted error from causing an explosive grow on
      // the next step.
      facold_ = std::max(kErr, 1e-4);
      StepResult res;
      res.x = state_from_vec(y_new);
      res.phi = p_new;
      res.dt_actually_taken = h;
      return res;
    }

    // Reject: Lund-stabilised PI factor.
    //   fac11 = err^expo1 ; fac = fac11 / facold^beta
    //   h_new = h / max(facc2, min(facc1, fac/safety))
    const double kFac11 = std::pow(kErr, kExpo1);
    const double kFac = kFac11 / std::pow(facold_, opts_.beta);
    const double kFacc1 = 1.0 / opts_.max_grow;
    const double kFacc2 = 1.0 / opts_.min_shrink;
    const double kFactor = std::max(kFacc2, std::min(kFacc1, kFac / opts_.safety));
    h = h / kFactor;
    if (h < opts_.dt_min) {
      h = opts_.dt_min;
    }
  }

  // Exhausted attempts: surface as tolerance breach (matches Dp54 convention).
  StepResult res;
  res.x = x_in;
  res.phi = phi_in;
  res.dt_actually_taken = h;
  return res;
}

}  // namespace apsis::integrate
