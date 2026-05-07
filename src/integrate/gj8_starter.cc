// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §D'' (re-attempt of D2): GJ8 startup procedure. See
// gj8_starter.h for the contract.
//
// Algorithm (Berry-Healy 2004 §5.1, simplified for Phase 1A scope):
//   1. Propagate state at j=-4..-1 via backward f-and-g from center (j=0).
//   2. Propagate state at j=+1..+4 via forward f-and-g from center.
//   3. Evaluate force model at each of the 9 stencil epochs, capturing
//      acceleration and dPhi/dt = A * Phi (with Phi at the stencil position).
//   4. For Phi at off-center positions, use a central-difference oracle:
//      perturb each axis of (r, v) at the seed by (fd_pos_eps, fd_vel_eps),
//      propagate via f-and-g to each j epoch, and assemble Phi columns.
//   5. Bootstrap leading-edge first/second sums (s_lead, S_lead) and the
//      Phi-side first sum from the row-j=+4 ordinate-form formula
//      reproducing the f-and-g state at the leading edge.
//
// Phase 1A scope justification (per the deliverable plan): the f-and-g
// starter assumes the local dynamics are well-approximated by Kepler over
// the half-stencil span (4*h). For the conformance horizon (1 period
// circular Kepler at h=60s, or 1 hour Phi at h=60s) this is exact to
// machine precision (f-and-g IS Kepler). For perturbed dynamics in
// production, Berry-Healy 2004 §5.1 specifies a SECECE...CE iterative
// refinement of the starter accelerations; that lands in Phase 7
// alongside the closed-form Kepler STM upgrade (currently FD-oracle).

#include "gj8_starter.h"

#include "apsis/integrate/iintegrator.h"

#include "../math/f_and_g_series.h"
#include "gj8_coeffs.h"

namespace apsis::integrate::detail {
namespace {

namespace coeff = apsis::integrate::gj8;

// Propagate Phi to time t = j*h via central-difference perturbation of the
// seed state. Returns Phi(j*h) = STM mapping (r0, v0) -> (r(j*h), v(j*h))
// under Kepler-only motion. Six columns; each requires two f-and-g calls.
apsis::math::Mat6
phi_via_central_difference(const apsis::frames::State<apsis::frames::tags::ICRF>& x0, double dt,
                           double mu, double fd_pos_eps, double fd_vel_eps) {
  apsis::math::Mat6 phi;
  for (int axis = 0; axis < 6; ++axis) {
    auto x_plus = x0;
    auto x_minus = x0;
    const double kEps = (axis < 3) ? fd_pos_eps : fd_vel_eps;
    if (axis < 3) {
      x_plus.r[axis] += kEps;
      x_minus.r[axis] -= kEps;
    } else {
      x_plus.v[axis - 3] += kEps;
      x_minus.v[axis - 3] -= kEps;
    }
    const auto kStatePlus = apsis::math::fandg::propagate(x_plus, dt, mu);
    const auto kStateMinus = apsis::math::fandg::propagate(x_minus, dt, mu);
    apsis::math::Vec6 dx;
    dx.head<3>() = (kStatePlus.r - kStateMinus.r) / (2.0 * kEps);
    dx.tail<3>() = (kStatePlus.v - kStateMinus.v) / (2.0 * kEps);
    phi.col(axis) = dx;
  }
  return phi;
}

}  // namespace

StarterResult run_starter(apsis::time::Time<apsis::time::tags::TT> t_center,
                          const apsis::frames::State<apsis::frames::tags::ICRF>& x_center,
                          const apsis::math::Mat6& phi_center, double h,
                          const apsis::force::IForceModel& force, double mu_starter,
                          double fd_pos_eps, double fd_vel_eps) {
  StarterResult out;

  // 1+2. Populate states at j=-4..+4 using f-and-g.
  for (int j = -4; j <= 4; ++j) {
    const std::size_t kIdxJ{static_cast<std::size_t>(j + 4)};
    if (j == 0) {
      out.state.at(kIdxJ) = x_center;
    } else {
      out.state.at(kIdxJ) = apsis::math::fandg::propagate(x_center, j * h, mu_starter);
    }
  }

  // 3. Force-model accelerations at each stencil position.
  // For Phi at each j, propagate the seed Phi by the central-difference STM.
  for (int j = -4; j <= 4; ++j) {
    const std::size_t kIdxJ{static_cast<std::size_t>(j + 4)};
    const auto kTJ = t_center + apsis::time::Duration{static_cast<double>(j) * h};
    const auto kA = force.acceleration(kTJ, out.state.at(kIdxJ));
    out.accel.at(kIdxJ) = kA;

    apsis::math::Mat6 phi_j;
    if (j == 0) {
      phi_j = phi_center;
    } else {
      // Phi(j*h) under Kepler-only motion. Compose with seed Phi for the
      // ride-along propagation (right-multiply by phi_center, since the
      // seed Phi maps t0 -> t_center and the per-step Phi_kepler maps
      // t_center -> t_j).
      const auto kPhiKepler =
          phi_via_central_difference(x_center, j * h, mu_starter, fd_pos_eps, fd_vel_eps);
      phi_j = kPhiKepler * phi_center;
    }
    const auto kJ36 = force.partials_dadx(kTJ, out.state.at(kIdxJ));
    const auto kAMat = assemble_a(kJ36);
    out.dphi.at(kIdxJ) = kAMat * phi_j;

    // Cache forward Phi values at j=+1..+4 for the first 4 post-bootstrap
    // step() returns.
    if (j >= 1 && j <= 4) {
      out.phi_forward.at(static_cast<std::size_t>(j - 1)) = phi_j;
    }
  }

  // 5. Bootstrap leading-edge sums (s_lead, S_lead) from the row-9
  // (paper-j=+4) ordinate-form formulas reproducing the f-and-g state
  // at j=+4:
  //
  //   v_{n+4} = h * (s_lead + sum_k kB[9][k+4] * accel[k+4])
  //   r_{n+4} = h^2 * (S_lead + sum_k kA[9][k+4] * accel[k+4])
  //
  // Solving:
  //   s_lead = v_{n+4} / h - sum_k kB[9][k+4] * accel[k+4]
  //   S_lead = r_{n+4} / h^2 - sum_k kA[9][k+4] * accel[k+4]
  apsis::math::Vec3 sum_b_accel = apsis::math::Vec3::Zero();
  apsis::math::Vec3 sum_a_accel = apsis::math::Vec3::Zero();
  apsis::math::Mat6 sum_b_dphi = apsis::math::Mat6::Zero();
  for (std::size_t k = 0; k < coeff::kStencil; ++k) {
    sum_b_accel += coeff::kB.at(coeff::kRowCorrector).at(k) * out.accel.at(k);
    sum_a_accel += coeff::kA.at(coeff::kRowCorrector).at(k) * out.accel.at(k);
    sum_b_dphi += coeff::kB.at(coeff::kRowCorrector).at(k) * out.dphi.at(k);
  }
  const auto& x_lead = out.state.at(8);          // paper-j=+4
  const auto& phi_lead = out.phi_forward.at(3);  // paper-j=+4
  out.s_lead = x_lead.v / h - sum_b_accel;
  out.S_lead = x_lead.r / (h * h) - sum_a_accel;
  // Phi is first-order; bootstrap from row-9 reproducing phi at j=+4.
  out.s_phi_lead = phi_lead / h - sum_b_dphi;

  return out;
}

}  // namespace apsis::integrate::detail
