// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5 / Phase-1A §C: SphericalHarmonic — Cunningham non-singular
// recursion of the gravitational acceleration up to user-selectable
// degree-and-order (Phase 1 ships with EGM2008 truncated to deg 20).
//
// Coefficients are passed in normalised form (the same convention as
// IERS / EGM2008 distribution): the normalised Stokes coefficients C_{n,m}
// and S_{n,m} (held in members `c_norm` / `s_norm`) live in row-major
// triangular storage with index `idx(n, m) = n * (n + 1) / 2 + m` for
// 0 <= m <= n.
//
// Acceleration is computed analytically via Cunningham's V/W functions
// (Cunningham 1970; Vallado §8.6). Partials are also analytical: the
// Phase-1A §C2 upgrade implements the Cunningham gradient (∂a/∂r as a
// Hessian of the potential, evaluated through the same V/W table extended
// to (N+2) rows). The implementation is cross-attributed to Orekit's
// `HolmesFeatherstoneAttractionModel.java` (Apache-2.0); see
// [[wiki/sources/orekit-holmes-featherstone-impl]] and the source comment
// in `src/force/spherical_harmonic.cc`.
//
// Phase-1A §C1: this adapter operates in the body's body-fixed frame
// (Earth-fixed for EGM2008). The constructor requires a `const EopTable&`;
// `acceleration()` and `partials_dadx()` use the EOP-aware ICRF<->ITRS
// rotation to map ICRF input -> body-fixed, evaluate Cunningham V/W in
// body-fixed, then rotate the acceleration (and conjugate the Jacobian)
// back to ICRF. The Phase 1 "treat body-fixed and ICRF as aligned"
// shortcut is removed.

#pragma once

#include <cstddef>
#include <vector>

#include "apsis/force/iforce_model.h"

namespace apsis::time {
class EopTable;  // fwd-decl; full definition in apsis/time/eop_table.h
}  // namespace apsis::time

namespace apsis::force {

class SphericalHarmonic final : public IForceModel {
 public:
  // ADR-009 conformance flag: partials_dadx() is now the analytical Cunningham
  // gradient (Phase-1A §C2). Re-included in the VE-contract conformance gate.
  static constexpr bool kAnalyticalPartials = true;

  // Coefficient block. Phase 1 holds normalised C/S directly; the
  // un-normalising recurrence is built into the Cunningham recursion in
  // the .cc file.
  struct Coefficients {
    int degree = 0;      // n_max
    int order = 0;       // m_max  (m_max <= n_max)
    double mu = 0.0;     // central-body GM [m^3/s^2]
    double r_ref = 0.0;  // reference radius (R_E in geodesy literature, e.g.
                         // Earth's equatorial radius) [m]
    // Triangular storage: size = (degree + 1) * (degree + 2) / 2.
    // c_norm[idx(n, m)] = C_{n,m} normalised (Geodesy convention).
    // s_norm[idx(n, m)] = S_{n,m} normalised (Geodesy convention).
    std::vector<double> c_norm;
    std::vector<double> s_norm;

    [[nodiscard]] static std::size_t idx(int n, int m) noexcept {
      // Widen both operands before multiplication to avoid clang-tidy
      // bugprone-misplaced-widening-cast and -Wsign-conversion (and any
      // genuine overflow risk if degree ever exceeds ~46k).
      return static_cast<std::size_t>(n) * static_cast<std::size_t>(n + 1) / 2 +
             static_cast<std::size_t>(m);
    }
    [[nodiscard]] int triangular_size() const noexcept { return (degree + 1) * (degree + 2) / 2; }
  };

  // Phase-1A §C1: the constructor takes a non-owning reference to an
  // `EopTable`; `acceleration()` / `partials_dadx()` query it on every call
  // to obtain the ICRF -> ITRS rotation at the input epoch. The table must
  // outlive the adapter. Held by pointer internally so the class stays
  // assignable (IForceModel is non-copyable so the practical effect is
  // identical to a reference member, but the pointer form documents the
  // non-owning relationship and avoids the rebinding gotcha).
  SphericalHarmonic(Coefficients coeffs, const apsis::time::EopTable& eop);

  [[nodiscard]] apsis::math::Vec3
  acceleration(apsis::time::Time<apsis::time::tags::TT> t,
               const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

  [[nodiscard]] apsis::math::Mat36
  partials_dadx(apsis::time::Time<apsis::time::tags::TT> t,
                const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

  [[nodiscard]] const Coefficients& coefficients() const noexcept { return coeffs_; }

 private:
  // Internal: acceleration in body-fixed frame given a body-fixed position.
  [[nodiscard]] apsis::math::Vec3 acceleration_body(const apsis::math::Vec3& r_bf) const;

  // Internal: position-Jacobian (∂a_bf/∂r_bf) in body-fixed frame,
  // evaluated analytically via the Cunningham gradient (V/W to N+2 rows).
  // See `src/force/spherical_harmonic.cc` for the formulation and
  // attribution.
  [[nodiscard]] apsis::math::Mat3 partials_body(const apsis::math::Vec3& r_bf) const;

  Coefficients coeffs_;
  const apsis::time::EopTable* eop_;
};

}  // namespace apsis::force
