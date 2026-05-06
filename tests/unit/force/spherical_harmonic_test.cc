// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-PHY-003, REQ-PHY-004, REQ-PHY-016
//
// Phase-1 §5 / Phase-1A §C: SphericalHarmonic adapter unit test.
// Verifies:
//   * The deg=0/order=0 limit (only C_{0,0} = 1) reproduces point-mass
//     acceleration to machine epsilon (rotation is irrelevant for the
//     central term).
//   * The J2-only case (C_{2,0} = -J2 normalised) gives a J2 acceleration
//     of the right sign and order of magnitude on a polar position.
//   * The analytical position-Jacobian agrees with a central-difference
//     oracle (Phase-1A §C2).
//   * Phase-1A §C1 rotation observable: a tesseral C_{2,2}-only model
//     produces accelerations whose direction tracks the body-fixed
//     equator bulge — at a fixed inertial point, the magnitude / azimuth
//     of the perturbation differs at two epochs separated by ~6 h
//     because the Earth-fixed bulge has rotated underneath. (A
//     non-rotating implementation would give the same vector at both
//     epochs, so this test fails on the Phase 1 "treat body-fixed and
//     ICRF as aligned" implementation.)

#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "apsis/force/spherical_harmonic.h"
#include "apsis/time/eop_table.h"

namespace af = apsis::force;
namespace at = apsis::time;
namespace afr = apsis::frames;

