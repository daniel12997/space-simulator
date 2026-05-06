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
  const double kR = x.r.norm();
  const double kR3 = kR * kR * kR;
  return -(mu_ / kR3) * x.r;
}

apsis::math::Mat36
PointMass::partials(apsis::time::Time<apsis::time::tags::TT>,
                    const apsis::frames::State<apsis::frames::tags::ICRF>& x) const {
  const double kR2 = x.r.squaredNorm();
  const double kR = std::sqrt(kR2);
  const double kR5 = kR2 * kR2 * kR;
  apsis::math::Mat36 jac = apsis::math::Mat36::Zero();
  // ∂a/∂r = -mu/|r|^5 * (|r|^2 * I - 3 * r * r^T)
  apsis::math::Mat3 dadr = apsis::math::Mat3::Zero();
  dadr = -mu_ / kR5 * (kR2 * apsis::math::Mat3::Identity() - 3.0 * x.r * x.r.transpose());
  jac.block<3, 3>(0, 0) = dadr;
  // No velocity dependence: cols 3..5 stay zero.
  return jac;
}

}  // namespace apsis::force
