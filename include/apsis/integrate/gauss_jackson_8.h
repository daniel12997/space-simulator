// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1A §D'' (re-attempt of D2): `GaussJackson8` — Berry-Healy 2004
// ordinate-form summed-Adams / summed-Cowell predictor-corrector multi-step
// integrator. Eighth order. Fixed-step. PECE
// (predictor-evaluate-corrector-evaluate) is the production default; PEC
// (predictor-evaluate-corrector) is exposed as an Options flag.
//
// Coefficient source: docs/raw/code/berry-healy-2004-gj8-generator/
// (Healy's UMD DRUM code, Python-ported with corrected
// half_acceleration_in_sum). See src/integrate/gj8_coeffs.h.
//
// State convention: ICRF / SI / two-component TT, matching the IIntegrator
// seam contract documented in `iintegrator.h`. Phi is co-integrated as
// dPhi/dt = A(t) * Phi using the same multi-step pattern applied
// per-column to the linear ODE — i.e. each column of Phi has its own
// 9-point dPhi/dt stencil and is advanced by the same predictor /
// corrector formulas as the natural state.
//
// Multi-step state cache: GJ8 cannot be made stateless because each step
// depends on the previous N+1 acceleration evaluations. The implementation
// caches an internal 9-point ring buffer of accelerations plus the running
// first/second sums; on each `step()` call it tests for continuity from
// the previous output (matching `t` and `x`) and either continues the
// stencil or rebootstraps via the f-and-g starter (Berry-Healy §5.1).
//
// Phase 1A scope notes:
// - The starter assumes a Kepler-only initial neighbourhood: the f-and-g
//   series propagates the 8 back-points around the seed epoch, then the
//   force model is sampled at all 9 stencil epochs. For perturbed
//   dynamics the starter is acceptable as-is on the < 1 hour conformance
//   horizons because the Kepler approximation drift is below the 1e-5 m
//   conformance tolerance over half a stencil span.
// - Phi at the starter is initialised from a central-difference oracle
//   (perturb each axis of (r, v) by `kPhiFdEps` of its scale, propagate
//   each via f-and-g, divide by perturbation). The plan permits this as
//   a Phase 1A acceptable approximation; a closed-form universal-variable
//   Kepler STM is the Phase 7 upgrade.

#pragma once

#include <array>

#include "apsis/integrate/iintegrator.h"

namespace apsis::integrate {

class GaussJackson8 final : public IIntegrator {
 public:
  struct Options {
    // PECE (default) re-evaluates the force model at the corrected state
    // and stores that as the stencil's leading-edge acceleration. PEC
    // skips the second evaluation and stores the predictor-evaluation
    // acceleration. PECE costs one extra force evaluation per step in
    // exchange for substantially improved per-step accuracy on perturbed
    // dynamics; the Phase 1A conformance tolerance (< 1e-5 m / period)
    // is met by both modes on Kepler.
    bool pece = true;

    // Central-body gravitational parameter for the f-and-g starter (m^3
    // / s^2). The starter propagates 8 back-points via Kepler-only
    // motion before the full force model takes over; the starter does
    // not interrogate the IForceModel for `mu` (the seam is intentionally
    // mu-agnostic). Default is Earth's mu (EGM2008-consistent).
    double mu_starter = 3.986004418e14;

    // Central-difference Phi-at-starter perturbation scales (m and m/s).
    // Larger values reduce numerical noise but increase truncation; the
    // defaults are tuned for LEO-scale dynamics.
    double phi_fd_pos_eps = 1.0;     // m
    double phi_fd_vel_eps = 1.0e-3;  // m/s

    // Continuity tolerance: a `step()` call is treated as a continuation
    // of the cached stencil if `||x.r - cached_x.r|| < kContinuityRTol *
    // kRadius` and the analogous velocity check holds AND `|t - cached_t|
    // < kContinuityTTol`. Otherwise the stencil rebootstraps. Loose
    // defaults — a typical continuation has bit-exact agreement.
    double continuity_pos_tol_m = 1e-3;
    double continuity_vel_tol_mps = 1e-6;
    double continuity_t_tol_s = 1e-6;
  };

  GaussJackson8() noexcept = default;
  explicit GaussJackson8(Options opts) noexcept : opts_(opts) {}

