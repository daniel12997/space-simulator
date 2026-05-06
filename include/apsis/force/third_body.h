// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5: ThirdBody — gravitational pull of a third body on a spacecraft
// orbiting a central body, expressed in the ICRF (central-body-centred)
// frame.
//
// The Phase 1 form is the conventional Vallado §8.7.2 / Montenbruck-Gill
// §3.3.2 expression:
//
//   a(r) = mu_3 * [ (r_3 - r) / |r_3 - r|^3  -  r_3 / |r_3|^3 ]
//
// The expression has known cancellation when |r| << |r_3| — at LEO
// against the Sun the loss is ~5 of ~16 double-precision sig figs, which
// remains far above the adapter's declared 1e-6 conformance tolerance.
// The Battin / f(q) stable substitution is a Phase 7 hardening item if a
// future caller (e.g. close-approach conjunction screening at sub-pc
// distances) needs it; the analytical Jacobian below is unchanged at
// that upgrade because both forms describe the same vector field.
//
// (REQ-PHY-005 originally called for the Battin form here; the Phase 1
// implementation downgrades to the conventional form so the analytical
// Jacobian for ADR-009 lands cleanly. The REQ stays satisfied at the
// Phase 7 upgrade.)

#pragma once

#include "apsis/ephemeris/iephemeris.h"
#include "apsis/force/iforce_model.h"

namespace apsis::force {

class ThirdBody final : public IForceModel {
 public:
  // ADR-009 conformance flag: partials_dadx() below is analytical (the
  // closed-form ∂a/∂r of the conventional third-body acceleration; the
  // indirect term is constant in r and drops out). The VE-contract
  // conformance test parameterises over adapters with
  // `kAnalyticalPartials == true`.
  static constexpr bool kAnalyticalPartials = true;

  // `central_body_naif_id` is the body the spacecraft is orbiting (e.g. 399
  // = Earth); `third_body_naif_id` is the perturber (e.g. 10 = Sun, 301 =
  // Moon). `mu_third` is the perturber's GM in m^3/s^2. The IEphemeris
  // pointer must outlive this object (no ownership transferred).
  ThirdBody(const apsis::ephemeris::IEphemeris* ephem, int central_body_naif_id,
            int third_body_naif_id, double mu_third) noexcept
      : ephem_(ephem), central_body_naif_id_(central_body_naif_id),
        third_body_naif_id_(third_body_naif_id), mu_third_(mu_third) {}

  [[nodiscard]] apsis::math::Vec3
  acceleration(apsis::time::Time<apsis::time::tags::TT> t,
               const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

  [[nodiscard]] apsis::math::Mat36
  partials_dadx(apsis::time::Time<apsis::time::tags::TT> t,
                const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

 private:
  // Internal: query the third body's position relative to the central body
  // at TT epoch t (converts TT->TDB internally).
  [[nodiscard]] apsis::math::Vec3
  third_body_pos_central(apsis::time::Time<apsis::time::tags::TT> t) const;

  const apsis::ephemeris::IEphemeris* ephem_;
  int central_body_naif_id_;
  int third_body_naif_id_;
  double mu_third_;
};

}  // namespace apsis::force
