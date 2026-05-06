// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §6: Yoshida4 — 4th-order symplectic via Yoshida's triple
// composition of velocity-Verlet (kicks-drifts-kicks). Coefficients:
//
//   w0 = -2^(1/3) / (2 - 2^(1/3))
//   w1 = 1 / (2 - 2^(1/3))
//
// The composition applies Verlet steps with sizes (w1*dt, w0*dt, w1*dt).
// Each Verlet step is "drift dt/2; kick dt; drift dt/2" (DKD form).

#include "apsis/integrate/yoshida4.h"

#include <array>
#include <cmath>

#include "apsis/force/iforce_model.h"
#include "apsis/integrate/iintegrator.h"

namespace apsis::integrate {
namespace {

// Yoshida 1990 §3.
const double kCubeRoot2 = std::cbrt(2.0);
const double kW1 = 1.0 / (2.0 - kCubeRoot2);
const double kW0 = -kCubeRoot2 / (2.0 - kCubeRoot2);
const std::array<double, 3> kSubsteps = {kW1, kW0, kW1};

// One Verlet (DKD) sub-step of size h, advancing both state and Phi.
void verlet_step(apsis::time::Time<apsis::time::tags::TT>& t,
                 apsis::frames::State<apsis::frames::tags::ICRF>& x, apsis::math::Mat6& phi,
                 double h, const apsis::force::IForceModel& force) {
  // Drift dt/2.
  x.r += 0.5 * h * x.v;
  // Phi drift: top half advances by velocity-block of A. For the linear
  // augmented system we use a midpoint-style update consistent with the
  // Verlet flow: split A into kinetic (upper-right block = I) and
  // potential (bottom block = da/dr) parts, applying drifts and kicks
  // separately.
  apsis::math::Mat6 phi_drift_top = apsis::math::Mat6::Identity();
  phi_drift_top.block<3, 3>(0, 3) = 0.5 * h * apsis::math::Mat3::Identity();
  phi = phi_drift_top * phi;

  t += apsis::time::Duration{0.5 * h};

  // Kick (full-h on velocity using force at midpoint).
  const auto a = force.acceleration(t, x);
  const auto J36 = force.partials(t, x);
  x.v += h * a;
  apsis::math::Mat6 phi_kick = apsis::math::Mat6::Identity();
  phi_kick.block<3, 3>(3, 0) = h * J36.block<3, 3>(0, 0);
  phi = phi_kick * phi;

  // Drift dt/2.
  x.r += 0.5 * h * x.v;
  apsis::math::Mat6 phi_drift_bot = apsis::math::Mat6::Identity();
  phi_drift_bot.block<3, 3>(0, 3) = 0.5 * h * apsis::math::Mat3::Identity();
  phi = phi_drift_bot * phi;

  t += apsis::time::Duration{0.5 * h};
}

}  // namespace

StepResult Yoshida4::step(apsis::time::Time<apsis::time::tags::TT> t,
                          const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                          const apsis::math::Mat6& phi, double dt,
                          const apsis::force::IForceModel& force) {
  apsis::frames::State<apsis::frames::tags::ICRF> x_cur = x;
  apsis::math::Mat6 phi_cur = phi;
  apsis::time::Time<apsis::time::tags::TT> t_cur = t;

  for (double w : kSubsteps) {
    verlet_step(t_cur, x_cur, phi_cur, w * dt, force);
  }

  StepResult res;
  res.x = x_cur;
  res.phi = phi_cur;
  res.dt_actually_taken = dt;
  return res;
}

}  // namespace apsis::integrate
