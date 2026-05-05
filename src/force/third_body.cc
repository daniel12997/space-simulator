// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5: ThirdBody force-model adapter. See header for math notation.

#include "apsis/force/third_body.h"

#include <cmath>

#include "apsis/time/convert.h"

namespace apsis::force {
namespace {

// f(q) = q * (3 + 3q + q^2) / (1 + (1 + q)^(3/2)). Numerically stable for
// q -> 0 because the numerator and denominator both vanish; the algebraic
// form above is the standard Battin substitution that exposes the smooth
// limit.
double f_q(double q) {
  const double one_plus_q = 1.0 + q;
  // pow with 1.5 is well-conditioned; switch to series for very small q
  // would marginally reduce noise but is not necessary at our tolerances.
  const double denom = 1.0 + std::pow(one_plus_q, 1.5);
  return q * (3.0 + 3.0 * q + q * q) / denom;
}

}  // namespace

apsis::math::Vec3 ThirdBody::third_body_pos_central(
    apsis::time::Time<apsis::time::tags::TT> t) const {
  const auto t_tdb = apsis::time::convert<apsis::time::tags::TDB>(t);
  const auto third_state    = ephem_->state(third_body_naif_id_, t_tdb);
  const auto central_state  = ephem_->state(central_body_naif_id_, t_tdb);
  return third_state.r - central_state.r;
}

apsis::math::Vec3 ThirdBody::acceleration(
    apsis::time::Time<apsis::time::tags::TT> t,
    const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  const auto r3 = third_body_pos_central(t);
  const double r3_sq = r3.squaredNorm();
  const double r3_norm = std::sqrt(r3_sq);
  const double r3_cubed = r3_sq * r3_norm;

  // q = r . (r - 2 r_3) / |r_3|^2 (Battin's stable scalar).
  const double q = x.r.dot(x.r - 2.0 * r3) / r3_sq;
  const double f = f_q(q);

  // a = -(mu_3 / |r_3|^3) * (r + f * (r - r_3))   [Battin form]
  // Note: literature varies in sign convention; the form below matches
  // Vallado §8.7.2 / Montenbruck-Gill §3.3.2 once expanded.
  return -(mu_third_ / r3_cubed) * (x.r + f * (x.r - r3));
}

apsis::math::Mat36 ThirdBody::partials(
    apsis::time::Time<apsis::time::tags::TT> t,
    const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  // Per the plan: third_body derivatives treat the third-body position as
  // frozen at evaluation time (the effective acceleration depends only on
  // the active spacecraft's position). Closed-form is straightforward but
  // we reuse the central-difference pattern for consistency with the SH
  // adapter; h = 1 m gives ~1e-13 truncation error at LEO which is well
  // inside the 1e-6 conformance tolerance for this adapter.
  apsis::math::Mat36 J = apsis::math::Mat36::Zero();
  constexpr double h = 1.0;
  for (int i = 0; i < 3; ++i) {
    auto x_plus = x;
    auto x_minus = x;
    x_plus.r[i]  += h;
    x_minus.r[i] -= h;
    const auto a_plus  = acceleration(t, x_plus);
    const auto a_minus = acceleration(t, x_minus);
    J.col(i) = (a_plus - a_minus) / (2.0 * h);
  }
  return J;
}

}  // namespace apsis::force
