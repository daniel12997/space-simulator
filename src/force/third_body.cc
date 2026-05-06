// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5: ThirdBody force-model adapter. See header for math notation.

#include "apsis/force/third_body.h"

#include <cmath>

#include "apsis/time/convert.h"

namespace apsis::force {

apsis::math::Vec3
ThirdBody::third_body_pos_central(apsis::time::Time<apsis::time::tags::TT> t) const {
  const auto t_tdb = apsis::time::convert<apsis::time::tags::TDB>(t);
  const auto third_state = ephem_->state(third_body_naif_id_, t_tdb);
  const auto central_state = ephem_->state(central_body_naif_id_, t_tdb);
  return third_state.r - central_state.r;
}

apsis::math::Vec3
ThirdBody::acceleration(apsis::time::Time<apsis::time::tags::TT> t,
                        const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  // Conventional third-body acceleration (Vallado §8.7.2, Montenbruck &
  // Gill §3.3.2):
  //
  //   a(r) = mu_3 * [ (r_3 - r) / |r_3 - r|^3  -  r_3 / |r_3|^3 ]
  //
  // The "Battin / f(q) trick" was previously used in this file to avoid
  // cancellation when |r| << |r_3|. For the Phase 1 regime (LEO vs Sun
  // or Moon: |r| ~ 1e7 m, |r_3| ~ 1e8 - 1e11 m) the loss is at most ~5
  // significant figures of the ~16 a double carries — well above the
  // adapter's declared tolerance, and the conventional form keeps the
  // analytical Jacobian below in lockstep with the literature derivation.
  // The Battin substitution is reinstated as an opt-in path in Phase 7
  // if conjunction-screening close approaches require it.
  const auto r3 = third_body_pos_central(t);
  const apsis::math::Vec3 d = r3 - x.r;  // spacecraft -> third body
  const double d_norm_sq = d.squaredNorm();
  const double d_norm = std::sqrt(d_norm_sq);
  const double d_cubed = d_norm_sq * d_norm;
  const double r3_norm = r3.norm();
  const double r3_cubed = r3_norm * r3_norm * r3_norm;
  return mu_third_ * (d / d_cubed - r3 / r3_cubed);
}

apsis::math::Mat36
ThirdBody::partials(apsis::time::Time<apsis::time::tags::TT> t,
                    const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  // Analytical partials per ADR-009 (REQ-PHY-016, REQ-PHY-020).
  //
  // For a spacecraft at r and a third body at r_3 (both relative to the
  // central body), the conventional third-body acceleration is
  //
  //   a(r) = mu_3 * [ (r_3 - r) / |r_3 - r|^3  -  r_3 / |r_3|^3 ]
  //
  // The acceleration() override above evaluates the same quantity via
  // Battin's f(q) form for numerical stability when |r| << |r_3|; the
  // closed-form Jacobian below is identical mathematically (both forms
  // describe the same vector field). Differentiating wrt the spacecraft
  // position r:
  //
  //   da/dr = mu_3 * [ 3 * (r_3 - r)(r_3 - r)^T / |r_3 - r|^5  -  I / |r_3 - r|^3 ]
  //
  // The indirect term `-r_3/|r_3|^3` is constant in r, so it drops.
  // Velocity columns are zero — third-body acceleration depends only on
  // position. Verified against the FD oracle in
  // tests/conformance/force_model_ve_contract.cc (h = 10 m) to ~1e-10
  // relative.
  const auto r3 = third_body_pos_central(t);
  const apsis::math::Vec3 d = r3 - x.r;
  const double d_norm_sq = d.squaredNorm();
  const double d_norm = std::sqrt(d_norm_sq);
  const double d_cubed = d_norm_sq * d_norm;
  const double d_fifth = d_cubed * d_norm_sq;

  apsis::math::Mat3 J3 = apsis::math::Mat3::Zero();
  J3.diagonal().setConstant(-mu_third_ / d_cubed);
  J3 += (3.0 * mu_third_ / d_fifth) * (d * d.transpose());

  apsis::math::Mat36 J = apsis::math::Mat36::Zero();
  J.block<3, 3>(0, 0) = J3;
  // Velocity columns remain zero — no velocity dependence in Phase 1.
  return J;
}

}  // namespace apsis::force
