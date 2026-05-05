// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5: ThirdBody — gravitational pull of a third body on a spacecraft
// orbiting a central body, expressed in the ICRF (central-body-centred)
// frame.
//
// Per REQ-PHY-005, the formulation must be numerically stable at small
// spacecraft-central-body distances (the naive form  a = mu_3 * (r_3 - r) /
// |r_3 - r|^3 - mu_3 * r_3 / |r_3|^3  cancels). We use the f(q) trick
// (Battin §8.6.4):
//
//     a = -mu_3 / |s|^3 * (r + f(q) * (r - 2 r_3 . r / |r_3|^2 * r_3 / |r_3|^... ))
//
// More specifically:
//
//   d   = r - r_3                  (spacecraft-to-third-body vector, sign convention)
//   q   = (r . (r - 2 r_3)) / |r_3|^2
//   f_q = q (3 + 3 q + q^2) / (1 + (1 + q)^{3/2})
//   a   = -(mu_3 / |r_3|^3) * (r + f_q * r_3)
//
// (Equivalent to Battin's formulation; numerically stable when r << r_3.)

#pragma once

#include "apsis/ephemeris/iephemeris.h"
#include "apsis/force/iforce_model.h"

namespace apsis::force {

class ThirdBody final : public IForceModel {
 public:
  // `central_body_naif_id` is the body the spacecraft is orbiting (e.g. 399
  // = Earth); `third_body_naif_id` is the perturber (e.g. 10 = Sun, 301 =
  // Moon). `mu_third` is the perturber's GM in m^3/s^2. The IEphemeris
  // pointer must outlive this object (no ownership transferred).
  ThirdBody(const apsis::ephemeris::IEphemeris* ephem,
            int central_body_naif_id,
            int third_body_naif_id,
            double mu_third) noexcept
      : ephem_(ephem),
        central_body_naif_id_(central_body_naif_id),
        third_body_naif_id_(third_body_naif_id),
        mu_third_(mu_third) {}

  apsis::math::Vec3 acceleration(
      apsis::time::Time<apsis::time::tags::TT> t,
      const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

  apsis::math::Mat36 partials(
      apsis::time::Time<apsis::time::tags::TT> t,
      const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

 private:
  // Internal: query the third body's position relative to the central body
  // at TT epoch t (converts TT->TDB internally).
  apsis::math::Vec3 third_body_pos_central(
      apsis::time::Time<apsis::time::tags::TT> t) const;

  const apsis::ephemeris::IEphemeris* ephem_;
  int central_body_naif_id_;
  int third_body_naif_id_;
  double mu_third_;
};

}  // namespace apsis::force
