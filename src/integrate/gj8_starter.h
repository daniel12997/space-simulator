// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §D'' (re-attempt of D2): GJ8 startup procedure (Berry-Healy 2004
// §5.1). Builds an 8-point symmetric stencil around the seed epoch using
// f-and-g (universal-variable Kepler) propagation, then evaluates the full
// force model at all 9 stencil epochs to produce the initial acceleration
// ring buffer. The natural-state first/second sums are bootstrapped from
// the row-(j=0) ordinate-form formula evaluated at the center.
//
// Internal-only header — used by gauss_jackson_8.cc; not public.

#pragma once

#include <array>

#include "apsis/force/iforce_model.h"
#include "apsis/frames/frame_tags.h"
#include "apsis/frames/state.h"
#include "apsis/math/types.h"
#include "apsis/time/scale_tags.h"
#include "apsis/time/time.h"

namespace apsis::integrate::detail {

// Outputs of one GJ8 startup pass.
struct StarterResult {
  // Accelerations at stencil positions paper-j = -4..+4 (9 entries).
  std::array<apsis::math::Vec3, 9> accel;
  // dPhi/dt = A(t) * Phi at each stencil position.
  std::array<apsis::math::Mat6, 9> dphi;
  // States at stencil positions j = -4..+4 (used for sum bootstrap and
  // for the first 4 post-bootstrap step() deliveries at j=+1..+4).
  std::array<apsis::frames::State<apsis::frames::tags::ICRF>, 9> state;
  // Phi values at j=+1..+4 (central-difference oracle), cached for the
  // first 4 post-bootstrap step() returns. j=0 is the seed Phi (caller-
  // supplied).
  std::array<apsis::math::Mat6, 4> phi_forward;
  // First and second sums at the LEADING EDGE of the stencil (paper-j=+4),
  // bootstrapped from the row-9 (paper-j=+4) ordinate-form formulas
  // reproducing the f-and-g state at j=+4. See gauss_jackson_8.h for the
  // recurrence that advances these per step.
  apsis::math::Vec3 s_lead;
  apsis::math::Vec3 S_lead;
  // Phi-side first sum at the leading edge.
  apsis::math::Mat6 s_phi_lead;
};

// Run the GJ8 starter at center epoch `t_center`, center state `x_center`,
// center Phi `phi_center`, time step `h`, force model `force`, central-body
// `mu_starter` (for the Kepler-only f-and-g back-step), and Phi finite-
// difference perturbation epsilons (`fd_pos_eps`, `fd_vel_eps`).
StarterResult run_starter(apsis::time::Time<apsis::time::tags::TT> t_center,
                          const apsis::frames::State<apsis::frames::tags::ICRF>& x_center,
                          const apsis::math::Mat6& phi_center, double h,
                          const apsis::force::IForceModel& force, double mu_starter,
                          double fd_pos_eps, double fd_vel_eps);

}  // namespace apsis::integrate::detail
