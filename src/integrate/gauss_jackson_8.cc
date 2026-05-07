// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §D'' (re-attempt of D2): GaussJackson8 — Berry-Healy 2004
// ordinate-form summed-Adams / summed-Cowell PECE. See gauss_jackson_8.h
// for the contract; gj8_coeffs.h for the table source-of-truth.
//
// Step routine outline:
//   * On the first call (or any call where the input (t, x) is not a
//     continuation of the last returned state), the f-and-g starter is
//     run to populate the 9-point stencil and bootstrap the running
//     first/second sums.
//   * The first 4 post-bootstrap calls return state_[5..8] — the
//     f-and-g forward points at paper-j = +1..+4 — directly. This is
//     exact for Kepler dynamics (the conformance scope) and acceptable
//     within the half-stencil span for perturbed dynamics (the SECECE
//     starter refinement that Berry-Healy §5.1 describes for production
//     use is a Phase 7 follow-up).
//   * From call 5 onward, each step() runs the PECE cycle:
//       1. Predict r/v at paper-j=+5 using row-0 of kA / kB and the
//          OLD stencil's sums. Predict Phi at j=+5 using row-0 of kB.
//       2. Evaluate force at the predicted state to produce
//          ddot_r_pred and dPhi/dt_pred.
//       3. Shift the ring buffers by one (drop j=-4, slide all entries
//          by one, leaving j=+4 of the new stencil as the predicted
//          point's slot).
//       4. Correct r/v at the new j=+4 using row-9 of kA / kB and the
//          NEW stencil + trapezoidal sums. Same for Phi.
//       5. (PECE) Re-evaluate force at the corrected state and
//          replace accel_[8] / dphi_[8] with the corrected values. PEC
//          (opts_.pece == false) skips this re-evaluation.
//
// Running-sum convention: see ADR-009 Phase 1A Note D'' for the
// derivation. Briefly: predictor uses half-step `s = s_lead + 0.5 *
// ddot_r_lead`; corrector uses trapezoidal `s = s_lead + 0.5 *
// (ddot_r_lead + ddot_r_slot8_new)`. Second sum `S_new = S_lead +
// s_lead + 0.5 * ddot_r_lead` for both. Verified empirically against
// circular Kepler (residual ~4e-12 m/s per step at h=60 s).

#include "apsis/integrate/gauss_jackson_8.h"

#include <cmath>

#include "gj8_coeffs.h"
#include "gj8_starter.h"

