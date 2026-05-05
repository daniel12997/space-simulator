// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5: PointMass — a = -mu * r / |r|^3, with closed-form 3x3 partial
// ∂a/∂r = -mu/|r|^5 * (|r|^2 * I - 3 * r * r^T). No velocity dependence.

#pragma once

#include "apsis/force/iforce_model.h"

namespace apsis::force {

class PointMass final : public IForceModel {
 public:
  // mu in SI: m^3 / s^2.
  explicit PointMass(double mu) noexcept : mu_(mu) {}

  apsis::math::Vec3 acceleration(
      apsis::time::Time<apsis::time::tags::TT> t,
      const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

  apsis::math::Mat36 partials(
      apsis::time::Time<apsis::time::tags::TT> t,
      const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

  [[nodiscard]] double mu() const noexcept { return mu_; }

 private:
  double mu_;
};

}  // namespace apsis::force
