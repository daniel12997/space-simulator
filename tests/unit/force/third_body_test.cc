// Copyright 2026 Apsis Contributors
// SPDX-License-Identifier: Apache-2.0
//
// requirements: REQ-PHY-005, REQ-PHY-016
//
// Phase-1 §5: ThirdBody unit test using a stub IEphemeris that returns a
// constant Solar-System geometry. Verifies:
//   * Acceleration vanishes when the spacecraft sits at the central body
//     (r = 0): the stable Battin form gives zero rather than NaN.
//   * Magnitude is consistent with a hand-computed value for a small
//     spacecraft offset.

#include <gtest/gtest.h>

#include <cmath>

#include "apsis/ephemeris/iephemeris.h"
#include "apsis/force/third_body.h"

namespace af = apsis::force;
namespace at = apsis::time;
namespace afr = apsis::frames;
namespace ae = apsis::ephemeris;

namespace {

// Stub: planet at fixed inertial offset.
class StubEphem : public ae::IEphemeris {
 public:
  StubEphem(int third_id, const apsis::math::Vec3& r_third, int central_id,
            const apsis::math::Vec3& r_central)
      : third_id_(third_id), r_third_(r_third), central_id_(central_id), r_central_(r_central) {}

  afr::State<afr::tags::ICRF> state(int body, at::Time<at::tags::TDB>) const override {
    afr::State<afr::tags::ICRF> s;
    if (body == third_id_)
      s.r = r_third_;
    else if (body == central_id_)
      s.r = r_central_;
    return s;
  }

 private:
  int third_id_;
  apsis::math::Vec3 r_third_;
  int central_id_;
  apsis::math::Vec3 r_central_;
};

constexpr double kAU = 1.495978707e11;       // m
constexpr double kMuSun = 1.32712440018e20;  // m^3/s^2

TEST(ThirdBody, AccelerationVanishesAtCentralBody) {
  StubEphem ephem(/*third=*/10, apsis::math::Vec3(kAU, 0.0, 0.0),
                  /*central=*/399, apsis::math::Vec3::Zero());
  af::ThirdBody tb(&ephem, /*central=*/399, /*third=*/10, kMuSun);
  afr::State<afr::tags::ICRF> x;  // r = 0
  const auto a = tb.acceleration(at::Time<at::tags::TT>{}, x);
  EXPECT_LT(a.norm(), 1e-10) << "Expected ~0 at central body, got " << a.transpose();
}

TEST(ThirdBody, AccelerationOrderOfMagnitude) {
  StubEphem ephem(/*third=*/10, apsis::math::Vec3(kAU, 0.0, 0.0),
                  /*central=*/399, apsis::math::Vec3::Zero());
  af::ThirdBody tb(&ephem, /*central=*/399, /*third=*/10, kMuSun);
  // Spacecraft 7000 km from Earth's centre, in the Earth->Sun direction.
  afr::State<afr::tags::ICRF> x;
  x.r << 7.0e6, 0.0, 0.0;
  const auto a = tb.acceleration(at::Time<at::tags::TT>{}, x);
  // For the Battin form (effective acceleration on the spacecraft relative
  // to Earth), the leading term on the radial-aligned point is
  //   |a| ≈ mu_sun * |r| / d^3 * (factor in [2, 4] depending on geometry).
  // We assert it's within an order of magnitude of the tidal scale.
  const double tidal_mag = kMuSun * x.r.norm() / std::pow(kAU, 3);
  EXPECT_GT(a.norm(), 0.5 * tidal_mag);
  EXPECT_LT(a.norm(), 5.0 * tidal_mag);
}

TEST(ThirdBody, PartialsSelfConsistent) {
  StubEphem ephem(/*third=*/10, apsis::math::Vec3(kAU, 0.0, 0.0),
                  /*central=*/399, apsis::math::Vec3::Zero());
  af::ThirdBody tb(&ephem, /*central=*/399, /*third=*/10, kMuSun);
  afr::State<afr::tags::ICRF> x;
  x.r << 7.0e6, 1.0e6, 5.0e5;
  const auto J = tb.partials(at::Time<at::tags::TT>{}, x);
  // Independent oracle at h = 10 m.
  constexpr double h = 10.0;
  for (int i = 0; i < 3; ++i) {
    auto xp = x;
    auto xm = x;
    xp.r[i] += h;
    xm.r[i] -= h;
    const auto ap = tb.acceleration(at::Time<at::tags::TT>{}, xp);
    const auto am = tb.acceleration(at::Time<at::tags::TT>{}, xm);
    const auto col = (ap - am) / (2.0 * h);
    for (int row = 0; row < 3; ++row) {
      EXPECT_NEAR(J(row, i), col[row], 1e-13) << "row=" << row << " col=" << i;
    }
  }
}

}  // namespace