namespace {

constexpr double kMuEarth = 3.986004418e14;
constexpr double kReEarth = 6378136.3;         // m
constexpr double kJ2 = 1.0826266835531513e-3;  // EGM2008 zonal J2

// Flat (zero-EOP, constant-everything) table covering Phase 1 epochs;
// dut1 = 0, polar motion = 0. Two rows is the EopTable invariant minimum.
at::EopTable make_flat_eop() {
  std::vector<at::EopRow> rows{
      {/*mjd_utc=*/51000.0, 0.0, 0.0, 0.0},
      {/*mjd_utc=*/60000.0, 0.0, 0.0, 0.0},
  };
  return at::EopTable(std::move(rows));
}

af::SphericalHarmonic::Coefficients make_point_mass() {
  af::SphericalHarmonic::Coefficients c;
  c.degree = 0;
  c.order = 0;
  c.mu = kMuEarth;
  c.r_ref = kReEarth;
  // (degree+1)(degree+2)/2 = 1 entry
  c.c_norm = {1.0};
  c.s_norm = {0.0};
  return c;
}

af::SphericalHarmonic::Coefficients make_j2_only() {
  af::SphericalHarmonic::Coefficients c;
  c.degree = 2;
  c.order = 0;
  c.mu = kMuEarth;
  c.r_ref = kReEarth;
  // Triangular size = 6 entries: (0,0), (1,0), (1,1), (2,0), (2,1), (2,2)
  c.c_norm.assign(6, 0.0);
  c.s_norm.assign(6, 0.0);
  c.c_norm[0] = 1.0;  // C_{0,0}
  // C_{2,0} normalised: -J2 / sqrt(5).
  c.c_norm[3] = -kJ2 / std::sqrt(5.0);
  return c;
}

// Tesseral C_{2,2}-only coefficient block (no central term, no zonal). A
// pure C_{2,2} term has the longitude dependence cos(2 lambda) — the
// geopotential's "equator-bulge" mode. We use a contrived large value
// (1e-3) to make the perturbation easy to observe at 16-digit precision.
af::SphericalHarmonic::Coefficients make_c22_only(double c22_normalised) {
  af::SphericalHarmonic::Coefficients c;
  c.degree = 2;
  c.order = 2;
  c.mu = kMuEarth;
  c.r_ref = kReEarth;
  c.c_norm.assign(6, 0.0);
  c.s_norm.assign(6, 0.0);
  // C_{2,2} index is (n=2, m=2): idx = 2*3/2 + 2 = 5.
  c.c_norm[5] = c22_normalised;
  return c;
}

TEST(SphericalHarmonic, DegreeZeroEqualsPointMass) {
  const auto eop = make_flat_eop();
  af::SphericalHarmonic sh(make_point_mass(), eop);
  afr::State<afr::tags::ICRF> x;
  x.r << 7.0e6, 1.0e6, 5.0e5;
  const auto a = sh.acceleration(at::Time<at::tags::TT>{2451545.0, 0.0}, x);
  // Reference: point-mass. Central-term (C_{0,0}) is rotation-invariant.
  const double r = x.r.norm();
  const auto a_ref = -(kMuEarth / (r * r * r)) * x.r;
  EXPECT_NEAR((a - a_ref).norm() / a_ref.norm(), 0.0, 1e-12);
}

TEST(SphericalHarmonic, J2OnlyAtPole) {
  const auto eop = make_flat_eop();
  af::SphericalHarmonic sh(make_j2_only(), eop);
  // Polar position (in ICRF). The Earth's polar axis is approximately
  // aligned with ICRF +z to within the precession-nutation small angle, so
  // a polar ICRF position is also a polar body-fixed position to working
  // precision; the J2 perturbation magnitude check below is robust to this.
  afr::State<afr::tags::ICRF> x;
  x.r << 0.0, 0.0, kReEarth + 400e3;
  const auto t = at::Time<at::tags::TT>{2451545.0, 0.0};
  const auto a = sh.acceleration(t, x);
  af::SphericalHarmonic pm(make_point_mass(), eop);
  const auto a_pm = pm.acceleration(t, x);
  const auto a_j2 = a - a_pm;
  const double r = x.r.norm();
  const double mag_ref = 3.0 * kJ2 * (kMuEarth / (r * r)) * std::pow(kReEarth / r, 2);
  EXPECT_NEAR(a_j2.norm(), mag_ref, mag_ref * 5e-2) << "actual=" << a_j2.transpose();
  EXPECT_GT(a_j2.z(), 0.0);
}

TEST(SphericalHarmonic, AnalyticalPartialsAgreeWithFD) {
  // The analytical Cunningham gradient (Phase-1A §C2) must agree with a
  // central-difference oracle on the j2-only model.
  const auto eop = make_flat_eop();
  af::SphericalHarmonic sh(make_j2_only(), eop);
  afr::State<afr::tags::ICRF> x;
  x.r << 7.0e6, 1.0e6, 5.0e5;
  const auto t = at::Time<at::tags::TT>{2451545.0, 0.0};
  const auto J = sh.partials_dadx(t, x);
  // h=10 m oracle (large enough to dominate over h^3 truncation, small
  // enough to keep round-off out of the dominant J2 partials at LEO).
  constexpr double h = 10.0;
  for (int i = 0; i < 3; ++i) {
    auto xp = x;
    auto xm = x;
    xp.r[i] += h;
    xm.r[i] -= h;
    const auto ap = sh.acceleration(t, xp);
    const auto am = sh.acceleration(t, xm);
    const auto col = (ap - am) / (2.0 * h);
    for (int row = 0; row < 3; ++row) {
      EXPECT_NEAR(J(row, i), col[row], 1e-7) << "row=" << row << " col=" << i;
    }
  }
}

TEST(SphericalHarmonic, C22RotationObservable) {
  // Phase-1A §C1 rotation observable. The C_{2,2} term has body-fixed
  // longitude dependence cos(2 lambda); evaluating the same ICRF position
  // at two TT epochs separated by ~6 h must produce different perturbation
  // vectors because Earth has rotated ~90° underneath. The Phase 1
  // "body-fixed and ICRF aligned" shortcut would give the same vector at
  // both epochs, so this test fails on that implementation.
  const auto eop = make_flat_eop();
  af::SphericalHarmonic sh22(make_c22_only(/*c22_normalised=*/1e-3), eop);
  afr::State<afr::tags::ICRF> x;
  // Point near the equator (where C_{2,2} is largest). Use ICRF +x,
  // 700 km altitude; in body-fixed at J2000 this lies near a particular
  // longitude, which rotates ~90° in 6 sidereal hours.
  x.r << kReEarth + 700e3, 0.0, 0.0;

  const auto t0 = at::Time<at::tags::TT>{2451545.0, 0.0};
  const auto t1 = at::Time<at::tags::TT>{2451545.0, 6.0 * 3600.0};  // +6 h

  const auto a0 = sh22.acceleration(t0, x);
  const auto a1 = sh22.acceleration(t1, x);

  // At equator, C_{2,2} contributes ~3 mu R^2/r^4 amplitude order. For our
  // contrived 1e-3 coefficient this is ~3e-3 m/s^2 — easily distinguishable
  // from numerical zero. The two epochs must produce *different* vectors.
  const double da = (a0 - a1).norm();
  EXPECT_GT(da, 1e-5) << "C22 acceleration unchanged across 6h ICRF<->ITRS rotation; "
                      << "this is the Phase 1 'body-fixed=ICRF' shortcut leaking through. "
                      << "a0=" << a0.transpose() << " a1=" << a1.transpose();
}

}  // namespace