namespace apsis::integrate {
namespace {

namespace coeff = apsis::integrate::gj8;

// Evaluate the row-j position formula:
//   r_{n+j} = h^2 * (S_at_j + sum_k kA[row][k] * accel[k])
apsis::math::Vec3 eval_row_position(std::size_t row, const apsis::math::Vec3& s_sum,
                                    const std::array<apsis::math::Vec3, 9>& accel, double h) {
  apsis::math::Vec3 acc_sum = apsis::math::Vec3::Zero();
  for (std::size_t k = 0; k < coeff::kStencil; ++k) {
    acc_sum += coeff::kA.at(row).at(k) * accel.at(k);
  }
  return h * h * (s_sum + acc_sum);
}

// Evaluate the row-j velocity formula:
//   dot_r_{n+j} = h * (s_at_j + sum_k kB[row][k] * accel[k])
apsis::math::Vec3 eval_row_velocity(std::size_t row, const apsis::math::Vec3& s_sum,
                                    const std::array<apsis::math::Vec3, 9>& accel, double h) {
  apsis::math::Vec3 acc_sum = apsis::math::Vec3::Zero();
  for (std::size_t k = 0; k < coeff::kStencil; ++k) {
    acc_sum += coeff::kB.at(row).at(k) * accel.at(k);
  }
  return h * (s_sum + acc_sum);
}

// Evaluate the row-j Phi formula (first-order, summed-Adams only):
//   Phi_{n+j} = h * (s_phi_at_j + sum_k kB[row][k] * dphi[k])
apsis::math::Mat6 eval_row_phi(std::size_t row, const apsis::math::Mat6& s_sum,
                               const std::array<apsis::math::Mat6, 9>& dphi, double h) {
  apsis::math::Mat6 acc_sum = apsis::math::Mat6::Zero();
  for (std::size_t k = 0; k < coeff::kStencil; ++k) {
    acc_sum += coeff::kB.at(row).at(k) * dphi.at(k);
  }
  return h * (s_sum + acc_sum);
}

// Compute the dPhi/dt = A(t) * Phi product given a force Jacobian and Phi.
apsis::math::Mat6 eval_dphi(apsis::time::Time<apsis::time::tags::TT> t,
                            const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                            const apsis::math::Mat6& phi,
                            const apsis::force::IForceModel& force) {
  const auto kJ36 = force.partials_dadx(t, x);
  return assemble_a(kJ36) * phi;
}

}  // namespace

void GaussJackson8::bootstrap(apsis::time::Time<apsis::time::tags::TT> t,
                              const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                              const apsis::math::Mat6& phi, double dt,
                              const apsis::force::IForceModel& force) {
  cached_dt_ = dt;
  center_t_ = t;
  starter_delivery_count_ = 0;

  const auto kStarter = detail::run_starter(t, x, phi, dt, force, opts_.mu_starter,
                                            opts_.phi_fd_pos_eps, opts_.phi_fd_vel_eps);
  for (std::size_t k = 0; k < coeff::kStencil; ++k) {
    accel_.at(k) = kStarter.accel.at(k);
    dphi_.at(k) = kStarter.dphi.at(k);
    state_.at(k) = kStarter.state.at(k);
  }
  // Phi at j=+1..+4: from the f-and-g central-difference STM cached in
  // the starter (used directly for the first 4 post-bootstrap step
  // returns). For j=-4..0 we don't need explicit phi_ slots — Phi
  // delivery in the first 4 calls only reads slots 5..8 (j=+1..+4),
  // and from call 5 onward the per-step PECE writes phi_[8] directly.
  // We seed slot 4 (j=0) with the caller's Phi for completeness.
  phi_.at(4) = phi;
  for (int j = 1; j <= 4; ++j) {
    const std::size_t kIdxJ{static_cast<std::size_t>(j) + 4U};
    phi_.at(kIdxJ) = kStarter.phi_forward.at(static_cast<std::size_t>(j - 1));
  }

  s_lead_ = kStarter.s_lead;
  S_lead_ = kStarter.S_lead;
  s_phi_lead_ = kStarter.s_phi_lead;

  initialised_ = true;
}

void GaussJackson8::advance_one_step(const apsis::force::IForceModel& force) {
  const double kH = cached_dt_;

  // 1. Compute the running sums valid at the new step's "current point"
  //    (paper-j=+5 of OLD stencil = paper-j=+4 of NEW stencil = t_n + 5*h).
  //    See ADR-009 Phase 1A Note D'' for the convention triage.
  const apsis::math::Vec3 kDdotRLead = accel_.at(8);
  const apsis::math::Mat6 kDphiLead = dphi_.at(8);
  const apsis::math::Vec3 kSPred = s_lead_ + 0.5 * kDdotRLead;
  const apsis::math::Vec3 kSNew = S_lead_ + s_lead_ + 0.5 * kDdotRLead;
  const apsis::math::Mat6 kSPhiPred = s_phi_lead_ + 0.5 * kDphiLead;

  // 2. Predict r, v, Phi at paper-j=+5 using row-0 of the table, the
  //    CURRENT (pre-shift) stencil's accelerations, and the predictor-
  //    side sums.
  const apsis::math::Vec3 kRPred =
      eval_row_position(coeff::kRowPredictor, kSNew, accel_, kH);
  const apsis::math::Vec3 kVPred = eval_row_velocity(coeff::kRowPredictor, kSPred, accel_, kH);
  const apsis::math::Mat6 kPhiPred = eval_row_phi(coeff::kRowPredictor, kSPhiPred, dphi_, kH);

  apsis::frames::State<apsis::frames::tags::ICRF> x_pred;
  x_pred.r = kRPred;
  x_pred.v = kVPred;

  // 3. Evaluate force model at the predicted state to obtain ddot_r and
  //    dPhi/dt at the new leading-edge epoch t_center + 5*h.
  const auto kTLead = center_t_ + apsis::time::Duration{5.0 * kH};
  const apsis::math::Vec3 kAPred = force.acceleration(kTLead, x_pred);
  const apsis::math::Mat6 kDphiPred = eval_dphi(kTLead, x_pred, kPhiPred, force);

  // 4. Shift ring buffers: drop slot 0, slide 1..8 → 0..7, place
  //    predicted point in slot 8 (NEW paper-j=+4).
  for (std::size_t i = 0; i < 8; ++i) {
    accel_.at(i) = accel_.at(i + 1);
    dphi_.at(i) = dphi_.at(i + 1);
    state_.at(i) = state_.at(i + 1);
    phi_.at(i) = phi_.at(i + 1);
  }
  accel_.at(8) = kAPred;
  dphi_.at(8) = kDphiPred;
  state_.at(8) = x_pred;
  phi_.at(8) = kPhiPred;

  // 5. Correct r, v, Phi at NEW paper-j=+4 using row-9 + the trapezoidal
  //    first sums + NEW stencil. The trapezoidal first sum needs the NEW
  //    leading-edge acceleration (post-shift slot 8 = a_pred), so we
  //    compute it AFTER the shift. Note kSNew is unchanged from the
  //    predictor — only the second-sum half-step recurrence is needed.
  const apsis::math::Vec3 kSCorr = s_lead_ + 0.5 * (kDdotRLead + accel_.at(8));
  const apsis::math::Mat6 kSPhiCorr = s_phi_lead_ + 0.5 * (kDphiLead + dphi_.at(8));
  const apsis::math::Vec3 kRCorr = eval_row_position(coeff::kRowCorrector, kSNew, accel_, kH);
  const apsis::math::Vec3 kVCorr = eval_row_velocity(coeff::kRowCorrector, kSCorr, accel_, kH);
  const apsis::math::Mat6 kPhiCorr = eval_row_phi(coeff::kRowCorrector, kSPhiCorr, dphi_, kH);

  apsis::frames::State<apsis::frames::tags::ICRF> x_corr;
  x_corr.r = kRCorr;
  x_corr.v = kVCorr;

  // 6. PECE: re-evaluate force at the corrected state and replace the
  //    leading-edge stencil entry. PEC (opts_.pece == false) skips this.
  if (opts_.pece) {
    const apsis::math::Vec3 kACorr = force.acceleration(kTLead, x_corr);
    const apsis::math::Mat6 kDphiCorr = eval_dphi(kTLead, x_corr, kPhiCorr, force);
    accel_.at(8) = kACorr;
    dphi_.at(8) = kDphiCorr;
  }

  // 7. Commit corrected state and update leading-edge sums.
  state_.at(8) = x_corr;
  phi_.at(8) = kPhiCorr;
  // Recompute trapezoidal s with the FINAL slot-8 acceleration (which
  // for PECE has just been re-evaluated). This is what the next step's
  // initial s_lead should be.
  s_lead_ = s_lead_ + 0.5 * (kDdotRLead + accel_.at(8));
  s_phi_lead_ = s_phi_lead_ + 0.5 * (kDphiLead + dphi_.at(8));
  S_lead_ = kSNew;
  center_t_ = center_t_ + apsis::time::Duration{kH};
}

StepResult GaussJackson8::step(apsis::time::Time<apsis::time::tags::TT> t,
                               const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                               const apsis::math::Mat6& phi, double dt,
                               const apsis::force::IForceModel& force) {
  // Continuity test: is this call a continuation of the previous one, or
  // a fresh segment? A fresh segment fires the f-and-g starter; a
  // continuation just advances the cached stencil.
  bool needs_bootstrap = !initialised_;
  if (initialised_) {
    if (std::abs(dt - cached_dt_) > 1e-12 * std::max(std::abs(dt), 1.0)) {
      // Step size changed — restart.
      needs_bootstrap = true;
    } else {
      const double kPosDiff = (x.r - last_returned_x_.r).norm();
      const double kVelDiff = (x.v - last_returned_x_.v).norm();
      const double kTDiff = std::abs((t - last_returned_t_).seconds());
      if (kPosDiff > opts_.continuity_pos_tol_m || kVelDiff > opts_.continuity_vel_tol_mps ||
          kTDiff > opts_.continuity_t_tol_s) {
        needs_bootstrap = true;
      }
    }
  }

  if (needs_bootstrap) {
    bootstrap(t, x, phi, dt, force);
  }

  // Deliver the next state. For the first 4 post-bootstrap calls, we
  // return state_[5..8] (the f-and-g forward points at paper-j = +1..+4)
  // directly. After 4 deliveries we advance via PECE.
  StepResult res;
  if (starter_delivery_count_ < 4) {
    const std::size_t kDeliverIdx{5U + static_cast<std::size_t>(starter_delivery_count_)};
    res.x = state_.at(kDeliverIdx);
    res.phi = phi_.at(kDeliverIdx);
    res.dt_actually_taken = dt;
    ++starter_delivery_count_;
  } else {
    advance_one_step(force);
    res.x = state_.at(8);
    res.phi = phi_.at(8);
    res.dt_actually_taken = dt;
  }

  // Cache for the next continuity check.
  last_returned_t_ = t + apsis::time::Duration{dt};
  last_returned_x_ = res.x;
  last_returned_phi_ = res.phi;

  return res;
}

}  // namespace apsis::integrate
