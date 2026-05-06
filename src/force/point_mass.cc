// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5: PointMass force model.

#include "apsis/force/point_mass.h"

#include <cmath>

namespace apsis::force {

apsis::math::Vec3
PointMass::acceleration(apsis::time::Time<apsis::time::tags::TT>,
                        const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  const double r = x.r.norm();
  const double r3 = r * r * r;
  return -(mu_ / r3) * x.r;
}

apsis::math::Mat36
PointMass::partials(apsis::time::Time<apsis::time::tags::TT>,
                    const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  const double r2 = x.r.squaredNorm();
  const double r = std::sqrt(r2);
  const double r5 = r2 * r2 * r;
  apsis::math::Mat36 J = apsis::math::Mat36::Zero();
  // ∂a/∂r = -mu/|r|^5 * (|r|^2 * I - 3 * r * r^T)
  apsis::math::Mat3 dadr = apsis::math::Mat3::Zero();
  dadr = -mu_ / r5 * (r2 * apsis::math::Mat3::Identity() - 3.0 * x.r * x.r.transpose());
  J.block<3, 3>(0, 0) = dadr;
  // No velocity dependence: cols 3..5 stay zero.
  return J;
}

}  // namespace apsis::force
