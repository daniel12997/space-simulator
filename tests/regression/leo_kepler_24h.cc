// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-PHY-002, REQ-PHY-003, REQ-INT-001, REQ-INT-002
//
// Phase-1 §10: LEO Kepler 24-hour regression.
//
// Replaces the previous "ISS state-vector reproduction" deliverable. The
// original deliverable required real NASA-published reference vectors
// which are out of scope for Phase 1 (deferred to Phase 7 as a separate
// regression with proper sourcing). Per option-C of the PR review, this
// regression is recast as an *analytical-oracle* comparison:
//
//   1. Synthetic ISS-like initial state in J2000 (LEO-altitude orbit, ~6.8e6 m
//      geocentric radius, ~7.66 km/s circular speed).
//   2. Convert J2000 -> ICRF via `transform<ICRF, J2000>` so the plan §10
//      requirement to exercise the frame-bias seam is honoured.
//   3. Force model: `PointMass(Earth)` only — no spherical harmonics, no
//      third-body. This makes the trajectory exactly Keplerian, so the
//      f-and-g universal-variable propagator (`apsis::math::fandg`) is the
//      analytical oracle and the residual is pure integrator truncation.
//   4. Propagate forward 24 h with `Dop853` at rtol=1e-12 / atol=1e-9.
//   5. Compare to the f-and-g closed-form at the same final epoch.
//
// Tolerance: 2e-4 m position, 2e-7 m/s velocity. Picked experimentally
// against the local Phase 1A §D1 build — the Dop853 (Hairer Vol I Table
// 5.2) 24-h residual at rtol=1e-12 / atol=1e-9 / dt_max=60 s runs
// ~1.9e-5 m / ~2.1e-8 m/s on the development host (about 3 orders below
// Dp54's prior ~1.5e-2 m residual). The asserted bounds give ~10x margin
// so a noisy CI host doesn't flake the gate. The Phase 1 plan §10
// "anticipated ~7 orders" was over-optimistic: the dominant residual is
// step-control noise at rtol=1e-12 (rtol * r * sqrt(N_orbits) = ~3e-5 m),
// not the method's truncation. Tightening rtol below 1e-12 hits double-
// precision floor in the universal-anomaly oracle.
//
// What this test does NOT cover (deferred to Phase 7):
//   - Spherical-harmonic gravity (the SH adapter ships with FD partials in
//     Phase 1; analytical Pines gradient is a Phase 7 hardening item).
//   - Third-body perturbations (require a DE kernel; the Phase 1 JPL DE
//     round-trip in jpl_de_roundtrip.cc covers the third-body wiring).
//   - Real ISS state-vector reproduction (Phase 7 with sourced NASA data).

#include <gtest/gtest.h>

#include <cmath>

#include "apsis/force/point_mass.h"
#include "apsis/frames/state.h"
#include "apsis/frames/transform.h"
#include "apsis/integrate/dop853.h"

// f-and-g universal-variable propagator. Internal-only header under
// src/math/; the test target adds src/ to its include path so this
// resolves.
#include "math/f_and_g_series.h"

namespace ai = apsis::integrate;
namespace af = apsis::force;
namespace at = apsis::time;
namespace afr = apsis::frames;

namespace {

constexpr double kMuEarth = 3.986004418e14;
constexpr double kRadius = 6.8e6;  // ~ISS altitude (geocentric radius)

// Pure circular ISS-like initial state in J2000.
afr::State<afr::tags::J2000> initial_state_j2000() {
  afr::State<afr::tags::J2000> s;
  s.r << kRadius, 0.0, 0.0;
  // Circular speed v = sqrt(mu / r), inclined ~51.6 deg (ISS-like).
  const double v = std::sqrt(kMuEarth / kRadius);
  constexpr double kInc = 51.6 * M_PI / 180.0;
  s.v << 0.0, v * std::cos(kInc), v * std::sin(kInc);
  return s;
}

afr::State<afr::tags::ICRF> propagate_dop853(const af::IForceModel& force,
                                             afr::State<afr::tags::ICRF> x,
                                             at::Time<at::tags::TT> t, double horizon_sec) {
  ai::Dop853::Options opts;
  opts.rtol = 1e-12;
  opts.atol = 1e-9;
  opts.dt_max = 60.0;  // cap step at 60 s for fine resolution
  ai::Dop853 integ(opts);

  apsis::math::Mat6 phi = apsis::math::Mat6::Identity();
  double t_acc = 0.0;
  while (t_acc < horizon_sec) {
    const double step_dt = std::min(60.0, horizon_sec - t_acc);
    auto res = integ.step(t, x, phi, step_dt, force);
    x = res.x;
    phi = res.phi;
    t = t + at::Duration{res.dt_actually_taken};
    t_acc += res.dt_actually_taken;
  }
  return x;
}

TEST(LeoKepler24h, Dop853MatchesFAndGOracle) {
  // 1. Build J2000 initial state and 2. transform to ICRF via the seam.
  // The frame-bias transform is a constant rotation (~17 mas magnitude);
  // this exercises the same code path the future ISS-from-NASA-data test
  // will need.
  const auto x0_j2000 = initial_state_j2000();
  at::Time<at::tags::TT> t0{2460676.5, 0.0};
  const auto x0_icrf = afr::transform<afr::tags::ICRF, afr::tags::J2000>(x0_j2000, t0);

  // 3. Force model: pure central-body Kepler.
  af::PointMass earth_pm(kMuEarth);

  // 4. Propagate 24 h with Dop853 (numerical).
  constexpr double kHorizon = 24.0 * 3600.0;
  const auto x_num = propagate_dop853(earth_pm, x0_icrf, t0, kHorizon);

  // 5. Analytical oracle: f-and-g universal-variable Kepler propagation
  //    from the same initial state over the same horizon.
  const auto x_oracle = apsis::math::fandg::propagate(x0_icrf, kHorizon, kMuEarth);

  // Tolerances: numerical Dop853 vs f-and-g closed-form on a pure Kepler
  // orbit over 24 h. Empirical residual at rtol=1e-12 / atol=1e-9 with
  // dt_max=60 s is ~1.9e-5 m / ~2.1e-8 m/s on the development host (about
  // 3 orders below Dp54's residual on the same problem; see the file
  // header for why this is less than the §10-anticipated ~7 orders).
  // The bounds below give ~10x margin.
  const double dr = (x_num.r - x_oracle.r).norm();
  const double dv = (x_num.v - x_oracle.v).norm();
  EXPECT_LT(dr, 2e-4) << "Dop853 vs f-and-g 24-h closure: " << dr << " m";
  EXPECT_LT(dv, 2e-7) << "Dop853 vs f-and-g 24-h velocity closure: " << dv << " m/s";
}

}  // namespace
