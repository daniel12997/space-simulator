// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// Phase-1 §5: SphericalHarmonic — Cunningham non-singular recursion of the
// gravitational acceleration up to user-selectable degree-and-order
// (Phase 1 ships with EGM2008 truncated to deg 20).
//
// Coefficients are passed in normalised form (the same convention as
// IERS / EGM2008 distribution): the normalised Stokes coefficients C_{n,m}
// and S_{n,m} (held in members `c_norm` / `s_norm`) live in row-major
// triangular storage with index `idx(n, m) = n * (n + 1) / 2 + m` for
// 0 <= m <= n.
//
// Acceleration is computed analytically via Cunningham's V/W functions
// (Cunningham 1970; Vallado §8.6). Partials are computed by central
// difference of the analytical acceleration with h = 1.0 m — a Phase 1
// trade pending the Phase 7 analytical Pines-gradient implementation.
// Because both sides of the VE-contract conformance test would be FD
// evaluations of the same gradient (a tautology), this adapter declares
// `kAnalyticalPartials = false` and is **excluded** from that gate.
// ADR-009 Phase 1 Implementation Note documents the deferral; the Phase
// 7 follow-up issue is "Pines analytical gradient for SphericalHarmonic;
// re-include in VE-contract conformance test".

#pragma once

#include <cstddef>
#include <vector>

#include "apsis/force/iforce_model.h"

namespace apsis::force {

class SphericalHarmonic final : public IForceModel {
 public:
  // ADR-009 conformance flag: partials() below is **finite-difference**
  // pending the Phase 7 Pines analytical-gradient implementation. The
  // VE-contract conformance test (which compares analytical partials
  // against an independent FD oracle) is **not meaningful** on this
  // adapter while both sides are FD evaluations of the same gradient,
  // so it is excluded from the gate. See ADR-009 Phase 1 Implementation
  // Note. (Internal sanity is still covered by
  // tests/unit/force/spherical_harmonic_test.cc.)
  static constexpr bool kAnalyticalPartials = false;

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

  // The body-fixed-to-inertial rotation is applied externally (the SH model
  // operates in body-fixed coordinates — Earth-fixed for EGM2008 — and the
  // caller composes with an ITRS<->ICRF transform at the seam). For Phase 1,
  // we accept an inertial state but assume the caller uses this adapter only
  // when the body-fixed rotation can be accounted for. To keep the unit
  // tests honest and the conformance gate exercising the full pipeline, this
  // adapter optionally takes an inertial-to-body rotation provider; when
  // absent, body-fixed and inertial are treated as aligned (suitable for
  // the conformance test grid which uses contrived synthetic states).
  explicit SphericalHarmonic(Coefficients coeffs);

  [[nodiscard]] apsis::math::Vec3
  acceleration(apsis::time::Time<apsis::time::tags::TT> t,
               const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

  [[nodiscard]] apsis::math::Mat36
  partials(apsis::time::Time<apsis::time::tags::TT> t,
           const apsis::frames::State<apsis::frames::tags::ICRF>& x) const override;

  [[nodiscard]] const Coefficients& coefficients() const noexcept { return coeffs_; }

 private:
  // Internal: acceleration in body-fixed frame given a body-fixed position.
  // Phase 1 treats body-fixed and ICRF as aligned (see class comment).
  [[nodiscard]] apsis::math::Vec3 acceleration_body(const apsis::math::Vec3& r_bf) const;

  Coefficients coeffs_;
};

}  // namespace apsis::force