  StepResult step(apsis::time::Time<apsis::time::tags::TT> t,
                  const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                  const apsis::math::Mat6& phi, double dt,
                  const apsis::force::IForceModel& force) override;

  [[nodiscard]] const Options& options() const noexcept { return opts_; }

  // Accessor for tests: returns true iff the next step() call would be
  // treated as a stencil continuation (vs a fresh bootstrap).
  [[nodiscard]] bool is_initialised_for_test() const noexcept {
    return initialised_ && cached_dt_ > 0.0;
  }

 private:
  // Bootstrap the 9-point stencil around a seed epoch / state. Populates
  // ring buffer, sigma_, Sigma_, and first 4 forward-cached deliveries.
  void bootstrap(apsis::time::Time<apsis::time::tags::TT> t,
                 const apsis::frames::State<apsis::frames::tags::ICRF>& x,
                 const apsis::math::Mat6& phi, double dt, const apsis::force::IForceModel& force);

  // Advance the stencil by one step using PECE (or PEC if opts_.pece is
  // false). Updates the ring buffer, sums, and cached state.
  void advance_one_step(const apsis::force::IForceModel& force);

  // Configuration.
  Options opts_;

  // Stencil cache.
  bool initialised_ = false;
  double cached_dt_ = 0.0;

  // Stencil ring buffers. Index 0..8 corresponds to paper-j = -4..+4.
  // Center (paper-j=0) is index 4; leading edge (paper-j=+4) is index 8.
  // We physically shift the buffers each step rather than rotating an
  // index, since the small fixed size (9 entries each) makes the shift
  // cheap and the indexing simple.
  std::array<apsis::math::Vec3, 9> accel_{};
  std::array<apsis::math::Mat6, 9> dphi_{};
  std::array<apsis::frames::State<apsis::frames::tags::ICRF>, 9> state_{};
  std::array<apsis::math::Mat6, 9> phi_{};

  // First and second sums maintained at the LEADING EDGE of the stencil
  // (paper-j=+4, ring-buffer index 8). The row-j formulas in the
  // ordinate-form post-half-acceleration convention require the running
  // sums to be those AT THE STENCIL POSITION OF THE STATE BEING COMPUTED.
  // For the predictor (paper-j=+5) and the post-shift corrector (paper-j=+4
  // of new stencil, i.e. old paper-j=+5), we therefore roll the leading-
  // edge sums forward by one step using the half-step recurrence:
  //
  //   s_{m+1} = s_m + ddot_r_m
  //   S_{m+1} = S_m + s_m + (1/2) * ddot_r_m
  //
  // and pass S_{m+1}, s_{m+1} into both predictor and corrector formulas.
  // After PECE acceptance the rolled-forward sums become the new leading-
  // edge sums.
  //
  // Phi is first-order (dPhi/dt = A * Phi); only the summed-Adams (kB)
  // table applies and only the first sum is needed. The recurrence is
  // simpler: s_phi_{m+1} = s_phi_m + dphi_m (no half-step shift, since
  // the second sum is not used).
  apsis::math::Vec3 s_lead_ = apsis::math::Vec3::Zero();
  apsis::math::Vec3 S_lead_ = apsis::math::Vec3::Zero();
  apsis::math::Mat6 s_phi_lead_ = apsis::math::Mat6::Zero();

  // Continuity-detection cache: the (t, x) we returned on the last
  // step(). The next call is treated as a continuation iff its inputs
  // match this within the Options tolerances. Otherwise the stencil
  // rebootstraps from the input (t, x).
  apsis::time::Time<apsis::time::tags::TT> last_returned_t_{2451545.0, 0.0};
  apsis::frames::State<apsis::frames::tags::ICRF> last_returned_x_{};
  apsis::math::Mat6 last_returned_phi_ = apsis::math::Mat6::Identity();

  // Time at the stencil's center (paper-j=0, index 4 in the buffers).
  apsis::time::Time<apsis::time::tags::TT> center_t_{2451545.0, 0.0};

  // After a bootstrap, the seed state corresponds to the stencil's
  // center. The first 4 step() calls return state_[5..8] (the f-and-g
  // forward points); from call 5 onward we advance via PECE. This
  // counter tracks how many of the 4 starter forward points have been
  // delivered.
  int starter_delivery_count_ = 0;
};

}  // namespace apsis::integrate
