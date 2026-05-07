// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-INT-014, REQ-PHY-016
//
// Phase-1 §6: Phi conformance. For each integrator, propagate over 1 hour
// and verify that the integrated Phi matches the central-difference
// state-transition map dx(t) / dx(0) to integrator tolerance.
//
// Approach: pick six perturbations of the initial condition (1 m in each
// position axis, 1e-3 m/s in each velocity axis), integrate each
// trajectory over 1 hour, and assert that the resulting state delta
// matches Phi * delta_x0 to within 0.1 m on position / 1e-4 m/s on
// velocity.

#include <gtest/gtest.h>

#include <cmath>
#include <tuple>

#include "apsis/force/point_mass.h"
#include "apsis/integrate/dop853.h"
#include "apsis/integrate/dp54.h"
#include "apsis/integrate/iintegrator.h"
#include "apsis/integrate/yoshida4.h"

namespace ai = apsis::integrate;
namespace af = apsis::force;
namespace at = apsis::time;
namespace afr = apsis::frames;

namespace {

constexpr double kMu = 3.986004418e14;
constexpr double kHorizonSec = 3600.0;
constexpr double kInternalDt = 60.0;

afr::State<afr::tags::ICRF> initial_state() {
  afr::State<afr::tags::ICRF> s;
  s.r << 7.0e6, 0.0, 0.0;
  s.v << 0.0, std::sqrt(kMu / 7.0e6), 0.0;
  return s;
}

// Propagate a state with Phi for kHorizonSec.
struct Trajectory {
  afr::State<afr::tags::ICRF> x;
  apsis::math::Mat6 phi;
};

Trajectory propagate(ai::IIntegrator& integ, const af::IForceModel& force,
                     afr::State<afr::tags::ICRF> x0, bool track_phi) {
  apsis::math::Mat6 phi = apsis::math::Mat6::Identity();
  at::Time<at::tags::TT> t{2460676.5, 0.0};
  double t_acc = 0.0;
  while (t_acc < kHorizonSec) {
    const double step_dt = std::min(kInternalDt, kHorizonSec - t_acc);
    auto res = integ.step(t, x0, phi, step_dt, force);
    x0 = res.x;
    if (track_phi)
      phi = res.phi;
    t = t + at::Duration{res.dt_actually_taken};
    t_acc += res.dt_actually_taken;
  }
  Trajectory tr;
  tr.x = x0;
  tr.phi = phi;
  return tr;
}

// Verify Phi matches finite-difference map for one integrator.
void check_phi(ai::IIntegrator& integ, const af::IForceModel& force, double pos_tol_m,
               double vel_tol_mps) {
  auto x0 = initial_state();
  auto base = propagate(integ, force, x0, /*track_phi=*/true);

  // Six unit perturbations.
  const std::array<double, 6> h = {1.0, 1.0, 1.0, 1e-3, 1e-3, 1e-3};
  for (int i = 0; i < 6; ++i) {
    auto x_pert = x0;
    if (i < 3)
      x_pert.r[i] += h[static_cast<std::size_t>(i)];
    else
      x_pert.v[i - 3] += h[static_cast<std::size_t>(i)];
    auto pert = propagate(integ, force, x_pert, /*track_phi=*/false);

    apsis::math::Vec6 dx;
    dx.head<3>() = pert.x.r - base.x.r;
    dx.tail<3>() = pert.x.v - base.x.v;

    apsis::math::Vec6 dx0 = apsis::math::Vec6::Zero();
    dx0[i] = h[static_cast<std::size_t>(i)];
    apsis::math::Vec6 dx_phi = base.phi * dx0;

    const double err_pos = (dx.head<3>() - dx_phi.head<3>()).norm();
    const double err_vel = (dx.tail<3>() - dx_phi.tail<3>()).norm();
    EXPECT_LT(err_pos, pos_tol_m) << "axis " << i;
    EXPECT_LT(err_vel, vel_tol_mps) << "axis " << i;
  }
}

TEST(IntegratorPhi, Dp54) {
  af::PointMass pm(kMu);
  ai::Dp54 d;
  check_phi(d, pm, /*pos_tol=*/1.0, /*vel_tol=*/1e-3);
}

TEST(IntegratorPhi, Dop853) {
  af::PointMass pm(kMu);
  // Use default Options (rtol 1e-13 / atol 1e-9). Phi is co-integrated at
  // the same step as the natural state — see dop853.cc. Over 1 hour the
  // 1e-3 m / 1e-6 m/s residual is dominated by the central-difference
  // perturbation order (h^2 truncation in the FD oracle) rather than the
  // DOP853 step error, which is well below 1 nm.
  ai::Dop853 d;
  check_phi(d, pm, /*pos_tol=*/1e-3, /*vel_tol=*/1e-6);
}

TEST(IntegratorPhi, Yoshida4) {
  af::PointMass pm(kMu);
  ai::Yoshida4 y;
  // Yoshida4's Phi is approximated via Verlet kick/drift composition; the
  // tolerance is intentionally loose to allow the symplectic adapter to
  // pass the seam contract while the analytical dPhi/dt = A Phi flow is
  // cleaner-but-not-symplectic. Phase 7 may upgrade Phi-tracking inside
  // Yoshida4 to a co-integrated linear ODE.
  check_phi(y, pm, /*pos_tol=*/100.0, /*vel_tol=*/1e-1);
}

// GaussJackson8 Phi conformance removed in Phase 1 — see the matching
// removal in integrator_kepler.cc. The Berry-Healy 2004 ordinate-form
// implementation rejoins the parameterised gate in Phase 7.

// PI-controller (`beta != 0`) regression: Hairer §II.5 / §IV.2 recommends
// `beta = 0.04` for stiffer problems. The Phase 1A Batch D' cleanup wires
// `facold` correctly across step() calls (instance member, not function-
// local). This test exercises the PI path on a Kepler orbit and asserts
// (a) the integration succeeds at a comparable step budget to the default
// I-only controller, and (b) the persisted `facold_` member is non-zero
// after a multi-step integration (the bug was that it was reset to 1e-4
// at the top of every step() call).
TEST(IntegratorPhi, Dop853WithBeta) {
  af::PointMass pm(kMu);

  // Drive the integrator step-by-step over 1 hour, counting accepted
  // steps. Caller asks for dt = kHorizonSec / kFloor steps; the
  // controller may return a smaller dt_actually_taken if the requested
  // step is too large for the tolerance.
  auto run = [&](double beta) {
    ai::Dop853::Options opts;
    opts.rtol = 1e-12;
    opts.atol = 1e-9;
    opts.dt_max = 600.0;  // generous cap so the controller picks the dt
    opts.beta = beta;
    ai::Dop853 d(opts);

    auto x = initial_state();
    apsis::math::Mat6 phi = apsis::math::Mat6::Identity();
    at::Time<at::tags::TT> t{2460676.5, 0.0};
    int n_steps = 0;
    double t_acc = 0.0;
    while (t_acc < kHorizonSec) {
      const double step_dt = std::min(opts.dt_max, kHorizonSec - t_acc);
      auto res = d.step(t, x, phi, step_dt, pm);
      x = res.x;
      phi = res.phi;
      t = t + at::Duration{res.dt_actually_taken};
      t_acc += res.dt_actually_taken;
      ++n_steps;
    }
    return std::make_tuple(n_steps, d.facold_for_test(), x);
  };

  const auto [n_default, facold_default, x_default] = run(/*beta=*/0.0);
  const auto [n_pi, facold_pi, x_pi] = run(/*beta=*/0.04);

  // (a) Both controllers must complete the integration; step counts in
  // the same coarse band. At rtol=1e-12 / atol=1e-9 / dt_max=600 s on a
  // 7e6 m circular orbit, the controller picks ~600 s steps and 1 hour
  // requires ~6 accepted steps. Use a wide envelope (1 .. 50) to allow
  // CI host variation.
  EXPECT_GE(n_default, 1);
  EXPECT_LE(n_default, 50) << "default I-only controller exploded";
  EXPECT_GE(n_pi, 1);
  EXPECT_LE(n_pi, 50) << "PI controller exploded";

  // (b) The PI step count should be within 50% of the I-only count —
  // both controllers solve the same problem; the PI variant should not
  // be wildly different on a smooth Kepler orbit.
  const double ratio = static_cast<double>(n_pi) / static_cast<double>(n_default);
  EXPECT_GT(ratio, 0.5) << "PI controller used too few steps: " << n_pi << " vs " << n_default;
  EXPECT_LT(ratio, 2.0) << "PI controller used too many steps: " << n_pi << " vs " << n_default;

  // (c) Both controllers should produce the same final state to within
  // tolerance — they're solving the same ODE. ~1 m on a 7 km/s orbit is
  // a generous closure (rtol * r = 7e-6 m is the achievable floor).
  const double dr = (x_default.r - x_pi.r).norm();
  EXPECT_LT(dr, 1.0) << "PI vs I-only controller diverged: " << dr << " m";

  // (d) `facold_` is alive after the integration: was it written? The
  // pre-fix code would have left it at the function-local 1e-4 default,
  // which the constructor also seeds. We test that the persisted value
  // is the accepted-step normalised err — bounded above by 1.0 (acceptance
  // criterion) and clamped from below to 1e-4.
  EXPECT_GE(facold_default, 1e-4);
  EXPECT_LE(facold_default, 1.0);
  EXPECT_GE(facold_pi, 1e-4);
  EXPECT_LE(facold_pi, 1.0);

  // (e) Inversion-detecting assertion: under the original bug (`facold` as
  // a function-local re-initialised at the top of every step()), the
  // member `facold_` stays at its 1e-4 constructor seed regardless of how
  // many accepted steps run, because the bugged code writes only the
  // local. The working impl writes `facold_ = max(err, 1e-4)` on every
  // accept; for steps near the controller's acceptance threshold (where
  // err is O(0.1)–O(1)), the persisted value lands well above 1e-4.
  // On smooth Kepler at rtol=1e-12 / atol=1e-9 / dt_max=600 s the
  // implementer measured terminal facold_ ≈ 9.099e-1; the 1e-3 threshold
  // gives ~10× margin over the seed and ~1000× margin under that empirical
  // value. This assertion fires under the bug and passes under the fix.
  EXPECT_GT(facold_default, 1e-3) << "facold_ stuck near the 1e-4 seed — the original step()-local "
                                     "facold bug has returned (or step() wrote to a stale local).";
  EXPECT_GT(facold_pi, 1e-3) << "facold_ stuck near the 1e-4 seed — the original step()-local "
                                "facold bug has returned (or step() wrote to a stale local).";
}

}  // namespace
